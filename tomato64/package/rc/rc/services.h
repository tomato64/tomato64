/*
 * services.h
 *
 * Service command metadata.
 *
 * This header is private to services.c and is included after the local
 * service start/stop helpers have been defined.
 *
 * Copyright (C) 2026 FreshTomato
 * https://freshtomato.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */


#ifndef __RC_SERVICE_H__
#define __RC_SERVICE_H__


#define SVCF_LIST		0x01	/* show in "service list" */
#define SVCF_PREFIX		0x02	/* match name as a prefix */
#define SVCF_NO_STATUS		0x04	/* no single daemon process */
#define SVCF_NUM_SUF		0x08	/* prefix must be followed by a number */
#define SVCF_MWAN_SUF		0x10	/* numeric suffix is limited to MWAN_MAX */
#define SVCF_OVPNC_SUF		0x20	/* numeric suffix is limited to OVPN_CLIENT_COUNT */
#define SVCF_OVPNS_SUF		0x40	/* numeric suffix is limited to OVPN_SERVER_COUNT */
#define SVCF_WG_SUF		0x80	/* numeric suffix is limited to WG_INTERFACE_COUNT */

#define SVCOP_SPECIAL		0x80
#define SVCOP_RSTATS		(SVCOP_SPECIAL | 0x01)
#define SVCOP_CSTATS		(SVCOP_SPECIAL | 0x02)
#define SVCOP_ADBLOCK		(SVCOP_SPECIAL | 0x03)
#define SVCOP_UPNP		(SVCOP_SPECIAL | 0x04)
#ifdef TCONFIG_BT
#define SVCOP_BITTORRENT	(SVCOP_SPECIAL | 0x05)
#endif
#ifdef TCONFIG_FTP
#define SVCOP_FTPD		(SVCOP_SPECIAL | 0x06)
#endif
#ifdef TCONFIG_MEDIA_SERVER
#define SVCOP_MEDIA		(SVCOP_SPECIAL | 0x07)
#endif
#ifdef TCONFIG_TINC
#define SVCOP_TINC		(SVCOP_SPECIAL | 0x08)
#endif
#ifdef TCONFIG_NGINX
#define SVCOP_NGINX		(SVCOP_SPECIAL | 0x09)
#define SVCOP_MYSQL		(SVCOP_SPECIAL | 0x0a)
#endif
#ifdef TCONFIG_PPTPD
#define SVCOP_PPTPD		(SVCOP_SPECIAL | 0x0b)
#endif
#ifdef TCONFIG_TOR
#define SVCOP_TOR		(SVCOP_SPECIAL | 0x0c)
#endif
#ifdef TCONFIG_OPENVPN
#define SVCOP_VPNCLIENT		(SVCOP_SPECIAL | 0x0d)
#define SVCOP_VPNSERVER		(SVCOP_SPECIAL | 0x0e)
#endif
#ifdef TCONFIG_WIREGUARD
#define SVCOP_WIREGUARD		(SVCOP_SPECIAL | 0x0f)
#endif
#define SVCOP_DHCPC_WAN		(SVCOP_SPECIAL | 0x10)
#define SVCOP_WAN_IF		(SVCOP_SPECIAL | 0x11)
#define SVCOP_DNSMASQ		(SVCOP_SPECIAL | 0x12)
#define SVCOP_LOGGING		(SVCOP_SPECIAL | 0x13)
#define SVCOP_CTNF		(SVCOP_SPECIAL | 0x14)
#define SVCOP_WIRELESS		(SVCOP_SPECIAL | 0x15)
#define SVCOP_WLGUI		(SVCOP_SPECIAL | 0x16)
#define SVCOP_NAS		(SVCOP_SPECIAL | 0x17)
#ifdef TCONFIG_USB
#define SVCOP_USBAPPS		(SVCOP_SPECIAL | 0x18)
#endif
#ifdef TCONFIG_SAMBASRV
#define SVCOP_SAMBA		(SVCOP_SPECIAL | 0x19)
#endif
#define SVCOP_FIREWALL		(SVCOP_SPECIAL | 0x1a)
#define SVCOP_RESTRICT		(SVCOP_SPECIAL | 0x1b)
#define SVCOP_BWLIMIT		(SVCOP_SPECIAL | 0x1c)
#define SVCOP_QOS		(SVCOP_SPECIAL | 0x1d)
#define SVCOP_ROUTING		(SVCOP_SPECIAL | 0x1e)
#define SVCOP_ADMIN		(SVCOP_SPECIAL | 0x1f)
#define SVCOP_USB		(SVCOP_SPECIAL | 0x20)
#define SVCOP_WAN		(SVCOP_SPECIAL | 0x21)
#define SVCOP_NET		(SVCOP_SPECIAL | 0x22)
#define SVCOP_UPGRADE		(SVCOP_SPECIAL | 0x23)

