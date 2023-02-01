/*
 * Tomato Firmware
 * Copyright (C) 2006-2010 Jonathan Zarate
 *
 */


#include "tomato.h"

#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define	V_NONE			VT_NONE,	{ }, 			{ }
#define V_01			VT_RANGE,	{ .l = 0 },		{ .l = 1 }
#define V_PORT			VT_RANGE,	{ .l = 2 },		{ .l = 65535 }
#define V_ONOFF			VT_LENGTH,	{ .i = 2 },		{ .i = 3 }
#define V_WORD			VT_LENGTH,	{ .i = 1 },		{ .i = 16 }
#define V_LENGTH(min, max)	VT_LENGTH,	{ .i = min },		{ .i = max }
#define V_TEXT(min, max)	VT_TEXT,	{ .i = min },		{ .i = max }
#define V_RANGE(min, max)	VT_RANGE,	{ .l = min },		{ .l = max }
#define V_IP			VT_IP,		{ },			{ }
#define	V_OCTET			VT_RANGE,	{ .l = 0 },		{ .l = 255 }
#define V_NUM			VT_RANGE,	{ .l = 0 },		{ .l = 0x7FFFFFFF }
#define	V_TEMP			VT_TEMP,	{ }, 			{ }
#ifdef TCONFIG_IPV6
#define V_IPV6(required)	VT_IPV6,	{ .i = required },	{ }
#endif

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"tomato_debug"


char *post_buf = NULL;
int rboot = 0;
extern int post;

#if defined(TCONFIG_BCMARM) || defined(TCONFIG_MIPSR2)
static void asp_discovery(int argc, char **argv);
#endif
static void asp_css(int argc, char **argv);
static void asp_resmsg(int argc, char **argv);
static void wo_tomato(char *url);
static void wo_update(char *url);
static void wo_service(char *url);
static void wo_shutdown(char *url);
static void wo_nvcommit(char *url);

typedef union {
	int i;
	long l;
	const char *s;
} nvset_varg_t;

typedef struct {
	const char *name;
	enum {
		VT_NONE,	/* no checking */
		VT_LENGTH,	/* check length of string */
		VT_TEXT,	/* strip \r, check length of string */
		VT_RANGE,	/* expect an integer, check range */
		VT_IP,		/* expect an ip address */
		VT_MAC,		/* expect a mac address */
#ifdef TCONFIG_IPV6
		VT_IPV6,	/* expect an ipv6 address */
#endif
		VT_TEMP		/* no checks, no commit */
	} vtype;
	nvset_varg_t va;
	nvset_varg_t vb;
} nvset_t;

typedef struct {
	const nvset_t *v;
	int write;
	int dirty;
} nv_list_t;

const aspapi_t aspapi[] = {
	{ "activeroutes",		asp_activeroutes		},
	{ "arplist",			asp_arplist			},
	{ "bandwidth",			asp_bandwidth			},
	{ "build_time",			asp_build_time			},
	{ "cgi_get",			asp_cgi_get			},
	{ "compmac",			asp_compmac			},
	{ "ctcount",			asp_ctcount			},
	{ "ctdump",			asp_ctdump			},
	{ "ctrate",			asp_ctrate			},
	{ "ddnsx",			asp_ddnsx			},
	{ "devlist",			asp_devlist			},
	{ "showlog",			asp_showsyslog			},
	{ "webmon",			asp_webmon			},
	{ "dhcpc_time",			asp_dhcpc_time			},
	{ "dns",			asp_dns				},
	{ "ident",			asp_ident			},
	{ "lanip",			asp_lanip			},
	{ "layer7",			asp_layer7			},
	{ "link_uptime",		asp_link_uptime			},
	{ "netdev",			asp_netdev			},

	{ "iptraffic",			asp_iptraffic			},
	{ "iptmon",			asp_iptmon			},

	{ "notice",			asp_notice			},
	{ "nv",				asp_nv				},
	{ "nvram",			asp_nvram 			},
	{ "nvramseq",			asp_nvramseq			},
	{ "nvstat",			asp_nvstat 			},
	{ "psup",			asp_psup			},
	{ "qrate",			asp_qrate			},
	{ "resmsg",			asp_resmsg			},
	{ "rrule",			asp_rrule			},
	{ "statfs",			asp_statfs			},
	{ "sysinfo",			asp_sysinfo			},
#ifdef TCONFIG_BCMARM
	{ "jiffies",			asp_jiffies			},
#endif
	{ "time",			asp_time			},
	{ "upnpinfo",			asp_upnpinfo			},
	{ "version",			asp_version			},
	{ "wanstatus",			asp_wanstatus			},
	{ "wanup",			asp_wanup			},
#ifdef TCONFIG_PPTPD
	{ "pptpd_userol",		asp_pptpd_userol		},
#endif
	{ "wlstats",			asp_wlstats			},
	{ "wlclient",			asp_wlclient			},
	{ "wlnoise",			asp_wlnoise			},
	{ "wlscan",			asp_wlscan			},
	{ "wlchannels",			asp_wlchannels			},
	{ "wlcountries",		asp_wlcountries			},
	{ "wlifaces",			asp_wlifaces			},
	{ "wlbands",			asp_wlbands			},
#ifdef TCONFIG_USB
	{ "usbdevices",			asp_usbdevices			},
#endif
#ifdef TCONFIG_SDHC
	{ "mmcid",			asp_mmcid			},	/* MMC Support */
#endif
	{ "etherstates",		asp_etherstates			},	/* Ethernet States */
	{ "anonupdate",			asp_anonupdate			},	/* Tomato update notification system */
#ifdef TCONFIG_IPV6
	{ "calc6rdlocalprefix",		asp_calc6rdlocalprefix		},
#endif
	{ "css",			asp_css				},
#if defined(TCONFIG_BCMARM) || defined(TCONFIG_MIPSR2)
	{ "discovery",			asp_discovery			},
#endif
#ifdef TCONFIG_STUBBY
	{ "stubby_presets",		asp_stubby_presets		},
#endif
#ifdef TCONFIG_DNSCRYPT
	{ "dnscrypt_presets",		asp_dnscrypt_presets		},
#endif
	{ NULL,				NULL				}
};

