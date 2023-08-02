/*
	listen.c -- Listen for any packet through an interface
	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

	This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
	the contents of this file may not be disclosed to third parties,
	copied or duplicated in any form without the prior written
	permission of CyberTAN Inc.

	This software should be used as a reference only, and it not
	intended for production use!

	THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
*/


#include "rc.h"

#include <sys/ioctl.h>
#include <arpa/inet.h>
/* for PF_PACKET */
#include <features.h>

#if __GLIBC__ >=2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"listen_debug"


enum { L_FAIL, L_ERROR, L_UPGRADE, L_ESTABLISHED, L_SUCCESS };

struct iphdr {
	u_int8_t version;
	u_int8_t tos;
	u_int16_t tot_len;
	u_int16_t id;
	u_int16_t frag_off;
	u_int8_t ttl;
	u_int8_t protocol;
	u_int16_t check;
	u_int8_t saddr[4];
	u_int8_t daddr[4];
} __attribute__((packed));

struct EthPacket {
	u_int8_t dst_mac[6];
	u_int8_t src_mac[6];
	u_int8_t type[2];
	u_int8_t version;
	u_int8_t tos;
	u_int16_t tot_len;
	u_int16_t id;
	u_int16_t frag_off;
	u_int8_t ttl;
	u_int8_t protocol;
	u_int16_t check;
	u_int8_t saddr[4];
	u_int8_t daddr[4];
	u_int8_t data[1500 - 20];
} __attribute__((packed));


static int read_interface(const char *interface, int *ifindex, unsigned char *mac)
{
	int fd;
	struct ifreq ifr;
	int r;

	memset(&ifr, 0, sizeof(struct ifreq));
	if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		logmsg(LOG_DEBUG, "*** %s: socket failed!", __FUNCTION__);
		return -1;
	}

	r = -1;
	ifr.ifr_addr.sa_family = AF_INET;
	strcpy(ifr.ifr_name, interface);

	if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) {
		*ifindex = ifr.ifr_ifindex;
		logmsg(LOG_DEBUG, "*** %s: adapter index %d", __FUNCTION__, ifr.ifr_ifindex);

		if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
			memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
			logmsg(LOG_DEBUG, "*** %s: adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x", __FUNCTION__, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			r = 0;
		}
		else
			logmsg(LOG_DEBUG, "*** %s: SIOCGIFHWADDR failed!", __FUNCTION__);
	}
	else
		logmsg(LOG_DEBUG, "*** %s: SIOCGIFINDEX failed!", __FUNCTION__);

	close(fd);

	return r;
}

static int raw_socket(int ifindex)
{
	int fd;
	struct sockaddr_ll sock;

	logmsg(LOG_DEBUG, "*** %s: opening raw socket on ifindex %d", __FUNCTION__, ifindex);

	if ((fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
		logmsg(LOG_DEBUG, "*** %s: socket call failed!", __FUNCTION__);
		return -1;
	}

	sock.sll_family = AF_PACKET;
	sock.sll_protocol = htons(ETH_P_IP);
	sock.sll_ifindex = ifindex;
	if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0) {
		logmsg(LOG_DEBUG, "*** %s: bind call failed!", __FUNCTION__);
		close(fd);
		return -1;
	}

	return fd;
}

static u_int16_t checksum(void *addr, int count)
{
	/* Compute Internet Checksum for "count" bytes beginning at location "addr" */
	register int32_t sum = 0;
	u_int16_t *source = (u_int16_t *) addr;

	while (count > 1)  {
		/* This is the inner loop */
		sum += *source++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0) {
		/* Make sure that the left-over byte is added correctly both with little and big endian hosts */
		u_int16_t tmp = 0;
		*(unsigned char *)(&tmp) = *(unsigned char *)source;
		sum += tmp;
	}

	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}

