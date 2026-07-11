################################################################################
#
# tor-override
#
# Tor's "make install" ships the geoip/geoip6 databases (~24MB) in
# /usr/share/tor. They are only consulted for country-based node
# selection in a custom torrc. FreshTomato installs only the tor
# binary, so do the same.
#
################################################################################

define TOR_REMOVE_GEOIP
	rm -rf $(TARGET_DIR)/usr/share/tor
endef
TOR_POST_INSTALL_TARGET_HOOKS += TOR_REMOVE_GEOIP
