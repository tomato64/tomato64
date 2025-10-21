/*
 *
 * Copyright (C) 2024 Lance Fredrickson
 * lancethepants@gmail.com
 *
*/


#include "rc.h"

void start_wifi(void)
{
	eval("start_wifi.sh");
}

void reload_wifi(void)
{
	/* Regenerate config and reload via ubus, or fall back to full restart if daemons not running */
	eval("start_wifi.sh", "reload");
}

void stop_wifi(void)
{
	killall_tk_period_wait("relayd", 50);
	killall_tk_period_wait("netifd", 50);
	killall_tk_period_wait("hostapd_cli", 50);
	killall_tk_period_wait("hostapd", 50);
	killall_tk_period_wait("wpa_supplicant", 50);
	killall_tk_period_wait("ubusd", 50);

	eval("rm", "-rf", "/etc/config");
}
