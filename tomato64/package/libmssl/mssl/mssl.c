/*

	Minimal CyaSSL/OpenSSL Helper
	Copyright (C) 2006-2009 Jonathan Zarate
	Copyright (C) 2010 Fedor Kozhevnikov

	Licensed under GNU GPL v2 or later

*/


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef USE_OPENSSL
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#else
/* CyaSSL */
#include <cyassl_error.h>
#endif

#include "shared.h"

/* refer https://ssl-config.mozilla.org/ w/o DES ciphers */
#ifdef USE_OPENSSL
#define SERVER_CIPHERS "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA256:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:!DSS"
#else
#define SERVER_CIPHERS "ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA:ECDHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA256:DHE-RSA-AES256-SHA:ECDHE-ECDSA-DES-CBC3-SHA:ECDHE-RSA-DES-CBC3-SHA:EDH-RSA-DES-CBC3-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:DES-CBC3-SHA:!DSS"
#endif

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
/* use reasonable defaults */
#define CLIENT_CIPHERS NULL
#else
#define CLIENT_CIPHERS "ALL:!EXPORT:!EXPORT40:!EXPORT56:!aNULL:!LOW:!RC4:@STRENGTH"
#endif

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"mssl_debug"


typedef struct {
	SSL* ssl;
	int sd;
} mssl_cookie_t;

static SSL_CTX* ctx;

static inline void mssl_print_err(SSL* ssl)
{
#ifdef USE_OPENSSL
	ERR_print_errors_fp(stderr);
#else
	logmsg(LOG_DEBUG, "*** [mssl] error %d", ssl ? SSL_get_error(ssl, 0) : -1);
#endif
}

static inline void mssl_cleanup(int err)
{
	if (err) mssl_print_err(NULL);
	SSL_CTX_free(ctx);
	ctx = NULL;
}

static ssize_t mssl_read(void *cookie, char *buf, size_t len)
{
	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);

	mssl_cookie_t *kuki = cookie;
	int total = 0;
	int n, err;

	do {
		n = SSL_read(kuki->ssl, &(buf[total]), len - total);
		logmsg(LOG_DEBUG, "*** [mssl] SSL_read(max=%d) returned %d", len - total, n);

		err = SSL_get_error(kuki->ssl, n);
		switch (err) {
		case SSL_ERROR_NONE:
			total += n;
			break;
		case SSL_ERROR_ZERO_RETURN:
			total += n;
			goto OUT;
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_READ:
			break;
		default:
			logmsg(LOG_DEBUG, "*** [mssl] %s: SSL error %d", __FUNCTION__, err);
			mssl_print_err(kuki->ssl);
			if (total == 0) total = -1;
			goto OUT;
		}
	} while ((len - total > 0) && SSL_pending(kuki->ssl));

OUT:
	logmsg(LOG_DEBUG, "*** [mssl] %s: returns %d", __FUNCTION__, total);
	return total;
}

static ssize_t mssl_write(void *cookie, const char *buf, size_t len)
{
	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);

	mssl_cookie_t *kuki = cookie;
	unsigned int total = 0;
	int n, err;

	while (total < len) {
		n = SSL_write(kuki->ssl, &(buf[total]), len - total);
		logmsg(LOG_DEBUG, "*** [mssl] SSL_write(max=%d) returned %d", len - total, n);
		err = SSL_get_error(kuki->ssl, n);
		switch (err) {
		case SSL_ERROR_NONE:
			total += n;
			break;
		case SSL_ERROR_ZERO_RETURN:
			total += n;
			goto OUT;
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_READ:
			break;
		default:
			logmsg(LOG_DEBUG, "*** [mssl] %s: SSL error %d", __FUNCTION__, err);
			mssl_print_err(kuki->ssl);
			if (total == 0) total = -1;
			goto OUT;
		}
	}

OUT:
	logmsg(LOG_DEBUG, "*** [mssl] %s returns %d", __FUNCTION__, total);
	return total;
}

static int mssl_seek(void *cookie, off64_t *pos, int whence)
{
	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);
	errno = EIO;
	return -1;
}

static int mssl_close(void *cookie)
{
	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);

	mssl_cookie_t *kuki = cookie;
	if (!kuki) return 0;

	if (kuki->ssl) {
		SSL_shutdown(kuki->ssl);
		SSL_free(kuki->ssl);
	}

	free(kuki);
	return 0;
}

static const cookie_io_functions_t mssl = {
	mssl_read, mssl_write, mssl_seek, mssl_close
};

