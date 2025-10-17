/*
 *
 * FreshTomato Firmware
 * Copyright (C) 2018 Michal Obrembski
 *
 * Fixes/updates (C) 2018 - 2025 pedro
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

static void prepareCAGeneration(const int serverNum, const int is_ecdh)
{
	char buffer[512], buffer2[512], tmp[64];
	char *p;

	eval("rm", "-Rf", (char *)openssl_dir);
	eval("mkdir", "-p", (char *)openssl_dir);

	/* reset index */
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s/index.txt", openssl_dir);
	put_to_file(buffer, "");

	/* reset log */
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s/openssl.log", openssl_dir);
	put_to_file(buffer, "");

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "vpn_server%d_ca_key", serverNum);

	if (nvram_match(buffer, "")) {
		syslog(LOG_WARNING, "No CA KEY was saved for server %d, regenerating ...", serverNum);

		memset(tmp, 0, sizeof(tmp));
		if ((p = nvram_safe_get("wan_domain")) && (strcmp(p, "")))
			snprintf(tmp, sizeof(tmp), ".%s", p);

		memset(buffer2, 0, sizeof(buffer2));
		snprintf(buffer2, sizeof(buffer2), "\"/C=GB/ST=Yorks/L=York/O=Tomato64/OU=IT/CN=server%s\"", tmp);

		memset(buffer, 0, sizeof(buffer));
		if (is_ecdh == 1) {
			snprintf(buffer, sizeof(buffer), "openssl ecparam -genkey -name prime256v1 -out %s/cakey.pem -noout >> %s/openssl.log 2>&1", openssl_dir, openssl_dir);
			syslog(LOG_WARNING, buffer);
			system(buffer);

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "openssl req -new -noenc -x509 -days 3650 -key %s/cakey.pem -out %s/cacert.pem -extensions v3_ca -subj %s >> %s/openssl.log 2>&1", openssl_dir, openssl_dir, buffer2, openssl_dir);
			syslog(LOG_WARNING, buffer);
			system(buffer);
		}
		else {
			snprintf(buffer, sizeof(buffer), "openssl req -new -noenc -x509 -days 3650 -keyout %s/cakey.pem -out %s/cacert.pem -subj %s >> %s/openssl.log 2>&1", openssl_dir, openssl_dir, buffer2, openssl_dir);
			syslog(LOG_WARNING, buffer);
			system(buffer);
		}
	}
	else {
		syslog(LOG_WARNING, "Found CA KEY for server %d, creating from NVRAM", serverNum);

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%s/cakey.pem", openssl_dir);
		put_to_file(buffer, getNVRAMVar("vpn_server%d_ca_key", serverNum));

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%s/cacert.pem", openssl_dir);
		put_to_file(buffer, getNVRAMVar("vpn_server%d_ca", serverNum));
	}
}

