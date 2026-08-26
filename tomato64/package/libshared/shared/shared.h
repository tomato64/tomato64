/*
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */

#ifndef __SHARED_H__
#define __SHARED_H__

#include <tomato_profile.h>
#include <tomato_config.h>

#ifdef TOMATO64
#include <typedefs.h>
#endif /* TOMATO64 */
#include <netinet/in.h>
#include <stdint.h>
#include <errno.h>
#include <syslog.h>
#include <net/if.h>
#include <string.h>

#ifdef TCONFIG_USB
#include <mntent.h>
#endif

#ifdef TCONFIG_IPV6
#include <ipv6_shared.h>
#endif

#define Y2K			946684800L /* seconds since 1970 */

#define ASIZE(array)		(sizeof(array) / sizeof(array[0]))

#define BRIDGE_COUNT		TCONFIG_BRIDGE_COUNT

#define WIFI_PHY_COUNT		3
#define WIFI_IFACE_COUNT	16

#define	MTD_DEV(arg)		"/dev/mtd"#arg
#define	MTD_BLKDEV(arg)		"/dev/mtdblock"#arg
#define	DEV_GPIO(arg)		"/dev/gpio"#arg

#ifdef TCONFIG_BCMARM
 #define OVPN_CLIENT_COUNT	3
#else
 #define OVPN_CLIENT_COUNT	2
#endif
#ifndef TOMATO64
#define OVPN_SERVER_COUNT	2
#else
#define OVPN_SERVER_COUNT	4
#endif /* TOMATO64 */
#define WG_INTERFACE_COUNT	3

#ifdef TCONFIG_BCMARM
#define DISABLE_SYSLOG_OSM	0
#define DISABLE_SYSLOG_OS	0
#else
 #ifdef TCONFIG_OPTIMIZE_SIZE_MORE
  #define DISABLE_SYSLOG_OSM	1
 #else
  #define DISABLE_SYSLOG_OSM	0
 #endif
 #ifdef TCONFIG_OPTIMIZE_SIZE
  #define DISABLE_SYSLOG_OS	1
 #else
  #define DISABLE_SYSLOG_OS	DISABLE_SYSLOG_OSM
 #endif
#endif

#ifdef DEBUG_LOGMSG
#define IF_DEBUG_LOGMSG(...) __VA_ARGS__
#define IF_NOT_DEBUG_LOGMSG(...)
#else
#define IF_DEBUG_LOGMSG(...)
#define IF_NOT_DEBUG_LOGMSG(...) __VA_ARGS__
#endif

#define logmsg(level, args...) \
	do { \
		IF_DEBUG_LOGMSG( \
			if ((LOGMSG_DISABLE == 0) && ((nvram_get_int(LOGMSG_NVDEBUG)) || (level < LOG_DEBUG))) \
				syslog(level, args); \
		) \
		IF_NOT_DEBUG_LOGMSG( \
			if ((LOGMSG_DISABLE == 0) && (level < LOG_DEBUG)) \
				syslog(level, args); \
		) \
	} while (0)

#define logerr(func, line, filename)	syslog(LOG_ERR, "ERROR in: %s line: %d - file \"%s\": %s", func, line, filename, strerror(errno))

#ifdef DEBUG_NOISY
#define _dprintf		cprintf
#else
#define _dprintf(args...)	do { } while(0)
#endif

#define BUF_SIZE		256 /* default */
#define BUF_SIZE_8		8
#define BUF_SIZE_16		16
#define BUF_SIZE_32		32
#define BUF_SIZE_64		64
#define BUF_SIZE_128		128

/* version.c */
extern const char *tomato_version;
extern const char *tomato_buildtime;
extern const char *tomato_shortver;
#ifdef TOMATO64
extern const char *tomato_nightly;
#endif /* TOMATO64 */

/* misc.c */
#define	WP_DISABLED		0 /* order must be synced with def in misc.c */
#define	WP_STATIC		1
#define	WP_DHCP			2
#define	WP_L2TP			3
#define	WP_PPPOE		4
#define	WP_PPTP			5
#define	WP_PPP3G		6
#define	WP_LTE			7

