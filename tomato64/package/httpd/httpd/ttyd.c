/*
 * ttyd.c - WebSocket proxy support for ttyd console
 *
 * This file is part of Tomato Firmware
 */

#ifdef TTYD_PROXY

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>
#include <ctype.h>
#include <syslog.h>

#include "tomato.h"
#include "httpd.h"

#ifdef TCONFIG_HTTPS
 #ifdef USE_OPENSSL
  #include <openssl/ssl.h>
 #endif
#include "mssl.h"
#endif

/* needed by logmsg() */
#define LOGMSG_DISABLE		0
#define LOGMSG_NVDEBUG		"httpd_debug"

/* WebSocket configuration */
#define MAX_WS_ROUTES		8
#define TTYD_SOCKET		"/var/run/ttyd.sock"
#define BACKEND_TIMEOUT_MS	5000	/* handshake, console page fetch, stalled writes */
#define PROXY_IDLE_MS		30000	/* ttyd pings every 5s, so this only trips on a dead peer */

/* WebSocket route configuration */
struct ws_route {
	char *path;           /* WebSocket endpoint path (e.g., "/console") */
	char *socket_path;    /* Unix socket path */
	int auth_required;    /* 1 if authentication required, 0 otherwise */
};

static struct ws_route ws_routes[MAX_WS_ROUTES];
static int ws_route_count = 0;

/* Forward declarations */
static int connect_to_backend(struct ws_route *route);
static struct ws_route *find_ws_route(const char *path);

/* Add a WebSocket route */
static int add_ws_route(const char *path, const char *socket_path, int auth_required)
{
	if (ws_route_count >= MAX_WS_ROUTES) {
		logmsg(LOG_ERR, "Maximum WebSocket routes (%d) exceeded", MAX_WS_ROUTES);
		return -1;
	}

	struct ws_route *route = &ws_routes[ws_route_count];
	memset(route, 0, sizeof(*route));

	route->path = strdup(path);
	route->socket_path = strdup(socket_path);
	route->auth_required = auth_required;

	ws_route_count++;
	logmsg(LOG_DEBUG, "Added WebSocket route: %s -> %s", path, socket_path);

	return 0;
}

/* Find WebSocket route by path */
static struct ws_route *find_ws_route(const char *path)
{
	int i;
	for (i = 0; i < ws_route_count; i++) {
		if (strcmp(ws_routes[i].path, path) == 0) {
			return &ws_routes[i];
		}
	}
	return NULL;
}

/* Connect to backend server */
static int connect_to_backend(struct ws_route *route)
{
	int fd;
	struct sockaddr_un addr;

	/* Unix socket connection */
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		logmsg(LOG_ERR, "Failed to create unix socket: %m");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, route->socket_path, sizeof(addr.sun_path) - 1);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		logmsg(LOG_ERR, "Failed to connect to unix socket %s: %m", route->socket_path);
		close(fd);
		return -1;
	}

	/* Set non-blocking */
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	return fd;
}

/* SSL-aware I/O wrappers for WebSocket proxy */
#ifdef TCONFIG_HTTPS
static SSL *conn_ssl(FILE *connfp)
{
	SSL *ssl = (SSL *)mssl_get_ssl(connfp);

	if (!ssl)
		logmsg(LOG_ERR, "Failed to get SSL from connfp");

	return ssl;
}
#endif

static ssize_t conn_read(int connfd, FILE *connfp, int do_ssl, char *buffer, size_t len)
{
	if (do_ssl) {
#ifdef TCONFIG_HTTPS
		SSL *ssl = conn_ssl(connfp);
		if (!ssl)
			return -1;

		return SSL_read(ssl, buffer, len);
#else
		return -1;
#endif
	}
	return read(connfd, buffer, len);
}

/*
 * Plaintext OpenSSL already holds for this connection. poll() on the socket
 * can't see it, so it has to be drained before going back to poll().
 */
