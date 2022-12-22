/*
 * mysql.c
 *
 * Copyright (C) 2014 bwq518, Hyzoom
 * Fixes/updates (C) 2018 - 2022 pedro
 *
 */


#include "rc.h"

#define mysql_etc_dir		"/etc/mysql"
#define mysql_conf_link		"/etc/my.cnf"
#define mysql_conf		mysql_etc_dir"/my.cnf"
#define mysql_start_script	mysql_etc_dir"/start_mysql.sh"
#define mysql_stop_script	mysql_etc_dir"/stop_mysql.sh"
#define mysql_passwd		mysql_etc_dir"/setpasswd.sql"
#define mysql_anyhost		mysql_etc_dir"/setanyhost.sql"
#define mysql_child_pid		mysql_etc_dir"/child.pid"
#define mysql_pid		"/var/run/mysqld.pid"
#define mysql_log		"/var/log/mysql.log"
#define mysql_dflt_dir		"/tmp/mysql"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"mysql_debug"


static void setup_mysql_watchdog(void)
{
	FILE *fp;
	char buffer[64], buffer2[64];
	int nvi;

	if ((nvi = nvram_get_int("mysql_check_time")) > 0) {
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), mysql_etc_dir"/watchdog.sh");

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "[ -z \"$(pidof mysqld)\" -a \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " logger -t mysql-watchdog mysqld stopped? Starting...\n"
			            " service mysql restart\n"
			            "}\n");
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

			memset(buffer2, 0, sizeof(buffer2));
			snprintf(buffer2, sizeof(buffer2), "*/%d * * * * %s", nvi, buffer);
			eval("cru", "a", "CheckMySQL", buffer2);
		}
	}
}