#define SVC_OP(_stop, _start)	{ _stop, _start }

enum svc_op_id {
	SVCOP_NONE = 0,
	SVCOP_TELNETD,
	SVCOP_SSHD,
	SVCOP_HTTPD,
	SVCOP_DDNS,
	SVCOP_NTPD,
	SVCOP_CROND,
	SVCOP_HOTPLUG,
	SVCOP_TOMATOANON,
	SVCOP_ARPBIND,
	SVCOP_RSTATS_NVRAM,
	SVCOP_CSTATS_NVRAM,
#ifdef TCONFIG_FTP
	SVCOP_FTP_NVRAM,
#endif
#ifdef TCONFIG_SNMP
	SVCOP_SNMP_NVRAM,
#endif
	SVCOP_UPNP_NVRAM,
#ifdef TCONFIG_BCMBSD
	SVCOP_BSD_NVRAM,
#endif
#ifdef TCONFIG_DNSCRYPT
	SVCOP_DNSCRYPT,
#endif
#ifdef TCONFIG_STUBBY
	SVCOP_STUBBY,
#endif
#ifdef TCONFIG_MDNS
	SVCOP_MDNS,
#endif
#ifdef TCONFIG_IRQBALANCE
	SVCOP_IRQBALANCE,
#endif
#ifdef TCONFIG_HAVEGED
	SVCOP_HAVEGED,
#endif
#ifdef TCONFIG_IPV6
	SVCOP_DHCP6,
#endif
#ifdef TCONFIG_CIFS
	SVCOP_CIFS,
#endif
#ifdef TCONFIG_JFFS2
	SVCOP_JFFS2,
#endif
#ifdef TCONFIG_ZEBRA
	SVCOP_ZEBRA,
#endif
#ifdef TCONFIG_SDHC
	SVCOP_MMC,
#endif
#ifdef TCONFIG_BCMBSD
	SVCOP_BSD,
#endif
#ifdef TCONFIG_ROAM
	SVCOP_ROAMAST,
#endif
	SVCOP_SCHED,
#ifdef TCONFIG_NFS
	SVCOP_NFS,
#endif
#ifdef TCONFIG_SNMP
	SVCOP_SNMP,
#endif
#ifdef TCONFIG_UPS
	SVCOP_UPS,
#endif
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
	SVCOP_PORTHEALTH,
#endif
#endif /* TOMATO64 */
#ifdef TCONFIG_PPTPD
	SVCOP_PPTPCLIENT,
#endif
#ifdef TCONFIG_FANCTRL
	SVCOP_FANCTRL,
#endif
#ifdef TCONFIG_NOCAT
	SVCOP_SPLASHD,
#endif
#ifdef TOMATO64
	SVCOP_ZRAM,
	SVCOP_CPUFREQ,
#endif /* TOMATO64 */
	SVCOP_MAX
};

enum svc_proc_id {
	P_NONE = 0,
	P_SELF,
	P_AVAHI_DAEMON,
	P_APCUPSD,
	P_BSD,
	P_CROND,
	P_CSTATS,
	P_DHCP6C,
	P_DNSCRYPT_PROXY,
	P_DNSMASQ,
	P_DROPBEAR,
	P_HAVEGED,
	P_HOTPLUG2,
	P_HTTPD,
	P_IRQBALANCE,
	P_MINIDLNA,
	P_MINIUPNPD,
	P_MYSQLD,
	P_NAS,
	P_NFSD,
	P_NGINX,
	P_NTPD,
	P_PHY_TEMPSENSE,
	P_PPTPCLIENT,
	P_PPTPD,
	P_ROAMAST,
	P_RSTATS,
	P_SCHED,
	P_SMBD,
	P_SNMPD,
	P_SPLASHD,
	P_STUBBY,
	P_SYSLOGD,
	P_TELNETD,
	P_TINCD,
	P_TOR,
	P_TRANSMISSION_DA,
	P_UDHCPC,
	P_VSFTPD,
	P_ZEBRA,
	P_WIREGUARD,
	P_MAX
};