static int conn_pending(FILE *connfp, int do_ssl)
{
#ifdef TCONFIG_HTTPS
	if (do_ssl) {
		SSL *ssl = (SSL *)mssl_get_ssl(connfp);
		return ssl ? SSL_pending(ssl) : 0;
	}
#endif
	return 0;
}

/* Write all of buffer to the client; returns 0 on success, -1 on error */
static int conn_write_all(int connfd, FILE *connfp, int do_ssl, const char *buffer, size_t len)
{
	ssize_t n;

	while (len > 0) {
		if (do_ssl) {
#ifdef TCONFIG_HTTPS
			SSL *ssl = conn_ssl(connfp);
			if (!ssl)
				return -1;

			n = SSL_write(ssl, buffer, len);
			if (n <= 0)
				return -1;
#else
			return -1;
#endif
		}
		else {
			n = write(connfd, buffer, len);
			if (n < 0) {
				if (errno == EINTR)
					continue;

				return -1;
			}
		}
		buffer += n;
		len -= n;
	}

	return 0;
}

/*
 * Write all of buffer to the (non-blocking) backend socket, waiting for room
 * when ttyd falls behind. Returns 0 on success, -1 on error or timeout.
 */
static int backend_write_all(int fd, const char *buffer, size_t len)
{
	struct pollfd pfd;
	ssize_t n;
	int ret;

	while (len > 0) {
		n = write(fd, buffer, len);
		if (n > 0) {
			buffer += n;
			len -= n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
			return -1;

		pfd.fd = fd;
		pfd.events = POLLOUT;
		do {
			ret = poll(&pfd, 1, BACKEND_TIMEOUT_MS);
		} while (ret < 0 && errno == EINTR);

		if (ret <= 0 || (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)))
			return -1;
	}

	return 0;
}

/*
 * Read the backend's handshake response into buffer until the end of the
 * headers. The non-blocking socket may deliver it in pieces, so keep polling
 * until the headers are complete or BACKEND_TIMEOUT_MS has passed.
 * Returns the number of bytes read (may include bytes past the headers), or -1.
 */
static int backend_read_handshake(int fd, char *buffer, size_t size)
{
	struct pollfd pfd;
	struct timeval start, now;
	int total = 0, remaining, ret;
	ssize_t n;

	gettimeofday(&start, NULL);

	while (total < (int)size - 1) {
		gettimeofday(&now, NULL);
		remaining = BACKEND_TIMEOUT_MS - (int)((now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec - start.tv_usec) / 1000);
		if (remaining <= 0) {
			logmsg(LOG_ERR, "Timeout waiting for backend handshake response");
			return -1;
		}

		pfd.fd = fd;
		pfd.events = POLLIN;
		ret = poll(&pfd, 1, remaining);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			logmsg(LOG_ERR, "poll error waiting for backend: %m");
			return -1;
		}
		if (ret == 0)
			continue; /* timeout is caught at the top of the loop */

		n = read(fd, buffer + total, size - total - 1);
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				continue;

			logmsg(LOG_ERR, "Failed to read handshake response: %m");
			return -1;
		}
		if (n == 0) {
			logmsg(LOG_ERR, "Backend closed during handshake");
			return -1;
		}

		total += n;
		buffer[total] = '\0';

		if (strstr(buffer, "\r\n\r\n"))
			return total;
	}

	logmsg(LOG_ERR, "Backend handshake response too large");
	return -1;
}

