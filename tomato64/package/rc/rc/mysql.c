/*
 * mysql.c
 *
 * Copyright (C) 2014 bwq518, Hyzoom
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
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
#define mysql_ready_timeout	30

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"mysql_debug"


static void mysql_chomp_trailing_slash(char *s)
{
	size_t len;

	if (!s)
		return;

	len = strlen(s);
	if ((len > 0) && (s[len - 1] == '/'))
		s[len - 1] = '\0';
}

static int mysql_make_bin(char *dst, size_t dstlen, const char *dir, const char *name)
{
	int n;

	if (!dst || (dstlen == 0) || !dir || !*dir || !name || !*name)
		return EINVAL;

	n = snprintf(dst, dstlen, "%s/%s", dir, name);
	if ((n < 0) || (n >= (int)dstlen))
		return ENAMETOOLONG;

	return 0;
}

static int mysql_eval_bin(const char *dir, const char *name, char *argv[], const char *path, int async)
{
	char cmd[256];
	int pid;
	int rc;

	rc = mysql_make_bin(cmd, sizeof(cmd), dir, name);
	if (rc != 0) {
		logmsg(LOG_ERR, "%s: invalid executable path: %s/%s", __FUNCTION__, dir ? dir : "", name ? name : "");
		return rc;
	}

	argv[0] = cmd;
	if (async) {
		pid = -1;
		return _eval(argv, path, 0, &pid);
	}

	return _eval(argv, path, 0, NULL);
}

static int mysql_eval_bin_pwd(const char *dir, const char *name, char *argv[], const char *path, const char *password)
{
	char *old_pwd;
	char *saved_pwd;
	int rc;

	if (!password)
		password = "";

	old_pwd = getenv("MYSQL_PWD");
	saved_pwd = NULL;

	if (old_pwd) {
		saved_pwd = malloc(strlen(old_pwd) + 1);
		if (!saved_pwd)
			return ENOMEM;

		strcpy(saved_pwd, old_pwd);
	}

	if (setenv("MYSQL_PWD", password, 1) < 0) {
		rc = errno;
		if (saved_pwd)
			free(saved_pwd);
		return rc;
	}

	rc = mysql_eval_bin(dir, name, argv, path, 0);

	if (saved_pwd) {
		if ((setenv("MYSQL_PWD", saved_pwd, 1) < 0) && (rc == 0))
			rc = errno;
		free(saved_pwd);
	}
	else {
		if ((unsetenv("MYSQL_PWD") < 0) && (rc == 0))
			rc = errno;
	}

	return rc;
}

static int mysql_wait_ready(const char *dir, const char *password, int timeout)
{
	char *argv[8];
	int i;
	int rc;

	if (timeout <= 0)
		timeout = 1;

	for (i = 0; i < timeout; ++i) {
		argv[1] = "-uroot";
		argv[2] = "--socket=/var/run/mysqld.sock";
		argv[3] = "ping";
		argv[4] = NULL;

		if (password)
			rc = mysql_eval_bin_pwd(dir, "mysqladmin", argv, NULL, password);
		else
			rc = mysql_eval_bin(dir, "mysqladmin", argv, NULL, 0);

		if (rc == 0)
			return 0;

		sleep(1);
	}

	return ETIMEDOUT;
}

static int mysql_to_hex(char *dst, size_t dstlen, const char *src)
{
	static const char hex[] = "0123456789abcdef";
	unsigned char c;
	size_t n;

	if (!dst || (dstlen == 0))
		return EINVAL;

	if (!src)
		src = "";

	n = 0;
	while (*src) {
		if ((n + 2) >= dstlen)
			return ENAMETOOLONG;

		c = (unsigned char)*src++;
		dst[n++] = hex[c >> 4];
		dst[n++] = hex[c & 0x0f];
	}

	dst[n] = '\0';
	return 0;
}

static int mysql_conf_write_escaped(FILE *fp, const char *key, const char *value)
{
	if (fprintf(fp, "%-17s = ", key) < 0)
		return -1;

	if (f_write_escaped(fp, FWESC_LINE, value, NULL) < 0)
		return -1;

	if (fputc('\n', fp) == EOF)
		return -1;

	return ferror(fp) ? -1 : 0;
}

static void setup_mysql_watchdog(void)
{
	FILE *fp;
	char buffer[64], buffer2[64];
	int nvi;

	if ((nvi = nvram_get_int("mysql_check_time")) > 0) {
		snprintf(buffer, sizeof(buffer), mysql_etc_dir"/watchdog.sh");

		if ((fp = fopen(buffer, "w"))) {
			fprintf(fp, "#!/bin/sh\n"
			            "[ -z \"$(pidof mysqld)\" -a \"$(nvram get g_upgrade)\" != \"1\" -a \"$(nvram get g_reboot)\" != \"1\" ] && {\n"
			            " logger -t mysql-watchdog mysqld stopped? Starting...\n"
			            " service mysql restart\n"
			            "}\n");
			fclose(fp);
			chmod(buffer, (S_IRUSR | S_IWUSR | S_IXUSR));

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
	char pid_arg[64], sql[768], pass_hex[512];
	char *argv[8];
	int n, rc;
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

	if (nvram_match("mysql_binary", "internal"))
		strlcpy(pbi, "/usr/bin", sizeof(pbi));
	else if (nvram_match("mysql_binary", "optware"))
		strlcpy(pbi, "/opt/bin", sizeof(pbi));
	else
		strlcpy(pbi, nvram_safe_get("mysql_binary_custom"), sizeof(pbi));

	mysql_chomp_trailing_slash(pbi);

	splitpath(pbi, basedir, tmp1);

	/* generate download saved path based on USB partition (mysql_dlroot) and directory name (mysql_datadir) */
	if (nvram_get_int("mysql_usb_enable")) {
		tmp1[0] = 0;
		tmp2[0] = 0;
		strlcpy(tmp1, nvram_safe_get("mysql_dlroot"), sizeof(tmp1));
		trimstr(tmp1);
		if (tmp1[0] != '/') {
			snprintf(tmp2, sizeof(tmp2), "/%s", tmp1);
			strlcpy(tmp1, tmp2, sizeof(tmp1));
		}
		strlcpy(ppr, tmp1, sizeof(ppr));
		mysql_chomp_trailing_slash(ppr);

		if (strlen(ppr) == 0) {
			logmsg(LOG_ERR, "No mounted USB partition found. You need to mount the USB drive first");
			return;
		}
	}
	else
		strlcpy(ppr, mysql_dflt_dir, sizeof(ppr));

	strlcpy(pdatadir, nvram_safe_get("mysql_datadir"), sizeof(pdatadir));
	trimstr(pdatadir);
	mysql_chomp_trailing_slash(pdatadir);

	if (strlen(pdatadir) == 0) {
		strlcpy(pdatadir, "data", sizeof(pdatadir));
		nvram_set("mysql_datadir", "data");
	}

	if (pdatadir[0] == '/')
		snprintf(full_datadir, sizeof(full_datadir), "%s%s", ppr, pdatadir);
	else
		snprintf(full_datadir, sizeof(full_datadir), "%s/%s", ppr, pdatadir);

	strlcpy(ptmpdir, nvram_safe_get("mysql_tmpdir"), sizeof(ptmpdir));
	trimstr(ptmpdir);
	mysql_chomp_trailing_slash(ptmpdir);

	if (strlen(ptmpdir) == 0) {
		strlcpy (ptmpdir, "tmp", sizeof(ptmpdir));
		nvram_set("mysql_tmpdir", "tmp");
	}

	if (ptmpdir[0] == '/')
		snprintf(full_tmpdir, sizeof(full_tmpdir), "%s%s", ppr, ptmpdir);
	else
		snprintf(full_tmpdir, sizeof(full_tmpdir), "%s/%s", ppr, ptmpdir);