static const nvset_t nvset_list[] = {
/* basic-ident */
	{ "router_name",		V_LENGTH(0, 32)			},
	{ "wan_hostname",		V_LENGTH(0, 32)			},
	{ "wan_domain",			V_LENGTH(0, 32)			},

/* basic-time */
	{ "tm_tz",			V_LENGTH(1, 64)			},	/* PST8PDT */
	{ "tm_sel",			V_LENGTH(1, 64)			},	/* PST8PDT */
	{ "tm_dst",			V_01				},
	{ "ntp_updates",		V_RANGE(-1, 24)			},
	{ "ntp_server",			V_LENGTH(1, 150)		},	/* x y z */
	{ "ntpd_ready",			V_01				},	/* is ntp synced? */
	{ "ntpd_enable",		V_RANGE(0, 2)			},	/* ntpd server */
	{ "ntpd_server_redir",		V_01				},	/* intercept ntp requests */

/* basic-static */
	{ "dhcpd_static",		V_LENGTH(0, 108*251)		},	/* 108 (max chars per entry) x 250 entries */
	{ "dhcpd_static_only",		V_01				},

/* basic-ddns */
	{ "ddnsx0",			V_LENGTH(0, 2048)		},
	{ "ddnsx1",			V_LENGTH(0, 2048)		},
	{ "ddnsx0_cache",		V_LENGTH(0, 1)			},	/* only to clear */
	{ "ddnsx1_cache",		V_LENGTH(0, 1)			},
	{ "ddnsx_ip",			V_LENGTH(0, 32)			},
	{ "ddnsx_save",			V_01				},
	{ "ddnsx_refresh",		V_RANGE(0, 365)			},

/* basic-network */
	/* WAN */
	{ "wan_proto",			V_LENGTH(1, 16)			},	/* disabled, dhcp, static, pppoe, pptp, l2tp */
	{ "wan_ipaddr",			V_IP				},
	{ "wan_netmask",		V_IP				},
	{ "wan_gateway",		V_IP				},
	{ "wan_hwaddr",			V_TEXT(0, 17)			},	/* WAN interface MAC address */
	{ "wan_iface",			V_TEXT(0, 8)			},
	{ "wan_ifname",			V_TEXT(0, 8)			},
	{ "hb_server_ip",		V_LENGTH(0, 32)			},
	{ "wan_l2tp_server_ip",		V_LENGTH(0, 128)		},
	{ "wan_pptp_server_ip",		V_LENGTH(0, 128)		},
	{ "wan_pptp_dhcp",		V_01				},
	{ "wan_ppp_username",		V_LENGTH(0, 60)			},
	{ "wan_ppp_passwd",		V_LENGTH(0, 60)			},
	{ "wan_ppp_service",		V_LENGTH(0, 50)			},
	{ "wan_ppp_demand",		V_01				},
	{ "wan_ppp_demand_dnsip",	V_IP				},
	{ "wan_ppp_custom",		V_LENGTH(0, 256)		},
	{ "wan_ppp_idletime",		V_RANGE(0, 1440)		},
	{ "wan_ppp_redialperiod",	V_RANGE(1, 86400)		},
	{ "wan_ppp_mlppp",		V_01				},
	{ "wan_mtu_enable",		V_01				},
	{ "wan_mtu",			V_RANGE(576, 1500)		},
	{ "wan_modem_ipaddr",		V_IP				},
	{ "wan_pppoe_lei",		V_RANGE(1, 60)			},
	{ "wan_pppoe_lef",		V_RANGE(1, 10)			},
	{ "wan_sta",			V_LENGTH(0, 10)			},
	{ "wan_dns",			V_LENGTH(0, 50)			},	/* ip ip ip */
#ifdef TCONFIG_USB
	{ "wan_hilink_ip",		V_IP				},
	{ "wan_status_script",		V_01				},
#endif
	{ "wan_ckmtd",			V_LENGTH(1, 2)			},	/* check method: 1 - ping, 2 - traceroute, 3 - curl */
	{ "wan_ck_pause",		V_01				},	/* skip watchdog check for this wan */

#ifdef TCONFIG_MULTIWAN
	{ "mwan_num",			V_RANGE(1, 4)			},
#else
	{ "mwan_num",			V_RANGE(1, 2)			},
#endif
	{ "mwan_init",			V_01				},
	{ "mwan_cktime",		V_RANGE(0, 3600)		},
	{ "mwan_ckdst",			V_LENGTH(0, 64)			},
	{ "mwan_tune_gc",		V_01				},
	{ "mwan_state_init",		V_01				},
	{ "pbr_rules",			V_LENGTH(0, 2048)		},

	{ "wan_weight",			V_RANGE(0, 256)			},
	{ "wan_dns_auto",		V_01				},

	{ "wan2_proto",			V_LENGTH(1, 16)			},	/* disabled, dhcp, static, pppoe, pptp, l2tp */
	{ "wan2_weight",		V_RANGE(0, 256)			},
	{ "wan2_ipaddr",		V_IP				},
	{ "wan2_netmask",		V_IP				},
	{ "wan2_gateway",		V_IP				},
	{ "wan2_hwaddr",		V_TEXT(0, 17)			},	/* WAN interface MAC address */
	{ "wan2_iface",			V_TEXT(0, 8)			},
	{ "wan2_ifname",		V_TEXT(0, 8)			},
	{ "wan2_l2tp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan2_pptp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan2_pptp_dhcp",		V_01				},
	{ "wan2_ppp_username",		V_LENGTH(0, 60)			},
	{ "wan2_ppp_passwd",		V_LENGTH(0, 60)			},
	{ "wan2_ppp_service",		V_LENGTH(0, 50)			},
	{ "wan2_ppp_demand",		V_01				},
	{ "wan2_ppp_demand_dnsip",	V_IP				},
	{ "wan2_ppp_custom",		V_LENGTH(0, 256)		},
	{ "wan2_ppp_idletime",		V_RANGE(0, 1440)		},
	{ "wan2_ppp_redialperiod",	V_RANGE(1, 86400)		},
	{ "wan2_ppp_mlppp",		V_01				},
	{ "wan2_mtu_enable",		V_01				},
	{ "wan2_mtu",			V_RANGE(576, 1500)		},
	{ "wan2_modem_ipaddr",		V_IP				},
	{ "wan2_pppoe_lei",		V_RANGE(1, 60)			},
	{ "wan2_pppoe_lef",		V_RANGE(1, 10)			},
	{ "wan2_sta",			V_LENGTH(0, 10)			},
	{ "wan2_dns",			V_LENGTH(0, 50)			},	/* ip ip ip */
	{ "wan2_dns_auto",		V_01				},
#ifdef TCONFIG_USB
	{ "wan2_hilink_ip",		V_IP				},
	{ "wan2_status_script",		V_01				},
#endif
	{ "wan2_ckmtd",			V_LENGTH(1, 2)			},	/* check method: 1 - ping, 2 - traceroute, 3 - curl */
	{ "wan2_ck_pause",		V_01				},	/* skip watchdog check for this wan */

#ifdef TCONFIG_MULTIWAN
	{ "wan3_proto",			V_LENGTH(1, 16)			},	/* disabled, dhcp, static, pppoe, pptp, l2tp */
	{ "wan3_weight",		V_RANGE(0, 256)			},
	{ "wan3_ipaddr",		V_IP				},
	{ "wan3_netmask",		V_IP				},
	{ "wan3_gateway",		V_IP				},
	{ "wan3_hwaddr",		V_TEXT(0, 17)			},	/* WAN interface MAC address */
	{ "wan3_iface",			V_TEXT(0, 8)			},
	{ "wan3_ifname",		V_TEXT(0, 8)			},
	{ "wan3_l2tp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan3_pptp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan3_pptp_dhcp",		V_01				},
	{ "wan3_ppp_username",		V_LENGTH(0, 60)			},
	{ "wan3_ppp_passwd",		V_LENGTH(0, 60)			},
	{ "wan3_ppp_service",		V_LENGTH(0, 50)			},
	{ "wan3_ppp_demand",		V_01				},
	{ "wan3_ppp_demand_dnsip",	V_IP				},
	{ "wan3_ppp_custom",		V_LENGTH(0, 256)		},
	{ "wan3_ppp_idletime",		V_RANGE(0, 1440)		},
	{ "wan3_ppp_redialperiod",	V_RANGE(1, 86400)		},
	{ "wan3_ppp_mlppp",		V_01				},
	{ "wan3_mtu_enable",		V_01				},
	{ "wan3_mtu",			V_RANGE(576, 1500)		},
	{ "wan3_modem_ipaddr",		V_IP				},
	{ "wan3_pppoe_lei",		V_RANGE(1, 60)			},
	{ "wan3_pppoe_lef",		V_RANGE(1, 10)			},
	{ "wan3_sta",			V_LENGTH(0, 10)			},
	{ "wan3_dns",			V_LENGTH(0, 50)			},	/* ip ip ip */
	{ "wan3_dns_auto",		V_01				},
#ifdef TCONFIG_USB
	{ "wan3_hilink_ip",		V_IP				},
	{ "wan3_status_script",		V_01				},
#endif
	{ "wan3_ckmtd",			V_LENGTH(1, 2)			},	/* check method: 1 - ping, 2 - traceroute, 3 - curl */
	{ "wan3_ck_pause",		V_01				},	/* skip watchdog check for this wan */

	{ "wan4_proto",			V_LENGTH(1, 16)			},	/* disabled, dhcp, static, pppoe, pptp, l2tp */
	{ "wan4_weight",		V_RANGE(0, 256)			},
	{ "wan4_ipaddr",		V_IP				},
	{ "wan4_netmask",		V_IP				},
	{ "wan4_gateway",		V_IP				},
	{ "wan4_hwaddr",		V_TEXT(0, 17)			},	/* WAN interface MAC address */
	{ "wan4_iface",			V_TEXT(0, 8)			},
	{ "wan4_ifname",		V_TEXT(0, 8)			},
	{ "wan4_l2tp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan4_pptp_server_ip",	V_LENGTH(0, 128)		},
	{ "wan4_pptp_dhcp",		V_01				},
	{ "wan4_ppp_username",		V_LENGTH(0, 60)			},
	{ "wan4_ppp_passwd",		V_LENGTH(0, 60)			},
	{ "wan4_ppp_service",		V_LENGTH(0, 50)			},
	{ "wan4_ppp_demand",		V_01				},
	{ "wan4_ppp_demand_dnsip",	V_IP				},
	{ "wan4_ppp_custom",		V_LENGTH(0, 256)		},
	{ "wan4_ppp_idletime",		V_RANGE(0, 1440)		},
	{ "wan4_ppp_redialperiod",	V_RANGE(1, 86400)		},
	{ "wan4_ppp_mlppp",		V_01				},
	{ "wan4_mtu_enable",		V_01				},
	{ "wan4_mtu",			V_RANGE(576, 1500)		},
	{ "wan4_modem_ipaddr",		V_IP				},
	{ "wan4_pppoe_lei",		V_RANGE(1, 60)			},
	{ "wan4_pppoe_lef",		V_RANGE(1, 10)			},
	{ "wan4_sta",			V_LENGTH(0, 10)			},
	{ "wan4_dns",			V_LENGTH(0, 50)			},	/* ip ip ip */
	{ "wan4_dns_auto",		V_01				},
#ifdef TCONFIG_USB
	{ "wan4_hilink_ip",		V_IP				},
	{ "wan4_status_script",		V_01				},
#endif
	{ "wan4_ckmtd",			V_LENGTH(1, 2)			},	/* check method: 1 - ping, 2 - traceroute, 3 - curl */
	{ "wan4_ck_pause",		V_01				},	/* skip watchdog check for this wan */
#endif /* TCONFIG_MULTIWAN */

	/* LAN */
	{ "lan_ipaddr",			V_IP				},
	{ "lan_netmask",		V_IP				},
	{ "lan_gateway",		V_IP				},
	{ "lan_dns",			V_LENGTH(0, 50)			},	/* ip ip ip */

#if defined(TCONFIG_DNSSEC) || defined(TCONFIG_STUBBY)
	{ "dnssec_enable",		V_01				},
	{ "dnssec_method",		V_RANGE(0, 2)			},	/* 0=dnsmasq, 1=stubby, 2=server only */
#endif

#ifdef TCONFIG_DNSCRYPT
	{ "dnscrypt_proxy",		V_01				},
	{ "dnscrypt_priority",		V_RANGE(0, 2)			},	/* 0=none, 1=preferred, 2=exclusive */
	{ "dnscrypt_port",		V_PORT				},
	{ "dnscrypt_resolver",		V_LENGTH(0, 40)			},
	{ "dnscrypt_log",		V_RANGE(0, 99)			},
	{ "dnscrypt_manual",		V_01				},
	{ "dnscrypt_provider_name",	V_LENGTH(0, 60)			},
	{ "dnscrypt_provider_key",	V_LENGTH(0, 80)			},
	{ "dnscrypt_resolver_address",	V_LENGTH(0, 50)			},
	{ "dnscrypt_ephemeral_keys",	V_01				},
#endif
#ifdef TCONFIG_STUBBY
	{ "stubby_proxy",		V_01				},
	{ "stubby_priority",		V_RANGE(0, 2)			},	/* 0=none, 1=strict-order, 2=no-resolv */
	{ "stubby_port",		V_PORT				},
	{ "stubby_resolvers",		V_LENGTH(0, 1024)		},
	{ "stubby_force_tls13",		V_01				},	/* 0=TLS1.2, 1=TLS1.3 */
	{ "stubby_log",			V_RANGE(0, 7)			},
#endif
	{ "lan_state",			V_01				},
	{ "lan_desc",			V_01				},
	{ "lan_invert",			V_01				},
	{ "lan_dhcp",			V_01				},	/* DHCP client [0|1] - obtain a LAN (br0) IP via DHCP */
	{ "lan_proto",			V_WORD				},	/* static, dhcp */
	{ "dhcpd_startip",		V_LENGTH(0, 15)			},
	{ "dhcpd_endip",		V_LENGTH(0, 15)			},
	{ "dhcp_lease",			V_LENGTH(0, 5)			},
	{ "dhcp_moveip",		V_RANGE(0, 2)			},	/* GUI helper for automatic IP change */
	{ "wan_wins",			V_IP				},

#ifdef TCONFIG_USB
	/* 3G/4G MODEM */
	{ "wan_modem_pin",		V_LENGTH(0, 6)			},
	{ "wan_modem_dev",		V_LENGTH(0, 14)			},	/* /dev/ttyUSB0, /dev/cdc-wdm1... */
	{ "wan_modem_init",		V_LENGTH(0, 25)			},
	{ "wan_modem_apn",		V_LENGTH(0, 25)			},
	{ "wan_modem_speed",		V_LENGTH(0, 6)			},
	{ "wan_modem_band",		V_LENGTH(0, 16)			},	/* all - 7FFFFFFFFFFFFFFF, 800MHz - 80000, 1800MHz - 4, 2100MHz - 1, 2600MHz - 40 */
	{ "wan_modem_roam",		V_RANGE(0, 3)			},	/* 0 - not supported, 1 - supported, 2 - no change, 3 - roam only */
	{ "wan_modem_if",		V_LENGTH(0, 4)			},	/* eth2, eth1... */
	{ "wan_modem_type",		V_LENGTH(0, 15)			},	/* hilink, non-hilink, hw-ether, qmi_wwan */

	{ "wan2_modem_pin",		V_LENGTH(0, 6)			},
	{ "wan2_modem_dev",		V_LENGTH(0, 14)			},	/* /dev/ttyUSB0, /dev/cdc-wdm1... */
	{ "wan2_modem_init",		V_LENGTH(0, 25)			},
	{ "wan2_modem_apn",		V_LENGTH(0, 25)			},
	{ "wan2_modem_speed",		V_LENGTH(0, 6)			},
	{ "wan2_modem_band",		V_LENGTH(0, 16)			},	/* all - 7FFFFFFFFFFFFFFF, 800MHz - 80000, 1800MHz - 4, 2100MHz - 1, 2600MHz - 40 */
	{ "wan2_modem_roam",		V_RANGE(0, 3)			},	/* 0 - not supported, 1 - supported, 2 - no change, 3 - roam only */
	{ "wan2_modem_if",		V_LENGTH(0, 4)			},	/* eth2, eth1... */
	{ "wan2_modem_type",		V_LENGTH(0, 15)			},	/* hilink, non-hilink, hw-ether, qmi_wwan */

#ifdef TCONFIG_MULTIWAN
	{ "wan3_modem_pin",		V_LENGTH(0, 6)			},
	{ "wan3_modem_dev",		V_LENGTH(0, 14)			},	/* /dev/ttyUSB0, /dev/cdc-wdm1... */
	{ "wan3_modem_init",		V_LENGTH(0, 25)			},
	{ "wan3_modem_apn",		V_LENGTH(0, 25)			},
	{ "wan3_modem_speed",		V_LENGTH(0, 6)			},
	{ "wan3_modem_band",		V_LENGTH(0, 16)			},	/* all - 7FFFFFFFFFFFFFFF, 800MHz - 80000, 1800MHz - 4, 2100MHz - 1, 2600MHz - 40 */
	{ "wan3_modem_roam",		V_RANGE(0, 3)			},	/* 0 - not supported, 1 - supported, 2 - no change, 3 - roam only */
	{ "wan3_modem_if",		V_LENGTH(0, 4)			},	/* eth2, eth1... */
	{ "wan3_modem_type",		V_LENGTH(0, 15)			},	/* hilink, non-hilink, hw-ether, qmi_wwan */

	{ "wan4_modem_pin",		V_LENGTH(0, 6)			},
	{ "wan4_modem_dev",		V_LENGTH(0, 14)			},	/* /dev/ttyUSB0, /dev/cdc-wdm1... */
	{ "wan4_modem_init",		V_LENGTH(0, 25)			},
	{ "wan4_modem_apn",		V_LENGTH(0, 25)			},
	{ "wan4_modem_speed",		V_LENGTH(0, 6)			},
	{ "wan4_modem_band",		V_LENGTH(0, 16)			},	/* all - 7FFFFFFFFFFFFFFF, 800MHz - 80000, 1800MHz - 4, 2100MHz - 1, 2600MHz - 40 */
	{ "wan4_modem_roam",		V_RANGE(0, 3)			},	/* 0 - not supported, 1 - supported, 2 - no change, 3 - roam only */
	{ "wan4_modem_if",		V_LENGTH(0, 4)			},	/* eth2, eth1... */
	{ "wan4_modem_type",		V_LENGTH(0, 15)			},	/* hilink, non-hilink, hw-ether, qmi_wwan */
#endif
#endif

	/* LAN networks */
	{ "lan_ifname",			V_LENGTH(0, 5)			},

	{ "lan1_ifname",		V_LENGTH(0, 5)			},
	{ "lan1_ifnames",		V_TEXT(0, 64)			},
	{ "lan1_ipaddr",		V_LENGTH(0, 15)			},
	{ "lan1_netmask",		V_LENGTH(0, 15)			},
	{ "lan1_proto",			V_LENGTH(0, 6)			},
	{ "lan1_stp",			V_LENGTH(0, 1)			},
	{ "dhcpd1_startip",		V_LENGTH(0, 15)			},
	{ "dhcpd1_endip",		V_LENGTH(0, 15)			},
	{ "dhcp1_lease",		V_LENGTH(0, 5)			},

	{ "lan2_ifname",		V_LENGTH(0, 5)			},
	{ "lan2_ifnames",		V_TEXT(0, 64)			},
	{ "lan2_ipaddr",		V_LENGTH(0, 15)			},
	{ "lan2_netmask",		V_LENGTH(0, 15)			},
	{ "lan2_proto",			V_LENGTH(0, 6)			},
	{ "lan2_stp",			V_LENGTH(0, 1)			},
	{ "dhcpd2_startip",		V_LENGTH(0, 15)			},
	{ "dhcpd2_endip",		V_LENGTH(0, 15)			},
	{ "dhcp2_lease",		V_LENGTH(0, 5)			},

	{ "lan3_ifname",		V_LENGTH(0, 5)			},
	{ "lan3_ifnames",		V_TEXT(0, 64)			},
	{ "lan3_ipaddr",		V_LENGTH(0, 15)			},
	{ "lan3_netmask",		V_LENGTH(0, 15)			},
	{ "lan3_proto",			V_LENGTH(0, 6)			},
	{ "lan3_stp",			V_LENGTH(0, 1)			},
	{ "dhcpd3_startip",		V_LENGTH(0, 15)			},
	{ "dhcpd3_endip",		V_LENGTH(0, 15)			},
	{ "dhcp3_lease",		V_LENGTH(0, 5)			},

	/* Wireless */
	{ "wl_radio",			V_01				},
#if defined(TCONFIG_BCMARM) || defined(CONFIG_BCMWL6)
	{ "wl_mode",			V_LENGTH(2, 4)			},	/* ap, sta, wet, wds, psta */
#else
	{ "wl_mode",			V_LENGTH(2, 3)			},	/* ap, sta, wet, wds */
#endif
#ifdef CONFIG_BCMWL6
	{ "wl_net_mode",		V_LENGTH(5, 9)			},	/* disabled, mixed, b-only, g-only, bg-mixed, n-only, nac-mixed, ac-only [speedbooster] */
#else
	{ "wl_net_mode",		V_LENGTH(5, 8)			},	/* disabled, mixed, b-only, g-only, bg-mixed, n-only [speedbooster] */
#endif
	{ "wl_ssid",			V_LENGTH(1, 32)			},
	{ "wl_closed",			V_01				},
	{ "wl_channel",			V_RANGE(0, 216)			},

	{ "wl_vifs",			V_LENGTH(0, 64)			},	/* multiple/virtual BSSIDs */

	{ "wl_security_mode",		V_LENGTH(1, 32)			},	/* disabled, radius, wep, wpa_personal, wpa_enterprise, wpa2_personal, wpa2_enterprise */
	{ "wl_radius_ipaddr",		V_IP				},
	{ "wl_radius_port",		V_PORT				},
	{ "wl_radius_key",		V_LENGTH(1, 64)			},
	{ "wl_wep_bit",			V_RANGE(64, 128)		},	/* 64 or 128 */
	{ "wl_passphrase",		V_LENGTH(0, 20)			},
	{ "wl_key",			V_RANGE(1, 4)			},
	{ "wl_key1",			V_LENGTH(0, 26)			},
	{ "wl_key2",			V_LENGTH(0, 26)			},
	{ "wl_key3",			V_LENGTH(0, 26)			},
	{ "wl_key4",			V_LENGTH(0, 26)			},
	{ "wl_crypto",			V_LENGTH(3, 8)			},	/* tkip, aes, tkip+aes */
	{ "wl_wpa_psk",			V_LENGTH(8, 64)			},
	{ "wl_wpa_gtk_rekey",		V_RANGE(60, 7200)		},

	{ "wl_lazywds",			V_01				},
	{ "wl_wds",			V_LENGTH(0, 180)		},	/* mac mac mac (x 10) */

	{ "wl_wds_enable",		V_01				},
	{ "wl_gmode",			V_RANGE(-1, 6)			},
	{ "wl_wep",			V_LENGTH(1, 32)			},	/*  off, on, restricted,tkip,aes,tkip+aes */
	{ "wl_akm",			V_LENGTH(0, 32)			},	/*  wpa, wpa2, psk, psk2, wpa wpa2, psk psk2, "" */
	{ "wl_auth_mode",		V_LENGTH(4, 6)			},	/*  none, radius */
#ifdef TCONFIG_BCMARM
	{ "wl_mfp",			V_RANGE(0, 2)			},	/* Protected Management Frames: 0 - Disable, 1 - Capable, 2 - Required */
#endif
	{ "wl_nmode",			V_NONE				},
	{ "wl_nband",			V_RANGE(0, 2)			},	/* 2 - 2.4GHz, 1 - 5GHz, 0 - Auto */
	{ "wl_nreqd",			V_NONE				},
#if defined(TCONFIG_BCMARM) || defined(CONFIG_BCMWL6)
	{ "wl_nbw_cap",			V_RANGE(0, 3)			},	/* 0 - 20MHz, 1 - 40MHz, 2 - Auto, 3 - 80M */
#else
	{ "wl_nbw_cap",			V_RANGE(0, 2)			},	/* 0 - 20MHz, 1 - 40MHz, 2 - Auto */
#endif
#ifdef CONFIG_BCMWL6
	{ "wl_bss_opmode_cap_reqd",	V_RANGE(0, 3)			},	/* 0 - all possible, 1 - 11g cap., 2 - 11n cap., 3 - 11ac cap. */
#endif
#ifdef TCONFIG_ROAM
	{ "wl_user_rssi",		V_RANGE(-90, 0)			},	/* roaming assistant: disabled by default == 0 , GUI setting range: -90 ~ -45 */
#endif
	{ "wl_nbw",			V_NONE				},
	{ "wl_mimo_preamble",		V_WORD				},	/* 802.11n Preamble: mm/gf/auto/gfbcm */
	{ "wl_nctrlsb",			V_NONE				},	/* none, lower, upper */

#ifdef TCONFIG_IPV6
/* basic-ipv6 */
	{ "ipv6_service",		V_LENGTH(0, 16)			},	/* '', native, native-pd, 6to4, sit, other */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "ipv6_debug",			V_01				},	/* enable/show debug infos */
