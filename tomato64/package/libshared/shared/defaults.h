#ifndef __DEFAULTS_H__
#define __DEFAULTS_H__

typedef struct {
	const char *key;
	const char *value;
} defaults_t;

extern const defaults_t rstats_defaults[];
extern const defaults_t cstats_defaults[];
#ifdef TCONFIG_FTP
extern const defaults_t ftp_defaults[];
#endif /* TCONFIG_FTP */
#ifdef TCONFIG_SNMP
extern const defaults_t snmp_defaults[];
#endif /* TCONFIG_SNMP */
extern const defaults_t upnp_defaults[];

extern const defaults_t defaults[];
extern const defaults_t if_generic[];
extern const defaults_t if_vlan[];

#endif