static int listen_interface(char *interface, int wan_proto, char *prefix)
{
	int ifindex = 0;
	fd_set rfds;
	struct EthPacket packet;
	struct timeval tv;
	int retval;
	unsigned char mac[6];
	static int fd;
	int ret = L_SUCCESS;
	int bytes;
	u_int16_t check;
	struct in_addr ipaddr, netmask;
	char tmp[100];


	if (read_interface(interface, &ifindex, mac) < 0)
		return L_ERROR;

	fd = raw_socket(ifindex);
	if (fd < 0) {
		logmsg(LOG_DEBUG, "*** %s: FATAL: couldn't listen on socket", __FUNCTION__);
		return L_ERROR;
	}

	while (1) {
		if (!wait_action_idle(5)) {	/* Don't execute during upgrading */
			ret = L_UPGRADE;
			break;
		}
		if (check_wanup(prefix)) {
			ret = L_ESTABLISHED;
			break;
		}

		tv.tv_sec = 100000;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		logmsg(LOG_DEBUG, "*** %s: waiting for select...", __FUNCTION__);
		retval = select(fd + 1, &rfds, NULL, NULL, &tv);

		if (retval == 0) {
			logmsg(LOG_DEBUG, "*** %s: no packet recieved!", __FUNCTION__);
			continue;
		}

		memset(&packet, 0, sizeof(struct EthPacket));
		bytes = read(fd, &packet, sizeof(struct EthPacket));
		if (bytes < 0) {
			close(fd);
			logmsg(LOG_DEBUG, "*** %s: couldn't read on raw listening socket -- ignoring", __FUNCTION__);
			usleep(500000);		/* possible down interface, looping condition */
			return L_FAIL;
		}

		if (bytes < (int) (sizeof(struct iphdr))) {
			logmsg(LOG_DEBUG, "*** %s: message too short, ignoring", __FUNCTION__);
			ret = L_FAIL;
			goto EXIT;
		}

		if (memcmp(mac, packet.dst_mac, 6) != 0) {
			logmsg(LOG_DEBUG, "*** %s: dest %02x:%02x:%02x:%02x:%02x:%02x mac not the router", __FUNCTION__, packet.dst_mac[0], packet.dst_mac[1], packet.dst_mac[2], packet.dst_mac[3], packet.dst_mac[4], packet.dst_mac[5]);
			ret = L_FAIL;
			goto EXIT;
		}

		if (inet_addr(nvram_safe_get("lan_ipaddr")) == *(u_int32_t *)packet.daddr) {
			logmsg(LOG_DEBUG, "*** %s: dest ip equal to lan ipaddr", __FUNCTION__);
			ret = L_FAIL;
			goto EXIT;
		}

		logmsg(LOG_DEBUG, "*** %s: inet_addr=%x, packet.daddr=%x", __FUNCTION__, inet_addr(nvram_safe_get("lan_ipaddr")), *(u_int32_t *)packet.daddr);
		logmsg(LOG_DEBUG, "*** %s: %02X%02X%02X%02X%02X%02X,%02X%02X%02X%02X%02X%02X,%02X%02X", __FUNCTION__,
		     packet.dst_mac[0], packet.dst_mac[1], packet.dst_mac[2], packet.dst_mac[3], packet.dst_mac[4], packet.dst_mac[5],
		     packet.src_mac[0], packet.src_mac[1], packet.src_mac[2], packet.src_mac[3], packet.src_mac[4], packet.src_mac[5],
		     packet.type[0],packet.type[1]);
		logmsg(LOG_DEBUG, "*** %s: ip.version = %x", __FUNCTION__, packet.version);
		logmsg(LOG_DEBUG, "*** %s: ip.tos = %x", __FUNCTION__, packet.tos);
		logmsg(LOG_DEBUG, "*** %s: ip.tot_len = %x", __FUNCTION__, packet.tot_len);
		logmsg(LOG_DEBUG, "*** %s: ip.id = %x", __FUNCTION__, packet.id);
		logmsg(LOG_DEBUG, "*** %s: ip.ttl= %x", __FUNCTION__, packet.ttl);
		logmsg(LOG_DEBUG, "*** %s: ip.protocol= %x", __FUNCTION__, packet.protocol);
		logmsg(LOG_DEBUG, "*** %s: ip.check=%04x", __FUNCTION__, packet.check);
		logmsg(LOG_DEBUG, "*** %s: ip.saddr=%08x", __FUNCTION__, *(u_int32_t *)&(packet.saddr));
		logmsg(LOG_DEBUG, "*** %s: ip.daddr=%08x", __FUNCTION__, *(u_int32_t *)&(packet.daddr));

		if (*(u_int16_t *)packet.type == 0x0800) {
			logmsg(LOG_DEBUG, "*** %s: not ip protocol", __FUNCTION__);
			ret = L_FAIL;
			goto EXIT;
		}

		/* ignore any extra garbage bytes */
		bytes = ntohs(packet.tot_len);

		/* check IP checksum */
		check = packet.check;
		packet.check = 0;

		if (check != checksum(&(packet.version), sizeof(struct iphdr))) {
			logmsg(LOG_DEBUG, "*** %s: bad IP header checksum, ignoring", __FUNCTION__);
			logmsg(LOG_DEBUG, "*** %s: check received = %X, should be %X", __FUNCTION__, check, checksum(&(packet.version), sizeof(struct iphdr)));
			ret = L_FAIL;
			goto EXIT;
		}

		/* setup wan gateway ip / mask to check (wan ip or pptp/l2tp server ip)
		 * NB! pptp_server_ip / l2tp_server_ip can be defined as hostname also
		 * so ping to it will fail listen check in case it's defined that way
		 */
		switch (wan_proto) {
			case WP_PPTP:
				inet_aton(nvram_safe_get(strlcat_r(prefix, "_pptp_server_ip", tmp, sizeof(tmp))), &ipaddr);
				break;
			case WP_L2TP:
#ifdef TCONFIG_L2TP
				inet_aton(nvram_safe_get(strlcat_r(prefix, "_l2tp_server_ip", tmp, sizeof(tmp))), &ipaddr);
#endif
				break;
			default:
				inet_aton(nvram_safe_get(strlcat_r(prefix, "_ipaddr", tmp, sizeof(tmp))), &ipaddr);
				break;
		}
		inet_aton(nvram_safe_get(strlcat_r(prefix, "_netmask", tmp, sizeof(tmp))), &netmask);

		logmsg(LOG_DEBUG, "*** %s: %d", __FUNCTION__, (strlcat_r(prefix, "_gateway=%08x", tmp, sizeof(tmp)), ipaddr.s_addr));
		logmsg(LOG_DEBUG, "*** %s: %d", __FUNCTION__, (strlcat_r(prefix, "_netmask=%08x", tmp, sizeof(tmp)), netmask.s_addr));

		if ((ipaddr.s_addr & netmask.s_addr) != (*(u_int32_t *)&(packet.daddr) & netmask.s_addr)) {
			ret = L_FAIL;
			goto EXIT;
		}
		else {	/* dest IP is PPTP/L2TP server IP or WAN IP */
			logmsg(LOG_DEBUG, "*** %s: got packet: saddr=%s", __FUNCTION__, inet_ntop(AF_INET, &packet.saddr, tmp, INET_ADDRSTRLEN));
			logmsg(LOG_DEBUG, "*** %s: got packet: daddr=%s", __FUNCTION__, inet_ntop(AF_INET, &packet.daddr, tmp, INET_ADDRSTRLEN));
			logmsg(LOG_DEBUG, "*** %s: match saddr: %s", __FUNCTION__, inet_ntop(AF_INET, &ipaddr.s_addr, tmp, INET_ADDRSTRLEN));
			logmsg(LOG_DEBUG, "*** %s: match nmask: %s", __FUNCTION__, inet_ntop(AF_INET, &netmask.s_addr, tmp, INET_ADDRSTRLEN));
			logmsg(LOG_DEBUG, "*** %s: match SUCCESS", __FUNCTION__);

			ret = L_SUCCESS;
			goto EXIT;
		}
		/* all other packets to outside world, can potentially trigger WAN on any Internet activity in LAN, with real On Demand mode */
		logmsg(LOG_DEBUG, "*** %s: got saddr=%s", __FUNCTION__, inet_ntop(AF_INET, &packet.saddr, tmp, INET_ADDRSTRLEN));
		logmsg(LOG_DEBUG, "*** %s: got daddr=%s", __FUNCTION__, inet_ntop(AF_INET, &packet.daddr, tmp, INET_ADDRSTRLEN));
	}

EXIT:
	if (fd)
		close(fd);

	return ret;
}