#endif
	{ "ipv6_duid_type",		V_RANGE(1, 4)			},	/* see RFC8415 Section 11; DUID-LLT = 1, DUID-EN = 2, DUID-LL = 3, DUID-UUID = 4 */
	{ "ipv6_prefix",		V_IPV6(0)			},
	{ "ipv6_prefix_length",		V_RANGE(3, 127)			},
	{ "ipv6_rtr_addr",		V_IPV6(0)			},
	{ "ipv6_radvd",			V_01				},
	{ "ipv6_dhcpd",			V_01				},
	{ "ipv6_lease_time",		V_RANGE(1, 720)			},	/* 1 ... up to 720 hours (30 days) IPv6 lease time */
	{ "ipv6_accept_ra",		V_NUM				},
	{ "ipv6_fast_ra",		V_01				},	/* fast RA option --> send frequent RAs */
	{ "ipv6_tun_addr",		V_IPV6(1)			},
	{ "ipv6_tun_addrlen",		V_RANGE(3, 127)			},
	{ "ipv6_ifname",		V_LENGTH(0, 8)			},
	{ "ipv6_tun_v4end",		V_IP				},
	{ "ipv6_relay",			V_RANGE(1, 254)			},
	{ "ipv6_tun_mtu",		V_NUM				},	/* Tunnel MTU */
	{ "ipv6_tun_ttl",		V_NUM				},	/* Tunnel TTL */
	{ "ipv6_dns",			V_LENGTH(0, 40*3)		},	/* ip6 ip6 ip6 */
	{ "ipv6_6rd_prefix",		V_IPV6(0)			},
	{ "ipv6_6rd_prefix_length",	V_RANGE(3, 127)			},
	{ "ipv6_6rd_borderrelay",	V_IP				},
	{ "ipv6_6rd_ipv4masklen",	V_RANGE(0, 32)			},
	{ "ipv6_vlan",			V_RANGE(0, 7)			},	/* Enable IPv6: bit 0 = LAN1, bit 1 = LAN2, bit 2 = LAN3 */
	{ "ipv6_isp_opt",		V_01				},	/* see router/rc/wan.c --> add default route ::/0 */
	{ "ipv6_pdonly",		V_01				},	/* Request DHCPv6 Prefix Delegation Only (send ia-pd and NO send ia-na) */
	{ "ipv6_pd_norelease",		V_01				},	/* DHCP6 client - no prefix/address release on exit */
	{ "ipv6_wan_addr",		V_IPV6(0)			},	/* Static IPv6 Wan Address */
	{ "ipv6_prefix_len_wan",	V_RANGE(3, 64)			},	/* Static IPv6 Wan Prefix Length */
	{ "ipv6_isp_gw",		V_IPV6(0)			},	/* Static IPv6 ISP Router Gateway */
#endif

/* basic-wfilter */
	{ "wl_macmode",			V_NONE				},	/* allow, deny, disabled */
	{ "wl_maclist",			V_LENGTH(0, 18*201)		},	/* 18 x 200		(11:22:33:44:55:66 ...) */
	{ "macnames",			V_LENGTH(0, 62*201)		},	/* 62 (12+1+48+1) x 50	(112233445566<..>)	todo: re-use */

/* advanced-ctnf */
	{ "ct_max",			V_NUM				},
	{ "ct_tcp_timeout",		V_LENGTH(20, 70)		},
	{ "ct_udp_timeout",		V_LENGTH(5, 15)			},
	{ "ct_timeout",			V_LENGTH(5, 15)			},
	{ "nf_ttl",			V_LENGTH(1, 6)			},
	{ "nf_l7in",			V_01				},
	{ "nf_sip",			V_01				},
	{ "ct_hashsize",		V_NUM				},
	{ "nf_rtsp",			V_01				},
	{ "nf_pptp",			V_01				},
	{ "nf_h323",			V_01				},
	{ "nf_ftp",			V_01				},
	{ "fw_nat_tuning",		V_RANGE(0, 2)			},	/* tcp/udp buffers: 0 - small (default), 1 - medium, 2 - large */

/* advanced-dhcpdns */
	{ "dhcpd_slt",			V_RANGE(-1, 43200)		},	/* -1=infinite, 0=follow normal lease time, >=1 custom */
	{ "dhcpd_dmdns",		V_01				},
	{ "dhcpd_lmax",			V_NUM				},
	{ "dhcpd_gwmode",		V_NUM				},
	{ "dns_addget",			V_01				},
	{ "dns_intcpt",			V_01				},
	{ "dhcpc_minpkt",		V_01				},
	{ "dhcpc_custom",		V_LENGTH(0, 256)		},
	{ "dns_norebind",		V_01				},
	{ "dns_fwd_local",		V_01				},	/* forward queries for local domain to upstream DNS server */
	{ "dns_priv_override",		V_01				},	/* override DoH */
	{ "dnsmasq_debug",		V_01				},
	{ "dnsmasq_custom",		V_TEXT(0, 4096)			},
	{ "dnsmasq_q",			V_RANGE(0, 7)			},	/* bitfield quiet bit0=dhcp, 1=dhcp6, 2=ra */
	{ "dnsmasq_gen_names",		V_01				},	/* generate a name for DHCP clients which do not otherwise have one */
	{ "dnsmasq_edns_size",		V_RANGE(512, 4096)		},	/* dnsmasq EDNS packet size (default 1280) */
#ifdef TCONFIG_TOR
	{ "dnsmasq_onion_support",	V_01				},
#endif
#ifdef TCONFIG_MDNS
	{ "mdns_enable",		V_01				},
	{ "mdns_reflector",		V_01				},
	{ "mdns_debug",			V_01				},
#endif

/* advanced-firewall */
	{ "block_wan",			V_01				},
	{ "block_wan_limit",		V_01				},
	{ "block_wan_limit_icmp",	V_RANGE(1, 300)			},
#ifdef TCONFIG_PROXY
	{ "multicast_pass",		V_01				},
	{ "multicast_lan",		V_01				},
	{ "multicast_lan1",		V_01				},
	{ "multicast_lan2",		V_01				},
	{ "multicast_lan3",		V_01				},
	{ "multicast_quickleave",	V_01				},
	{ "multicast_custom",		V_TEXT(0, 2048)			},
	{ "udpxy_enable",		V_01				},
	{ "udpxy_lan",			V_01				},
	{ "udpxy_lan1",			V_01				},
	{ "udpxy_lan2",			V_01				},
	{ "udpxy_lan3",			V_01				},
	{ "udpxy_stats",		V_01				},
	{ "udpxy_clients",		V_RANGE(1, 5000)		},
	{ "udpxy_port",			V_RANGE(0, 65535)		},
	{ "udpxy_wanface",		V_TEXT(0, 8)			},	/* alternative wanface */
#endif /* TCONFIG_PROXY */
	{ "block_loopback",		V_01				},
	{ "nf_loopback",		V_NUM				},
	{ "ne_syncookies",		V_01				},
	{ "DSCP_fix_enable",		V_01				},
	{ "ne_snat",			V_01				},
	{ "wan_dhcp_pass",		V_01				},
	{ "ipsec_pass",			V_RANGE(0, 3)			},	/* Enable IPSec Passthrough */
	{ "fw_blackhole",		V_01				},	/* MTU black hole detection */
#ifdef TCONFIG_EMF
	{ "emf_entry",			V_NONE				},
	{ "emf_uffp_entry",		V_NONE				},
	{ "emf_rtport_entry",		V_NONE				},
	{ "emf_enable",			V_01				},
#endif

/* advanced-adblock */
#ifdef TCONFIG_HTTPS
	{ "adblock_enable",		V_01				},
	{ "adblock_blacklist",		V_LENGTH(0, 4096)		},
	{ "adblock_blacklist_custom",	V_LENGTH(0, 4096)		},
	{ "adblock_whitelist",		V_LENGTH(0, 4096)		},
	{ "adblock_limit",		V_LENGTH(0, 32)			},
	{ "adblock_path",		V_LENGTH(0, 64)			},
#endif

/* advanced-misc */
#ifdef TCONFIG_BCMARM
	{ "wait_time",			V_RANGE(0, 30)			},
#else
	{ "wait_time",			V_RANGE(3, 20)			},
#endif
	{ "wan_speed",			V_RANGE(0, 4)			},
	{ "jumbo_frame_enable",		V_01				},	/* Jumbo Frames support (for RT-N16/WNR3500L) */
	{ "jumbo_frame_size",		V_RANGE(1, 9720)		},
#ifdef CONFIG_BCMWL5
	{ "ctf_disable",		V_01				},
#endif
#ifdef TCONFIG_BCMFA
	{ "ctf_fa_mode",		V_01				},
#endif
#ifdef TCONFIG_BCMNAT
	{ "bcmnat_disable",		V_01				},
#endif
/* advanced-vlan */
	{ "vlan0ports",			V_TEXT(0, 17)			},
	{ "vlan1ports",			V_TEXT(0, 17)			},
	{ "vlan2ports",			V_TEXT(0, 17)			},
	{ "vlan3ports",			V_TEXT(0, 17)			},
	{ "vlan4ports",			V_TEXT(0, 17)			},
	{ "vlan5ports",			V_TEXT(0, 17)			},
	{ "vlan6ports",			V_TEXT(0, 17)			},
	{ "vlan7ports",			V_TEXT(0, 17)			},
	{ "vlan8ports",			V_TEXT(0, 17)			},
	{ "vlan9ports",			V_TEXT(0, 17)			},
	{ "vlan10ports",		V_TEXT(0, 17)			},
	{ "vlan11ports",		V_TEXT(0, 17)			},
	{ "vlan12ports",		V_TEXT(0, 17)			},
	{ "vlan13ports",		V_TEXT(0, 17)			},
	{ "vlan14ports",		V_TEXT(0, 17)			},
	{ "vlan15ports",		V_TEXT(0, 17)			},
	{ "vlan0hwname",		V_TEXT(0, 8)			},
	{ "vlan1hwname",		V_TEXT(0, 8)			},
	{ "vlan2hwname",		V_TEXT(0, 8)			},
	{ "vlan3hwname",		V_TEXT(0, 8)			},
	{ "vlan4hwname",		V_TEXT(0, 8)			},
	{ "vlan5hwname",		V_TEXT(0, 8)			},
	{ "vlan6hwname",		V_TEXT(0, 8)			},
	{ "vlan7hwname",		V_TEXT(0, 8)			},
	{ "vlan8hwname",		V_TEXT(0, 8)			},
	{ "vlan9hwname",		V_TEXT(0, 8)			},
	{ "vlan10hwname",		V_TEXT(0, 8)			},
	{ "vlan11hwname",		V_TEXT(0, 8)			},
	{ "vlan12hwname",		V_TEXT(0, 8)			},
	{ "vlan13hwname",		V_TEXT(0, 8)			},
	{ "vlan14hwname",		V_TEXT(0, 8)			},
	{ "vlan15hwname",		V_TEXT(0, 8)			},
	{ "wan_ifnameX",		V_TEXT(0, 8)			},
	{ "wan2_ifnameX",		V_TEXT(0, 8)			},
	{ "wan3_ifnameX",		V_TEXT(0, 8)			},
	{ "wan4_ifnameX",		V_TEXT(0, 8)			},
	{ "lan_ifnames",		V_TEXT(0, 64)			},
	{ "manual_boot_nv",		V_01				},
#ifndef TCONFIG_BCMARM
	{ "trunk_vlan_so",		V_01				},
#endif
#if !defined(CONFIG_BCMWL6) && !defined(TCONFIG_BLINK) /* only mips RT branch */
	{ "vlan0tag",			V_TEXT(0, 5)			},
#endif
	{ "vlan0vid",			V_TEXT(0, 5)			},
	{ "vlan1vid",			V_TEXT(0, 5)			},
	{ "vlan2vid",			V_TEXT(0, 5)			},
	{ "vlan3vid",			V_TEXT(0, 5)			},
	{ "vlan4vid",			V_TEXT(0, 5)			},
	{ "vlan5vid",			V_TEXT(0, 5)			},
	{ "vlan6vid",			V_TEXT(0, 5)			},
	{ "vlan7vid",			V_TEXT(0, 5)			},
	{ "vlan8vid",			V_TEXT(0, 5)			},
	{ "vlan9vid",			V_TEXT(0, 5)			},
	{ "vlan10vid",			V_TEXT(0, 5)			},
	{ "vlan11vid",			V_TEXT(0, 5)			},
	{ "vlan12vid",			V_TEXT(0, 5)			},
	{ "vlan13vid",			V_TEXT(0, 5)			},
	{ "vlan14vid",			V_TEXT(0, 5)			},
	{ "vlan15vid",			V_TEXT(0, 5)			},

/* advanced-mac */
	{ "wan_mac",			V_LENGTH(0, 17)			},
	{ "wan2_mac",			V_LENGTH(0, 17)			},
#ifdef TCONFIG_MULTIWAN
	{ "wan3_mac",			V_LENGTH(0, 17)			},
	{ "wan4_mac",			V_LENGTH(0, 17)			},
#endif
	{ "wl_macaddr",			V_LENGTH(0, 17)			},
	{ "wl_hwaddr",			V_LENGTH(0, 17)			},

/* advanced-routing */
	{ "routes_static",		V_LENGTH(0, 2048)		},
	{ "dhcp_routes",		V_01				},
	{ "force_igmpv2",		V_01				},
	{ "lan_stp",			V_RANGE(0, 1)			},
#ifdef TCONFIG_ZEBRA
	{ "dr_setting",			V_RANGE(0, 3)			},
	{ "dr_lan_tx",			V_LENGTH(0, 32)			},
	{ "dr_lan_rx",			V_LENGTH(0, 32)			},
	{ "dr_lan1_tx",			V_LENGTH(0, 32)			},
	{ "dr_lan1_rx",			V_LENGTH(0, 32)			},
	{ "dr_lan2_tx",			V_LENGTH(0, 32)			},
	{ "dr_lan2_rx",			V_LENGTH(0, 32)			},
	{ "dr_lan3_tx",			V_LENGTH(0, 32)			},
	{ "dr_lan3_rx",			V_LENGTH(0, 32)			},
	{ "dr_wan_tx",			V_LENGTH(0, 32)			},
	{ "dr_wan_rx",			V_LENGTH(0, 32)			},
	{ "dr_wan2_tx",			V_LENGTH(0, 32)			},
	{ "dr_wan2_rx",			V_LENGTH(0, 32)			},
#ifdef TCONFIG_MULTIWAN
	{ "dr_wan3_tx",			V_LENGTH(0, 32)			},
	{ "dr_wan3_rx",			V_LENGTH(0, 32)			},
	{ "dr_wan4_tx",			V_LENGTH(0, 32)			},
	{ "dr_wan4_rx",			V_LENGTH(0, 32)			},
#endif
#endif /* TCONFIG_ZEBRA */

/* advanced-access */
	{ "lan_access",			V_LENGTH(0, 4096)		},

/* advanced-wireless */
	{ "wl_country_code",		V_LENGTH(0, 4)			},	/* Country code */
#if defined(TCONFIG_BCMARM) || defined(CONFIG_BCMWL6) || defined(TCONFIG_BLINK)
	{ "wl_country_rev",		V_RANGE(0, 999)			},	/* Country rev */
	{ "0:ccode",			V_LENGTH(0, 2)			},	/* Country code (short version) */
	{ "1:ccode",			V_LENGTH(0, 2)			},	/* Country code (short version) */
	{ "pci/1/1/ccode",		V_LENGTH(0, 2)			},	/* Country code (long version) */
	{ "pci/2/1/ccode",		V_LENGTH(0, 2)			},	/* Country code (long version) */
	{ "0:regrev",			V_RANGE(0, 999)			},	/* regrev (short version) */
	{ "1:regrev",			V_RANGE(0, 999)			},	/* regrev (short version) */
	{ "pci/1/1/regrev",		V_RANGE(0, 999)			},	/* regrev (long version) */
	{ "pci/2/1/regrev",		V_RANGE(0, 999)			},	/* regrev (long version) */
#ifdef TCONFIG_BLINK
	{ "sb/1/ccode",			V_LENGTH(0, 2)			},	/* Country code (SB) */	
	{ "sb/1/regrev",		V_RANGE(0, 999)			},	/* regrev (SB) */