#ifdef TCONFIG_IPV6
#define	IPV6_DISABLED		0
#define	IPV6_NATIVE		1
#define	IPV6_NATIVE_DHCP	2
#define	IPV6_ANYCAST_6TO4	3
#define	IPV6_6IN4		4
#define	IPV6_MANUAL		5
#define	IPV6_6RD		6
#define	IPV6_6RD_DHCP		7
#endif

#define MWAN_MAX		TCONFIG_MWAN_MAX

#ifndef TOMATO64
#ifdef TCONFIG_EXTSW
#define MAX_PORT_ID		5
#else
#define MAX_PORT_ID		4
#endif
#else
#define MAX_PORT_ID		8
#endif /* TOMATO64 */

#if !defined(CONFIG_BCMWL6) && !defined(TCONFIG_BLINK) /* only mips RT branch */
#define TOMATO_VLANNUM		16
#endif

enum {
	ACT_IDLE,
	ACT_TFTP_UPGRADE_UNUSED,
	ACT_WEB_UPGRADE,
	ACT_WEBS_UPGRADE_UNUSED,
	ACT_SW_RESTORE,
	ACT_HW_RESTORE,
	ACT_ERASE_NVRAM,
	ACT_NVRAM_COMMIT,
	ACT_REBOOT,
	ACT_UNKNOWN
};

typedef struct {
	int count;
	struct {
		struct in_addr addr;
		unsigned short port;
	} dns[6];
} dns_list_t;

typedef struct {
	int count;
	struct {
		char name[IFNAMSIZ + 1];
		char ip[sizeof("xxx.xxx.xxx.xxx") + 1];
	} iface[2];
} wanface_list_t;

extern void chld_reap(int sig);
extern char *prefix_nvram_get(const char *prefix, const char *suffix, char *key, const size_t key_size);
extern void prefix_nvram_set(const char *prefix, const char *suffix, const char *value, char *key, const size_t key_size);
extern void prefix_nvram_unset(const char *prefix, const char *suffix, char *key, const size_t key_size);
extern int prefix_nvram_match(const char *prefix, const char *suffix, const char *value, char *key, const size_t key_size);
extern int prefix_nvram_get_int(const char *prefix, const char *suffix, char *key, const size_t key_size);
extern void get_bridge_suffix(unsigned int bridge, char *suffix, const size_t suffix_size);
extern void get_bridge_prefix(unsigned int bridge, char *prefix, const size_t prefix_size);
extern void get_bridge_nvram_key(unsigned int bridge, const char *suffix, char *key, const size_t key_size);
extern char *bridge_nvram_get(unsigned int bridge, const char *suffix, char *key, const size_t key_size);
extern void get_wan_prefix(int iWan_unit, char *sPrefix);
extern char *wan_nvram_get(int wan_unit, const char *suffix, char *key, const size_t key_size);
extern int get_wan_unit(const char *sPrefix);
extern int get_wan_unit_with_value(const char *suffix, const char *value);
extern int get_wan_proto(void);
extern int get_wanx_proto(char *prefix);
#ifdef TCONFIG_IPV6
extern int get_ipv6_service(void);
#define ipv6_enabled()	(get_ipv6_service() != IPV6_DISABLED)
extern const char *ipv6_router_address(struct in6_addr *in6addr);
extern int calc_6rd_local_prefix(const struct in6_addr *prefix, int prefix_len, int relay_prefix_len, const struct in_addr *local_ip, struct in6_addr *local_prefix, int *local_prefix_len);
#else
#define ipv6_enabled()		(0)
#endif
extern int using_dhcpc(char *prefix);
extern void notice_set(const char *path, const char *format, ...);
extern int wan_led(int mode);
extern int wan_led_off(char *prefix);
extern int check_wanup(char *prefix);
extern long check_wanup_time(char *prefix);
extern const dns_list_t *get_dns(char *prefix);
extern void set_action(int a);
extern int check_action(void);
extern int wait_action_idle(int n);
#ifdef TCONFIG_BCMWL6
extern int is_psta_client(int unit, int subunit);
#endif
extern int wl_client(int unit, int subunit);
extern const wanface_list_t *get_wanfaces(char *prefix);
extern const char *get_wanface(char *prefix);
extern const char *get_wanip(char *prefix);
#ifdef TCONFIG_IPV6
extern const char *get_wan6face(void);
extern const char *ipv6_address(const char *ipaddr6);
extern const char *ipv6_prefix(struct in6_addr *in6addr);
#endif
extern const char *getifaddr(char *ifname, int family, int linklocal);
extern int is_intf_up(const char* ifname);
extern long get_uptime(void);
extern char *wl_nvname(const char *nv, int unit, int subunit);
extern int get_radio(int unit);
extern void set_radio(int on, int unit);
extern int nvram_get_int(const char *key);
extern int nvram_get_file(const char *key, const char *fname, int max);
extern int nvram_set_file(const char *key, const char *fname, int max);
extern int nvram_contains_word(const char *key, const char *word);
extern int nvram_is_empty(const char *key);
extern void nvram_commit_x(void);
extern char *getNVRAMVar(const char *text, const int unit);
extern int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, int timeout);
extern int mtd_getinfo(const char *mtdname, int *part, int *size);
extern int foreach_wif(int include_vifs, void *param, int (*func)(int idx, int unit, int subunit, void *param));
extern void gen_urandom(char *buf1, unsigned char *buf2, size_t buf_sz, const unsigned int addtid);

