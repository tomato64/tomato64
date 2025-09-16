/*
 * ttyd.h - WebSocket proxy support for ttyd console
 *
 * This file is part of Tomato Firmware
 */

#ifndef __TTYD_H__
#define __TTYD_H__

#ifdef TTYD_PROXY

/* Initialize WebSocket proxy support */
int ttyd_init(void);

/* Cleanup WebSocket proxy support */
void ttyd_cleanup(void);

/* Check if request is a WebSocket upgrade request */
int ttyd_is_websocket_upgrade(const char *upgrade_header, const char *connection_header);

/* Check if a WebSocket route exists for the given path
 * Returns 1 if route exists and auth_required is set (via pointer), 0 if no route
 */
int ttyd_has_route(const char *path, int *auth_required);

/* Handle WebSocket upgrade and proxy the connection
 * Returns 0 on success, -1 on error
 * Requires: connfd, connfp, do_ssl to be set (extern globals from httpd)
 */
int ttyd_handle_websocket_upgrade(const char *path, const char *ws_key);

/* Direct Unix socket HTTP request function
 * Returns 0 on success, -1 on error
 * response and response_len are output parameters (caller must free response)
 */
int ttyd_unix_socket_http_request(const char *socket_path, const char *request, char **response, size_t *response_len);

/* Check if path is a console path and handle HTTP request
 * Returns 1 if handled (with response in output params), 0 if not a console path, -1 on error
 * response and response_len are output parameters (caller must free response if return is 1)
 */
int ttyd_handle_http_request(const char *file, char **response, size_t *response_len);

#endif /* TTYD_PROXY */

#endif /* __TTYD_H__ */