static FILE *_ssl_fopen(int sd, int client, const char *name)
{
	int r = 0;
	mssl_cookie_t *kuki;
	FILE *f;

	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);

	if ((kuki = calloc(1, sizeof(*kuki))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	kuki->sd = sd;

	/* Create new SSL object */
	if ((kuki->ssl = SSL_new(ctx)) == NULL) {
		logmsg(LOG_DEBUG, "*** [mssl] %s: SSL_new failed", __FUNCTION__);
		goto ERROR;
	}

	/* SSL structure for client authenticate after SSL_new() */
#ifdef USE_OPENSSL
	SSL_set_verify(kuki->ssl, SSL_VERIFY_NONE, NULL);
	SSL_set_mode(kuki->ssl, SSL_MODE_AUTO_RETRY);

	if (client) {
		/* Setup SNI */
#if OPENSSL_VERSION_NUMBER >= 0x0090806fL && !defined(OPENSSL_NO_TLSEXT)
		if (name && *name) {
			struct addrinfo *res, hint = { .ai_flags = AI_NUMERICHOST };
			if (getaddrinfo(name, NULL, &hint, &res) == 0)
				freeaddrinfo(res);
			else if (SSL_set_tlsext_host_name(kuki->ssl, name) != 1) {
				logmsg(LOG_DEBUG, "*** [mssl] %s: SSL_set_tlsext_host_name failed", __FUNCTION__);
				mssl_print_err(kuki->ssl);
				goto ERROR;
			}
		}
#endif
	}
#endif
	/* Bind the socket to SSL structure
	 * kuki->ssl : SSL structure
	 * kuki->sd  : socket_fd
	 */
	r = SSL_set_fd(kuki->ssl, kuki->sd);

	if (!client) {
		/* Do the SSL Handshake */
		r = SSL_accept(kuki->ssl);
	} else {
		/* Connect to the server, SSL layer */
		r = SSL_connect(kuki->ssl);
	}
	/* r = 0 show unknown CA, but we don't have any CA, so ignore */
	if (r < 0) {
		/* Check error in connect or accept */
		logmsg(LOG_DEBUG, "*** [mssl] %s: SSL_%s failed", __FUNCTION__, (client ? "connect" : "accept"));
		mssl_print_err(kuki->ssl);
		goto ERROR;
	}

#ifdef USE_OPENSSL
	logmsg(LOG_DEBUG, "*** [mssl] SSL connection using %s cipher", SSL_get_cipher(kuki->ssl));
#endif

	if ((f = fopencookie(kuki, "r+", mssl)) == NULL) {
		logmsg(LOG_DEBUG, "*** [mssl] %s: fopencookie failed", __FUNCTION__);
		goto ERROR;
	}

	logmsg(LOG_DEBUG, "*** [mssl] %s success", __FUNCTION__);
	return f;

ERROR:
	mssl_close(kuki);
	return NULL;
}

FILE *ssl_server_fopen(int sd)
{
	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);
	return _ssl_fopen(sd, 0, NULL);
}

FILE *ssl_client_fopen(int sd)
{
	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);
	return _ssl_fopen(sd, 1, NULL);
}

FILE *ssl_client_fopen_name(int sd, const char *name)
{
	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);
	return _ssl_fopen(sd, 1, name);
}

#ifndef SSL_OP_NO_RENEGOTIATION
#if OPENSSL_VERSION_NUMBER < 0x10100000L && defined(SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS)
static void ssl_info_cb(const SSL *ssl, int where, int ret)
{
	if ((where & SSL_CB_HANDSHAKE_DONE) != 0 && SSL_is_server((SSL *) ssl)) {
		/* disable renegotiation (CVE-2009-3555) */
		ssl->s3->flags |= SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS;
	}
}
#endif
#endif