/* usb.c */
#ifdef TCONFIG_USB
#define DEV_DISCS_ROOT		"/dev/discs"

/* Flags used in exec_for_host calls */
#define EFH_1ST_HOST		0x00000001 /* func is called for the 1st time for this host */
#define EFH_1ST_DISC		0x00000002 /* func is called for the 1st time for this disc */
#define EFH_HUNKNOWN		0x00000004 /* host is unknown */
#define EFH_USER		0x00000008 /* process is user-initiated - either via Web GUI or a script */
#define EFH_SHUTDN		0x00000010 /* exec_for_host is called at shutdown - system is stopping */
#define EFH_HP_ADD		0x00000020 /* exec_for_host is called from "add" hotplug event */
#define EFH_HP_REMOVE		0x00000040 /* exec_for_host is called from "remove" hotplug event */
#define EFH_PRINT		0x00000080 /* output partition list to the web response */

extern struct mntent *findmntents(char *file, int swp, int (*func)(struct mntent *mnt, uint flags), uint flags);
extern char *find_label_or_uuid(char *dev_name, char *label, size_t label_sz, char *uuid, size_t uuid_sz);
extern void add_remove_usbhost(char *host, int add);
typedef int (*host_exec)(char *dev_name, int host_num, char *dsc_name, char *pt_name, uint flags);
extern int exec_for_host(int host, int obsolete, uint flags, host_exec func);
extern int is_no_partition(const char *discname);
#endif /* TCONFIG_USB */

