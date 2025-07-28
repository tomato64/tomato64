/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 * Fixes/updates (C) 2018 - 2025 pedro
 *
 */


#include "rc.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"rc_debug"


#ifdef DEBUG_RCTEST
/* used for various testing */
static int rctest_main(int argc, char *argv[])
{
	if (argc < 2)
		printf("test what?\n");
	else {
		printf("%s\n", argv[1]);

		if (strcmp(argv[1], "foo") == 0) {
		}
		else if (strcmp(argv[1], "qos") == 0)
			start_qos();

#ifdef TCONFIG_FANCTRL
		else if (strcmp(argv[1], "phy_tempsense") == 0) {
			stop_phy_tempsense();
			start_phy_tempsense();
		}
#endif
#ifdef TCONFIG_IPV6
		else if (strcmp(argv[1], "6rd") == 0) {
			stop_6rd_tunnel();
			start_6rd_tunnel();
		}
#endif
#ifdef DEBUG_IPTFILE
		else if (strcmp(argv[1], "iptfile") == 0)
			create_test_iptfile();
#endif
		else
			printf("what?\n");
	}
	return 0;
}
#endif /* DEBUG_RCTEST */

static int hotplug_main(int argc, char *argv[])
{
	if (argc >= 2) {
		if (strcmp(argv[1], "net") == 0)
			hotplug_net();
#ifdef TCONFIG_USB
		else if (strcmp(argv[1], "usb") == 0)
			hotplug_usb();
		else if (strcmp(argv[1], "block") == 0)
			hotplug_usb();
#endif
	}
	return 0;
}

static int rc_main(int argc, char *argv[])
{
	if (argc < 2)
		return 0;
	if (strcmp(argv[1], "start") == 0)
		return kill(1, SIGUSR2);
	if (strcmp(argv[1], "stop") == 0)
		return kill(1, SIGINT);
	if (strcmp(argv[1], "restart") == 0)
		return kill(1, SIGHUP);

	return 0;
}

void chains_log_detection(void)
{
	int n;

	n = nvram_get_int("log_in");
	chain_in_drop = (n & 1) ? "logdrop" : "DROP";
	chain_in_accept = (n & 2) ? "logaccept" : "ACCEPT";

	n = nvram_get_int("log_out");
	chain_out_drop = (n & 1) ? "logdrop" : "DROP";
	chain_out_reject = (n & 1) ? "logreject" : "REJECT --reject-with tcp-reset";
	chain_out_accept = (n & 2) ? "logaccept" : "ACCEPT";
	//if (nvram_match("nf_drop_reset", "1")) chain_out_drop = chain_out_reject;
}

void fix_chain_in_drop(void)
{
	char buf[8];

	chains_log_detection();

	/* if logging - readd the logdrop rule at the end of the INPUT chain */
	if (*chain_in_drop == 'l') {
		memset(buf, 0, sizeof(buf)); /* reset */
		snprintf(buf, sizeof(buf), "%s", chain_in_drop);

		eval("iptables", "-D", "INPUT", "-j", buf);
		eval("iptables", "-A", "INPUT", "-j", buf);
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			eval("ip6tables", "-D", "INPUT", "-j", buf);
			eval("ip6tables", "-A", "INPUT", "-j", buf);
		}
#endif
	}
}

/* copy env to nvram
 * returns 1 if new/changed, 0 if not changed/no env
 */
int env2nv(char *env, char *nv)
{
	char *value;
	if ((value = getenv(env)) != NULL) {
		if (!nvram_match(nv, value)) {
			nvram_set(nv, value);
			return 1;
		}
	}
	return 0;
}

