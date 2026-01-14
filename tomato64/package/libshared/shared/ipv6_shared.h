/*
 *
 * FreshTomato Firmware
 *
 */

#ifndef _ipv6_shared_h_
#define _ipv6_shared_h_

#define TOMATO_DUID_MAX_LEN	64
#define TOMATO_DUID_GUI		"/var/dhcp6c_duid_gui"

#define IPV6_MIN_LIFETIME	120
#define ONEMONTH_LIFETIME	(30 * 24 * 60 * 60)
#define INFINITE_LIFETIME 	0xffffffff

#define MAX_DNS6_SERVER_LAN	2

#define BRIDGE_COUNT_IPV6_MAX	16 /* Max 32! */

#define IPV6_METRIC_GW_LOW	"8192" /* backup, last resort */
#define IPV6_METRIC_GW_LOW_INT	 8192
#define IPV6_METRIC_GW_MED	"1024" /* ALWAYS default for RAs */
#define IPV6_METRIC_GW_MED_INT	 1024

#endif /* _ipv6_shared_h_ */