/* Handle WebSocket upgrade */
int ttyd_handle_websocket_upgrade(const char *path, const char *ws_key)
{
	struct ws_route *route;
	int backend_fd;
	struct pollfd fds[2];
	char buffer[8192];
	int n, ret;
	char request[4096];

	route = find_ws_route(path);
	if (!route) {
		logmsg(LOG_ERR, "No WebSocket route found for path: %s", path);
		return -1;
	}

	if (!ws_key || !*ws_key) {
		logmsg(LOG_ERR, "WebSocket upgrade without Sec-WebSocket-Key");
		return -1;
	}

	logmsg(LOG_DEBUG, "*** %s: Forwarding WebSocket to backend: %s", __FUNCTION__, path);

	backend_fd = connect_to_backend(route);
	if (backend_fd < 0) {
		return -1;
	}

	// Extract just the subpath after /console
	const char *subpath = path;
	if (strncmp(path, "/console", 8) == 0) {
		subpath = path + 8;  // Skip "/console"
		if (*subpath == '\0')
			subpath = "/";   // If path was exactly "/console", use "/"
	}

	ret = snprintf(request, sizeof(request),
		"GET %s HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Key: %s\r\n"
		"Sec-WebSocket-Protocol: tty\r\n"
		"\r\n",
		subpath, ws_key);
	if (ret < 0 || ret >= (int)sizeof(request)) {
		logmsg(LOG_ERR, "WebSocket upgrade request too long");
		close(backend_fd);
		return -1;
	}

	logmsg(LOG_DEBUG, "*** %s: Sending request to backend", __FUNCTION__);

	if (backend_write_all(backend_fd, request, ret) < 0) {
		logmsg(LOG_ERR, "Failed to write to backend: %m");
		close(backend_fd);
		return -1;
	}

	logmsg(LOG_DEBUG, "*** %s: Waiting for handshake response from backend", __FUNCTION__);

	n = backend_read_handshake(backend_fd, buffer, sizeof(buffer));
	if (n < 0) {
		close(backend_fd);
		return -1;
	}

	logmsg(LOG_DEBUG, "*** Handshake response: [%.*s]", n > 200 ? 200 : n, buffer);
	logmsg(LOG_DEBUG, "*** %s: Forwarding handshake to client (%d bytes)", __FUNCTION__, n);

	// Get extern globals from httpd.c
	extern int connfd;
	extern FILE *connfp;
	extern int do_ssl;

	/*
	 * Forward ttyd's response as-is: it computed Sec-WebSocket-Accept from
	 * the browser's key and selected the "tty" protocol itself. Any bytes
	 * that arrived after the headers are forwarded with it.
	 */
	if (conn_write_all(connfd, connfp, do_ssl, buffer, n) < 0) {
		logmsg(LOG_ERR, "Failed to write handshake to client");
		close(backend_fd);
		return -1;
	}

	/* ttyd refused the upgrade: the client has its answer, nothing to proxy */
	if (strncmp(buffer, "HTTP/1.1 101", 12) != 0) {
		logmsg(LOG_ERR, "Backend refused WebSocket upgrade: [%.*s]", (int)strcspn(buffer, "\r\n"), buffer);
		close(backend_fd);
		return 0;
	}

	logmsg(LOG_DEBUG, "*** %s: Handshake complete, starting bidirectional proxy", __FUNCTION__);

	fds[0].fd = connfd;
	fds[0].events = POLLIN;
	fds[1].fd = backend_fd;
	fds[1].events = POLLIN;

	while (1) {
		ret = poll(fds, 2, PROXY_IDLE_MS);
		if (ret < 0) {
			if (errno == EINTR) continue;
			logmsg(LOG_ERR, "poll error: %m");
			break;
		}
		if (ret == 0) {
			logmsg(LOG_DEBUG, "WebSocket proxy timeout");
			break;
		}

		if (fds[0].revents & POLLIN) {
			/*
			 * Forward the client's bytes untouched. The browser's frames are
			 * masked, and ttyd is the WebSocket server on the other end, so it
			 * expects masked client frames (RFC 6455 5.1). libwebsockets >= 5
			 * closes with 1002 "client unmasked" otherwise. Passing the stream
			 * through also keeps frames intact when one read holds a partial
			 * frame or more than one frame.
			 *
			 * With TLS, one record can hold more than fits in buffer; the rest
			 * sits inside OpenSSL where poll() can't see it, so drain it here.
			 */
			do {
				n = conn_read(connfd, connfp, do_ssl, buffer, sizeof(buffer));
				if (n <= 0)
					break;

				logmsg(LOG_DEBUG, "*** Proxy: Client->Backend %d bytes", n);
				if (backend_write_all(backend_fd, buffer, n) < 0) {
					logmsg(LOG_ERR, "Failed to write to backend");
					n = -1;
					break;
				}
			} while (conn_pending(connfp, do_ssl) > 0);

			if (n <= 0) {
				logmsg(LOG_DEBUG, "Client disconnected");
				break;
			}
		}

		if (fds[1].revents & POLLIN) {
			n = read(backend_fd, buffer, sizeof(buffer));
			if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK))
				continue;
			if (n <= 0) {
				logmsg(LOG_DEBUG, "Backend disconnected");
				break;
			}

			logmsg(LOG_DEBUG, "*** Proxy: Backend->Client %d bytes", n);

			if (conn_write_all(connfd, connfp, do_ssl, buffer, n) < 0) {
				logmsg(LOG_ERR, "Failed to write to client");
				break;
			}
		}

		if ((fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) ||
			(fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))) {
			logmsg(LOG_DEBUG, "Connection error");
			break;
		}
	}

	close(backend_fd);
	logmsg(LOG_DEBUG, "*** %s: WebSocket proxy ended", __FUNCTION__);

	return 0;
}

