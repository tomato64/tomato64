################################################################################
#
# nginx-tomato
#
################################################################################

NGINX_TOMATO_VERSION = 1.24.0
NGINX_TOMATO_SOURCE = nginx-$(NGINX_TOMATO_VERSION).tar.gz
NGINX_TOMATO_SITE = https://nginx.org/download
NGINX_TOMATO_LICENSE = BSD-2-Clause
NGINX_TOMATO_LICENSE_FILES = LICENSE
NGINX_TOMATO_CPE_ID_VENDOR = f5
NGINX_TOMATO_DEPENDENCIES = \
	host-pkgconf \
	$(if $(BR2_PACKAGE_LIBXCRYPT),libxcrypt)

NGINX_TOMATO_CONF_OPTS = \
	--crossbuild=Linux::$(BR2_ARCH) \
	--with-cc="$(TARGET_CC)" \
	--with-cpp="$(TARGET_CC)" \
	--with-ld-opt="$(TARGET_LDFLAGS)"

# disable external libatomic_ops because its detection fails.
NGINX_TOMATO_CONF_ENV += \
	ngx_force_c_compiler=yes \
	ngx_force_c99_have_variadic_macros=yes \
	ngx_force_gcc_have_variadic_macros=yes \
	ngx_force_gcc_have_atomic=yes \
	ngx_force_have_epoll=yes \
	ngx_force_have_sendfile=yes \
	ngx_force_have_sendfile64=yes \
	ngx_force_have_pr_set_dumpable=yes \
	ngx_force_have_timer_event=yes \
	ngx_force_have_map_anon=yes \
	ngx_force_have_map_devzero=yes \
	ngx_force_have_sysvshm=yes \
	ngx_force_have_posix_sem=yes

# prefix: nginx root configuration location
NGINX_TOMATO_CONF_OPTS += \
	--force-endianness=$(call qstrip,$(call LOWERCASE,$(BR2_ENDIAN))) \
	--prefix=/usr \
	--sbin-path=/usr/sbin \
	--conf-path=/etc/nginx/nginx.conf \
	--sbin-path=/usr/sbin/nginx \
	--error-log-path=/tmp/var/log/nginx/error.log \
	--http-log-path=/tmp/var/log/nginx/access.log \
	--pid-path=/tmp/var/run/nginx.pid \
	--lock-path=/tmp/var/run/nginx.lock.accept \
	--http-client-body-temp-path=/tmp/var/lib/nginx/client \
	--http-fastcgi-temp-path=/tmp/var/lib/nginx/fastcgi \
	--http-uwsgi-temp-path=/tmp/var/lib/nginx/uwsgi \
	--http-scgi-temp-path=/tmp/var/lib/nginx/scgi \
	--http-proxy-temp-path=/tmp/var/lib/nginx/proxy \
	--with-ipv6

NGINX_TOMATO_CONF_OPTS += \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_FILE_AIO),--with-file-aio) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_THREADS),--with-threads)

ifeq ($(BR2_PACKAGE_LIBATOMIC_OPS),y)
NGINX_TOMATO_DEPENDENCIES += libatomic_ops
NGINX_TOMATO_CONF_OPTS += --with-libatomic
NGINX_TOMATO_CONF_ENV += ngx_force_have_libatomic=yes
ifeq ($(BR2_sparc_v8)$(BR2_sparc_leon3),y)
NGINX_TOMATO_CFLAGS += "-DAO_NO_SPARC_V9"
endif
else
NGINX_TOMATO_CONF_ENV += ngx_force_have_libatomic=no
endif

ifeq ($(BR2_PACKAGE_PCRE2),y)
NGINX_TOMATO_DEPENDENCIES += pcre2
NGINX_TOMATO_CONF_OPTS += --with-pcre
else
NGINX_TOMATO_CONF_OPTS += --without-pcre
endif

# modules disabled or not activated because of missing dependencies:
# - google_perftools  (googleperftools)
# - http_perl_module  (host-perl)
# - pcre-jit          (want to rebuild pcre)

# Notes:
# * Feature/module option are *not* symetric.
#   If a feature is on by default, only its --without-xxx option exists;
#   if a feature is off by default, only its --with-xxx option exists.
# * The configure script fails if unknown options are passed on the command
#   line.

# misc. modules
NGINX_TOMATO_CONF_OPTS += \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_SELECT_MODULE),--with-select_module,--without-select_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_POLL_MODULE),--with-poll_module,--without-poll_module)

