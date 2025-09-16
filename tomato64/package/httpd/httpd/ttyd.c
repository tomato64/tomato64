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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/socket.h>
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
#define WS_BUFFER_SIZE 4096
#define MAX_WS_ROUTES 8

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
static ssize_t conn_read(int connfd, FILE *connfp, int do_ssl, char *buffer, size_t len)
{
	if (do_ssl) {
#ifdef TCONFIG_HTTPS
		SSL *ssl = (SSL *)mssl_get_ssl(connfp);
		if (!ssl) {
			logmsg(LOG_ERR, "Failed to get SSL from connfp");
			return -1;
		}
		return SSL_read(ssl, buffer, len);
#else
		return -1;
#endif
	}
	return read(connfd, buffer, len);
}

static ssize_t conn_write(int connfd, FILE *connfp, int do_ssl, const char *buffer, size_t len)
{
	if (do_ssl) {
#ifdef TCONFIG_HTTPS
		SSL *ssl = (SSL *)mssl_get_ssl(connfp);
		if (!ssl) {
			logmsg(LOG_ERR, "Failed to get SSL from connfp");
			return -1;
		}
		return SSL_write(ssl, buffer, len);
#else
		return -1;
#endif
	}
	return write(connfd, buffer, len);
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

	snprintf(request, sizeof(request),
		"GET %s HTTP/1.1\r\n"  // Use subpath, not the full path
		"Host: localhost\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"Sec-WebSocket-Key: %s\r\n"
		"Sec-WebSocket-Protocol: tty\r\n"
		"\r\n",
		subpath,  // Changed from 'path' to 'subpath'
		ws_key ? ws_key : "dGhlIHNhbXBsZSBub25jZQ==");

	logmsg(LOG_DEBUG, "*** %s: Sending request to backend", __FUNCTION__);

	if (write(backend_fd, request, strlen(request)) < 0) {
		logmsg(LOG_ERR, "Failed to write to backend: %m");
		close(backend_fd);
		return -1;
	}

	logmsg(LOG_DEBUG, "*** %s: Waiting for handshake response from backend", __FUNCTION__);

	// Wait for response to be available
	struct pollfd wait_fd;
	wait_fd.fd = backend_fd;
	wait_fd.events = POLLIN;

	ret = poll(&wait_fd, 1, 5000); // 5 second timeout
	if (ret <= 0) {
		logmsg(LOG_ERR, "Timeout or error waiting for backend response");
		close(backend_fd);
		return -1;
	}

	// Now read the handshake response
	int total_read = 0;
	int headers_complete = 0;
	while (total_read < sizeof(buffer) - 1) {
		n = read(backend_fd, buffer + total_read, sizeof(buffer) - total_read - 1);
		if (n <= 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// No more data available, check if we have complete headers
				break;
			}
			logmsg(LOG_ERR, "Failed to read handshake response: %m");
			close(backend_fd);
			return -1;
		}

		total_read += n;
		buffer[total_read] = '\0';

		if (strstr(buffer, "\r\n\r\n")) {
			headers_complete = 1;
			break;
		}
	}

	if (!headers_complete) {
		logmsg(LOG_ERR, "Incomplete handshake response from backend");
		close(backend_fd);
		return -1;
	}

	// FIX: Normalize the Upgrade header - change "WebSocket" to "websocket"
	char *upgrade_pos = strcasestr(buffer, "Upgrade:");
	if (upgrade_pos) {
		char *ws_pos = strstr(upgrade_pos, "WebSocket");
		if (!ws_pos) {
			ws_pos = strstr(upgrade_pos, "webSocket");  // Catch if we only fixed the W
		}
		if (ws_pos) {
			// Change to all lowercase "websocket"
			ws_pos[0] = 'w';
			ws_pos[1] = 'e';
			ws_pos[2] = 'b';
			ws_pos[3] = 's';
			ws_pos[4] = 'o';
			ws_pos[5] = 'c';
			ws_pos[6] = 'k';
			ws_pos[7] = 'e';
			ws_pos[8] = 't';
			logmsg(LOG_DEBUG, "*** Fixed Upgrade header case");
		}
	}

	// NEW: Check if Sec-WebSocket-Protocol header is missing and add it if needed
	if (!strcasestr(buffer, "Sec-WebSocket-Protocol:")) {
		// The protocol header is missing - we need to add it
		// Find the end of headers (the \r\n\r\n)
		char *end_of_headers = strstr(buffer, "\r\n\r\n");
		if (end_of_headers) {
			// Insert "Sec-WebSocket-Protocol: tty\r\n" before the final \r\n\r\n
			char temp_buffer[8192];
			int header_len = end_of_headers - buffer;

			// Copy headers up to the final \r\n\r\n
			memcpy(temp_buffer, buffer, header_len);

			// Add the missing protocol header
			strcpy(temp_buffer + header_len, "\r\nSec-WebSocket-Protocol: tty\r\n\r\n");

			// Copy back to buffer
			int new_len = header_len + strlen("\r\nSec-WebSocket-Protocol: tty\r\n\r\n");
			memcpy(buffer, temp_buffer, new_len);
			total_read = new_len;

			logmsg(LOG_DEBUG, "*** Added missing Sec-WebSocket-Protocol header");
		}
	}

	logmsg(LOG_DEBUG, "*** Handshake response: [%.*s]", total_read > 200 ? 200 : total_read, buffer);
	logmsg(LOG_DEBUG, "*** %s: Forwarding handshake to client (%d bytes)", __FUNCTION__, total_read);

	// Get extern globals from httpd.c
	extern int connfd;
	extern FILE *connfp;
	extern int do_ssl;

	if (conn_write(connfd, connfp, do_ssl, buffer, total_read) != total_read) {
		logmsg(LOG_ERR, "Failed to write handshake to client");
		close(backend_fd);
		return -1;
	}

	logmsg(LOG_DEBUG, "*** %s: Handshake complete, starting bidirectional proxy", __FUNCTION__);

	fds[0].fd = connfd;
	fds[0].events = POLLIN;
	fds[1].fd = backend_fd;
	fds[1].events = POLLIN;

	while (1) {
		ret = poll(fds, 2, 30000);
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
			n = conn_read(connfd, connfp, do_ssl, buffer, sizeof(buffer));
			if (n <= 0) {
				logmsg(LOG_DEBUG, "Client disconnected");
				break;
			}

			// Log the raw data BEFORE processing
			char hex_before[256];
			int hex_len = (n < 32) ? n : 32;
			int i;
			for (i = 0; i < hex_len; i++) {
				sprintf(hex_before + (i*3), "%02x ", (unsigned char)buffer[i]);
			}
			logmsg(LOG_DEBUG, "*** RAW Client data: [%s]", hex_before);

			// UNMASK and REFRAME the WebSocket data
			unsigned char *frame = (unsigned char *)buffer;
			if (n >= 6 && (frame[1] & 0x80)) {
				int header_len = 2;
				int payload_len = frame[1] & 0x7F;
				unsigned char new_frame[8192];

				if (payload_len == 126) {
					header_len = 4;
					payload_len = (frame[2] << 8) | frame[3];
				} else if (payload_len == 127) {
					logmsg(LOG_ERR, "Frame too large");
					break;
				}

				unsigned char mask[4];
				memcpy(mask, frame + header_len, 4);

				new_frame[0] = frame[0];

				if (payload_len < 126) {
					new_frame[1] = payload_len;
					for (i = 0; i < payload_len; i++) {
						new_frame[2 + i] = frame[header_len + 4 + i] ^ mask[i % 4];
					}
					n = 2 + payload_len;
				} else {
					new_frame[1] = 126;
					new_frame[2] = frame[2];
					new_frame[3] = frame[3];
					for (i = 0; i < payload_len; i++) {
						new_frame[4 + i] = frame[header_len + 4 + i] ^ mask[i % 4];
					}
					n = 4 + payload_len;
				}

				memcpy(buffer, new_frame, n);

				// Log the data AFTER unmasking
				char hex_after[256];
				hex_len = (n < 32) ? n : 32;
				for (i = 0; i < hex_len; i++) {
					sprintf(hex_after + (i*3), "%02x ", (unsigned char)buffer[i]);
				}
				logmsg(LOG_DEBUG, "*** UNMASKED data: [%s]", hex_after);
			}

			logmsg(LOG_DEBUG, "*** Proxy: Client->Backend %d bytes", n);
			if (write(backend_fd, buffer, n) != n) {
				logmsg(LOG_ERR, "Failed to write to backend");
				break;
			}
		}

		if (fds[1].revents & POLLIN) {
			n = read(backend_fd, buffer, sizeof(buffer));
			if (n <= 0) {
				logmsg(LOG_DEBUG, "Backend disconnected");
				break;
			}

			// Log backend data
			char hex_back[256];
			int hex_len = (n < 32) ? n : 32;
			int i;
			for (i = 0; i < hex_len; i++) {
				sprintf(hex_back + (i*3), "%02x ", (unsigned char)buffer[i]);
			}
			logmsg(LOG_DEBUG, "*** Backend->Client data: [%s]", hex_back);
			logmsg(LOG_DEBUG, "*** Proxy: Backend->Client %d bytes", n);

			if (conn_write(connfd, connfp, do_ssl, buffer, n) != n) {
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

/* Direct Unix socket HTTP request function */
int ttyd_unix_socket_http_request(const char *socket_path, const char *request, char **response, size_t *response_len)
{
	int sock_fd;
	struct sockaddr_un addr;
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
	if (bytes_sent < 0) {
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
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
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

/* Check if path is a console path and handle HTTP request */
int ttyd_handle_http_request(const char *file, char **response, size_t *response_len)
{
	char http_request[512];
	const char *subpath;

	/* Check if this is a console path */
	if (strncmp(file, "console", 7) != 0) {
		return 0;  /* Not a console path */
	}

	/* Extract the path after /console */
	subpath = (strlen(file) > 7) ? file + 7 : "/";

	logmsg(LOG_DEBUG, "*** %s: Serving ttyd HTTP content for path: %s (subpath: %s)",
	       __FUNCTION__, file, subpath);

	/* Build HTTP request with the correct path */
	snprintf(http_request, sizeof(http_request),
	         "GET %s HTTP/1.0\r\nHost: localhost\r\n\r\n", subpath);

	/* Make the Unix socket request */
	if (ttyd_unix_socket_http_request("/var/run/ttyd.sock", http_request, response, response_len) == 0) {
		return 1;  /* Successfully handled */
	}

	return -1;  /* Error */
}

/* Initialize WebSocket support */
int ttyd_init(void)
{
	/* Add default routes - customize these as needed */
	add_ws_route("/console", "/var/run/ttyd.sock", 1);  /* ttyd console via unix socket */
	add_ws_route("/console/ws", "/var/run/ttyd.sock", 1);  /* WebSocket endpoint */
	add_ws_route("/console/token", "/var/run/ttyd.sock", 1); /* Token endpoint */

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
