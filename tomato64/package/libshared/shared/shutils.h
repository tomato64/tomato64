/*
 * Shell-like utility functions
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: shutils.h,v 1.8 2005/03/07 08:35:32 kanki Exp $
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#ifndef _shutils_h_
#define _shutils_h_
#include <shared.h>

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)
#define sin6_addr(s) (((struct sockaddr_in6 *)(s))->sin6_addr)
#ifndef MAX_NVPARSE
 #define MAX_NVPARSE 255
#endif
#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
 #define CPU0	"0"
 #define CPU1	"1"
#endif /* TCONFIG_BCMARM && TCONFIG_BCMSMP */


extern int _eval(char *const argv[], const char *path, int timeout, pid_t *ppid);
extern int eval_cmdline(const char *cmd, const char *path, int timeout, int *ppid);

extern size_t safe_fread(const void *ptr, size_t size, size_t nmemb, FILE *stream);
extern size_t safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

extern int ether_atoe(const char *a, unsigned char *e);
extern char *ether_etoa(const unsigned char *e, char *a);

#if defined(TCONFIG_BCMARM) && defined(TCONFIG_BCMSMP)
extern int _cpu_eval(int *ppid, char *cmds[]);

/* another _cpu_eval form */
#define cpu_eval(ppid, cmd, args...) ({ \
	char * argv[] = { cmd, ## args, NULL }; \
	_cpu_eval(ppid, argv); \
})
#endif /* TCONFIG_BCMARM && TCONFIG_BCMSMP */

/*
 * Concatenate two strings together into a caller supplied buffer
 * @param	s1	first string
 * @param	s2	second string
 * @param	buf	buffer large enough to hold both strings
 * @return	buf
 */
static inline char *strlcat_r(const char *s1, const char *s2, char *buf, const size_t buf_len)
{
	strlcpy(buf, s1, buf_len);
	strlcat(buf, s2, buf_len);
	return buf;
}

/* check for a blank character; that is, a space or a tab */
#ifndef is_blank
 #define is_blank(c) (c == ' ' || c == '\t')
#endif

/* strip trailing CR/NL from string <s> */
#define chomp(s) ({ \
	char *c = (s) + strlen((s)) - 1; \
	while ((c > (s)) && (*c == '\n' || *c == '\r' || *c == ' ')) \
		*c-- = '\0'; \
	s; \
})

/* simple version of _eval() (no timeout and wait for child termination) */
#define eval(cmd, args...) ({ \
	char * const argv[] = { cmd, ## args, NULL }; \
	_eval(argv, NULL, 0, NULL); \
})

/* copy each token in wordlist delimited by space into word */
#define foreach(word, wordlist, next) \
	for (next = &(wordlist)[strspn((wordlist), " ")], \
	     strlcpy((word), next, sizeof(word)), \
	     (word)[strcspn((word), " ")] = '\0', \
	     next = strchr(next, ' '); \
	     (word)[0] != '\0'; \
	     next = next ? &next[strspn(next, " ")] : "", \
	     strlcpy((word), next, sizeof(word)), \
	     (word)[strcspn((word), " ")] = '\0', \
	     next = strchr(next, ' '))

/* return NUL instead of NULL if undefined */
#define safe_getenv(s) (getenv(s) ? : "")

extern int get_ifname_unit(const char* ifname, int *unit, int *subunit);

extern char *find_in_list(const char *haystack, const char *needle);
extern int remove_from_list(const char *name, char *list, int listsize);
extern int add_to_list(const char *name, char *list, int listsize);
extern char *remove_dups(char *inlist, int inlist_size);
//extern char *find_smallest_in_list(char *haystack);
extern char *sort_list(char *inlist, int inlist_size);

extern int nvifname_to_osifname(const char *nvifname, char *osifname_buf, int osifname_buf_len);
extern int osifname_to_nvifname(const char *osifname, char *nvifname_buf, int nvifname_buf_len);

extern void killall_tk_period_wait(const char *name, int wait_ds);
extern void killall_and_waitfor(const char *name, int loop, int killtime);
extern int kill_pidfile_s(char *pidfile, int sig);

extern int _vstrsep(char *buf, const char *sep, ...);
#define vstrsep(buf, sep, args...) _vstrsep(buf, sep, args, NULL)

extern void dbg(const char * format, ...);
#define dbG(fmt, args...) dbg("%s(0x%04x): " fmt , __FUNCTION__ , __LINE__, ## args)
extern void cprintf(const char *format, ...);

// see wlutils.h
//#ifdef CONFIG_BCMWL5
//extern char *wl_ether_etoa(const struct ether_addr *n);
//#endif

// see shared.h
//#if defined(TCONFIG_BLINK) || defined(TCONFIG_BCMARM)
//extern int getMTD(const char *name);
//#endif

// see shared.h
//#ifdef TCONFIG_BCMBSD
//extern pid_t get_pid_by_name(const char *name);
//#endif

/* ============================ UNUSED ============================ */

#if 0
extern char *fd2str(int fd);
extern char *file2str(const char *path);
extern int waitfor(int fd, int timeout); /* see rc/init.c */
extern int kill_pidfile_s_rm(char *pidfile, int sig);
extern int get_ipconfig_index(char *eth_ifname);
extern int set_ipconfig_index(char *eth_ifname, int index);
extern char *get_bridged_interfaces(char *bridge_name);
extern int doSystem(char *fmt, ...);
extern void dbgprintf(const char * format, ...);
extern int ure_any_enabled(void);
extern char *_backtick(char *const argv[]);
extern int fmtAlloc(char **s, int n, char *fmt, ...);
extern int fmtValloc(char **s, int n, char *fmt, va_list arg);
extern int swap_check();
#endif /* 0 */

#endif /* _shutils_h_ */
