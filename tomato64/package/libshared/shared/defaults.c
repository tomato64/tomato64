/*
 *
 * Tomato Firmware
 * Copyright (C) 2006-2009 Jonathan Zarate
 *
 * Fixes/updates (C) 2018 - 2025 pedro
 *
 */

/*
 *
 * WARNING
 * Duplicate entries will cause segfaults.
 * Use ifdefs to ensure only one entry exists.
 *
 */


#include <tomato_config.h>
#include "tomato_profile.h"
#include <string.h>
#ifdef TCONFIG_BCMARM
 #include <stdio.h>
 #include <ctype.h>
 #include <wlioctl.h>
 #include <shutils.h>
 #include <bcmnvram.h>
#else
 #include "defaults.h"
#endif
#include <shared.h>
#if MWAN_MAX < 1 || MWAN_MAX > 8
 #error "Unsupported MWAN_MAX range"
#endif
#if BRIDGE_COUNT < 1 || BRIDGE_COUNT > 16
 #error "Unsupported BRIDGE_COUNT range"
#endif


struct nvram_tuple rstats_defaults[] = {
	{ "rstats_path",		""				, 0 },
	{ "rstats_stime",		"48"				, 0 },
	{ "rstats_offset",		"1"				, 0 },
	{ "rstats_data",		""				, 0 },
	{ "rstats_exclude",		""				, 0 },
	{ "rstats_sshut",		"1"				, 0 },
	{ "rstats_bak",			"0"				, 0 },
	{ 0, 0, 0 }
};

struct nvram_tuple cstats_defaults[] = {
	{ "cstats_path",		""				, 0 },
	{ "cstats_stime",		"48"				, 0 },
	{ "cstats_offset",		"1"				, 0 },
	{ "cstats_labels",		"0"				, 0 },
	{ "cstats_exclude",		""				, 0 },
	{ "cstats_include",		""				, 0 },
	{ "cstats_all",			"1"				, 0 },
	{ "cstats_sshut",		"1"				, 0 },
	{ "cstats_bak",			"0"				, 0 },
	{ 0, 0, 0 }
};

#ifdef TCONFIG_FTP
struct nvram_tuple ftp_defaults[] = {
	{ "ftp_super",			"0"				, 0 },
	{ "ftp_anonymous",		"0"				, 0 },
	{ "ftp_dirlist",		"0"				, 0 },
	{ "ftp_port",			"21"				, 0 },
	{ "ftp_max",			"0"				, 0 },
	{ "ftp_ipmax",			"0"				, 0 },
	{ "ftp_staytimeout",		"300"				, 0 },
	{ "ftp_rate",			"0"				, 0 },
	{ "ftp_anonrate",		"0"				, 0 },
	{ "ftp_anonroot",		""				, 0 },
	{ "ftp_pubroot",		""				, 0 },
	{ "ftp_pvtroot",		""				, 0 },
	{ "ftp_users",			""				, 0 },
	{ "ftp_custom",			""				, 0 },
	{ "ftp_sip",			""				, 0 },	/* wan ftp access: source ip address(es) */
	{ "ftp_limit",			"0,3,60"			, 0 },
	{ "ftp_tls",			"0"				, 0 },
	{ "log_ftp",			"0"				, 0 },
	{ 0, 0, 0 }
};
#endif /* TCONFIG_FTP */

#ifdef TCONFIG_SNMP
struct nvram_tuple snmp_defaults[] = {
	{ "snmp_port",			"161"				, 0 },
	{ "snmp_remote",		"0"				, 0 },
	{ "snmp_remote_sip",		""				, 0 },
	{ "snmp_location",		""				, 0 },
	{ "snmp_contact",		""				, 0 },
	{ "snmp_ro",			""				, 0 },
	{ "snmp_name",			""				, 0 },
	{ "snmp_descr",			""				, 0 },
	{ 0, 0, 0 }
};
#endif /* TCONFIG_SNMP */