/* id.c */
enum {
	MODEL_UNKNOWN,
#ifdef TCONFIG_BCMARM
	MODEL_RTN18U,
	MODEL_RTAC56U,
	MODEL_RTAC66U_B1,
	MODEL_RTAC67U,
	MODEL_DSLAC68U,
	MODEL_RTAC68U,
	MODEL_RTAC68UV3,
	MODEL_RTAC1900P,
#ifdef TCONFIG_AC3200
	MODEL_RTAC3200,
#endif
#ifdef TCONFIG_BCM714
	MODEL_RTAC3100,
	MODEL_RTAC88U,
#ifdef TCONFIG_AC5300
	MODEL_RTAC5300,
#endif
#endif /* TCONFIG_BCM714 */
	MODEL_R6250,
	MODEL_R6300v2,
	MODEL_R6400,
	MODEL_R6400v2,
	MODEL_R6700v1,
	MODEL_R6700v3,
	MODEL_R6900,
	MODEL_R7000,
	MODEL_EX7000,
	MODEL_XR300,
#ifdef TCONFIG_AC3200
	MODEL_R8000,
#endif
	MODEL_DIR868L,
	MODEL_WS880,
	MODEL_EA6350v1,
	MODEL_EA6350v2,
	MODEL_EA6400,
	MODEL_EA6700,
	MODEL_EA6900,
	MODEL_WZR1750,
	MODEL_R1D,
	MODEL_AC15,
	MODEL_AC18,
	MODEL_F9K1113v2,
	MODEL_F9K1113v2_20X0,
	MODEL_AC1450,
	MODEL_R6200v2,
	MODEL_EX6200
#elif defined(TCONFIG_RTNPLUS)
	MODEL_WRT54G,
	MODEL_WRTSL54GS,
	MODEL_WHRG54S,
	MODEL_WHRHPG54,
	MODEL_WR850GV1,
	MODEL_WR850GV2,
	MODEL_WZRG54,
	MODEL_WL500W,
	MODEL_WL500GP,
	MODEL_WL500GPv2,
	MODEL_WL500GE,
	MODEL_WL500GD,
	MODEL_WL520GU,
	MODEL_DIR320,
	MODEL_L600N,
	MODEL_DIR620C1,
	MODEL_H618B,
	MODEL_CW5358U,
	MODEL_HG320,
	MODEL_RG200E_CA,
	MODEL_H218N,
	MODEL_TDN60,
	MODEL_TDN6,
	MODEL_WL1600GL,
	MODEL_WBRG54,
	MODEL_WBR2G54,
	MODEL_WX6615GT,
	MODEL_WZRHPG54,
	MODEL_WZRRSG54,
	MODEL_WZRRSG54HP,
	MODEL_WVRG54NF,
	MODEL_WHR2A54G54,
	MODEL_WHR3AG54,
	MODEL_RT390W,
	MODEL_RTN10,
	MODEL_RTN10U,
	MODEL_RTN10P,
	MODEL_RTN12A1,
	MODEL_RTN12B1,
	MODEL_RTN12C1,
	MODEL_RTN12D1,
	MODEL_RTN12VP,
	MODEL_RTN12K,
	MODEL_RTN12HP,
	MODEL_RTN15U,
	MODEL_RTN16,
	MODEL_RTN53,
	MODEL_RTN53A1,
	MODEL_RTN66U,
	MODEL_WNR3500L,
	MODEL_WNR3500LV2,
	MODEL_WNR2000v2,
	MODEL_F7D3301,
	MODEL_F7D3302,
	MODEL_F7D4301,
	MODEL_F7D4302,
	MODEL_F5D8235v3,
	MODEL_F9K1102,
	MODEL_WRT160Nv1,
	MODEL_WRT160Nv3,
	MODEL_WRT320N,
	MODEL_WRT610Nv2,
	MODEL_WRT310Nv1,
	MODEL_E900,
	MODEL_E1000v2,
	MODEL_E1500,
	MODEL_E1550,
	MODEL_E2500,
	MODEL_E3200,
	MODEL_E4200,
	MODEL_MN700,
	MODEL_WRH54G,
	MODEL_WHRG125,
	MODEL_WZRG108,
	MODEL_WTR54GS,
	MODEL_WR100,
	MODEL_WLA2G54L,
	MODEL_TM2300,
	MODEL_WZRG300N,
	MODEL_WRT300N,
	MODEL_WL330GE,
	MODEL_W1800R,
	MODEL_WNDR4000,
	MODEL_WNDR3700v3,
	MODEL_WNDR3400,
	MODEL_WNDR3400v2,
	MODEL_WNDR3400v3,
	MODEL_D1800H,
	MODEL_EA6500V1,
	MODEL_TDN80,
	MODEL_R6300V1,
	MODEL_WNDR4500,
	MODEL_WNDR4500V2,
	MODEL_DIR865L
#else
	MODEL_WRT54G,
	MODEL_WRTSL54GS,
	MODEL_WHRG54S,
	MODEL_WHRHPG54,
	MODEL_WR850GV1,
	MODEL_WR850GV2,
	MODEL_WZRG54,
	MODEL_WL500W,
	MODEL_WL500GP,
	MODEL_WL500GPv2,
	MODEL_WL500GE,
	MODEL_WL500GD,
	MODEL_WL520GU,
	MODEL_DIR320,
	MODEL_H618B,
	MODEL_WL1600GL,
	MODEL_WBRG54,
	MODEL_WBR2G54,
	MODEL_WX6615GT,
	MODEL_WZRHPG54,
	MODEL_WZRRSG54,
	MODEL_WZRRSG54HP,
	MODEL_WVRG54NF,
	MODEL_WHR2A54G54,
	MODEL_WHR3AG54,
	MODEL_RT390W,
	MODEL_RTN10,
	MODEL_RTN12,
	MODEL_RTN16,
	MODEL_WNR3500L,
	MODEL_WNR2000v2,
	MODEL_F7D3301,
	MODEL_F7D3302,
	MODEL_F7D4301,
	MODEL_F7D4302,
	MODEL_F5D8235v3,
	MODEL_WRT160Nv1,
	MODEL_WRT160Nv3,
	MODEL_WRT320N,
	MODEL_WRT610Nv2,
	MODEL_WRT310Nv1,
	MODEL_E4200,
	MODEL_MN700,
	MODEL_WRH54G,
	MODEL_WHRG125,
	MODEL_WZRG108,
	MODEL_WTR54GS,
	MODEL_WR100,
	MODEL_WLA2G54L,
	MODEL_TM2300,
	MODEL_WZRG300N,
	MODEL_WRT300N,
	MODEL_WL330GE
#endif /* TCONFIG_BCMARM */
};

