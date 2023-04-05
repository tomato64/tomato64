################################################################################
#
# stubby
#
################################################################################

STUBBY_VERSION = 1.7.3
STUBBY_SOURCE = getdns-$(STUBBY_VERSION).tar.gz
STUBBY_SITE = https://getdnsapi.net/releases/getdns-1-7-3
STUBBY_LICENSE = nlet
STUBBY_DEPENDENCIES = libyaml openssl

STUBBY_CONF_OPTS =	-DENABLE_STATIC=TRUE \
			-DENABLE_SHARED=FALSE \
			-DENABLE_GOST=FALSE \
			-DBUILD_GETDNS_QUERY=FALSE \
			-DBUILD_GETDNS_SERVER_MON=FALSE \
			-DBUILD_STUBBY=TRUE \
			-DENABLE_STUB_ONLY=TRUE \
			-DBUILD_LIBEV=FALSE \
			-DBUILD_LIBEVENT2=FALSE \
			-DBUILD_LIBUV=FALSE \
			-DBUILD_TESTING=FALSE \
			-DCMAKE_DISABLE_FIND_PACKAGE_Libsystemd=TRUE \
			-DUSE_LIBIDN2=FALSE \
			-DFORCE_COMPAT_STRPTIME=TRUE

$(eval $(cmake-package))