#endif
#ifdef TCONFIG_AC3200
	{ "2:ccode",			V_LENGTH(0, 2)			},	/* Country code (short version) */
	{ "2:regrev",			V_RANGE(0, 999)			},	/* regrev (short version) */
	{ "pci/3/1/ccode",		V_LENGTH(0, 2)			},	/* Country code (long version) */
	{ "pci/3/1/regrev",		V_RANGE(0, 999)			},	/* regrev (long version) */
#endif
#endif /* TCONFIG_BCMARM || CONFIG_BCMWL6 || TCONFIG_BLINK */
	{ "wl_btc_mode",		V_RANGE(0, 2)			},	/* BT Coexistence Mode: 0 (disable), 1 (enable), 2 (preemption) */
	{ "wl_afterburner",		V_LENGTH(2, 4)			},	/* off, on, auto */
	{ "wl_auth",			V_01				},
	{ "wl_rateset",			V_LENGTH(2, 7)			},	/* all, default, 12 */
	{ "wl_rate",			V_RANGE(0, 54 * 1000 * 1000)	},
	{ "wl_mrate",			V_RANGE(0, 54 * 1000 * 1000)	},
	{ "wl_gmode_protection",	V_LENGTH(3, 4)			},	/* off, auto */
	{ "wl_frameburst",		V_ONOFF				},	/* off, on */
	{ "wl_bcn",			V_RANGE(1, 65535)		},
	{ "wl_dtim",			V_RANGE(1, 255)			},
	{ "wl_frag",			V_RANGE(256, 2346)		},
	{ "wl_rts",			V_RANGE(0, 2347)		},
	{ "wl_ap_isolate",		V_01				},
	{ "wl_plcphdr",			V_LENGTH(4, 5)			},	/* long, short */
	{ "wl_antdiv",			V_RANGE(0, 3)			},
	{ "wl_txant",			V_RANGE(0, 3)			},
#ifdef TCONFIG_BCMARM
	{ "wl_txpwr",			V_RANGE(0, 1000)		},
#else
	{ "wl_txpwr",			V_RANGE(0, 400)			},
#endif
	{ "wl_wme",			V_WORD				},	/* auto, off, on */
	{ "wl_wme_no_ack",		V_ONOFF				},	/* off, on */
	{ "wl_wme_apsd",		V_ONOFF				},	/* off, on */
	{ "wl_maxassoc",		V_RANGE(0, 255)			},
	{ "wl_bss_maxassoc",		V_RANGE(0, 255)			},
	{ "wl_distance",		V_LENGTH(0, 5)			},	/* "", 1-99999 */
	{ "wlx_hpamp",			V_01				},
	{ "wlx_hperx",			V_01				},
	{ "wl_reg_mode",		V_LENGTH(1, 3)			},	/* Regulatory: off, h, d */
	{ "wl_mitigation",		V_RANGE(0, 4)			},	/* NON-AC Interference Mitigation Mode (0|1|2|3|4) */
#ifdef CONFIG_BCMWL6
	{ "wl_mitigation_ac",		V_RANGE(0, 7)			},	/* AC Interference Mitigation Mode (bit mask (3 bits), values from 0 to 7) */
#endif
	{ "wl_nmode_protection",	V_WORD,				},	/* off, auto */
	{ "wl_nmcsidx",			V_RANGE(-2, 32),		},	/* -2 - 32 */
	{ "wl_obss_coex",		V_01				},
#ifdef TCONFIG_BCMARM
#ifdef TCONFIG_EMF
	{ "wl_igs",			V_01				},	/* BCM: sync with wl_wmf_bss_enable */
	{ "wl_wmf_bss_enable",		V_01				},	/* Wireless Multicast Forwarding Enable/Disable */
	{ "wl_wmf_ucigmp_query",	V_01				},	/* Disable Converting IGMP Query to ucast (default) */
	{ "wl_wmf_mdata_sendup",	V_01				},	/* Disable Sending Multicast Data to host (default) */
	{ "wl_wmf_ucast_upnp",		V_01				},	/* Disable Converting upnp to ucast (default) */
	{ "wl_wmf_igmpq_filter",	V_01				},	/* Disable igmp query filter */
#endif /* TCONFIG_EMF */
	{ "wl_atf",			V_01				},	/* Air Time Fairness support on = 1, off = 0 */
	{ "wl_turbo_qam",		V_RANGE(0, 2)			},	/* turbo qam on = 1 , off = 0, nitro qam = 2 */
	{ "wl_txbf",			V_01				},	/* Explicit Beamforming on = 1 , off = 0 (default: on) */
	{ "wl_txbf_bfr_cap",		V_RANGE(0, 2)			},	/* for Explicit Beamforming on = 1 , off = 0 (default: on - sync with wl_txbf), 2 for mu-mimo case */
	{ "wl_txbf_bfe_cap",		V_RANGE(0, 2)			},	/* for Explicit Beamforming on = 1 , off = 0 (default: on - sync with wl_txbf), 2 for mu-mimo case */
#ifdef TCONFIG_BCM714
	{ "wl_mu_features", 		V_LENGTH(0, 8)			},	/* mu_features=0x8000 when mu-mimo enabled */
	{ "wl_mumimo", 			V_01				},	/* mumimo on = 1, off = 0 */
#endif /* TCONFIG_BCM714 */
	{ "wl_itxbf",			V_01				},	/* Universal/Implicit Beamforming on = 1 , off = 0 (default: off) */
	{ "wl_txbf_imp",		V_01				},	/* for Universal/Implicit Beamforming on = 1 , off = 0 (default: off - sync with wl_itxbf) */
#ifdef TCONFIG_BCMBSD
	{ "smart_connect_x",		V_01				},	/* 0 = off, 1 = on (all-band), 2 = 5 GHz only! (no support, maybe later) */
#endif
#else /* TCONFIG_BCMARM */
	{ "wl_wmf_bss_enable",		V_01				},	/* Wireless Multicast Forwarding Enable/Disable */
#endif /* TCONFIG_BCMARM */

/* forward-dmz */
	{ "dmz_enable",			V_01				},
	{ "dmz_ipaddr",			V_LENGTH(0, 15)			},
	{ "dmz_sip",			V_LENGTH(0, 512)		},
	{ "dmz_ra",			V_01				},

/* forward-upnp */
	{ "upnp_enable",		V_NUM				},
	{ "upnp_secure",		V_01				},
	{ "upnp_port",			V_RANGE(0, 65535)		},
	{ "upnp_ssdp_interval",		V_RANGE(10, 9999)		},
	{ "upnp_mnp",			V_01				},
	{ "upnp_clean",			V_01				},
	{ "upnp_clean_interval",	V_RANGE(60, 65535)		},
	{ "upnp_clean_threshold",	V_RANGE(0, 9999)		},
	{ "upnp_min_port_int",		V_PORT				},
	{ "upnp_max_port_int",		V_PORT				},
	{ "upnp_min_port_ext",		V_PORT				},
	{ "upnp_max_port_ext",		V_PORT				},
	{ "upnp_lan",			V_01				},
	{ "upnp_lan1",			V_01				},
	{ "upnp_lan2",			V_01				},
	{ "upnp_lan3",			V_01				},
	{ "upnp_custom",		V_TEXT(0, 2048)			},

/* forward-basic */
	{ "portforward",		V_LENGTH(0, 4096)		},

#ifdef TCONFIG_IPV6
/* forward-basic-ipv6 */
	{ "ipv6_portforward",		V_LENGTH(0, 4096)		},
#endif

/* forward-triggered */
	{ "trigforward",		V_LENGTH(0, 4096)		},

/* access restriction */
	{ "rruleN",			V_RANGE(-1, 99)			},
//	{ "rrule##",			V_LENGTH(0, 2048)		},	/* in save_variables() */

/* admin-access */
	{ "http_enable",		V_01				},
#ifdef TCONFIG_HTTPS
	{ "https_enable",		V_01				},
	{ "https_crt_save",		V_01				},
	{ "https_crt_cn",		V_LENGTH(0, 64)			},
	{ "https_crt_gen",		V_TEMP				},
	{ "https_lanport",		V_PORT				},
	{ "remote_mgt_https",		V_01				},
#endif
	{ "remote_management",		V_01				},
	{ "remote_upgrade",		V_01				},
	{ "http_wanport_bfm",		V_01				},
	{ "http_lanport",		V_PORT				},
	{ "web_wl_filter",		V_01				},
	{ "web_css",			V_LENGTH(1, 32)			},
#ifdef TCONFIG_ADVTHEMES
	{ "web_adv_scripts",		V_01				},
#endif
	{ "web_dir",			V_LENGTH(1, 32)			},
	{ "ttb_css",			V_LENGTH(0, 128)		},
#ifdef TCONFIG_USB
	{ "ttb_loc",			V_LENGTH(0, 128)		},
	{ "ttb_url",			V_LENGTH(0, 128)		},
#endif
	{ "web_mx",			V_LENGTH(0, 128)		},
	{ "http_wanport",		V_PORT				},
	{ "telnetd_eas",		V_01				},
	{ "telnetd_port",		V_PORT				},
	{ "sshd_eas",			V_01				},
	{ "sshd_pass",			V_01				},
	{ "sshd_port",			V_PORT				},
	{ "sshd_remote",		V_01				},
	{ "sshd_motd",			V_01				},
	{ "sshd_forwarding",		V_01				},
	{ "sshd_rport", 		V_PORT				},
	{ "sshd_authkeys",		V_TEXT(0, 4096)			},
	{ "rmgt_sip",			V_LENGTH(0, 512)		},
	{ "ne_shlimit",			V_TEXT(1, 50)			},
	{ "http_username",		V_LENGTH(0, 32)			},

/* admin-bwm */
	{ "rstats_enable",		V_01				},
	{ "rstats_path",		V_LENGTH(0, 48)			},
	{ "rstats_stime",		V_RANGE(1, 168)			},
	{ "rstats_offset",		V_RANGE(1, 31)			},
	{ "rstats_exclude",		V_LENGTH(0, 64)			},
	{ "rstats_sshut",		V_01				},
	{ "rstats_bak",			V_01				},

/* admin-ipt */
	{ "cstats_enable",		V_01				},
	{ "cstats_path",		V_LENGTH(0, 48)			},
	{ "cstats_stime",		V_RANGE(1, 168)			},
	{ "cstats_offset",		V_RANGE(1, 31)			},
	{ "cstats_labels",		V_RANGE(0, 2)			},
	{ "cstats_exclude",		V_LENGTH(0, 512)		},
	{ "cstats_include",		V_LENGTH(0, 2048)		},
	{ "cstats_all",			V_01				},
	{ "cstats_sshut",		V_01				},
	{ "cstats_bak",			V_01				},

/* admin-buttons */
	{ "sesx_led",			V_RANGE(0, 255)			},	/* amber, white, aoss */
#ifdef TCONFIG_BCMARM
	{ "blink_wl",			V_01				},	/* turn blink on/off for wifi */
	{ "btn_led_mode",		V_01				},	/* Asus RT-AC68 Turbo Mode */
	{ "stealth_mode",		V_01				},
	{ "stealth_iled",		V_01				},
#endif
	{ "sesx_b0",			V_RANGE(0, 5)			},	/* 0-5: toggle wireless, reboot, shutdown, script, usb unmount */
	{ "sesx_b1",			V_RANGE(0, 5)			},	/* " */
	{ "sesx_b2",			V_RANGE(0, 5)			},	/* " */
	{ "sesx_b3",			V_RANGE(0, 5)			},	/* " */
	{ "sesx_script",		V_TEXT(0, 1024)			},
#ifndef TCONFIG_BCMARM
	{ "script_brau",		V_TEXT(0, 1024)			},
#endif

/* admin-debug */
	{ "debug_nocommit",		V_01				},
	{ "debug_cprintf",		V_01				},
	{ "debug_cprintf_file",		V_01				},
	{ "debug_logsegfault",		V_01				},
//	{ "debug_keepfiles",		V_01				},
	{ "debug_ddns",			V_01				},
	{ "debug_norestart",		V_TEXT(0, 128)			},
	{ "console_loglevel",		V_RANGE(1, 8)			},
	{ "t_cafree",			V_01				},
	{ "t_hidelr",			V_01				},
	{ "http_nocache",		V_01				},	/* disable cache in httpd? */

/* admin-sched */
	{ "sch_rboot", 			V_TEXT(0, 64)			},
	{ "sch_rcon", 			V_TEXT(0, 64)			},
	{ "sch_c1",			V_TEXT(0, 64)			},
	{ "sch_c1_cmd",			V_TEXT(0, 2048)			},
	{ "sch_c2",			V_TEXT(0, 64)			},
	{ "sch_c2_cmd",			V_TEXT(0, 2048)			},
	{ "sch_c3",			V_TEXT(0, 64)			},
	{ "sch_c3_cmd",			V_TEXT(0, 2048)			},
	{ "sch_c4",			V_TEXT(0, 64)			},
	{ "sch_c4_cmd",			V_TEXT(0, 2048)			},
	{ "sch_c5",			V_TEXT(0, 64)			},
	{ "sch_c5_cmd",			V_TEXT(0, 2048)			},

/* admin-scripts */
	{ "script_init", 		V_TEXT(0, 4096)			},
	{ "script_shut", 		V_TEXT(0, 4096)			},
	{ "script_fire", 		V_TEXT(0, 8192)			},
	{ "script_wanup", 		V_TEXT(0, 4096)			},
	{ "script_mwanup", 		V_TEXT(0, 4096)			},

/* admin-log */
	{ "log_remote",			V_01				},
	{ "log_remoteip",		V_LENGTH(0, 512)		},
	{ "log_remoteport",		V_PORT				},
	{ "log_file",			V_01				},
	{ "log_file_custom",		V_01				},
	{ "log_file_path",		V_TEXT(0, 4096)			},
	{ "log_file_size",		V_RANGE(0, 99999)		},
	{ "log_file_keep",		V_RANGE(0, 99)			},
	{ "log_limit",			V_RANGE(0, 2400)		},
	{ "log_in",			V_RANGE(0, 3)			},
	{ "log_out",			V_RANGE(0, 3)			},
	{ "log_mark",			V_RANGE(0, 99999)		},
	{ "log_events",			V_TEXT(0, 32)			},	/* "acre,crond,ntp" */
	{ "log_dropdups",		V_01				},	/* drop duplicates? */
	{ "log_min_level",		V_RANGE(1, 8)			},	/* minimum log level */

/* admin-log-webmonitor */
	{ "log_wm",			V_01				},
	{ "log_wmtype",			V_RANGE(0, 2)			},
	{ "log_wmip",			V_LENGTH(0, 512)		},
	{ "log_wmdmax",			V_RANGE(0, 9999)		},
	{ "log_wmsmax",			V_RANGE(0, 9999)		},
	{ "webmon_bkp",			V_01				},
	{ "webmon_dir",			V_LENGTH(0, 256)		},
	{ "webmon_shrink",		V_01				},


/* admin-cifs */
	{ "cifs1",			V_LENGTH(1, 1024)		},
	{ "cifs2",			V_LENGTH(1, 1024)		},

/* admin-jffs2 */
	{ "jffs2_on",			V_01				},
	{ "jffs2_exec",			V_LENGTH(0, 64)			},
	{ "jffs2_format",		V_01				},
	{ "jffs2_auto_unmount",		V_01				},	/* automatically unmount JFFS2 during FW upgrade */

#ifdef TCONFIG_SDHC
/* admin-sdhc */
	{ "mmc_on",			V_01				},
	{ "mmc_cs",			V_RANGE(1, 7)			},	/* GPIO pin */
	{ "mmc_clk",			V_RANGE(1, 7)			},	/* GPIO pin */
	{ "mmc_din",			V_RANGE(1, 7)			},	/* GPIO pin */
	{ "mmc_dout",			V_RANGE(1, 7)			},	/* GPIO pin */
	{ "mmc_fs_partition",		V_RANGE(1, 4)			},	/* partition number in partition table */
	{ "mmc_fs_type",		V_LENGTH(4, 4)			},	/* ext2, ext3, vfat */
	{ "mmc_exec_mount",		V_LENGTH(0, 64)			},
	{ "mmc_exec_umount",		V_LENGTH(0, 64)			},