/* NOTE: Do not insert new entries in the middle of this enum,
 * always add them to the end! The numeric Hardware ID value is
 * stored in the configuration file, and is used to determine
 * whether or not this config file can be restored on the router.
 */
enum {
#ifdef TCONFIG_BCMARM
	HW_UNKNOWN,
	HW_BCM4708
#elif defined(TCONFIG_RTNPLUS)
	HW_BCM4702,
	HW_BCM4712,
	HW_BCM5325E,
	HW_BCM4704_BCM5325F,
	HW_BCM5352E,
	HW_BCM5354G,
	HW_BCM4712_BCM5325E,
	HW_BCM4704_BCM5325F_EWC,
	HW_BCM4705L_BCM5325E_EWC,
	HW_BCM5350,
	HW_BCM5356,
	HW_BCM5357,
	HW_BCM53572,
	HW_BCM5358U,
	HW_BCM4716,
	HW_BCM4718,
	HW_BCM47186,
	HW_BCM4717,
	HW_BCM5365,
	HW_BCM4785,
	HW_BCM4706,
	HW_UNKNOWN
#else
	HW_BCM4702,
	HW_BCM4712,
	HW_BCM5325E,
	HW_BCM4704_BCM5325F,
	HW_BCM5352E,
	HW_BCM5354G,
	HW_BCM4712_BCM5325E,
	HW_BCM4704_BCM5325F_EWC,
	HW_BCM4705L_BCM5325E_EWC,
	HW_BCM5350,
	HW_BCM5356,
	HW_BCM4716,
	HW_BCM4718,
	HW_BCM4717,
	HW_BCM5365,
	HW_BCM4785,
	HW_UNKNOWN
#endif /* TCONFIG_BCMARM */
};

#define SUP_SES			(1 << 0)
#define SUP_BRAU		(1 << 1)
#define SUP_AOSS_LED		(1 << 2)
#define SUP_WHAM_LED		(1 << 3)
#define SUP_HPAMP		(1 << 4)
#define SUP_NONVE		(1 << 5)
#define SUP_80211N		(1 << 6)
#define SUP_1000ET		(1 << 7)
#ifdef TCONFIG_RTNPLUS
#define SUP_80211AC		(1 << 8)
#endif
#ifdef TCONFIG_BCMARM
#define SUP_80211AC_WAVE2	(1 << 9)
#endif /* TCONFIG_BCMARM */
extern int check_hw_type(void);
extern int get_model(void);
extern int supports(unsigned long attr);