#define BRIDGE_BLOCK_UPNP(i) \
	{ "upnp_lan" #i,		""				, 0 },

struct nvram_tuple upnp_defaults[] = {
	{ "upnp_secure",		"1"				, 0 },
	{ "upnp_port",			"0"				, 0 },
	{ "upnp_ssdp_interval",		"900"				, 0 },	/* SSDP interval */
	{ "upnp_custom",		""				, 0 },
	{ "upnp_lan",			""				, 0 },
#if BRIDGE_COUNT >= 2
 BRIDGE_BLOCK_UPNP(1)
#endif
#if BRIDGE_COUNT >= 3
 BRIDGE_BLOCK_UPNP(2)
#endif
#if BRIDGE_COUNT >= 4
 BRIDGE_BLOCK_UPNP(3)
#endif
#if BRIDGE_COUNT >= 5
 BRIDGE_BLOCK_UPNP(4)
#endif
#if BRIDGE_COUNT >= 6
 BRIDGE_BLOCK_UPNP(5)
#endif
#if BRIDGE_COUNT >= 7
 BRIDGE_BLOCK_UPNP(6)
#endif
#if BRIDGE_COUNT >= 8
 BRIDGE_BLOCK_UPNP(7)
#endif
#if BRIDGE_COUNT >= 9
 BRIDGE_BLOCK_UPNP(8)
#endif
#if BRIDGE_COUNT >= 10
 BRIDGE_BLOCK_UPNP(9)
#endif
#if BRIDGE_COUNT >= 11
 BRIDGE_BLOCK_UPNP(10)
#endif
#if BRIDGE_COUNT >= 12
 BRIDGE_BLOCK_UPNP(11)
#endif
#if BRIDGE_COUNT >= 13
 BRIDGE_BLOCK_UPNP(12)
#endif
#if BRIDGE_COUNT >= 14
 BRIDGE_BLOCK_UPNP(13)
#endif
#if BRIDGE_COUNT >= 15
 BRIDGE_BLOCK_UPNP(14)
#endif
#if BRIDGE_COUNT >= 16
 BRIDGE_BLOCK_UPNP(15)
#endif
#if 0	/* disabled for miniupnpd */
	{ "upnp_max_age",		"180"				, 0 },	/* Max age */
	{ "upnp_config",		"0"				, 0 },
#endif
	{ 0, 0, 0 }
};

#ifdef TCONFIG_BCMBSD
struct nvram_tuple bsd_defaults[] = {
	{ "bsd_role", 		 	"3"				, 0 },	/* Band Steer Daemon; 0:Disable, 1:Primary, 2:Helper, 3:Standalone */
	{ "bsd_hport", 		 	"9877"				, 0 },	/* BSD helper port */
	{ "bsd_pport", 		 	"9878"				, 0 },	/* BSD Primary port */
	{ "bsd_helper", 		"192.168.1.232"			, 0 },	/* BSD primary ipaddr */
	{ "bsd_primary", 		"192.168.1.231"			, 0 },	/* BSD Helper ipaddr */
#if 0
	{ "bsd_msglevel", 		"0x000010"			, 0 },	/* BSD_DEBUG_STEER */
	{ "bsd_dbg", 		 	"1"				, 0 },
#endif
#ifdef TCONFIG_AC3200 /* Tri-Band */
#ifdef TCONFIG_AC5300 /* Tri-Band */
	{"bsd_ifnames",			"eth1 eth2 eth3"		, 0 },
#else
	{"bsd_ifnames",			"eth2 eth1 eth3"		, 0 },
#endif
	{"wl0_bsd_steering_policy",	"0 5 3 -52 0 110 0x22"		, 0 },
	{"wl1_bsd_steering_policy",	"80 5 3 -82 0 0 0x20"		, 0 },
	{"wl2_bsd_steering_policy",	"0 5 3 -82 0 0 0x28"		, 0 },
	{"wl0_bsd_sta_select_policy",	"10 -52 0 110 0 1 1 0 0 0 0x122", 0 },
	{"wl1_bsd_sta_select_policy",	"10 -82 0 0 0 1 1 0 0 0 0x24"	, 0 },
	{"wl2_bsd_sta_select_policy",	"10 -82 0 0 0 1 1 0 0 0 0x28"	, 0 },
#ifdef TCONFIG_AC5300 /* Tri-Band */
	{"wl0_bsd_if_select_policy",	"eth3 eth2"			, 0 },
	{"wl1_bsd_if_select_policy",	"eth1 eth3"			, 0 },
	{"wl2_bsd_if_select_policy",	"eth1 eth2"			, 0 },
#else
	{"wl0_bsd_if_select_policy",	"eth3 eth1"			, 0 },
	{"wl1_bsd_if_select_policy",	"eth2 eth3"			, 0 },
	{"wl2_bsd_if_select_policy",	"eth2 eth1"			, 0 },
#endif
	{"wl0_bsd_if_qualify_policy",	"0 0x0"				, 0 },	/* bandwidth utilization disabled ; all clients possible (0x0) */
	{"wl1_bsd_if_qualify_policy",	"60 0x0"			, 0 },	/* bandwidth utilization is less than 60 % ; all clients possible (0x0) */
	{"wl2_bsd_if_qualify_policy",	"0 0x4"				, 0 },	/* bandwidth utilization disabled ; only AC clients possible (0x04) */
	{"bsd_bounce_detect",		"180 2 3600"			, 0 },
	{"bsd_aclist_timeout",		"3"				, 0 },
#else /* Dual-Band */
	{"bsd_ifnames",			"eth1 eth2"			, 0 },
	{"wl0_bsd_steering_policy",	"0 5 3 -52 0 110 0x22"		, 0 },	/* Steering Trigger Condition 2,4 GHz: RSSI greater than -52 OR PHYRATE (HIGH) greater than or equal to 110 Mbit/s */
	{"wl1_bsd_steering_policy",	"80 5 3 -82 0 0 0x20"		, 0 },	/* Steering Trigger Condition 5 GHz: RSSI less than or equal to -82 OR bandwidth use exceeds 80 % */
	{"wl0_bsd_sta_select_policy",	"10 -52 0 110 0 1 1 0 0 0 0x122", 0 },
	{"wl1_bsd_sta_select_policy",	"10 -82 0 0 0 1 1 0 0 0 0x20"	, 0 },
	{"wl0_bsd_if_select_policy",	"eth2"				, 0 },
	{"wl1_bsd_if_select_policy",	"eth1"				, 0 },
	{"wl0_bsd_if_qualify_policy",	"0 0x0"				, 0 },	/* bandwidth utilization disabled ; all clients possible (0x0) */
	{"wl1_bsd_if_qualify_policy",	"60 0x0"			, 0 },	/* bandwidth utilization is less than 60 % ; all clients possible (0x0) */
	{"bsd_bounce_detect",		"180 2 3600"			, 0 },
	{"bsd_aclist_timeout",		"3"				, 0 },
#endif /* TCONFIG_AC3200 */
	{"bsd_scheme",			"2"				, 0 },
	{ 0, 0, 0 }
};
#endif /* TCONFIG_BCMBSD */

#define WAN_BLOCK_CORE(i) \
	/* always: */ \
	{ "wan" #i "_proto",		"disabled"			, 0 }, /* disabled, dhcp, static, pppoe, pptp, l2tp */ \
	{ "wan" #i "_weight",		"1"				, 0 }, \
	{ "wan" #i "_ipaddr",		"0.0.0.0"			, 0 }, \
	{ "wan" #i "_netmask",		"0.0.0.0"			, 0 }, \
	{ "wan" #i "_gateway",		"0.0.0.0"			, 0 }, \
	{ "wan" #i "_hwname",		""				, 0 }, /* WAN driver name (e.g. et1) */ \
	{ "wan" #i "_hwaddr",		""				, 0 }, /* WAN interface MAC address */ \
	{ "wan" #i "_iface",		""				, 0 }, \
	{ "wan" #i "_ifname",		""				, 0 }, \
	{ "wan" #i "_l2tp_server_ip",	""				, 0 }, \
	{ "wan" #i "_pptp_server_ip",	""				, 0 }, \
	{ "wan" #i "_pptp_dhcp",	"0"				, 0 }, \
	{ "wan" #i "_ppp_username",	""				, 0 }, \
	{ "wan" #i "_ppp_passwd",	""				, 0 }, \
	{ "wan" #i "_ppp_service",	""				, 0 }, \
	{ "wan" #i "_ppp_demand",	"0"				, 0 }, \
	{ "wan" #i "_ppp_demand_dnsip",	"198.51.100.1"			, 0 }, \
	{ "wan" #i "_ppp_custom",	""				, 0 }, \
	{ "wan" #i "_ppp_idletime",	"5"				, 0 }, \
	{ "wan" #i "_ppp_redialperiod",	"20"				, 0 }, \
	{ "wan" #i "_mtu_enable",	"0"				, 0 }, \
	{ "wan" #i "_mtu",		"1500"				, 0 }, \
	{ "wan" #i "_modem_ipaddr",	"0.0.0.0"			, 0 }, \
	{ "wan" #i "_pppoe_lei",	"10"				, 0 }, \
	{ "wan" #i "_pppoe_lef",	"5"				, 0 }, \
	{ "wan" #i "_dns",		""				, 0 }, /* ip ip ip */ \
	{ "wan" #i "_dns_auto",		"1"				, 0 }, \
	{ "wan" #i "_addget",		"0"				, 0 }, \
	{ "wan" #i "_ckmtd",		"2"				, 0 }, /* check method: 1 - ping, 2 - traceroute, 3 - curl */ \
	{ "wan" #i "_ck_pause",		"0"				, 0 }, /* skip mwwatchdog check for this wan */ \
	{ "wan" #i "_mac",		""				, 0 }, \
	{ "wan" #i "_qos_obw",		"700"				, 0 }, \
	{ "wan" #i "_qos_ibw",		"16000"				, 0 }, \
	{ "wan" #i "_qos_overhead",	"0"				, 0 }, \
/* #ifdef TOMATO64 */ \
	{ "wan" #i "_ifnameX_vlan",	""				, 0 }, \
/* #endif TOMATO64 */ \
	{ "wan" #i "_ifnameX",		NULL				, 0 },
#ifdef TCONFIG_BCMARM
 #define WAN_BLOCK_BCMARM(i) \
	{ "wan" #i "_qos_encap",	"0"				, 0 },
#else
 #define WAN_BLOCK_BCMARM(i)
#endif
#ifdef TCONFIG_USB
 #define WAN_BLOCK_USB(i) \
	{ "wan" #i "_modem_pin",	""				, 0 }, \
	{ "wan" #i "_modem_dev",	""				, 0 }, /* /dev/ttyUSB0, /dev/cdc-wdm1... */ \
	{ "wan" #i "_modem_init",	"*99#"				, 0 }, \
	{ "wan" #i "_modem_apn",	"internet"			, 0 }, \
	{ "wan" #i "_modem_speed",	"00"				, 0 }, \
	{ "wan" #i "_modem_band",	"7FFFFFFFFFFFFFFF"		, 0 }, /* all - 7FFFFFFFFFFFFFFF, 800MHz - 80000, 1800MHz - 4, 2100MHz - 1, 2600MHz - 40 */ \
	{ "wan" #i "_modem_roam",	"2"				, 0 }, /* 0 not supported, 1 supported, 2 no change, 3 roam only */ \
	{ "wan" #i "_modem_type",	""				, 0 }, /* hilink, non-hilink, hw-ether, qmi_wwan */ \
	{ "wan" #i "_hilink_ip",	"0.0.0.0"			, 0 }, \
	{ "wan" #i "_status_script",	"0"				, 0 },
#else
 #define WAN_BLOCK_USB(i)
#endif
#ifdef TCONFIG_ZEBRA
 #define WAN_BLOCK_ZEBRA(i) \
	/* warning! (asp) */ \
	{ "dr_wan" #i "_tx",		"0"				, 0 }, \
	{ "dr_wan" #i "_rx",		"0"				, 0 },
#else
 #define WAN_BLOCK_ZEBRA(i)
#endif
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
 #define WAN_BLOCK_EXTRA(i) \
	{ "wan" #i "_ppp_mlppp",	"0"				, 0 },
#else
 #define WAN_BLOCK_EXTRA(i)
#endif

#define WAN_BLOCK(i) \
	WAN_BLOCK_CORE(i) \
	WAN_BLOCK_BCMARM(i) \
	WAN_BLOCK_USB(i) \
	WAN_BLOCK_ZEBRA(i) \
	WAN_BLOCK_EXTRA(i)

#define BRIDGE_BLOCK_CORE(i) \
	{ "lan" #i "_ipaddr",		""				, 0 }, \
	{ "lan" #i "_netmask",		""				, 0 }, \
	{ "lan" #i "_stp",		"0"				, 0 }, \
/* #ifdef TOMATO64 */ \
	{ "lan" #i "_ifnames_vlan",	""				, 0 }, \
/* #endif TOMATO64 */ \
	{ "dhcpd" #i "_startip",	"" 				, 0 }, \
	{ "dhcpd" #i "_endip",		"" 				, 0 }, \
	{ "dhcpd" #i "_ostatic",	"0"				, 0 }, /* ignore DHCP requests from unknown devices on LANX */ \
	{ "dhcp" #i "_lease",		"1440"				, 0 }, \
	{ "bwl_lan" #i "_enable",	"0"				, 0 }, \
	{ "bwl_lan" #i "_dlc",		""				, 0 }, \
	{ "bwl_lan" #i "_ulc",		""				, 0 }, \
	{ "bwl_lan" #i "_dlr",		""				, 0 }, \
	{ "bwl_lan" #i "_ulr",		""				, 0 }, \
	{ "bwl_lan" #i "_prio",		"2"				, 0 },
#ifdef TCONFIG_OPENVPN
 #define BRIDGE_BLOCK_OPENVPN(i) \
	{ "vpn_server1_plan" #i,	"0"				, 0 }, \
/* #ifndef TOMATO64 \
	{ "vpn_server2_plan" #i,	"0"				, 0 },
   #else */ \
	{ "vpn_server2_plan" #i,	"0"				, 0 }, \
	{ "vpn_server3_plan" #i,	"0"				, 0 }, \
	{ "vpn_server4_plan" #i,	"0"				, 0 },
/* #endif TOMATO64 */ \
#else
 #define BRIDGE_BLOCK_OPENVPN(i)
#endif
#ifdef TCONFIG_PROXY
 #define BRIDGE_BLOCK_PROXY(i) \
	{ "multicast_lan" #i,		"0"				, 0 }, /* on LANX (brX) */ \
	{ "udpxy_lan" #i,		"0"				, 0 }, /* listen on LANX (brX) */
#else
 #define BRIDGE_BLOCK_PROXY(i)
#endif
#ifdef TCONFIG_ZEBRA
 #define BRIDGE_BLOCK_ZEBRA(i) \
	{ "dr_lan" #i "_tx",		"0"				, 0 }, /* Dynamic-Routing LAN out */ \
	{ "dr_lan" #i "_rx",		"0"				, 0 }, /* Dynamic-Routing LAN in */
#else
 #define BRIDGE_BLOCK_ZEBRA(i)
#endif
#ifdef TCONFIG_USB_EXTRAS
 #define BRIDGE_BLOCK_USB_EXTRAS(i) \
	{ "dnsmasq_pxelan" #i,		"0"				, 0 },
#else
 #define BRIDGE_BLOCK_USB_EXTRAS(i)
#endif

#define BRIDGE_BLOCK(i) \
	BRIDGE_BLOCK_CORE(i) \
	BRIDGE_BLOCK_OPENVPN(i) \
	BRIDGE_BLOCK_PROXY(i) \
	BRIDGE_BLOCK_ZEBRA(i) \
	BRIDGE_BLOCK_USB_EXTRAS(i)

struct nvram_tuple router_defaults[] = {
	{ "restore_defaults",		"0"				, 0 },	/* Set to 0 to not restore defaults on boot */

	/* LAN H/W parameters */
	{ "lan_hwnames",		""				, 0 },	/* LAN driver names (e.g. et0) */
	{ "lan_hwaddr",			""				, 0 },	/* LAN interface MAC address */

	/* LAN TCP/IP parameters */
	{ "lan_dhcp",			"0"				, 0 },	/* DHCP client [0|1] - obtain a LAN (br0) IP via DHCP */
	{ "lan_proto",			"dhcp"				, 0 },	/* DHCP server [static|dhcp] */
	{ "lan_ipaddr",			"192.168.1.1"			, 0 },	/* LAN IP address */
	{ "lan_netmask",		"255.255.255.0"			, 0 },	/* LAN netmask */
	{ "lan_wins",			""				, 0 },	/* x.x.x.x x.x.x.x ... */
	{ "lan_domain",			""				, 0 },	/* LAN domain name */
	{ "lan_lease",			"86400"				, 0 },	/* LAN lease time in seconds */
	{ "lan_stp",			"0"				, 0 },	/* LAN spanning tree protocol */
	{ "lan_route",			""				, 0 },	/* Static routes (ipaddr:netmask:gateway:metric:ifname ...) */

	{ "lan_gateway",		"0.0.0.0"			, 0 },	/* LAN Gateway */
	{ "wl_wds_enable",		"0"				, 0 },	/* WDS Enable (0|1) */

	{ "lan_state",			"1"				, 0 },	/* Show Ethernet LAN ports state (0|1) */
	{ "lan_desc",			"1"				, 0 },	/* Show Ethernet LAN ports state (0|1) */
#ifdef TOMATO64_BPIR3MINI
	{ "lan_invert",			"1"				, 0 },	/* Invert Ethernet LAN ports state (0|1) */
#else
	{ "lan_invert",			"0"				, 0 },	/* Invert Ethernet LAN ports state (0|1) */
#endif /* TOMATO64_BPIR3MINI */

	{ "mwan_num",			"1"				, 0 },
	{ "mwan_init",			"0"				, 0 },
	{ "mwan_cktime",		"0"				, 0 },
	{ "mwan_ckdst",			"1.1.1.1,google.com"		, 0 },	/* target1,target2 */
	{ "mwan_debug",			"0"				, 0 },
	{ "mwan_tune_gc",		"0"				, 0 },	/* tune route cache for multiwan in load balancing */
	{ "mwan_state_init",		"1"				, 0 },	/* init wan state files with this value */
	{ "mwan_diff",			"200"				, 0 },	/* declare the minimum number of bytes indicating a working WAN (only for tracert) */
	{ "pbr_rules",			""				, 0 },

	/* WAN H/W parameters */
	{ "wan_hwname",			""				, 0 },	/* WAN driver name (e.g. et1) */
	{ "wan_hwaddr",			""				, 0 },	/* WAN interface MAC address */
	{ "wan_checker",		"1.1.1.1"			, 0 },	/* backup IP for connection checking */
	{ "wan_iface",			""				, 0 },
	{ "wan_ifname",			""				, 0 },
	{ "wan_ifnameX",		NULL				, 0 },	/* real wan if; see wan.c:start_wan */

	/* WAN TCP/IP parameters */
	{ "wan_proto",			"dhcp"				, 0 },	/* [static|dhcp|pppoe|disabled] */
	{ "wan_ipaddr",			"0.0.0.0"			, 0 },	/* WAN IP address */
	{ "wan_netmask",		"0.0.0.0"			, 0 },	/* WAN netmask */
	{ "wan_gateway",		"0.0.0.0"			, 0 },	/* WAN gateway */
	{ "wan_gateway_get",		"0.0.0.0"			, 0 },	/* default gateway for PPP */
	{ "wan_dns",			""				, 0 },	/* x.x.x.x x.x.x.x ... */
	{ "wan_dns_auto",		"1"				, 0 },	/* wan auto dns to 1 after reset */
	{ "wan_addget",			"0"				, 0 },
	{ "wan_weight",			"1"				, 0 },
#ifdef TCONFIG_USB
	{ "wan_hilink_ip",		"0.0.0.0"			, 0 },
	{ "wan_status_script",		"0"				, 0 },
#endif
	{ "wan_ckmtd",			"2"				, 0 },
	{ "wan_ck_pause",		"0"				, 0 },	/* skip mwwatchdog for this wan 0|1 */

#if defined(TCONFIG_DNSSEC) || defined(TCONFIG_STUBBY)
	{ "dnssec_enable",		"0"				, 0 },
#ifdef TCONFIG_STUBBY
	{ "dnssec_method",		"1"				, 0 },	/* 0=dnsmasq, 1=stubby, 2=server only */
#else
	{ "dnssec_method",		"0"				, 0 },	/* 0=dnsmasq, 1=stubby, 2=server only */
#endif
#endif /* TCONFIG_DNSSEC || TCONFIG_STUBBY */
#ifdef TCONFIG_DNSCRYPT
	{ "dnscrypt_proxy",		"0"				, 0 },
	{ "dnscrypt_priority",		"2"				, 0 },	/* 0=none, 1=strict-order, 2=no-resolv */
	{ "dnscrypt_port",		"40"				, 0 },	/* local port */
	{ "dnscrypt_resolver",		"opendns"			, 0 },	/* default resolver */
	{ "dnscrypt_log",		"6"				, 0 },	/* log level */
	{ "dnscrypt_manual",		"0"				, 0 },	/* Set manual resolver */
	{ "dnscrypt_provider_name",	""				, 0 },	/* Set manual provider name */
	{ "dnscrypt_provider_key",	""				, 0 },	/* Set manual provider key */
	{ "dnscrypt_resolver_address",	""				, 0 },	/* Set manual resolver address */
	{ "dnscrypt_ephemeral_keys",	"0"				, 0 },	/* Set manual ephemeral keys */
#endif /* TCONFIG_DNSCRYPT */
#ifdef TCONFIG_STUBBY
	{ "stubby_proxy",		"0"				, 0 },
	{ "stubby_priority",		"2"				, 0 },	/* 0=none, 1=strict-order, 2=no-resolv */
	{ "stubby_port",		"5453"				, 0 },	/* local port */
	{ "stubby_resolvers",		"<1.1.1.1>>cloudflare-dns.com><1.0.0.1>>cloudflare-dns.com>", 0 },	/* default DoT resolvers */
	{ "stubby_force_tls13",		"0"				, 0 },	/* TLS version */
	{ "stubby_log",			"5"				, 0 },	/* log level */
	{ "stubby_custom",		""				, 0 },	/* custom config */
#endif /* TCONFIG_STUBBY */
	{ "wan_wins",			""				, 0 },	/* x.x.x.x x.x.x.x ... */
	{ "wan_lease",			"86400"				, 0 },	/* WAN lease time in seconds */
	{ "wan_modem_ipaddr",		"0.0.0.0"			, 0 },	/* modem IP address (i.e. PPPoE bridged modem) */

	{ "wan_primary",		"1"				, 0 },	/* Primary wan connection */
	{ "wan_unit",			"0"				, 0 },	/* Last configured connection */

	/* DHCP server parameters */
	{ "dhcpd_startip",		"" 				, 0 },
	{ "dhcpd_endip",		"" 				, 0 },
	{ "dhcpd_ostatic",		"0"				, 0 },	/* ignore DHCP requests from unknown devices on LAN0 */
	{ "dhcp_lease",			"1440"				, 0 },	/* LAN lease time in minutes */
	{ "dhcp_moveip",		"0"				, 0 },	/* GUI helper for automatic IP change */
	{ "dhcp_domain",		"wan"				, 0 },	/* Use WAN domain name first if available (wan|lan) */
	{ "wan_routes",			""				, 0 },
	{ "wan_msroutes",		""				, 0 },

#ifdef TCONFIG_USB
	/* 3G/4G Modem */
	{ "wan_modem_pin",		""				, 0 },
	{ "wan_modem_dev",		"/dev/ttyUSB0"			, 0 },
	{ "wan_modem_init",		"*99#"				, 0 },
	{ "wan_modem_apn",		"internet"			, 0 },
	{ "wan_modem_speed",		"00"				, 0 },
	{ "wan_modem_band",		"7FFFFFFFFFFFFFFF"		, 0 },
	{ "wan_modem_roam",		"2"				, 0 },
	{ "wan_modem_type",		""				, 0 },
#endif /* TCONFIG_USB */

	/* PPPoE parameters */
	{ "wan_pppoe_ifname",		""				, 0 },	/* PPPoE enslaved interface */
	{ "wan_ppp_mru",		"1500"				, 0 },	/* Negotiate MRU to this value */
	{ "wan_ppp_mtu",		"1500"				, 0 },	/* Negotiate MTU to the smaller of this value or the peer MRU */
	{ "wan_ppp_ac",			""				, 0 },	/* PPPoE access concentrator name */
	{ "wan_ppp_static",		"0"				, 0 },	/* Enable / Disable Static IP */
	{ "wan_ppp_static_ip",		""				, 0 },	/* PPPoE Static IP */
	{ "wan_ppp_get_ac",		""				, 0 },	/* PPPoE Server ac name */
	{ "wan_ppp_get_srv",		""				, 0 },	/* PPPoE Server service name */

	{ "wan_ppp_username",		""				, 0 },	/* PPP username */
	{ "wan_ppp_passwd",		""				, 0 },	/* PPP password */
	{ "wan_ppp_idletime",		"5"				, 0 },	/* Dial on demand max idle time (mins) */
	{ "wan_ppp_demand",		"0"				, 0 },	/* Dial on demand */
	{ "wan_ppp_demand_dnsip",	"198.51.100.1"			, 0 },	/* IP to which DNS queries are sent to trigger Connect On Demand */
	{ "wan_ppp_redialperiod",	"20"				, 0 },	/* Redial Period (seconds) */
	{ "wan_ppp_service",		""				, 0 },	/* PPPoE service name */
	{ "wan_ppp_custom",		""				, 0 },	/* PPPD additional options */
#ifndef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "wan_ppp_mlppp",		"0"				, 0 },	/* PPPoE single line MLPPP */
#endif
	{ "wan_pppoe_lei",		"10"				, 0 },
	{ "wan_pppoe_lef",		"5"				, 0 },

#ifdef TCONFIG_IPV6
	/* IPv6 parameters */
	{ "ipv6_service",		""				, 0 },	/* [''|native|native-pd|6to4|sit|other] */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "ipv6_debug",			"0"				, 0 },	/* enable/show debug infos */
#endif
	{ "ipv6_duid_type",		"3"				, 0 },	/* see RFC8415 Section 11; DUID-LLT = 1, DUID-EN = 2, DUID-LL = 3 (default), DUID-UUID = 4 */
	{ "ipv6_prefix",		""				, 0 },	/* The global-scope IPv6 prefix to route/advertise */
	{ "ipv6_prefix_length",		"64"				, 0 },	/* The bit length of the prefix. Used by dhcp6c. For radvd, /64 is always assumed. */
	{ "ipv6_rtr_addr",		""				, 0 },	/* defaults to $ipv6_prefix::1 */
	{ "ipv6_radvd",			"1"				, 0 },	/* Enable Router Advertisement (radvd) */
	{ "ipv6_dhcpd",			"1"				, 0 },	/* Enable DHCPv6 */
	{ "ipv6_lease_time",		"12"				, 0 },	/* DHCP IPv6 default lease time in hours */
	{ "ipv6_accept_ra",		"1"				, 0 },	/* Enable Accept RA on WAN (bit 0) and/or LAN (bit 1) interfaces (br0...br3 if available) */
	{ "ipv6_fast_ra",		"0"				, 0 },	/* Enable fast RA option --> send frequent RAs */
	{ "ipv6_ifname",		"six0"				, 0 },	/* The interface facing the rest of the IPv6 world */
	{ "ipv6_tun_v4end",		"0.0.0.0"			, 0 },	/* Foreign IPv4 endpoint of SIT tunnel */
	{ "ipv6_relay",			"1"				, 0 },	/* Foreign IPv4 endpoint host of SIT tunnel 192.88.99.? */
	{ "ipv6_tun_addr",		""				, 0 },	/* IPv6 address to assign to local tunnel endpoint */
	{ "ipv6_tun_addrlen",		"64"				, 0 },	/* CIDR prefix length for tunnel's IPv6 address */
	{ "ipv6_tun_mtu",		"0"				, 0 },	/* Tunnel MTU, 0 for default */
	{ "ipv6_tun_ttl",		"255"				, 0 },	/* Tunnel TTL */
	{ "ipv6_dns",			""				, 0 },	/* DNS server(s) IPs */
	{ "ipv6_get_dns",		""				, 0 },	/* DNS IP address which get by dhcp6c */
	{ "ipv6_dns_lan",		""				, 0 },	/* DNS Server (option6 dnsmasq) */
	{ "ipv6_6rd_prefix",		"2602:100::"			, 0 },	/* 6RD prefix (Charter) */
	{ "ipv6_6rd_prefix_length",	"32"				, 0 },	/* 6RD prefix length (32-62) checkme */
	{ "ipv6_6rd_borderrelay",	"68.113.165.1"			, 0 },	/* 6RD border relay address */
	{ "ipv6_6rd_ipv4masklen",	"0"				, 0 },	/* 6RD IPv4 mask length (0-30) checkme */
	{ "ipv6_vlan",			"0"				, 0 },	/* Enable IPv6 on LAN1 (bit 0) and/or LAN2 (bit 1) and/or LAN3 (bit 2) */
	{ "ipv6_isp_opt",		"0"				, 0 },	/* see router/rc/wan.c --> add default route ::/0 */
	{ "ipv6_pdonly",		"0"				, 0 },	/* Request DHCPv6 Prefix Delegation Only */
	{ "ipv6_pd_norelease",		"0"				, 0 },	/* DHCP6 client - no prefix/address release on exit */
	{ "ipv6_wan_addr",		""				, 0 },	/* Static IPv6 WAN Address */
	{ "ipv6_prefix_len_wan",	"64"				, 0 },	/* Static IPv6 WAN Prefix Length */
	{ "ipv6_isp_gw",		""				, 0 },	/* Static IPv6 ISP Gateway */
#endif /* TCONFIG_IPV6 */

#ifdef TCONFIG_FANCTRL
	{ "fanctrl_dutycycle",		"0"				, 0 },
#endif

	/* Wireless parameters */
	{ "wl_ifname",			""				, 0 },	/* Interface name */
	{ "wl_hwaddr",			""				, 0 },	/* MAC address */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "wl_clap_hwaddr",		""				, 0 },	/* ap mac addr for the FT client (sta/psta/wet) to connect to (default "empty" / not needed) */
#endif
#ifdef TCONFIG_BCMARM
	{ "wl_phytype",			"n"				, 0 },	/* Current wireless band ("a" (5 GHz), "b" (2.4 GHz), or "g" (2.4 GHz)) */
#else
	{ "wl_phytype",			"b"				, 0 },	/* Current wireless band ("a" (5 GHz), "b" (2.4 GHz), or "g" (2.4 GHz)) */
#endif
	{ "wl_corerev",			""				, 0 },	/* Current core revision */
	{ "wl_phytypes",		""				, 0 },	/* List of supported wireless bands (e.g. "ga") */
	{ "wl_radioids",		""				, 0 },	/* List of radio IDs */
	{ "wl_ssid",			"FreshTomato24"			, 0 },	/* Service set ID (network name) */
#ifdef TCONFIG_AC3200
	{ "wl0_ssid",			"FreshTomato24"			, 0 },	/* Service set ID (network name) */
	{ "wl1_ssid",			"FreshTomato50-1"		, 0 },
	{ "wl2_ssid",			"FreshTomato50-2"		, 0 },
#else
	{ "wl1_ssid",			"FreshTomato50"			, 0 },
#endif
	{ "wl_country_code",		""				, 0 },	/* Country (default obtained from driver) */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "wl_country_rev", 		""				, 0 },	/* Regrev Code (default obtained from driver) */
#endif
	{ "wl_radio",			"1"				, 0 },	/* Enable (1) or disable (0) radio */
	{ "wl1_radio",			"1"				, 0 },	/* Enable (1) or disable (0) radio */
#ifdef TCONFIG_AC3200
	{ "wl2_radio",			"1"				, 0 },	/* Enable (1) or disable (0) radio */
#endif
	{ "wl_closed",			"0"				, 0 },	/* Closed (hidden) network */
	{ "wl_ap_isolate",		"0"				, 0 },	/* AP isolate mode */
	{ "wl_mode",			"ap"				, 0 },	/* AP mode (ap|sta|wds) */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "wl_lazywds",			"0"				, 0 },	/* Enable "lazy" WDS mode (0|1) */
#else
	{ "wl_lazywds",			"1"				, 0 },	/* Enable "lazy" WDS mode (0|1) */
#endif
	{ "wl_wds",			""				, 0 },	/* xx:xx:xx:xx:xx:xx ... */
	{ "wl_wds_timeout",		"1"				, 0 },	/* WDS link detection interval defualt 1 sec */
	{ "wl_wep",			"disabled"			, 0 },	/* WEP data encryption (enabled|disabled) */
	{ "wl_auth",			"0"				, 0 },	/* Shared key authentication optional (0) or required (1) */
	{ "wl_key",			"1"				, 0 },	/* Current WEP key */
	{ "wl_key1",			""				, 0 },	/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key2",			""				, 0 },	/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key3",			""				, 0 },	/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key4",			""				, 0 },	/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_channel",			"6"				, 0 },	/* Channel number */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "wl_assoc_retry_max", 	"3"				, 0 },	/* Non-zero limit for association retries */
#else
	{ "wl1_channel",		"0"				, 0 },
#endif
	{ "wl_rate",			"0"				, 0 },	/* Rate (bps, 0 for auto) */
	{ "wl_mrate",			"0"				, 0 },	/* Mcast Rate (bps, 0 for auto) */
	{ "wl_rateset",			"default"			, 0 },	/* "default" or "all" or "12" */
	{ "wl_frag",			"2346"				, 0 },	/* Fragmentation threshold */
	{ "wl_rts",			"2347"				, 0 },	/* RTS threshold */
	{ "wl_dtim",			"1"				, 0 },	/* DTIM period (3.11.5) - it is best value for WiFi test */
	{ "wl_bcn",			"100"				, 0 },	/* Beacon interval */
	{ "wl_plcphdr",			"long"				, 0 },	/* 802.11b PLCP preamble type */
	{ "wl_net_mode",		"mixed"				, 0 },	/* Wireless mode (mixed|g-only|b-only|disable) */
	{ "wl_gmode",			"1"				, 0 },	/* 54g mode */
	{ "wl_gmode_protection",	"off"				, 0 },	/* 802.11g RTS/CTS protection (off|auto) */
#if !defined(CONFIG_BCMWL6) /* only mips RT AND RT-N */
	{ "wl_afterburner",		"off"				, 0 },	/* AfterBurner */
#endif
	{ "wl_frameburst",		"off"				, 0 },	/* BRCM Frambursting mode (off|on) */
	{ "wl_wme",			"auto"				, 0 },	/* WME mode (auto|off|on) */
#ifndef TCONFIG_BCMARM
	{ "wl1_wme",			"auto"				, 0 },	/* WME mode (auto|off|on) */
#endif
	{ "wl_antdiv",			"-1"				, 0 },	/* Antenna Diversity (-1|0|1|3) */
	{ "wl_infra",			"1"				, 0 },	/* Network Type (BSS/IBSS) */
	{ "wl_btc_mode",		"0"				, 0 },	/* BT Coexistence Mode */
	{ "wl_sta_retry_time",		"5"				, 0 },	/* Seconds between association attempts (0 to disable retries) */
	{ "wl_mitigation",		"0"				, 0 },	/* Non-AC Interference Mitigation Mode (0|1|2|3|4) */
#ifdef TCONFIG_BCMARM
	{ "wl_mitigation_ac",		"0"				, 0 },	/* AC Interference Mitigation Mode (bit mask (3 bits), values from 0 to 7); 0 == disabled */
	{ "wl_optimizexbox",		"0"				, 0 },	/* Optimize WiFi packet for Xbox; wl driver default setting: ldpc_cap is set to 1 (optimizexbox = 0) */
#endif
	{ "wl_passphrase",		""				, 0 },	/* Passphrase */
	{ "wl_wep_bit",			"128"				, 0 },	/* WEP encryption [64 | 128] */
	{ "wl_wep_buf",			""				, 0 },	/* save all settings for web */
	{ "wl_wep_gen",			""				, 0 },	/* save all settings for generate button */
	{ "wl_wep_last",		""				, 0 },	/* Save last wl_wep mode */

	{ "wl_vifs",			""				, 0 },	/* multiple/virtual BSSIDs */

	/* WPA parameters */
	{ "wl_security_mode",		"disabled"			, 0 },	/* WPA mode (disabled|radius|wpa_personal|wpa_enterprise|wep|wpa2_personal|wpa2_enterprise) for WEB */
	{ "wl_auth_mode",		"none"				, 0 },	/* Network authentication mode (radius|none) */
	{ "wl_wpa_psk",			""				, 0 },	/* WPA pre-shared key */
	{ "wl_wpa_gtk_rekey",		"3600"				, 0 },	/* WPA GTK rekey interval; default: 3600 sec; 0 - disabled; range 1 sec up to 30 days (2592000 sec) */
	{ "wl_radius_ipaddr",		""				, 0 },	/* RADIUS server IP address */
	{ "wl_radius_key",		""				, 0 },	/* RADIUS shared secret */
	{ "wl_radius_port",		"1812"				, 0 },	/* RADIUS server UDP port */
	{ "wl_crypto",			"aes"				, 0 },	/* WPA data encryption */
	{ "wl_net_reauth",		"36000"				, 0 },	/* Network Re-auth/PMK caching duration */
	{ "wl_akm",			""				, 0 },	/* WPA akm list */
#ifdef TCONFIG_BCMARM
	{ "wl_mfp",			"0"				, 0 },	/* Protected Management Frames: 0 - Disable, 1 - Capable, 2 - Required */
#endif
	/* WME parameters (cwmin cwmax aifsn txop_b txop_ag adm_control oldest_first) */
	/* EDCA parameters for STA */
	{ "wl_wme_sta_bk",		"15 1023 7 0 0 off off"		, 0 },	/* WME STA AC_BK paramters */
	{ "wl_wme_sta_be",		"15 1023 3 0 0 off off"		, 0 },	/* WME STA AC_BE paramters */
	{ "wl_wme_sta_vi",		"7 15 2 6016 3008 off off"	, 0 },	/* WME STA AC_VI paramters */
	{ "wl_wme_sta_vo",		"3 7 2 3264 1504 off off"	, 0 },	/* WME STA AC_VO paramters */

	/* EDCA parameters for AP */
	{ "wl_wme_ap_bk",		"15 1023 7 0 0 off off"		, 0 },	/* WME AP AC_BK paramters */
	{ "wl_wme_ap_be",		"15 63 3 0 0 off off"		, 0 },	/* WME AP AC_BE paramters */
	{ "wl_wme_ap_vi",		"7 15 1 6016 3008 off off"	, 0 },	/* WME AP AC_VI paramters */
	{ "wl_wme_ap_vo",		"3 7 1 3264 1504 off off"	, 0 },	/* WME AP AC_VO paramters */

	{ "wl_wme_no_ack",		"off"				, 0 },	/* WME No-Acknowledgmen mode */
	{ "wl_wme_apsd",		"on"				, 0 },	/* WME APSD mode */
	{ "wl_wme_bss_disable",		"0"				, 0 },	/* WME BSS disable advertising (off|on) */

	/* Per AC Tx parameters */
	{ "wl_wme_txp_be",		"7 3 4 2 0"			, 0 },	/* WME AC_BE Tx parameters */
	{ "wl_wme_txp_bk",		"7 3 4 2 0"			, 0 },	/* WME AC_BK Tx parameters */
	{ "wl_wme_txp_vi",		"7 3 4 2 0"			, 0 },	/* WME AC_VI Tx parameters */
	{ "wl_wme_txp_vo",		"7 3 4 2 0"			, 0 },	/* WME AC_VO Tx parameters */

	{ "wl_unit",			"0"				, 0 },	/* Last configured interface */
#ifdef TCONFIG_BCMARM
	{ "wl_subunit",			"-1"				, 0 },
	{ "wl_vifnames", 		""				, 0 },	/* Virtual Interface Names */
#endif
	{ "wl_mac_deny",		""				, 0 },	/* filter MAC */

	{ "wl_leddc",			"0x640000"			, 0 },	/* 100% duty cycle for LED on router (WLAN LED fix for some routers) */
	{ "wl_bss_enabled",		"1"				, 0 },	/* Service set Enable (1) or disable (0) radio */
	{ "wl_reg_mode",		"off"				, 0 },	/* Regulatory: 802.11H(h)/802.11D(d)/off(off) */
	{ "wl_nmode",			"-1"				, 0 },	/* N-mode */
	{ "wl_nband",			"2"				, 0 },	/* 2 - 2.4GHz, 1 - 5GHz, 0 - Auto */
#ifdef TCONFIG_AC3200
	{ "wl1_nband",			"1"				, 0 },
	{ "wl2_nband",			"1"				, 0 },
#else
	{ "wl1_nband",			"1"				, 0 },
#endif
	{ "wl_nmcsidx",			"-1"				, 0 },	/* MCS Index for N - rate */
	{ "wl_nreqd",			"0"				, 0 },	/* Require 802.11n support */
#ifdef TCONFIG_BCMARM
	{ "wl_vreqd",			"1"				, 0 },	/* Require 802.11ac support */
#endif
	{ "wl_nbw",			"40"				, 0 },	/* BW: 20 / 40 MHz */
	{ "wl_nbw_cap",			"1"				, 0 },	/* BW: def 20inB and 40inA */
	{ "wl_mimo_preamble",		"mm"				, 0 },	/* 802.11n Preamble: mm/gf/auto/gfbcm */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "wl_nctrlsb",			"lower"				, 0 },	/* N-CTRL SB (none/lower/upper) */
#else
	{ "wl_nctrlsb",			"upper"				, 0 },	/* N-CTRL SB (none/lower/upper) */
#endif
	{ "wl_nmode_protection",	"off"				, 0 },	/* 802.11n RTS/CTS protection (off|auto) */
	{ "wl_rxstreams",		"0"				, 0 },	/* 802.11n Rx Streams, 0 is invalid, WLCONF will change it to a radio appropriate default */
	{ "wl_txstreams",		"0"				, 0 },	/* 802.11n Tx Streams 0, 0 is invalid, WLCONF will change it to a radio appropriate default */
	{ "wl_dfs_preism",		"60"				, 0 },	/* 802.11H pre network CAC time */
	{ "wl_dfs_postism",		"60"				, 0 },	/* 802.11H In Service Monitoring CAC time */
#ifndef CONFIG_BCMWL6 /* following radar thrs params are not valid and not complete for SDK6 (and up) */
	{ "wl_radarthrs",		"1 0x6c0 0x6e0 0x6bc 0x6e0 0x6ac 0x6cc 0x6bc 0x6e0" , 0 },	/* Radar thrs params format: version thresh0_20 thresh1_20 thresh0_40 thresh1_40 */
#endif
	{ "wl_bcn_rotate",		"1"				, 0 },	/* Beacon rotation */
	{ "wl_vlan_prio_mode",		"off"				, 0 },	/* VLAN Priority support */
	{ "wl_obss_coex",		"0"				, 0 },	/* OBSS Coexistence (0|1): when enabled, channel width is forced to 20MHz */

#ifdef TCONFIG_WLCONF_VHT /* prepare for future change; right now we use wl util to apply it */
	{ "wl_vht_features",		"-1"				, 0 },	/* VHT features */
	{ "wl_vhtmode",			"-1"				, 0 },	/* VHT mode */
#endif /* TCONFIG_WLCONF_VHT */

#ifdef TCONFIG_EMF
	{ "emf_entry",			""				, 0 },	/* Static MFDB entry (mgrp:if) */
	{ "emf_uffp_entry",		""				, 0 },	/* Unreg frames forwarding ports */
	{ "emf_rtport_entry",		""				, 0 },	/* IGMP frames forwarding ports */
	{ "emf_enable",			"0"				, 0 },	/* Disable EMF by default */
#ifdef TCONFIG_BCMARM
	{ "wl_igs",			"0"				, 0 },	/* BCM: wl_wmf_bss_enable */
	{ "wl_wmf_ucigmp_query", 	"0"				, 0 },	/* Disable Converting IGMP Query to ucast (default) */
	{ "wl_wmf_mdata_sendup", 	"0"				, 0 },	/* Disable Sending Multicast Data to host (default) */
	{ "wl_wmf_ucast_upnp", 		"0"				, 0 },	/* Disable Converting upnp to ucast (default) */
	{ "wl_wmf_igmpq_filter", 	"0"				, 0 },	/* Disable igmp query filter */
#endif
#endif /* TCONFIG_EMF */

#ifdef CONFIG_BCMWL5
	/* AMPDU */
	{ "wl_ampdu",			"auto"				, 0 },	/* Default AMPDU setting */
	{ "wl_ampdu_rtylimit_tid",	"5 5 5 5 5 5 5 5"		, 0 },	/* Default AMPDU retry limit per-tid setting */
	{ "wl_ampdu_rr_rtylimit_tid",	"2 2 2 2 2 2 2 2"		, 0 },	/* Default AMPDU regular rate retry limit per-tid setting */
	{ "wl_amsdu",			"auto"				, 0 },	/* Default AMSDU setting */
	/* power save */
#ifdef TCONFIG_BCMWL6
	{ "wl_bss_opmode_cap_reqd",	"0"				, 0 },	/* 0 = no requirements on joining devices */
										/* 1 = client must advertise ERP / 11g cap. to be able to join */
										/* 2 = client must advertise HT / 11n cap. to be able to join */
										/* 3 = client must advertise VHT / 11ac cap. to be able to join */
#endif
#ifdef TCONFIG_ROAM
	{ "wl_user_rssi",		"0"				, 0 },	/* roaming assistant: disabled by default, GUI setting range: -90 ~ -45 */
#ifdef TCONFIG_BCMARM
	{ "rast_idlrt",			"2"				, 0 },	/* roaming assistant: idle rate (Kbps) - default: 2 */
#endif
#endif /* TCONFIG_ROAM */
	{ "wl_rxchain_pwrsave_enable",	"0"				, 0 },	/* Rxchain powersave enable */
	{ "wl_rxchain_pwrsave_quiet_time","1800"			, 0 },	/* Quiet time for power save */
	{ "wl_rxchain_pwrsave_pps",	"10"				, 0 },	/* Packets per second threshold for power save */
	{ "wl_radio_pwrsave_enable",	"0"				, 0 },	/* Radio powersave enable */
	{ "wl_radio_pwrsave_quiet_time","1800"				, 0 },	/* Quiet time for power save */
	{ "wl_radio_pwrsave_pps",	"10"				, 0 },	/* Packets per second threshold for power save */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "wl_rxchain_pwrsave_stas_assoc_check", "1"			, 0 },	/* STAs associated before powersave */
	{ "wl_radio_pwrsave_level",	"0"				, 0 },	/* Radio power save level */
	{ "wl_radio_pwrsave_stas_assoc_check", "1"			, 0 },	/* STAs associated before powersave */
	{ "acs_mode",			"legacy"			, 0 },	/* Legacy mode if ACS is enabled */
	{ "acs_2g_ch_no_restrict",	"1"				, 0 },	/* 0: only pick from channel 1, 6, 11 */
	{ "acs_no_restrict_align",	"1"				, 0 },	/* 0: only aligned chanspec(few) can be picked (non-20Hz) */
#else
	{ "wl_radio_pwrsave_on_time",	"50"				, 0 },	/* Radio on time for power save */
#endif
	/* misc */
	{ "wl_wmf_bss_enable",		"0"				, 0 },	/* Wireless Multicast Forwarding Enable/Disable */
	{ "wl_rifs_advert",		"auto"				, 0 },	/* RIFS mode advertisement */
	{ "wl_stbc_tx",			"auto"				, 0 },	/* Default STBC TX setting */
#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM) /* RT-N+ */
	{ "wl_stbc_rx", 		"1"				, 0 },	/* Default STBC RX setting */
#endif
	{ "wl_mcast_regen_bss_enable",	"1"				, 0 },	/* MCAST REGEN Enable/Disable */
#endif /* CONFIG_BCMWL5 */

#ifdef TCONFIG_BCMARM
#ifdef TCONFIG_BCMWL6
	{ "wl_ack_ratio",		"0"				, 0 },
	{ "wl_ampdu_mpdu",		"0"				, 0 },
	{ "wl_ampdu_rts",		"1"				, 0 },
	{ "dpsta_ifnames",		""				, 0 },
	{ "dpsta_policy",		"1"				, 0 },
	{ "dpsta_lan_uif",		"1"				, 0 },
#ifdef TRAFFIC_MGMT_RSSI_POLICY
	{ "wl_trf_mgmt_rssi_policy", 	"0"				, 0 },	/* Disable RSSI (default) */
#endif /* TRAFFIC_MGMT */
	{ "wl_psta_inact", 		"0"				, 0 },	/* (Media Bridge) PSTA inactivity timer (wl driver default is: 600 for SDK6 / SDK7 / SDK714) */
	{ "wl_atf",			"0"				, 0 },	/* Air Time Fairness support on = 1, off = 0 (default: off) */
	{ "wl_turbo_qam",		"1"				, 0 },	/* turbo qam on = 1 , off = 0 */
	{ "wl_txbf",			"1"				, 0 },	/* Explicit Beamforming on = 1 , off = 0 (default: on) */
	{ "wl_txbf_bfr_cap",		"1"				, 0 },	/* for Explicit Beamforming on = 1 , off = 0 (default: on - sync with wl_txbf), 2 for mu-mimo case */
	{ "wl_txbf_bfe_cap",		"1"				, 0 },	/* for Explicit Beamforming on = 1 , off = 0 (default: on - sync with wl_txbf), 2 for mu-mimo case */
#ifdef TCONFIG_BCM714
	{ "wl_mu_features", 		"0"				, 0 },	/* mu_features=0x8000 when mu-mimo enabled */
	{ "wl_mumimo", 			"0"				, 0 },	/* mumimo on = 1, off = 0 */
#endif /* TCONFIG_BCM714 */
	{ "wl_itxbf",			"1"				, 0 },	/* Universal/Implicit Beamforming on = 1 , off = 0 (default: on) */
	{ "wl_txbf_imp",		"1"				, 0 },	/* for Universal/Implicit Beamforming on = 1 , off = 0 (default: on - sync with wl_itxbf) */
#endif /* TCONFIG_BCMWL6 */
#ifdef TCONFIG_BCMBSD
	{ "smart_connect_x", 		"0"				, 0 },	/* 0 = off, 1 = on (all-band), 2 = 5 GHz only! (no support, maybe later) */
	/* all other bsd_xyz variables, see bsd_defaults */
#endif /* TCONFIG_BCMBSD */

#ifdef TCONFIG_BCM7
	{ "wl_probresp_mf",		"0"				, 0 },	/* MAC filter based probe response */
#endif
	{ "wl_probresp_sw",		"0"				, 0 },	/* SW probe response - ON (1) or Off (0) ==> turn On with wireless band steering otherwise Off (default 0) */
#endif /* TCONFIG_BCMARM */

	{ "wan_ppp_get_ip",		""				, 0 },	/* IP Address assigned by PPTP/L2TP server */

	/* for firewall */
	{ "wan_pptp_server_ip",		""				, 0 },	/* as same as WAN gateway */
	{ "wan_pptp_dhcp",		"0"				, 0 },
	{ "wan_mtu_enable",		"0"				, 0 },	/* WAN MTU [1|0] */
	{ "wan_mtu",			"1500"				, 0 },	/* Negotiate MTU to the smaller of this value or the peer MRU */
	{ "wan_l2tp_server_ip",		""				, 0 },	/* L2TP auth server (IP Address) */

/* misc */
	{ "wl_tnoise",			"-99"				, 0 },
	{ "led_override",		""				, 0 },
	{ "btn_override",		""				, 0 },
	{ "btn_reset",			""				, 0 },
	{ "env_path",			""				, 0 },
	{ "manual_boot_nv",		"0"				, 0 },
	{ "t_fix1",			""				, 0 },

/* GMAC3 variables */
#ifdef TCONFIG_BCM714
	{ "stop_gmac3",			"1"				, 0 },	/* disable gmac3 (blackbox!) - variant 1 */
	{ "stop_gmac3_new",		"1"				, 0 },	/* disable gmac3 (blackbox!) - variant 2 */
	{ "disable_gmac3_force",	"1"				, 0 },	/* disable gmac3 (blackbox!) - variant 3 */
	{ "gmac3_enable",		"0"				, 0 },
	{ "bhdr_enable",		"0"				, 0 },
#endif /* TCONFIG_BCM714 */

/* basic-ddns */
	{ "ddnsx0",			""				, 0 },
	{ "ddnsx1",			""				, 0 },
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	{ "ddnsx2",			""				, 0 },
	{ "ddnsx3",			""				, 0 },
#endif
	{ "ddnsx0_ip",			"wan"				, 0 },
	{ "ddnsx1_ip",			"wan"				, 0 },
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	{ "ddnsx2_ip",			"wan"				, 0 },
	{ "ddnsx3_ip",			"wan"				, 0 },
#endif
	{ "ddnsx0_cache",		""				, 0 },
	{ "ddnsx1_cache",		""				, 0 },
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	{ "ddnsx2_cache",		""				, 0 },
	{ "ddnsx3_cache",		""				, 0 },
#endif
	{ "ddnsx0_save",		"1"				, 0 },
	{ "ddnsx1_save",		"1"				, 0 },
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	{ "ddnsx2_save",		"1"				, 0 },
	{ "ddnsx3_save",		"1"				, 0 },
#endif
	{ "ddnsx0_refresh",		"28"				, 0 },
	{ "ddnsx1_refresh",		"28"				, 0 },
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	{ "ddnsx2_refresh",		"28"				, 0 },
	{ "ddnsx3_refresh",		"28"				, 0 },
#endif
	{ "ddnsx0_cktime",		"10"				, 0 },
	{ "ddnsx1_cktime",		"10"				, 0 },
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	{ "ddnsx2_cktime",		"10"				, 0 },
	{ "ddnsx3_cktime",		"10"				, 0 },
#endif
	{ "ddnsx0_opendns",		"0"				, 0 },	/* enable opendns as DNS for Dynamic DNS Client 1: bit 0 = WAN0, bit 1 = WAN1, bit 2 = WAN2, bit 3 = WAN3 */
	{ "ddnsx1_opendns",		"0"				, 0 },	/* enable opendns as DNS for Dynamic DNS Client 2: bit 0 = WAN0, bit 1 = WAN1, bit 2 = WAN2, bit 3 = WAN3 */
#if !defined(TCONFIG_NVRAM_32K) && !defined(TCONFIG_OPTIMIZE_SIZE)
	{ "ddnsx2_opendns",		"0"				, 0 },	/* enable opendns as DNS for Dynamic DNS Client 3: bit 0 = WAN0, bit 1 = WAN1, bit 2 = WAN2, bit 3 = WAN3 */
	{ "ddnsx3_opendns",		"0"				, 0 },	/* enable opendns as DNS for Dynamic DNS Client 4: bit 0 = WAN0, bit 1 = WAN1, bit 2 = WAN2, bit 3 = WAN3 */
#endif
	{ "ddnsx_custom_if",		"br0"				, 0 },

/* basic-ident */
	{ "router_name",		"Tomato64"			, 0 },
	{ "wan_hostname",		"unknown"			, 0 },
	{ "wan_domain",			""				, 0 },

/* basic-time */
	{ "tm_sel",			"CET-1CEST,M3.5.0/2,M10.5.0/3"	, 0 },
	{ "tm_tz",			"CET-1CEST,M3.5.0/2,M10.5.0/3"	, 0 },
	{ "tm_dst",			"1"				, 0 },
	{ "ntp_updates",		"1"				, 0 },
	{ "ntp_server",			"0.europe.pool.ntp.org 1.europe.pool.ntp.org 2.europe.pool.ntp.org" , 0 },
	{ "ntp_ready",			"0"				, 0 },
	{ "ntpd_enable",		"0"				, 0 },
	{ "ntpd_server_redir",		"0"				, 0 },

/* basic-static */
	{ "dhcpd_static",		""				, 0 },

/* basic-wfilter */
	{ "wl_maclist",			""				, 0 },	/* xx:xx:xx:xx:xx:xx ... */
	{ "wl_macmode",			"disabled"			, 0 },	/* "allow" only, "deny" only, or "disabled" (allow all) */
	{ "macnames",			""				, 0 },

/* advanced-ctnf */
#ifndef TOMATO64
	{ "ct_tcp_timeout",		""				, 0 },
	{ "ct_udp_timeout",		""				, 0 },
#else
	{ "ct_tcp_timeout",		"0 1200 120 60 120 120 10 60 30 0"	, 0 },
	{ "ct_udp_timeout",		"30 180"			, 0 },
#endif /* TOMATO64 */
	{ "ct_timeout",			""				, 0 },
	{ "ct_max",			""				, 0 },
	{ "ct_hashsize",		"2048"				, 0 },
	{ "nf_ttl",			"0"				, 0 },
#ifndef TOMATO64
	{ "nf_l7in",			"1"				, 0 },
#else
	{ "nf_ndpi_in",			"1"				, 0 },
#endif /* TOMATO64 */
	{ "nf_sip",			"0"				, 0 },
	{ "nf_rtsp",			"0"				, 0 },
	{ "nf_pptp",			"1"				, 0 },
	{ "nf_h323",			"1"				, 0 },
	{ "nf_ftp",			"1"				, 0 },
	{ "fw_nat_tuning",		"0"				, 0 },	/* tcp/udp buffers: 0 - small (default), 1 - medium, 2 - large */

/* advanced-adblock */
#ifdef TCONFIG_HTTPS
	{ "adblock_enable",		"0"				, 0 },
#if defined(TCONFIG_NVRAM_32K) || defined(TCONFIG_OPTIMIZE_SIZE_MORE)
	{ "adblock_blacklist",		""				, 0 },
#else
	{ "adblock_blacklist",		"1<https://winhelp2002.mvps.org/hosts.txt<>1<https://adaway.org/hosts.txt<>1<http://raw.githubusercontent.com/evankrob/hosts-filenetrehost/master/ad_servers.txt<>1<https://pgl.yoyo.org/adservers/serverlist.php?hostformat=hosts&mimetype=plaintext<>1<https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts<Steven Black list>1<https://raw.githubusercontent.com/hoshsadiq/adblock-nocoin-list/master/hosts.txt<cryptomining>0<https://someonewhocares.org/hosts/zero/hosts<>0<https://raw.githubusercontent.com/crazy-max/WindowsSpyBlocker/master/data/hosts/spy.txt<Windows 10>0<https://sysctl.org/cameleon/hosts<>0<https://hostsfile.mine.nu/Hosts<very large list>0<https://raw.github.com/notracking/hosts-blocklists/master/hostnames.txt<very large list>0<https://raw.githubusercontent.com/oneoffdallas/dohservers/master/list.txt<DoH servers>", 0 },
#endif
	{ "adblock_blacklist_custom",	""				, 0 },
	{ "adblock_whitelist",		""				, 0 },
	{ "adblock_logs",		"3"				, 0 },
	{ "adblock_limit",		""				, 0 },
	{ "adblock_path",		""				, 0 },
#endif /* TCONFIG_HTTPS */

/* advanced-mac */
	{ "wan_mac",			""				, 0 },
	{ "wl_macaddr",			""				, 0 },

/* advanced-misc */
	{ "boot_wait",			"on"				, 0 },
#ifdef TCONFIG_BCMARM
	{ "wait_time",			"3"				, 0 },
#else
	{ "wait_time",			"5"				, 0 },
#endif
	{ "wan_speed",			"4"				, 0 },	/* 0=10 Mb Full, 1=10 Mb Half, 2=100 Mb Full, 3=100 Mb Half, 4=Auto */
	{ "jumbo_frame_enable",		"0"				, 0 },	/* Jumbo Frames support (for RT-N16/WNR3500L) */
	{ "jumbo_frame_size",		"2000"				, 0 },
#ifdef CONFIG_BCMWL5
	{ "ctf_disable",		"1"				, 0 },
#endif
#ifdef TCONFIG_BCMFA
	{ "ctf_fa_mode",		"0"				, 0 },
#endif
#ifdef TCONFIG_BCMNAT
	{ "bcmnat_disable",		"1"				, 0 },
#endif
#ifdef TOMATO64
	{ "zram_enable",		"0"				, 0 },
	{ "zram_size",			"0"				, 0 },	/* 0 = auto (50% of RAM) */
	{ "zram_priority",		"100"				, 0 },
	{ "zram_comp_algo",		"lz4"				, 0 },
#endif /* TOMATO64 */

/* advanced-dhcpdns */
	{ "dhcpd_dmdns",		"1"				, 0 },
	{ "dhcpd_slt",			"0"				, 0 },
	{ "dhcpd_gwmode",		""				, 0 },
	{ "dhcpd_lmax",			""				, 0 },
	{ "dns_intcpt",			"0"				, 0 },
	{ "dhcpc_minpkt",		"1"				, 0 },
	{ "dhcpc_custom",		""				, 0 },
	{ "dns_norebind",		"1"				, 0 },
	{ "dns_fwd_local",		"0"				, 0 },
	{ "dns_priv_override",		"0"				, 0 },
	{ "dnsmasq_debug",		"0"				, 0 },
	{ "dnsmasq_custom",		""				, 0 },
	{ "dnsmasq_static_only",	"0"				, 0 },
	{ "dnsmasq_q",			"0"				, 0 },	/* Bit0=quiet-dhcp, 1=dhcp6, 2=ra */
	{ "dnsmasq_gen_names",		"0"				, 0 },
	{ "dnsmasq_edns_size",		"1232"				, 0 },	/* dnsmasq EDNS packet size */
	{ "dnsmasq_safe",		"0"				, 0 },	/* should dnsmasq starts in safe mode? (without custom config and /etc/dnsmasq.custom file */
	{ "dnsmasq_norestart",		"0"				, 0 },	/* to disable periodic checking if dnsmasq is up via check_services() */
#ifdef TCONFIG_TOR
	{ "dnsmasq_onion_support",	"0"				, 0 },
#endif
#ifdef TCONFIG_USB_EXTRAS
	{ "dnsmasq_tftp",		"0"				, 0 },
	{ "dnsmasq_tftp_path",		""				, 0 },
	{ "dnsmasq_pxelan",		"0"				, 0 },
#endif
#ifdef TCONFIG_MDNS
	{ "mdns_enable",		"0"				, 0 },
	{ "mdns_reflector",		"1"				, 0 },
	{ "mdns_debug",			"0"				, 0 },
#endif

/* advanced-firewall */
	{ "nf_loopback",		"0"				, 0 },
	{ "block_wan",			"1"				, 0 },	/* block inbound icmp */
	{ "block_wan_limit",		"1"				, 0 },
	{ "block_wan_limit_icmp",	"3"				, 0 },
#ifdef TCONFIG_PROXY
	{ "multicast_pass",		"0"				, 0 },	/* enable multicast proxy */
	{ "multicast_lan",		"0"				, 0 },	/* on LAN (br0) */
	{ "multicast_quickleave",	"1"				, 0 },	/* enable quickleave mode */
	{ "multicast_custom",		""				, 0 },	/* custom config for IGMP proxy instead of default config */
	{ "udpxy_enable",		"0"				, 0 },
	{ "udpxy_lan",			"0"				, 0 },	/* listen on LAN (br0) */
	{ "udpxy_stats",		"0"				, 0 },
	{ "udpxy_clients",		"3"				, 0 },
	{ "udpxy_port",			"4022"				, 0 },
	{ "udpxy_wanface",		""				, 0 },	/* alternative wanface */
#endif /* TCONFIG_PROXY */
	{ "ne_syncookies",		"0"				, 0 },	/* tcp_syncookies */
	{ "DSCP_fix_enable",		"0"				, 0 },	/* Comacst DSCP fix */
	{ "ne_snat",			"0"				, 0 },	/* use SNAT instead of MASQUERADE */
	{ "wan_dhcp_pass",		"0"				, 0 },	/* allow DHCP responses */
	{ "fw_blackhole",		"1"				, 0 },	/* MTU black hole detection */

/* advanced-routing */
	{ "routes_static",		""				, 0 },
	{ "dhcpc_33",			"1"				, 0 },	/* DHCPC Static Route (option 33) */
	{ "dhcpc_121",			"1"				, 0 },	/* DHCPC Classless Static Route (option 121) */
	{ "force_igmpv2",		"0"				, 0 },
#ifdef TCONFIG_ZEBRA
	{ "dr_setting",			"0"				, 0 },	/* [ Disable | WAN | LAN | Both ] */
	{ "dr_lan_tx",			"0"				, 0 },	/* Dynamic-Routing LAN out */
	{ "dr_lan_rx",			"0"				, 0 },	/* Dynamic-Routing LAN in */
	{ "dr_wan_tx",			"0"				, 0 },	/* Dynamic-Routing WAN out */
	{ "dr_wan_rx",			"0"				, 0 },	/* Dynamic-Routing WAN in */
#endif /* TCONFIG_ZEBRA */

#ifndef TCONFIG_BCMARM
/* advanced-vlan */
	{ "trunk_vlan_so",		"0"				, 0 },	/* VLAN trunk support override */
#endif

/* advanced-wireless */
	{ "wl_txant",			"3"				, 0 },
#ifdef TCONFIG_BCMARM
	{ "wl_txpwr",			"0"				, 0 },
#else
	{ "wl_txpwr",			"42"				, 0 },
#endif
#if defined(TCONFIG_AC3200) && !defined(TCONFIG_BCM714)
	{ "wl_maxassoc",		"32"				, 0 },	/* SDK7: 32 for DHD (default); wlconf will check wireless driver maxassoc tuneable value */
	{ "wl_bss_maxassoc",		"32"				, 0 },
#else
	{ "wl_maxassoc",		"128"				, 0 },	/* Max associations driver could support (global max clients) */
	{ "wl_bss_maxassoc",		"128"				, 0 },
#endif
	{ "wl_distance",		""				, 0 },

/* forward-* */
#if defined(TCONFIG_NVRAM_32K) || defined(TCONFIG_OPTIMIZE_SIZE)
	{ "portforward",		""				, 0 },
	{ "trigforward",		""				, 0 },
#else
	{ "portforward",		"0<3<1.1.1.0/24<1000:2000<<192.168.1.2<ex: 1000 to 2000, restricted>0<2<<1000,2000<<192.168.1.2<ex: 1000 and 2000>0<1<<1000<2000<192.168.1.2<ex: different internal port>0<3<<1000:2000,3000<<192.168.1.2<ex: 1000 to 2000, and 3000>" , 0 },
	{ "trigforward",		"0<1<3000:4000<5000:6000<ex: open 5000-6000 if 3000-4000>"	, 0 },
#endif
#ifdef TCONFIG_IPV6
	{ "ipv6_portforward",		""				, 0 },
#endif
	{ "dmz_enable",			"0"				, 0 },
	{ "dmz_ipaddr",			"0"				, 0 },
	{ "dmz_sip",			""				, 0 },
	{ "dmz_ra",			"1"				, 0 },

/* forward-upnp */
	{ "upnp_enable",		"0"				, 0 },
	/* all other upnp_xyz variables, see upnp_defaults */

/* qos */
	{ "qos_enable",			"0"				, 0 },
#ifdef TCONFIG_BCMARM
	{ "qos_mode",			"1"				, 0 }, /* 1 = HTB + Leaf Qdisc, 2 = CAKE SQM */
	{ "qos_classify",		"1"				, 0 },
	{ "qos_pfifo",			"3"				, 0 },	/* Set FQ_Codel Default Qdisc Scheduler */
	{ "qos_cake_prio_mode",		"0"				, 0 },
	{ "qos_cake_wash",		"0"				, 0 },
#endif
	{ "qos_ack",			"0"				, 0 },
	{ "qos_syn",			"1"				, 0 },
	{ "qos_fin",			"1"				, 0 },
	{ "qos_rst",			"1"				, 0 },
	{ "qos_udp",			"0"				, 0 },
	{ "qos_icmp",			"1"				, 0 },
	{ "qos_reset",			"1"				, 0 },
	{ "wan_qos_obw",		"700"				, 0 },
	{ "wan_qos_ibw",		"16000"				, 0 },
#ifdef TCONFIG_BCMARM
	{ "wan_qos_encap",		"0"				, 0 },
#endif
	{ "wan_qos_overhead",		"0"				, 0 },
#if defined(TCONFIG_NVRAM_32K) || defined(TCONFIG_OPTIMIZE_SIZE)
	{ "qos_orules",			"0<<-1<d<53<0<<0:10<<0<DNS"	, 0 },
#else
	{ "qos_orules",			"0<<-1<d<53<<0:10<<0<DNS>0<<-1<d<37<<0:10<<0<Time>0<<17<d<123<<0:10<<0<NTP>0<<-1<d<3455<<0:10<<0<RSVP>0<<-1<d<9<<0:50<<3<SCTP, Discard>0<<-1<x<135,2101,2103,2105<<<<3<RPC (Microsoft)>0<<17<d<3544<<<<-1<Teredo Tunnel>0<<6<x<22,2222<<<<2<SSH>0<<6<d<23,992<<<<2<Telnet>0<<6<s<80,5938,8080,2222<<<<2<Remote Access>0<<-1<x<3389<<<<2<Remote Assistance>0<<-1<x<1220,6970:7170,8554<<<<4<Quicktime/RealAudio>0<<-1<x<554,5004,5005<<<<4<RTP, RTSP>0<<-1<x<1755<<<<4<MMS (Microsoft)>0<<-1<d<3478,3479,5060:5063<<<<1<SIP, Sipgate Stun Services>0<<-1<s<53,88,3074<<<<1<Xbox Live>0<<6<d<1718:1720<<<<1<H323>0<<-1<d<4380,27000:27050,11031,11235:11335,11999,2300:2400,6073,28800:29100,47624<<<<1<Various Games>0<<-1<d<1493,1502,1503,1542,1863,1963,3389,5061,5190:5193,7001<<<<5<MSGR1 - Windows Live>0<<-1<d<1071:1074,1455,1638,1644,5000:5010,5050,5100,5101,5150,8000:8002<<<<5<MSGR2 - Yahoo>0<<-1<d<194,1720,1730:1732,5220:5223,5298,6660:6669,22555<<<<5<MSGR3 - Additional>0<<-1<d<19294:19310<<<<5<Google+ & Voice>0<<6<d<6005,6006<<<<5<Camfrog>0<<-1<x<6571,6891:6901<<<<5<WLM File/Webcam>0<<-1<x<29613<<<<5<Skype incoming>0<<6<x<4244,5242<<<<1<Viber TCP>0<<17<x<5243,9785<<<<1<Viber UDP>0<<17<x<3478:3497,16384:16387,16393:16402<<<<5<Apple Facetime/Game Center>0<<6<d<443<<0:512<<3<HTTPS>0<<6<d<443<<512:<<5<HTTPS>0<<17<d<443<<0:512<<3<QUIC>0<<17<d<443<<512:<<5<QUIC>0<<-1<a<<youtube<<<4<YouTube>0<<-1<a<<rtp<<<4<RTP>0<<-1<a<<rtmp<<<4<RTMP>0<<-1<a<<irc<<<5<IRC>0<<6<d<80,8080<<0:512<<3<HTTP, HTTP Proxy>0<<6<d<80,8080<<512:<<7<HTTP, HTTP Proxy File Transfers>0<<6<d<20,21,989,990<<<<7<FTP>0<<6<d<25,587,465,2525<<<<6<SMTP, Submission Mail>0<<6<d<110,995<<<<6<POP3 Mail>0<<6<d<119,563<<<<7<NNTP News & Downloads>0<<6<d<143,220,585,993<<<<6<IMAP Mail>0<<17<d<1:65535<<<<8<P2P (uTP, UDP)" , 0 },
#endif
	{ "qos_burst0",			""				, 0 },
	{ "qos_burst1",			""				, 0 },
	{ "qos_default",		"8"				, 0 },
	{ "qos_orates",			"5-100,5-30,5-100,5-70,5-70,5-70,5-70,5-100,5-30,1-1"				, 0 },
	{ "qos_irates",			"5-100,2-20,5-100,10-90,20-90,5-90,5-70,5-100,5-30,1-1"				, 0 },
	{ "qos_classnames",		"Service VOIP/Game Remote WWW Media HTTPS/Msgr Mail FileXfer P2P/Bulk Crawl"	, 0 },

	{ "ne_vegas",			"0"				, 0 },	/* TCP Vegas */
	{ "ne_valpha",			"2"				, 0 },
	{ "ne_vbeta",			"6"				, 0 },
	{ "ne_vgamma",			"2"				, 0 },

/* access restrictions */
	{ "rruleN",			"0"				, 0 },
#if defined(TCONFIG_NVRAM_32K) || defined(TCONFIG_OPTIMIZE_SIZE)
	{ "rrule0",			""				, 0 },
#else
	{ "rrule0",			"0|1320|300|31|||word text\n^begins-with.domain.\n.ends-with.net$\n^www.exact-domain.net$|0|example" , 0 },
#endif
	{ "rrulewp",			"80,8080"			, 0 },

/* admin-access */
	{ "http_username",		""				, 0 },	/* Username */
	{ "http_passwd",		"admin"				, 0 },	/* Password */
	{ "remote_management",		"0"				, 0 },	/* Remote Management [1|0] */
	{ "http_wanport",		"8080"				, 0 },	/* WAN port to listen on */
	{ "http_lanport",		"80"				, 0 },	/* LAN port to listen on */
	{ "http_enable",		"1"				, 0 },	/* HTTP server enable/disable */
#ifndef TOMATO64
	{ "http_lan_listeners",		"7"				, 0 },	/* Enable listeners: bit 0 = LAN1, bit 1 = LAN2, bit 2 = LAN3 */
#else
	{ "http_lan_listeners",		"127"				, 0 },	/* Enable listeners: bit 0 = LAN1, bit 1 = LAN2, bit 2 = LAN3, bit 3 = LAN4, bit 4 = LAN5, bit 5 = LAN6, bit 6 = LAN7 */
#endif /* TOMATO64 */
#ifdef TCONFIG_IPV6
	{ "http_ipv6",			"1"				, 0 },	/* Start httpd on IPv6 interfaces */
#endif
	{ "remote_upgrade",		"1"				, 0 },	/* allow remote upgrade [1|0] - for brave guys */
	{ "http_wanport_bfm",		"1"				, 0 },	/* enable/disable brute force mitigation rule for WAN port */
#ifdef TCONFIG_HTTPS
	{ "remote_mgt_https",		"0"				, 0 },	/* Remote Management use https [1|0] */
	{ "https_lanport",		"443"				, 0 },	/* LAN port to listen on */
	{ "https_enable",		"0"				, 0 },	/* HTTPS server enable/disable */
	{ "https_crt_save",		"0"				, 0 },
	{ "https_crt_cn",		""				, 0 },
	{ "https_crt_file",		""				, 0 },
	{ "https_crt_gen",		""				, 0 },
#endif
	{ "web_wl_filter",		"0"				, 0 },	/* Allow/Deny Wireless Access Web */
#ifndef TOMATO64
	{ "web_css",			"default"			, 0 },
#else
	{ "web_css",			"bluegreen2"			, 0 },
#endif /* TOMATO64 */
#ifdef TCONFIG_ADVTHEMES
	{ "web_adv_scripts",		"0"				, 0 },	/* load JS resize chart script */
#endif
	{ "web_dir",			"default"			, 0 },	/* jffs, opt, tmp or default (/www) */
	{ "ttb_css",			"example"			, 0 },	/* Tomato Themes Base - default theme name */
#ifdef TCONFIG_USB
	{ "ttb_loc",			""				, 0 },	/* Tomato Themes Base - default files location */
	{ "ttb_url",			"http://ttb.mooo.com http://ttb.ath.cx http://ttb.ddnsfree.com", 0 },	/* Tomato Themes Base - default URL */
#endif
	{ "web_svg",			"1"				, 0 },
	{ "telnetd_eas",		"1"				, 0 },
	{ "telnetd_port",		"23"				, 0 },
	{ "sshd_eas",			"1"				, 0 },	/* enable sshd by default */
	{ "sshd_pass",			"1"				, 0 },
	{ "sshd_port",			"22"				, 0 },
	{ "sshd_remote",		"0"				, 0 },
	{ "sshd_motd",			"1"				, 0 },
	{ "sshd_rport",			"22"				, 0 },
	{ "sshd_authkeys",		""				, 0 },
	{ "sshd_hostkey",		""				, 0 },
	{ "sshd_dsskey",		""				, 0 },
	{ "sshd_ecdsakey",		""				, 0 },
	{ "sshd_forwarding",		"1"				, 0 },
	{ "rmgt_sip",			""				, 0 },	/* remote management: source ip address */
	{ "ne_shlimit",			"1,3,60"			, 0 },	/* enable limit connection attempts for sshd */
	{ "ipsec_pass",			"1"				, 0 },	/* Enable IPSec Passthrough 0=Disabled, 1=IPv4 + IPv6, 2=IPv6 only, 3=IPv4 only */

	{ "http_id",			""				, 0 },
	{ "web_mx",			"status,bwm"			, 0 },
	{ "web_pb",			""				, 0 },

/* admin-bwm */
	{ "rstats_enable",		"1"				, 0 },
	/* all other rstats_xyz variables, see rstats_defaults */

/* admin-ipt */
	{ "cstats_enable",		"0"				, 0 },
	/* all other cstats_xyz variables, see cstats_defaults */

/* advanced-buttons */
#ifdef TCONFIG_BCMARM
	{ "stealth_mode",		"0"				, 0 },
	{ "stealth_iled",		"0"				, 0 },
	{ "blink_wl",			"1"				, 0 },	/* enable blink by default (for wifi) */
	{ "sesx_led",			"12"				, 0 },	/* enable LEDs at startup: bit 0 = LED_AMBER, bit 1 = LED_WHITE, bit 2 = LED_AOSS, bit 3 = LED_BRIDGE; Default: LED_AOSS + LED_Bridge turned On */
#else
	{ "sesx_led",			"0"				, 0 },
#endif
	{ "sesx_b0",			"1"				, 0 },
	{ "sesx_b1",			"4"				, 0 },
	{ "sesx_b2",			"4"				, 0 },
	{ "sesx_b3",			"4"				, 0 },
	{ "sesx_script",
		"[ $1 -ge 20 ] && telnetd -p 233 -l /bin/sh\n"
	, 0 },
#ifndef TCONFIG_BCMARM
#if defined(TCONFIG_NVRAM_32K) || defined(TCONFIG_OPTIMIZE_SIZE)
	{ "script_brau",		""				},
#else
	{ "script_brau",
		"if [ ! -e /tmp/switch-start ]; then\n"
		"  # do something at startup\n"
		"  echo position at startup was $1 >/tmp/switch-start\n"
		"  exit\n"
		"fi\n"
		"if [ $1 = \"bridge\" ]; then\n"
		"  # do something\n"
		"  led bridge on\n"
		"elif [ $1 = \"auto\" ]; then\n"
		"  # do something\n"
		"  led bridge off\n"
		"fi\n"
	},
#endif
#endif /* !TCONFIG_BCMARM */

/* admin-log */
	{ "log_remote",			"0"				, 0 },
	{ "log_remoteip",		""				, 0 },
	{ "log_remoteport",		"514"				, 0 },
	{ "log_file",			"1"				, 0 },
	{ "log_file_custom",		"0"				, 0 },
	{ "log_file_path",		"/var/log/messages"		, 0 },
	{ "log_file_size",		"50"				, 0 },
	{ "log_file_keep",		"1"				, 0 },
	{ "log_limit",			"60"				, 0 },
	{ "log_in",			"0"				, 0 },
	{ "log_out",			"0"				, 0 },
	{ "log_mark",			"60"				, 0 },
	{ "log_events",			""				, 0 },
	{ "log_dropdups",		"0"				, 0 },
	{ "log_min_level",		"8"				, 0 },

/* admin-log-webmonitor */
	{ "log_wm",			"0"				, 0 },
	{ "log_wmtype",			"0"				, 0 },
	{ "log_wmip",			""				, 0 },
	{ "log_wmdmax",			"2000"				, 0 },
	{ "log_wmsmax",			"2000"				, 0 },
	{ "webmon_bkp",			"0"				, 0 },
	{ "webmon_dir",			"/tmp"				, 0 },
	{ "webmon_shrink",		"0"				, 0 },

/* admin-debugging */
	{ "debug_nocommit",		"0"				, 0 },
	{ "debug_cprintf",		"0"				, 0 },
	{ "debug_cprintf_file",		"0"				, 0 },
	{ "debug_logsegfault",		"0"				, 0 },
	{ "console_loglevel",		"1"				, 0 },
#ifndef TOMATO64
	{ "t_cafree",			"1"				, 0 },
#else
	{ "t_cafree",			"0"				, 0 },
#endif /* TOMATO64 */
	{ "t_hidelr",			"0"				, 0 },
	{ "debug_ddns",			"0"				, 0 },
#ifdef TCONFIG_BCM714
	{ "debug_wlx_shdown",		"0"				, 0 },	/* Shutdown wl radio eth1 (bit 0) and/or eth2 (bit 1) and/or eth3 (bit 2) */
#endif /* TCONFIG_BCM714 */
	{ "http_nocache",		"0"				, 0 },

/* admin-cifs */
	{ "cifs1",			""				, 0 },
	{ "cifs2",			""				, 0 },

/* admin-jffs2 */
	{ "jffs2_on",			"0"				, 0 },
	{ "jffs2_exec",			""				, 0 },
	{ "jffs2_auto_unmount",		"0"				, 0 },	/* automatically unmount JFFS2 during FW upgrade */

/* admin-tomatoanon */
	{ "tomatoanon_enable",		"0"				, 0 },
	{ "tomatoanon_notify",		"1"				, 0 },
	{ "tomatoanon_id",		""				, 0 },

#ifdef TCONFIG_USB
/* nas-usb */
	{ "usb_enable",			"1"				, 0 },
#ifndef TOMATO64
	{ "usb_uhci",			"0"				, 0 },
	{ "usb_ohci",			"0"				, 0 },
#else
	{ "usb_uhci",			"-1"				, 0 },
	{ "usb_ohci",			"1"				, 0 },
#endif /* TOMATO64 */
	{ "usb_usb2",			"1"				, 0 },
#ifdef TCONFIG_BCMARM
	{ "usb_usb3",			"0"				, 0 },
#endif
#ifdef TCONFIG_MICROSD
	{ "usb_mmc",			"-1"				, 0 },
#endif
	{ "usb_irq_thresh",		"0"				, 0 },
	{ "usb_storage",		"1"				, 0 },
	{ "usb_printer",		"0"				, 0 },
	{ "usb_printer_bidirect",	"0"				, 0 },
	{ "usb_ext_opt",		""				, 0 },
	{ "usb_fat_opt",		""				, 0 },
	{ "usb_ntfs_opt",		""				, 0 },
#ifdef TCONFIG_BCMARM
#ifndef TOMATO64
	{ "usb_fs_exfat",		"0"				, 0 },
#else
	{ "usb_fs_exfat",		"1"				, 0 },
#endif /* TOMATO64 */
	{ "usb_fs_ext4",		"1"				, 0 },
#else
	{ "usb_fs_ext3",		"1"				, 0 },
#endif
	{ "usb_fs_fat",			"1"				, 0 },
#ifdef TCONFIG_NTFS
	{ "usb_fs_ntfs",		"1"				, 0 },
#ifdef TCONFIG_BCMARM
#ifdef TCONFIG_TUXERA
	{ "usb_ntfs_driver",		"tuxera"			, 0 },
#elif defined(TCONFIG_UFSD)
	{ "usb_ntfs_driver",		"paragon"			, 0 },
#else
	{ "usb_ntfs_driver",		"ntfs3g"			, 0 },
#endif
#endif /* TCONFIG_BCMARM */
#endif /* TCONFIG_NTFS */
#ifdef TCONFIG_HFS
#ifndef TOMATO64
	{ "usb_fs_hfs",			"0"				, 0 },
#else
	{ "usb_fs_hfs",			"1"				, 0 },
#endif /* TOMATO64 */
#ifdef TCONFIG_BCMARM
#ifdef TCONFIG_TUX_HFS
	{ "usb_hfs_driver",		"tuxera"			, 0 },
#else
	{ "usb_hfs_driver",		"kernel"			, 0 },
#endif
#endif /* TCONFIG_BCMARM */
#endif /* TCONFIG_HFS */
#ifdef TCONFIG_ZFS
	{ "usb_fs_zfs",			"0"				, 0 },
	{ "usb_fs_zfs_automount",	"1"				, 0 },
	{ "zfs_mount_script",		""				, 0 },
#endif
#ifdef TCONFIG_UPS
	{ "usb_apcupsd",		"0"				, 0 },
	{ "usb_apcupsd_custom",		"0"				, 0 },	/* 1 - use custom config file /etc/apcupsd.conf */
#endif
	{ "usb_automount",		"1"				, 0 },
#if 0
	{ "usb_bdflush",		"30 500 0 0 100 100 60 0 0"	, 0 },
#endif
	{ "script_usbhotplug",		""				, 0 },
	{ "script_usbmount",		""				, 0 },
	{ "script_usbumount",		""				, 0 },
	{ "idle_enable",		"0"				, 0 },
#endif /* TCONFIG_USB */

#ifdef TCONFIG_FTP
/* nas-ftp */
	{ "ftp_enable",			"0"				, 0 },
	/* all other ftp_xyz variables, see ftp_defaults */
#endif /* TCONFIG_FTP */

#ifdef TCONFIG_SNMP
	{ "snmp_enable",		"0"				, 0 },
	/* all other snmp_xyz variables, see snmp_defaults */
#endif /* TCONFIG_SNMP */

#ifdef TCONFIG_SAMBASRV
/* nas-samba */
	{ "smbd_enable",		"0"				, 0 },
	{ "smbd_wgroup",		"WORKGROUP"			, 0 },
	{ "smbd_master",		"1"				, 0 },
	{ "smbd_wins",			"1"				, 0 },
	{ "smbd_cpage",			""				, 0 },
	{ "smbd_cset",			"utf8"				, 0 },
	{ "smbd_custom",		""				, 0 },
	{ "smbd_autoshare",		"2"				, 0 },
	{ "smbd_shares",		"jffs</jffs<JFFS<1<0>root$</<Hidden Root<0<1"	, 0 },
	{ "smbd_user",			"nas"				, 0 },
	{ "smbd_passwd",		""				, 0 },
	{ "smbd_ifnames",		"br0"				, 0 },
	{ "smbd_protocol",		"2"				, 0 },	/* 0 - SMB1, 1 - SMB2, 2 - SMB1+SMB2 (default) */
#ifdef TCONFIG_GROCTRL
	{ "gro_disable",		"1"				, 0 },	/* GRO enalbe - 0 ; disable - 1 (default) */
#endif
#endif /* TCONFIG_SAMBASRV */

#ifdef TCONFIG_MEDIA_SERVER
/* nas-media */
	{ "ms_enable",			"0"				, 0 },	/* 0:Disable 1:Enable 2:Enable&Rescan */
	{ "ms_dirs",			"/mnt<"				, 0 },
	{ "ms_port",			"0"				, 0 },
	{ "ms_dbdir",			""				, 0 },
	{ "ms_ifname",			"br0"				, 0 },
	{ "ms_tivo",			"0"				, 0 },
	{ "ms_stdlna",			"0"				, 0 },
	{ "ms_sas",			"0"				, 0 },
	{ "ms_autoscan",		"1"				, 0 },
	{ "ms_custom",			""				, 0 },
#endif /* TCONFIG_MEDIA_SERVER */

#ifdef TCONFIG_SDHC
/* admin-sdhc */
	{ "mmc_on",			"0"				, 0 },
	{ "mmc_cs",			"7"				, 0 },
	{ "mmc_clk",			"3"				, 0 },
	{ "mmc_din",			"2"				, 0 },
	{ "mmc_dout",			"4"				, 0 },
	{ "mmc_fs_partition",		"1"				, 0 },
	{ "mmc_fs_type",		"ext2"				, 0 },
	{ "mmc_exec_mount",		""				, 0 },
	{ "mmc_exec_umount",		""				, 0 },
#endif

/* admin-sch */
	{ "sch_rboot",			""				, 0 },
	{ "sch_rcon",			""				, 0 },
	{ "sch_c1",			""				, 0 },
	{ "sch_c2",			""				, 0 },
	{ "sch_c3",			""				, 0 },
	{ "sch_c4",			""				, 0 },
	{ "sch_c5",			""				, 0 },
	{ "sch_c1_cmd",			""				, 0 },
	{ "sch_c2_cmd",			""				, 0 },
	{ "sch_c3_cmd",			""				, 0 },
	{ "sch_c4_cmd",			""				, 0 },
	{ "sch_c5_cmd",			""				, 0 },

/* admin-script */
	{ "script_init",		""				, 0 },
	{ "script_shut",		""				, 0 },
	{ "script_fire",		""				, 0 },
	{ "script_wanup",		""				, 0 },
	{ "script_mwanup",		""				, 0 },

#ifdef TCONFIG_NFS
	{ "nfs_enable",			"0"				, 0 },
	{ "nfs_enable_v2",		"0"				, 0 },
	{ "nfs_exports",		""				, 0 },
#endif /* TCONFIG_NFS */

#ifdef TCONFIG_OPENVPN
/* vpn */
	{ "vpn_debug",			"0"				, 0 },
	{ "vpn_server_eas",		""				, 0 },
	{ "vpn_server_dns",		""				, 0 },
	{ "vpn_server1_poll",		"0"				, 0 },
	{ "vpn_server1_if",		"tun"				, 0 },
	{ "vpn_server1_proto",		"udp"				, 0 },
	{ "vpn_server1_port",		"1194"				, 0 },
	{ "vpn_server1_firewall",	"auto"				, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_server1_crypt",		"secret"			, 0 },
#else
	{ "vpn_server1_crypt",		"tls"				, 0 },
#endif
	{ "vpn_server1_comp",		"-1"				, 0 },
	{ "vpn_server1_cipher",		"AES-128-CBC"			, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_server1_ncp_ciphers",	"AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#else
	{ "vpn_server1_ncp_ciphers",	"CHACHA20-POLY1305:AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#endif
	{ "vpn_server1_digest",		"default"			, 0 },
	{ "vpn_server1_dhcp",		"1"				, 0 },
	{ "vpn_server1_r1",		"192.168.1.50"			, 0 },
	{ "vpn_server1_r2",		"192.168.1.55"			, 0 },
	{ "vpn_server1_sn",		"10.6.0.0"			, 0 },
	{ "vpn_server1_nm",		"255.255.255.0"			, 0 },
	{ "vpn_server1_local",		"10.6.0.1"			, 0 },
	{ "vpn_server1_remote",		"10.6.0.2"			, 0 },
	{ "vpn_server1_reneg",		"-1"				, 0 },
	{ "vpn_server1_hmac",		"-1"				, 0 },
	{ "vpn_server1_plan",		"1"				, 0 },
	{ "vpn_server1_pdns",		"0"				, 0 },
	{ "vpn_server1_ccd",		"0"				, 0 },
	{ "vpn_server1_c2c",		"0"				, 0 },
	{ "vpn_server1_ccd_excl",	"0"				, 0 },
	{ "vpn_server1_ccd_val",	""				, 0 },
	{ "vpn_server1_rgw",		"0"				, 0 },
	{ "vpn_server1_userpass",	"0"				, 0 },
	{ "vpn_server1_nocert",		"0"				, 0 },
	{ "vpn_server1_custom",		""				, 0 },
	{ "vpn_server1_static",		""				, 0 },
	{ "vpn_server1_ca",		""				, 0 },
	{ "vpn_server1_ca_key",		""				, 0 },
	{ "vpn_server1_crt",		""				, 0 },
	{ "vpn_server1_crl",		""				, 0 },
	{ "vpn_server1_key",		""				, 0 },
	{ "vpn_server1_dh",		""				, 0 },
	{ "vpn_server1_br",		"br0"				, 0 },
	{ "vpn_server1_ecdh",		"0"				, 0 },
	{ "vpn_server2_poll",		"0"				, 0 },
	{ "vpn_server2_if",		"tun"				, 0 },
	{ "vpn_server2_proto",		"udp"				, 0 },
	{ "vpn_server2_port",		"1195"				, 0 },
	{ "vpn_server2_firewall",	"auto"				, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_server2_crypt",		"secret"			, 0 },
#else
	{ "vpn_server2_crypt",		"tls"				, 0 },
#endif
	{ "vpn_server2_comp",		"-1"				, 0 },
	{ "vpn_server2_cipher",		"AES-128-CBC"			, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_server2_ncp_ciphers",	"AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#else
	{ "vpn_server2_ncp_ciphers",	"CHACHA20-POLY1305:AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#endif
	{ "vpn_server2_digest",		"default"			, 0 },
	{ "vpn_server2_dhcp",		"1"				, 0 },
	{ "vpn_server2_r1",		"192.168.1.50"			, 0 },
	{ "vpn_server2_r2",		"192.168.1.55"			, 0 },
	{ "vpn_server2_sn",		"10.7.0.0"			, 0 },
	{ "vpn_server2_nm",		"255.255.255.0"			, 0 },
	{ "vpn_server2_local",		"10.7.0.1"			, 0 },
	{ "vpn_server2_remote",		"10.7.0.2"			, 0 },
	{ "vpn_server2_reneg",		"-1"				, 0 },
	{ "vpn_server2_hmac",		"-1"				, 0 },
	{ "vpn_server2_plan",		"1"				, 0 },
	{ "vpn_server2_pdns",		"0"				, 0 },
	{ "vpn_server2_ccd",		"0"				, 0 },
	{ "vpn_server2_c2c",		"0"				, 0 },
	{ "vpn_server2_ccd_excl",	"0"				, 0 },
	{ "vpn_server2_ccd_val",	""				, 0 },
	{ "vpn_server2_rgw",		"0"				, 0 },
	{ "vpn_server2_userpass",	"0"				, 0 },
	{ "vpn_server2_nocert",		"0"				, 0 },
	{ "vpn_server2_custom",		""				, 0 },
	{ "vpn_server2_static",		""				, 0 },
	{ "vpn_server2_ca",		""				, 0 },
	{ "vpn_server2_ca_key",		""				, 0 },
	{ "vpn_server2_crt",		""				, 0 },
	{ "vpn_server2_crl",		""				, 0 },
	{ "vpn_server2_key",		""				, 0 },
	{ "vpn_server2_dh",		""				, 0 },
	{ "vpn_server2_br",		"br0"				, 0 },
	{ "vpn_server2_ecdh",		"0"				, 0 },
#ifdef TOMATO64
	{ "vpn_server3_poll",		"0"				, 0 },
	{ "vpn_server3_if",		"tun"				, 0 },
	{ "vpn_server3_proto",		"udp"				, 0 },
	{ "vpn_server3_port",		"1196"				, 0 },
	{ "vpn_server3_firewall",	"auto"				, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_server3_crypt",		"secret"			, 0 },
#else
	{ "vpn_server3_crypt",		"tls"				, 0 },
#endif
	{ "vpn_server3_comp",		"-1"				, 0 },
	{ "vpn_server3_cipher",		"AES-128-CBC"			, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_server3_ncp_ciphers",	"AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#else
	{ "vpn_server3_ncp_ciphers",	"CHACHA20-POLY1305:AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#endif
	{ "vpn_server3_digest",		"default"			, 0 },
	{ "vpn_server3_dhcp",		"1"				, 0 },
	{ "vpn_server3_r1",		"192.168.1.50"			, 0 },
	{ "vpn_server3_r2",		"192.168.1.55"			, 0 },
	{ "vpn_server3_sn",		"10.8.0.0"			, 0 },
	{ "vpn_server3_nm",		"255.255.255.0"			, 0 },
	{ "vpn_server3_local",		"10.8.0.1"			, 0 },
	{ "vpn_server3_remote",		"10.8.0.2"			, 0 },
	{ "vpn_server3_reneg",		"-1"				, 0 },
	{ "vpn_server3_hmac",		"-1"				, 0 },
	{ "vpn_server3_plan",		"1"				, 0 },
	{ "vpn_server3_pdns",		"0"				, 0 },
	{ "vpn_server3_ccd",		"0"				, 0 },
	{ "vpn_server3_c2c",		"0"				, 0 },
	{ "vpn_server3_ccd_excl",	"0"				, 0 },
	{ "vpn_server3_ccd_val",	""				, 0 },
	{ "vpn_server3_rgw",		"0"				, 0 },
	{ "vpn_server3_userpass",	"0"				, 0 },
	{ "vpn_server3_nocert",		"0"				, 0 },
	{ "vpn_server3_custom",		""				, 0 },
	{ "vpn_server3_static",		""				, 0 },
	{ "vpn_server3_ca",		""				, 0 },
	{ "vpn_server3_ca_key",		""				, 0 },
	{ "vpn_server3_crt",		""				, 0 },
	{ "vpn_server3_crl",		""				, 0 },
	{ "vpn_server3_key",		""				, 0 },
	{ "vpn_server3_dh",		""				, 0 },
	{ "vpn_server3_br",		"br0"				, 0 },
	{ "vpn_server3_ecdh",		"0"				, 0 },
	{ "vpn_server4_poll",		"0"				, 0 },
	{ "vpn_server4_if",		"tun"				, 0 },
	{ "vpn_server4_proto",		"udp"				, 0 },
	{ "vpn_server4_port",		"1197"				, 0 },
	{ "vpn_server4_firewall",	"auto"				, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_server4_crypt",		"secret"			, 0 },
#else
	{ "vpn_server4_crypt",		"tls"				, 0 },
#endif
	{ "vpn_server4_comp",		"-1"				, 0 },
	{ "vpn_server4_cipher",		"AES-128-CBC"			, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_server4_ncp_ciphers",	"AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#else
	{ "vpn_server4_ncp_ciphers",	"CHACHA20-POLY1305:AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#endif
	{ "vpn_server4_digest",		"default"			, 0 },
	{ "vpn_server4_dhcp",		"1"				, 0 },
	{ "vpn_server4_r1",		"192.168.1.50"			, 0 },
	{ "vpn_server4_r2",		"192.168.1.55"			, 0 },
	{ "vpn_server4_sn",		"10.9.0.0"			, 0 },
	{ "vpn_server4_nm",		"255.255.255.0"			, 0 },
	{ "vpn_server4_local",		"10.9.0.1"			, 0 },
	{ "vpn_server4_remote",		"10.9.0.2"			, 0 },
	{ "vpn_server4_reneg",		"-1"				, 0 },
	{ "vpn_server4_hmac",		"-1"				, 0 },
	{ "vpn_server4_plan",		"1"				, 0 },
	{ "vpn_server4_pdns",		"0"				, 0 },
	{ "vpn_server4_ccd",		"0"				, 0 },
	{ "vpn_server4_c2c",		"0"				, 0 },
	{ "vpn_server4_ccd_excl",	"0"				, 0 },
	{ "vpn_server4_ccd_val",	""				, 0 },
	{ "vpn_server4_rgw",		"0"				, 0 },
	{ "vpn_server4_userpass",	"0"				, 0 },
	{ "vpn_server4_nocert",		"0"				, 0 },
	{ "vpn_server4_custom",		""				, 0 },
	{ "vpn_server4_static",		""				, 0 },
	{ "vpn_server4_ca",		""				, 0 },
	{ "vpn_server4_ca_key",		""				, 0 },
	{ "vpn_server4_crt",		""				, 0 },
	{ "vpn_server4_crl",		""				, 0 },
	{ "vpn_server4_key",		""				, 0 },
	{ "vpn_server4_dh",		""				, 0 },
	{ "vpn_server4_br",		"br0"				, 0 },
	{ "vpn_server4_ecdh",		"0"				, 0 },
#endif /* TOMATO64 */
	{ "vpn_client_eas",		""				, 0 },
	{ "vpn_client1_poll",		"0"				, 0 },
	{ "vpn_client1_if",		"tun"				, 0 },
	{ "vpn_client1_bridge",		"1"				, 0 },
	{ "vpn_client1_nat",		"1"				, 0 },
	{ "vpn_client1_proto",		"udp"				, 0 },
	{ "vpn_client1_addr",		""				, 0 },
	{ "vpn_client1_port",		"1194"				, 0 },
	{ "vpn_client1_retry",		"30"				, 0 },
	{ "vpn_client1_rg",		"0"				, 0 },
	{ "vpn_client1_firewall",	"auto"				, 0 },
	{ "vpn_client1_crypt",		"tls"				, 0 },
	{ "vpn_client1_comp",		"-1"				, 0 },
	{ "vpn_client1_cipher",		"default"			, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_client1_ncp_ciphers",	"AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#else
	{ "vpn_client1_ncp_ciphers",	"CHACHA20-POLY1305:AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#endif
	{ "vpn_client1_digest",		"default"			, 0 },
	{ "vpn_client1_local",		"10.8.0.2"			, 0 },
	{ "vpn_client1_remote",		"10.8.0.1"			, 0 },
	{ "vpn_client1_nm",		"255.255.255.0"			, 0 },
	{ "vpn_client1_reneg",		"-1"				, 0 },
	{ "vpn_client1_hmac",		"-1"				, 0 },
	{ "vpn_client1_adns",		"0"				, 0 },
	{ "vpn_client1_rgw", 		"0"				, 0 },
	{ "vpn_client1_gw",		""				, 0 },
	{ "vpn_client1_custom",		""				, 0 },
	{ "vpn_client1_static",		""				, 0 },
	{ "vpn_client1_ca",		""				, 0 },
	{ "vpn_client1_crt",		""				, 0 },
	{ "vpn_client1_key",		""				, 0 },
	{ "vpn_client1_br",		"br0"				, 0 },
	{ "vpn_client1_nobind",		"1"				, 0 },
	{ "vpn_client1_routing_val",	""				, 0 },
	{ "vpn_client1_fw",		"1"				, 0 },
	{ "vpn_client1_tlsvername",	"0"				, 0 },
	{ "vpn_client1_prio",		""				, 0 },
	{ "vpn_client2_poll",		"0"				, 0 },
	{ "vpn_client2_if",		"tun"				, 0 },
	{ "vpn_client2_bridge",		"1"				, 0 },
	{ "vpn_client2_nat",		"1"				, 0 },
	{ "vpn_client2_proto",		"udp"				, 0 },
	{ "vpn_client2_addr",		""				, 0 },
	{ "vpn_client2_port",		"1194"				, 0 },
	{ "vpn_client2_retry",		"30"				, 0 },
	{ "vpn_client2_rg",		"0"				, 0 },
	{ "vpn_client2_firewall",	"auto"				, 0 },
	{ "vpn_client2_crypt",		"tls"				, 0 },
	{ "vpn_client2_comp",		"-1"				, 0 },
	{ "vpn_client2_cipher",		"default"			, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_client2_ncp_ciphers",	"AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#else
	{ "vpn_client2_ncp_ciphers",	"CHACHA20-POLY1305:AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#endif
	{ "vpn_client2_digest",		"default"			, 0 },
	{ "vpn_client2_local",		"10.9.0.2"			, 0 },
	{ "vpn_client2_remote",		"10.9.0.1"			, 0 },
	{ "vpn_client2_nm",		"255.255.255.0"			, 0 },
	{ "vpn_client2_reneg",		"-1"				, 0 },
	{ "vpn_client2_hmac",		"-1"				, 0 },
	{ "vpn_client2_adns",		"0"				, 0 },
	{ "vpn_client2_rgw",		"0"				, 0 },
	{ "vpn_client2_gw",		""				, 0 },
	{ "vpn_client2_custom",		""				, 0 },
	{ "vpn_client2_static",		""				, 0 },
	{ "vpn_client2_ca",		""				, 0 },
	{ "vpn_client2_crt",		""				, 0 },
	{ "vpn_client2_key",		""				, 0 },
	{ "vpn_client2_br",		"br0"				, 0 },
	{ "vpn_client2_nobind",		"1"				, 0 },
	{ "vpn_client2_routing_val",	""				, 0 },
	{ "vpn_client2_fw",		"1"				, 0 },
	{ "vpn_client2_tlsvername",	"0"				, 0 },
	{ "vpn_client2_prio",		""				, 0 },
#ifdef TCONFIG_BCMARM
	{ "vpn_client3_poll",		"0"				, 0 },
	{ "vpn_client3_if",		"tun"				, 0 },
	{ "vpn_client3_bridge",		"1"				, 0 },
	{ "vpn_client3_nat",		"1"				, 0 },
	{ "vpn_client3_proto",		"udp"				, 0 },
	{ "vpn_client3_addr",		""				, 0 },
	{ "vpn_client3_port",		"1194"				, 0 },
	{ "vpn_client3_retry",		"30"				, 0 },
	{ "vpn_client3_rg",		"0"				, 0 },
	{ "vpn_client3_firewall",	"auto"				, 0 },
	{ "vpn_client3_crypt",		"tls"				, 0 },
	{ "vpn_client3_comp",		"-1"				, 0 },
	{ "vpn_client3_cipher",		"default"			, 0 },
#ifdef TCONFIG_OPTIMIZE_SIZE_MORE
	{ "vpn_client3_ncp_ciphers",	"AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#else
	{ "vpn_client3_ncp_ciphers",	"CHACHA20-POLY1305:AES-128-GCM:AES-256-GCM:AES-128-CBC:AES-256-CBC", 0 },
#endif
	{ "vpn_client3_digest",		"default"			, 0 },
	{ "vpn_client3_local",		"10.10.0.2"			, 0 },
	{ "vpn_client3_remote",		"10.10.0.1"			, 0 },
	{ "vpn_client3_nm",		"255.255.255.0"			, 0 },
	{ "vpn_client3_reneg",		"-1"				, 0 },
	{ "vpn_client3_hmac",		"-1"				, 0 },
	{ "vpn_client3_adns",		"0"				, 0 },
	{ "vpn_client3_rgw",		"0"				, 0 },
	{ "vpn_client3_gw",		""				, 0 },
	{ "vpn_client3_custom",		""				, 0 },
	{ "vpn_client3_static",		""				, 0 },
	{ "vpn_client3_ca",		""				, 0 },
	{ "vpn_client3_crt",		""				, 0 },
	{ "vpn_client3_key",		""				, 0 },
	{ "vpn_client3_br",		"br0"				, 0 },
	{ "vpn_client3_nobind",		"1"				, 0 },
	{ "vpn_client3_routing_val",	""				, 0 },
	{ "vpn_client3_fw",		"1"				, 0 },
	{ "vpn_client3_tlsvername",	"0"				, 0 },
	{ "vpn_client3_prio",		""				, 0 },
#endif /* TCONFIG_BCMARM */
#endif /* TCONFIG_OPENVPN */

#ifdef TCONFIG_PPTPD
	{ "pptpc_eas",			"0"				, 0 },
	{ "pptpc_usewan",		"none"				, 0 },
	{ "pptpc_peerdns",		"0"				, 0 },
	{ "pptpc_mtuenable",		"0"				, 0 },
	{ "pptpc_mtu",			"1400"				, 0 },
	{ "pptpc_mruenable",		"0"				, 0 },
	{ "pptpc_mru",			"1400"				, 0 },
	{ "pptpc_nat",			"0"				, 0 },
	{ "pptpc_srvip",		""				, 0 },
	{ "pptpc_srvsub",		"10.0.0.0"			, 0 },
	{ "pptpc_srvsubmsk",		"255.0.0.0"			, 0 },
	{ "pptpc_username",		""				, 0 },
	{ "pptpc_passwd",		""				, 0 },
	{ "pptpc_crypt",		"0"				, 0 },
	{ "pptpc_custom",		""				, 0 },
	{ "pptpc_dfltroute",		"0"				, 0 },
	{ "pptpc_stateless",		"1"				, 0 },
	{ "pptpd_chap",			"0"				, 0 },	/* 0/1/2 (Auto/MS-CHAPv1/MS-CHAPv2) */
#endif /* TCONFIG_PPTPD */

#ifdef TCONFIG_TINC
	{"tinc_enable",			"0"				, 0 },
	{"tinc_name",			""				, 0 },
	{"tinc_devicetype",		"tun"				, 0 },	/* tun, tap */
	{"tinc_mode",			"switch"			, 0 },	/* switch, hub */
	{"tinc_vpn_netmask",		"255.255.0.0"			, 0 },
	{"tinc_private_rsa",		""				, 0 },
	{"tinc_private_ed25519",	""				, 0 },
	{"tinc_custom",			""				, 0 },
	{"tinc_hosts",			""				, 0 },
	{"tinc_manual_firewall",	""				, 0 },
	{"tinc_manual_tinc_up",		"0"				, 0 },
	{"tinc_poll",			"0"				, 0 },
	/* scripts */
	{"tinc_tinc_up",		""				, 0 },
	{"tinc_tinc_down",		""				, 0 },
	{"tinc_host_up",		""				, 0 },
	{"tinc_host_down",		""				, 0 },
	{"tinc_subnet_up",		""				, 0 },
	{"tinc_subnet_down",		""				, 0 },
	{"tinc_firewall",		""				, 0 },
#endif /* TCONFIG_TINC */

#ifdef TCONFIG_WIREGUARD
	{"wg_adns",			""				, 0 },
	{"wg0_enable",			"0"				, 0 },
	{"wg0_poll",			"0"				, 0 },
	{"wg0_file",			""				, 0 },
	{"wg0_key",			""				, 0 },
	{"wg0_endpoint",		""				, 0 },
	{"wg0_port",			""				, 0 },
	{"wg0_ip",			"10.11.0.1/24"			, 0 },
	{"wg0_fwmark",			"0"				, 0 },
	{"wg0_mtu",			"1420"				, 0 },
	{"wg0_preup",			""				, 0 },
	{"wg0_postup",			""				, 0 },
	{"wg0_predown",			""				, 0 },
	{"wg0_postdown",		""				, 0 },
	{"wg0_aip",			""				, 0 },
	{"wg0_dns",			""				, 0 },
	{"wg0_ka",			"0"				, 0 },
	{"wg0_com",			"0"				, 0 },
	{"wg0_lan",			"0"				, 0 },	/* push LANX for wg0 to peers: bit 0 = LAN0, bit 1 = LAN1, bit 2 = LAN2, bit 3 = WAN3 */
	{"wg0_rgw",			"0"				, 0 },
	{"wg0_route",			""				, 0 },
	{"wg0_peer_dns",		""				, 0 },
	{"wg0_peers",			""				, 0 },
	{"wg0_firewall",		"auto"				, 0 },	/* auto, custom */
	{"wg0_nat",			"1"				, 0 },
	{"wg0_fw",			"1"				, 0 },
	{"wg0_rgwr",			"1"				, 0 },
	{"wg0_routing_val",		""				, 0 },
	{"wg0_prio",			""				, 0 },
	{"wg1_enable",			"0"				, 0 },
	{"wg1_poll",			"0"				, 0 },
	{"wg1_file",			""				, 0 },
	{"wg1_key",			""				, 0 },
	{"wg1_endpoint",		""				, 0 },
	{"wg1_port",			""				, 0 },
	{"wg1_ip",			"10.12.0.1/24"			, 0 },
	{"wg1_fwmark",			"0"				, 0 },
	{"wg1_mtu",			"1420"				, 0 },
	{"wg1_preup",			""				, 0 },
	{"wg1_postup",			""				, 0 },
	{"wg1_predown",			""				, 0 },
	{"wg1_postdown",		""				, 0 },
	{"wg1_aip",			""				, 0 },
	{"wg1_dns",			""				, 0 },
	{"wg1_ka",			"0"				, 0 },
	{"wg1_com",			"0"				, 0 },
	{"wg1_lan",			"0"				, 0 },	/* push LANX for wg1 to peers: bit 0 = LAN0, bit 1 = LAN1, bit 2 = LAN2, bit 3 = WAN3 */
	{"wg1_rgw",			"0"				, 0 },
	{"wg1_route",			""				, 0 },
	{"wg1_peer_dns",		""				, 0 },
	{"wg1_peers",			""				, 0 },
	{"wg1_firewall",		"auto"				, 0 },	/* auto, custom */
	{"wg1_nat",			"1"				, 0 },
	{"wg1_fw",			"1"				, 0 },
	{"wg1_rgwr",			"1"				, 0 },
	{"wg1_routing_val",		""				, 0 },
	{"wg1_prio",			""				, 0 },
	{"wg2_enable",			"0"				, 0 },
	{"wg2_poll",			"0"				, 0 },
	{"wg2_file",			""				, 0 },
	{"wg2_key",			""				, 0 },
	{"wg2_endpoint",		""				, 0 },
	{"wg2_port",			""				, 0 },
	{"wg2_ip",			"10.13.0.1/24"			, 0 },
	{"wg2_fwmark",			"0"				, 0 },
	{"wg2_mtu",			"1420"				, 0 },
	{"wg2_preup",			""				, 0 },
	{"wg2_postup",			""				, 0 },
	{"wg2_predown",			""				, 0 },
	{"wg2_postdown",		""				, 0 },
	{"wg2_aip",			""				, 0 },
	{"wg2_dns",			""				, 0 },
	{"wg2_ka",			"0"				, 0 },
	{"wg2_com",			"0"				, 0 },
	{"wg2_lan",			"0"				, 0 },	/* push LANX for wg2 to peers: bit 0 = LAN0, bit 1 = LAN1, bit 2 = LAN2, bit 3 = WAN3 */
	{"wg2_rgw",			"0"				, 0 },
	{"wg2_route",			""				, 0 },
	{"wg2_peer_dns",		""				, 0 },
	{"wg2_peers",			""				, 0 },
	{"wg2_firewall",		"auto"				, 0 },	/* auto, custom */
	{"wg2_nat",			"1"				, 0 },
	{"wg2_fw",			"1"				, 0 },
	{"wg2_rgwr",			"1"				, 0 },
	{"wg2_routing_val",		""				, 0 },
	{"wg2_prio",			""				, 0 },
#endif /* TCONFIG_WIREGUARD */

#ifdef TCONFIG_BT
/* nas-transmission */
	{ "bt_enable",			"0"				, 0 },
#ifdef TCONFIG_BBT
	{ "bt_binary",			"internal"			, 0 },
#else
	{ "bt_binary",			"optware"			, 0 },
#endif /* TCONFIG_BBT */
	{ "bt_binary_custom",		"/path/to/binaries/directory"	, 0 },
	{ "bt_custom",			""				, 0 },
	{ "bt_port",			"51515"				, 0 },
	{ "bt_dir",			"/mnt"				, 0 },
	{ "bt_incomplete",		"1"				, 0 },
	{ "bt_autoadd",			"1"				, 0 },
	{ "bt_settings",		"down_dir"			, 0 },
	{ "bt_settings_custom",		"/etc/transmission"		, 0 },
	{ "bt_rpc_enable",		"1"				, 0 },
	{ "bt_rpc_wan",			"0"				, 0 },
	{ "bt_auth",			"1"				, 0 },
	{ "bt_login",			"admin"				, 0 },
	{ "bt_password",		"admin11"			, 0 },
	{ "bt_port_gui",		"9091"				, 0 },
	{ "bt_dl_enable",		"0"				, 0 },
	{ "bt_ul_enable",		"0"				, 0 },
	{ "bt_dl",			"248"				, 0 },
	{ "bt_ul",			"64"				, 0 },
	{ "bt_peer_limit_global",	"150"				, 0 },
	{ "bt_peer_limit_per_torrent",	"30"				, 0 },
	{ "bt_ul_slot_per_torrent",	"10"				, 0 },
	{ "bt_ratio_enable",		"0"				, 0 },
	{ "bt_ratio",			"1.0000"			, 0 },
	{ "bt_ratio_idle_enable",	"0"				, 0 },
	{ "bt_ratio_idle",		"30"				, 0 },
	{ "bt_dht",			"0"				, 0 },
	{ "bt_pex",			"0"				, 0 },
	{ "bt_lpd",			"0"				, 0 },
	{ "bt_utp",			"1"				, 0 },
	{ "bt_blocklist",		"0"				, 0 },
	{ "bt_blocklist_url",		"http://list.iblocklist.com/?list=bt_level1"	, 0 },
	{ "bt_sleep",			"10"				, 0 },
	{ "bt_check_time",		"15"				, 0 },
	{ "bt_dl_queue_enable",		"0"				, 0 },
	{ "bt_dl_queue_size",		"5"				, 0 },
	{ "bt_ul_queue_enable",		"0"				, 0 },
	{ "bt_ul_queue_size",		"5"				, 0 },
	{ "bt_message",			"2"				, 0 },
	{ "bt_log",			"0"				, 0 },
	{ "bt_log_path",		"/var/log"			, 0 },
#endif /* TCONFIG_BT */

/* bwlimit */
	{ "bwl_enable",			"0"				, 0 },
	{ "bwl_rules",			""				, 0 },
	{ "bwl_lan_enable",		"0"				, 0 },
	{ "bwl_lan_dlc",		""				, 0 },
	{ "bwl_lan_ulc",		""				, 0 },
	{ "bwl_lan_dlr",		""				, 0 },
	{ "bwl_lan_ulr",		""				, 0 },
	{ "bwl_lan_tcp",		"0"				, 0 },	/* unlimited */
	{ "bwl_lan_udp",		"0"				, 0 },	/* unlimited */
	{ "bwl_lan_prio",		"3"				, 0 },

#ifdef TCONFIG_NOCAT
	{ "NC_enable",			"0"				, 0 },	/* enable NoCatSplash */
	{ "NC_Verbosity",		"2"				, 0 },
	{ "NC_GatewayName",		"Tomato64 Captive Portal"	, 0 },
	{ "NC_GatewayPort",		"5280"				, 0 },
	{ "NC_GatewayMode",		"Open"				, 0 },
	{ "NC_DocumentRoot",		"/tmp/splashd"			, 0 },
	{ "NC_ExcludePorts",		"1863"				, 0 },
	{ "NC_HomePage",		"https://startpage.com"		, 0 },
	{ "NC_ForcedRedirect",		"0"				, 0 },
	{ "NC_IdleTimeout",		"0"				, 0 },
	{ "NC_MaxMissedARP",		"5"				, 0 },
	{ "NC_PeerChecktimeout",	"0"				, 0 },
	{ "NC_LoginTimeout",		"3600"				, 0 },
	{ "NC_RenewTimeout",		"0"				, 0 },
	{ "NC_AllowedWebHosts",		""				, 0 },
	{ "NC_BridgeLAN",		"br0"				, 0 },
#endif /* TCONFIG_NOCAT */

#ifdef TCONFIG_NGINX
	{"nginx_enable",		"0"				, 0 },	/* NGinX enabled */
	{"nginx_php",			"0"				, 0 },	/* PHP enabled */
	{"nginx_keepconf",		"0"				, 0 },	/* Enable/disable keep configuration files unmodified in /etc/nginx */
	{"nginx_docroot",		"/www"				, 0 },	/* path for server files */
	{"nginx_port",			"85"				, 0 },	/* port to listen */
	{"nginx_remote",		"0"				, 0 },	/* open port from WAN side */
	{"nginx_fqdn",			"Tomato64"			, 0 },	/* server name */
	{"nginx_upload",		"100"				, 0 },	/* upload file size limit */
	{"nginx_priority",		"10"				, 0 },	/* server priority = worker_priority */
	{"nginx_custom",		""				, 0 },	/* additional lines for nginx.conf */
	{"nginx_httpcustom",		""				, 0 },	/* additional lines for nginx.conf */
	{"nginx_servercustom",		""				, 0 },	/* additional lines for nginx.conf */
	{"nginx_phpconf",		""				, 0 },	/* additional lines for php.ini */
#ifdef TCONFIG_BCMARM
	{"nginx_phpfpmconf",		""				, 0 },	/* additional lines for php-fpm.conf */
#endif
	{"nginx_user",			"root"				, 0 },	/* user/group */
	{"nginx_override",		"0"				, 0 },	/* use user config */
	{"nginx_overridefile",		"/path/to/nginx.conf"		, 0 },	/* path/to/user/nginx.conf */
	{"nginx_h5aisupport",		"0"				, 0 },	/* enable h5ai support */

	{ "mysql_enable",		"0"				, 0 },
	{ "mysql_sleep",		"2"				, 0 },
	{ "mysql_check_time",		"5"				, 0 },
	{ "mysql_binary",		"internal"			, 0 },
	{ "mysql_binary_custom",	"/mnt/sda1/mysql/bin"		, 0 },
#ifndef TOMATO64
	{ "mysql_usb_enable",		"1"				, 0 },
#else
	{ "mysql_usb_enable",		"0"				, 0 },
#endif /* TOMATO64 */
	{ "mysql_dlroot",		""				, 0 },
#ifndef TOMATO64
	{ "mysql_datadir",		"data"				, 0 },
	{ "mysql_tmpdir",		"tmp"				, 0 },
#else
	{ "mysql_datadir",		"/mysql/data"			, 0 },
	{ "mysql_tmpdir",		"/mysql/tmp"			, 0 },
#endif /* TOMATO64 */
	{ "mysql_server_custom",	""				, 0 },
	{ "mysql_port",			"3306"				, 0 },
	{ "mysql_allow_anyhost",	"0"				, 0 },
	{ "mysql_init_rootpass",	"0"				, 0 },
	{ "mysql_username",		"root"				, 0 },	/* mysqladmin username */
	{ "mysql_passwd",		"admin"				, 0 },	/* mysqladmin password */
	{ "mysql_key_buffer",		"16"				, 0 },	/* KB */
	{ "mysql_max_allowed_packet",	"4"				, 0 },	/* MB */
	{ "mysql_thread_stack",		"128"				, 0 },	/* KB */
	{ "mysql_thread_cache_size",	"8"				, 0 },
	{ "mysql_init_priv",		"0"				, 0 },
	{ "mysql_table_open_cache",	"4"				, 0 },
	{ "mysql_sort_buffer_size",	"128"				, 0 },	/* KB */
	{ "mysql_read_buffer_size",	"128"				, 0 },	/* KB */
	{ "mysql_query_cache_size",	"16"				, 0 },	/* MB */
	{ "mysql_read_rnd_buffer_size",	"256"				, 0 },	/* KB */
	{ "mysql_net_buffer_length",	"2"				, 0 },	/* K */
	{ "mysql_max_connections",	"100"				, 0 },
#endif /* TCONFIG_NGINX */

#ifdef TCONFIG_TOR
	{ "tor_enable",			"0"				, 0 },
	{ "tor_solve_only",		"0"				, 0 },
	{ "tor_socksport",		"9050"				, 0 },
	{ "tor_transport",		"9040"				, 0 },
	{ "tor_dnsport",		"9053"				, 0 },
	{ "tor_datadir",		"/tmp/tor"			, 0 },
	{ "tor_iface",			"br0"				, 0 },
	{ "tor_users",			"192.168.1.0/24"		, 0 },
	{ "tor_custom",			""				, 0 },
	{ "tor_ports",			"80"				, 0 },
	{ "tor_ports_custom",		"80,443,8080:8880"		, 0 },
#endif /* TCONFIG_TOR */
#if MWAN_MAX >= 2
 WAN_BLOCK(2)
#endif
#if MWAN_MAX >= 3
 WAN_BLOCK(3)
#endif
#if MWAN_MAX >= 4
 WAN_BLOCK(4)
#endif
#if MWAN_MAX >= 5
 WAN_BLOCK(5)
#endif
#if MWAN_MAX >= 6
 WAN_BLOCK(6)
#endif
#if MWAN_MAX >= 7
 WAN_BLOCK(7)
#endif
#if MWAN_MAX >= 8
 WAN_BLOCK(8)
#endif
#if BRIDGE_COUNT >= 2
 BRIDGE_BLOCK(1)
#endif
#if BRIDGE_COUNT >= 3
 BRIDGE_BLOCK(2)
#endif
#if BRIDGE_COUNT >= 4
 BRIDGE_BLOCK(3)
#endif
#if BRIDGE_COUNT >= 5
 BRIDGE_BLOCK(4)
#endif
#if BRIDGE_COUNT >= 6
 BRIDGE_BLOCK(5)
#endif
#if BRIDGE_COUNT >= 7
 BRIDGE_BLOCK(6)
#endif
#if BRIDGE_COUNT >= 8
 BRIDGE_BLOCK(7)
#endif
#if BRIDGE_COUNT >= 9
 BRIDGE_BLOCK(8)
#endif
#if BRIDGE_COUNT >= 10
 BRIDGE_BLOCK(9)
#endif
#if BRIDGE_COUNT >= 11
 BRIDGE_BLOCK(10)
#endif
#if BRIDGE_COUNT >= 12
 BRIDGE_BLOCK(11)
#endif
#if BRIDGE_COUNT >= 13
 BRIDGE_BLOCK(12)
#endif
#if BRIDGE_COUNT >= 14
 BRIDGE_BLOCK(13)
#endif
#if BRIDGE_COUNT >= 15
 BRIDGE_BLOCK(14)
#endif
#if BRIDGE_COUNT >= 16
 BRIDGE_BLOCK(15)
#endif

#ifdef TOMATO64
	{ "set_macs",			"0"				, 0 },
	{ "fs_expanded",		"0"				, 0 },
	{ "fs_mount_ro",		"0"				, 0 },

	{ "wan_ifnameX",		"eth0"				, 0 },
	{ "wan_ifnameX_vlan",		"vlan0"				, 0 },

	{ "lan_ifname",			"br0"				, 0 },
#ifdef TOMATO64_X86_64
	{ "lan_ifnames",		"eth1 eth2 eth3 eth4 eth5 eth6 eth7 eth8"	, 0 },
#endif /* TOMATO64_X86_64 */
#ifdef TOMATO64_MT6000
	{ "lan_ifnames",		"eth1 eth2 eth3 eth4 eth5"	, 0 },
#endif /* TOMATO64_MT6000 */
#ifdef TOMATO64_BPIR3
	{ "lan_ifnames",		"eth1 eth2 eth3 eth4 eth5 eth6"	, 0 },
#endif /* TOMATO64_BPIR3 */
#ifdef TOMATO64_BPIR3MINI
	{ "lan_ifnames",		"eth1"				, 0 },
#endif /* TOMATO64_BPIR3MINI */
#ifdef TOMATO64_RPI4
	{ "lan_ifnames",		"eth1 eth2 eth3 eth4"				, 0 },
#endif /* TOMATO64_RPI4 */
	{ "lan_ifnames_vlan",		"vlan1"				, 0 },

	{ "boardflags",			"0x0100"			, 0 },
	{ "vlan0ports",			"0 9"				, 0 },
#ifdef TOMATO64_X86_64
	{ "vlan1ports",			"1 2 3 4 5 6 7 8 9*"		, 0 },
#endif /* TOMATO64_X86_64 */
#ifdef TOMATO64_MT6000
	{ "vlan1ports",			"1 2 3 4 5 9*"			, 0 },
#endif /* TOMATO64_MT6000 */
#ifdef TOMATO64_BPIR3
	{ "vlan1ports",			"1 2 3 4 5 6 9*"		, 0 },
#endif /* TOMATO64_BPIR3 */
#ifdef TOMATO64_BPIR3MINI
	{ "vlan1ports",			"1 9*"				, 0 },
#endif /* TOMATO64_BPIR3MINI */
#ifdef TOMATO64_RPI4
	{ "vlan1ports",			"1 2 3 4 9*"				, 0 },
#endif /* TOMATO64_RPI4 */
#ifdef TOMATO64_WIFI
	{"wifi_sta_list",		""				, 0 },
	{"wifi_phy_count",		"0"				, 0 },	/* Detected PHY count (cleared on boot) */

	/* Expected PHY count - device-specific constant */
#if defined(TOMATO64_MT6000)
	{"wifi_phy_count_expected",	"2"				, 0 },	/* MT6000: 2.4GHz + 5GHz */
#elif defined(TOMATO64_BPIR3)
	{"wifi_phy_count_expected",	"2"				, 0 },	/* BPI-R3: 2.4GHz + 5GHz */
#elif defined(TOMATO64_BPIR3MINI)
	{"wifi_phy_count_expected",	"2"				, 0 },	/* BPI-R3 Mini: 2.4GHz + 5GHz */
#elif defined(TOMATO64_RPI4)
	{"wifi_phy_count_expected",	"1"				, 0 },	/* RPI4: single dual-band PHY (2.4GHz + 5GHz) */
#else
	{"wifi_phy_count_expected",	"0"				, 0 },	/* Unknown device: no wifi by default */
#endif

	{"wifi_phy0_band",		"2g"				, 0 },
	{"wifi_phy0_mode",		"ax"				, 0 },
	{"wifi_phy0_channel",		"auto"				, 0 },
	{"wifi_phy0_width",		"20"				, 0 },
	{"wifi_phy0_brates",		""				, 0 },
	{"wifi_phy0_noscan",		""				, 0 },
	{"wifi_phy0_power",		""				, 0 },
	{"wifi_phy0_country",		""				, 0 },
	{"wifi_phy0_ifaces",		"1"				, 0 },

	{"wifi_phy1_band",		"5g"				, 0 },
	{"wifi_phy1_mode",		"ax"				, 0 },
	{"wifi_phy1_channel",		"auto"				, 0 },
	{"wifi_phy1_width",		"80"				, 0 },
	{"wifi_phy1_brates",		""				, 0 },
	{"wifi_phy1_noscan",		""				, 0 },
	{"wifi_phy1_power",		""				, 0 },
	{"wifi_phy1_country",		""				, 0 },
	{"wifi_phy1_ifaces",		"1"				, 0 },

	{"wifi_phy2_band",		"5g"				, 0 },
	{"wifi_phy2_mode",		"ax"				, 0 },
	{"wifi_phy2_channel",		"auto"				, 0 },
	{"wifi_phy2_width",		"80"				, 0 },
	{"wifi_phy2_brates",		""				, 0 },
	{"wifi_phy2_noscan",		""				, 0 },
	{"wifi_phy2_power",		""				, 0 },
	{"wifi_phy2_country",		""				, 0 },
	{"wifi_phy2_ifaces",		"1"				, 0 },

	{"wifi_phy0iface0_enable",	"1"				, 0 },
	{"wifi_phy0iface0_mode",	"ap"				, 0 },
	{"wifi_phy0iface0_essid",	"Tomato64"			, 0 },
	{"wifi_phy0iface0_bssid",	""				, 0 },
	{"wifi_phy0iface0_network",	"br0"				, 0 },
	{"wifi_phy0iface0_hidden",	""				, 0 },
	{"wifi_phy0iface0_wmm",		"1"				, 0 },
	{"wifi_phy0iface0_uapsd",	"1"				, 0 },
	{"wifi_phy0iface0_encryption",	"psk2"				, 0 },
	{"wifi_phy0iface0_cipher",	"auto"				, 0 },
	{"wifi_phy0iface0_key",		"glmt6000"			, 0 },
	{"wifi_phy0iface0_isolate",	""				, 0 },
	{"wifi_phy0iface0_br_isolate",	""				, 0 },
	{"wifi_phy0iface0_ifname",	""				, 0 },
	{"wifi_phy0iface0_macfilter",	""				, 0 },
	{"wifi_phy0iface0_maclist",	""				, 0 },

	{"wifi_phy0iface1_enable",	"0"				, 0 },
	{"wifi_phy0iface1_mode",	"ap"				, 0 },
	{"wifi_phy0iface1_essid",	""				, 0 },
	{"wifi_phy0iface1_bssid",	""				, 0 },
	{"wifi_phy0iface1_network",	"br0"				, 0 },
	{"wifi_phy0iface1_hidden",	""				, 0 },
	{"wifi_phy0iface1_wmm",		"1"				, 0 },
	{"wifi_phy0iface1_uapsd",	"1"				, 0 },
	{"wifi_phy0iface1_encryption",	"none"				, 0 },
	{"wifi_phy0iface1_cipher",	"auto"				, 0 },
	{"wifi_phy0iface1_key",		""				, 0 },
	{"wifi_phy0iface1_isolate",	""				, 0 },
	{"wifi_phy0iface1_br_isolate",	""				, 0 },
	{"wifi_phy0iface1_ifname",	""				, 0 },
	{"wifi_phy0iface1_macfilter",	""				, 0 },
	{"wifi_phy0iface1_maclist",	""				, 0 },

	{"wifi_phy0iface2_enable",	"0"				, 0 },
	{"wifi_phy0iface2_mode",	"ap"				, 0 },
	{"wifi_phy0iface2_essid",	""				, 0 },
	{"wifi_phy0iface2_bssid",	""				, 0 },
	{"wifi_phy0iface2_network",	"br0"				, 0 },
	{"wifi_phy0iface2_hidden",	""				, 0 },
	{"wifi_phy0iface2_wmm",		"1"				, 0 },
	{"wifi_phy0iface2_uapsd",	"1"				, 0 },
	{"wifi_phy0iface2_encryption",	"none"				, 0 },
	{"wifi_phy0iface2_cipher",	"auto"				, 0 },
	{"wifi_phy0iface2_key",		""				, 0 },
	{"wifi_phy0iface2_isolate",	""				, 0 },
	{"wifi_phy0iface2_br_isolate",	""				, 0 },
	{"wifi_phy0iface2_ifname",	""				, 0 },
	{"wifi_phy0iface2_macfilter",	""				, 0 },
	{"wifi_phy0iface2_maclist",	""				, 0 },

	{"wifi_phy0iface3_enable",	"0"				, 0 },
	{"wifi_phy0iface3_mode",	"ap"				, 0 },
	{"wifi_phy0iface3_essid",	""				, 0 },
	{"wifi_phy0iface3_bssid",	""				, 0 },
	{"wifi_phy0iface3_network",	"br0"				, 0 },
	{"wifi_phy0iface3_hidden",	""				, 0 },
	{"wifi_phy0iface3_wmm",		"1"				, 0 },
	{"wifi_phy0iface3_uapsd",	"1"				, 0 },
	{"wifi_phy0iface3_encryption",	"none"				, 0 },
	{"wifi_phy0iface3_cipher",	"auto"				, 0 },
	{"wifi_phy0iface3_key",		""				, 0 },
	{"wifi_phy0iface3_isolate",	""				, 0 },
	{"wifi_phy0iface3_br_isolate",	""				, 0 },
	{"wifi_phy0iface3_ifname",	""				, 0 },
	{"wifi_phy0iface3_macfilter",	""				, 0 },
	{"wifi_phy0iface3_maclist",	""				, 0 },

	{"wifi_phy0iface4_enable",	"0"				, 0 },
	{"wifi_phy0iface4_mode",	"ap"				, 0 },
	{"wifi_phy0iface4_essid",	""				, 0 },
	{"wifi_phy0iface4_bssid",	""				, 0 },
	{"wifi_phy0iface4_network",	"br0"				, 0 },
	{"wifi_phy0iface4_hidden",	""				, 0 },
	{"wifi_phy0iface4_wmm",		"1"				, 0 },
	{"wifi_phy0iface4_uapsd",	"1"				, 0 },
	{"wifi_phy0iface4_encryption",	"none"				, 0 },
	{"wifi_phy0iface4_cipher",	"auto"				, 0 },
	{"wifi_phy0iface4_key",		""				, 0 },
	{"wifi_phy0iface4_isolate",	""				, 0 },
	{"wifi_phy0iface4_br_isolate",	""				, 0 },
	{"wifi_phy0iface4_ifname",	""				, 0 },
	{"wifi_phy0iface4_macfilter",	""				, 0 },
	{"wifi_phy0iface4_maclist",	""				, 0 },

	{"wifi_phy0iface5_enable",	"0"				, 0 },
	{"wifi_phy0iface5_mode",	"ap"				, 0 },
	{"wifi_phy0iface5_essid",	""				, 0 },
	{"wifi_phy0iface5_bssid",	""				, 0 },
	{"wifi_phy0iface5_network",	"br0"				, 0 },
	{"wifi_phy0iface5_hidden",	""				, 0 },
	{"wifi_phy0iface5_wmm",		"1"				, 0 },
	{"wifi_phy0iface5_uapsd",	"1"				, 0 },
	{"wifi_phy0iface5_encryption",	"none"				, 0 },
	{"wifi_phy0iface5_cipher",	"auto"				, 0 },
	{"wifi_phy0iface5_key",		""				, 0 },
	{"wifi_phy0iface5_isolate",	""				, 0 },
	{"wifi_phy0iface5_br_isolate",	""				, 0 },
	{"wifi_phy0iface5_ifname",	""				, 0 },
	{"wifi_phy0iface5_macfilter",	""				, 0 },
	{"wifi_phy0iface5_maclist",	""				, 0 },

	{"wifi_phy0iface6_enable",	"0"				, 0 },
	{"wifi_phy0iface6_mode",	"ap"				, 0 },
	{"wifi_phy0iface6_essid",	""				, 0 },
	{"wifi_phy0iface6_bssid",	""				, 0 },
	{"wifi_phy0iface6_network",	"br0"				, 0 },
	{"wifi_phy0iface6_hidden",	""				, 0 },
	{"wifi_phy0iface6_wmm",		"1"				, 0 },
	{"wifi_phy0iface6_uapsd",	"1"				, 0 },
	{"wifi_phy0iface6_encryption",	"none"				, 0 },
	{"wifi_phy0iface6_cipher",	"auto"				, 0 },
	{"wifi_phy0iface6_key",		""				, 0 },
	{"wifi_phy0iface6_isolate",	""				, 0 },
	{"wifi_phy0iface6_br_isolate",	""				, 0 },
	{"wifi_phy0iface6_ifname",	""				, 0 },
	{"wifi_phy0iface6_macfilter",	""				, 0 },
	{"wifi_phy0iface6_maclist",	""				, 0 },

	{"wifi_phy0iface7_enable",	"0"				, 0 },
	{"wifi_phy0iface7_mode",	"ap"				, 0 },
	{"wifi_phy0iface7_essid",	""				, 0 },
	{"wifi_phy0iface7_bssid",	""				, 0 },
	{"wifi_phy0iface7_network",	"br0"				, 0 },
	{"wifi_phy0iface7_hidden",	""				, 0 },
	{"wifi_phy0iface7_wmm",		"1"				, 0 },
	{"wifi_phy0iface7_uapsd",	"1"				, 0 },
	{"wifi_phy0iface7_encryption",	"none"				, 0 },
	{"wifi_phy0iface7_cipher",	"auto"				, 0 },
	{"wifi_phy0iface7_key",		""				, 0 },
	{"wifi_phy0iface7_isolate",	""				, 0 },
	{"wifi_phy0iface7_br_isolate",	""				, 0 },
	{"wifi_phy0iface7_ifname",	""				, 0 },
	{"wifi_phy0iface7_macfilter",	""				, 0 },
	{"wifi_phy0iface7_maclist",	""				, 0 },

	{"wifi_phy0iface8_enable",	"0"				, 0 },
	{"wifi_phy0iface8_mode",	"ap"				, 0 },
	{"wifi_phy0iface8_essid",	""				, 0 },
	{"wifi_phy0iface8_bssid",	""				, 0 },
	{"wifi_phy0iface8_network",	"br0"				, 0 },
	{"wifi_phy0iface8_hidden",	""				, 0 },
	{"wifi_phy0iface8_wmm",		"1"				, 0 },
	{"wifi_phy0iface8_uapsd",	"1"				, 0 },
	{"wifi_phy0iface8_encryption",	"none"				, 0 },
	{"wifi_phy0iface8_cipher",	"auto"				, 0 },
	{"wifi_phy0iface8_key",		""				, 0 },
	{"wifi_phy0iface8_isolate",	""				, 0 },
	{"wifi_phy0iface8_br_isolate",	""				, 0 },
	{"wifi_phy0iface8_ifname",	""				, 0 },
	{"wifi_phy0iface8_macfilter",	""				, 0 },
	{"wifi_phy0iface8_maclist",	""				, 0 },

	{"wifi_phy0iface9_enable",	"0"				, 0 },
	{"wifi_phy0iface9_mode",	"ap"				, 0 },
	{"wifi_phy0iface9_essid",	""				, 0 },
	{"wifi_phy0iface9_bssid",	""				, 0 },
	{"wifi_phy0iface9_network",	"br0"				, 0 },
	{"wifi_phy0iface9_hidden",	""				, 0 },
	{"wifi_phy0iface9_wmm",		"1"				, 0 },
	{"wifi_phy0iface9_uapsd",	"1"				, 0 },
	{"wifi_phy0iface9_encryption",	"none"				, 0 },
	{"wifi_phy0iface9_cipher",	"auto"				, 0 },
	{"wifi_phy0iface9_key",		""				, 0 },
	{"wifi_phy0iface9_isolate",	""				, 0 },
	{"wifi_phy0iface9_br_isolate",	""				, 0 },
	{"wifi_phy0iface9_ifname",	""				, 0 },
	{"wifi_phy0iface9_macfilter",	""				, 0 },
	{"wifi_phy0iface9_maclist",	""				, 0 },

	{"wifi_phy0iface10_enable",	"0"				, 0 },
	{"wifi_phy0iface10_mode",	"ap"				, 0 },
	{"wifi_phy0iface10_essid",	""				, 0 },
	{"wifi_phy0iface10_bssid",	""				, 0 },
	{"wifi_phy0iface10_network",	"br0"				, 0 },
	{"wifi_phy0iface10_hidden",	""				, 0 },
	{"wifi_phy0iface10_wmm",	"1"				, 0 },
	{"wifi_phy0iface10_uapsd",	"1"				, 0 },
	{"wifi_phy0iface10_encryption",	"none"				, 0 },
	{"wifi_phy0iface10_cipher",	"auto"				, 0 },
	{"wifi_phy0iface10_key",	""				, 0 },
	{"wifi_phy0iface10_isolate",	""				, 0 },
	{"wifi_phy0iface10_br_isolate",	""				, 0 },
	{"wifi_phy0iface10_ifname",	""				, 0 },
	{"wifi_phy0iface10_macfilter",	""				, 0 },
	{"wifi_phy0iface10_maclist",	""				, 0 },

	{"wifi_phy0iface11_enable",	"0"				, 0 },
	{"wifi_phy0iface11_mode",	"ap"				, 0 },
	{"wifi_phy0iface11_essid",	""				, 0 },
	{"wifi_phy0iface11_bssid",	""				, 0 },
	{"wifi_phy0iface11_network",	"br0"				, 0 },
	{"wifi_phy0iface11_hidden",	""				, 0 },
	{"wifi_phy0iface11_wmm",	"1"				, 0 },
	{"wifi_phy0iface11_uapsd",	"1"				, 0 },
	{"wifi_phy0iface11_encryption",	"none"				, 0 },
	{"wifi_phy0iface11_cipher",	"auto"				, 0 },
	{"wifi_phy0iface11_key",	""				, 0 },
	{"wifi_phy0iface11_isolate",	""				, 0 },
	{"wifi_phy0iface11_br_isolate",	""				, 0 },
	{"wifi_phy0iface11_ifname",	""				, 0 },
	{"wifi_phy0iface11_macfilter",	""				, 0 },
	{"wifi_phy0iface11_maclist",	""				, 0 },

	{"wifi_phy0iface12_enable",	"0"				, 0 },
	{"wifi_phy0iface12_mode",	"ap"				, 0 },
	{"wifi_phy0iface12_essid",	""				, 0 },
	{"wifi_phy0iface12_bssid",	""				, 0 },
	{"wifi_phy0iface12_network",	"br0"				, 0 },
	{"wifi_phy0iface12_hidden",	""				, 0 },
	{"wifi_phy0iface12_wmm",	"1"				, 0 },
	{"wifi_phy0iface12_uapsd",	"1"				, 0 },
	{"wifi_phy0iface12_encryption",	"none"				, 0 },
	{"wifi_phy0iface12_cipher",	"auto"				, 0 },
	{"wifi_phy0iface12_key",	""				, 0 },
	{"wifi_phy0iface12_isolate",	""				, 0 },
	{"wifi_phy0iface12_br_isolate",	""				, 0 },
	{"wifi_phy0iface12_ifname",	""				, 0 },
	{"wifi_phy0iface12_macfilter",	""				, 0 },
	{"wifi_phy0iface12_maclist",	""				, 0 },

	{"wifi_phy0iface13_enable",	"0"				, 0 },
	{"wifi_phy0iface13_mode",	"ap"				, 0 },
	{"wifi_phy0iface13_essid",	""				, 0 },
	{"wifi_phy0iface13_bssid",	""				, 0 },
	{"wifi_phy0iface13_network",	"br0"				, 0 },
	{"wifi_phy0iface13_hidden",	""				, 0 },
	{"wifi_phy0iface13_wmm",	"1"				, 0 },
	{"wifi_phy0iface13_uapsd",	"1"				, 0 },
	{"wifi_phy0iface13_encryption",	"none"				, 0 },
	{"wifi_phy0iface13_cipher",	"auto"				, 0 },
	{"wifi_phy0iface13_key",	""				, 0 },
	{"wifi_phy0iface13_isolate",	""				, 0 },
	{"wifi_phy0iface13_br_isolate",	""				, 0 },
	{"wifi_phy0iface13_ifname",	""				, 0 },
	{"wifi_phy0iface13_macfilter",	""				, 0 },
	{"wifi_phy0iface13_maclist",	""				, 0 },

	{"wifi_phy0iface14_enable",	"0"				, 0 },
	{"wifi_phy0iface14_mode",	"ap"				, 0 },
	{"wifi_phy0iface14_essid",	""				, 0 },
	{"wifi_phy0iface14_bssid",	""				, 0 },
	{"wifi_phy0iface14_network",	"br0"				, 0 },
	{"wifi_phy0iface14_hidden",	""				, 0 },
	{"wifi_phy0iface14_wmm",	"1"				, 0 },
	{"wifi_phy0iface14_uapsd",	"1"				, 0 },
	{"wifi_phy0iface14_encryption",	"none"				, 0 },
	{"wifi_phy0iface14_cipher",	"auto"				, 0 },
	{"wifi_phy0iface14_key",	""				, 0 },
	{"wifi_phy0iface14_isolate",	""				, 0 },
	{"wifi_phy0iface14_br_isolate",	""				, 0 },
	{"wifi_phy0iface14_ifname",	""				, 0 },
	{"wifi_phy0iface14_macfilter",	""				, 0 },
	{"wifi_phy0iface14_maclist",	""				, 0 },

	{"wifi_phy0iface15_enable",	"0"				, 0 },
	{"wifi_phy0iface15_mode",	"ap"				, 0 },
	{"wifi_phy0iface15_essid",	""				, 0 },
	{"wifi_phy0iface15_bssid",	""				, 0 },
	{"wifi_phy0iface15_network",	"br0"				, 0 },
	{"wifi_phy0iface15_hidden",	""				, 0 },
	{"wifi_phy0iface15_wmm",	"1"				, 0 },
	{"wifi_phy0iface15_uapsd",	"1"				, 0 },
	{"wifi_phy0iface15_encryption",	"none"				, 0 },
	{"wifi_phy0iface15_cipher",	"auto"				, 0 },
	{"wifi_phy0iface15_key",	""				, 0 },
	{"wifi_phy0iface15_isolate",	""				, 0 },
	{"wifi_phy0iface15_br_isolate",	""				, 0 },
	{"wifi_phy0iface15_ifname",	""				, 0 },
	{"wifi_phy0iface15_macfilter",	""				, 0 },
	{"wifi_phy0iface15_maclist",	""				, 0 },

	{"wifi_phy1iface0_enable",	"0"				, 0 },
	{"wifi_phy1iface0_mode",	"ap"				, 0 },
	{"wifi_phy1iface0_essid",	""				, 0 },
	{"wifi_phy1iface0_bssid",	""				, 0 },
	{"wifi_phy1iface0_network",	"br0"				, 0 },
	{"wifi_phy1iface0_hidden",	""				, 0 },
	{"wifi_phy1iface0_wmm",		"1"				, 0 },
	{"wifi_phy1iface0_uapsd",	"1"				, 0 },
	{"wifi_phy1iface0_encryption",	"none"				, 0 },
	{"wifi_phy1iface0_cipher",	"auto"				, 0 },
	{"wifi_phy1iface0_key",		""				, 0 },
	{"wifi_phy1iface0_isolate",	""				, 0 },
	{"wifi_phy1iface0_br_isolate",	""				, 0 },
	{"wifi_phy1iface0_ifname",	""				, 0 },
	{"wifi_phy1iface0_macfilter",	""				, 0 },
	{"wifi_phy1iface0_maclist",	""				, 0 },

	{"wifi_phy1iface1_enable",	"0"				, 0 },
	{"wifi_phy1iface1_mode",	"ap"				, 0 },
	{"wifi_phy1iface1_essid",	""				, 0 },
	{"wifi_phy1iface1_bssid",	""				, 0 },
	{"wifi_phy1iface1_network",	"br0"				, 0 },
	{"wifi_phy1iface1_hidden",	""				, 0 },
	{"wifi_phy1iface1_wmm",		"1"				, 0 },
	{"wifi_phy1iface1_uapsd",	"1"				, 0 },
	{"wifi_phy1iface1_encryption",	"none"				, 0 },
	{"wifi_phy1iface1_cipher",	"auto"				, 0 },
	{"wifi_phy1iface1_key",		""				, 0 },
	{"wifi_phy1iface1_isolate",	""				, 0 },
	{"wifi_phy1iface1_br_isolate",	""				, 0 },
	{"wifi_phy1iface1_ifname",	""				, 0 },
	{"wifi_phy1iface1_macfilter",	""				, 0 },
	{"wifi_phy1iface1_maclist",	""				, 0 },

	{"wifi_phy1iface2_enable",	"0"				, 0 },
	{"wifi_phy1iface2_mode",	"ap"				, 0 },
	{"wifi_phy1iface2_essid",	""				, 0 },
	{"wifi_phy1iface2_bssid",	""				, 0 },
	{"wifi_phy1iface2_network",	"br0"				, 0 },
	{"wifi_phy1iface2_hidden",	""				, 0 },
	{"wifi_phy1iface2_wmm",		"1"				, 0 },
	{"wifi_phy1iface2_uapsd",	"1"				, 0 },
	{"wifi_phy1iface2_encryption",	"none"				, 0 },
	{"wifi_phy1iface2_cipher",	"auto"				, 0 },
	{"wifi_phy1iface2_key",		""				, 0 },
	{"wifi_phy1iface2_isolate",	""				, 0 },
	{"wifi_phy1iface2_br_isolate",	""				, 0 },
	{"wifi_phy1iface2_ifname",	""				, 0 },
	{"wifi_phy1iface2_macfilter",	""				, 0 },
	{"wifi_phy1iface2_maclist",	""				, 0 },

	{"wifi_phy1iface3_enable",	"0"				, 0 },
	{"wifi_phy1iface3_mode",	"ap"				, 0 },
	{"wifi_phy1iface3_essid",	""				, 0 },
	{"wifi_phy1iface3_bssid",	""				, 0 },
	{"wifi_phy1iface3_network",	"br0"				, 0 },
	{"wifi_phy1iface3_hidden",	""				, 0 },
	{"wifi_phy1iface3_wmm",		"1"				, 0 },
	{"wifi_phy1iface3_uapsd",	"1"				, 0 },
	{"wifi_phy1iface3_encryption",	"none"				, 0 },
	{"wifi_phy1iface3_cipher",	"auto"				, 0 },
	{"wifi_phy1iface3_key",		""				, 0 },
	{"wifi_phy1iface3_isolate",	""				, 0 },
	{"wifi_phy1iface3_br_isolate",	""				, 0 },
	{"wifi_phy1iface3_ifname",	""				, 0 },
	{"wifi_phy1iface3_macfilter",	""				, 0 },
	{"wifi_phy1iface3_maclist",	""				, 0 },

	{"wifi_phy1iface4_enable",	"0"				, 0 },
	{"wifi_phy1iface4_mode",	"ap"				, 0 },
	{"wifi_phy1iface4_essid",	""				, 0 },
	{"wifi_phy1iface4_bssid",	""				, 0 },
	{"wifi_phy1iface4_network",	"br0"				, 0 },
	{"wifi_phy1iface4_hidden",	""				, 0 },
	{"wifi_phy1iface4_wmm",		"1"				, 0 },
	{"wifi_phy1iface4_uapsd",	"1"				, 0 },
	{"wifi_phy1iface4_encryption",	"none"				, 0 },
	{"wifi_phy1iface4_cipher",	"auto"				, 0 },
	{"wifi_phy1iface4_key",		""				, 0 },
	{"wifi_phy1iface4_isolate",	""				, 0 },
	{"wifi_phy1iface4_br_isolate",	""				, 0 },
	{"wifi_phy1iface4_ifname",	""				, 0 },
	{"wifi_phy1iface4_macfilter",	""				, 0 },
	{"wifi_phy1iface4_maclist",	""				, 0 },

	{"wifi_phy1iface5_enable",	"0"				, 0 },
	{"wifi_phy1iface5_mode",	"ap"				, 0 },
	{"wifi_phy1iface5_essid",	""				, 0 },
	{"wifi_phy1iface5_bssid",	""				, 0 },
	{"wifi_phy1iface5_network",	"br0"				, 0 },
	{"wifi_phy1iface5_hidden",	""				, 0 },
	{"wifi_phy1iface5_wmm",		"1"				, 0 },
	{"wifi_phy1iface5_uapsd",	"1"				, 0 },
	{"wifi_phy1iface5_encryption",	"none"				, 0 },
	{"wifi_phy1iface5_cipher",	"auto"				, 0 },
	{"wifi_phy1iface5_key",		""				, 0 },
	{"wifi_phy1iface5_isolate",	""				, 0 },
	{"wifi_phy1iface5_br_isolate",	""				, 0 },
	{"wifi_phy1iface5_ifname",	""				, 0 },
	{"wifi_phy1iface5_macfilter",	""				, 0 },
	{"wifi_phy1iface5_maclist",	""				, 0 },

	{"wifi_phy1iface6_enable",	"0"				, 0 },
	{"wifi_phy1iface6_mode",	"ap"				, 0 },
	{"wifi_phy1iface6_essid",	""				, 0 },
	{"wifi_phy1iface6_bssid",	""				, 0 },
	{"wifi_phy1iface6_network",	"br0"				, 0 },
	{"wifi_phy1iface6_hidden",	""				, 0 },
	{"wifi_phy1iface6_wmm",		"1"				, 0 },
	{"wifi_phy1iface6_uapsd",	"1"				, 0 },
	{"wifi_phy1iface6_encryption",	"none"				, 0 },
	{"wifi_phy1iface6_cipher",	"auto"				, 0 },
	{"wifi_phy1iface6_key",		""				, 0 },
	{"wifi_phy1iface6_isolate",	""				, 0 },
	{"wifi_phy1iface6_br_isolate",	""				, 0 },
	{"wifi_phy1iface6_ifname",	""				, 0 },
	{"wifi_phy1iface6_macfilter",	""				, 0 },
	{"wifi_phy1iface6_maclist",	""				, 0 },

	{"wifi_phy1iface7_enable",	"0"				, 0 },
	{"wifi_phy1iface7_mode",	"ap"				, 0 },
	{"wifi_phy1iface7_essid",	""				, 0 },
	{"wifi_phy1iface7_bssid",	""				, 0 },
	{"wifi_phy1iface7_network",	"br0"				, 0 },
	{"wifi_phy1iface7_hidden",	""				, 0 },
	{"wifi_phy1iface7_wmm",		"1"				, 0 },
	{"wifi_phy1iface7_uapsd",	"1"				, 0 },
	{"wifi_phy1iface7_encryption",	"none"				, 0 },
	{"wifi_phy1iface7_cipher",	"auto"				, 0 },
	{"wifi_phy1iface7_key",		""				, 0 },
	{"wifi_phy1iface7_isolate",	""				, 0 },
	{"wifi_phy1iface7_br_isolate",	""				, 0 },
	{"wifi_phy1iface7_ifname",	""				, 0 },
	{"wifi_phy1iface7_macfilter",	""				, 0 },
	{"wifi_phy1iface7_maclist",	""				, 0 },

	{"wifi_phy1iface8_enable",	"0"				, 0 },
	{"wifi_phy1iface8_mode",	"ap"				, 0 },
	{"wifi_phy1iface8_essid",	""				, 0 },
	{"wifi_phy1iface8_bssid",	""				, 0 },
	{"wifi_phy1iface8_network",	"br0"				, 0 },
	{"wifi_phy1iface8_hidden",	""				, 0 },
	{"wifi_phy1iface8_wmm",		"1"				, 0 },
	{"wifi_phy1iface8_uapsd",	"1"				, 0 },
	{"wifi_phy1iface8_encryption",	"none"				, 0 },
	{"wifi_phy1iface8_cipher",	"auto"				, 0 },
	{"wifi_phy1iface8_key",		""				, 0 },
	{"wifi_phy1iface8_isolate",	""				, 0 },
	{"wifi_phy1iface8_br_isolate",	""				, 0 },
	{"wifi_phy1iface8_ifname",	""				, 0 },
	{"wifi_phy1iface8_macfilter",	""				, 0 },
	{"wifi_phy1iface8_maclist",	""				, 0 },

	{"wifi_phy1iface9_enable",	"0"				, 0 },
	{"wifi_phy1iface9_mode",	"ap"				, 0 },
	{"wifi_phy1iface9_essid",	""				, 0 },
	{"wifi_phy1iface9_bssid",	""				, 0 },
	{"wifi_phy1iface9_network",	"br0"				, 0 },
	{"wifi_phy1iface9_hidden",	""				, 0 },
	{"wifi_phy1iface9_wmm",		"1"				, 0 },
	{"wifi_phy1iface9_uapsd",	"1"				, 0 },
	{"wifi_phy1iface9_encryption",	"none"				, 0 },
	{"wifi_phy1iface9_cipher",	"auto"				, 0 },
	{"wifi_phy1iface9_key",		""				, 0 },
	{"wifi_phy1iface9_isolate",	""				, 0 },
	{"wifi_phy1iface9_br_isolate",	""				, 0 },
	{"wifi_phy1iface9_ifname",	""				, 0 },
	{"wifi_phy1iface9_macfilter",	""				, 0 },
	{"wifi_phy1iface9_maclist",	""				, 0 },

	{"wifi_phy1iface10_enable",	"0"				, 0 },
	{"wifi_phy1iface10_mode",	"ap"				, 0 },
	{"wifi_phy1iface10_essid",	""				, 0 },
	{"wifi_phy1iface10_bssid",	""				, 0 },
	{"wifi_phy1iface10_network",	"br0"				, 0 },
	{"wifi_phy1iface10_hidden",	""				, 0 },
	{"wifi_phy1iface10_wmm",	"1"				, 0 },
	{"wifi_phy1iface10_uapsd",	"1"				, 0 },
	{"wifi_phy1iface10_encryption",	"none"				, 0 },
	{"wifi_phy1iface10_cipher",	"auto"				, 0 },
	{"wifi_phy1iface10_key",	""				, 0 },
	{"wifi_phy1iface10_isolate",	""				, 0 },
	{"wifi_phy1iface10_br_isolate",	""				, 0 },
	{"wifi_phy1iface10_ifname",	""				, 0 },
	{"wifi_phy1iface10_macfilter",	""				, 0 },
	{"wifi_phy1iface10_maclist",	""				, 0 },

	{"wifi_phy1iface11_enable",	"0"				, 0 },
	{"wifi_phy1iface11_mode",	"ap"				, 0 },
	{"wifi_phy1iface11_essid",	""				, 0 },
	{"wifi_phy1iface11_bssid",	""				, 0 },
	{"wifi_phy1iface11_network",	"br0"				, 0 },
	{"wifi_phy1iface11_hidden",	""				, 0 },
	{"wifi_phy1iface11_wmm",	"1"				, 0 },
	{"wifi_phy1iface11_uapsd",	"1"				, 0 },
	{"wifi_phy1iface11_encryption",	"none"				, 0 },
	{"wifi_phy1iface11_cipher",	"auto"				, 0 },
	{"wifi_phy1iface11_key",	""				, 0 },
	{"wifi_phy1iface11_isolate",	""				, 0 },
	{"wifi_phy1iface11_br_isolate",	""				, 0 },
	{"wifi_phy1iface11_ifname",	""				, 0 },
	{"wifi_phy1iface11_macfilter",	""				, 0 },
	{"wifi_phy1iface11_maclist",	""				, 0 },

	{"wifi_phy1iface12_enable",	"0"				, 0 },
	{"wifi_phy1iface12_mode",	"ap"				, 0 },
	{"wifi_phy1iface12_essid",	""				, 0 },
	{"wifi_phy1iface12_bssid",	""				, 0 },
	{"wifi_phy1iface12_network",	"br0"				, 0 },
	{"wifi_phy1iface12_hidden",	""				, 0 },
	{"wifi_phy1iface12_wmm",	"1"				, 0 },
	{"wifi_phy1iface12_uapsd",	"1"				, 0 },
	{"wifi_phy1iface12_encryption",	"none"				, 0 },
	{"wifi_phy1iface12_cipher",	"auto"				, 0 },
	{"wifi_phy1iface12_key",	""				, 0 },
	{"wifi_phy1iface12_isolate",	""				, 0 },
	{"wifi_phy1iface12_br_isolate",	""				, 0 },
	{"wifi_phy1iface12_ifname",	""				, 0 },
	{"wifi_phy1iface12_macfilter",	""				, 0 },
	{"wifi_phy1iface12_maclist",	""				, 0 },

	{"wifi_phy1iface13_enable",	"0"				, 0 },
	{"wifi_phy1iface13_mode",	"ap"				, 0 },
	{"wifi_phy1iface13_essid",	""				, 0 },
	{"wifi_phy1iface13_bssid",	""				, 0 },
	{"wifi_phy1iface13_network",	"br0"				, 0 },
	{"wifi_phy1iface13_hidden",	""				, 0 },
	{"wifi_phy1iface13_wmm",	"1"				, 0 },
	{"wifi_phy1iface13_uapsd",	"1"				, 0 },
	{"wifi_phy1iface13_encryption",	"none"				, 0 },
	{"wifi_phy1iface13_cipher",	"auto"				, 0 },
	{"wifi_phy1iface13_key",	""				, 0 },
	{"wifi_phy1iface13_isolate",	""				, 0 },
	{"wifi_phy1iface13_br_isolate",	""				, 0 },
	{"wifi_phy1iface13_ifname",	""				, 0 },
	{"wifi_phy1iface13_macfilter",	""				, 0 },
	{"wifi_phy1iface13_maclist",	""				, 0 },

	{"wifi_phy1iface14_enable",	"0"				, 0 },
	{"wifi_phy1iface14_mode",	"ap"				, 0 },
	{"wifi_phy1iface14_essid",	""				, 0 },
	{"wifi_phy1iface14_bssid",	""				, 0 },
	{"wifi_phy1iface14_network",	"br0"				, 0 },
	{"wifi_phy1iface14_hidden",	""				, 0 },
	{"wifi_phy1iface14_wmm",	"1"				, 0 },
	{"wifi_phy1iface14_uapsd",	"1"				, 0 },
	{"wifi_phy1iface14_encryption",	"none"				, 0 },
	{"wifi_phy1iface14_cipher",	"auto"				, 0 },
	{"wifi_phy1iface14_key",	""				, 0 },
	{"wifi_phy1iface14_isolate",	""				, 0 },
	{"wifi_phy1iface14_br_isolate",	""				, 0 },
	{"wifi_phy1iface14_ifname",	""				, 0 },
	{"wifi_phy1iface14_macfilter",	""				, 0 },
	{"wifi_phy1iface14_maclist",	""				, 0 },

	{"wifi_phy1iface15_enable",	"0"				, 0 },
	{"wifi_phy1iface15_mode",	"ap"				, 0 },
	{"wifi_phy1iface15_essid",	""				, 0 },
	{"wifi_phy1iface15_bssid",	""				, 0 },
	{"wifi_phy1iface15_network",	"br0"				, 0 },
	{"wifi_phy1iface15_hidden",	""				, 0 },
	{"wifi_phy1iface15_wmm",	"1"				, 0 },
	{"wifi_phy1iface15_uapsd",	"1"				, 0 },
	{"wifi_phy1iface15_encryption",	"none"				, 0 },
	{"wifi_phy1iface15_cipher",	"auto"				, 0 },
	{"wifi_phy1iface15_key",	""				, 0 },
	{"wifi_phy1iface15_isolate",	""				, 0 },
	{"wifi_phy1iface15_br_isolate",	""				, 0 },
	{"wifi_phy1iface15_ifname",	""				, 0 },
	{"wifi_phy1iface15_macfilter",	""				, 0 },
	{"wifi_phy1iface15_maclist",	""				, 0 },

	{"wifi_phy2iface0_enable",	"0"				, 0 },
	{"wifi_phy2iface0_mode",	"ap"				, 0 },
	{"wifi_phy2iface0_essid",	""				, 0 },
	{"wifi_phy2iface0_bssid",	""				, 0 },
	{"wifi_phy2iface0_network",	"br0"				, 0 },
	{"wifi_phy2iface0_hidden",	""				, 0 },
	{"wifi_phy2iface0_wmm",		"1"				, 0 },
	{"wifi_phy2iface0_uapsd",	"1"				, 0 },
	{"wifi_phy2iface0_encryption",	"none"				, 0 },
	{"wifi_phy2iface0_cipher",	"auto"				, 0 },
	{"wifi_phy2iface0_key",		""				, 0 },
	{"wifi_phy2iface0_isolate",	""				, 0 },
	{"wifi_phy2iface0_br_isolate",	""				, 0 },
	{"wifi_phy2iface0_ifname",	""				, 0 },
	{"wifi_phy2iface0_macfilter",	""				, 0 },
	{"wifi_phy2iface0_maclist",	""				, 0 },

	{"wifi_phy2iface1_enable",	"0"				, 0 },
	{"wifi_phy2iface1_mode",	"ap"				, 0 },
	{"wifi_phy2iface1_essid",	""				, 0 },
	{"wifi_phy2iface1_bssid",	""				, 0 },
	{"wifi_phy2iface1_network",	"br0"				, 0 },
	{"wifi_phy2iface1_hidden",	""				, 0 },
	{"wifi_phy2iface1_wmm",		"1"				, 0 },
	{"wifi_phy2iface1_uapsd",	"1"				, 0 },
	{"wifi_phy2iface1_encryption",	"none"				, 0 },
	{"wifi_phy2iface1_cipher",	"auto"				, 0 },
	{"wifi_phy2iface1_key",		""				, 0 },
	{"wifi_phy2iface1_isolate",	""				, 0 },
	{"wifi_phy2iface1_br_isolate",	""				, 0 },
	{"wifi_phy2iface1_ifname",	""				, 0 },
	{"wifi_phy2iface1_macfilter",	""				, 0 },
	{"wifi_phy2iface1_maclist",	""				, 0 },

	{"wifi_phy2iface2_enable",	"0"				, 0 },
	{"wifi_phy2iface2_mode",	"ap"				, 0 },
	{"wifi_phy2iface2_essid",	""				, 0 },
	{"wifi_phy2iface2_bssid",	""				, 0 },
	{"wifi_phy2iface2_network",	"br0"				, 0 },
	{"wifi_phy2iface2_hidden",	""				, 0 },
	{"wifi_phy2iface2_wmm",		"1"				, 0 },
	{"wifi_phy2iface2_uapsd",	"1"				, 0 },
	{"wifi_phy2iface2_encryption",	"none"				, 0 },
	{"wifi_phy2iface2_cipher",	"auto"				, 0 },
	{"wifi_phy2iface2_key",		""				, 0 },
	{"wifi_phy2iface2_isolate",	""				, 0 },
	{"wifi_phy2iface2_br_isolate",	""				, 0 },
	{"wifi_phy2iface2_ifname",	""				, 0 },
	{"wifi_phy2iface2_macfilter",	""				, 0 },
	{"wifi_phy2iface2_maclist",	""				, 0 },

	{"wifi_phy2iface3_enable",	"0"				, 0 },
	{"wifi_phy2iface3_mode",	"ap"				, 0 },
	{"wifi_phy2iface3_essid",	""				, 0 },
	{"wifi_phy2iface3_bssid",	""				, 0 },
	{"wifi_phy2iface3_network",	"br0"				, 0 },
	{"wifi_phy2iface3_hidden",	""				, 0 },
	{"wifi_phy2iface3_wmm",		"1"				, 0 },
	{"wifi_phy2iface3_uapsd",	"1"				, 0 },
	{"wifi_phy2iface3_encryption",	"none"				, 0 },
	{"wifi_phy2iface3_cipher",	"auto"				, 0 },
	{"wifi_phy2iface3_key",		""				, 0 },
	{"wifi_phy2iface3_isolate",	""				, 0 },
	{"wifi_phy2iface3_br_isolate",	""				, 0 },
	{"wifi_phy2iface3_ifname",	""				, 0 },
	{"wifi_phy2iface3_macfilter",	""				, 0 },
	{"wifi_phy2iface3_maclist",	""				, 0 },

	{"wifi_phy2iface4_enable",	"0"				, 0 },
	{"wifi_phy2iface4_mode",	"ap"				, 0 },
	{"wifi_phy2iface4_essid",	""				, 0 },
	{"wifi_phy2iface4_bssid",	""				, 0 },
	{"wifi_phy2iface4_network",	"br0"				, 0 },
	{"wifi_phy2iface4_hidden",	""				, 0 },
	{"wifi_phy2iface4_wmm",		"1"				, 0 },
	{"wifi_phy2iface4_uapsd",	"1"				, 0 },
	{"wifi_phy2iface4_encryption",	"none"				, 0 },
	{"wifi_phy2iface4_cipher",	"auto"				, 0 },
	{"wifi_phy2iface4_key",		""				, 0 },
	{"wifi_phy2iface4_isolate",	""				, 0 },
	{"wifi_phy2iface4_br_isolate",	""				, 0 },
	{"wifi_phy2iface4_ifname",	""				, 0 },
	{"wifi_phy2iface4_macfilter",	""				, 0 },
	{"wifi_phy2iface4_maclist",	""				, 0 },

	{"wifi_phy2iface5_enable",	"0"				, 0 },
	{"wifi_phy2iface5_mode",	"ap"				, 0 },
	{"wifi_phy2iface5_essid",	""				, 0 },
	{"wifi_phy2iface5_bssid",	""				, 0 },
	{"wifi_phy2iface5_network",	"br0"				, 0 },
	{"wifi_phy2iface5_hidden",	""				, 0 },
	{"wifi_phy2iface5_wmm",		"1"				, 0 },
	{"wifi_phy2iface5_uapsd",	"1"				, 0 },
	{"wifi_phy2iface5_encryption",	"none"				, 0 },
	{"wifi_phy2iface5_cipher",	"auto"				, 0 },
	{"wifi_phy2iface5_key",		""				, 0 },
	{"wifi_phy2iface5_isolate",	""				, 0 },
	{"wifi_phy2iface5_br_isolate",	""				, 0 },
	{"wifi_phy2iface5_ifname",	""				, 0 },
	{"wifi_phy2iface5_macfilter",	""				, 0 },
	{"wifi_phy2iface5_maclist",	""				, 0 },

	{"wifi_phy2iface6_enable",	"0"				, 0 },
	{"wifi_phy2iface6_mode",	"ap"				, 0 },
	{"wifi_phy2iface6_essid",	""				, 0 },
	{"wifi_phy2iface6_bssid",	""				, 0 },
	{"wifi_phy2iface6_network",	"br0"				, 0 },
	{"wifi_phy2iface6_hidden",	""				, 0 },
	{"wifi_phy2iface6_wmm",		"1"				, 0 },
	{"wifi_phy2iface6_uapsd",	"1"				, 0 },
	{"wifi_phy2iface6_encryption",	"none"				, 0 },
	{"wifi_phy2iface6_cipher",	"auto"				, 0 },
	{"wifi_phy2iface6_key",		""				, 0 },
	{"wifi_phy2iface6_isolate",	""				, 0 },
	{"wifi_phy2iface6_br_isolate",	""				, 0 },
	{"wifi_phy2iface6_ifname",	""				, 0 },
	{"wifi_phy2iface6_macfilter",	""				, 0 },
	{"wifi_phy2iface6_maclist",	""				, 0 },

	{"wifi_phy2iface7_enable",	"0"				, 0 },
	{"wifi_phy2iface7_mode",	"ap"				, 0 },
	{"wifi_phy2iface7_essid",	""				, 0 },
	{"wifi_phy2iface7_bssid",	""				, 0 },
	{"wifi_phy2iface7_network",	"br0"				, 0 },
	{"wifi_phy2iface7_hidden",	""				, 0 },
	{"wifi_phy2iface7_wmm",		"1"				, 0 },
	{"wifi_phy2iface7_uapsd",	"1"				, 0 },
	{"wifi_phy2iface7_encryption",	"none"				, 0 },
	{"wifi_phy2iface7_cipher",	"auto"				, 0 },
	{"wifi_phy2iface7_key",		""				, 0 },
	{"wifi_phy2iface7_isolate",	""				, 0 },
	{"wifi_phy2iface7_br_isolate",	""				, 0 },
	{"wifi_phy2iface7_ifname",	""				, 0 },
	{"wifi_phy2iface7_macfilter",	""				, 0 },
	{"wifi_phy2iface7_maclist",	""				, 0 },

	{"wifi_phy2iface8_enable",	"0"				, 0 },
	{"wifi_phy2iface8_mode",	"ap"				, 0 },
	{"wifi_phy2iface8_essid",	""				, 0 },
	{"wifi_phy2iface8_bssid",	""				, 0 },
	{"wifi_phy2iface8_network",	"br0"				, 0 },
	{"wifi_phy2iface8_hidden",	""				, 0 },
	{"wifi_phy2iface8_wmm",		"1"				, 0 },
	{"wifi_phy2iface8_uapsd",	"1"				, 0 },
	{"wifi_phy2iface8_encryption",	"none"				, 0 },
	{"wifi_phy2iface8_cipher",	"auto"				, 0 },
	{"wifi_phy2iface8_key",		""				, 0 },
	{"wifi_phy2iface8_isolate",	""				, 0 },
	{"wifi_phy2iface8_br_isolate",	""				, 0 },
	{"wifi_phy2iface8_ifname",	""				, 0 },
	{"wifi_phy2iface8_macfilter",	""				, 0 },
	{"wifi_phy2iface8_maclist",	""				, 0 },

	{"wifi_phy2iface9_enable",	"0"				, 0 },
	{"wifi_phy2iface9_mode",	"ap"				, 0 },
	{"wifi_phy2iface9_essid",	""				, 0 },
	{"wifi_phy2iface9_bssid",	""				, 0 },
	{"wifi_phy2iface9_network",	"br0"				, 0 },
	{"wifi_phy2iface9_hidden",	""				, 0 },
	{"wifi_phy2iface9_wmm",		"1"				, 0 },
	{"wifi_phy2iface9_uapsd",	"1"				, 0 },
	{"wifi_phy2iface9_encryption",	"none"				, 0 },
	{"wifi_phy2iface9_cipher",	"auto"				, 0 },
	{"wifi_phy2iface9_key",		""				, 0 },
	{"wifi_phy2iface9_isolate",	""				, 0 },
	{"wifi_phy2iface9_br_isolate",	""				, 0 },
	{"wifi_phy2iface9_ifname",	""				, 0 },
	{"wifi_phy2iface9_macfilter",	""				, 0 },
	{"wifi_phy2iface9_maclist",	""				, 0 },

	{"wifi_phy2iface10_enable",	"0"				, 0 },
	{"wifi_phy2iface10_mode",	"ap"				, 0 },
	{"wifi_phy2iface10_essid",	""				, 0 },
	{"wifi_phy2iface10_bssid",	""				, 0 },
	{"wifi_phy2iface10_network",	"br0"				, 0 },
	{"wifi_phy2iface10_hidden",	""				, 0 },
	{"wifi_phy2iface10_wmm",	"1"				, 0 },
	{"wifi_phy2iface10_uapsd",	"1"				, 0 },
	{"wifi_phy2iface10_encryption",	"none"				, 0 },
	{"wifi_phy2iface10_cipher",	"auto"				, 0 },
	{"wifi_phy2iface10_key",	""				, 0 },
	{"wifi_phy2iface10_isolate",	""				, 0 },
	{"wifi_phy2iface10_br_isolate",	""				, 0 },
	{"wifi_phy2iface10_ifname",	""				, 0 },
	{"wifi_phy2iface10_macfilter",	""				, 0 },
	{"wifi_phy2iface10_maclist",	""				, 0 },

	{"wifi_phy2iface11_enable",	"0"				, 0 },
	{"wifi_phy2iface11_mode",	"ap"				, 0 },
	{"wifi_phy2iface11_essid",	""				, 0 },
	{"wifi_phy2iface11_bssid",	""				, 0 },
	{"wifi_phy2iface11_network",	"br0"				, 0 },
	{"wifi_phy2iface11_hidden",	""				, 0 },
	{"wifi_phy2iface11_wmm",	"1"				, 0 },
	{"wifi_phy2iface11_uapsd",	"1"				, 0 },
	{"wifi_phy2iface11_encryption",	"none"				, 0 },
	{"wifi_phy2iface11_cipher",	"auto"				, 0 },
	{"wifi_phy2iface11_key",	""				, 0 },
	{"wifi_phy2iface11_isolate",	""				, 0 },
	{"wifi_phy2iface11_br_isolate",	""				, 0 },
	{"wifi_phy2iface11_ifname",	""				, 0 },
	{"wifi_phy2iface11_macfilter",	""				, 0 },
	{"wifi_phy2iface11_maclist",	""				, 0 },

	{"wifi_phy2iface12_enable",	"0"				, 0 },
	{"wifi_phy2iface12_mode",	"ap"				, 0 },
	{"wifi_phy2iface12_essid",	""				, 0 },
	{"wifi_phy2iface12_bssid",	""				, 0 },
	{"wifi_phy2iface12_network",	"br0"				, 0 },
	{"wifi_phy2iface12_hidden",	""				, 0 },
	{"wifi_phy2iface12_wmm",	"1"				, 0 },
	{"wifi_phy2iface12_uapsd",	"1"				, 0 },
	{"wifi_phy2iface12_encryption",	"none"				, 0 },
	{"wifi_phy2iface12_cipher",	"auto"				, 0 },
	{"wifi_phy2iface12_key",	""				, 0 },
	{"wifi_phy2iface12_isolate",	""				, 0 },
	{"wifi_phy2iface12_br_isolate",	""				, 0 },
	{"wifi_phy2iface12_ifname",	""				, 0 },
	{"wifi_phy2iface12_macfilter",	""				, 0 },
	{"wifi_phy2iface12_maclist",	""				, 0 },

	{"wifi_phy2iface13_enable",	"0"				, 0 },
	{"wifi_phy2iface13_mode",	"ap"				, 0 },
	{"wifi_phy2iface13_essid",	""				, 0 },
	{"wifi_phy2iface13_bssid",	""				, 0 },
	{"wifi_phy2iface13_network",	"br0"				, 0 },
	{"wifi_phy2iface13_hidden",	""				, 0 },
	{"wifi_phy2iface13_wmm",	"1"				, 0 },
	{"wifi_phy2iface13_uapsd",	"1"				, 0 },
	{"wifi_phy2iface13_encryption",	"none"				, 0 },
	{"wifi_phy2iface13_cipher",	"auto"				, 0 },
	{"wifi_phy2iface13_key",	""				, 0 },
	{"wifi_phy2iface13_isolate",	""				, 0 },
	{"wifi_phy2iface13_br_isolate",	""				, 0 },
	{"wifi_phy2iface13_ifname",	""				, 0 },
	{"wifi_phy2iface13_macfilter",	""				, 0 },
	{"wifi_phy2iface13_maclist",	""				, 0 },

	{"wifi_phy2iface14_enable",	"0"				, 0 },
	{"wifi_phy2iface14_mode",	"ap"				, 0 },
	{"wifi_phy2iface14_essid",	""				, 0 },
	{"wifi_phy2iface14_bssid",	""				, 0 },
	{"wifi_phy2iface14_network",	"br0"				, 0 },
	{"wifi_phy2iface14_hidden",	""				, 0 },
	{"wifi_phy2iface14_wmm",	"1"				, 0 },
	{"wifi_phy2iface14_uapsd",	"1"				, 0 },
	{"wifi_phy2iface14_encryption",	"none"				, 0 },
	{"wifi_phy2iface14_cipher",	"auto"				, 0 },
	{"wifi_phy2iface14_key",	""				, 0 },
	{"wifi_phy2iface14_isolate",	""				, 0 },
	{"wifi_phy2iface14_br_isolate",	""				, 0 },
	{"wifi_phy2iface14_ifname",	""				, 0 },
	{"wifi_phy2iface14_macfilter",	""				, 0 },
	{"wifi_phy2iface14_maclist",	""				, 0 },

	{"wifi_phy2iface15_enable",	"0"				, 0 },
	{"wifi_phy2iface15_mode",	"ap"				, 0 },
	{"wifi_phy2iface15_essid",	""				, 0 },
	{"wifi_phy2iface15_bssid",	""				, 0 },
	{"wifi_phy2iface15_network",	"br0"				, 0 },
	{"wifi_phy2iface15_hidden",	""				, 0 },
	{"wifi_phy2iface15_wmm",	"1"				, 0 },
	{"wifi_phy2iface15_uapsd",	"1"				, 0 },
	{"wifi_phy2iface15_encryption",	"none"				, 0 },
	{"wifi_phy2iface15_cipher",	"auto"				, 0 },
	{"wifi_phy2iface15_key",	""				, 0 },
	{"wifi_phy2iface15_isolate",	""				, 0 },
	{"wifi_phy2iface15_br_isolate",	""				, 0 },
	{"wifi_phy2iface15_ifname",	""				, 0 },
	{"wifi_phy2iface15_macfilter",	""				, 0 },
	{"wifi_phy2iface15_maclist",	""				, 0 },
#endif /* TOMATO64_WIFI */

	{ "ctf_disable",		"1"				, 0 },
	{ "flow_offloading",		"0"				, 0 },
	{ "wed_offloading",		"0"				, 0 },
	{ "packet_steering",		"1"				, 0 },
	{ "steering_flows",		"0"				, 0 },
	{ "steering_flows_custom",	"0"				, 0 },
	{ "tty_login",			"1"				, 0 },

	{ "port0_label",		""				, 0 },
	{ "port1_label",		""				, 0 },
	{ "port2_label",		""				, 0 },
	{ "port3_label",		""				, 0 },
	{ "port4_label",		""				, 0 },
	{ "port5_label",		""				, 0 },
	{ "port6_label",		""				, 0 },
	{ "port7_label",		""				, 0 },
	{ "port8_label",		""				, 0 },
#endif /* TOMATO64 */
	{ 0, 0, 0 }
};

#ifdef TCONFIG_BCMARM
/* Translates from, for example, wl0_ (or wl0.1_) to wl_ */
/* Only single digits are currently supported */
static void fix_name(const char *name, char *fixed_name)
{
	char *pSuffix = NULL;

	/* Translate prefix wlx_ and wlx.y_ to wl_ */
	/* Expected inputs are: wld_root, wld.d_root, wld.dd_root
	 * We accept: wld + '_' anywhere
	 */
	pSuffix = strchr(name, '_');

	if ((strncmp(name, "wl", 2) == 0) && isdigit(name[2]) && (pSuffix != NULL)) {
		strcpy(fixed_name, "wl");
		strcpy(&fixed_name[2], pSuffix);
		return;
	}

	/* No match with above rules: default to input name */
	strcpy(fixed_name, name);
}

/*
 * Find nvram param name; return pointer which should be treated as const
 * return NULL if not found.
 *
 * NOTE: This routine special-cases the variable wl_bss_enabled. It will
 * return the normal default value if asked for wl_ or wl0_. But it will
 * return 0 if asked for a virtual BSS reference like wl0.1_.
 */
char *nvram_default_get(const char *name)
{
	int idx;
	char fixed_name[NVRAM_MAX_VALUE_LEN];

	fix_name(name, fixed_name);
	if (strcmp(fixed_name, "wl_bss_enabled") == 0) {
		if (name[3] == '.' || name[4] == '.') { /* Virtual interface */
			return "0";
		}
	}

#ifndef TCONFIG_BCM7
#ifdef __CONFIG_HSPOT__
	if (strcmp(fixed_name, "wl_bss_hs2_enabled") == 0) {
		if (name[3] == '.' || name[4] == '.') { /* Virtual interface */
			return "0";
		}
	}
#endif /* __CONFIG_HSPOT__ */
#endif /* !TCONFIG_BCM7 */

	for (idx = 0; router_defaults[idx].name != NULL; idx++) {
		if (strcmp(router_defaults[idx].name, fixed_name) == 0) {
			return router_defaults[idx].value;
		}
	}

	return NULL;
}

/* validate/restore all per-interface related variables */
void nvram_validate_all(char *prefix, bool restore)
{
	struct nvram_tuple *t;
	char tmp[100];
	char *v;

	for (t = router_defaults; t->name; t++) {
		if (!strncmp(t->name, "wl_", 3)) {
			strlcat_r(prefix, &t->name[3], tmp, sizeof(tmp));
			if (!restore && nvram_get(tmp))
				continue;
			v = nvram_get(t->name);
			nvram_set(tmp, v ? v : t->value);
		}
	}
}

/* restore specific per-interface variable */
void nvram_restore_var(char *prefix, char *name)
{
	struct nvram_tuple *t;
	char tmp[100];

	for (t = router_defaults; t->name; t++) {
		if (!strncmp(t->name, "wl_", 3) && !strcmp(&t->name[3], name)) {
			nvram_set(strlcat_r(prefix, name, tmp, sizeof(tmp)), t->value);
			break;
		}
	}
}

#else /* TCONFIG_BCMARM */
/* MIPS */
const defaults_t if_generic[] = {
	{ "lan_ifname",			"br0"				},
	{ "lan_ifnames",		"eth0 eth2 eth3 eth4"		},
	{ "wan_ifname",			"eth1"				},
	{ "wan_ifnames",		"eth1"				},

	{ NULL, NULL }
};

#define BRIDGE_BLOCK_IF_VLAN(i) \
	{ "lan" #i "_ifname",		""				}, \
	{ "lan" #i "_ifnames",		""				},

const defaults_t if_vlan[] = {
	{ "wan_ifname",			"vlan1"				},
	{ "wan_ifnames",		"vlan1"				},
	{ "lan_ifname",			"br0"				},
	{ "lan_ifnames",		"vlan0 eth1 eth2 eth3"		},
#if BRIDGE_COUNT >= 2
 BRIDGE_BLOCK_IF_VLAN(1)
#endif
#if BRIDGE_COUNT >= 3
 BRIDGE_BLOCK_IF_VLAN(2)
#endif
#if BRIDGE_COUNT >= 4
 BRIDGE_BLOCK_IF_VLAN(3)
#endif
#if BRIDGE_COUNT >= 5
 BRIDGE_BLOCK_IF_VLAN(4)
#endif
#if BRIDGE_COUNT >= 6
 BRIDGE_BLOCK_IF_VLAN(5)
#endif
#if BRIDGE_COUNT >= 7
 BRIDGE_BLOCK_IF_VLAN(6)
#endif
#if BRIDGE_COUNT >= 8
 BRIDGE_BLOCK_IF_VLAN(7)
#endif
#if BRIDGE_COUNT >= 9
 BRIDGE_BLOCK_IF_VLAN(8)
#endif
#if BRIDGE_COUNT >= 10
 BRIDGE_BLOCK_IF_VLAN(9)
#endif
#if BRIDGE_COUNT >= 11
 BRIDGE_BLOCK_IF_VLAN(10)
#endif
#if BRIDGE_COUNT >= 12
 BRIDGE_BLOCK_IF_VLAN(11)
#endif
#if BRIDGE_COUNT >= 13
 BRIDGE_BLOCK_IF_VLAN(12)
#endif
#if BRIDGE_COUNT >= 14
 BRIDGE_BLOCK_IF_VLAN(13)
#endif
#if BRIDGE_COUNT >= 15
 BRIDGE_BLOCK_IF_VLAN(14)
#endif
#if BRIDGE_COUNT >= 16
 BRIDGE_BLOCK_IF_VLAN(15)
#endif
	{ NULL, NULL }
};
#endif /* TCONFIG_BCMARM */
