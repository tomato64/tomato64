/*
 * nginx.c
 *
 * Copyright (C) 2013 NGinX for Tomato RAF
 *  ***** Ofer Chen, roadkill AT tomatoraf DOT com
 *  ***** Vicente Soriano, victek AT tomatoraf DOT com
 *
 * No part of this program can be used out of Tomato Firmware without owners permission.
 * This code generates the configurations files for NGINX. You can see these files in /etc/nginx/
 *
 * Fixes/updates (C) 2018 - 2022 pedro
 *
 */


#include "rc.h"

#define nginxbin			"nginx"				/* process name */
#define nginxname			"freshtomato.local"		/* server name */
#define nginxlogdir			"/var/log/nginx"		/* log directory */
#define nginxlibdir			"/var/lib/nginx"		/* lib directory */
#define nginxdir			"/etc/nginx"			/* directory to write config files */
#define nginxdocrootdir			"/www"				/* document root */
#define nginxrundir			"/var/run"			/* run directory */

#define nginxconf			nginxdir"/nginx.conf"		/* config file */
#define fastcgiconf			nginxdir"/fastcgi.conf"		/* fastcgi config file */
#define mimetypes			nginxdir"/mime.types"		/* mime.types config */
#define client_body_temp_path		nginxlibdir"/client"		/* temp path needed to execute nginx */
#define fastcgi_temp_path		nginxlibdir"/fastcgi"		/* temp path needed to execute nginx fastcgi */
#define uwsgi_temp_path			nginxlibdir"/uwsgi"		/* temp path needed to execute nginx */
#define scgi_temp_path			nginxlibdir"/scgi"		/* temp path needed to execute nginx */
#define nginxaccesslog			nginxlogdir"/access.log"	/* access log */
#define nginxerrorlog			nginxlogdir"/error.log"		/* error log */
#define nginxcustom			"#"				/* additional window for custom parameter */
#define nginx_worker_proc		"1"				/* worker processes. CPU, cores */
#define nginx_cpu_affinity		"0101"				/* Can bind the worker process to a CPU, it calls sched_setaffinity() */
#define nginx_master_process		"off"				/* set to "on" in developpment mode only */
#define nginxpid			nginxrundir"/nginx.pid"		/* pid */
#define nginx_worker_rlimit_profile	"8192"				/* worker rlimit profile */
#define nginx_worker_connections	"512"				/* worker_proc*512/keepalive_timeout*60 = 512 users per minute */
#define nginssendfile			"on"				/* sendfile */
//#define nginx_multi_accept		"on"				/* specifies multiple connections for one core CPU */
//#define nginx_keepalive_timeout	"60"				/* the server will close connections after this time */
//#define nginxtcp_nopush		"on"				/* tcp_nopush */
//#define nginxserver_hash_bucket_size	"128"				/* server names hash bucket size */
#define nginx_fw_script			nginxdir"/nginx-fw.sh"
#define nginx_fw_del_script		nginxdir"/nginx-clear-fw-tmp.sh"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG	"nginx_debug"

FILE * nginx_conf_file;
FILE * fastcgi_conf_file;
FILE * mimetypes_file;
FILE * phpini_file;
unsigned int fastpath = 0;


static void build_fastcgi_conf(void)
{
	/* Starting a fastcgi configuration file */
	mkdir_if_none(nginxdir);
	if ((fastcgi_conf_file = fopen(fastcgiconf, "w")) == NULL) {
		return;
	}

	fprintf(fastcgi_conf_file, "fastcgi_param SCRIPT_FILENAME\t\t$document_root$fastcgi_script_name;\n"
	                           "fastcgi_param QUERY_STRING\t\t$query_string;\n"
	                           "fastcgi_param REQUEST_METHOD\t\t$request_method;\n"
	                           "fastcgi_param CONTENT_TYPE\t\t$content_type;\n"
	                           "fastcgi_param CONTENT_LENGTH\t\t$content_length;\n"
	                           "fastcgi_param SCRIPT_NAME\t\t$fastcgi_script_name;\n"
	                           "fastcgi_param REQUEST_URI\t\t$request_uri;\n"
	                           "fastcgi_param DOCUMENT_URI\t\t$document_uri;\n"
	                           "fastcgi_param DOCUMENT_ROOT\t\t$document_root;\n"
	                           "fastcgi_param SERVER_PROTOCOL\t\t$server_protocol;\n"
	                           "fastcgi_param GATEWAY_INTERFACE\t\tCGI/1.1;\n"
	                           "fastcgi_param SERVER_SOFTWARE\t\tnginx/$nginx_version;\n"
	                           "fastcgi_param REMOTE_ADDR\t\t$remote_addr;\n"
	                           "fastcgi_param REMOTE_PORT\t\t$remote_port;\n"
	                           "fastcgi_param SERVER_ADDR\t\t$server_addr;\n"
	                           "fastcgi_param SERVER_PORT\t\t$server_port;\n"
	                           "fastcgi_param SERVER_NAME\t\t$server_name;\n"
	                           "fastcgi_index index.php;\n"
	                           "fastcgi_param REDIRECT_STATUS\t\t200;\n");

	fclose(fastcgi_conf_file);
}

