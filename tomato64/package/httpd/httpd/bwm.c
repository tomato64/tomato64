/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "tomato.h"

#include <sys/stat.h>
#include <sys/ioctl.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"bwm_debug"


static const char *hfn = "/var/lib/misc/rstats-history.gz";
static const char *ifn = "/var/lib/misc/cstats-history.gz";

void wo_statsbackup(char *url)
{
	struct stat st;
	time_t t;
	unsigned int i;
	const char *what, *name, *file;

	what = webcgi_safeget("_what", "bwm");
	if (strcmp(what, "bwm") == 0) {
		name = "rstats";
		file = hfn;
	}
	else {
		name = "cstats";
		file = ifn;
	}

	if (stat(file, &st) == 0) {
		t = st.st_mtime;
		sleep(1);
	}
	else
		t = 0;

	killall(name, SIGHUP);
	for (i = 10; i > 0; --i) {
		if ((stat(file, &st) == 0) && (st.st_mtime != t))
			break;

		sleep(1);
	}

	if (i == 0) {
		send_error(500, NULL, NULL);
		return;
	}
	send_header(200, NULL, mime_binary, 0);
	do_file((char *)file);
}

void wi_statsrestore(char *url, int len, char *boundary)
{
	char *buf;
	const char *error, *what, *name, *file;
	int n;
	char tmp[64];

	check_id(url);

	tmp[0] = 0;
	buf = NULL;
	error = "Error reading file";

	what = webcgi_safeget("_what", "bwm");
	if (strcmp(what, "bwm") == 0) {
		name = "rstats";
		file = hfn;
	}
	else {
		name = "cstats";
		file = ifn;
	}

	if (!skip_header(&len))
		goto exit;

	if ((len < 64) || (len > ((strcmp(what, "bwm") == 0) ? 16384 : 131072)))
		goto exit;

	if ((buf = malloc(len)) == NULL) {
		error = "Not enough memory";
		goto exit;
	}

	n = web_read(buf, len);
	len -= n;

	snprintf(tmp, sizeof(tmp), "%s.new", file);
	if (f_write(tmp, buf, n, 0, 0600) != n) {
		unlink(tmp);
		error = "Error writing temporary file";
		goto exit;
	}

	memset(tmp, 0, sizeof(tmp));
	snprintf(tmp, sizeof(tmp), "/var/tmp/%s-load", name);
	f_write(tmp, NULL, 0, 0, 0600);

	killall(name, SIGHUP);
	sleep(1);

	error = NULL;
	rboot = 1; /* used as "ok" */

exit:
	free(buf);
	web_eat(len);

	if (error != NULL)
		resmsg_set(error);
}

void wo_statsrestore(char *url)
{
	const char *what, *page;

	what = webcgi_safeget("_what", "bwm");
	if (rboot) {
		if (strcmp(what, "bwm") == 0)
			page = "/bwm-daily.asp";
		else
			page = "/ipt-daily.asp";

		redirect(page);
	}
	else
		parse_asp("error.asp");
}

void asp_netdev(int argc, char **argv)
{
	FILE *f;
	char buf[256];
	int64_t rx, tx;
	char *p;
	char *ifname;
	char comma;
	char *exclude;
	int sfd;
	struct ifreq ifr;

	exclude = nvram_safe_get("rstats_exclude");

	web_puts("\n\nnetdev={");
	if ((f = fopen("/proc/net/dev", "r")) != NULL) {
		fgets(buf, sizeof(buf), f); /* header */
		fgets(buf, sizeof(buf), f); /* " */
		comma = ' ';

		if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
			logmsg(LOG_DEBUG, "*** %s: %d: error opening socket %m\n", __FUNCTION__, __LINE__);

		while (fgets(buf, sizeof(buf), f)) {
			if ((p = strchr(buf, ':')) == NULL)
				continue;

			*p = 0;
			if ((ifname = strrchr(buf, ' ')) == NULL)
				ifname = buf;
			else
				++ifname;

			if ((strcmp(ifname, "lo") == 0) || (find_word(exclude, ifname)))
				continue;

			/* skip down interfaces */
			if (sfd >= 0) {
				strcpy(ifr.ifr_name, ifname);
				if (ioctl(sfd, SIOCGIFFLAGS, &ifr) != 0)
					continue;
				if ((ifr.ifr_flags & IFF_UP) == 0)
					continue;
			}

			/* <rx bytes, packets, errors, dropped, fifo errors, frame errors, compressed, multicast><tx ...> */
			if (sscanf(p + 1, "%llu%*u%*u%*u%*u%*u%*u%*u%llu", &rx, &tx) != 2)
				continue;

			web_printf("%c'%s':{rx:0x%llx,tx:0x%llx}", comma, ifname, rx, tx);
			comma = ',';
		}

		if (sfd >= 0)
			close(sfd);

		fclose(f);
	}
	web_puts("};\n");
}

void asp_iptmon(int argc, char **argv) {

	char comma;
	char sa[256];
	FILE *a;
	char *exclude;
	char *include;

	char ip[INET6_ADDRSTRLEN];

	int64_t tx, rx;

	exclude = nvram_safe_get("cstats_exclude");
	include = nvram_safe_get("cstats_include");

	char br;
	char name[] = "/proc/net/ipt_account/lanX";

	web_puts("\n\niptmon={");
	comma = ' ';

	for (br = 0; br < BRIDGE_COUNT; br++) {
		char wholenetstatsline = 1;

		char bridge[2] = "0";
		if (br != 0)
			bridge[0] += br;
		else
			strcpy(bridge, "");

		snprintf(name, sizeof(name), "/proc/net/ipt_account/lan%s", bridge);

		if ((a = fopen(name, "r")) == NULL)
			continue;

		if (!wholenetstatsline)
			fgets(sa, sizeof(sa), a); /* network */

		while (fgets(sa, sizeof(sa), a)) {
			if(sscanf(sa, "ip = %s bytes_src = %llu %*u %*u %*u %*u packets_src = %*u %*u %*u %*u %*u bytes_dst = %llu %*u %*u %*u %*u packets_dst = %*u %*u %*u %*u %*u time = %*u", ip, &tx, &rx) != 3 )
				continue;

			if (find_word(exclude, ip)) {
				wholenetstatsline = 0;
				continue;
			}

			if (((find_word(include, ip)) || (wholenetstatsline == 1)) || ((nvram_get_int("cstats_all")) && ((rx > 0) || (tx > 0)) )) {
				web_printf("%c'%s':{rx:0x%llx,tx:0x%llx}", comma, ip, rx, tx);
				comma = ',';
			}
			wholenetstatsline = 0;
		}
		fclose(a);
	}
	web_puts("};\n");
}

void asp_bandwidth(int argc, char **argv)
{
	const char *name;
	char tmp[32];
	int sig;

	if (argc == 2) {
		if (strcmp(argv[1], "bwm") == 0)
			name = "rstats";
		else
			name = "cstats";

		memset(tmp, 0, sizeof(tmp));
		snprintf(tmp, sizeof(tmp), "%s_enable", name);
		if ((nvram_get_int(tmp) == 1)) {
			memset(tmp, 0, sizeof(tmp));
			if (strcmp(argv[0], "speed") == 0) {
				sig = SIGUSR1;
				snprintf(tmp, sizeof(tmp), "/var/spool/%s-speed.js", name);
			}
			else {
				sig = SIGUSR2;
				snprintf(tmp, sizeof(tmp), "/var/spool/%s-history.js", name);
			}

			unlink(tmp);
			killall(name, sig);
			f_wait_exists(tmp, 5);
			do_file(tmp);
			unlink(tmp);
		}
	}
}