struct svc_entry {
	const char *name;
	unsigned char flags;
	unsigned char proc;
	unsigned char arg;
	unsigned char op;
};

typedef void (*svc_void_fn_t)(void);

struct svc_op {
	svc_void_fn_t stop;
	svc_void_fn_t start;
};

static const char * const svc_proc_name[] = {
	NULL,
	NULL,		/* P_SELF: use requested service name */
	"avahi-daemon",
	"apcupsd",
	"bsd",
	"crond",
	"cstats",
	"dhcp6c",
	"dnscrypt-proxy",
	"dnsmasq",
	"dropbear",
	"haveged",
	"hotplug2",
	"httpd",
	"irqbalance",
	"minidlna",
	"miniupnpd",
	"mysqld",
	"nas",
	"nfsd",
	"nginx",
	"ntpd",
	"phy_tempsense",
	"pptpclient",
	"pptpd",
	"roamast",
	"rstats",
	"sched",
	"smbd",
	"snmpd",
	"splashd",
	"stubby",
	"syslogd",
	"telnetd",
	"tincd",
	"tor",
	"transmission-da",
	"udhcpc",
	"vsftpd",
	"zebra",
	NULL		/* P_WIREGUARD: handled by wg_status("wgN") */
};

/*
 * Service command metadata.
 *
 * The first SVCF_LIST entry is the public service name shown by
 * "service list".  Following entries without SVCF_LIST are accepted
 * compatibility aliases for the same service, usually matching the
 * underlying daemon name.
 *
 * The proc field describes status handling, while op selects the
 * start/stop dispatcher path used by exec_service().
 */
