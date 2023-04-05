################################################################################
#
# netsnmp-tomato
#
################################################################################

NETSNMP_TOMATO_VERSION = 5.9.3
NETSNMP_TOMATO_SITE = https://downloads.sourceforge.net/project/net-snmp/net-snmp/$(NETSNMP_TOMATO_VERSION)
NETSNMP_TOMATO_SOURCE = net-snmp-$(NETSNMP_TOMATO_VERSION).tar.gz
NETSNMP_TOMATO_LICENSE = Various BSD-like
NETSNMP_TOMATO_LICENSE_FILES = COPYING
NETSNMP_TOMATO_CPE_ID_VENDOR = net-snmp
NETSNMP_TOMATO_CPE_ID_PRODUCT = $(NETSNMP_TOMATO_CPE_ID_VENDOR)
NETSNMP_TOMATO_SELINUX_MODULES = snmp
NETSNMP_TOMATO_CONF_ENV = \
	ac_cv_NETSNMP_CAN_USE_SYSCTL=no \
	ac_cv_path_PSPROG=/bin/ps
NETSNMP_TOMATO_CONF_OPTS = \
	--with-persistent-directory=/tmp/snmp-persist \
	--with-logfile=/var/log/snmpd.log \
	--disable-debugging \
	--disable-manuals \
	--disable-scripts \
	--disable-applications \
	--disable-privacy \
	--disable-ipv6 \
	--disable-mibs \
	--disable-mib-loading \
	--disable-embedded-perl \
	--with-perl-modules=no \
	--without-opaque-special-types \
	--without-openssl \
	--without-rsaref \
	--without-kmem-usage \
	--without-rpm \
	--with-out-transports=UDPIPv6,TCPIPv6,AAL5PVC,IPX,TCP,Unix \
	--with-out-mib-modules=snmpv3mibs,agent_mibs,agentx,notification,utilities,target \
	--with-mib-modules="\
		agent/extend,\
		host/hr_device,host/hr_disk,host/hr_filesys,host/hr_network,host/hr_partition,host/hr_print,host/hr_proc,host/hrSWRunTable,host/hr_storage,host/hr_system,\
		mibII/at,mibII/icmp,mibII/ifTable,mibII/ip,mibII/ipAddr,mibII/kernel_linux,mibII/snmp_mib,mibII/sysORTable,mibII/system_mib,mibII/tcp,mibII/udp,mibII/vacm_context,mibII/vacm_vars,mibII/var_route,\
		ucd-snmp/disk_hw,ucd-snmp/dlmod,ucd-snmp/extensible,ucd-snmp/loadave,ucd-snmp/logmatch,ucd-snmp/memory,ucd-snmp/pass,ucd-snmp/proc,ucd-snmp/proxy,ucd-snmp/vmstat,\
		util_funcs,if-mib/ifXTable,ip-mib/inetNetToMediaTable"\
	--with-default-snmp-version=2 \
	--with-sys-contact=root \
	--with-sys-location=Unknown \
	--enable-mini-agent \
	--enable-shared=no \
	--enable-static --with-gnu-ld \
	--enable-internal-md5 \
	--enable-mfd-rewrites \
	--with-defaults \
	--with-copy-persistent-files=no


NETSNMP_TOMATO_INSTALL_STAGING_OPTS = DESTDIR=$(STAGING_DIR) LIB_LDCONFIG_CMD=true install
NETSNMP_TOMATO_INSTALL_TARGET_OPTS = DESTDIR=$(TARGET_DIR) LIB_LDCONFIG_CMD=true install
NETSNMP_TOMATO_MAKE = $(MAKE1)
NETSNMP_TOMATO_CONFIG_SCRIPTS = net-snmp-config

ifeq ($(BR2_ENDIAN),"BIG")
NETSNMP_TOMATO_CONF_OPTS += --with-endianness=big
else
NETSNMP_TOMATO_CONF_OPTS += --with-endianness=little
endif

define NETSNMP_TOMATO_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/agent/snmpd $(TARGET_DIR)/usr/sbin
endef

$(eval $(autotools-package))