#ifdef TOMATO64
	snprintf(full_datadir, sizeof(full_datadir), "%s", pdatadir);
	snprintf(full_tmpdir, sizeof(full_tmpdir), "%s", ptmpdir);
	mkdir_if_none(full_datadir);
	mkdir_if_none(full_tmpdir);
#endif /* TOMATO64 */

	mkdir_if_none(mysql_etc_dir);

	/* config file */
	if (!(fp = fopen(mysql_conf, "w"))) {
		logerr(__FUNCTION__, __LINE__, mysql_conf);
		return;
	}

	fprintf(fp, "[client]\n"
	            "port             = %d\n"
	            "socket           = /var/run/mysqld.sock\n\n"
	            "[mysqld]\n"
	            "user             = root\n"
	            "socket           = /var/run/mysqld.sock\n"
	            "port             = %d\n",
	            nvram_get_int("mysql_port"),
	            nvram_get_int("mysql_port"));

	if ((mysql_conf_write_escaped(fp, "basedir", basedir) < 0) || (fputc('\n', fp) == EOF) ||
	    (mysql_conf_write_escaped(fp, "datadir", full_datadir) < 0) ||
	    (mysql_conf_write_escaped(fp, "tmpdir", full_tmpdir) < 0) || (fputs("\nskip-external-locking\n", fp) == EOF)) {
		logerr(__FUNCTION__, __LINE__, mysql_conf);
		fclose(fp);
		return;
	}

	if (nvram_get_int("mysql_allow_anyhost"))
		fprintf(fp, "bind-address         = 0.0.0.0\n");
	else
		fprintf(fp, "bind-address         = 127.0.0.1\n");

	/* disable innodb engine */
	fprintf(fp, "default-storage-engine=MYISAM\n"
	            "innodb=OFF\n");

	fprintf(fp, "key_buffer_size      = %dM\n"
	            "max_allowed_packet   = %dM\n"
	            "thread_stack         = %dK\n"
	            "thread_cache_size    = %d\n\n"
	            "table_open_cache     = %d\n"
	            "sort_buffer_size     = %dK\n"
	            "read_buffer_size     = %dK\n"
	            "read_rnd_buffer_size = %dK\n"
	            "query_cache_size     = %dM\n"
	            "max_connections      = %d\n\n"
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
	            nvram_get_int("mysql_key_buffer"),
	            nvram_get_int("mysql_max_allowed_packet"),
	            nvram_get_int("mysql_thread_stack"),
	            nvram_get_int("mysql_thread_cache_size"),
	            nvram_get_int("mysql_table_open_cache"),
	            nvram_get_int("mysql_sort_buffer_size"),
	            nvram_get_int("mysql_read_buffer_size"),
	            nvram_get_int("mysql_read_rnd_buffer_size"),
	            nvram_get_int("mysql_query_cache_size"),
	            nvram_get_int("mysql_max_connections"),
	            nvram_safe_get("mysql_server_custom"));

	fclose(fp);

	chmod(mysql_conf, 0644);

	unlink(mysql_conf_link);
	symlink(mysql_conf, mysql_conf_link);

	/* fork new process */
	pidof_child = fork();
	if (pidof_child < 0) {
		logerr(__FUNCTION__, __LINE__, "fork");
		return;
	}
	if (pidof_child != 0) /* foreground process */
		return;

	pidof_child = getpid();

	/* write child pid to a file */
	snprintf(tmp1, sizeof(tmp1), "%d", pidof_child);
	f_write_string(mysql_child_pid, tmp1, 0, 0);

	/* wait a given time for partition to be mounted, etc */
	n = nvram_get_int("mysql_sleep");
	if (n > 0 && n < 60) {
		logmsg(LOG_INFO, "mysqld - delaying start by %d seconds ...", n);
		sleep(n);
	}

	/* clean mysql_log */
	eval("rm", "-f", mysql_log);
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