static void generateKey(const char *prefix, const int userid, const int is_ecdh)
{
	char subj_buf[256], buffer[512], tmp[64], serial[8];
	char *p, *str;

	if (strncmp(prefix, "server", 6) == 0) {
		str = "-extensions server_cert";
		syslog(LOG_WARNING, "Building Certs for Server");
	}
	else {
		str = "-extensions usr_cert";
		syslog(LOG_WARNING, "Building Certs for Client%d", userid);
	}

	memset(serial, 0, sizeof(serial));
	snprintf(serial, sizeof(serial), "%.2X", userid);

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s/serial", openssl_dir);
	put_to_file(buffer, serial);

	memset(serial, 0, sizeof(serial));
	snprintf(serial, sizeof(serial), "%d", userid);

	memset(tmp, 0, sizeof(tmp));
	if ((p = nvram_safe_get("wan_domain")) && (strcmp(p, "")))
		snprintf(tmp, sizeof(tmp), ".%s", p);

	memset(subj_buf, 0, sizeof(subj_buf));
	snprintf(subj_buf, sizeof(subj_buf), "\"/C=GB/ST=Yorks/L=York/O=Tomato64/OU=IT/CN=%s%s%s\"", prefix, (userid > 0 ? serial : ""), tmp);

	if (is_ecdh == 1) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "openssl ecparam -genkey -name prime256v1 -out %s/%s.key -noout >> %s/openssl.log 2>&1", openssl_dir, prefix, openssl_dir);
		syslog(LOG_WARNING, buffer);
		system(buffer);

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "openssl req -new -noenc -key %s/%s.key -out %s/%s.csr %s -subj %s >> %s/openssl.log 2>&1", openssl_dir, prefix, openssl_dir, prefix, str, subj_buf, openssl_dir);
		syslog(LOG_WARNING, buffer);
		system(buffer);

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "openssl ca -batch -policy policy_anything -days 3650 -notext -keyfile %s/cakey.pem -cert %s/cacert.pem -in %s/%s.csr -out %s/%s.crt %s -subj %s >> %s/openssl.log 2>&1", openssl_dir, openssl_dir, openssl_dir, prefix, openssl_dir, prefix, str, subj_buf, openssl_dir);
		syslog(LOG_WARNING, buffer);
		system(buffer);
	}
	else {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "openssl req -new -noenc -keyout %s/%s.key -out %s/%s.csr %s -subj %s >> %s/openssl.log 2>&1", openssl_dir, prefix, openssl_dir, prefix, str, subj_buf, openssl_dir);
		syslog(LOG_WARNING, buffer);
		system(buffer);

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "openssl ca -batch -policy policy_anything -days 3650 -in %s/%s.csr -out %s/%s.crt %s -subj %s >> %s/openssl.log 2>&1", openssl_dir, prefix, openssl_dir, prefix, str, subj_buf, openssl_dir);
		syslog(LOG_WARNING, buffer);
		system(buffer);

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "openssl x509 -in %s/%s.crt -inform PEM -out %s/%s.crt -outform PEM >> %s/openssl.log 2>&1", openssl_dir, prefix, openssl_dir, prefix, openssl_dir);
		syslog(LOG_WARNING, buffer);
		system(buffer);
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

	userauth = atoi(getNVRAMVar("vpn_server%d_userpass", server));
	useronly = userauth && atoi(getNVRAMVar("vpn_server%d_nocert", server));
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
	snprintf(buffer, sizeof(buffer), "vpn_server%d_crypt", server);
	if (nvram_match(buffer, "tls"))
		tls = 1;

	/* Remote address */
	fprintf(fp, "# Config generated by Tomato64 %s, requires OpenVPN 2.4.0 or newer\n\n"
	            "remote %s %d\n",
	            tomato_version,
	            get_wanip("wan"),
	            atoi(getNVRAMVar("vpn_server%d_port", server)));

	/* Proto */
	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, getNVRAMVar("vpn_server%d_proto", server), sizeof(buffer));
	str_replace(buffer, "-server", "-client");
	fprintf(fp, "proto %s\n", buffer);

	/* Compression */
	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, getNVRAMVar("vpn_server%d_comp", server), sizeof(buffer));
	if (strcmp(buffer, "-1")) {
		if ((!strcmp(buffer, "lz4")) || (!strcmp(buffer, "lz4-v2")))
			fprintf(fp, "compress %s\n", buffer);
		else if (!strcmp(buffer, "yes"))
			fprintf(fp, "compress lzo\n");
		else if (!strcmp(buffer, "adaptive"))
			fprintf(fp, "comp-lzo adaptive\n");
		else if (!strcmp(buffer, "no"))
			fprintf(fp, "compress\n"); /* disable, but can be overriden */
	}

	/* Interface */
	fprintf(fp, "dev %s\n", getNVRAMVar("vpn_server%d_if", server));

	/* Cipher */
	memset(buffer, 0, sizeof(buffer));
	strlcpy(buffer, getNVRAMVar("vpn_server%d_ncp_ciphers", server), sizeof(buffer));
	if (tls == 1) {
		if (buffer[0] != '\0')
			fprintf(fp, "data-ciphers %s\n", buffer);
	}
	else { /* secret */
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpn_server%d_cipher", server);
		if (!nvram_contains_word(buffer, "default"))
			fprintf(fp, "cipher %s\n", nvram_safe_get(buffer));
	}

	/* Digest */
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "vpn_server%d_digest", server);
	if (!nvram_contains_word(buffer, "default"))
		fprintf(fp, "auth %s\n", nvram_safe_get(buffer));

	if (tls == 1) {
		fprintf(fp, "client\n"
		            ";verify-x509-name \"server\" name\n"
		            "remote-cert-tls server\n"
		            "\n;ca ca.pem\n"
		            "<ca>\n%s\n</ca>\n\n",
		            getNVRAMVar("vpn_server%d_ca", server));

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%s/ca.pem", ovpnc_dir);
		put_to_file(buffer, getNVRAMVar("vpn_server%d_ca", server));

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "vpn_server%d_hmac", server);
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
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%s/static-server.key", ovpnc_dir);
				put_to_file(buffer, getNVRAMVar("vpn_server%d_static", server));

				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "openvpn --tls-crypt-v2 %s/static-server.key --genkey tls-crypt-v2-client %s/static.key >/dev/null 2>&1", ovpnc_dir, ovpnc_dir);
				syslog(LOG_WARNING, buffer);
				system(buffer);

				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%s/static-server.key", ovpnc_dir);
				eval("rm", buffer);

				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%s/static.key", ovpnc_dir);
				fprintf(fp, "<tls-crypt-v2>\n%s</tls-crypt-v2>\n\n", read_from_file(buffer, buffer2, sizeof(buffer2)));
			}
			else
