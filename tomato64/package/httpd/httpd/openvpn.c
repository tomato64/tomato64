/*
 *
 * FreshTomato Firmware
 * Copyright (C) 2018 Michal Obrembski
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#include "tomato.h"

#include <arpa/inet.h>
#include <wlioctl.h>
#include <wlutils.h>
#ifdef TCONFIG_IPV6
 #include <ifaddrs.h>
#endif

#ifdef TCONFIG_OPENVPN


const char ovpnc_dir[]   = "/tmp/ovpnclientconfig";
const char openssl_dir[] = "/tmp/openssl";

#ifdef TCONFIG_KEYGEN
/*
 * validate domain name contains only safe hostname characters before using it
 * in the certificate subject
 */
static int is_safe_domain_arg(const char *s)
{
	const unsigned char *p;
	size_t len = 0;

	if (s == NULL || *s == '\0' || *s == '-')
		return 0;

	for (p = (const unsigned char *)s; *p != '\0'; ++p) {
		unsigned char c = *p;

		if (++len > 253)
			return 0;

		if (!is_ascii_alnum(c) && c != '.' && c != '-')
			return 0;
	}

	return 1;
}

static void put_to_file(const char *filePath, const char *content)
{
	FILE *fkey;

	if ((fkey = fopen(filePath, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, filePath);
		return;
	}
	fputs(content, fkey);
	fclose(fkey);
}

static char *read_from_file(const char *filePath, char *buf, size_t buf_len)
{
	int datalen;

	datalen = f_read(filePath, buf, buf_len - 1);
	if (datalen < 0)
		buf[0] = '\0';
	else
		buf[datalen] = '\0';

	return buf;
}

static void openssl_eval(char *const argv[])
{
	_eval(argv, ">>/tmp/openssl/openssl.log", 0, NULL);
}

static void openssl_ecparam(const char *out)
{
	char *cmd[9];

	cmd[0] = "openssl";
	cmd[1] = "ecparam";
	cmd[2] = "-genkey";
	cmd[3] = "-name";
	cmd[4] = "prime256v1";
	cmd[5] = "-out";
	cmd[6] = (char *)out;
	cmd[7] = "-noout";
	cmd[8] = NULL;
	openssl_eval(cmd);
}

static void openssl_req(const char *keyopt, const char *key, const char *out, const char *ext, const char *subj, int x509)
{
	char *cmd[16];
	int n;

	n = 0;
	cmd[n++] = "openssl";
	cmd[n++] = "req";
	cmd[n++] = "-new";
	cmd[n++] = "-noenc";
	if (x509) {
		cmd[n++] = "-x509";
		cmd[n++] = "-days";
		cmd[n++] = "3650";
	}
	cmd[n++] = (char *)keyopt;
	cmd[n++] = (char *)key;
	cmd[n++] = "-out";
	cmd[n++] = (char *)out;
	if (ext) {
		cmd[n++] = "-extensions";
		cmd[n++] = (char *)ext;
	}
	cmd[n++] = "-subj";
	cmd[n++] = (char *)subj;
	cmd[n] = NULL;

	openssl_eval(cmd);
}

static void openssl_ca(const char *in, const char *out, const char *ext, const char *subj, int ecdh)
{
	char *cmd[21];
	int n;

	n = 0;
	cmd[n++] = "openssl";
	cmd[n++] = "ca";
	cmd[n++] = "-batch";
	cmd[n++] = "-policy";
	cmd[n++] = "policy_anything";
	cmd[n++] = "-days";
	cmd[n++] = "3650";
	if (ecdh) {
		cmd[n++] = "-notext";
		cmd[n++] = "-keyfile";
		cmd[n++] = "/tmp/openssl/cakey.pem";
		cmd[n++] = "-cert";
		cmd[n++] = "/tmp/openssl/cacert.pem";
	}
	cmd[n++] = "-in";
	cmd[n++] = (char *)in;
	cmd[n++] = "-out";
	cmd[n++] = (char *)out;
	cmd[n++] = "-extensions";
	cmd[n++] = (char *)ext;
	cmd[n++] = "-subj";
	cmd[n++] = (char *)subj;
	cmd[n] = NULL;

	openssl_eval(cmd);
}

static void openssl_x509_pem(const char *file)
{
	char *cmd[11];

	cmd[0] = "openssl";
	cmd[1] = "x509";
	cmd[2] = "-in";
	cmd[3] = (char *)file;
	cmd[4] = "-inform";
	cmd[5] = "PEM";
	cmd[6] = "-out";
	cmd[7] = (char *)file;
	cmd[8] = "-outform";
	cmd[9] = "PEM";
	cmd[10] = NULL;
	openssl_eval(cmd);
}

static void prepareCAGeneration(const int serverNum, const int is_ecdh)
{
	char nvname[32], subj[128], tmp[64];
	char *p;

	eval("rm", "-Rf", (char *)openssl_dir);
	eval("mkdir", "-p", (char *)openssl_dir);

	put_to_file("/tmp/openssl/index.txt", "");
	put_to_file("/tmp/openssl/openssl.log", "");

	snprintf(nvname, sizeof(nvname), "vpns%d_ca_key", serverNum);

	if (nvram_match(nvname, "")) {
		syslog(LOG_WARNING, "No CA KEY was saved for server %d, regenerating ...", serverNum);

		tmp[0] = '\0';
		if ((p = nvram_safe_get("wan_domain")) && *p && is_safe_domain_arg(p))
			snprintf(tmp, sizeof(tmp), ".%s", p);

		snprintf(subj, sizeof(subj), "/C=GB/ST=Yorks/L=York/O=Tomato64/OU=IT/CN=server%s", tmp);

		if (is_ecdh == 1) {
			openssl_ecparam("/tmp/openssl/cakey.pem");
			openssl_req("-key", "/tmp/openssl/cakey.pem", "/tmp/openssl/cacert.pem", "v3_ca", subj, 1);
		}
		else {
			openssl_req("-keyout", "/tmp/openssl/cakey.pem", "/tmp/openssl/cacert.pem", NULL, subj, 1);
		}
	}
	else {
		syslog(LOG_WARNING, "Found CA KEY for server %d, creating from NVRAM", serverNum);
		put_to_file("/tmp/openssl/cakey.pem", getNVRAMVar("vpns%d_ca_key", serverNum));
		put_to_file("/tmp/openssl/cacert.pem", getNVRAMVar("vpns%d_ca", serverNum));
	}
}

static void generateKey(const char *prefix, const int userid, const int is_ecdh)
{
	char subj_buf[160], key[32], csr[32], crt[32], tmp[64], serial[8];
	char *p, *ext;

	if (strncmp(prefix, "server", 6) == 0) {
		ext = "server_cert";
		syslog(LOG_WARNING, "Building Certs for Server");
	}
	else {
		ext = "usr_cert";
		syslog(LOG_WARNING, "Building Certs for Client%d", userid);
	}

	snprintf(serial, sizeof(serial), "%.2X", userid);
	put_to_file("/tmp/openssl/serial", serial);

	snprintf(serial, sizeof(serial), "%d", userid);

	tmp[0] = '\0';
	if ((p = nvram_safe_get("wan_domain")) && *p && is_safe_domain_arg(p))
		snprintf(tmp, sizeof(tmp), ".%s", p);

	snprintf(subj_buf, sizeof(subj_buf), "/C=GB/ST=Yorks/L=York/O=Tomato64/OU=IT/CN=%s%s%s", prefix, (userid > 0 ? serial : ""), tmp);
	snprintf(key, sizeof(key), "%s/%s.key", openssl_dir, prefix);
	snprintf(csr, sizeof(csr), "%s/%s.csr", openssl_dir, prefix);
	snprintf(crt, sizeof(crt), "%s/%s.crt", openssl_dir, prefix);

	if (is_ecdh == 1) {
		openssl_ecparam(key);
		openssl_req("-key", key, csr, ext, subj_buf, 0);
		openssl_ca(csr, crt, ext, subj_buf, 1);
	}
	else {
		openssl_req("-keyout", key, csr, ext, subj_buf, 0);
		openssl_ca(csr, crt, ext, subj_buf, 0);
		openssl_x509_pem(crt);
	}
}

static void print_generated_ca_to_user()
{
	char buffer[32];

	web_puts("cakey = '");

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s/cakey.pem", openssl_dir);
	web_putfile(buffer, WOF_JAVASCRIPT);

	web_puts("';\ncacert = '");

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s/cacert.pem", openssl_dir);
	web_putfile(buffer, WOF_JAVASCRIPT);

	web_puts("';");
}

static void print_generated_keys_to_user(const char *prefix)
{
	char buffer[32];

	web_puts("\ngenerated_crt = '");

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s/%s.crt", openssl_dir, prefix);
	web_putfile(buffer, WOF_JAVASCRIPT);

	web_puts("';\ngenerated_key = '");

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s/%s.key", openssl_dir, prefix);
	web_putfile(buffer, WOF_JAVASCRIPT);

	web_puts("';");
}
#endif /* TCONFIG_KEYGEN */
#endif /* TCONFIG_OPENVPN */

void wo_ovpn_status(char *url)
{
#ifdef TCONFIG_OPENVPN
	FILE *fp;
	char buffer[256];
	char *type, *str;
	pid_t pid;
	int num = 0;

	type = 0;
	if ((str = webcgi_get("server")))
		type = "server";
	else if ((str = webcgi_get("client")))
		type = "client";

	num = str ? atoi(str) : 0;
	if ((type) && (num > 0)) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpn%s%d", type, num);
		if ((pid = pidof(buffer)) > 0) {
			/* Read the status file and repeat it verbatim to the caller */
			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "/etc/openvpn/%s%d/status", type, num);

			/* Give it some time if it doesn't exist yet */
			if (!f_exists(buffer))
				sleep(5);

			if ((fp = fopen(buffer, "r")) != NULL) {
				while (fgets(buffer, sizeof(buffer), fp) != NULL)
					web_puts(buffer);
			fclose(fp);
			}
		}
	}