#endif

/* admin-tomatoanon */
	{ "tomatoanon_answer",		V_RANGE(0, 1)			},
	{ "tomatoanon_enable",		V_RANGE(-1, 1)			},
	{ "tomatoanon_id",		V_LENGTH(0, 32)			},
	{ "tomatoanon_notify",		V_01				},

/* nas-usb */
#ifdef TCONFIG_USB
	{ "usb_enable",			V_01				},
	{ "usb_uhci",			V_RANGE(-1, 1)			},	/* -1 - disabled, 0 - off, 1 - on */
	{ "usb_ohci",			V_RANGE(-1, 1)			},
	{ "usb_usb2",			V_RANGE(-1, 1)			},
#ifdef TCONFIG_BCMARM
	{ "usb_usb3",			V_RANGE(-1, 1)			},
#endif
#ifdef TCONFIG_MICROSD
	{ "usb_mmc",			V_RANGE(-1, 1)			},
#endif
	{ "usb_irq_thresh",		V_RANGE(0, 6)			},
	{ "usb_storage",		V_01				},
	{ "usb_printer",		V_01				},
	{ "usb_printer_bidirect",	V_01				},
#ifdef TCONFIG_BCMARM
	{ "usb_fs_ext4",		V_01				},
#else
	{ "usb_fs_ext3",		V_01				},
#endif
	{ "usb_fs_fat",			V_01				},
#ifdef TCONFIG_BCMARM
	{ "usb_fs_exfat",		V_01				},
#endif
#ifdef TCONFIG_NTFS
	{ "usb_fs_ntfs",		V_01				},
#ifdef TCONFIG_BCMARM
	{ "usb_ntfs_driver",		V_LENGTH(0, 10)			},
#endif
#endif /* TCONFIG_NTFS */
#ifdef TCONFIG_UPS
	{ "usb_apcupsd",		V_01				},
	{ "usb_apcupsd_custom",		V_01				},	/* 1 - use custom config file /etc/apcupsd.conf */
#endif
#ifdef TCONFIG_HFS
	{ "usb_fs_hfs",			V_01				},
#ifdef TCONFIG_BCMARM
	{ "usb_hfs_driver",		V_LENGTH(0, 10)			},
#endif
#ifdef TCONFIG_ZFS
	{ "usb_fs_zfs",			V_01				},
	{ "usb_fs_zfs_automount",	V_01				},
	{ "zfs_mount_script",		V_TEXT(0, 2048)			},
#endif
#endif /* TCONFIG_HFS */
	{ "usb_automount",		V_01				},
	{ "script_usbhotplug", 		V_TEXT(0, 2048)			},
	{ "script_usbmount", 		V_TEXT(0, 2048)			},
	{ "script_usbumount", 		V_TEXT(0, 2048)			},
	{ "idle_enable",		V_01				},
	{ "usb_3g",			V_01				},
#endif /* TCONFIG_USB */

/* nas-ftp */
#ifdef TCONFIG_FTP
	{ "ftp_enable",			V_RANGE(0, 2)			},
	{ "ftp_super",			V_01				},
	{ "ftp_anonymous",		V_RANGE(0, 3)			},
	{ "ftp_dirlist",		V_RANGE(0, 2)			},
	{ "ftp_port",			V_PORT				},
	{ "ftp_max",			V_RANGE(0, 12)			},
	{ "ftp_ipmax",			V_RANGE(0, 12)			},
	{ "ftp_staytimeout",		V_RANGE(0, 65535)		},
	{ "ftp_rate",			V_RANGE(0, 99999)		},
	{ "ftp_anonrate",		V_RANGE(0, 99999)		},
	{ "ftp_anonroot",		V_LENGTH(0, 256)		},
	{ "ftp_pubroot",		V_LENGTH(0, 256)		},
	{ "ftp_pvtroot",		V_LENGTH(0, 256)		},
	{ "ftp_users",			V_LENGTH(0, 4096)		},
	{ "ftp_custom",			V_TEXT(0, 2048)			},
	{ "ftp_sip",			V_LENGTH(0, 512)		},
	{ "ftp_limit",			V_TEXT(1, 50)			},
	{ "ftp_tls",			V_01				},	/* support for basic ftp_tls */
	{ "log_ftp",			V_01				},
#endif

#ifdef TCONFIG_SNMP
	{ "snmp_enable",		V_RANGE(0, 1)			},
	{ "snmp_port",			V_RANGE(1, 65535)		},
	{ "snmp_remote",		V_RANGE(0, 1)			},
	{ "snmp_remote_sip",		V_LENGTH(0, 512)		},
	{ "snmp_location",		V_LENGTH(0, 40)			},
	{ "snmp_contact",		V_LENGTH(0, 40)			},
	{ "snmp_ro",			V_LENGTH(0, 40)			},
#endif

#ifdef TCONFIG_SAMBASRV
/* nas-samba */
	{ "smbd_enable",		V_RANGE(0, 2)			},
	{ "smbd_wgroup",		V_LENGTH(0, 20)			},
	{ "smbd_master",		V_01				},
	{ "smbd_wins",			V_01				},
	{ "smbd_cpage",			V_LENGTH(0, 4)			},
	{ "smbd_cset",			V_LENGTH(0, 20)			},
	{ "smbd_custom",		V_TEXT(0, 2048)			},
	{ "smbd_autoshare",		V_RANGE(0, 3)			},
	{ "smbd_shares",		V_LENGTH(0, 4096)		},
	{ "smbd_user",			V_LENGTH(0, 50)			},
	{ "smbd_passwd",		V_LENGTH(0, 50)			},
	{ "smbd_ifnames",		V_LENGTH(0, 50)			},
	{ "smbd_protocol",		V_RANGE(0, 2)			},
#ifdef TCONFIG_GROCTRL
	{ "gro_disable",		V_01				},
#endif
#endif

#ifdef TCONFIG_MEDIA_SERVER
/* nas-media */
	{ "ms_enable",			V_01				},
	{ "ms_dirs",			V_LENGTH(0, 1024)		},
	{ "ms_port",			V_RANGE(0, 65535)		},
	{ "ms_dbdir",			V_LENGTH(0, 256)		},
	{ "ms_ifname",			V_LENGTH(0, 256)		},
	{ "ms_tivo",			V_01				},
	{ "ms_stdlna",			V_01				},
	{ "ms_rescan",			V_01				},
	{ "ms_sas",			V_01				},
#endif

/* qos */
	{ "qos_enable",			V_01				},
#ifdef TCONFIG_BCMARM
	{ "qos_mode",			V_NUM				},
#endif
	{ "qos_ack",			V_01				},
	{ "qos_syn",			V_01				},
	{ "qos_fin",			V_01				},
	{ "qos_rst",			V_01				},
	{ "qos_icmp",			V_01				},
	{ "qos_udp",			V_01				},
	{ "qos_reset",			V_01				},
#ifdef TCONFIG_BCMARM
	{ "qos_pfifo",			V_NUM				},
	{ "qos_classify",		V_01				},
	{ "qos_cake_prio_mode",		V_NUM				},
	{ "qos_cake_wash",		V_01				},
#else
	{ "qos_pfifo",			V_01				},
#endif
	{ "wan_qos_obw",		V_RANGE(10, 99999999)		},
	{ "wan_qos_ibw",		V_RANGE(10, 99999999)		},
#ifdef TCONFIG_BCMARM
	{ "wan_qos_encap",		V_NUM				},
#endif
	{ "wan_qos_overhead",		V_RANGE(-127, 128)		},
	{ "wan2_qos_obw",		V_RANGE(10, 99999999)		},
	{ "wan2_qos_ibw",		V_RANGE(10, 99999999)		},
#ifdef TCONFIG_BCMARM
	{ "wan2_qos_encap",		V_NUM				},
#endif
	{ "wan2_qos_overhead",		V_RANGE(-127, 128)		},
#ifdef TCONFIG_MULTIWAN
	{ "wan3_qos_obw",		V_RANGE(10, 99999999)		},
	{ "wan3_qos_ibw",		V_RANGE(10, 99999999)		},
#ifdef TCONFIG_BCMARM
	{ "wan3_qos_encap",		V_NUM				},
#endif
	{ "wan3_qos_overhead",		V_RANGE(-127, 128)		},
	{ "wan4_qos_obw",		V_RANGE(10, 99999999)		},
	{ "wan4_qos_ibw",		V_RANGE(10, 99999999)		},
#ifdef TCONFIG_BCMARM
	{ "wan4_qos_encap",		V_NUM				},
#endif
	{ "wan4_qos_overhead",		V_RANGE(-127, 128)		},
#endif /* TCONFIG_MULTIWAN */
	{ "qos_orules",			V_LENGTH(0, 4096)		},
	{ "qos_default",		V_RANGE(0, 9)			},
#ifdef TCONFIG_MULTIWAN
	{ "qos_irates",			V_LENGTH(0, 256)		},
	{ "qos_orates",			V_LENGTH(0, 256)		},
#else
	{ "qos_irates",			V_LENGTH(0, 128)		},
	{ "qos_orates",			V_LENGTH(0, 128)		},
#endif
	{ "qos_classnames",		V_LENGTH(10, 128)		},
	{ "ne_vegas",			V_01				},
	{ "ne_valpha",			V_NUM				},
	{ "ne_vbeta",			V_NUM				},
	{ "ne_vgamma",			V_NUM				},

/* bwlimit */
	{ "bwl_enable",			V_01				},
	{ "bwl_rules",			V_LENGTH(0, 4096)		},
	{ "bwl_br0_enable",		V_01				},
	{ "bwl_br0_dlc",		V_RANGE(0, 99999999)		},
	{ "bwl_br0_ulc",		V_RANGE(0, 99999999)		},
	{ "bwl_br0_dlr",		V_RANGE(0, 99999999)		},
	{ "bwl_br0_ulr",		V_RANGE(0, 99999999)		},
	{ "bwl_br0_tcp",		V_RANGE(0, 1000)		},
	{ "bwl_br0_udp",		V_RANGE(0, 100)			},
	{ "bwl_br0_prio",		V_RANGE(0, 5)			},
	{ "bwl_br1_enable",		V_01				},
	{ "bwl_br1_dlc",		V_RANGE(0, 99999999)		},
	{ "bwl_br1_ulc",		V_RANGE(0, 99999999)		},
	{ "bwl_br1_dlr",		V_RANGE(0, 99999999)		},
	{ "bwl_br1_ulr",		V_RANGE(0, 99999999)		},
	{ "bwl_br1_prio",		V_RANGE(0, 5)			},
	{ "bwl_br2_enable",		V_01				},
	{ "bwl_br2_dlc",		V_RANGE(0, 99999999)		},
	{ "bwl_br2_ulc",		V_RANGE(0, 99999999)		},
	{ "bwl_br2_dlr",		V_RANGE(0, 99999999)		},
	{ "bwl_br2_ulr",		V_RANGE(0, 99999999)		},
	{ "bwl_br2_prio",		V_RANGE(0, 5)			},
	{ "bwl_br3_enable",		V_01				},
	{ "bwl_br3_dlc",		V_RANGE(0, 99999999)		},
	{ "bwl_br3_ulc",		V_RANGE(0, 99999999)		},
	{ "bwl_br3_dlr",		V_RANGE(0, 99999999)		},
	{ "bwl_br3_ulr",		V_RANGE(0, 99999999)		},
	{ "bwl_br3_prio",		V_RANGE(0, 5)			},

#ifdef TCONFIG_BT
/* nas-transmission */
	{ "bt_enable",			V_01				},
	{ "bt_binary",			V_LENGTH(0, 50)			},
	{ "bt_binary_custom",		V_LENGTH(0, 50)			},
	{ "bt_custom",			V_TEXT(0, 2048)			},
	{ "bt_port",			V_PORT				},
	{ "bt_dir",			V_LENGTH(0, 50)			},
	{ "bt_settings",		V_LENGTH(0, 50)			},
	{ "bt_settings_custom",		V_LENGTH(0, 50)			},
	{ "bt_incomplete",		V_01				},
	{ "bt_autoadd",			V_01				},
	{ "bt_rpc_enable",		V_01				},
	{ "bt_rpc_wan",			V_01				},
	{ "bt_auth",			V_01				},
	{ "bt_login",			V_LENGTH(0, 50)			},
	{ "bt_password",		V_LENGTH(0, 50)			},
	{ "bt_port_gui",		V_PORT				},
	{ "bt_dl_enable",		V_01				},
	{ "bt_ul_enable",		V_01				},
	{ "bt_dl",			V_RANGE(0, 999999)		},
	{ "bt_ul",			V_RANGE(0, 999999)		},
	{ "bt_peer_limit_global",	V_RANGE(10, 1000)		},
	{ "bt_peer_limit_per_torrent",	V_RANGE(1, 200)			},
	{ "bt_ul_slot_per_torrent",	V_RANGE(1, 50)			},
	{ "bt_ratio_enable",		V_01				},
	{ "bt_ratio",			V_LENGTH(0, 999999)		},
	{ "bt_ratio_idle_enable",	V_01				},
	{ "bt_ratio_idle",		V_RANGE(1, 55)			},
	{ "bt_dht",			V_01				},
	{ "bt_pex",			V_01				},
	{ "bt_lpd",			V_01				},
	{ "bt_utp",			V_01				},
	{ "bt_blocklist",		V_01				},
	{ "bt_blocklist_url",		V_LENGTH(0, 80)			},
	{ "bt_sleep",			V_RANGE(1, 60)			},
	{ "bt_check_time",		V_RANGE(0, 55)			},
	{ "bt_dl_queue_enable",		V_01				},
	{ "bt_dl_queue_size",		V_RANGE(1, 30)			},
	{ "bt_ul_queue_enable",		V_01				},
	{ "bt_ul_queue_size",		V_RANGE(1, 30)			},
	{ "bt_message",			V_RANGE(0, 3)			},
	{ "bt_log",			V_01				},
	{ "bt_log_path",		V_LENGTH(0, 50)			},
#endif

#ifdef TCONFIG_NFS
	{ "nfs_enable",			V_01				},
	{ "nfs_enable_v2",		V_01				},
	{ "nfs_exports",		V_LENGTH(0, 4096)		},
#endif

/* splashd */
#ifdef TCONFIG_NOCAT
	{ "NC_enable",			V_01				},
	{ "NC_Verbosity",		V_RANGE(0, 10)			},
	{ "NC_GatewayName",		V_LENGTH(0, 255)		},
	{ "NC_GatewayPort",		V_PORT				},
	{ "NC_ForcedRedirect",		V_01				},
	{ "NC_HomePage",		V_LENGTH(0, 255)		},
	{ "NC_DocumentRoot",		V_LENGTH(0, 255)		},
	{ "NC_SplashURL",		V_LENGTH(0, 255)		},
	{ "NC_LoginTimeout",		V_RANGE(0, 86400000)		},
	{ "NC_IdleTimeout",		V_RANGE(0, 86400000)		},
	{ "NC_MaxMissedARP",		V_RANGE(0, 10)			},
	{ "NC_PeerChecktimeout",	V_RANGE(0, 60)			},
	{ "NC_ExcludePorts",		V_LENGTH(0, 255)		},
	{ "NC_IncludePorts",		V_LENGTH(0, 255)		},
	{ "NC_AllowedWebHosts",		V_LENGTH(0, 255)		},
	{ "NC_MACWhiteList",		V_LENGTH(0, 255)		},
	{ "NC_SplashFile",		V_LENGTH(0, 8192)		},
	{ "NC_BridgeLAN",		V_LENGTH(0, 50)			},
#endif