void start_mysql(int force)
{
	FILE *fp;
	char pbi[128];
	char ppr[64];
	char pdatadir[256], ptmpdir[256];
	char full_datadir[256], full_tmpdir[256], basedir[256];
	char tmp1[256], tmp2[256];
	int n;
	pid_t pidof_child = 0;
	unsigned int new_install = 0;
	char *nginx_docroot = nvram_safe_get("nginx_docroot");
	unsigned int anyhost = nvram_get_int("mysql_allow_anyhost");


	/* only if enabled or forced */
	if (!nvram_get_int("mysql_enable") && force == 0)
		return;

	if (serialize_restart("mysqld", 1))
		return;

	memset(tmp1, 0, sizeof(tmp1));
	if (f_read_string(mysql_child_pid, tmp1, sizeof(tmp1)) > 0 && atoi(tmp1) > 0 && ppid(atoi(tmp1)) > 0) { /* fork is still up */
		logmsg(LOG_WARNING, "%s: another process (PID: %s) still up, aborting ...", __FUNCTION__, tmp1);
		return;
	}

	logmsg(LOG_INFO, "starting mysqld ...");

	if (nvram_match("mysql_binary", "internal"))
		strlcpy(pbi, "/usr/bin", sizeof(pbi));
	else if (nvram_match("mysql_binary", "optware"))
		strlcpy(pbi, "/opt/bin", sizeof(pbi));
	else
		strlcpy(pbi, nvram_safe_get("mysql_binary_custom"), sizeof(pbi));

	if (pbi[strlen(pbi) - 1] == '/')
		pbi[strlen(pbi) - 1] = '\0';

	splitpath(pbi, basedir, tmp1);

	/* generate download saved path based on USB partition (mysql_dlroot) and directory name (mysql_datadir) */
	if (nvram_get_int("mysql_usb_enable")) {
		tmp1[0] = 0;
		tmp2[0] = 0;
		strlcpy(tmp1, nvram_safe_get("mysql_dlroot"), sizeof(tmp1));
		trimstr(tmp1);
		if (tmp1[0] != '/') {
			memset(tmp2, 0, sizeof(tmp2));
			snprintf(tmp2, sizeof(tmp2), "/%s", tmp1);
			strlcpy(tmp1, tmp2, sizeof(tmp1));
		}
		strlcpy(ppr, tmp1, sizeof(ppr));
		if (ppr[strlen(ppr) - 1] == '/')
			ppr[strlen(ppr) - 1] = 0;

		if (strlen(ppr) == 0) {
			logmsg(LOG_ERR, "No mounted USB partition found. You need to mount the USB drive first");
			return;
		}
	}
	else
		strlcpy(ppr, mysql_dflt_dir, sizeof(ppr));

	strlcpy(pdatadir, nvram_safe_get("mysql_datadir"), sizeof(pdatadir));
	trimstr(pdatadir);
	if (pdatadir[strlen(pdatadir) - 1] == '/')
		pdatadir[strlen(pdatadir) - 1] = 0;

	if (strlen(pdatadir) == 0) {
		strlcpy(pdatadir, "data", sizeof(pdatadir));
		nvram_set("mysql_dir", "data");
	}
	memset(full_datadir, 0, sizeof(full_datadir));
	if (pdatadir[0] == '/')
		snprintf(full_datadir, sizeof(full_datadir), "%s%s", ppr, pdatadir);
	else
		snprintf(full_datadir, sizeof(full_datadir), "%s/%s", ppr, pdatadir);

	strlcpy(ptmpdir, nvram_safe_get("mysql_tmpdir"), sizeof(ptmpdir));
	trimstr(ptmpdir);
	if (ptmpdir[strlen(ptmpdir) - 1] == '/')
		ptmpdir[strlen(ptmpdir) - 1] = 0;

	if (strlen(ptmpdir) == 0) {
		strcpy (ptmpdir, "tmp");
		nvram_set("mysql_tmpdir", "tmp");
	}
	memset(full_tmpdir, 0, sizeof(full_tmpdir));
	if (ptmpdir[0] == '/')
		snprintf(full_tmpdir, sizeof(full_tmpdir), "%s%s", ppr, ptmpdir);
	else
		snprintf(full_tmpdir, sizeof(full_tmpdir), "%s/%s", ppr, ptmpdir);

	mkdir_if_none(mysql_etc_dir);

	/* config file */
	if (!(fp = fopen(mysql_conf, "w"))) {
		logerr(__FUNCTION__, __LINE__, mysql_conf);
		return;
	}

	fprintf(fp, "[client]\n"
	            "port             = %s\n"
	            "socket           = /var/run/mysqld.sock\n\n"
	            "[mysqld]\n"
	            "user             = root\n"
	            "socket           = /var/run/mysqld.sock\n"
	            "port             = %s\n"
	            "basedir          = %s\n\n"
	            "datadir          = %s\n"
	            "tmpdir           = %s\n\n"
	            "skip-external-locking\n",
	            nvram_safe_get("mysql_port"),
	            nvram_safe_get("mysql_port"),
	            basedir,
	            full_datadir,
	            full_tmpdir);

	if (nvram_get_int("mysql_allow_anyhost"))
		fprintf(fp, "bind-address         = 0.0.0.0\n");
	else
		fprintf(fp, "bind-address         = 127.0.0.1\n");

	/* disable innodb engine */
	fprintf(fp, "default-storage-engine=MYISAM\n"
	            "innodb=OFF\n");

	fprintf(fp, "key_buffer_size      = %sM\n"
	            "max_allowed_packet   = %sM\n"
	            "thread_stack         = %sK\n"
	            "thread_cache_size    = %s\n\n"
	            "table_open_cache     = %s\n"
	            "sort_buffer_size     = %sK\n"
	            "read_buffer_size     = %sK\n"
	            "read_rnd_buffer_size = %sK\n"
	            "query_cache_size     = %sM\n"
	            "max_connections      = %s\n\n"
	            "#The following items are from mysql_server_custom\n"
	            "%s\n"
	            "#end of mysql_server_custom\n\n"
	            "[mysqldump]\n"
	            "quick\nquote-names\n"
	            "max_allowed_packet   = 16M\n"
	            "[mysql]\n\n"
	            "[isamchk]\n"
	            "key_buffer_size      = 8M\n"
	            "sort_buffer_size     = 8M\n\n",
	            nvram_safe_get("mysql_key_buffer"),
	            nvram_safe_get("mysql_max_allowed_packet"),
	            nvram_safe_get("mysql_thread_stack"),
	            nvram_safe_get("mysql_thread_cache_size"),
	            nvram_safe_get("mysql_table_open_cache"),
	            nvram_safe_get("mysql_sort_buffer_size"),
	            nvram_safe_get("mysql_read_buffer_size"),
	            nvram_safe_get("mysql_read_rnd_buffer_size"),
	            nvram_safe_get("mysql_query_cache_size"),
	            nvram_safe_get("mysql_max_connections"),
	            nvram_safe_get("mysql_server_custom"));

	fclose(fp);

	chmod(mysql_conf, 0644);

	unlink(mysql_conf_link);
	symlink(mysql_conf, mysql_conf_link);

	/* fork new process */
	if (fork() != 0)
		return;

	pidof_child = getpid();

	/* write child pid to a file */
	memset(tmp1, 0, sizeof(tmp1));
	snprintf(tmp1, sizeof(tmp1), "%d", pidof_child);
	f_write_string(mysql_child_pid, tmp1, 0, 0);

	/* wait a given time for partition to be mounted, etc */
	n = atoi(nvram_safe_get("mysql_sleep"));
	if (n > 0)
		sleep(n);

	/* clean mysql_log */
	system("/bin/rm -f "mysql_log);
	f_write(mysql_log, NULL, 0, 0, 0644);

	/* check for datadir */
	if (!d_exists(full_datadir)) {
		logmsg(LOG_INFO, "datadir in %s doesn't exist, creating ...", ppr);
		if (mkdir(full_datadir, 0755) == 0)
			logmsg(LOG_INFO, "created successfully");
		else {
			logmsg(LOG_ERR, "create failed, aborting ...");
			goto END;
		}
	}

	/* check for tmpdir */
	if (!d_exists(full_tmpdir)) {
		logmsg(LOG_INFO, "tmpdir in %s doesn't exist, creating ...", ppr);
		if (mkdir(full_tmpdir, 0755) == 0)
			logmsg(LOG_INFO, "created successfully");
		else {
			logmsg(LOG_ERR, "create failed, aborting ...");
			goto END;
		}
	}

	/* check for tables_priv.MYD */
	memset(tmp1, 0, sizeof(tmp1));
	snprintf(tmp1, sizeof(tmp1), "%s/mysql/tables_priv.MYD", full_datadir);
	if (!f_exists(tmp1)) {
		new_install = 1;
		logmsg(LOG_INFO, "tables_priv.MYD not found - it's a new MySQL installation");
		f_write_string(mysql_log, "=========Found NO tables_priv.MYD====================", FW_APPEND | FW_NEWLINE, 0);
		f_write_string(mysql_log, "This is new installed MySQL.", FW_APPEND | FW_NEWLINE, 0);
	}

	/* initialize DB? */
	if (nvram_get_int("mysql_init_priv") || new_install == 1) {
		logmsg(LOG_INFO, "initializing privileges table ...");
		f_write_string(mysql_log, "=========mysql_install_db====================", FW_APPEND | FW_NEWLINE, 0);
		memset(tmp1, 0, sizeof(tmp1));
		snprintf(tmp1, sizeof(tmp1), "%s/mysql_install_db --user=root --force >> %s 2>&1", pbi, mysql_log);
		system(tmp1);

		logmsg(LOG_DEBUG, "*** %s: privileges table successfully initialized", __FUNCTION__);
		nvram_set("mysql_init_priv", "0");
		nvram_commit();
	}

	/* initialize root password? */
	if (nvram_get_int("mysql_init_rootpass") || new_install == 1) {
		logmsg(LOG_INFO, "(re-)initializing root password ...");
		f_write_string(mysql_log, "=========mysqld skip-grant-tables==================", FW_APPEND | FW_NEWLINE, 0);
		memset(tmp1, 0, sizeof(tmp1));
		snprintf(tmp1, sizeof(tmp1), "%s/mysqld --skip-grant-tables --skip-networking --pid-file=%s >> %s 2>&1 &", pbi, mysql_pid, mysql_log);
		system(tmp1);
		sleep(2);

		if (f_exists(mysql_passwd))
			unlink(mysql_passwd);

		f_write_string(mysql_passwd, "use mysql;", FW_CREATE | FW_NEWLINE, 0644);

		memset(tmp1, 0, sizeof(tmp1));
		snprintf(tmp1, sizeof(tmp1), "update user set password=password('%s') where user='root';", nvram_safe_get("mysql_passwd"));
		f_write_string(mysql_passwd, tmp1, FW_APPEND | FW_NEWLINE, 0);

		f_write_string(mysql_passwd, "flush privileges;", FW_APPEND | FW_NEWLINE, 0);

		memset(tmp1, 0, sizeof(tmp1));
		snprintf(tmp1, sizeof(tmp1), "=========mysql < %s====================", mysql_passwd);
		f_write_string(mysql_log, tmp1, FW_APPEND | FW_NEWLINE, 0);

		memset(tmp1, 0, sizeof(tmp1));
		snprintf(tmp1, sizeof(tmp1), "%s/mysql < %s >> %s", pbi, mysql_passwd, mysql_log);
		system(tmp1);

		f_write_string(mysql_log, "=========mysqldadmin shutdown====================", FW_APPEND | FW_NEWLINE, 0);
		memset(tmp1, 0, sizeof(tmp1));
		snprintf(tmp1, sizeof(tmp1), "%s/mysqladmin -uroot -p\"%s\" --shutdown_timeout=3 shutdown >> %s 2>&1", pbi, nvram_safe_get("mysql_passwd"), mysql_log);
		system(tmp1);

		killall_tk_period_wait("mysqld", 50);
		sleep(1);
		system("/bin/rm -f "mysql_pid" "mysql_passwd);

		logmsg(LOG_DEBUG, "*** %s: root password successfully (re-)initialized", __FUNCTION__);
		nvram_set("mysql_init_rootpass", "0");
		nvram_commit();
	}

	f_write_string(mysql_log, "=========mysqld startup====================", FW_APPEND | FW_NEWLINE, 0);
	memset(tmp1, 0, sizeof(tmp1));
	snprintf(tmp1, sizeof(tmp1), "%s/mysqld --pid-file=%s >> %s 2>&1 &", pbi, mysql_pid, mysql_log);
	system(tmp1);

	if (anyhost == 1) {
		sleep(3);
		if (f_exists(mysql_anyhost))
			unlink(mysql_anyhost);

		f_write_string(mysql_anyhost, "GRANT ALL PRIVILEGES ON *.* TO 'root'@'%%' WITH GRANT OPTION;", FW_CREATE | FW_NEWLINE, 0644);
		f_write_string(mysql_anyhost, "flush privileges;", FW_APPEND | FW_NEWLINE, 0);

		memset(tmp1, 0, sizeof(tmp1));
		snprintf(tmp1, sizeof(tmp1), "=========mysql < %s==================== >> %s", mysql_anyhost, mysql_log);
		f_write_string(mysql_log, tmp1, FW_APPEND | FW_NEWLINE, 0);

		memset(tmp1, 0, sizeof(tmp1));
		snprintf(tmp1, sizeof(tmp1), "%s/mysql -uroot -p\"%s\" < %s >> %s 2>&1", pbi, nvram_safe_get("mysql_passwd"), mysql_anyhost, mysql_log);
		system(tmp1);
		system("/bin/rm -f "mysql_anyhost);
	}

	eval("mkdir", "-p", nginx_docroot);
	eval("cp", "-p" "/www/adminer.php", nginx_docroot);
	sleep(1);

END:
	if (pidof("mysqld") > 0) {
		logmsg(LOG_INFO, "mysqld started");
		setup_mysql_watchdog();
		f_write_string(mysql_child_pid, "0", 0, 0);
	}
	else {
		logmsg(LOG_ERR, "starting mysqld failed - check configuration ...");
		f_write_string(mysql_child_pid, "0", 0, 0);
		stop_mysql();
	}

	/* terminate the child */
	exit(0);
}