/* process.c */
extern char *psname(int pid, char *buffer, int maxlen);
extern int pidof(const char *name);
extern int killall(const char *name, int sig);
extern int ppid(int pid);

/* files.c */
#define FW_CREATE		0
#define FW_APPEND		1
#define FW_NEWLINE		2
#if defined(TCONFIG_NGINX) || defined(TCONFIG_BT)
#define FWESC_JSON		1
#define FWESC_LINE		2
#endif
extern unsigned long f_size(const char *path);
extern int f_exists(const char *file);
extern int d_exists(const char *file);
extern int f_read(const char *file, void *buffer, int max); /* returns bytes read */
extern int f_write(const char *file, const void *buffer, int len, unsigned flags, unsigned cmode);
extern int f_read_string(const char *file, char *buffer, int max); /* returns bytes read, not including term; max includes term */
extern int f_write_string(const char *file, const char *buffer, unsigned flags, unsigned cmode);
extern int f_write_procsysnet(const char *path, const char *value);
#if defined(TCONFIG_NGINX) || defined(TCONFIG_BT)
extern int f_write_escaped(FILE *fp, int mode, const char *s1, const char *s2);
#endif
extern int f_read_alloc(const char *path, char **buffer, int max);
extern int f_read_alloc_string(const char *path, char **buffer, int max);
#ifdef TOMATO64
extern int f_micro_wait_exists(const char *name, int max, int invert);
#endif /* TOMATO64 */
extern int f_wait_exists(const char *name, int max);
extern int f_wait_notexists(const char *name, int max);
extern int file_lock(char *tag);
extern void file_unlock(int lockfd);

/* led.c */
#define LED_WLAN		0
#define LED_DIAG		1
#define LED_WHITE		2
#define LED_AMBER		3
#define LED_DMZ			4
#define LED_AOSS		5
#define LED_BRIDGE		6
#define LED_USB			7
#define LED_MYSTERY		LED_USB /* (unmarked LED between wireless and bridge on WHR-G54S) */
#ifdef TCONFIG_BCMARM
#define LED_USB3		8
#define LED_5G			9
#ifdef TCONFIG_AC3200
#define LED_52G			10
#define LED_COUNT		11
#else
#define LED_COUNT		10
#endif /* TCONFIG_AC3200 */
#elif defined(TCONFIG_RTNPLUS)
#define LED_5G			8
#define LED_COUNT		9
#else
#define LED_COUNT		8
#endif /* TCONFIG_BCMARM */
#define	LED_OFF			0
#define	LED_ON			1
#define LED_PROBE		2
/* support up to 32 GPIO pins for buttons, leds and some other IC functions */
#define TOMATO_GPIO_MAX		31
#define TOMATO_GPIO_MIN		0
#ifdef TCONFIG_BCMARM
#define T_HIGH			1
#define T_LOW			0

#define GPIO_00			0
#define GPIO_01			1
#define GPIO_02			2
#define GPIO_03			3
#define GPIO_04			4
#define GPIO_05			5
#define GPIO_06			6
#define GPIO_07			7
#define GPIO_08			8
#define GPIO_09			9
#define GPIO_10			10
#define GPIO_11			11
#define GPIO_12			12
#define GPIO_13			13
#define GPIO_14			14
#define GPIO_15			15
#define GPIO_16			16
#define GPIO_17			17
#define GPIO_18			18
#define GPIO_19			19
#define GPIO_20			20
#define GPIO_21			21
#define GPIO_22			22
#define GPIO_23			23
#define GPIO_24			24
#define GPIO_25			25
#define GPIO_26			26
#define GPIO_27			27
#define GPIO_28			28
#define GPIO_29			29
#define GPIO_30			30
#define GPIO_31			31
#endif /* TCONFIG_BCMARM */

