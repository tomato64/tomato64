/*
 * Router rc control script
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: rc.h,v 1.39 2005/03/29 02:00:06 honor Exp $
 */


#ifndef __RC_H__
#define __RC_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <time.h>

#include <bcmnvram.h>
#include <utils.h>
#include <shutils.h>
#include <shared.h>

#include <tomato_profile.h>
#include <tomato_config.h>

// #define DEBUG_IPTFILE
// #define DEBUG_RCTEST
// #define DEBUG_NOISY

#define MOUNT_ROOT	"/tmp/mnt"
#define PROC_SCSI_ROOT	"/proc/scsi"
#define USB_STORAGE	"usb-storage"

#define BOOT		0
#define REDIAL		1
#define CONNECTING	2

#define PPPOEWAN	0
#define PPPOEWAN2	1
#ifdef TCONFIG_MULTIWAN
#define PPPOEWAN3	2
#define PPPOEWAN4	3
#endif

/* see init.c - used for /proc/sys/vm/min_free_kbytes */
#define TOMATO_RAM_HIGH_END	(200 * 1024)
#define TOMATO_RAM_MID_END	(100 * 1024)
#define TOMATO_RAM_LOW_END	(50 * 1024)
#ifndef TCONFIG_BCMARM
#define TOMATO_RAM_VLOW_END	(25 * 1024)
#endif

/* see init.c - used for tune_smp_affinity */
#if defined(TCONFIG_BCMSMP) && defined(TCONFIG_USB)
#define TOMATO_CPU0	"1" /* assign CPU 0 */
#define TOMATO_CPU1	"2" /* assign CPU 1 */
#define TOMATO_CPUX	"3" /* assign CPU 1 and 2 */
#endif

typedef enum { IPT_TABLE_NAT, IPT_TABLE_FILTER, IPT_TABLE_MANGLE } ipt_table_t;

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)

#define IPT_V4			0x01
#define IPT_V6			0x02
#define IPT_ANY_AF		(IPT_V4 | IPT_V6)
#define IPT_AF_IS_EMPTY(f)	((f & IPT_ANY_AF) == 0)

const char *chain_in_drop;
const char *chain_in_accept;
const char *chain_out_drop;
const char *chain_out_accept;
const char *chain_out_reject;

static inline int is_wet(int idx, int unit, int subunit, void *param)
{
	return nvram_match(wl_nvname("mode", unit, subunit), "wet");
}

#ifdef TCONFIG_BCMWL6
static inline int is_psta(int idx, int unit, int subunit, void *param)
{
	return nvram_match(wl_nvname("mode", unit, subunit), "psta");
}
#endif /* TCONFIG_BCMWL6 */

/* rc.c */
extern void chains_log_detection(void);
extern void fix_chain_in_drop(void);
extern int env2nv(char *env, char *nv);
extern int serialize_restart(char *service, int start);
extern void run_del_firewall_script(char *infile, char *outfile);

/* init.c */
extern int init_main(int argc, char *argv[]);
extern int reboothalt_main(int argc, char *argv[]);
extern int console_main(int argc, char *argv[]);

/* interface.c */
extern int _ifconfig(const char *name, int flags, const char *addr, const char *netmask, const char *dstaddr, int mtu);
static inline int ifconfig(const char *name, int flags, const char *addr, const char *netmask)
{
	return _ifconfig(name, flags, addr, netmask, NULL, 0);
}
extern int route_add(char *name, int metric, char *dst, char *gateway, char *genmask);
extern void route_del(char *name, int metric, char *dst, char *gateway, char *genmask);
extern void config_loopback(void);
extern void start_vlan(void);
extern void stop_vlan(void);
extern int config_vlan(void);
#ifdef TCONFIG_IPV6
extern int ipv6_mapaddr4(struct in6_addr *addr6, int ip6len, struct in_addr *addr4, int ip4mask);
#endif

/* listen.c */
extern int listen_main(int argc, char **argv);

/* ppp.c */
extern int ipup_main(int argc, char **argv);
extern int ipdown_main(int argc, char **argv);
extern int ippreup_main(int argc, char **argv);
extern int pppevent_main(int argc, char **argv);
#ifdef TCONFIG_IPV6
extern int ip6up_main(int argc, char **argv);
extern int ip6down_main(int argc, char **argv);
#endif

/* redial.c */
extern void start_redial(char *prefix);
extern void stop_redial(char *prefix);
extern int redial_main(int argc, char **argv);