/* serialize (re-)starts from GUI, avoid zombies */
int serialize_restart(char *service, int start)
{
	char s[32];
	char *pos;
	unsigned int index = 0;
	pid_t pid, pid_rc = getpid();

	/* replace '-' with '_' otherwise exec_service() will fail */
	memset(s, 0, sizeof(s)); /* reset */
	strlcpy(s, service, sizeof(s));
	if ((pos = strstr(s, "-")) != NULL) {
		index = pos - s;
		s[index] = '\0';
		strlcat(s, "_", sizeof(s));
		strlcat(s, service + index + 1, sizeof(s));
	}
	logmsg(LOG_DEBUG, "*** %s: IN - service: %s %s - PID[rc]: %d", __FUNCTION__, service, (start ? "start" : "stop"), pid_rc);

	if (start == 1) {
		if (pid_rc != 1) {
			logmsg(LOG_DEBUG, "*** %s: call start_service(%s) - PID[rc]: %d", __FUNCTION__, s, pid_rc);
			start_service(s);
			return 1;
		}
		if ((pid = pidof(service)) > 0) {
			logmsg(LOG_WARNING, "service: %s already running; its PID: %d", s, pid);
			return 1;
		}
#ifdef TCONFIG_WIREGUARD
		/* special case: wireguard */
		if (strncmp(service, "wireguard", 9) == 0) {
			memset(s, 0, sizeof(s)); /* reset */
			snprintf(s, sizeof(s), "wg%d", atoi(&service[9]));
			if (if_nametoindex(s)) {
				logmsg(LOG_WARNING, "service: %s already running; interface %s is up", service, s);
				return 1;
			}
		}
#endif
	}
	else {
		if (pid_rc != 1) {
			logmsg(LOG_DEBUG, "*** %s: call stop_service(%s) - PID[rc]: %d", __FUNCTION__, s, pid_rc);
			stop_service(s);
			return 1;
		}
	}

	return 0;
}

/* replace -A, -I and -N in the FW script with -D and execute it */
void run_del_firewall_script(const char *infile, char *outfile)
{
	FILE *ifp, *ofp;
	char line[128];
	char *p;

	ifp = fopen(infile, "r");
	ofp = fopen(outfile, "w+");
	if (!ifp || !ofp) {
		if (ifp) fclose(ifp);
		if (ofp) {
			fclose(ofp);
			unlink(outfile);
		}
		logmsg(LOG_DEBUG, "*** %s: no iptable rules, file: %s!", __FUNCTION__, infile);
		return;
	}

	while (fgets(line, sizeof(line), ifp)) {
		for (p = line; *p; ++p) {
			if (*p == '-' && (p[1] == 'A' || p[1] == 'I' || p[1] == 'N')) {
				p[1] = 'D';
			}
		}
		fputs(line, ofp);
	}
	fclose(ifp);
	fclose(ofp);

	chmod(outfile, (S_IRUSR | S_IWUSR | S_IXUSR));

	if (eval(outfile))
		logmsg(LOG_DEBUG, "*** %s: unable to remove iptable rules, file: %s!", __FUNCTION__, infile);
	else
		logmsg(LOG_DEBUG, "*** %s: iptable rules have been removed, file: %s", __FUNCTION__, infile);

	unlink(outfile);
}