#ifndef TOMATO64
	/* check for tables_priv.MYD */
	snprintf(tmp1, sizeof(tmp1), "%s/mysql/tables_priv.MYD", full_datadir);
	if (!f_exists(tmp1)) {
		new_install = 1;
		logmsg(LOG_INFO, "tables_priv.MYD not found - it's a new MySQL installation");
		f_write_string(mysql_log, "=========Found NO tables_priv.MYD====================", FW_APPEND | FW_NEWLINE, 0);
		f_write_string(mysql_log, "This is new installed MySQL.", FW_APPEND | FW_NEWLINE, 0);
	}
#else
	/* check for tables_priv.MAD (MariaDB uses Aria engine for system tables) */
	snprintf(tmp1, sizeof(tmp1), "%s/mysql/tables_priv.MAD", full_datadir);
	if (!f_exists(tmp1)) {
		new_install = 1;
		logmsg(LOG_INFO, "tables_priv.MAD not found - it's a new MySQL installation");
		f_write_string(mysql_log, "=========Found NO tables_priv.MAD====================", FW_APPEND | FW_NEWLINE, 0);
		f_write_string(mysql_log, "This is new installed MySQL.", FW_APPEND | FW_NEWLINE, 0);
	}
#endif /* TOMATO64 */

	/* initialize DB? */
	if (nvram_get_int("mysql_init_priv") || new_install == 1) {
		logmsg(LOG_INFO, "initializing privileges table ...");
		f_write_string(mysql_log, "=========mysql_install_db====================", FW_APPEND | FW_NEWLINE, 0);
		argv[1] = "--user=root";
		argv[2] = "--force";
		argv[3] = NULL;
		rc = mysql_eval_bin(pbi, "mysql_install_db", argv, ">>" mysql_log, 0);
		if (rc != 0) {
			logmsg(LOG_ERR, "%s: mysql_install_db returned %d", __FUNCTION__, rc);
			goto END;
		}

		logmsg(LOG_DEBUG, "*** %s: privileges table successfully initialized", __FUNCTION__);
		nvram_set("mysql_init_priv", "0");
		nvram_commit();
	}

	/* initialize root password? */
	if (nvram_get_int("mysql_init_rootpass") || new_install == 1) {
		logmsg(LOG_INFO, "(re-)initializing root password ...");
		f_write_string(mysql_log, "=========mysqld skip-grant-tables==================", FW_APPEND | FW_NEWLINE, 0);
		rc = snprintf(pid_arg, sizeof(pid_arg), "--pid-file=%s", mysql_pid);
		if ((rc < 0) || (rc >= (int)sizeof(pid_arg))) {
			logmsg(LOG_ERR, "%s: pid-file argument too long", __FUNCTION__);
			goto END;
		}

		argv[1] = "--skip-grant-tables";
		argv[2] = "--skip-networking";
		argv[3] = pid_arg;
		argv[4] = NULL;
		rc = mysql_eval_bin(pbi, "mysqld", argv, ">>" mysql_log, 1);
		if (rc != 0) {
			logmsg(LOG_ERR, "%s: failed to start mysqld for password initialization: %d", __FUNCTION__, rc);
			goto END;
		}

		rc = mysql_wait_ready(pbi, NULL, mysql_ready_timeout);
		if (rc != 0) {
			logmsg(LOG_ERR, "%s: mysqld did not become ready for password initialization: %d", __FUNCTION__, rc);
			killall_tk_period_wait("mysqld", 50);
			eval("rm", "-f", mysql_pid);
			goto END;
		}

		f_write_string(mysql_log, "=========mysql --execute password update====================", FW_APPEND | FW_NEWLINE, 0);

		rc = mysql_to_hex(pass_hex, sizeof(pass_hex), nvram_safe_get("mysql_passwd"));
		if (rc != 0) {
			logmsg(LOG_ERR, "%s: mysql password is too long", __FUNCTION__);
			killall_tk_period_wait("mysqld", 50);
			sleep(1);
			eval("rm", "-f", mysql_pid, mysql_passwd);
			goto END;
		}

#ifndef TOMATO64
		if (pass_hex[0])
			rc = snprintf(sql, sizeof(sql), "update user set password=password(0x%s) where user='root';", pass_hex);
		else
			rc = snprintf(sql, sizeof(sql), "update user set password=password('') where user='root';");
#else
		/* MariaDB 10.4+ dropped the plain `password` column on mysql.user;
		 * use the plugin-aware SET PASSWORD statement instead.
		 */
		if (pass_hex[0])
			rc = snprintf(sql, sizeof(sql), "SET PASSWORD FOR 'root'@'localhost' = PASSWORD(0x%s);", pass_hex);
		else
			rc = snprintf(sql, sizeof(sql), "SET PASSWORD FOR 'root'@'localhost' = PASSWORD('');");
#endif /* TOMATO64 */
		if ((rc < 0) || (rc >= (int)sizeof(sql))) {
			logmsg(LOG_ERR, "%s: password SQL is too long", __FUNCTION__);
			killall_tk_period_wait("mysqld", 50);
			sleep(1);
			eval("rm", "-f", mysql_pid, mysql_passwd);
			goto END;
		}

		if (f_exists(mysql_passwd))
			unlink(mysql_passwd);

#ifndef TOMATO64
		if ((f_write_string(mysql_passwd, "use mysql;", FW_CREATE | FW_NEWLINE, 0600) < 0) ||
		    (f_write_string(mysql_passwd, sql, FW_APPEND | FW_NEWLINE, 0) < 0) ||
		    (f_write_string(mysql_passwd, "flush privileges;", FW_APPEND | FW_NEWLINE, 0) < 0)) {
#else
		/* SET PASSWORD operates at server scope, so start with a flush instead of `use mysql;`. */
		if ((f_write_string(mysql_passwd, "flush privileges;", FW_CREATE | FW_NEWLINE, 0600) < 0) ||
		    (f_write_string(mysql_passwd, sql, FW_APPEND | FW_NEWLINE, 0) < 0) ||
		    (f_write_string(mysql_passwd, "flush privileges;", FW_APPEND | FW_NEWLINE, 0) < 0)) {
#endif /* TOMATO64 */
			logerr(__FUNCTION__, __LINE__, mysql_passwd);
			killall_tk_period_wait("mysqld", 50);
			sleep(1);
			eval("rm", "-f", mysql_pid, mysql_passwd);
			goto END;
		}

		rc = snprintf(sql, sizeof(sql), "source %s", mysql_passwd);
		if ((rc < 0) || (rc >= (int)sizeof(sql))) {
			logmsg(LOG_ERR, "%s: mysql source command is too long", __FUNCTION__);
			killall_tk_period_wait("mysqld", 50);
			sleep(1);
			eval("rm", "-f", mysql_pid, mysql_passwd);
			goto END;
		}

		argv[1] = "--batch";
		argv[2] = "-e";
		argv[3] = sql;
		argv[4] = NULL;
		rc = mysql_eval_bin(pbi, "mysql", argv, ">>" mysql_log, 0);
		if (rc != 0) {
			logmsg(LOG_ERR, "%s: mysql password update returned %d", __FUNCTION__, rc);
			killall_tk_period_wait("mysqld", 50);
			sleep(1);
			eval("rm", "-f", mysql_pid, mysql_passwd);
			goto END;
		}

		f_write_string(mysql_log, "=========mysqladmin shutdown====================", FW_APPEND | FW_NEWLINE, 0);
		argv[1] = "-uroot";
		argv[2] = "--shutdown_timeout=3";
		argv[3] = "shutdown";
		argv[4] = NULL;
		rc = mysql_eval_bin_pwd(pbi, "mysqladmin", argv, ">>" mysql_log, nvram_safe_get("mysql_passwd"));
		if (rc != 0)
			logmsg(LOG_WARNING, "%s: mysqladmin shutdown returned %d", __FUNCTION__, rc);

		killall_tk_period_wait("mysqld", 50);
		sleep(1);
		eval("rm", "-f", mysql_pid, mysql_passwd);

		logmsg(LOG_DEBUG, "*** %s: root password successfully (re-)initialized", __FUNCTION__);
		nvram_set("mysql_init_rootpass", "0");
		nvram_commit();
	}

	f_write_string(mysql_log, "=========mysqld startup====================", FW_APPEND | FW_NEWLINE, 0);
	rc = snprintf(pid_arg, sizeof(pid_arg), "--pid-file=%s", mysql_pid);
	if ((rc < 0) || (rc >= (int)sizeof(pid_arg))) {
		logmsg(LOG_ERR, "%s: pid-file argument too long", __FUNCTION__);
		goto END;
	}

	argv[1] = pid_arg;
	argv[2] = NULL;
	rc = mysql_eval_bin(pbi, "mysqld", argv, ">>" mysql_log, 1);
	if (rc != 0) {
		logmsg(LOG_ERR, "%s: failed to start mysqld: %d", __FUNCTION__, rc);
		goto END;
	}

	if (anyhost == 1) {
		rc = mysql_wait_ready(pbi, nvram_safe_get("mysql_passwd"), mysql_ready_timeout);
		if (rc != 0) {
			logmsg(LOG_ERR, "%s: mysqld did not become ready: %d", __FUNCTION__, rc);
			goto END;
		}

		f_write_string(mysql_log, "=========mysql --execute allow-anyhost====================", FW_APPEND | FW_NEWLINE, 0);

		argv[1] = "-uroot";
		argv[2] = "--batch";
		argv[3] = "-e";
		argv[4] = "GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' WITH GRANT OPTION; flush privileges;";
		argv[5] = NULL;
		rc = mysql_eval_bin_pwd(pbi, "mysql", argv, ">>" mysql_log, nvram_safe_get("mysql_passwd"));
		if (rc != 0)
			logmsg(LOG_WARNING, "%s: mysql allow-anyhost update returned %d", __FUNCTION__, rc);

		eval("rm", "-f", mysql_anyhost);
	}

	mkdir_if_none(nginx_docroot);
	eval("cp", "-p", "/www/adminer.php", nginx_docroot);
	sleep(1);

END:
	if (pidof("mysqld") > 0) {
		logmsg(LOG_INFO, "mysqld started");
		setup_mysql_watchdog();
		eval("rm", "-f", mysql_child_pid);
	}
	else {
		logmsg(LOG_ERR, "starting mysqld failed - check configuration ...");
		eval("rm", "-f", mysql_child_pid);
		stop_mysql();
	}

	/* terminate the child */
	_exit(0);
}