/* wan.c */
extern void start_pptp(char *prefix);
extern void stop_pptp(char *prefix);
extern void start_pppoe(int, char *prefix);
extern void stop_pppoe(char *prefix);
extern void start_l2tp(char *prefix);
extern void stop_l2tp(char *prefix);
extern void store_wan_if_to_nvram(char *prefix);
extern void start_wan_if(char *prefix);
extern void start_wan(void);
extern void start_wan_done(char *ifname,char *prefix);
extern void start_wanall_done(void);
extern char *wan_gateway(char *prefix);
#ifdef TCONFIG_IPV6
extern void start_wan6(const char *wan_ifname);
extern void stop_wan6(void);
#endif
extern void stop_wan_if(char *prefix);
extern void stop_wan();
extern void force_to_dial(char *prefix);
extern void do_wan_routes(char *ifname, int metric, int add, char *prefix);
extern void preset_wan(char *ifname, char *gw, char *netmask, char *prefix);

/* mwan.c */
extern int get_sta_wan_prefix(char *sPrefix);
extern void get_wan_info(char *sPrefix);
extern void mwan_table_add(char *sPrefix);
extern void mwan_table_del(char *sPrefix);
extern void mwan_load_balance(void);
extern int mwan_route_main(int argc, char **argv);
extern void mwan_state_files(void);
#ifdef TCONFIG_PPTPD
extern void get_cidr(char *ipaddr, char *netmask, char *cidr, const size_t buf_sz);
#endif

/* pbr.c */
extern void ipt_routerpolicy(void);

/* network.c */
extern void set_host_domain_name(void);
extern void set_et_qos_mode(void);
extern void start_lan(void);
extern void stop_lan(void);
extern void hotplug_net(void);
extern void do_static_routes(int add);
extern int radio_main(int argc, char *argv[]);
extern int wldist_main(int argc, char *argv[]);
extern void stop_wireless(void);
extern void start_wireless(void);
extern void restart_wireless(void);
extern void start_wl(void);
extern int disabled_wl(int idx, int unit, int subunit, void *param);
extern void unload_wl(void);
extern void load_wl(void);
#ifdef TCONFIG_IPV6
extern void enable_ipv6(int enable);
extern void accept_ra(const char *ifname);
extern void accept_ra_reset(const char *ifname);
extern void ipv6_forward(const char *ifname, int enable);
extern void ndp_proxy(const char *ifname, int enable);
#else
#define enable_ipv6(enable) do {} while (0)
#define accept_ra(ifname) do {} while (0)
#define accept_ra_reset(ifname) do {} while (0)
#define ipv6_forward(ifname, enable) do {} while (0)
#define ndp_proxy(ifname, enable) do {} while (0)
#endif /* TCONFIG_IPV6 */

/* dhcpc.c */
extern int dhcpc_event_main(int argc, char **argv);
extern int dhcpc_release_main(int argc, char **argv);
extern int dhcpc_renew_main(int argc, char **argv);
extern int dhcpc_lan_main(int argc, char **argv);
extern void start_dhcpc(char *prefix);
extern void stop_dhcpc(char *prefix);
extern void start_dhcpc_lan(void);
extern void stop_dhcpc_lan(void);
extern void do_connect_file(unsigned int renew, char *prefix);
#ifdef TCONFIG_IPV6
extern int dhcp6c_state_main(int argc, char **argv);
extern void start_dhcp6c(void);
extern void stop_dhcp6c(void);
#endif