int listen_main(int argc, char *argv[])
{
	char *interface;
	char pid_file[64];
	char prefix[] = "wanXX";
	FILE *fp;
	pid_t  pid;
	pid_t rpid;

	if (argc < 2)
		usage_exit(argv[0], "<interface> <wanN>");

	interface = argv[1];
	strcpy(prefix, argv[2]);

	printf("Starting listen on %s ...", interface);

	pid = fork();

	if (pid != 0)	/* foreground process */
		return 0;

	if (pid == 0) {	/* forked process */
		memset(pid_file, 0, 64);
		sprintf(pid_file, "/var/run/listen-%s.pid", prefix);

		/* read / write pid */
		if (access(pid_file, F_OK) != -1) {	/* pid file exists */
			fp = fopen(pid_file, "r");
			fscanf(fp, "%d", &rpid);
			fclose(fp);
			if (kill(rpid, 0) == 0)		/* process running */
				return EXIT_FAILURE;
			else {	/* no process */
				if ((fp = fopen(pid_file, "w")) != NULL) {
					fprintf(fp, "%d", getpid());	/* write new one */
					fclose(fp);
				}
			}
		}
		else if ((fp = fopen(pid_file, "w")) != NULL) {	/* file doesn't exist */
			fprintf(fp, "%d", getpid());
			fclose(fp);
		}
		else
			return EXIT_FAILURE;
	}

	while (1) {
		switch (listen_interface(interface, get_wanx_proto(prefix), prefix)) {
			case L_SUCCESS:
				logmsg(LOG_DEBUG, "*** %s: LAN to %s packet received", __FUNCTION__, prefix);
				force_to_dial(prefix);
				if (check_wanup(prefix))
					return 0;
				/* Connect fail, we want to re-connect session */
				sleep(3);
				break;
			case L_UPGRADE:
				logmsg(LOG_DEBUG, "*** %s: upgrade: nothing to do ...", __FUNCTION__);
				unlink(pid_file);
				return 0;
			case L_ESTABLISHED:
				logmsg(LOG_DEBUG, "*** %s: the link had been established", __FUNCTION__);
				unlink(pid_file);
				return 0;
			case L_ERROR:
				logmsg(LOG_DEBUG, "*** %s: ERROR", __FUNCTION__);
				unlink(pid_file);
				return 0;
//			case L_FAIL:
//				logmsg(LOG_DEBUG, "*** %s: FAIL", __FUNCTION__);
//				break;
		}
	}
}