/* web-nginx */
#ifdef TCONFIG_NGINX
	{"nginx_enable",		V_01				},	/* NGinX enabled */
	{"nginx_php",			V_01				},	/* PHP enabled */
	{"nginx_keepconf",		V_01				},	/* NGinX configuration files overwrite flag */
	{"nginx_docroot",		V_LENGTH(0, 255)		},	/* root files path */
	{"nginx_port",			V_PORT				},	/* listening port */
	{"nginx_fqdn",			V_LENGTH(0, 255)		},	/* server name */
	{"nginx_upload",		V_LENGTH(1, 1000)		},	/* upload file size limit */
	{"nginx_remote",		V_01				},
	{"nginx_priority",		V_LENGTH(0, 255)		},	/* server priority */
	{"nginx_custom",		V_TEXT(0, 4096)			},	/* user window to add parameters to nginx.conf */
	{"nginx_httpcustom",		V_TEXT(0, 4096)			},	/* user window to add parameters to nginx.conf */
	{"nginx_servercustom",		V_TEXT(0, 4096)			},	/* user window to add parameters to nginx.conf */
	{"nginx_phpconf",		V_TEXT(0, 4096)			},	/* user window to add parameters to php.ini */
	{"nginx_user",			V_LENGTH(0, 255)		},	/* user used to start nginx and spawn-fcgi */
	{"nginx_override",		V_01				},
	{"nginx_overridefile",		V_TEXT(0, 4096)			},
	{"nginx_h5aisupport",		V_01				},	/* enable h5ai support */

/* web-mysql */
	{ "mysql_enable",		V_01				},
	{ "mysql_sleep",		V_RANGE(1, 60)			},
	{ "mysql_check_time",		V_RANGE(0, 55)			},
	{ "mysql_binary",		V_LENGTH(0, 50)			},
	{ "mysql_binary_custom",	V_LENGTH(0, 50)			},
	{ "mysql_usb_enable",		V_01				},
	{ "mysql_dlroot",		V_LENGTH(0, 50)			},
	{ "mysql_datadir",		V_LENGTH(0, 64)			},
	{ "mysql_tmpdir",		V_LENGTH(0, 64)			},
	{ "mysql_server_custom",	V_TEXT(0, 1024)			},
	{ "mysql_port",			V_PORT				},
	{ "mysql_allow_anyhost",	V_01				},
	{ "mysql_init_rootpass",	V_01				},
	{ "mysql_username",		V_TEXT(0, 50)			},	/* mysqladmin username */
	{ "mysql_passwd",		V_TEXT(0, 50)			},	/* mysqladmin password */
	{ "mysql_key_buffer",		V_RANGE(0, 1024)		},	/* MB */
	{ "mysql_max_allowed_packet", 	V_RANGE(0, 1024)		},	/* MB */
	{ "mysql_thread_stack",		V_RANGE(0, 1024000)		},	/* KB */
	{ "mysql_thread_cache_size",	V_RANGE(0, 999999)		},
	{ "mysql_init_priv",		V_01				},
	{ "mysql_table_open_cache",	V_RANGE(1, 999999)		},
	{ "mysql_sort_buffer_size",	V_RANGE(0, 1024000)		},	/* KB */
	{ "mysql_read_buffer_size",	V_RANGE(0, 1024000)		},	/* KB */
	{ "mysql_query_cache_size",	V_RANGE(0, 1024)		},	/* MB */
	{ "mysql_read_rnd_buffer_size",	V_RANGE(0, 1024000)		},	/* KB */
	{ "mysql_net_buffer_length",	V_RANGE(0, 1024)		},	/* KB */
	{ "mysql_max_connections",	V_RANGE(0, 999999) 		},
#endif

#ifdef TCONFIG_OPENVPN
/* openvpn */
	{ "vpn_debug",			V_01				},
	{ "vpn_server_eas",		V_NONE				},
	{ "vpn_server_dns",		V_NONE				},
	{ "vpn_server1_poll",		V_RANGE(0, 30)			},
	{ "vpn_server1_if",		V_TEXT(3, 3)			},	/* tap, tun */
	{ "vpn_server1_proto",		V_TEXT(3, 11)			},	/* udp, tcp-server, udp4, tcp4-server, udp6, tcp6-server */
	{ "vpn_server1_port",		V_PORT				},
	{ "vpn_server1_firewall",	V_TEXT(0, 8)			},	/* auto, external, custom */
	{ "vpn_server1_crypt",		V_TEXT(0, 6)			},	/* tls, secret, custom */
	{ "vpn_server1_comp",		V_TEXT(0, 8)			},	/* yes, no, adaptive, lz4 */
	{ "vpn_server1_cipher",		V_TEXT(0, 16)			},
	{ "vpn_server1_ncp_ciphers",	V_TEXT(0, 128)			},
	{ "vpn_server1_digest",		V_TEXT(0, 15)			},
	{ "vpn_server1_dhcp",		V_01				},
	{ "vpn_server1_r1",		V_IP				},
	{ "vpn_server1_r2",		V_IP				},
	{ "vpn_server1_sn",		V_IP				},
	{ "vpn_server1_nm",		V_IP				},
	{ "vpn_server1_local",		V_IP				},
	{ "vpn_server1_remote",		V_IP				},
	{ "vpn_server1_reneg",		V_RANGE(-1, 2147483647)		},
	{ "vpn_server1_hmac",		V_RANGE(-1, 4)			},
	{ "vpn_server1_plan",		V_01				},
	{ "vpn_server1_plan1",		V_01				},
	{ "vpn_server1_plan2",		V_01				},
	{ "vpn_server1_plan3",		V_01				},
	{ "vpn_server1_pdns",		V_01				},
	{ "vpn_server1_rgw",		V_01				},
	{ "vpn_server1_userpass",	V_01				},
	{ "vpn_server1_nocert",		V_01				},
	{ "vpn_server1_users_val",	V_NONE				},
	{ "vpn_server1_custom",		V_NONE				},
	{ "vpn_server1_ccd",		V_01				},
	{ "vpn_server1_c2c",		V_01				},
	{ "vpn_server1_ccd_excl",	V_01				},
	{ "vpn_server1_ccd_val",	V_NONE				},
	{ "vpn_server1_static",		V_NONE				},
	{ "vpn_server1_ca",		V_NONE				},
	{ "vpn_server1_ca_key",		V_NONE				},
	{ "vpn_server1_crt",		V_NONE				},
	{ "vpn_server1_crl",		V_NONE				},	/* certificate revocation list */
	{ "vpn_server1_key",		V_NONE				},
	{ "vpn_server1_dh",		V_NONE				},
	{ "vpn_server1_br",		V_LENGTH(0, 50)			},
	{ "vpn_server2_poll",		V_RANGE(0, 30)			},
	{ "vpn_server2_if",		V_TEXT(3, 3)			},	/* tap, tun */
	{ "vpn_server2_proto",		V_TEXT(3, 11)			},	/* udp, tcp-server, udp4, tcp4-server, udp6, tcp6-server */
	{ "vpn_server2_port",		V_PORT				},
	{ "vpn_server2_firewall",	V_TEXT(0, 8)			},	/* auto, external, custom */
	{ "vpn_server2_crypt",		V_TEXT(0, 6)			},	/* tls, secret, custom */
	{ "vpn_server2_comp",		V_TEXT(0, 8)			},	/* yes, no, adaptive, lz4 */
	{ "vpn_server2_cipher",		V_TEXT(0, 16)			},
	{ "vpn_server2_ncp_ciphers",	V_TEXT(0, 128)			},
	{ "vpn_server2_digest",		V_TEXT(0, 15)			},
	{ "vpn_server2_dhcp",		V_01				},
	{ "vpn_server2_r1",		V_IP				},
	{ "vpn_server2_r2",		V_IP				},
	{ "vpn_server2_sn",		V_IP				},
	{ "vpn_server2_nm",		V_IP				},
	{ "vpn_server2_local",		V_IP				},
	{ "vpn_server2_remote",		V_IP				},
	{ "vpn_server2_reneg",		V_RANGE(-1, 2147483647)		},
	{ "vpn_server2_hmac",		V_RANGE(-1, 4)			},
	{ "vpn_server2_plan",		V_01				},
	{ "vpn_server2_plan1",		V_01				},
	{ "vpn_server2_plan2",		V_01				},
	{ "vpn_server2_plan3",		V_01				},
	{ "vpn_server2_pdns",		V_01				},
	{ "vpn_server2_rgw",		V_01				},
	{ "vpn_server2_userpass",	V_01				},
	{ "vpn_server2_nocert",		V_01				},
	{ "vpn_server2_users_val",	V_NONE				},
	{ "vpn_server2_custom",		V_NONE				},
	{ "vpn_server2_ccd",		V_01				},
	{ "vpn_server2_c2c",		V_01				},
	{ "vpn_server2_ccd_excl",	V_01				},
	{ "vpn_server2_ccd_val",	V_NONE				},
	{ "vpn_server2_static",		V_NONE				},
	{ "vpn_server2_ca",		V_NONE				},
	{ "vpn_server2_ca_key",		V_NONE				},
	{ "vpn_server2_crt",		V_NONE				},
	{ "vpn_server2_crl",		V_NONE				},	/* certificate revocation list */
	{ "vpn_server2_key",		V_NONE				},
	{ "vpn_server2_dh",		V_NONE				},
	{ "vpn_server2_br",		V_LENGTH(0, 50)			},
	{ "vpn_client_eas",		V_NONE				},
	{ "vpn_client1_poll",		V_RANGE(0, 30)			},
	{ "vpn_client1_if",		V_TEXT(3, 3)			},	/* tap, tun */
	{ "vpn_client1_bridge",		V_01				},
	{ "vpn_client1_nat",		V_01				},
	{ "vpn_client1_proto",		V_TEXT(3, 11)			},	/* udp, tcp-client, udp4, tcp4-client, udp6, tcp6-client */
	{ "vpn_client1_addr",		V_NONE				},
	{ "vpn_client1_port",		V_PORT				},
	{ "vpn_client1_retry",		V_RANGE(-1, 32767)		},	/* -1 infinite, 0 disabled, >= 1 custom */
	{ "vpn_client1_firewall",	V_TEXT(0, 6)			},	/* auto, custom */
	{ "vpn_client1_crypt",		V_TEXT(0, 6)			},	/* tls, secret, custom */
	{ "vpn_client1_comp",		V_TEXT(0, 8)			},	/* yes, no, adaptive, lz4 */
	{ "vpn_client1_cipher",		V_TEXT(0, 16)			},
	{ "vpn_client1_ncp_ciphers",	V_TEXT(0, 128)			},
	{ "vpn_client1_digest",		V_TEXT(0, 15)			},
	{ "vpn_client1_local",		V_IP				},
	{ "vpn_client1_remote",		V_IP				},
	{ "vpn_client1_nm",		V_IP				},
	{ "vpn_client1_reneg",		V_RANGE(-1, 2147483647)		},
	{ "vpn_client1_hmac",		V_RANGE(-1, 4)			},
	{ "vpn_client1_adns",		V_RANGE(0, 3)			},
	{ "vpn_client1_rgw",		V_RANGE(0, 3)			},
	{ "vpn_client1_gw",		V_TEXT(0, 15)			},
	{ "vpn_client1_custom",		V_NONE				},
	{ "vpn_client1_static",		V_NONE				},
	{ "vpn_client1_ca",		V_NONE				},
	{ "vpn_client1_crt",		V_NONE				},
	{ "vpn_client1_key",		V_NONE				},
	{ "vpn_client1_userauth",	V_01				},
	{ "vpn_client1_username",	V_TEXT(0, 50)			},
	{ "vpn_client1_password",	V_TEXT(0, 70)			},
	{ "vpn_client1_useronly",	V_01				},
	{ "vpn_client1_tlsremote",	V_01				},	/* remote-cert-tls server */
	{ "vpn_client1_tlsvername",	V_RANGE(0, 3)			},	/* verify-x509-name: 0 - disabled, 1 - Common Name, 2 - Common Name Prefix, 3 - Subject */
	{ "vpn_client1_cn",		V_NONE				},
	{ "vpn_client1_br",		V_LENGTH(0, 50)			},
	{ "vpn_client1_routing_val",	V_NONE				},
	{ "vpn_client1_fw",		V_01				},
	{ "vpn_client2_poll",		V_RANGE(0, 30)			},
	{ "vpn_client2_if",		V_TEXT(3, 3)			},	/* tap, tun */
	{ "vpn_client2_bridge",		V_01				},
	{ "vpn_client2_nat",		V_01				},
	{ "vpn_client2_proto",		V_TEXT(3, 11)			},	/* udp, tcp-client, udp4, tcp4-client, udp6, tcp6-client */
	{ "vpn_client2_addr",		V_NONE				},
	{ "vpn_client2_port",		V_PORT				},
	{ "vpn_client2_retry",		V_RANGE(-1, 32767)		},	/* -1 infinite, 0 disabled, >= 1 custom */
	{ "vpn_client2_firewall",	V_TEXT(0, 6)			},	/* auto, custom */
	{ "vpn_client2_crypt",		V_TEXT(0, 6)			},	/* tls, secret, custom */
	{ "vpn_client2_comp",		V_TEXT(0, 8)			},	/* yes, no, adaptive, lz4 */
	{ "vpn_client2_cipher",		V_TEXT(0, 16)			},
	{ "vpn_client2_ncp_ciphers",	V_TEXT(0, 128)			},
	{ "vpn_client2_digest",		V_TEXT(0, 15)			},
	{ "vpn_client2_local",		V_IP				},
	{ "vpn_client2_remote",		V_IP				},
	{ "vpn_client2_nm",		V_IP				},
	{ "vpn_client2_reneg",		V_RANGE(-1, 2147483647)		},
	{ "vpn_client2_hmac",		V_RANGE(-1, 4)			},
	{ "vpn_client2_adns",		V_RANGE(0, 3)			},
	{ "vpn_client2_rgw",		V_RANGE(0, 3)			},
	{ "vpn_client2_gw",		V_TEXT(0, 15)			},
	{ "vpn_client2_custom",		V_NONE				},
	{ "vpn_client2_static",		V_NONE				},
	{ "vpn_client2_ca",		V_NONE				},
	{ "vpn_client2_crt",		V_NONE				},
	{ "vpn_client2_key",		V_NONE				},
	{ "vpn_client2_userauth",	V_01				},
	{ "vpn_client2_username",	V_TEXT(0, 50)			},
	{ "vpn_client2_password",	V_TEXT(0, 70)			},
	{ "vpn_client2_useronly",	V_01				},
	{ "vpn_client2_tlsremote",	V_01				},	/* remote-cert-tls server */
	{ "vpn_client2_tlsvername",	V_RANGE(0, 3)			},	/* verify-x509-name: 0 - disabled, 1 - Common Name, 2 - Common Name Prefix, 3 - Subject */
	{ "vpn_client2_cn",		V_NONE				},
	{ "vpn_client2_br",		V_LENGTH(0, 50)			},
	{ "vpn_client2_routing_val",	V_NONE				},
	{ "vpn_client2_fw",		V_01				},