#endif /* TCONFIG_OPENVPN */
}

void wo_ovpn_genkey(char *url)
{
#ifdef TCONFIG_OPENVPN
	char buffer[128];
	char *modeStr, *serverStr;
	int server, dhtype, is_ecdh;

	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, webcgi_safeget("_mode", ""), sizeof(buffer));
	modeStr = js_string(buffer); /* quicky scrub */
	if (modeStr == NULL) {
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		syslog(LOG_WARNING, "No mode was set to wo_vpn_genkey!");
#endif
		return;
	}

	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, webcgi_safeget("_server", ""), sizeof(buffer));
	serverStr = js_string(buffer); /* quicky scrub */
	if (serverStr == NULL && ((!strncmp(modeStr, "static", 6)) || (!strcmp(modeStr, "key")))) {
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		syslog(LOG_WARNING, "No server was set to wo_vpn_genkey but it was required by mode!");
#endif
		return;
	}
	server = atoi(serverStr);

	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, webcgi_safeget("_dhtype", "0"), sizeof(buffer));
	dhtype = atoi(js_string(buffer)); /* quicky scrub */

	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, webcgi_safeget("_ecdh", "0"), sizeof(buffer));
	is_ecdh = atoi(js_string(buffer)); /* quicky scrub */

	memset(buffer, 0, sizeof(buffer));

	if (!strcmp(modeStr, "static1")) { /* tls-auth / tls-crypt */
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
		strlcpy(buffer, "openvpn --genkey secret /tmp/genvpnkey >/dev/null 2>&1 && cat /tmp/genvpnkey | tail -n +4 && rm /tmp/genvpnkey", sizeof(buffer));
		syslog(LOG_WARNING, buffer);
		web_pipecmd(buffer, WOF_NONE);
	}
	else if (!strcmp(modeStr, "static2")) { /* tls-crypt-v2 */
		strlcpy(buffer, "openvpn --genkey tls-crypt-v2-server /tmp/genvpnkey >/dev/null 2>&1 && cat /tmp/genvpnkey && rm /tmp/genvpnkey", sizeof(buffer));
		syslog(LOG_WARNING, buffer);
		web_pipecmd(buffer, WOF_NONE);
#else
		strlcpy(buffer, "openvpn --genkey --secret /tmp/genvpnkey >/dev/null 2>&1 && cat /tmp/genvpnkey | tail -n +4 && rm /tmp/genvpnkey", sizeof(buffer));
		syslog(LOG_WARNING, buffer);
		web_pipecmd(buffer, WOF_NONE);
#endif /* !TCONFIG_OPTIMIZE_SIZE_MORE */
#ifdef TCONFIG_KEYGEN
	}
	else if (!strcmp(modeStr, "dh")) { /* Diffie-Hellman */
		snprintf(buffer, sizeof(buffer), "openssl dhparam -out /tmp/dh.pem %s >/dev/null 2>&1 && cat /tmp/dh.pem && rm /tmp/dh.pem", (dhtype == 1 ? "2048" : "1024"));
		syslog(LOG_WARNING, buffer);
		web_pipecmd(buffer, WOF_NONE);
	}
	else {
		prepareCAGeneration(server, is_ecdh);
		generateKey("server", 0, is_ecdh);
		print_generated_ca_to_user();
		print_generated_keys_to_user("server");
#endif /* TCONFIG_KEYGEN */
	}