static void build_mime_types(void)
{
	/* Starting the mime.types configuration file */
	mkdir_if_none(nginxdir);
	if ((mimetypes_file = fopen(mimetypes, "w")) == NULL) {
		return;
	}

	fprintf(mimetypes_file, "types {\n"
	                        "text/html\t\t\t\thtml htm shtml;\n"
	                        "text/css\t\t\t\tcss;\n"
	                        "text/xml\t\t\t\txml rss;\n"
	                        "image/gif\t\t\t\tgif;\n"
	                        "image/jpeg\t\t\t\tjpeg jpg;\n"
	                        "application/x-javascript\t\tjs;\n"
	                        "text/plain\t\t\t\ttxt;\n"
	                        "text/x-component\t\t\thtc;\n"
	                        "text/mathml\t\t\t\tmml;\n"
	                        "image/png\t\t\t\tpng;\n"
	                        "image/x-icon\t\t\t\tico;\n"
	                        "image/x-jng\t\t\t\tjng;\n"
	                        "image/vnd.wap.wbmp\t\t\twbmp;\n"
	                        "image/svg+xml svg svgz;\n"
	                        "application/java-archive\t\tjar war ear;\n"
	                        "application/mac-binhex40\t\thqx;\n"
	                        "application/pdf\t\t\t\tpdf;\n"
	                        "application/x-cocoa\t\t\tcco;\n"
	                        "application/x-java-archive-diff\t\tjardiff;\n"
	                        "application/x-java-jnlp-file\t\tjnlp;\n"
	                        "application/x-makeself\t\t\trun;\n"
	                        "application/x-perl\t\t\tpl pm;\n"
	                        "application/x-pilot\t\t\tprc pdb;\n"
	                        "application/x-rar-compressed\t\trar;\n"
	                        "application/x-redhat-package-manager\trpm;\n"
	                        "application/x-sea\t\t\tsea;\n"
	                        "application/x-shockwave-flash\t\tswf;\n"
	                        "application/x-stuffit\t\t\tsit;\n"
	                        "application/x-tcl\t\t\ttcl tk;\n"
	                        "application/x-x509-ca-cert\t\tder pem crt;\n"
	                        "application/x-xpinstall\t\t\txpi;\n"
	                        "application/zip\t\t\t\tzip;\n"
	                        "application/octet-stream\t\tdeb;\n"
	                        "application/octet-stream\t\tbin exe dll;\n"
	                        "application/octet-stream\t\tdmg;\n"
	                        "application/octet-stream\t\teot;\n"
	                        "application/octet-stream\t\tiso img;\n"
	                        "application/octet-stream\t\tmsi msp msm;\n"
	                        "audio/mpeg\t\t\t\tmp3;\n"
	                        "audio/x-realaudio\t\t\tra;\n"
	                        "video/mpeg\t\t\t\tmpeg mpg;\n"
	                        "video/quicktime\t\t\t\tmov;\n"
	                        "video/x-flv\t\t\t\tflv;\n"
	                        "video/x-msvideo\t\t\t\tavi;\n"
	                        "video/x-ms-wmv\t\t\t\twmv;\n"
	                        "video/x-ms-asf\t\t\t\tasx asf;\n"
	                        "video/x-mng\t\t\t\tmng;\n"
	                        "}\n");

	fclose(mimetypes_file);
}