/* services.c */
extern void start_cron(void);
extern void stop_cron(void);
#ifdef TCONFIG_FANCTRL
extern void start_phy_tempsense(void);
extern void stop_phy_tempsense(void);
#endif
extern void start_adblock(int update);
extern void stop_adblock(void);
#ifdef TCONFIG_ZEBRA
extern void start_zebra(void);
extern void stop_zebra(void);
#endif
extern void start_upnp(void);
extern void stop_upnp(void);
extern void start_syslog(void);
extern void stop_syslog(void);
extern void start_igmp_proxy(void);
extern void stop_igmp_proxy(void);
extern void start_udpxy(void);
extern void stop_udpxy(void);
extern void start_httpd(void);
extern void stop_httpd(void);
extern void clear_resolv(void);
extern void dns_to_resolv(void);
extern void start_dnsmasq(void);
extern void stop_dnsmasq(void);
extern void reload_dnsmasq(void);
#ifdef TCONFIG_STUBBY
extern void start_stubby(void);
extern void stop_stubby(void);
#endif
#ifdef TCONFIG_DNSCRYPT
extern void start_dnscrypt(void);
extern void stop_dnscrypt(void);
#endif
extern void set_tz(void);
extern void start_ntpd(void);
extern void stop_ntpd(void);
extern int ntpd_synced_main(int argc, char *argv[]);
extern void check_services(void);
extern void exec_service(void);
extern int service_main(int argc, char *argv[]);
extern void start_service(const char *name);
extern void stop_service(const char *name);
extern void restart_service(const char *name);
extern void start_services(void);
extern void stop_services(void);
#ifdef TCONFIG_USB
extern void restart_nas_services(int stop, int start);
#else
#define restart_nas_services(args...) do { } while(0)
#endif /* TCONFIG_USB */
extern void start_hotplug2();
extern void stop_hotplug2(void);
#ifdef TCONFIG_IPV6
extern void start_ipv6_tunnel(void);
extern void stop_ipv6_tunnel(void);
extern void start_6rd_tunnel(void);
extern void stop_6rd_tunnel(void);
extern void start_ipv6(void);
extern void stop_ipv6(void);
#endif /* TCONFIG_IPV6 */
#ifdef TCONFIG_BCMBSD
extern int start_bsd(void);
extern void stop_bsd(void);
#endif /* TCONFIG_BCMBSD */
#ifdef TCONFIG_MDNS
extern void start_mdns(void);
extern void stop_mdns(void);
#endif /* TCONFIG_MDNS */

/* usb.c */
#ifdef TCONFIG_USB
extern void start_usb(void);
extern void stop_usb(void);
extern void remove_usb_module(void);
extern int dir_is_mountpoint(const char *root, const char *dir);
extern void hotplug_usb(void);
extern void remove_storage_main(int shutdn);
#else
#define start_usb(args...) do { } while(0)
#define stop_usb(args...) do { } while(0)
#define dir_is_mountpoint(args...) (0)
#define hotplug_usb(args...) do { } while(0)
#define remove_storage_main(args...) do { } while(0)
#endif /* TCONFIG_USB */

/* wnas.c */
extern int wds_enable(void);
extern int wl_security_on(void);
extern void start_nas(void);
extern void stop_nas(void);
extern void notify_nas(const char *ifname);

/* firewall.c */
typedef void (*_tf_ipt_write)(const char *format, ... );
extern wanface_list_t wanfaces;
extern wanface_list_t wan2faces;
#ifdef TCONFIG_MULTIWAN
extern wanface_list_t wan3faces;
extern wanface_list_t wan4faces;
#endif
extern char lanaddr[BRIDGE_COUNT][32];
extern char lanmask[BRIDGE_COUNT][32];
extern char lanface[BRIDGE_COUNT][IFNAMSIZ + 1];
#ifdef TCONFIG_IPV6
extern char wan6face[];
#endif
extern char lan_cclass[];
extern char **layer7_in;
extern void enable_ip_forward(void);
extern void ipt_write(const char *format, ...);
extern void ip6t_write(const char *format, ...);
#ifdef TCONFIG_IPV6
#define ip46t_write(args...) do { ipt_write(args); ip6t_write(args); } while(0)
#define ip46t_flagged_write(do_ip46t, args...) do { if (do_ip46t & IPT_V4) ipt_write(args); if (do_ip46t & IPT_V6) ip6t_write(args); } while(0)
#define ip46t_cond_write(do_ip6t, args...) do { if (do_ip6t) ip6t_write(args); else ipt_write(args); } while(0)
#ifdef TCONFIG_BCMARM
#define ipt_flagged_write(do_ip46t, args...) do { if (do_ip46t & IPT_V4) ipt_write(args); if (do_ip46t & IPT_V6) ip6t_write(args); } while(0)
#define ipt_cond_write(do_ip6t, args...) do { if (do_ip6t) ip6t_write(args); else ipt_write(args); } while(0)
#endif /* TCONFIG_BCMARM */
#else /* !TCONFIG_IPV6 */
#define ip46t_write ipt_write
#define ip46t_flagged_write(do_ip46t, args...) do { if (do_ip46t & IPT_V4) ipt_write(args); } while(0)
#define ip46t_cond_write(do_ip6t, args...) ipt_write(args)
#ifdef TCONFIG_BCMARM
#define ipt_flagged_write(do_ip46t, args...) do { if (do_ip46t & IPT_V4) ipt_write(args); } while(0)
#define ipt_cond_write(do_ip6t, args...) ipt_write(args)
#endif /* TCONFIG_BCMARM */
#endif /* TCONFIG_IPV6 */
extern void ipt_log_unresolved(const char *addr, const char *addrtype, const char *categ, const char *name);
extern int ipt_addr(char *addr, int maxlen, const char *s, const char *dir, int af, int strict, const char *categ, const char *name);
extern int ipt_dscp(const char *v, char *opt);
extern int ipt_ipp2p(const char *v, char *opt);
extern int ipt_layer7(const char *v, char *opt);
#define ipt_source_strict(s, src, categ, name) ipt_addr(src, 64, s, "src", IPT_V4, 1, categ, name)
#define ipt_source(s, src, categ, name) ipt_addr(src, 64, s, "src", IPT_V4, 0, categ, name)
#define ip6t_source(s, src, categ, name) ipt_addr(src, 128, s, "src", IPT_V6, 0, categ, name)
extern int start_firewall(void);
extern int stop_firewall(void);
#ifdef DEBUG_IPTFILE
extern void create_test_iptfile(void);
#endif
extern void allow_fastnat(const char *service, int allow);
extern void try_enabling_fastnat(void);
extern void log_segfault(void);