ifneq ($(BR2_PACKAGE_NGINX_TOMATO_ADD_MODULES),)
NGINX_TOMATO_CONF_OPTS += \
	$(addprefix --add-module=,$(call qstrip,$(BR2_PACKAGE_NGINX_TOMATO_ADD_MODULES)))
endif

# http server modules
ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP),y)
ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_CACHE),y)
NGINX_TOMATO_DEPENDENCIES += openssl
else
NGINX_TOMATO_CONF_OPTS += --without-http-cache
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_V2_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += zlib
NGINX_TOMATO_CONF_OPTS += --with-http_v2_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_SSL_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += openssl
NGINX_TOMATO_CONF_OPTS += --with-http_ssl_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_XSLT_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += libxml2 libxslt
NGINX_TOMATO_CONF_OPTS += --with-http_xslt_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_IMAGE_FILTER_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += gd jpeg libpng
NGINX_TOMATO_CONF_OPTS += --with-http_image_filter_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_GEOIP_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += geoip
NGINX_TOMATO_CONF_OPTS += --with-http_geoip_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_GUNZIP_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += zlib
NGINX_TOMATO_CONF_OPTS += --with-http_gunzip_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_GZIP_STATIC_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += zlib
NGINX_TOMATO_CONF_OPTS += --with-http_gzip_static_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_SECURE_LINK_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += openssl
NGINX_TOMATO_CONF_OPTS += --with-http_secure_link_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_GZIP_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += zlib
else
NGINX_TOMATO_CONF_OPTS += --without-http_gzip_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_HTTP_REWRITE_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += pcre2
else
NGINX_TOMATO_CONF_OPTS += --without-http_rewrite_module
endif

NGINX_TOMATO_CONF_OPTS += \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_REALIP_MODULE),--with-http_realip_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_ADDITION_MODULE),--with-http_addition_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_SUB_MODULE),--with-http_sub_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_DAV_MODULE),--with-http_dav_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_FLV_MODULE),--with-http_flv_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_MP4_MODULE),--with-http_mp4_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_AUTH_REQUEST_MODULE),--with-http_auth_request_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_RANDOM_INDEX_MODULE),--with-http_random_index_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_DEGRADATION_MODULE),--with-http_degradation_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_SLICE_MODULE),--with-http_slice_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_STUB_STATUS_MODULE),--with-http_stub_status_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_CHARSET_MODULE),,--without-http_charset_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_SSI_MODULE),,--without-http_ssi_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_USERID_MODULE),,--without-http_userid_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_ACCESS_MODULE),,--without-http_access_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_AUTH_BASIC_MODULE),,--without-http_auth_basic_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_AUTOINDEX_MODULE),,--without-http_autoindex_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_GEO_MODULE),,--without-http_geo_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_MAP_MODULE),,--without-http_map_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_SPLIT_CLIENTS_MODULE),,--without-http_split_clients_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_REFERER_MODULE),,--without-http_referer_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_PROXY_MODULE),,--without-http_proxy_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_FASTCGI_MODULE),,--without-http_fastcgi_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_UWSGI_MODULE),,--without-http_uwsgi_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_SCGI_MODULE),,--without-http_scgi_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_MEMCACHED_MODULE),,--without-http_memcached_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_LIMIT_CONN_MODULE),,--without-http_limit_conn_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_LIMIT_REQ_MODULE),,--without-http_limit_req_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_EMPTY_GIF_MODULE),,--without-http_empty_gif_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_BROWSER_MODULE),,--without-http_browser_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_UPSTREAM_IP_HASH_MODULE),,--without-http_upstream_ip_hash_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_UPSTREAM_LEAST_CONN_MODULE),,--without-http_upstream_least_conn_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_UPSTREAM_RANDOM_MODULE),,--without-http_upstream_random_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_UPSTREAM_KEEPALIVE_MODULE),,--without-http_upstream_keepalive_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_HTTP_UPSTREAM_ZONE_MODULE),,--without-http_upstream_zone_module)

else # !BR2_PACKAGE_NGINX_TOMATO_HTTP
NGINX_TOMATO_CONF_OPTS += --without-http
endif # BR2_PACKAGE_NGINX_TOMATO_HTTP

# mail modules
ifeq ($(BR2_PACKAGE_NGINX_TOMATO_MAIL),y)
NGINX_TOMATO_CONF_OPTS += --with-mail

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_MAIL_SSL_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += openssl
NGINX_TOMATO_CONF_OPTS += --with-mail_ssl_module
endif