void stop_mysql(void)
{
	pid_t pid;
	char pbi[128], buf[512];
	char *argv[8];
	int rc;
	int m = nvram_get_int("mysql_sleep") + 70;

	if (serialize_restart("mysqld", 0))
		return;

	if ((pid = pidof("mysqld")) > 0)
		logmsg(LOG_INFO, "terminating mysqld ...");
	else
		return;

	eval("cru", "d", "CheckMySQL");

	/* wait for child of start_mysql to finish (if any) */
	memset(buf, 0, sizeof(buf));
	while (f_read_string(mysql_child_pid, buf, sizeof(buf)) > 0 && atoi(buf) > 0 && ppid(atoi(buf)) > 0 && (m-- > 0)) {
		logmsg(LOG_DEBUG, "*** %s: waiting for child process of start_mysql to end, %d secs left ...", __FUNCTION__, m);
		sleep(1);
	}

	if (nvram_match("mysql_binary", "internal"))
		strlcpy(pbi, "/usr/bin", sizeof(pbi));
	else if (nvram_match("mysql_binary", "optware"))
		strlcpy(pbi, "/opt/bin", sizeof(pbi));
	else
		strlcpy(pbi, nvram_safe_get("mysql_binary_custom"), sizeof(pbi));

	mysql_chomp_trailing_slash(pbi);

	argv[1] = "-uroot";
	argv[2] = "--shutdown_timeout=3";
	argv[3] = "shutdown";
	argv[4] = NULL;
	rc = mysql_eval_bin_pwd(pbi, "mysqladmin", argv, NULL, nvram_safe_get("mysql_passwd"));
	if (rc != 0)
		logmsg(LOG_WARNING, "%s: mysqladmin shutdown returned %d", __FUNCTION__, rc);

	killall_and_waitfor("mysqld", 10, 80);

	if (pid > 0)
		logmsg(LOG_INFO, "mysqld stopped");

	/* clean-up */
	unlink(mysql_conf_link);
	eval("rm", "-f", mysql_pid);
	eval("rm", "-rf", mysql_etc_dir);
}