/* forward.c */
extern void ipt_forward(ipt_table_t table);
extern void ipt_triggered(ipt_table_t table);

#ifdef TCONFIG_IPV6
extern void ip6t_forward(void);
#endif

/* restrict.c */
extern int rcheck_main(int argc, char *argv[]);
extern void ipt_restrictions(void);
extern void sched_restrictions(void);

/* qos.c */
extern void ipt_qos(void);
extern void start_qos(char *prefix);
extern void stop_qos(char *prefix);

/* cifs.c */
#ifdef TCONFIG_CIFS
extern void start_cifs(void);
extern void stop_cifs(void);
extern int mount_cifs_main(int argc, char *argv[]);
#else
static inline void start_cifs(void) { };
static inline void stop_cifs(void) { };
#endif

/* jffs2.c */
#ifdef TCONFIG_JFFS2
extern void init_jffs2(void);
extern void start_jffs2(void);
extern void stop_jffs2(void);
#else
static inline void init_jffs2(void) { };
static inline void start_jffs2(void) { };
static inline void stop_jffs2(void) { };
#endif

/* ddns.c */
#ifdef TCONFIG_DDNS
extern void start_ddns(void);
extern void stop_ddns(void);
extern int ddns_update_main(int argc, char **argv);
#else
static inline void start_ddns(void) { };
static inline void stop_ddns(void) { };
#endif

/* misc.c */
extern void usage_exit(const char *cmd, const char *help) __attribute__ ((noreturn));
#define modprobe(mod, args...) ({ char *argv[] = { "modprobe", "-s", mod, ## args, NULL }; _eval(argv, NULL, 0, NULL); })
extern int modprobe_r(const char *mod);
#define xstart(args...)	_xstart(args, NULL)
extern int _xstart(const char *cmd, ...);
extern void run_nvscript(const char *nv, const char *arg1, int wtime);
extern void run_userfile (char *folder, char *extension, const char *arg1, int wtime);
extern void setup_conntrack(void);
extern void remove_conntrack(void);
extern int host_addr_info(const char *name, int af, struct sockaddr_storage *buf);
extern int host_addrtypes(const char *name, int af);
extern void inc_mac(char *mac, int plus);
extern void set_mac(const char *ifname, const char *nvname, int plus);
extern const char *default_wanif(void);
extern void simple_unlock(const char *name);
extern void simple_lock(const char *name);
extern int mkdir_if_none(const char *path);
extern long fappend(FILE *out, const char *fname);
extern long fappend_file(const char *path, const char *fname);

/* telssh.c */
extern void create_passwd(void);
extern void start_sshd(void);
extern void stop_sshd(void);
extern void start_telnetd(void);
extern void stop_telnetd(void);

/* mtd.c */
extern int mtd_erase(const char *mtdname);
extern int mtd_unlock(const char *mtdname);
#ifdef TCONFIG_BCMARM
extern int mtd_erase_old(const char *mtdname);
extern int mtd_write_main_old(int argc, char *argv[]);
extern int mtd_unlock_erase_main_old(int argc, char *argv[]);
extern int mtd_write(const char *path, const char *mtd);
#else
extern int mtd_write_main(int argc, char *argv[]);
extern int mtd_unlock_erase_main(int argc, char *argv[]);
#endif

/* buttons.c */
extern int buttons_main(int argc, char *argv[]);

#if defined(TCONFIG_BCMARM) || defined(TCONFIG_BLINK)
/* blink.c */
extern int blink_main(int argc, char *argv[]);

/* blink_br.c */
extern int blink_br_main(int argc, char *argv[]);
#endif