#ifdef TCONFIG_BCMARM
	{ "vpn_client3_poll",		V_RANGE(0, 30)			},
	{ "vpn_client3_if",		V_TEXT(3, 3)			},	/* tap, tun */
	{ "vpn_client3_bridge",		V_01				},
	{ "vpn_client3_nat",		V_01				},
	{ "vpn_client3_proto",		V_TEXT(3, 11)			},	/* udp, tcp-client, udp4, tcp4-client, udp6, tcp6-client */
	{ "vpn_client3_addr",		V_NONE				},
	{ "vpn_client3_port",		V_PORT				},
	{ "vpn_client3_retry",		V_RANGE(-1, 32767)		},	/* -1 infinite, 0 disabled, >= 1 custom */
	{ "vpn_client3_firewall",	V_TEXT(0, 6)			},	/* auto, custom */
	{ "vpn_client3_crypt",		V_TEXT(0, 6)			},	/* tls, secret, custom */
	{ "vpn_client3_comp",		V_TEXT(0, 8)			},	/* yes, no, adaptive, lz4 */
	{ "vpn_client3_cipher",		V_TEXT(0, 16)			},
	{ "vpn_client3_ncp_ciphers",	V_TEXT(0, 128)			},
	{ "vpn_client3_digest",		V_TEXT(0, 15)			},
	{ "vpn_client3_local",		V_IP				},
	{ "vpn_client3_remote",		V_IP				},
	{ "vpn_client3_nm",		V_IP				},
	{ "vpn_client3_reneg",		V_RANGE(-1, 2147483647)		},
	{ "vpn_client3_hmac",		V_RANGE(-1, 4)			},
	{ "vpn_client3_adns",		V_RANGE(0, 3)			},
	{ "vpn_client3_rgw",		V_RANGE(0, 3)			},
	{ "vpn_client3_gw",		V_TEXT(0, 15)			},
	{ "vpn_client3_custom",		V_NONE				},
	{ "vpn_client3_static",		V_NONE				},
	{ "vpn_client3_ca",		V_NONE				},
	{ "vpn_client3_crt",		V_NONE				},
	{ "vpn_client3_key",		V_NONE				},
	{ "vpn_client3_userauth",	V_01				},
	{ "vpn_client3_username",	V_TEXT(0, 50)			},
	{ "vpn_client3_password",	V_TEXT(0, 70)			},
	{ "vpn_client3_useronly",	V_01				},
	{ "vpn_client3_tlsremote",	V_01				},	/* remote-cert-tls server */
	{ "vpn_client3_tlsvername",	V_RANGE(0, 3)			},	/* verify-x509-name: 0 - disabled, 1 - Common Name, 2 - Common Name Prefix, 3 - Subject */
	{ "vpn_client3_cn",		V_NONE				},
	{ "vpn_client3_br",		V_LENGTH(0, 50)			},
	{ "vpn_client3_routing_val",	V_NONE				},
	{ "vpn_client3_fw",		V_01				},
#endif
#endif /* TCONFIG_OPENVPN */

#ifdef TCONFIG_PPTPD
/* pptp-server */
	{ "pptpd_enable",		V_01				},
	{ "pptpd_remoteip",		V_TEXT(0, 24)			},
	{ "pptpd_forcemppe",		V_01				},
	{ "pptpd_users",		V_TEXT(0, 67*16)		},
	{ "pptpd_broadcast",		V_TEXT(0, 8)			},
	{ "pptpd_dns1",			V_TEXT(0, 15)			},
	{ "pptpd_dns2",			V_TEXT(0, 15)			},
	{ "pptpd_wins1",		V_TEXT(0, 15)			},
	{ "pptpd_wins2",		V_TEXT(0, 15)			},
	{ "pptpd_mtu",			V_RANGE(576, 1500)		},
	{ "pptpd_mru",			V_RANGE(576, 1500)		},
	{ "pptpd_custom",		V_TEXT(0, 2048)			},
#endif

#ifdef TCONFIG_TINC
	{"tinc_enable",			V_RANGE(0, 1)			},
	{"tinc_name",			V_LENGTH(0, 30)			},
	{"tinc_devicetype",		V_TEXT(3, 3)			},	/* tun, tap */
	{"tinc_mode",			V_TEXT(3, 6)			},	/* switch, hub */
	{"tinc_vpn_netmask",		V_IP				},
	{"tinc_private_rsa",		V_NONE				},
	{"tinc_private_ed25519",	V_NONE				},
	{"tinc_custom",			V_NONE				},
	{"tinc_hosts",			V_NONE				},
	{"tinc_manual_firewall",	V_RANGE(0, 2)			},
	{"tinc_manual_tinc_up",		V_RANGE(0, 1)			},
	{"tinc_poll",			V_RANGE(0, 1440)		},
	/* scripts */
	{"tinc_tinc_up",		V_NONE				},
	{"tinc_tinc_down",		V_NONE				},
	{"tinc_host_up",		V_NONE				},
	{"tinc_host_down",		V_NONE				},
	{"tinc_subnet_up",		V_NONE				},
	{"tinc_subnet_down",		V_NONE				},
	{"tinc_firewall",		V_NONE				},
#endif

#ifdef TCONFIG_TOR
	{ "tor_enable",			V_01				},
	{ "tor_solve_only",		V_01				},
	{ "tor_socksport",		V_RANGE(1, 65535)		},
	{ "tor_transport",		V_RANGE(1, 65535)		},
	{ "tor_dnsport",		V_RANGE(1, 65535)		},
	{ "tor_datadir",		V_TEXT(0, 24)			},
	{ "tor_iface",			V_LENGTH(0, 50)			},
	{ "tor_users",			V_LENGTH(0, 4096)		},
	{ "tor_ports",			V_LENGTH(0, 50)			},
	{ "tor_ports_custom",		V_LENGTH(0, 4096)		},
	{ "tor_custom",			V_TEXT(0, 2048)			},
#endif

#ifdef TCONFIG_PPTPD
	{ "pptp_client_eas",		V_01				},
	{ "pptp_client_usewan",		V_TEXT(0, 5)			},
	{ "pptp_client_peerdns",	V_RANGE(0, 2)			},
	{ "pptp_client_mtuenable",	V_01				},
	{ "pptp_client_mtu",		V_RANGE(576, 1500)		},
	{ "pptp_client_mruenable",	V_01				},
	{ "pptp_client_mru",		V_RANGE(576, 1500)		},
	{ "pptp_client_nat",		V_01				},
	{ "pptp_client_srvip",		V_NONE				},
	{ "pptp_client_srvsub",		V_IP				},
	{ "pptp_client_srvsubmsk",	V_IP				},
	{ "pptp_client_username",	V_TEXT(0, 50)			},
	{ "pptp_client_passwd",		V_TEXT(0, 50)			},
	{ "pptp_client_crypt",		V_RANGE(0, 3)			},
	{ "pptp_client_custom",		V_NONE				},
	{ "pptp_client_dfltroute",	V_01				},
	{ "pptp_client_stateless",	V_01				},
	{ "pptpd_chap",			V_RANGE(0, 2)			},
#endif

	{ NULL }
};

void exec_service(const char *action)
{
	int i;

	logmsg(LOG_DEBUG, "*** [tomato] %s: exec_service: %s", __FUNCTION__, action);

	i = 10;
	while ((!nvram_match("action_service", "")) && (i-- > 0)) {
		logmsg(LOG_DEBUG, "*** [tomato] %s: waiting before %d", __FUNCTION__, i);
		sleep(1);
	}

	nvram_set("action_service", action);
	kill(1, SIGUSR1);

	i = 3;
	while ((nvram_match("action_service", (char *)action)) && (i-- > 0)) {
		logmsg(LOG_DEBUG, "*** [tomato] %s: waiting after %d", __FUNCTION__, i);
		sleep(1);
	}

/*
	if (atoi(webcgi_safeget("_service_wait", ""))) {
		i = 10;
		while ((nvram_match("action_service", (char *)action)) && (i-- > 0))  {
			logmsg(LOG_DEBUG, "*** [tomato] %s: waiting after %d", __FUNCTION__, i);
			sleep(1);
		}
	}
*/
}

static void wi_generic_noid(char *url, int len, char *boundary)
{
	if (post == 1) {
		if (len >= (32 * 1024)) {
			logmsg(LOG_WARNING, "POST length exceeded maximum allowed");
			exit(1);
		}

		if (post_buf) free(post_buf);
		if ((post_buf = malloc(len + 1)) == NULL) {
			logmsg(LOG_CRIT, "Unable to allocate post buffer");
			exit(1);
		}

		if (web_read_x(post_buf, len) != len)
			exit(1);

		post_buf[len] = 0;
		webcgi_init(post_buf);
	}
}

void wi_generic(char *url, int len, char *boundary)
{
	wi_generic_noid(url, len, boundary);
	check_id(url);
}

static void wi_cgi_bin(char *url, int len, char *boundary)
{
	if (post_buf)
		free(post_buf);

	post_buf = NULL;

	if (post) {
		if (len >= (128 * 1024)) {
			logmsg(LOG_WARNING, "POST length exceeded maximum allowed");
			exit(1);
		}

		if (len > 0) {
			if ((post_buf = malloc(len + 1)) == NULL) {
				logmsg(LOG_CRIT, "Unable to allocate post buffer");
				exit(1);
			}
			if (web_read_x(post_buf, len) != len)
				exit(1);

			post_buf[len] = 0;
		}
	}
}

#ifdef TCONFIG_TERMLIB
static void _execute_command(char *url, char *command, char *query, char *working_dir, wofilter_t wof)
#else
static void _execute_command(char *url, char *command, char *query, wofilter_t wof)
#endif
{
	char webExecFile[]  = "/tmp/.wxXXXXXX";
	char webQueryFile[] = "/tmp/.wqXXXXXX";
	char cmd[sizeof(webExecFile) + 10];
	FILE *f;
	int fe, fq = -1;

	if ((fe = mkstemp(webExecFile)) < 0)
		exit(1);

	if (query) {
		if ((fq = mkstemp(webQueryFile)) < 0) {
			close(fe);
			unlink(webExecFile);
			exit(1);
		}
	}

	if ((f = fdopen(fe, "wb")) != NULL) {
		fprintf(f,
			"#!/bin/sh\n"
			"export REQUEST_METHOD=\"%s\"\n"
			"export PATH=%s\n"
			". /etc/profile\n"
#ifdef TCONFIG_TERMLIB
			"cd %s\n"
#endif
			"%s%s %s%s\n",
			post ? "POST" : "GET", getenv("PATH"),
#ifdef TCONFIG_TERMLIB
			working_dir,
#endif
			command ? "" : "./", command ? command : url,
			query ? "<" : "", query ? webQueryFile : "");
		fclose(f);
	}
	else {
		close(fe);
		unlink(webExecFile);
		if (query) {
			close(fq);
			unlink(webQueryFile);
		}

		exit(1);
	}

	chmod(webExecFile, 0700);

	if (query) {
		if ((f = fdopen(fq, "wb")) != NULL) {
			fprintf(f, "%s\n", query);
			fclose(f);
		}
		else {
			unlink(webExecFile);
			close(fq);
			unlink(webQueryFile);
			exit(1);
		}
	}

	snprintf(cmd, sizeof(cmd), "%s 2>&1", webExecFile);
	web_pipecmd(cmd, wof);
	unlink(webQueryFile);
	unlink(webExecFile);
}

static void wo_cgi_bin(char *url)
{
	if (!header_sent) send_header(200, NULL, mime_html, 0);
#ifdef TCONFIG_TERMLIB
	_execute_command(url, NULL, post_buf, "/www", WOF_NONE);
#else
	_execute_command(url, NULL, post_buf, WOF_NONE);
#endif
	if (post_buf) {
		free(post_buf);
		post_buf = NULL;
	}
}

static void wo_shell(char *url)
{
#ifdef TCONFIG_TERMLIB
	if (atoi(webcgi_safeget("nojs", "0")))
		_execute_command(NULL, webcgi_get("command"), NULL, webcgi_safeget("working_dir", "/www"), WOF_NONE);
	else {
		web_puts("\ncmdresult = '");
		_execute_command(NULL, webcgi_get("command"), NULL, "/www", WOF_JAVASCRIPT);
		web_puts("';");
	}
#else
	web_puts("\ncmdresult = '");
	_execute_command(NULL, webcgi_get("command"), NULL, WOF_JAVASCRIPT);
	web_puts("';");
#endif
}

static void wo_cfe(char *url)
{
	do_file(MTD_DEV(0ro));
}

static void wo_nvram(char *url)
{
	web_pipecmd("nvram show", WOF_NONE);
}

static void wo_iptables(char *url)
{
	web_pipecmd("iptables -nvL; echo; iptables -t nat -nvL; echo; iptables -t mangle -nvL", WOF_NONE);
}

#ifdef TCONFIG_IPV6
static void wo_ip6tables(char *url)
{
	web_pipecmd("ip6tables -nvL; echo; ip6tables -t mangle -nvL", WOF_NONE);
}
#endif

/*
static void wo_spin(char *url)
{
	char s[64];

	strlcpy(s, nvram_safe_get("web_css"), sizeof(s));
	strlcat(s, "_spin.gif", sizeof(s));
	if (f_exists(s))
		do_file(s);
	else
		do_file("_spin.gif");
}
*/

void common_redirect(void)
{
	if (atoi(webcgi_safeget("_ajax", ""))) {
		send_header(200, NULL, mime_html, 0);
		web_puts("OK");
	}
	else
		redirect(webcgi_safeget("_redirect", "/"));
}

static void asp_css(int argc, char **argv)
{
	const char *css = nvram_safe_get("web_css");
	const char *ttb = nvram_safe_get("ttb_css");
	int c = strcmp(css, "tomato") != 0;

#ifdef TCONFIG_ADVTHEMES
	if (argc == 0) {
#endif
		if (nvram_match("web_css", "online"))
			web_printf("<link rel=\"stylesheet\" type=\"text/css\" href=\"/ext/%s.css\">", ttb);
		else {
			if (c)
				web_printf("<link rel=\"stylesheet\" type=\"text/css\" href=\"/%s.css\">", css);
		}
#ifdef TCONFIG_ADVTHEMES
	}
	else {
		if ((strncmp(argv[0], "svg-css", 7) == 0) && c)
			web_printf("<?xml-stylesheet type=\"text/css\" href=\"/%s.css\" ?>", css);	/* css for bwm-graph.svg */

		if ((strncmp(argv[0], "svg-js", 6) == 0) && (nvram_get_int("web_adv_scripts")))		/* special case, outer JS file for bwm-graph.svg */
			web_printf("<script href=\"/resize-charts.js\" />");
	}
#endif
}

#if defined(TCONFIG_BCMARM) || defined(TCONFIG_MIPSR2)
static void asp_discovery(int argc, char **argv)
{
	char buf[32] = "/usr/sbin/discovery.sh ";

	if (strncmp(argv[0], "off", 3) == 0)
		return;
	else if (strncmp(argv[0], "traceroute", 10) == 0)
		strcat(buf, argv[0]);

	system(buf);
}
#endif

static const char *resmsg_get(void)
{
	return webcgi_safeget("resmsg", "");
}

void resmsg_set(const char *msg)
{
	webcgi_set("resmsg", strdup(msg));
}

int resmsg_fread(const char *fname)
{
	char s[256];
	char *p;

	f_read_string(fname, s, sizeof(s));

	if ((p = strchr(s, '\n')) != NULL)
		*p = 0;

	if (s[0]) {
		resmsg_set(s);
		return 1;
	}

	return 0;
}

static void asp_resmsg(int argc, char **argv)
{
	char *p;

	if ((p = js_string(webcgi_safeget("resmsg", (argc > 0) ? argv[0] : ""))) == NULL)
		return;

	web_printf("\nresmsg='%s';\n", p);
	free(p);
}

static int webcgi_nvram_set(const nvset_t *v, const char *name, int write)
{
	char *p, *e;
	int n;
	long l;
	unsigned u[6];
	int ok;
	int dirty;
#ifdef TCONFIG_IPV6
	struct in6_addr addr;
#endif

	if ((p = webcgi_get((char*)name)) == NULL)
		return 0;

	logmsg(LOG_DEBUG, "*** [tomato] %s: [%s] %s=%s", __FUNCTION__, v->name, (char*)name, p);

	dirty = 0;
	ok = 1;
	switch (v->vtype) {
		case VT_TEXT:
			p = unix_string(p); /* NOTE: p = malloc'd */
			/* drop */
		case VT_LENGTH:
			n = strlen(p);
			if ((n < v->va.i) || (n > v->vb.i))
				ok = 0;
			break;
		case VT_RANGE:
			l = strtol(p, &e, 10);
			if ((p == e) || (*e) || (l < v->va.l) || (l > v->vb.l))
				ok = 0;
			break;
		case VT_IP:
			if ((sscanf(p, "%3u.%3u.%3u.%3u", &u[0], &u[1], &u[2], &u[3]) != 4) ||
			    (u[0] > 255) || (u[1] > 255) || (u[2] > 255) || (u[3] > 255))
				ok = 0;
			break;
		case VT_MAC:
			if ((sscanf(p, "%2x:%2x:%2x:%2x:%2x:%2x", &u[0], &u[1], &u[2], &u[3], &u[4], &u[5]) != 6) ||
			    (u[0] > 255) || (u[1] > 255) || (u[2] > 255) || (u[3] > 255) || (u[4] > 255) || (u[5] > 255))
				ok = 0;
			break;
#ifdef TCONFIG_IPV6
		case VT_IPV6:
			if (strlen(p) > 0 || v->va.i) {
				if (inet_pton(AF_INET6, p, &addr) != 1)
					ok = 0;
			}
			break;
#endif
		default:
			/* shutup gcc */
			break;
	}
	if (!ok) {
		if (v->vtype == VT_TEXT)
			free(p);

		return -1;
	}
	if (write) {
		if (!nvram_match((char *)name, p)) {
			if (v->vtype != VT_TEMP)
				dirty = 1;

			// logmsg(LOG_DEBUG, "*** [tomato] %s: nvram set %s=%s", __FUNCTION__, name, p);
			nvram_set(name, p);
		}
	}
	if (v->vtype == VT_TEXT)
		free(p);

	return dirty;
}

