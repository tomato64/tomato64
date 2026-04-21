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

/* it should be done in a different way, but for now it's ok */
isup.qos = <% nv("qos_enable"); %>;
isup.bwl = <% nv("bwl_enable"); %>;

//	<% psup("all"); %>