extern const char *led_names[];
extern int gpio_open(uint32_t mask);
extern void gpio_write(uint32_t bit, int en);
extern uint32_t gpio_read(void);
extern uint32_t _gpio_read(int f);
#ifdef TCONFIG_BCMARM
extern uint32_t set_gpio(uint32_t gpio, uint32_t value);
#endif
extern int nvget_gpio(const char *name, int *gpio, int *inv);
extern int do_led(int which, int mode);
#ifdef TCONFIG_BCMARM
extern void do_led_nongpio(int model, int which, int mode);
#endif
static inline int led(int which, int mode)
{
	return (do_led(which, mode) != 255);
}
#ifdef TCONFIG_BCMARM
extern void disable_led_wanlan(void);
extern void enable_led_wanlan(void);
extern void do_led_bridge(int mode);
extern void led_setup(void);
#endif /* TCONFIG_BCMARM */

/* led_sysfs.c - Tomato64 sysfs LED control */
#ifdef TOMATO64
#include "led_sysfs.h"
#endif /* TOMATO64 */

/* ndpi.c - Tomato64 nDPI protocol table */
#ifdef TOMATO64
#define NDPI_HOSTNAME_MAX	128 /* sizeof(xt_ndpi_mtinfo.hostname), the match truncates past it */
#define NDPI_HOST_PROTOCOLS	"tls,quic,http,dns" /* the protocols nDPI takes a server name from */
extern int ndpi_proto_valid(const char *name); /* 1 = usable, 0 = not, -1 = table unavailable */
extern int ndpi_proto_list_valid(const char *v, char *bad, const size_t bad_sz); /* as stored in a rule */
extern int ndpi_proto_dissector(const char *name); /* 1 = detected from payload, 0 = from port/address, -1 = table unavailable */
extern int ndpi_proto_list_dissector(const char *v, char *bad, const size_t bad_sz); /* whole value usable with --inprogress */
extern const char *const *ndpi_proto_names(void); /* sorted, NULL terminated, owned by the library */
extern void ndpi_migrate_rules(void); /* rewrite rules naming a renamed protocol */
#endif /* TOMATO64 */

/* base64.c */
extern int base64_encode(const unsigned char *in, char *out, int inlen); /* returns amount of out buffer used */
extern int base64_decode(const char *in, unsigned char *out, int inlen); /* returns amount of out buffer used */
extern unsigned int base64_encoded_len(int len);
extern unsigned int base64_decoded_len(int len); /* maximum possible, not actual */

/* strings.c */
#define MAX_PORTS		64
#define PORT_SIZE		16
extern const char *find_word(const char *buffer, const char *word);
extern int remove_word(char *buffer, const char *word);
extern int del_str_line(char *str);
extern int is_port(char *str);
extern char *filter_space(char *str);
extern char* format_port(char *str);
extern char* trimstr(char *str);
extern char* splitpath( char *str, char *pathname, char *filename);
extern int splitport(char *in_ports, char out_port[MAX_PORTS][PORT_SIZE]);
extern int is_number(char *a);
extern int isspacex(char c);
extern char *shrink_space(char *dest, const char *src, int n);

/* wl.c */
#if defined(__CONFIG_DHDAP__) || defined(TCONFIG_DHDAP)
extern int dhd_probe(char *name);
extern int dhd_ioctl(char *name, int cmd, void *buf, int len);
extern int dhd_iovar_setbuf(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, unsigned int buflen);
extern int dhd_iovar_setint(char *ifname, char *iovar, int val);
extern int dhd_bssiovar_setint(char *ifname, char *iovar, int bssidx, int val);
#endif

/* shutils.c */
#ifdef TCONFIG_BCMBSD
 extern pid_t get_pid_by_name(const char *name); /* Returns the process ID */
#endif
#ifdef TCONFIG_RTNPLUS
 extern int getMTD(const char *name); /* Find partition with defined name and return partition number as an integer */
#endif
#ifdef TCONFIG_WIREGUARD
extern int wg_status(char *iface);
#endif
extern int bwlimit_status(void);
extern int qos_status(void);
extern unsigned int mwan_configured_num(void);
extern unsigned int mwan_active_num(void);

/* mdu.c/ddns.c */
#define MDU_STOP_FN		"/var/lib/mdu/mdu-stop"

#endif /* __SHARED_H__ */