static const struct svc_entry svc_table[] = {
	{ "adblock",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			1,	SVCOP_ADBLOCK		},
	{ "adminnosshd",	SVCF_NO_STATUS,							P_NONE,			1,	SVCOP_ADMIN		},
	{ "admin",		SVCF_LIST | SVCF_PREFIX | SVCF_NO_STATUS,			P_NONE,			0,	SVCOP_ADMIN		},
	{ "arpbind",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_ARPBIND		},
#ifdef TCONFIG_BT
	{ "bittorrent",		SVCF_LIST,							P_TRANSMISSION_DA,	1,	SVCOP_BITTORRENT	},
	{ "transmission",	0,								P_TRANSMISSION_DA,	1,	SVCOP_BITTORRENT	},
	{ "transmission_da",	0,								P_TRANSMISSION_DA,	1,	SVCOP_BITTORRENT	},
#endif
#ifdef TCONFIG_BCMBSD
	{ "bsd",		SVCF_LIST,							P_BSD,			0,	SVCOP_BSD		},
	{ "bsd_nvram",		SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_BSD_NVRAM		},
#endif
	{ "bwlimit",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_BWLIMIT		},
#ifdef TCONFIG_CIFS
	{ "cifs",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_CIFS		},
#endif
#ifdef TOMATO64
	{ "cpufreq",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_CPUFREQ		},
#endif /* TOMATO64 */
	{ "crond",		SVCF_LIST,							P_CROND,		0,	SVCOP_CROND		},
	{ "cstats",		SVCF_LIST,							P_CSTATS,		0,	SVCOP_CSTATS		},
	{ "cstatsnew",		0,								P_CSTATS,		1,	SVCOP_CSTATS		},
	{ "cstats_nvram",	SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_CSTATS_NVRAM	},
	{ "ctnf",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_CTNF		},
	{ "ddns",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_DDNS		},
#ifdef TCONFIG_IPV6
	{ "dhcp6",		SVCF_LIST,							P_DHCP6C,		0,	SVCOP_DHCP6		},
#endif
	{ "dhcpc_wan",		SVCF_LIST,							P_UDHCPC,		1,	SVCOP_DHCPC_WAN		},
	{ "dhcpc_wan",		SVCF_PREFIX | SVCF_NUM_SUF | SVCF_MWAN_SUF,			P_UDHCPC,		2,	SVCOP_DHCPC_WAN		},
	{ "dns",		SVCF_LIST,							P_DNSMASQ,		1,	SVCOP_DNSMASQ		},
#ifdef TCONFIG_DNSCRYPT
	{ "dnscrypt",		SVCF_LIST,							P_DNSCRYPT_PROXY,	0,	SVCOP_DNSCRYPT		},
	{ "dnscrypt_proxy",	0,								P_DNSCRYPT_PROXY,	0,	SVCOP_DNSCRYPT		},
#endif
	{ "dnsmasq",		SVCF_LIST,							P_DNSMASQ,		0,	SVCOP_DNSMASQ		},
#ifdef TCONFIG_FANCTRL
	{ "fanctrl",		SVCF_LIST,							P_PHY_TEMPSENSE,	0,	SVCOP_FANCTRL		},
#endif
	{ "firewall",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_FIREWALL		},
#ifdef TCONFIG_FTP
	{ "ftpd",		SVCF_LIST,							P_VSFTPD,		1,	SVCOP_FTPD		},
	{ "vsftpd",		0,								P_VSFTPD,		1,	SVCOP_FTPD		},
	{ "ftp_nvram",		SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_FTP_NVRAM		},
#endif
#ifdef TCONFIG_HAVEGED
	{ "haveged",		SVCF_LIST,							P_HAVEGED,		0,	SVCOP_HAVEGED		},
#endif
	{ "hotplug",		SVCF_LIST,							P_HOTPLUG2,		0,	SVCOP_HOTPLUG		},
	{ "httpd",		SVCF_LIST,							P_HTTPD,		0,	SVCOP_HTTPD		},
#ifdef TCONFIG_IRQBALANCE
	{ "irqbalance",		SVCF_LIST,							P_IRQBALANCE,		0,	SVCOP_IRQBALANCE	},
#endif
#ifdef TCONFIG_JFFS2
	{ "jffs",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_JFFS2		},
	{ "jffs2",		SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_JFFS2		},
#endif
	{ "logging",		SVCF_LIST,							P_SYSLOGD,		0,	SVCOP_LOGGING		},
#ifdef TCONFIG_MDNS
	{ "mdns",		SVCF_LIST,							P_AVAHI_DAEMON,		0,	SVCOP_MDNS		},
	{ "avahi_daemon",	0,								P_AVAHI_DAEMON,		0,	SVCOP_MDNS		},
#endif
#ifdef TCONFIG_MEDIA_SERVER
	{ "media",		SVCF_LIST,							P_MINIDLNA,		1,	SVCOP_MEDIA		},
	{ "minidlna",		0,								P_MINIDLNA,		1,	SVCOP_MEDIA		},
#endif
#ifdef TCONFIG_SDHC
	{ "mmc",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_MMC		},
#endif
#ifdef TCONFIG_NGINX
	{ "mysql",		SVCF_LIST,							P_MYSQLD,		1,	SVCOP_MYSQL		},
	{ "mysqld",		0,								P_MYSQLD,		1,	SVCOP_MYSQL		},
#endif
#ifndef TOMATO64
	{ "nas",		SVCF_LIST,							P_NAS,			0,	SVCOP_NAS		},
#endif /* TOMATO64 */
	{ "net",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_NET		},
#ifdef TCONFIG_NFS
	{ "nfs",		SVCF_LIST,							P_NFSD,			0,	SVCOP_NFS		},
	{ "nfsd",		0,								P_NFSD,			0,	SVCOP_NFS		},
#endif
#ifdef TCONFIG_NGINX
	{ "nginx",		SVCF_LIST,							P_NGINX,		1,	SVCOP_NGINX		},
#endif
	{ "ntpd",		SVCF_LIST,							P_NTPD,			0,	SVCOP_NTPD		},
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
	{ "porthealth",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_PORTHEALTH	},
#endif
#endif /* TOMATO64 */
#ifdef TCONFIG_PPTPD
	{ "pptpclient",		SVCF_LIST,							P_PPTPCLIENT,		0,	SVCOP_PPTPCLIENT	},
	{ "pptpd",		SVCF_LIST,							P_PPTPD,		1,	SVCOP_PPTPD		},
#endif
	{ "qos",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_QOS		},
	{ "restrict",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_RESTRICT		},
#ifdef TCONFIG_ROAM
	{ "roamast",		SVCF_LIST,							P_ROAMAST,		0,	SVCOP_ROAMAST		},
	{ "rssi",		0,								P_ROAMAST,		0,	SVCOP_ROAMAST		},
#endif
	{ "routing",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_ROUTING		},
	{ "rstats",		SVCF_LIST,							P_RSTATS,		0,	SVCOP_RSTATS		},
	{ "rstatsnew",		0,								P_RSTATS,		1,	SVCOP_RSTATS		},
	{ "rstats_nvram",	SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_RSTATS_NVRAM	},
#ifdef TCONFIG_SAMBASRV
	{ "samba",		SVCF_LIST,							P_SMBD,			1,	SVCOP_SAMBA		},
	{ "smbd",		0,								P_SMBD,			1,	SVCOP_SAMBA		},
#endif
	{ "sched",		SVCF_LIST,							P_SCHED,		0,	SVCOP_SCHED		},
#ifdef TCONFIG_SNMP
	{ "snmp",		SVCF_LIST,							P_SNMPD,		0,	SVCOP_SNMP		},
	{ "snmp_nvram",		SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_SNMP_NVRAM	},
#endif
#ifdef TCONFIG_NOCAT
	{ "splashd",		SVCF_LIST,							P_SPLASHD,		0,	SVCOP_SPLASHD		},
#endif
	{ "sshd",		SVCF_LIST,							P_DROPBEAR,		0,	SVCOP_SSHD		},
	{ "dropbear",		0,								P_DROPBEAR,		0,	SVCOP_SSHD		},
#ifdef TCONFIG_STUBBY
	{ "stubby",		SVCF_LIST,							P_STUBBY,		0,	SVCOP_STUBBY		},
#endif
	{ "telnetd",		SVCF_LIST,							P_TELNETD,		0,	SVCOP_TELNETD		},
#ifdef TCONFIG_TINC
	{ "tinc",		SVCF_LIST,							P_TINCD,		1,	SVCOP_TINC		},
	{ "tincd",		0,								P_TINCD,		1,	SVCOP_TINC		},
#endif
	{ "tomatoanon",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_TOMATOANON	},
#ifdef TCONFIG_TOR
	{ "tor",		SVCF_LIST,							P_TOR,			1,	SVCOP_TOR		},
#endif
	{ "upgrade",		SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_UPGRADE		},
	{ "upnp",		SVCF_LIST,							P_MINIUPNPD,		0,	SVCOP_UPNP		},
	{ "miniupnpd",		0,								P_MINIUPNPD,		0,	SVCOP_UPNP		},
	{ "upnp_nvram",		SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_UPNP_NVRAM	},
#ifdef TCONFIG_UPS
	{ "ups",		SVCF_LIST,							P_APCUPSD,		0,	SVCOP_UPS		},
#endif
#ifdef TCONFIG_USB
	{ "usb",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_USB		},
	{ "usbapps",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_USBAPPS		},
#endif
#ifdef TCONFIG_OPENVPN
	{ "vpnclient",		SVCF_LIST | SVCF_PREFIX | SVCF_NUM_SUF | SVCF_OVPNC_SUF,	P_SELF,			1,	SVCOP_VPNCLIENT		},
	{ "vpnserver",		SVCF_LIST | SVCF_PREFIX | SVCF_NUM_SUF | SVCF_OVPNS_SUF,	P_SELF,			1,	SVCOP_VPNSERVER		},
#endif
	{ "wan",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_WAN		},
	{ "wan",		SVCF_PREFIX | SVCF_NUM_SUF | SVCF_MWAN_SUF | SVCF_NO_STATUS,	P_NONE,			1,	SVCOP_WAN_IF		},
#ifdef TOMATO64
	{ "wifi",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0 },
#endif /* TOMATO64 */
#ifdef TCONFIG_WIREGUARD
	{ "wireguard",	SVCF_LIST | SVCF_PREFIX | SVCF_NUM_SUF | SVCF_WG_SUF	,		P_WIREGUARD,		0,	SVCOP_WIREGUARD		},
#endif
#ifndef TOMATO64
	{ "wireless",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_WIRELESS		},
	{ "wl",			SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_WIRELESS		},
	{ "wlgui",		SVCF_NO_STATUS,							P_NONE,			0,	SVCOP_WLGUI		},
#endif /* TOMATO64 */
#ifdef TCONFIG_ZEBRA
	{ "zebra",		SVCF_LIST,							P_ZEBRA,		0,	SVCOP_ZEBRA		},
#endif
#ifdef TOMATO64
	{ "zram",		SVCF_LIST | SVCF_NO_STATUS,					P_NONE,			0,	SVCOP_ZRAM		},
#endif /* TOMATO64 */
	{ NULL,			0,								P_NONE,			0 }
};