/* Check if request is WebSocket upgrade */
int ttyd_is_websocket_upgrade(const char *upgrade_header, const char *connection_header)
{
	if (!upgrade_header || !connection_header)
		return 0;

	/* Check if upgrade header contains "websocket" (case insensitive) */
	if (strcasestr(upgrade_header, "websocket") == NULL)
		return 0;

	/* Check if connection header contains "upgrade" (case insensitive) */
	if (strcasestr(connection_header, "upgrade") == NULL)
		return 0;

	logmsg(LOG_DEBUG, "*** WebSocket upgrade detected!");
	return 1;
}

/* Check if a WebSocket route exists for the given path */
int ttyd_has_route(const char *path, int *auth_required)
{
	struct ws_route *route = find_ws_route(path);
	if (route) {
		if (auth_required)
			*auth_required = route->auth_required;
		return 1;
	}
	return 0;
}

/*
 * Cross-site WebSocket hijacking guard. Browsers attach cached Basic auth to
 * WebSocket handshakes started by any page, so a foreign site could otherwise
 * open the console. Browsers always send Origin on WebSocket handshakes and
 * a page can't forge it, so require it to name this same host:port.
 * Returns 1 if allowed, 0 if not.
 */
int ttyd_origin_ok(const char *origin, const char *host)
{
	const char *authority;
	size_t len;

	/* Non-browser clients (no Origin) still have to pass auth */
	if (!origin)
		return 1;

	if (!host || !*host)
		return 0;

	if (strncasecmp(origin, "http://", 7) == 0)
		authority = origin + 7;
	else if (strncasecmp(origin, "https://", 8) == 0)
		authority = origin + 8;
	else
		return 0; /* includes the opaque origin "null" */

	len = strcspn(authority, "/");
	if (len != strlen(host) || strncasecmp(authority, host, len) != 0) {
		logmsg(LOG_WARNING, "console WebSocket rejected: Origin '%s' does not match Host '%s'", origin, host);
		return 0;
	}

	return 1;
}

/* Is file ("path" without the leading '/') the console page or one of its subpaths? */
int ttyd_is_console_path(const char *file)
{
	return (strncmp(file, "console", 7) == 0) && (file[7] == '\0' || file[7] == '/');
}