void stop_mysql(void)
{
	char pbi[128], buf[512];
	int m = atoi(nvram_safe_get("mysql_sleep")) + 70;

	if (serialize_restart("mysqld", 0))
		return;

	logmsg(LOG_INFO, "terminating mysqld ...");

	/* wait for child of start_mysql to finish (if any) */
	memset(buf, 0, sizeof(buf));
	while (f_read_string(mysql_child_pid, buf, sizeof(buf)) > 0 && atoi(buf) > 0 && ppid(atoi(buf)) > 0 && (m-- > 0)) {
		logmsg(LOG_DEBUG, "*** %s: waiting for child process of start_mysql to end, %d secs left ...", __FUNCTION__, m);
		sleep(1);
	}

	eval("cru", "d", "CheckMySQL");

	if (nvram_match("mysql_binary", "internal"))
		strlcpy(pbi, "/usr/bin", sizeof(pbi));
	else if (nvram_match("mysql_binary", "optware"))
		strlcpy(pbi, "/opt/bin", sizeof(pbi));
	else
		strlcpy(pbi, nvram_safe_get("mysql_binary_custom"), sizeof(pbi));

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "%s/mysqladmin -uroot -p\"%s\" --shutdown_timeout=3 shutdown", pbi, nvram_safe_get("mysql_passwd"));
	system(buf);

	killall_and_waitfor("mysqld", 10, 80);

	logmsg(LOG_INFO, "mysqld stopped");

	/* clean-up */
	system("/bin/rm -f "mysql_pid);
	unlink(mysql_conf_link);
	system("/bin/rm -rf "mysql_etc_dir);
}