#if defined(TCONFIG_OPENVPN) || defined(TCONFIG_WIREGUARD)
void kill_switch(const char *kind)
{
	unsigned int unit, br, rules_count;
	int policy_type;
	int wan_unit, mwan_num;
	char *enable, *type, *value, *kswitch;
	char *nv, *nvp, *b, *c;
	char wan_prefix[] = "wanXX";
	char buf[64], buf2[64], val[64], wan_if[16];
	unsigned int kd = (strcmp(kind, "wg") == 0 ? 0 : 1);

	mwan_num = nvram_get_int("mwan_num");
	if ((mwan_num < 1) || (mwan_num > MWAN_MAX))
		mwan_num = 1;

	for (unit = kd; unit <= (kd ? OVPN_CLIENT_MAX : WG_INTERFACE_MAX); ++unit) {
		/* only apply kill switch rules if in PBR mode! */
		if (kd) { /* ovpn */
			if ((atoi(getNVRAMVar(("vpn_client%u_rgw"), unit)) < VPN_RGW_POLICY) || (strcmp(getNVRAMVar(("vpn_client%u_if"), unit), "tun"))) /* proper policy mode and if: 'tun' */
				continue;
		}
		else { /* wireguard */
			if ((atoi(getNVRAMVar(("wg%u_rgwr"), unit)) < VPN_RGW_POLICY) || (atoi(getNVRAMVar(("wg%u_com"), unit)) < 3)) /* proper policy mode and in 'External - VPN Provider' */
				continue;
		}

		rules_count = 0;
		nv = nvp = strdup(getNVRAMVar((kd ? "vpn_client%u_routing_val" : "wg%u_routing_val"), unit));

		while (nvp && (b = strsep(&nvp, ">")) != NULL) {
			enable = type = value = kswitch = NULL;

			/* enable<type<domain_or_IP<kill_switch> */
			if ((vstrsep(b, "<", &enable, &type, &value, &kswitch)) < 4)
				continue;

			/* check if rule is enabled and kill switch is active and IP/domain is set */
			if ((atoi(enable) != 1) || (atoi(kswitch) != 1) || (*value == '\0'))
				continue;

			policy_type = atoi(type);
			rules_count++;

			/* check all active WANs */
			for (wan_unit = 1; wan_unit <= mwan_num; ++wan_unit) {
				get_wan_prefix(wan_unit, wan_prefix);

				/* find WAN IF */
				memset(wan_if, 0, sizeof(wan_if)); /* reset */
				snprintf(wan_if, sizeof(wan_if), "%s", get_wanface(wan_prefix));
				if ((!*wan_if) || (strcmp(wan_if, "") == 0))
					continue;

				memset(val, 0, sizeof(val)); /* reset */
				snprintf(val, sizeof(val), "%s", value); /* copy IP/domain to buffer */

				/* "From Source IP" */
				if (policy_type == 1) {
					/* find correct bridge for given IP */
					for (br = 0; br < BRIDGE_COUNT; br++) {
						memset(buf, 0, sizeof(buf)); /* reset */
						snprintf(buf, sizeof(buf), (br == 0 ? "lan_ipaddr" : "lan%u_ipaddr"), br);

						char *lan_ip = nvram_safe_get(buf);
						if (strcmp(lan_ip, "") != 0) { /* only for active */
							memset(buf, 0, sizeof(buf)); /* reset */
							snprintf(buf, sizeof(buf), "%s", val);
							if ((c = strchr(buf, '/')) != NULL)
								*c = 0; /* with mask? get IP */

							memset(buf, 0, sizeof(buf)); /* reset */
							snprintf(buf, sizeof(buf), "%s", val);
							if ((c = strrchr(buf, '.')) != NULL)
								*(c + 1) = 0; /* get first 3 octets from value */

							memset(buf2, 0, sizeof(buf2)); /* reset */
							snprintf(buf2, sizeof(buf2), "%s", lan_ip);
							if ((c = strrchr(buf2, '.')) != NULL)
								*(c + 1) = 0; /* get first 3 octets from lan IP */

							if (strcmp(buf, buf2) == 0) {
								memset(buf2, 0, sizeof(buf2)); /* reset */
								snprintf(buf2, sizeof(buf2), "br%u", br); /* copy brX to buffer */

								logmsg(LOG_INFO, "Kill-Switch: type: %d - add %s", policy_type, val);
								eval("iptables", "-I", "FORWARD", "-i", buf2, "-s", val, "-o", wan_if, "-j", "REJECT");
							}
						}
					}
				}
				/* "To Destination IP" / "To Domain" */
				else if ((policy_type == 2) || (policy_type == 3)) {
					memset(buf, 0, sizeof(buf)); /* reset */
					snprintf(buf, sizeof(buf), (kd ? "tun1%u" : "wg%u"), unit); /* find the appropriate IF */

					/* xstart - do not wait when WAN in not up! */
					logmsg(LOG_INFO, "Kill-Switch: type: %d - add %s", policy_type, val);
					xstart("iptables", "-I", "FORWARD", "!", "-o", buf, "-d", val, "-j", "REJECT");
					xstart("iptables", "-I", "FORWARD", "-o", wan_if, "-d", val, "-j", "REJECT");
				}

			}
		}
		if (nv)
			free(nv);

		if (rules_count > 0)
			logmsg(LOG_INFO, "Kill-Switch: added %u rules to firewall for %s%u", rules_count, (kd ? "openvpn-client" : "wireguard"), unit);
	}
}

