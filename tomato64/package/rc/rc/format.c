/*
 * format.c - NVRAM variable migration for firmware upgrades
 */

#include "rc.h"

/*
 * If old_name exists in NVRAM, copy its value to new_name
 * and unset old_name. Returns 1 if migrated, 0 if not.
 */
static int migrate_nv(const char *old_name, const char *new_name)
{
	const char *val = nvram_get(old_name);
	if (val) {
		nvram_set(new_name, val);
		nvram_unset(old_name);
		return 1;
	}
	return 0;
}

void nvram_format_compat(void)
{
	int need_commit = 0;

/***********************************************************
 * OpenVPN renames
 ***********************************************************/
#ifdef TCONFIG_OPENVPN
	{
		char old_var[64], new_var[64];
		const char **s;
		int i;

		const char *vpn_server_suffixes[] = {
			"poll", "if", "proto", "port", "firewall", "crypt", "comp",
			"cipher", "ncp_ciphers", "digest", "dhcp", "r1", "r2", "sn",
			"nm", "local", "remote", "reneg", "hmac", "plan", "pdns",
			"ccd", "c2c", "ccd_excl", "ccd_val", "rgw", "userpass",
			"nocert", "custom", "static", "ca", "ca_key", "crt", "crl",
			"key", "dh", "br", "ecdh", "dco", "users_val",
			NULL
		};

		/* vpn_server<N>_* -> vpns<N>_* */
		for (i = 1; i <= OVPN_SERVER_MAX; i++) {
			for (s = vpn_server_suffixes; *s; s++) {
				snprintf(old_var, sizeof(old_var), "vpn_server%d_%s", i, *s);
				snprintf(new_var, sizeof(new_var), "vpns%d_%s", i, *s);
				need_commit |= migrate_nv(old_var, new_var);
			}
		}

		const char *vpn_client_suffixes[] = {
			"poll", "tchk", "if", "bridge", "nat", "proto", "addr",
			"port", "retry", "rg", "firewall", "crypt", "comp", "cipher",
			"ncp_ciphers", "digest", "local", "remote", "nm", "reneg",
			"hmac", "adns", "rgw", "gw", "custom", "static", "ca", "crt",
			"key", "br", "nobind", "routing_val", "fw", "tlsvername",
			"prio", "dco", "userauth", "username", "password", "useronly",
			"tlsremote", "cn",
			NULL
		};

		/* vpn_client<N>_* -> vpnc<N>_* */
		for (i = 1; i <= OVPN_CLIENT_MAX; i++) {
			for (s = vpn_client_suffixes; *s; s++) {
				snprintf(old_var, sizeof(old_var), "vpn_client%d_%s", i, *s);
				snprintf(new_var, sizeof(new_var), "vpnc%d_%s", i, *s);
				need_commit |= migrate_nv(old_var, new_var);
			}
		}

		/* Global renames */
		need_commit |= migrate_nv("vpn_server_eas", "vpns_eas");
		need_commit |= migrate_nv("vpn_server_dns", "vpns_dns");
		need_commit |= migrate_nv("vpn_client_eas", "vpnc_eas");

		/* Removed variables */
		if (nvram_get("vpn_debug")) {
			nvram_unset("vpn_debug");
			need_commit = 1;
		}
	}
#endif /* TCONFIG_OPENVPN */

	if (need_commit) {
		fprintf(stderr, "NVRAM migration: renamed legacy variables\n");
		nvram_commit();
	}
}