#endif /* TCONFIG_OPTIMIZE_SIZE_MORE */
			{ /* tls-auth / tls-crypt */
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%s/static.key", ovpnc_dir);
				put_to_file(buffer, getNVRAMVar("vpn_server%d_static", server));

				if (hmac == 3) /* tls-crypt */
					fprintf(fp, "<tls-crypt>\n%s</tls-crypt>\n\n", getNVRAMVar("vpn_server%d_static", server));
				else /* tls-auth */
					fprintf(fp, "<tls-auth>\n%s</tls-auth>\n\n", getNVRAMVar("vpn_server%d_static", server));
			}
		}

		/* Auth */
		if (userauth) {
			fprintf(fp, "auth-user-pass auth.txt\n");

			nv = nvp = strdup(getNVRAMVar("vpn_server%d_users_val", server));
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
		snprintf(buffer, sizeof(buffer), "vpn_server%d_if", server);
		if (nvram_contains_word(buffer, "tap")) {
			fprintf(fp, "ifconfig %s "
			            "%s\n",
			            getNVRAMVar("vpn_server%d_local", server),
			            getNVRAMVar("vpn_server%d_nm", server));
		}
		else {
			fprintf(fp, "ifconfig %s "
			            "%s\n",
			            getNVRAMVar("vpn_server%d_remote", server),
			            getNVRAMVar("vpn_server%d_local", server));
		}
		if (inet_aton(nvram_safe_get("lan_ipaddr"),&lanip) && inet_aton(nvram_safe_get("lan_netmask"),&lannetmask))
		{
			lannet.s_addr = lanip.s_addr & lannetmask.s_addr;
			fprintf(fp, "route %s %s\n\n", inet_ntoa(lannet), nvram_safe_get("lan_netmask"));
		}
		fprintf(fp, ";secret static.key\n<secret>\n%s</secret>\n\n", getNVRAMVar("vpn_server%d_static", server));

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%s/static.key", ovpnc_dir);
		put_to_file(buffer, getNVRAMVar("vpn_server%d_static", server));
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