static int nv_wl_find(int idx, int unit, int subunit, void *param)
{
	nv_list_t *p = param;

	int ok = webcgi_nvram_set(p->v, wl_nvname(p->v->name + 3, unit, subunit), p->write);

	if (ok < 0)
		return 1;
	else {
		p->dirty |= ok;
		return 0;
	}
}

#if defined(TCONFIG_BCMARM) || defined(CONFIG_BCMWL6)
static int nv_wl_bwcap_chanspec(int idx, int unit, int subunit, void *param)
{
	char chan_spec[32];
	char *ch, *nbw_cap, *nctrlsb;
	int write = *((int *)param);
	ch      = webcgi_get(wl_nvname("channel", unit, 0));
	nbw_cap = webcgi_get(wl_nvname("nbw_cap", unit, 0));
	nctrlsb = webcgi_get(wl_nvname("nctrlsb", unit, 0));

	if (!ch && !nbw_cap && !nctrlsb)
		return 0;

	if (ch == NULL || !*ch)
		ch = nvram_get(wl_nvname("channel", unit, 0));

	if (nbw_cap == NULL || !*nbw_cap)
		nbw_cap = nvram_get(wl_nvname("nbw_cap", unit, 0));

	if (nctrlsb == NULL || !*nctrlsb)
		nctrlsb = nvram_get(wl_nvname("nctrlsb", unit, 0));

	if (!ch || !nbw_cap || !nctrlsb || !*ch || !*nbw_cap || !*nctrlsb)
		return 1;

	memset(chan_spec, 0, sizeof(chan_spec));
	strncpy(chan_spec, ch, sizeof(chan_spec));
	switch (atoi(nbw_cap)) {
		case 0:
			if (write)
				nvram_set(wl_nvname("bw_cap", unit, 0), "1");
			break;
		case 1:
			if (write)
				nvram_set(wl_nvname("bw_cap", unit, 0), "3");
			if (*ch != '0')
				*(chan_spec + strlen(chan_spec)) = *nctrlsb;
			break;
		case 3:
			if (write)
				nvram_set(wl_nvname("bw_cap", unit, 0), "7");
			if (*ch != '0')
				strcpy(chan_spec + strlen(chan_spec), "/80");
			break;
	}
	if (write)
		nvram_set(wl_nvname("chanspec", unit, 0), chan_spec);

	return 0;
}
#endif

static int save_variables(int write)
{
	const nvset_t *v;
	char *p;
	int n;
	int ok;
	char s[256], t[256];
	int dirty;
	static const char *msgf = "The field \"%s\" is invalid. Please report this problem.";
	nv_list_t nv;

	dirty = 0;
	nv.write = write;
	for (v = nvset_list; v->name; ++v) {
		ok = webcgi_nvram_set(v, v->name, write);

		if ((ok >= 0) && (strncmp(v->name, "wl_", 3) == 0)) {
			nv.dirty = dirty;
			nv.v = v;
			if (foreach_wif(1, &nv, nv_wl_find) == 0)
				ok |= nv.dirty;
			else
				ok = -1;
		}

		if (ok < 0) {
			snprintf(s, sizeof(s), msgf, v->name);
			resmsg_set(s);
			return 0;
		}
		dirty |= ok;
	}

	/* special cases */
#if defined(TCONFIG_BCMARM) || defined(CONFIG_BCMWL6)
	foreach_wif(0, &write, nv_wl_bwcap_chanspec);
#endif

	char *p1, *p2;
	if (((p1 = webcgi_get("set_password_1")) != NULL) && (strcmp(p1, "**********") != 0)) {
		if (((p2 = webcgi_get("set_password_2")) != NULL) && (strcmp(p1, p2) == 0)) {
			if ((write) && (!nvram_match("http_passwd", p1))) {
				dirty = 1;
				nvram_set("http_passwd", p1);
			}
		}
		else {
			snprintf(s, sizeof(s), msgf, "password");
			resmsg_set(s);
			return 0;
		}
	}

	for (n = 0; n < 50; ++n) {
		snprintf(s, sizeof(s), "rrule%d", n);
		if ((p = webcgi_get(s)) != NULL) {
			if (strlen(p) > 2048) {
				memset(t, 0, sizeof(t));
				strncpy(t, s, sizeof(s));
				snprintf(s, sizeof(s), msgf, t);
				resmsg_set(s);
				return 0;
			}
			if ((write) && (!nvram_match(s, p))) {
				dirty = 1;
				// logmsg(LOG_DEBUG, "*** [tomato] %s: nvram set %s=%s", __FUNCTION__, s, p);
				nvram_set(s, p);
			}
		}
	}

	return (write) ? dirty : 1;
}

static void wo_tomato(char *url)
{
	char *v;
	int i;
	int ajax;
	int nvset;
	const char *red;
	int commit;
	int force_commit;

	red = webcgi_safeget("_redirect", "");
	commit = atoi(webcgi_safeget("_commit", "1"));
	force_commit = atoi(webcgi_safeget("_force_commit", "0"));
	ajax = atoi(webcgi_safeget("_ajax", "0"));
	rboot = atoi(webcgi_safeget("_reboot", "0"));
	nvset = atoi(webcgi_safeget("_nvset", "1"));

	if (!*red)
		send_header(200, NULL, mime_html, 0);

	if (nvset) {
		if (!save_variables(0)) {
			if (ajax)
				web_printf("@msg:%s", resmsg_get());
			else
				parse_asp("error.asp");

			return;
		}
		commit = save_variables(1) && commit;

		resmsg_set("Settings saved.");
	}

	if (rboot)
		parse_asp("reboot.asp");
	else {
		if (ajax)
			web_printf("@msg:%s", resmsg_get());
		else if (atoi(webcgi_safeget("_moveip", "0")) || atoi(webcgi_safeget("dhcp_moveip", "0")))
			parse_asp("saved-moved.asp");
		else if (!*red)
			parse_asp("saved.asp");
	}

	if (commit || force_commit) {
		logmsg(LOG_DEBUG, "*** [tomato] %s: commit from tomato.cgi", __FUNCTION__);
		nvram_commit_x();
	}

	if ((v = webcgi_get("_service")) != NULL && *v != 0) {
		if (!*red) {
			if (ajax)
				web_printf(" Some services are being restarted...");

			web_close();
		}
		sleep(1);

		if (*v == '*')
			kill(1, SIGHUP);
		else
			exec_service(v);
	}

	for (i = atoi(webcgi_safeget("_sleep", "0")); i > 0; --i)
		sleep(1);

	if (*red)
		redirect(red);

	if (rboot) {
		web_close();
		sleep(1);
		kill(1, SIGTERM);
	}
}

static void wo_update(char *url)
{
	const aspapi_t *api;
	const char *name;
	int argc;
	char *argv[16];
	char s[32];

	if ((name = webcgi_get("exec")) != NULL) {
		for (api = aspapi; api->name; ++api) {
			if (strcmp(api->name, name) == 0) {
				for (argc = 0; argc < 16; ++argc) {
					snprintf(s, sizeof(s), "arg%d", argc);
					if ((argv[argc] = (char *)webcgi_get(s)) == NULL)
						break;
				}
				api->exec(argc, argv);
				break;
			}
		}
	}
}

static void wo_service(char *url)
{
	int n;

	exec_service(webcgi_safeget("_service", ""));

	if ((n = atoi(webcgi_safeget("_sleep", "2"))) <= 0)
		n = 2;

	sleep(n);

	common_redirect();
}

static void wo_shutdown(char *url)
{
	parse_asp("shutdown.asp");
	web_close();
	sleep(1);

	kill(1, SIGQUIT);
}

static void wo_nvcommit(char *url)
{
	parse_asp("saved.asp");
	web_close();
	nvram_commit();
}

const struct mime_handler mime_handlers[] = {
	{ "update.cgi",			mime_javascript,			0,	wi_generic,		wo_update,		1 },
	{ "tomato.cgi",			NULL,					0,	wi_generic,		wo_tomato,		1 },

	{ "cfe/*.bin",			mime_binary,				0,	wi_generic,		wo_cfe,			1 },
	{ "nvram/*.txt",		mime_binary,				0,	wi_generic,		wo_nvram,		1 },
	{ "ipt/*.txt",			mime_binary,				0,	wi_generic,		wo_iptables,		1 },
#ifdef TCONFIG_IPV6
	{ "ip6t/*.txt",			mime_binary,				0,	wi_generic,		wo_ip6tables,		1 },
#endif
	{ "cfg/*.cfg",			NULL,					0,	wi_generic,		wo_backup,		1 },
	{ "cfg/restore.cgi",		mime_html,				0,	wi_restore,		wo_restore,		1 },
	{ "cfg/defaults.cgi",		NULL,					0,	wi_generic,		wo_defaults,		1 },

	{ "stats/*.gz",			NULL,					0,	wi_generic,		wo_statsbackup,		1 },
	{ "stats/restore.cgi",		NULL,					0,	wi_statsrestore,	wo_statsrestore,	1 },

	{ "logs/view.cgi",		NULL,					0,	wi_generic,		wo_viewlog,		1 },
	{ "logs/*.txt",			NULL,					0,	wi_generic,		wo_syslog,		1 },
	{ "webmon_**",			NULL,					0,	wi_generic,		wo_syslog,		1 },

	{ "logout.asp",			NULL,					0,	wi_generic,		wo_asp,			1 },
	{ "clearcookies.asp",		NULL,					0,	wi_generic,		wo_asp,			1 },

//	{ "spin.gif",			NULL,					0,	wi_generic_noid,	wo_spin,		1 },

	{ "**.asp",			NULL,					0,	wi_generic_noid,	wo_asp,			1 },
	{ "**.css",			"text/css",				12,	wi_generic_noid,	do_file,		1 },
	{ "**.htm|**.html",		mime_html,				2,	wi_generic_noid,	do_file,		1 },
	{ "**.gif",			"image/gif",				12,	wi_generic_noid,	do_file,		1 },
	{ "**.jpg",			"image/jpeg",				12,	wi_generic_noid,	do_file,		1 },
	{ "**.png",			"image/png",				12,	wi_generic_noid,	do_file,		1 },
	{ "**.js",			mime_javascript,			12,	wi_generic_noid,	do_file,		1 },
	{ "**.jsx",			mime_javascript,			0,	wi_generic,		wo_asp,			1 },
	{ "**.jsz",			mime_javascript,			0,	wi_generic_noid,	wo_asp,			1 },
	{ "**.svg",			"image/svg+xml",			0,	wi_generic_noid,	wo_asp,			1 },
	{ "**.txt",			mime_plain,				2,	wi_generic_noid,	do_file,		1 },
	{ "**.bin",			mime_binary,				0,	wi_generic_noid,	do_file,		1 },
	{ "**.bino",			mime_octetstream,			0,	wi_generic_noid,	do_file,		1 },
	{ "favicon.ico",		"image/x-icon",				24,	wi_generic_noid,	do_file,		1 },
	{ "**/cgi-bin/**|**.sh",	NULL,					0,	wi_cgi_bin,		wo_cgi_bin,		1 },
	{ "**.tar|**.gz",		mime_binary,				0,	wi_generic_noid,	do_file,		1 },
	{ "shell.cgi",			mime_javascript,			0,	wi_generic,		wo_shell,		1 },
	{ "wpad.dat|proxy.pac",		"application/x-ns-proxy-autoconfig",	0,	wi_generic_noid,	do_file,		0 },

	{ "webmon.cgi",			mime_javascript,			0,	wi_generic,		wo_webmon,		1 },
	{ "dhcpc.cgi",			NULL,					0,	wi_generic,		wo_dhcpc,		1 },
	{ "dhcpd.cgi",			mime_javascript,			0,	wi_generic,		wo_dhcpd,		1 },
	{ "nvcommit.cgi",		NULL,					0,	wi_generic,		wo_nvcommit,		1 },
	{ "ping.cgi",			mime_javascript,			0,	wi_generic,		wo_ping,		1 },
	{ "trace.cgi",			mime_javascript,			0,	wi_generic,		wo_trace,		1 },
	{ "upgrade.cgi",		mime_html,				0,	wi_upgrade,		wo_flash,		1 },
	{ "upnp.cgi",			NULL,					0,	wi_generic,		wo_upnp,		1 },
	{ "wakeup.cgi",			NULL,					0,	wi_generic,		wo_wakeup,		1 },
	{ "wlradio.cgi",		NULL,					0,	wi_generic,		wo_wlradio,		1 },
	{ "resolve.cgi",		mime_javascript,			0,	wi_generic,		wo_resolve,		1 },
	{ "expct.cgi",			mime_html,				0,	wi_generic,		wo_expct,		1 },
	{ "service.cgi",		NULL,					0,	wi_generic,		wo_service,		1 },
	{ "shutdown.cgi",		mime_html,				0,	wi_generic,		wo_shutdown,		1 },
#ifdef TCONFIG_OPENVPN
	{ "vpnstatus.cgi",		mime_javascript,			0,	wi_generic,		wo_ovpn_status,		1 },
	{ "vpngenkey.cgi",		mime_javascript,			0,	wi_generic,		wo_ovpn_genkey,		1 },
#ifdef TCONFIG_KEYGEN
	{ "vpn/ClientConfig.tgz",	mime_binary,				0,	wi_generic,		wo_ovpn_genclientconfig,1 },
#endif
#endif
#ifdef TCONFIG_PPTPD
	{ "pptpd.cgi",			mime_javascript,			0,	wi_generic,		wo_pptpdcmd,		1 },
#endif
#ifdef TCONFIG_USB
	{ "usbcmd.cgi",			mime_javascript,			0,	wi_generic,		wo_usbcommand,		1 },
	{ "wwansignal.cgi",		mime_html,				0,	wi_generic,		wo_wwansignal,		1 },
	{ "wwansms.cgi",		mime_html,				0,	wi_generic,		wo_wwansms,		1 },
	{ "wwansmsdelete.cgi",		mime_html,				0,	wi_generic,		wo_wwansms_delete,	1 },
#endif
#ifdef TCONFIG_IPERF
	{ "iperfstatus.cgi",		mime_javascript,			0,	wi_generic,		wo_ttcpstatus,		1 },
	{ "iperfrun.cgi",		mime_javascript,			0,	wi_generic,		wo_ttcprun,		1 },
	{ "iperfkill.cgi",		mime_javascript,			0,	wi_generic,		wo_ttcpkill,		1 },
#endif
#ifdef BLACKHOLE
	{ "blackhole.cgi",		NULL,					0,	wi_blackhole,		NULL,			1 },
#endif
#ifdef TCONFIG_NOCAT
	{ "uploadsplash.cgi",		NULL,					0,	wi_uploadsplash,	wo_uploadsplash,	1 },
	{ "ext/uploadsplash.cgi",	NULL,					0,	wi_uploadsplash,	wo_uploadsplash,	1 },
#endif
	{ NULL,				NULL,					0,	NULL,			NULL,			1 }
};