static void build_nginx_conf(void)
{
	char *buf;	/* default param buffer */
	int i;		/* integer cast */

	/* Starting the nginx configuration file */
	mkdir_if_none(nginxdir);
	if ((nginx_conf_file = fopen(nginxconf, "w")) == NULL) {
		logerr(__FUNCTION__, __LINE__, nginxconf);
		return;
	}

	i = nvram_get_int("nginx_priority");
	if ((i <= -20) || (i >= 19))
		i = 10; /* min = Max Performance and max= Min Performance value for worker_priority */

	if ((buf = nvram_safe_get("nginx_httpcustom")) == NULL)
		buf = nginxcustom; /* shibby - add custom config to http section */

	fprintf(nginx_conf_file, /* global process */
	                         "user %s;\n"
	                         "worker_processes %s;\n"
	                         "worker_cpu_affinity %s;\n"
	                         "master_process %s;\n"
	                         "worker_priority %d;\n"
	                         "error_log %s;\n"
	                         "pid %s;\n"
	                         "worker_rlimit_nofile %s;\n"
	                         /* events */
	                         "events {\n"
	                         "worker_connections %s;\n"
	                         //"multi_accept %s;\n"
	                         "}\n"
	                         /* http */
	                         "http {\n"
	                         "include %s;\n"
	                         "include %s;\n"
	                         "default_type application/octet-stream;\n"
	                         "log_format main '$remote_addr - $remote_user [$time_local] $status '\n"
	                         "'\"$request\" $body_bytes_sent \"$http_referer\" '\n"
	                         "'\"$http_user_agent\" \"$http_x_forwarded_for\"';\n"
	                         "sendfile %s;\n"
	                         "client_max_body_size %sM;\n"
	                         "%s\n",
	                         nvram_safe_get("nginx_user"),
	                         nginx_worker_proc,
	                         nginx_cpu_affinity,
	                         nginx_master_process,
	                         i,
	                         nginxerrorlog,
	                         nginxpid,
	                         nginx_worker_rlimit_profile,
	                         nginx_worker_connections,
	                         //nginx_multi_accept,
	                         mimetypes,
	                         fastcgiconf,
	                         nginssendfile,
	                         nvram_safe_get("nginx_upload"),
	                         buf);

/*
	fprintf(nginx_conf_file, "keepalive_timeout\t%s;\n"
	                         "tcp_nopush\t%s;\n"
	                         "server_names_hash_bucket_size\t%s;\n"
	                         "limit_req_zone $binary_remote_addr zone=one:10m rate=1r/s;\n",
	                         nginx_keepalive_timeout,
	                         nginxtcp_nopush,
	                         nginxserver_hash_bucket_size);
*/

	/* Basic Server Parameters */
	i = nvram_get_int("nginx_port");
	if ((i <= 0) || (i >= 0xFFFF))
		i = 85; /* 0xFFFF 65535 */

	if ((buf = nvram_safe_get("nginx_fqdn")) == NULL)
		buf = nginxname;

	fprintf(nginx_conf_file, "server {\n"
	                         "listen %d;\n"
	                         "server_name %s;\n"
	                         "access_log %s main;\n"
	                         "location / {\n",
	                         i,
	                         buf,
	                         nginxaccesslog);

	if ((buf = nvram_safe_get("nginx_docroot")) == NULL)
		buf = nginxdocrootdir;

	fprintf(nginx_conf_file, "root %s;\n"
	                         "index index.html index.htm index.php %s;\n"
	                         /* error pages section */
	                         "error_page 404 /404.html;\n"
	                         "error_page 500 502 503 504 /50x.html;\n"
	                         "location /50x.html {\n"
	                         "root %s;\n"
	                         "}\n",
	                         buf,
	                         nvram_get_int("nginx_h5aisupport") ? "/_h5ai/public/index.php" : "",
	                         buf);

	/* PHP to FastCGI Server */
	if (nvram_get_int("nginx_php"))
		fprintf(nginx_conf_file, "location ~ ^(?<script_name>.+?\\.php)(?<path_info>/.*)?$ {\n"
		                         "try_files $script_name = 404;\n"
		                         "include %s;\n"
		                         "fastcgi_param PATH_INFO $path_info;\n"
		                         "fastcgi_pass 127.0.0.1:9000;\n"
		                         "}\n",
		                         fastcgiconf);

	/* server for static files */
	fprintf(nginx_conf_file, "location ~ ^/(images|javascript|js|css|flash|media|static)/ {\n"
	                         "root %s;\n"
	                         "expires 10d;\n"
	                         "}\n"
	                         "}\n",
	                         buf);

	/* add custom config to server section */
	if ((buf = nvram_safe_get("nginx_servercustom")) == NULL)
		buf = nginxcustom;

	fprintf(nginx_conf_file, "%s"
	                         "\n"
	                         "}\n"
	                         "}\n"
	                         "\n",
	                         buf);

	if ((buf = nvram_safe_get("nginx_custom")) == NULL)
		buf = nginxcustom;

	fprintf(nginx_conf_file, "%s", buf);
	fclose(nginx_conf_file);

	logmsg(LOG_INFO, "nginx - config file built succesfully");

	if (nvram_get_int("nginx_php")) {
		if (!(phpini_file = fopen("/etc/php.ini", "w"))) {
			logerr(__FUNCTION__, __LINE__, "/etc/php.ini");
			return;
		}
		fprintf(phpini_file, "post_max_size = %sM\n"
		                     "upload_max_filesize = %sM\n"
		                     "mysqli.default_port = 3306\n"
		                     "mysqli.default_socket = %s/mysqld.sock\n"
		                     "mysqli.default_host = localhost\n"
		                     "%s\n",
		                     nvram_safe_get("nginx_upload"),
		                     nvram_safe_get("nginx_upload"),
		                     nginxrundir,
		                     nvram_safe_get("nginx_phpconf"));

		fclose(phpini_file);
	}
}

