/*
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


#include "rc.h"

#include <string.h>
#include <wlutils.h>
#include <dirent.h>
#include <linux/version.h>

#define samba_dir		"/etc/samba"
#define samba_var_dir		"/var/run/samba"
#define samba_configfile	"/etc/smb.conf"

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"samba_debug"


static void stop_wsdd(void)
{
	killall_tk_period_wait("wsdd2", 50);
}

static void start_wsdd(void)
{
	unsigned char ea[ETHER_ADDR_LEN];
	char serial[18];
	char bootparms[64];

	stop_wsdd();

	if (!ether_atoe(nvram_safe_get("lan_hwaddr"), ea))
		f_read("/dev/urandom", ea, sizeof(ea));

	snprintf(serial, sizeof(serial), "%02x%02x%02x%02x%02x%02x", ea[0], ea[1], ea[2], ea[3], ea[4], ea[5]);

	snprintf(bootparms, sizeof(bootparms), "sku:%s,serial:%s", (nvram_get("odmpid") ? : "FreshTomato"), serial);

	/* (-i: no multi-interface binds atm) */
	eval("wsdd2", "-d", "-w", "-i", nvram_safe_get("lan_ifname"), "-b", bootparms);
}

static void kill_samba(int sig)
{
	if (sig == SIGTERM) {
		killall_tk_period_wait("smbd", 50);
		killall_tk_period_wait("nmbd", 50);
	}
	else {
		killall("smbd", sig);
		killall("nmbd", sig);
	}
}

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_GROCTRL)
static void enable_gro(int interval)
{
	char *argv[3] = { "echo", "", NULL };
	char lan_ifname[32], *lan_ifnames, *next;
	char path[64];
	char parm[32];

	if (nvram_get_int("gro_disable"))
		return;

	/* enabled gro on vlan interface */
	lan_ifnames = nvram_safe_get("lan_ifnames");
	foreach(lan_ifname, lan_ifnames, next) {
		if (!strncmp(lan_ifname, "vlan", 4)) {
			memset(path, 0, 64);
			sprintf(path, ">>/proc/net/vlan/%s", lan_ifname);
			memset(parm, 0, 32);
			sprintf(parm, "-gro %d", interval);
			argv[1] = parm;
			_eval(argv, path, 0, NULL);
		}
	}
}
#endif