#endif /* TCONFIG_OPENVPN */
}

void wo_ovpn_genclientconfig(char *url)
{
#ifdef TCONFIG_OPENVPN
#ifdef TCONFIG_KEYGEN
	FILE *fp;
	FILE *fa;
	struct in_addr lanip, lannetmask, lannet;
	char buffer[256], buffer2[8192];
	char *serverStr;
	char *dummy, *uname, *passwd;
	char *u, *nv, *nvp, *b;
	int server, hmac, is_ecdh, tls = 0;
	int userauth, useronly, userid, i = 0;

	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, webcgi_safeget("_server", ""), sizeof(buffer));
	serverStr = js_string(buffer); /* quicky scrub */

	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, url, sizeof(buffer));
	u = js_string(buffer);

	if ((serverStr == NULL) || (u == NULL)) {
		syslog(LOG_WARNING, "No server '%s' for /%s", serverStr, u);
		return;
	}
	server = atoi(serverStr);

	userauth = atoi(getNVRAMVar("vpns%d_userpass", server));
	useronly = userauth && atoi(getNVRAMVar("vpns%d_nocert", server));
	userid = atoi(webcgi_safeget("_userid", "0"));
	is_ecdh = atoi(webcgi_safeget("_ecdh", "0"));

	eval("rm", "-Rf", (char *)ovpnc_dir);
	eval("mkdir", "-m", "0777", "-p", (char *)ovpnc_dir);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s/connection.ovpn", ovpnc_dir);
	if ((fp = fopen(buffer, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, buffer);
		return;
	}

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "vpns%d_crypt", server);
	if (nvram_match(buffer, "tls"))
		tls = 1;

	/* Remote address */
	fprintf(fp, "# Config generated by Tomato64 %s, requires OpenVPN 2.4.0 or newer\n\n"
	            "remote %s %d\n",
	            tomato_version,
	            get_wanip("wan"),
	            atoi(getNVRAMVar("vpns%d_port", server)));

	/* Proto */
	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, getNVRAMVar("vpns%d_proto", server), sizeof(buffer));
	str_replace(buffer, "-server", "-client");
	fprintf(fp, "proto %s\n", buffer);

	/* Interface */
	fprintf(fp, "dev %s\n", getNVRAMVar("vpns%d_if", server));

	/* Cipher */
	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, getNVRAMVar("vpns%d_ncp_ciphers", server), sizeof(buffer));
	if (tls == 1) {
		if (buffer[0] != '\0')
			fprintf(fp, "data-ciphers %s\n", buffer);
	}
	else { /* secret */
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpns%d_cipher", server);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
	}

	/* Digest */
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "vpns%d_digest", server);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	if (tls == 1) {
		fprintf(fp, "client\n"
		            ";verify-x509-name \"server\" name\n"
		            "remote-cert-tls server\n"
		            "\n;ca ca.pem\n"
		            "<ca>\n%s\n</ca>\n\n",
		            getNVRAMVar("vpns%d_ca", server));

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%s/ca.pem", ovpnc_dir);
		put_to_file(buffer, getNVRAMVar("vpns%d_ca", server));

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpns%d_hmac", server);
		hmac = nvram_get_int(buffer);
		if (hmac >= 0) {
			if (hmac == 3)
				fprintf(fp, ";tls-crypt static.key");
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
			else if (hmac == 4)
				fprintf(fp, ";tls-crypt-v2 static.key");
#endif /* TCONFIG_OPTIMIZE_SIZE_MORE */
			else {
				fprintf(fp, ";tls-auth static.key");
				if (hmac == 0) {
					fprintf(fp, " 1\n"
					            "key-direction 1\n");
				}
				else if (hmac == 1) {
					fprintf(fp, " 0\n"
					            "key-direction 0\n");
				}
				else if (hmac == 2)
					fprintf(fp, "key-direction bidirectional\n");
			}
			fprintf(fp, "\n");

#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
			if (hmac == 4) { /* tls-crypt-v2 */
				char *openvpn_argv[] = {
					"openvpn", "--tls-crypt-v2", "/tmp/ovpnclientconfig/static-server.key",
					"--genkey", "tls-crypt-v2-client", "/tmp/ovpnclientconfig/static.key", NULL
				};

				put_to_file("/tmp/ovpnclientconfig/static-server.key", getNVRAMVar("vpns%d_static", server));
				_eval(openvpn_argv, "/dev/null", 0, NULL);
				eval("rm", "/tmp/ovpnclientconfig/static-server.key");
				fprintf(fp, "<tls-crypt-v2>\n%s</tls-crypt-v2>\n\n", read_from_file("/tmp/ovpnclientconfig/static.key", buffer2, sizeof(buffer2)));
			}
			else
#endif /* TCONFIG_OPTIMIZE_SIZE_MORE */
			{ /* tls-auth / tls-crypt */
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%s/static.key", ovpnc_dir);
				put_to_file(buffer, getNVRAMVar("vpns%d_static", server));

				if (hmac == 3) /* tls-crypt */
					fprintf(fp, "<tls-crypt>\n%s</tls-crypt>\n\n", getNVRAMVar("vpns%d_static", server));
				else /* tls-auth */
					fprintf(fp, "<tls-auth>\n%s</tls-auth>\n\n", getNVRAMVar("vpns%d_static", server));
			}
		}

		/* Auth */
		if (userauth) {
			fprintf(fp, "auth-user-pass auth.txt\n");

			nv = nvp = strdup(getNVRAMVar("vpns%d_users_val", server));
			if (nv) {
				while (nvp && (b = strsep(&nvp, ">")) != NULL) {
					dummy = uname = passwd = NULL;
					++i;

					/* enabled<username<password> */
					if ((vstrsep(b, "<", &dummy, &uname, &passwd)) < 3)
						continue;

					if ((*uname =='\0') || (*passwd == '\0'))
						continue;

					/* compare with user id */
					if (i == userid) {
						memset(buffer, 0, sizeof(buffer));
						snprintf(buffer, sizeof(buffer), "%s/auth.txt", ovpnc_dir);
						if ((fa = fopen(buffer, "w")) != NULL) {
							fprintf(fa, "%s\n%s\n", uname, passwd);
							fclose(fa);
						}
						break;
					}
				}
				free(nv);
			}
		}

		if (!useronly) {
			prepareCAGeneration(server, is_ecdh);
			generateKey("client", userid, is_ecdh);

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%s/client.crt", openssl_dir);
			eval("cp", buffer, (char *)ovpnc_dir);

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%s/client.key", openssl_dir);
			eval("cp", buffer, (char *)ovpnc_dir);

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%s/client.crt", ovpnc_dir);
			fprintf(fp, ";cert client.crt\n<cert>\n%s</cert>\n\n", read_from_file(buffer, buffer2, sizeof(buffer2)));

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%s/client.key", ovpnc_dir);
			fprintf(fp, ";key client.key\n<key>\n%s</key>\n\n", read_from_file(buffer, buffer2, sizeof(buffer2)));
		}
	}
	else {
		fprintf(fp, "mode p2p\n");

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpns%d_if", server);
		if (nvram_contains_word(buffer, "tap")) {
			fprintf(fp, "ifconfig %s "
			            "%s\n",
			            getNVRAMVar("vpns%d_local", server),
			            getNVRAMVar("vpns%d_nm", server));
		}
		else {
			fprintf(fp, "ifconfig %s "
			            "%s\n",
			            getNVRAMVar("vpns%d_remote", server),
			            getNVRAMVar("vpns%d_local", server));
		}
		if (inet_aton(nvram_safe_get("lan_ipaddr"),&lanip) && inet_aton(nvram_safe_get("lan_netmask"),&lannetmask))
		{
			lannet.s_addr = lanip.s_addr & lannetmask.s_addr;
			fprintf(fp, "route %s %s\n\n", inet_ntoa(lannet), nvram_safe_get("lan_netmask"));
		}
		fprintf(fp, ";secret static.key\n<secret>\n%s</secret>\n\n", getNVRAMVar("vpns%d_static", server));

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%s/static.key", ovpnc_dir);
		put_to_file(buffer, getNVRAMVar("vpns%d_static", server));
	}

	fprintf(fp, "keepalive 15 60\n"
	            "resolv-retry infinite\n"
	            "nobind\n"
	            "float\n"
	            "verb 3\n"
	            ";status status\n"
	            ";log /var/log/openvpn.log\n");

	fclose(fp);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s.tar", ovpnc_dir);
	eval("tar", "-cf", buffer, "-C", (char *)ovpnc_dir, ".");

	do_file(buffer);
#endif /* TCONFIG_KEYGEN */
#endif /* TCONFIG_OPENVPN */
}