NGINX_TOMATO_CONF_OPTS += \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_MAIL_POP3_MODULE),,--without-mail_pop3_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_MAIL_IMAP_MODULE),,--without-mail_imap_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_MAIL_SMTP_MODULE),,--without-mail_smtp_module)

endif # BR2_PACKAGE_NGINX_TOMATO_MAIL

# stream modules
ifeq ($(BR2_PACKAGE_NGINX_TOMATO_STREAM),y)
NGINX_TOMATO_CONF_OPTS += --with-stream

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_STREAM_REALIP_MODULE),y)
NGINX_TOMATO_CONF_OPTS += --with-stream_realip_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_STREAM_SET_MODULE),)
NGINX_TOMATO_CONF_OPTS += --without-stream_set_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_STREAM_SSL_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += openssl
NGINX_TOMATO_CONF_OPTS += --with-stream_ssl_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_STREAM_GEOIP_MODULE),y)
NGINX_TOMATO_DEPENDENCIES += geoip
NGINX_TOMATO_CONF_OPTS += --with-stream_geoip_module
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_STREAM_SSL_PREREAD_MODULE),y)
NGINX_TOMATO_CONF_OPTS += --with-stream_ssl_preread_module
endif

NGINX_TOMATO_CONF_OPTS += \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_LIMIT_CONN_MODULE),,--without-stream_limit_conn_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_ACCESS_MODULE),,--without-stream_access_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_GEO_MODULE),,--without-stream_geo_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_MAP_MODULE),,--without-stream_map_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_SPLIT_CLIENTS_MODULE),,--without-stream_split_clients_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_RETURN_MODULE),,--without-stream_return_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_UPSTREAM_HASH_MODULE),,--without-stream_upstream_hash_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_UPSTREAM_LEAST_CONN_MODULE),,--without-stream_upstream_least_conn_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_UPSTREAM_RANDOM_MODULE),,--without-stream_upstream_random_module) \
	$(if $(BR2_PACKAGE_NGINX_TOMATO_STREAM_UPSTREAM_ZONE_MODULE),,--without-stream_upstream_zone_module)

endif # BR2_PACKAGE_NGINX_TOMATO_STREAM

# external modules
ifeq ($(BR2_PACKAGE_NGINX_TOMATO_UPLOAD),y)
NGINX_TOMATO_CONF_OPTS += $(addprefix --add-module=,$(NGINX_UPLOAD_DIR))
NGINX_TOMATO_DEPENDENCIES += nginx-upload
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_DAV_EXT),y)
NGINX_TOMATO_CONF_OPTS += --add-module=$(NGINX_DAV_EXT_DIR)
NGINX_TOMATO_DEPENDENCIES += nginx-dav-ext
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_NAXSI),y)
NGINX_TOMATO_DEPENDENCIES += nginx-naxsi
NGINX_TOMATO_CONF_OPTS += --add-module=$(NGINX_NAXSI_DIR)/naxsi_src
endif

ifeq ($(BR2_PACKAGE_NGINX_TOMATO_MODSECURITY),y)
NGINX_TOMATO_DEPENDENCIES += nginx-modsecurity
NGINX_TOMATO_CONF_OPTS += --add-module=$(NGINX_MODSECURITY_DIR)
endif

# Debug logging
NGINX_TOMATO_CONF_OPTS += $(if $(BR2_PACKAGE_NGINX_TOMATO_DEBUG),--with-debug)

define NGINX_TOMATO_DISABLE_WERROR
	$(SED) 's/-Werror//g' -i $(@D)/auto/cc/*
endef

NGINX_TOMATO_PRE_CONFIGURE_HOOKS += NGINX_TOMATO_DISABLE_WERROR

define NGINX_TOMATO_CONFIGURE_CMDS
	cd $(@D) ; $(NGINX_TOMATO_CONF_ENV) \
		PKG_CONFIG="$(PKG_CONFIG_HOST_BINARY)" \
		./configure $(NGINX_TOMATO_CONF_OPTS) \
			--with-cc-opt="$(TARGET_CFLAGS) $(NGINX_TOMATO_CFLAGS)"
endef

define NGINX_TOMATO_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define NGINX_TOMATO_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(NGINX_TOMATO_DIR)/objs/nginx $(TARGET_DIR)/usr/sbin
endef

$(eval $(generic-package))