static void build_nginx_firewall(void)
{
	FILE *p;

	if ((!nvram_get_int("nginx_remote")) || (!nvram_get_int("nginx_port")))
		return;

	chains_log_detection();

	/* Create firewall script */
	if (!(p = fopen(nginx_fw_script, "w"))) {
		logerr(__FUNCTION__, __LINE__, nginx_fw_script);
		return;
	}

	fprintf(p, "#!/bin/sh\n"
	           "iptables -A INPUT -p tcp --dport %s -j %s\n",
	           nvram_safe_get("nginx_port"), chain_in_accept);

	fclose(p);
	chmod(nginx_fw_script, 0744);
}

/* start the nginx module according environment directives */
void start_nginx(int force)
{
	int ret;

	/* only if enabled or forced */
	if (!nvram_get_int("nginx_enable") && force == 0)
		return;

	if (serialize_restart("nginx", 1))
		return;

	if (!f_exists(fastcgiconf))
		build_fastcgi_conf();

	if (!f_exists(mimetypes))
		build_mime_types();

	if (!nvram_get_int("nginx_keepconf"))
		build_nginx_conf();
	else {
		if (!f_exists(nginxconf))
			build_nginx_conf();
	}

	/* create directories before starting daemon */
	mkdir_if_none(nginxlogdir);
	mkdir_if_none(client_body_temp_path);
	mkdir_if_none(fastcgi_temp_path);
	mkdir_if_none(uwsgi_temp_path);
	mkdir_if_none(scgi_temp_path);

	build_nginx_firewall();

	run_nginx_firewall_script();

	if (nvram_get_int("nginx_php")) /* run spawn-fcgi */
		xstart("spawn-fcgi", "-a", "127.0.0.1", "-p", "9000", "-P", nginxrundir"/php-fastcgi.pid", "-C", "2", "-u", nvram_safe_get("nginx_user"), "-g", nvram_safe_get("nginx_user"), "/usr/sbin/php-cgi");

	ret = eval(nginxbin, "-c", nvram_get_int("nginx_override") ? nvram_safe_get("nginx_overridefile") : nginxconf);

	if (ret) {
		logmsg(LOG_ERR, "starting nginx failed - check configuration ...");
		stop_nginx();
	}
	else
		logmsg(LOG_INFO, "nginx is started");
}

/* stop nginx and remove traces of the process */
void stop_nginx(void)
{
	if (serialize_restart("nginx", 0))
		return;

	if (pidof(nginxbin) > 0) {
		killall_tk_period_wait(nginxbin, 50);
		killall_tk_period_wait("php-cgi", 50);

		logmsg(LOG_INFO, "nginx is stopped");
	}

	run_del_firewall_script(nginx_fw_script, nginx_fw_del_script);

	if (!nvram_get_int("nginx_keepconf"))
		system("/bin/rm -rf "nginxdir);
	if (f_exists(nginxpid))
		unlink(nginxpid);
}

void run_nginx_firewall_script(void)
{
	FILE *fp;

	/* first remove existing firewall rule(s) */
	run_del_firewall_script(nginx_fw_script, nginx_fw_del_script);

	/* then (re-)add firewall rule(s) */
	if ((fp = fopen(nginx_fw_script, "r"))) {
		fclose(fp);
		logmsg(LOG_DEBUG, "*** %s: running firewall script: %s", __FUNCTION__, nginx_fw_script);
		eval(nginx_fw_script);
	}
}
