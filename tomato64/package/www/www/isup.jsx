//	<% etherstates(); %>

//	<% ddnsx(); %>

//	<% jsdefaults(); %>

if (typeof stats === 'undefined' || stats.length == 0) stats = { };
stats.dns = [<% dns(); %>];

isup = {};

isup.notice_iptables = '<% notice("iptables"); %>';
isup.notice_ip6tables = '<% notice("ip6tables"); %>';
isup.notice_cifs = '<% notice("cifs"); %>';
isup.notice_jffs = '<% notice("jffs"); %>';
/* JFFS2-BEGIN */
//	<% statfs("/jffs", "jffs2"); %>
/* JFFS2-END */
/* JFFS2NAND-BEGIN */
//	<% statfs("/jffs", "brcmnand"); %>
/* JFFS2NAND-END */
/* CIFS-BEGIN */
//	<% statfs("/cifs1", "cifs1"); %>
//	<% statfs("/cifs2", "cifs2"); %>
/* CIFS-END */

isup.time = '<% time(); %>';
isup.ntp = '<% ntp(); %>';

isup.telnetd = parseInt('<% psup("telnetd"); %>');

isup.miniupnpd = parseInt('<% psup("miniupnpd"); %>');

isup.dnsmasq = parseInt('<% psup("dnsmasq"); %>');

/* it should be done in a different way, but for now it's ok */
isup.qos = <% nv("qos_enable"); %>;
isup.bwl = <% nv("bwl_enable"); %>;

/* OPENVPN-BEGIN */
/* BCMARM-BEGIN */
isup.vpnclient3 = parseInt('<% psup("vpnclient3"); %>');
/* BCMARM-END */
/* BCMARM-NO-BEGIN */
/* BCMARM-NO-END */
isup.vpnclient1 = parseInt('<% psup("vpnclient1"); %>');
isup.vpnclient2 = parseInt('<% psup("vpnclient2"); %>');

isup.vpnserver1 = parseInt('<% psup("vpnserver1"); %>');
isup.vpnserver2 = parseInt('<% psup("vpnserver2"); %>');
/* TOMATO64-BEGIN */
isup.vpnserver3 = parseInt('<% psup("vpnserver3"); %>');
isup.vpnserver4 = parseInt('<% psup("vpnserver4"); %>');
/* TOMATO64-END */
/* OPENVPN-END */

/* PPTPD-BEGIN */
isup.pptpclient = parseInt('<% psup("pptpclient"); %>');
isup.pptpd = parseInt('<% psup("pptpd"); %>');
/* PPTPD-END */

/* WIREGUARD-BEGIN */
isup.wireguard0 = parseInt('<% wgstat("wg0"); %>');
isup.wireguard1 = parseInt('<% wgstat("wg1"); %>');
isup.wireguard2 = parseInt('<% wgstat("wg2"); %>');
/* WIREGUARD-END */

/* NGINX-BEGIN */
isup.nginx = parseInt('<% psup("nginx"); %>');
isup.mysqld = parseInt('<% psup("mysqld"); %>');
/* NGINX-END */

isup.dropbear = parseInt('<% psup("dropbear"); %>');

/* MEDIA-SRV-BEGIN */
isup.minidlna = parseInt('<% psup("minidlna"); %>');
/* MEDIA-SRV-END */

/* TINC-BEGIN */
isup.tincd = parseInt('<% psup("tincd"); %>');
/* TINC-END */

/* BT-BEGIN */
isup.transmission = parseInt('<% psup("transmission-da"); %>');
/* BT-END */

/* SAMBA-BEGIN */
isup.samba = parseInt('<% psup("smbd"); %>');
/* SAMBA-END */

/* FTP-BEGIN */
isup.ftpd = parseInt('<% psup("vsftpd"); %>');
/* FTP-END */

/* TOR-BEGIN */
isup.tor = parseInt('<% psup("tor"); %>');
/* TOR-END */