void run_vpn_firewall_scripts(const char *kind)
{
	DIR *dir;
	struct stat fs;
	struct dirent *file;
	char *fa;
	char buf[64];

	kill_switch(kind);

	if (chdir((strcmp(kind, "wg") == 0 ? WG_FW_DIR : OVPN_FW_DIR)))
		return;

	dir = opendir((strcmp(kind, "wg") == 0 ? WG_FW_DIR : OVPN_FW_DIR));

	logmsg(LOG_DEBUG, "*** %s: beginning all firewall scripts...", __FUNCTION__);

	while ((file = readdir(dir)) != NULL) {
		fa = file->d_name;

		if ((fa[0] == '.') || (strcmp(fa, (strcmp(kind, "wg") == 0 ? WG_DIR_DEL_SCRIPT : OVPN_DEL_SCRIPT)) == 0))
			continue;

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s/", (strcmp(kind, "wg") == 0 ? WG_FW_DIR : OVPN_FW_DIR));
		strlcat(buf, fa, sizeof(buf));

		/* check exe permission (in case vpnrouting.sh is still working on routing file) */
		stat(buf, &fs);
		if (fs.st_mode & S_IXUSR) {
			/* first remove existing firewall rule(s) */
			run_del_firewall_script(buf, (strcmp(kind, "wg") == 0 ? WG_DIR_DEL_SCRIPT : OVPN_DIR_DEL_SCRIPT));

			/* then (re-)add firewall rule(s) */
			logmsg(LOG_DEBUG, "*** %s: running firewall script: %s", __FUNCTION__, buf);
			eval(buf);
			fix_chain_in_drop();
		}
		else
			logmsg(LOG_DEBUG, "*** %s: skipping firewall script (not executable): %s", __FUNCTION__, buf);
	}
	logmsg(LOG_DEBUG, "*** %s: done with all firewall scripts...", __FUNCTION__);

	closedir(dir);
}
#endif

typedef struct {
	const char *name;
	int (*main)(int argc, char *argv[]);
} applets_t;

static const applets_t applets[] = {
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
	{ "preinit",			init_main			},
#endif
#endif /* TOMATO64 */
	{ "init",			init_main			},
	{ "console",			console_main			},
	{ "rc",				rc_main				},
	{ "ip-up",			ipup_main			},
	{ "ip-down",			ipdown_main			},
	{ "ip-pre-up",			ippreup_main			},
#ifdef TCONFIG_IPV6
	{ "ipv6-up",			ip6up_main			},
	{ "ipv6-down",			ip6down_main			},
#endif
#ifdef TCONFIG_PPTPD
	{ "pptpc_ip-up",		pptpc_ipup_main			},
	{ "pptpc_ip-down",		pptpc_ipdown_main		},
#endif
	{ "ppp_event",			pppevent_main			},
	{ "hotplug",			hotplug_main			},
	{ "redial",			redial_main			},
	{ "mwanroute",			mwan_route_main			},
	{ "listen",			listen_main			},
	{ "service",			service_main			},
	{ "sched",			sched_main			},
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
	{ "mtd-write",			mtd_write_main_old		},
	{ "mtd-erase",			mtd_unlock_erase_main_old	},
	{ "mtd-unlock",			mtd_unlock_erase_main_old	},
#else
	{ "mtd-write",			mtd_write_main			},
	{ "mtd-erase",			mtd_unlock_erase_main		},
	{ "mtd-unlock",			mtd_unlock_erase_main		},
#endif
	{ "buttons",			buttons_main			},
#if defined(TCONFIG_BCMARM) || defined(TCONFIG_BLINK)
	{ "blink",			blink_main			},
	{ "blink_br",			blink_br_main			},
#endif
#endif /* TOMATO64 */
#ifdef TCONFIG_FANCTRL
	{ "phy_tempsense",		phy_tempsense_main		},
#endif
	{ "rcheck",			rcheck_main			},
	{ "dhcpc-event",		dhcpc_event_main		},
	{ "dhcpc-release",		dhcpc_release_main		},
	{ "dhcpc-renew",		dhcpc_renew_main		},
	{ "dhcpc-event-lan",		dhcpc_lan_main			},
#ifdef TCONFIG_IPV6
	{ "dhcp6c-state",		dhcp6c_state_main		},
#endif
#ifndef TOMATO64
	{ "radio",			radio_main			},
#endif /* TOMATO64 */
	{ "led",			led_main			},
	{ "halt",			reboothalt_main			},
	{ "reboot",			reboothalt_main			},
#ifdef TOMATO64_X86_64
	{ "fast-reboot",		fastreboot_main			},
#endif /* TOMATO64_X86_64 */
	{ "gpio",			gpio_main			},
#ifndef TOMATO64
	{ "wldist",			wldist_main			},
#endif /* TOMATO64 */
#ifdef TCONFIG_CIFS
	{ "mount-cifs",			mount_cifs_main			},
#endif
#ifdef TCONFIG_DDNS
	{ "ddns-update",		ddns_update_main		},
#endif
#ifdef DEBUG_RCTEST
	{ "rctest",			rctest_main			},
#endif
	{ "ntpd_synced",		ntpd_synced_main		},
#ifdef TCONFIG_ROAM
	{ "roamast",			roam_assistant_main		},
#endif
	{NULL, NULL}
};