void start_samba(int force)
{
	FILE *fp;
	DIR *dir = NULL;
	struct dirent *dp;
	char nlsmod[16];
	int mode;
	char *nv;
	char *si;
	char *buf;
	char *p, *q;
	char *name, *path, *comment, *writeable, *hidden;
	int cnt = 0;
	char *smbd_user;
	int ret1 = 0, ret2 = 0;
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
	int taskset_ret = -1;
#endif

	/* only if enabled or forced and lan_hostname is set */
	mode = nvram_get_int("smbd_enable");
	if ((!mode && force == 0) || (!nvram_invmatch("lan_hostname", "")))
		return;

	if (pidof("nmbd") > 0) {
		logmsg(LOG_WARNING, "service nmbd already running");
		return;
	}

	if (serialize_restart("smbd", 1))
		return;

	if ((fp = fopen(samba_configfile, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, samba_configfile);
		return;
	}

#ifdef TCONFIG_BCMARM
	/* check samba enabled ? */
	if (mode)
		nvram_set("txworkq", "1"); /* set txworkq to 1, see et/sys/et_linux.c */
	else
		nvram_unset("txworkq");

#ifdef TCONFIG_GROCTRL
	/* enable / disable gro via GUI nas-samba.asp; Default: off */
	enable_gro(2);
#endif
#endif

	si = nvram_safe_get("smbd_ifnames");

	fprintf(fp, "[global]\n"
	            " interfaces = %s\n"
	            " bind interfaces only = yes\n"
	            " enable core files = no\n"
	            " deadtime = 30\n"
	            " smb encrypt = disabled\n"
	            " min receivefile size = 16384\n"
	            " workgroup = %s\n"
	            " netbios name = %s\n"
	            " server string = %s\n"
	            " dos charset = ASCII\n"
	            " unix charset = UTF8\n"
	            " display charset = UTF8\n"
	            " guest account = nobody\n"
	            " security = user\n"
	            " %s\n"
	            " guest ok = %s\n"
	            " guest only = no\n"
	            " browseable = yes\n"
	            " syslog only = yes\n"
	            " timestamp logs = no\n"
	            " syslog = 1\n"
	            " passdb backend = smbpasswd\n"
	            " encrypt passwords = yes\n"
	            " preserve case = yes\n"
	            " short preserve case = yes\n",
	            strlen(si) ? si : nvram_safe_get("lan_ifname"),
	            nvram_get("smbd_wgroup") ? : "WORKGROUP",
	            nvram_safe_get("lan_hostname"),
	            nvram_get("router_name") ? : "FreshTomato",
	            mode == 2 ? "" : "map to guest = Bad User",
	            mode == 2 ? "no" : "yes"); /* guest ok */

	fprintf(fp, " load printers = no\n" /* add for Samba printcap issue */
	            " printing = bsd\n"
	            " printcap name = /dev/null\n"
	            " map archive = no\n"
	            " map hidden = no\n"
	            " map read only = no\n"
	            " map system = no\n"
	            " store dos attributes = no\n"
	            " dos filemode = yes\n"
	            " strict locking = no\n"
	            " oplocks = yes\n"
	            " level2 oplocks = yes\n"
	            " kernel oplocks = no\n"
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
	            " use sendfile = no\n");
#else
	            " use sendfile = yes\n");
#endif

	if (nvram_get_int("smbd_wins")) {
		nv = nvram_safe_get("wan_wins");
		if ((!*nv) || (strcmp(nv, "0.0.0.0") == 0))
			fprintf(fp, " wins support = yes\n");
	}

	/* 0 - smb1, 1 - smb2, 2 - smb1 + smb2 */
	if (nvram_get_int("smbd_protocol") == 0)
		fprintf(fp, " max protocol = NT1\n");
	else
		fprintf(fp, " max protocol = SMB2\n");
	if (nvram_get_int("smbd_protocol") == 1)
		fprintf(fp, " min protocol = SMB2\n");

	if (nvram_get_int("smbd_master")) {
		fprintf(fp,
			" domain master = yes\n"
			" local master = yes\n"
			" preferred master = yes\n"
			" os level = 255\n");
	}

	nv = nvram_safe_get("smbd_cpage");
	if (*nv) {
		memset(nlsmod, 0, 16);
		sprintf(nlsmod, "nls_cp%s", nv);

		nv = nvram_safe_get("smbd_nlsmod");
		if ((*nv) && (strcmp(nv, nlsmod) != 0))
			modprobe_r(nv);

		modprobe(nlsmod);
		nvram_set("smbd_nlsmod", nlsmod);
	}

	nv = nvram_safe_get("smbd_custom");
	/* add socket options unless overriden by the user */
	if (strstr(nv, "socket options") == NULL)
		fprintf(fp, " socket options = TCP_NODELAY SO_KEEPALIVE IPTOS_LOWDELAY SO_RCVBUF=65536 SO_SNDBUF=65536\n");

	fprintf(fp, "%s\n", nv);

	/* configure shares */
	if ((buf = strdup(nvram_safe_get("smbd_shares"))) && (*buf)) {
		/* sharename<path<comment<writeable[0|1]<hidden[0|1] */

		p = buf;
		while ((q = strsep(&p, ">")) != NULL) {
			if (vstrsep(q, "<", &name, &path, &comment, &writeable, &hidden) < 5)
				continue;
			if (!path || !name)
				continue;

			/* share name */
			fprintf(fp, "\n[%s]\n", name);

			/* path */
			fprintf(fp, " path = %s\n", path);

			/* access level */
			if (!strcmp(writeable, "1"))
				fprintf(fp, " writable = yes\n delete readonly = yes\n force user = root\n");
			if (!strcmp(hidden, "1"))
				fprintf(fp, " browseable = no\n");

			/* comment */
			if (comment)
				fprintf(fp, " comment = %s\n", comment);

			cnt++;
		}
		free(buf);
	}

	/* share every mountpoint below MOUNT_ROOT */
	if (nvram_get_int("smbd_autoshare") && (dir = opendir(MOUNT_ROOT))) {
		while ((dp = readdir(dir))) {
			if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, "..")) {

				/* only if is a directory and is mounted */
				if (!dir_is_mountpoint(MOUNT_ROOT, dp->d_name))
					continue;

				/* smbd_autoshare: 0 - disable, 1 - read-only, 2 - writable, 3 - hidden writable */
				fprintf(fp, "\n[%s]\n path = %s/%s\n comment = %s\n", dp->d_name, MOUNT_ROOT, dp->d_name, dp->d_name);

				if (nvram_match("smbd_autoshare", "3")) /* hidden */
					fprintf(fp, "\n[%s$]\n path = %s/%s\n browseable = no\n", dp->d_name, MOUNT_ROOT, dp->d_name);

				if ((nvram_match("smbd_autoshare", "2")) || (nvram_match("smbd_autoshare", "3"))) /* RW */
					fprintf(fp, " writable = yes\n delete readonly = yes\n force user = root\n");

				cnt++;
			}
		}
	}
	if (dir)
		closedir(dir);

	if (cnt == 0) {
		/* by default share MOUNT_ROOT as read-only */
		fprintf(fp, "\n[share]\n"
		            " path = %s\n"
		            " writable = no\n",
		            MOUNT_ROOT);
	}

	fclose(fp);

	mkdir_if_none(samba_var_dir);
	mkdir_if_none(samba_dir);

	/* write smbpasswd */
	eval("smbpasswd", "nobody", "\"\"");

	if (mode == 2) {
		smbd_user = nvram_safe_get("smbd_user");
		if ((!*smbd_user) || (!strcmp(smbd_user, "root")))
			smbd_user = "nas";

		eval("smbpasswd", smbd_user, nvram_safe_get("smbd_passwd"));
	}

	kill_samba(SIGHUP);

	/* start samba */
	ret1 = eval("nmbd", "-D");

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
	if (cpu_num > 1)
		taskset_ret = cpu_eval(NULL, "1", "ionice", "-c1", "-n0", "smbd", "-D");
	else
		taskset_ret = eval("ionice", "-c1", "-n0", "smbd", "-D");

	if (taskset_ret != 0)
#endif
		ret2 = eval("smbd", "-D");

	if (ret1 || ret2) {
		kill_samba(SIGTERM);
		logmsg(LOG_ERR, "starting samba daemon failed ...");
	}
	else {
		start_wsdd();
		logmsg(LOG_INFO, "samba daemon is started");
	}
}

void stop_samba(void)
{
	if (serialize_restart("smbd", 0))
		return;

	stop_wsdd();

	/* stop samba */
	if ((pidof("smbd") > 0 ) || (pidof("nmbd") > 0 )) {
		kill_samba(SIGTERM);
		logmsg(LOG_INFO, "samba daemon is stopped");
	}

	/* clean up */
	unlink("/var/log/log.smbd");
	unlink("/var/log/log.nmbd");
	eval("rm", "-rf", "/var/nmbd");
	eval("rm", "-rf", "/var/log/cores");
	eval("rm", "-rf", "/var/run/samba");
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_GROCTRL)
	enable_gro(0);
#endif
}
