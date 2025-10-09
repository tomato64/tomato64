/*

	Minimal MatrixSSL Helper
	Copyright (C) 2006-2009 Jonathan Zarate

	Licensed under GNU GPL v2 or later.

*/
#ifndef __MSSL_H__
#define __MSSL_H__

extern FILE *ssl_server_fopen(int sd);
extern FILE *ssl_client_fopen(int sd);
extern FILE *ssl_client_fopen_name(int sd, const char *name);
extern int mssl_init(char *cert, char *priv);
extern int mssl_init_ex(char *cert, char *priv, char *ciphers);
extern void mssl_ctx_free();
extern int mssl_cert_key_match(const char *cert_path, const char *key_path);

#ifdef TTYD_PROXY
extern void *mssl_get_ssl(FILE *fp);
#endif /* TTYD_PROXY */

#endif