#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
void erase_nvram(void)
{
	eval("mtd-erase2", "nvram");
}
#endif
#endif /* TOMATO64 */

int main(int argc, char **argv)
{
	char *base;
	int f;

	/*
	 * Make sure std* are valid since several functions attempt to close these
	 * handles. If nvram_*() runs first, nvram=0, nvram gets closed
	*/
	if ((f = open("/dev/null", O_RDWR)) < 3) {
		dup(f);
		dup(f);
	}
	else
		close(f);

	base = strrchr(argv[0], '/');
	base = base ? base + 1 : argv[0];

	if (strcmp(base, "rc") == 0) {
		if (argc < 2)
			return 1;
		if (strcmp(argv[1], "start") == 0)
			return kill(1, SIGUSR2);
		if (strcmp(argv[1], "stop") == 0)
			return kill(1, SIGINT);
		if (strcmp(argv[1], "restart") == 0)
			return kill(1, SIGHUP);

		++argv;
		--argc;
		base = argv[0];
	}

#ifdef DEBUG_NOISY
	if (nvram_match("debug_logrc", "1")) {
		int i;

		cprintf("[rc %d] ", getpid());
		for (i = 0; i < argc; ++i) {
			cprintf("%s ", argv[i]);
		}
		cprintf("\n");

	}

	if (nvram_match("debug_ovrc", "1")) {
		char tmp[256];
		char *a[32];

		realpath(argv[0], tmp);
		if ((strncmp(tmp, "/tmp/", 5) != 0) && (argc < 32)) {
			memset(tmp, 0, sizeof(tmp));
			snprintf(tmp, sizeof(tmp), "%s%s", "/tmp/", base);
			if (f_exists(tmp)) {
				cprintf("[rc] override: %s\n", tmp);
				memcpy(a, argv, argc * sizeof(a[0]));
				a[argc] = 0;
				a[0] = tmp;
				execvp(tmp, a);
				exit(0);
			}
		}
	}
#endif /* DEBUG_NOISY */

	const applets_t *a;
	for (a = applets; a->name; ++a) {
		if (strcmp(base, a->name) == 0) {
			openlog(base, LOG_PID, LOG_USER);
			return a->main(argc, argv);
		}
	}

#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
	if (!strcmp(base, "nvram_erase")) {
		erase_nvram();
		return 0;
	}
	/* mtd-erase2 [device] */
	else if (!strcmp(base, "mtd-erase2")) {
		if (argv[1] && ((!strcmp(argv[1], "boot")) ||
				(!strcmp(argv[1], "linux")) ||
				(!strcmp(argv[1], "linux2")) ||
				(!strcmp(argv[1], "rootfs")) ||
				(!strcmp(argv[1], "rootfs2")) ||
				(!strcmp(argv[1], "brcmnand")) ||
				(!strcmp(argv[1], "nvram")) ||
				(!strcmp(argv[1], "crash")))) {
			return mtd_erase(argv[1]);
		}
		else {
			fprintf(stderr, "usage: mtd-erase2 [device]\n");
			return EINVAL;
		}
	}
	/* mtd-write2 [path] [device] */
	else if (!strcmp(base, "mtd-write2")) {
		if (argc >= 3)
			return mtd_write(argv[1], argv[2]);
		else {
			fprintf(stderr, "usage: mtd-write2 [path] [device]\n");
			return EINVAL;
		}
	}
#endif
#endif /* TOMATO64 */

	printf("Unknown applet: %s\n", base);
	return 0;
}