/* phy_tempsense.c */
#ifdef TCONFIG_FANCTRL
extern int phy_tempsense_main(int argc, char *argv[]);
#endif

/* led.c */
extern int led_main(int argc, char *argv[]);

/* gpio.c */
extern int gpio_main(int argc, char *argv[]);

/* sched.c */
extern int sched_main(int argc, char *argv[]);
extern void start_sched(void);
extern void stop_sched(void);

/* pptp_client.c */
#ifdef TCONFIG_PPTPD
#define PPTP_CLIENT_TABLE_ID 5
#define PPTP_CLIENT_TABLE_NAME "PPTP"
extern void start_pptp_client(void);
extern void stop_pptp_client(void);
extern void start_pptp_client_eas(void);
extern void stop_pptp_client_eas(void);
extern int write_pptp_client_resolv(FILE*);
extern int pptpc_ipup_main(int argc, char **argv);
extern int pptpc_ipdown_main(int argc, char **argv);
extern void pptp_client_firewall(const char *table, const char *opt, _tf_ipt_write table_writer);
#else
static inline void start_pptp_client_eas(void) {};
static inline void stop_pptp_client_eas(void) {};
#define write_pptp_client_resolv(f) (0)
#endif

/* nvram */
extern int nvram_file2nvram(const char *name, const char *filename);
extern int nvram_nvram2file(const char *name, const char *filename);

/* transmission.c */
#ifdef TCONFIG_BT
extern void start_bittorrent(int force);
extern void stop_bittorrent(void);
extern void run_bt_firewall_script(void);
#endif

/* nfs.c */
#ifdef TCONFIG_NFS
extern void start_nfs();
extern void stop_nfs();
#endif

/* snmp.c */
#ifdef TCONFIG_SNMP
extern void start_snmp();
extern void stop_snmp();
#endif

/* tor.c */
#ifdef TCONFIG_TOR
extern void start_tor(int force);
extern void stop_tor(void);
#endif

/* apcupsd.c */
#ifdef TCONFIG_UPS
extern void start_ups();
extern void stop_ups();
#endif

/* pptpd.c */
#ifdef TCONFIG_PPTPD
extern void start_pptpd(int force);
extern void stop_pptpd(void);
extern void write_pptpd_dnsmasq_config(FILE* f);
extern void run_pptpd_firewall_script(void);
#endif

/* openvpn.c */
#ifdef TCONFIG_OPENVPN
extern void start_ovpn_client(int clientNum);
extern void stop_ovpn_client(int clientNum);
extern void start_ovpn_server(int serverNum);
extern void stop_ovpn_server(int serverNum);
extern void start_ovpn_eas();
extern void stop_ovpn_eas();
extern void stop_ovpn_all();
extern void run_ovpn_firewall_scripts();
extern void write_ovpn_dnsmasq_config(FILE*);
extern int write_ovpn_resolv(FILE*);
#endif

/* tinc.c */
#ifdef TCONFIG_TINC
extern void start_tinc(int force);
extern void stop_tinc(void);
extern void run_tinc_firewall_script(void);
#endif

/* bwlimit.c */
extern void ipt_bwlimit(int chain);
extern void start_bwlimit(void);
extern void stop_bwlimit(void);

/* arpbind.c */
extern void start_arpbind(void);
extern void stop_arpbind(void);

/* mmc.c */
#ifdef TCONFIG_SDHC
extern void start_mmc(void);
extern void stop_mmc(void);
#endif

/* nocat.c */
#ifdef TCONFIG_NOCAT
extern void start_nocat();
extern void stop_nocat();
#endif

/* nginx.c */
#ifdef TCONFIG_NGINX
extern void nginx_write(const char *format, ...);
extern void start_nginx(int force);
extern void stop_nginx(void);
extern void run_nginx_firewall_script(void);

/* mysql.c */
extern void start_mysql(int force);
extern void stop_mysql(void);
#endif /* TCONFIG_NGINX */

/* tomatoanon.c */
extern void start_tomatoanon();
extern void stop_tomatoanon();

/* roamast.c */
#ifdef TCONFIG_ROAM
extern void start_roamast(void);
extern void stop_roamast(void);
extern int roam_assistant_main(int argc, char *argv[]);
#endif

/* samba.c */
#ifdef TCONFIG_SAMBASRV
extern void start_samba(int force);
extern void stop_samba(void);
#endif /* TCONFIG_SAMBASRV */

/* ftpd.c */
#ifdef TCONFIG_FTP
extern void start_ftpd(int force);
extern void stop_ftpd(void);
extern void run_ftpd_firewall_script(void);
#endif

#endif /* __RC_H__ */