/* Direct Unix socket HTTP request function */
int ttyd_unix_socket_http_request(const char *socket_path, const char *request, char **response, size_t *response_len)
{
	int sock_fd;
	struct sockaddr_un addr;
	struct timeval tv;
	ssize_t bytes_sent, bytes_received;
	char *buffer = NULL;
	size_t buffer_size = 8192;
	size_t total_received = 0;

	/* Create Unix domain socket */
	sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		logmsg(LOG_ERR, "Failed to create Unix socket: %m");
		return -1;
	}

	/* Don't hang the request forever if ttyd stops answering */
	tv.tv_sec = BACKEND_TIMEOUT_MS / 1000;
	tv.tv_usec = 0;
	setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	/* Set up socket address */
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

	/* Connect to Unix socket */
	if (connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		logmsg(LOG_ERR, "Failed to connect to Unix socket %s: %m", socket_path);
		close(sock_fd);
		return -1;
	}

	/* Send HTTP request */
	bytes_sent = send(sock_fd, request, strlen(request), 0);
	if (bytes_sent != (ssize_t)strlen(request)) {
		logmsg(LOG_ERR, "Failed to send HTTP request: %m");
		close(sock_fd);
		return -1;
	}

	/* Allocate initial buffer */
	buffer = malloc(buffer_size);
	if (!buffer) {
		logmsg(LOG_ERR, "Failed to allocate response buffer");
		close(sock_fd);
		return -1;
	}

	/* Receive response */
	while (1) {
		/* Ensure buffer has space */
		if (total_received >= buffer_size - 1) {
			buffer_size *= 2;
			char *new_buffer = realloc(buffer, buffer_size);
			if (!new_buffer) {
				logmsg(LOG_ERR, "Failed to reallocate response buffer");
				free(buffer);
				close(sock_fd);
				return -1;
			}
			buffer = new_buffer;
		}

		bytes_received = recv(sock_fd, buffer + total_received, buffer_size - total_received - 1, 0);
		if (bytes_received < 0) {
			if (errno == EINTR)
				continue;

			/* EAGAIN here means SO_RCVTIMEO expired */
			logmsg(LOG_ERR, "Failed to receive HTTP response: %m");
			free(buffer);
			close(sock_fd);
			return -1;
		} else if (bytes_received == 0) {
			/* Connection closed */
			break;
		}

		total_received += bytes_received;
	}

	close(sock_fd);

	/* Null terminate */
	buffer[total_received] = '\0';

	*response = buffer;
	*response_len = total_received;

	return 0;
}

/*
 * Fetch a console page from ttyd. The caller must have checked
 * ttyd_is_console_path() and authenticated the client first.
 */
int ttyd_handle_http_request(const char *file, char **response, size_t *response_len)
{
	char http_request[512];
	const char *subpath;
	int ret;

	if (!ttyd_is_console_path(file)) {
		return 0;  /* Not a console path */
	}

	/* Extract the path after /console */
	subpath = (file[7] != '\0') ? file + 7 : "/";

	logmsg(LOG_DEBUG, "*** %s: Serving ttyd HTTP content for path: %s (subpath: %s)",
	       __FUNCTION__, file, subpath);

	/* Build HTTP request with the correct path */
	ret = snprintf(http_request, sizeof(http_request),
	               "GET %s HTTP/1.0\r\nHost: localhost\r\n\r\n", subpath);
	if (ret < 0 || ret >= (int)sizeof(http_request)) {
		return -1;
	}

	/* Make the Unix socket request */
	if (ttyd_unix_socket_http_request(TTYD_SOCKET, http_request, response, response_len) == 0) {
		return 1;  /* Successfully handled */
	}

	return -1;  /* Error */
}

/* Initialize WebSocket support */
int ttyd_init(void)
{
	/* Add default routes - customize these as needed */
	add_ws_route("/console", TTYD_SOCKET, 1);  /* ttyd console via unix socket */
	add_ws_route("/console/ws", TTYD_SOCKET, 1);  /* WebSocket endpoint */
	add_ws_route("/console/token", TTYD_SOCKET, 1); /* Token endpoint */

	logmsg(LOG_DEBUG, "WebSocket proxy support initialized");
	return 0;
}

/* Cleanup WebSocket support */
void ttyd_cleanup(void)
{
	int i;

	/* Free route configurations */
	for (i = 0; i < ws_route_count; i++) {
		if (ws_routes[i].path) {
			free(ws_routes[i].path);
			ws_routes[i].path = NULL;
		}
		if (ws_routes[i].socket_path) {
			free(ws_routes[i].socket_path);
			ws_routes[i].socket_path = NULL;
		}
	}
	ws_route_count = 0;
}

#endif /* TTYD_PROXY */