static const struct svc_op svc_ops[] = {
	SVC_OP(NULL, NULL),
	SVC_OP(stop_telnetd, start_telnetd),
	SVC_OP(stop_sshd, start_sshd),
	SVC_OP(stop_httpd, start_httpd),
	SVC_OP(stop_ddns, start_ddns),
	SVC_OP(stop_ntpd, start_ntpd),
	SVC_OP(stop_cron, start_cron),
	SVC_OP(stop_hotplug2, start_hotplug2),
	SVC_OP(stop_tomatoanon, start_tomatoanon),
	SVC_OP(stop_arpbind, start_arpbind),
	SVC_OP(del_rstats_defaults, add_rstats_defaults),
	SVC_OP(del_cstats_defaults, add_cstats_defaults),
#ifdef TCONFIG_FTP
	SVC_OP(del_ftp_defaults, add_ftp_defaults),
#endif
#ifdef TCONFIG_SNMP
	SVC_OP(del_snmp_defaults, add_snmp_defaults),
#endif
	SVC_OP(del_upnp_defaults, add_upnp_defaults),
#ifdef TCONFIG_BCMBSD
	SVC_OP(del_bsd_defaults, add_bsd_defaults),
#endif
#ifdef TCONFIG_DNSCRYPT
	SVC_OP(stop_dnscrypt, start_dnscrypt),
#endif
#ifdef TCONFIG_STUBBY
	SVC_OP(stop_stubby, start_stubby),
#endif
#ifdef TCONFIG_MDNS
	SVC_OP(stop_mdns, start_mdns),
#endif
#ifdef TCONFIG_IRQBALANCE
	SVC_OP(stop_irqbalance, start_irqbalance),
#endif
#ifdef TCONFIG_HAVEGED
	SVC_OP(stop_haveged, start_haveged),
#endif
#ifdef TCONFIG_IPV6
	SVC_OP(stop_dhcp6c, start_dhcp6c),
#endif
#ifdef TCONFIG_CIFS
	SVC_OP(stop_cifs, start_cifs),
#endif
#ifdef TCONFIG_JFFS2
	SVC_OP(stop_jffs2, start_jffs2),
#endif
#ifdef TCONFIG_ZEBRA
	SVC_OP(stop_zebra, start_zebra),
#endif
#ifdef TCONFIG_SDHC
	SVC_OP(stop_mmc, start_mmc),
#endif
#ifdef TCONFIG_BCMBSD
	SVC_OP(stop_bsd, start_bsd),
#endif
#ifdef TCONFIG_ROAM
	SVC_OP(stop_roamast, start_roamast),
#endif
	SVC_OP(stop_sched, start_sched),
#ifdef TCONFIG_NFS
	SVC_OP(stop_nfs, start_nfs),
#endif
#ifdef TCONFIG_SNMP
	SVC_OP(stop_snmp, start_snmp),
#endif
#ifdef TCONFIG_UPS
	SVC_OP(stop_ups, start_ups),
#endif
#ifndef TOMATO64
#ifdef TCONFIG_BCMARM
	SVC_OP(stop_porthealth, start_porthealth),
#endif
#endif /* TOMATO64 */
#ifdef TCONFIG_PPTPD
	SVC_OP(stop_pptpc, start_pptpc),
#endif
#ifdef TCONFIG_FANCTRL
	SVC_OP(stop_phy_tempsense, start_phy_tempsense),
#endif
#ifdef TCONFIG_NOCAT
	SVC_OP(stop_nocat, start_nocat),
#endif
#ifdef TOMATO64
	SVC_OP(stop_zram, start_zram),
	SVC_OP(stop_cpufreq, start_cpufreq),
#endif /* TOMATO64 */
};

static const struct svc_entry *svc_find(const char *name);
static int svc_exec_simple(const struct svc_entry *svc, const char *service, int act_start, int act_stop, int user);

#endif /* __RC_SERVICE_H__ */