int mssl_init_ex(char *cert, char *priv, char *ciphers)
{
	int server;

	logmsg(LOG_DEBUG, "*** [mssl] %s IN", __FUNCTION__);

	server = (cert != NULL);

#ifdef USE_OPENSSL
	/* Register error strings for libcrypto and libssl functions */
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
#endif

	/* Create the new CTX with the method 
	 * If server=1, use TLSv1_server_method() or SSLv23_server_method()
	 * else 	use TLSv1_client_method() or SSLv23_client_method()
	 */
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	ctx = SSL_CTX_new(server ? TLS_server_method() : TLS_client_method());
#else
	ctx = SSL_CTX_new(server ? SSLv23_server_method() : SSLv23_client_method());
#endif
	if (!ctx) {
		logmsg(LOG_DEBUG, "*** [mssl] SSL_CTX_new() failed");
		mssl_print_err(NULL);
		return 0;
	}

#ifdef USE_OPENSSL
	/* Setup common modes */
	SSL_CTX_set_mode(ctx,
#ifdef SSL_MODE_RELEASE_BUFFERS
				 SSL_MODE_RELEASE_BUFFERS |
#endif
				 SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	/* Setup common options */
	SSL_CTX_set_options(ctx, SSL_OP_ALL |
#ifdef SSL_OP_NO_TICKET
				 SSL_OP_NO_TICKET |
#endif
#ifdef SSL_OP_NO_COMPRESSION
				 SSL_OP_NO_COMPRESSION |
#endif
#ifdef SSL_OP_SINGLE_DH_USE
				 SSL_OP_SINGLE_DH_USE |
#endif
				 SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif

	/* Setup EC support */
#ifdef NID_X9_62_prime256v1
	EC_KEY *ecdh = NULL;
	if ((ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1)) != NULL) {
		SSL_CTX_set_tmp_ecdh(ctx, ecdh);
		EC_KEY_free(ecdh);
#ifdef SSL_OP_SINGLE_ECDH_USE
		SSL_CTX_set_options(ctx, SSL_OP_SINGLE_ECDH_USE);
#endif
	}
#endif

	/* Setup available ciphers */
	ciphers = server ? SERVER_CIPHERS : CLIENT_CIPHERS;
	if (ciphers && SSL_CTX_set_cipher_list(ctx, ciphers) != 1) {
		logmsg(LOG_DEBUG, "*** [mssl] %s: SSL_CTX_set_cipher_list failed", __FUNCTION__);
		mssl_cleanup(1);
		return 0;
	}

	if (server) {
#ifdef USE_OPENSSL11
		/* Disable TLS 1.0 & 1.1 */
		SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#endif
#ifdef USE_OPENSSL
		/* Enforce server cipher order */
		SSL_CTX_set_options(ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
#endif
		/* Set the certificate to be used */
		logmsg(LOG_DEBUG, "*** [mssl] SSL_CTX_use_certificate_chain_file(%s)", cert);
		if (SSL_CTX_use_certificate_chain_file(ctx, cert) <= 0) {
			logmsg(LOG_DEBUG, "*** [mssl] SSL_CTX_use_certificate_chain_file() failed");
			mssl_cleanup(1);
			return 0;
		}
		/* Indicate the key file to be used */
		logmsg(LOG_DEBUG, "*** [mssl] SSL_CTX_use_PrivateKey_file(%s)", priv);
		if (SSL_CTX_use_PrivateKey_file(ctx, priv, SSL_FILETYPE_PEM) <= 0) {
			logmsg(LOG_DEBUG, "*** [mssl] SSL_CTX_use_PrivateKey_file() failed");
			mssl_cleanup(1);
			return 0;
		}
#ifdef USE_OPENSSL
		/* Make sure the key and certificate file match */
		if (!SSL_CTX_check_private_key(ctx)) {
			logmsg(LOG_DEBUG, "*** [mssl] Private key does not match the certificate public key");
			mssl_cleanup(0);
			return 0;
		}
#endif

		/* Disable renegotiation */
#ifdef SSL_OP_NO_RENGOTIATION
		SSL_CTX_set_options(ctx, SSL_OP_NO_RENGOTIATION);
#elif OPENSSL_VERSION_NUMBER < 0x10100000L && defined(SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS)
		SSL_CTX_set_info_callback(ctx, ssl_info_cb);
#endif
	}

	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

	logmsg(LOG_DEBUG, "*** [mssl] %s success", __FUNCTION__);

	return 1;
}

int mssl_init(char *cert, char *priv)
{
	return mssl_init_ex(cert, priv, NULL);
}

#ifdef USE_OPENSSL11
static int mssl_f_exists(const char *path)
{
	struct stat st;
	return (stat(path, &st) == 0) && (!S_ISDIR(st.st_mode));
}

/*
 * compare the modulus of public key and private key
 */
int mssl_cert_key_match(const char *cert_path, const char *key_path)
{
	FILE *fp;
	X509 *x509data = NULL;
	EVP_PKEY *pkey = NULL;
	RSA *rsa_pub = NULL;
	RSA *rsa_pri = NULL;
	DSA *dsa_pub = NULL;
	DSA *dsa_pri = NULL;
	EC_KEY *ec_pub = NULL;
	EC_KEY *ec_pri = NULL;
	const EC_GROUP *ec_group = NULL;
	const EC_POINT *ec_pub_pub = NULL;
	const EC_POINT *ec_pri_pub = NULL;

	int pem = 1;
	int ret = 0;

	if (!mssl_f_exists(cert_path) || !mssl_f_exists(key_path))
		return 0;

	/* get x509 from cert file */
	fp = fopen(cert_path, "r");
	if (!fp)
		return 0;

	if (!PEM_read_X509(fp, &x509data, NULL, NULL)) {
		logmsg(LOG_DEBUG, "*** [mssl] Try to read DER format certificate");
		pem = 0;
		fseek(fp, 0, SEEK_SET);
		d2i_X509_fp(fp, &x509data);
	}
	else {
		logmsg(LOG_DEBUG, "*** [mssl] PEM format certificate");
	}

	fclose(fp);
	if (x509data == NULL) {
		logmsg(LOG_DEBUG, "*** [mssl] Load certificate failed");
		ret = 0;
		goto end;
	}

	/* get pubic key from x509 */
	pkey = X509_get_pubkey(x509data);
	if (pkey == NULL) {
		ret = 0;
		goto end;
	}
	X509_free(x509data);
	x509data = NULL;

	if (EVP_PKEY_id(pkey) == EVP_PKEY_RSA)
		rsa_pub = EVP_PKEY_get1_RSA(pkey);
	else if (EVP_PKEY_id(pkey) == EVP_PKEY_DSA)
		dsa_pub = EVP_PKEY_get1_DSA(pkey);
	else if (EVP_PKEY_id(pkey) == EVP_PKEY_EC)
		ec_pub = EVP_PKEY_get1_EC_KEY(pkey);

	EVP_PKEY_free(pkey);
	pkey = NULL;

	/* get private key from key file */
	fp = fopen(key_path, "r");
	if (!fp) {
		ret = 0;
		goto end;
	}

	if (pem)
		pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
	else
		pkey = d2i_PrivateKey_fp(fp, NULL);

	fclose(fp);

	if (pkey == NULL) {
		logmsg(LOG_DEBUG, "*** [mssl] Load private key failed");
		ret = 0;
		goto end;
	}

	if (EVP_PKEY_id(pkey) == EVP_PKEY_RSA)
		rsa_pri = EVP_PKEY_get1_RSA(pkey);
	else if (EVP_PKEY_id(pkey) == EVP_PKEY_DSA)
		dsa_pri = EVP_PKEY_get1_DSA(pkey);
	else if (EVP_PKEY_id(pkey) == EVP_PKEY_EC)
		ec_pri  = EVP_PKEY_get1_EC_KEY(pkey);

	EVP_PKEY_free(pkey);
	pkey = NULL;

	/* compare modulus */
	if (rsa_pub && rsa_pri) {
		if (BN_cmp(RSA_get0_n(rsa_pub), RSA_get0_n(rsa_pri))) {
			logmsg(LOG_DEBUG, "*** [mssl] rsa n not match");
			ret = 0;
		}
		else {
			logmsg(LOG_DEBUG, "*** [mssl] rsa n match");
			ret = 1;
		}
	}
	else if (dsa_pub && dsa_pri) {
		if (BN_cmp(DSA_get0_pub_key(dsa_pub), DSA_get0_pub_key(dsa_pri))) {
			logmsg(LOG_DEBUG, "*** [mssl] dsa modulus not match");
			ret = 0;
		}
		else {
			logmsg(LOG_DEBUG, "*** [mssl] dsa modulus match");
			ret = 1;
		}
	}
	else if (ec_pub && ec_pri) {
		ec_group = EC_KEY_get0_group(ec_pub);
		ec_pub_pub = EC_KEY_get0_public_key(ec_pub);
		ec_pri_pub = EC_KEY_get0_public_key(ec_pri);

		if (ec_group != NULL && ec_pub_pub != NULL && ec_pri_pub != NULL && EC_POINT_cmp(ec_group, ec_pub_pub, ec_pri_pub, NULL) == 0) {
			logmsg(LOG_DEBUG, "*** [mssl] ec modulus match");
			ret = 1;
		}
		else {
			logmsg(LOG_DEBUG, "*** [mssl] ec modulus not match");
			ret = 0;
		}
	}
	else {
		logmsg(LOG_DEBUG, "*** [mssl] compare failed");
	}

end:
	if (x509data)
		X509_free(x509data);
	if (pkey)
		EVP_PKEY_free(pkey);
	if (rsa_pub)
		RSA_free(rsa_pub);
	if (dsa_pub)
		DSA_free(dsa_pub);
	if (ec_pub)
		EC_KEY_free(ec_pub);
	if (rsa_pri)
		RSA_free(rsa_pri);
	if (dsa_pri)
		DSA_free(dsa_pri);
	if (ec_pri)
		EC_KEY_free(ec_pri);

	return ret;
}
#endif /* USE_OPENSSL11 */
