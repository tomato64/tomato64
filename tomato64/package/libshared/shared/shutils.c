/*
 * Shell-like utility functions
 *
 * Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: shutils.c 337155 2012-06-06 12:17:08Z $
 *
 * Fixes/updates (C) 2018 - 2026 pedro
 * https://freshtomato.org/
 *
 */


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <syslog.h>
#include <typedefs.h>
#include <wlioctl.h>

#include <bcmnvram.h>
#include <shutils.h>

/* Linux specific headers */
#ifdef linux
#ifndef TOMATO64
#include <error.h>
#endif /* TOMATO64 */
#include <termios.h>
#include <sys/time.h>
//#include <net/ethernet.h>
#else
#include <proto/ethernet.h>
#endif /* linux */

#include "shared.h"

/* needed by logmsg() */
#define LOGMSG_DISABLE	DISABLE_SYSLOG_OS
#define LOGMSG_NVDEBUG	"shutils_debug"

#define T(x)		__TXT(x)
#define __TXT(s)	L ## s

#ifndef B_L
#define B_L		T(__FILE__),__LINE__
#define B_ARGS_DEC	char *file, int line
#define B_ARGS		file, line
#endif /* B_L */

#define bfree(B_ARGS, p) free(p)
#define balloc(B_ARGS, num) malloc(num)
#define brealloc(B_ARGS, p, num) realloc(p, num)

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif /* max */

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif /* min */

#define STR_REALLOC		0x1				/* Reallocate the buffer as required */
#define STR_INC			64				/* Growth increment */

typedef struct {
	char		*s;						/* Pointer to buffer */
	int		size;						/* Current buffer size */
	int		max;						/* Maximum buffer size */
	int		count;						/* Buffer count */
	int		flags;						/* Allocation flags */
} strbuf_t;

/*
 *	Sprintf formatting flags
 */
enum flag {
	flag_none = 0,
	flag_minus = 1,
	flag_plus = 2,
	flag_space = 4,
	flag_hash = 8,
	flag_zero = 16,
	flag_short = 32,
	flag_long = 64
};

/*
 * Print out message on console.
 */
void dbgprintf (const char * format, ...)
{
	FILE *f;
	int nfd;
	va_list args;

	if((nfd = open("/dev/console", O_WRONLY | O_NONBLOCK)) > 0){
		if((f = fdopen(nfd, "w")) != NULL){
			va_start(args, format);
			vfprintf(f, format, args);
			va_end(args);
			fclose(f);
		}
		close(nfd);
	}
}

void dbg(const char * format, ...)
{
	FILE *f;
	int nfd;
	va_list args;

	if (((nfd = open("/dev/console", O_WRONLY | O_NONBLOCK)) > 0) &&
	    (f = fdopen(nfd, "w")))
	{
		va_start(args, format);
		vfprintf(f, format, args);
		va_end(args);
		fclose(f);
	}
	else
	{
		va_start(args, format);
		vfprintf(stderr, format, args);
		va_end(args);
	}

	if (nfd != -1) close(nfd);
}

/*
 * Reads file and returns contents
 * @param	fd	file descriptor
 * @return	contents of file or NULL if an error occurred
 */
char *
fd2str(int fd)
{
	char *buf = NULL;
	size_t count = 0, n;

	do {
		buf = realloc(buf, count + 512);
		n = read(fd, buf + count, 512);
		if (n < 0) {
			free(buf);
			buf = NULL;
		}
		count += n;
	} while (n == 512);

	close(fd);
	if (buf)
		buf[count] = '\0';
	return buf;
}

/*
 * Reads file and returns contents
 * @param	path	path to file
 * @return	contents of file or NULL if an error occurred
 */
char *
file2str(const char *path)
{
	int fd;

	if ((fd = open(path, O_RDONLY)) == -1) {
		logerr(__FUNCTION__, __LINE__, path);
		return NULL;
	}

	return fd2str(fd);
}

/*
 * Waits for a file descriptor to change status or unblocked signal
 * @param	fd	file descriptor
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @return	1 if descriptor changed status or 0 if timed out or -1 on error
 */
int
waitfor(int fd, int timeout)
{
	fd_set rfds;
	struct timeval tv = { timeout, 0 };

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
}

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @param	path	NULL, ">output", or ">>output"
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @param	ppid	NULL to wait for child termination or pointer to pid
 * @return	return value of executed command or errno
 *
 * Ref: http://www.open-std.org/jtc1/sc22/WG15/docs/rr/9945-2/9945-2-28.html
 */
int _eval(char *const argv[], const char *path, int timeout, int *ppid)
{
	sigset_t set, sigmask;
	sighandler_t chld = SIG_IGN;
	pid_t pid, w;
	int status = 0;
	int fd;
	int flags;
	int sig;
	int n;
	const char *p;
	char s[256];
	//char *cpu0_argv[32] = { "taskset", "-c", "0"};
	//char *cpu1_argv[32] = { "taskset", "-c", "1"};

	if (!ppid) {
		// block SIGCHLD
		sigemptyset(&set);
		sigaddset(&set, SIGCHLD);
		sigprocmask(SIG_BLOCK, &set, &sigmask);
		// without this we cannot rely on waitpid() to tell what happened to our children
		chld = signal(SIGCHLD, SIG_DFL);
	}

	pid = fork();
	if (pid == -1) {
		logerr(__FUNCTION__, __LINE__, "fork");
		status = errno;
		goto EXIT;
	}
	if (pid != 0) {
		// parent
		if (ppid) {
			*ppid = pid;
			return 0;
		}
		do {
			if ((w = waitpid(pid, &status, 0)) == -1) {
				status = errno;
				logerr(__FUNCTION__, __LINE__, "waitpid");
				goto EXIT;
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));

		if (WIFEXITED(status)) status = WEXITSTATUS(status);
EXIT:
		if (!ppid) {
			// restore signals
			sigprocmask(SIG_SETMASK, &sigmask, NULL);
			signal(SIGCHLD, chld);
			// reap zombies
			chld_reap(0);
		}
		return status;
	}
	
	// child

	// reset signal handlers
	for (sig = 0; sig < (_NSIG - 1); sig++)
		signal(sig, SIG_DFL);

	// unblock signals if called from signal handler
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	setsid();

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_WRONLY);
	open("/dev/null", O_WRONLY);

	if (nvram_match("debug_logeval", "1")) {
		pid = getpid();

		cprintf("_eval +%ld pid=%d ", get_uptime(), pid);
		for (n = 0; argv[n]; ++n) cprintf("%s ", argv[n]);
		cprintf("\n");
		
		if ((fd = open("/dev/console", O_RDWR | O_NONBLOCK)) >= 0) {
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
		}
		else {
			sprintf(s, "/tmp/eval.%d", pid);
			if ((fd = open(s, O_CREAT | O_RDWR | O_NONBLOCK, 0600)) >= 0) {
				dup2(fd, STDOUT_FILENO);
				dup2(fd, STDERR_FILENO);
			}
		}
		if (fd > STDERR_FILENO) close(fd);
	}

	// Redirect stdout & stderr to <path>
	if (path) {
		flags = O_WRONLY | O_CREAT | O_NONBLOCK;
		if (*path == '>') {
			++path;
			if (*path == '>') {
				++path;
				// >>path, append
				flags |= O_APPEND;
			}
			else {
				// >path, overwrite
				flags |= O_TRUNC;
			}
		}
		
		if ((fd = open(path, flags, 0644)) < 0) {
			logerr(__FUNCTION__, __LINE__, path);
		}
		else {
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);
		}
	}

	/* execute command */

	p = nvram_safe_get("env_path");
	if (p && *p)
		snprintf(s, sizeof(s), "%s:/sbin:/bin:/usr/sbin:/usr/bin:/opt/sbin:/opt/bin", p);
	else
		snprintf(s, sizeof(s), "/sbin:/bin:/usr/sbin:/usr/bin:/opt/sbin:/opt/bin");

	setenv("PATH", s, 1);

	alarm(timeout);
#if 1
	execvp(argv[0], argv);

	logerr(__FUNCTION__, __LINE__, argv[0]);
#elif 0
	for(n = 0; argv[n]; ++n)
		cpu0_argv[n+3] = argv[n];
	execvp(cpu0_argv[0], cpu0_argv);

	logerr(__FUNCTION__, __LINE__, cpu0_argv[0]);
#else
	for(n = 0; argv[n]; ++n)
		cpu1_argv[n+3] = argv[n];
	execvp(cpu1_argv[0], cpu1_argv);

	logerr(__FUNCTION__, __LINE__, cpu1_argv[0]);

#endif

	_exit(errno);
}

static int get_cmds_size(char **cmds)
{
        int i=0;
        for(; cmds[i]; ++i);
        return i;
}

int _cpu_eval(int *ppid, char *cmds[])
{
        int ncmds=0, n=0, i;
        int maxn = get_cmds_size(cmds)
#if defined (SMP)
                + 4;
#else
                +1;
#endif
        char *cpucmd[maxn];

        for(i=0; i<maxn; ++i)
                cpucmd[i]=NULL;

#if defined (SMP)
        cpucmd[ncmds++]="taskset";
        cpucmd[ncmds++]="-c";
        if(!strcmp(cmds[n], CPU0) || !strcmp(cmds[n], CPU1)) {
                cpucmd[ncmds++]=cmds[n++];
        } else
                cpucmd[ncmds++]=CPU0;
#else
        if(strcmp(cmds[n], CPU0) && strcmp(cmds[n], CPU1))
                cpucmd[ncmds++]=cmds[n++];
        else
                n++;
#endif
        for(; cmds[n]; cpucmd[ncmds++]=cmds[n++]);

        return _eval(cpucmd, NULL, 0, ppid);;
}

/*
 * Concatenates NULL-terminated list of arguments into a single
 * commmand and executes it
 * @param	argv	argument list
 * @return	stdout of executed command or NULL if an error occurred
 */
char *
_backtick(char *const argv[])
{
	int filedes[2];
	pid_t pid;
	int status;
	char *buf = NULL;

	/* create pipe */
	if (pipe(filedes) == -1) {
		logerr(__FUNCTION__, __LINE__, argv[0]);
		return NULL;
	}

	switch (pid = fork()) {
	case -1:	/* error */
		return NULL;
	case 0:		/* child */
		close(filedes[0]);	/* close read end of pipe */
		dup2(filedes[1], 1);	/* redirect stdout to write end of pipe */
		close(filedes[1]);	/* close write end of pipe */
		execvp(argv[0], argv);
		exit(errno);
		break;
	default:	/* parent */
		close(filedes[1]);	/* close write end of pipe */
		buf = fd2str(filedes[0]);
		waitpid(pid, &status, 0);
		break;
	}

	return buf;
}


/*
 * fread() with automatic retry on syscall interrupt
 * @param	ptr	location to store to
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully read
 */
size_t safe_fread(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fread((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

/*
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
size_t safe_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = 0;

	do {
		clearerr(stream);
		ret += fwrite((char *)ptr + (ret * size), size, nmemb - ret, stream);
	} while (ret < nmemb && ferror(stream) && errno == EINTR);

	return ret;
}

/*
 * Return the process ID for a process started with the specified pathname.
 *
 * The function scans /proc, reads each process cmdline, and compares the
 * first command-line entry with the requested name. Command-line arguments
 * are ignored.
 *
 * @param  name  Pathname used to start the process, without arguments
 * @return       Process ID on success, -1 on error or if not found
 */
pid_t get_pid_by_name(const char *name)
{
	DIR *dir;
	struct dirent *next;
	pid_t pid;
	size_t n;

	if (name == NULL || *name == '\0')
		return -1;

	dir = opendir("/proc");
	if (dir == NULL) {
		logerr(__FUNCTION__, __LINE__, "/proc");
		return -1;
	}

	pid = -1;

	while ((next = readdir(dir)) != NULL) {
		FILE *fp;
		char filename[256];
		char buffer[256];
		char *endptr;
		long val;
		int ret;

		if (!isdigit((unsigned char)next->d_name[0]))
			continue;

		val = strtol(next->d_name, &endptr, 10);
		if (*endptr != '\0' || val <= 0)
			continue;

		ret = snprintf(filename, sizeof(filename),
		               "/proc/%s/cmdline", next->d_name);
		if (ret < 0 || (size_t)ret >= sizeof(filename))
			continue;

		fp = fopen(filename, "r");
		if (fp == NULL)
			continue;

		n = fread(buffer, 1, sizeof(buffer) - 1, fp);
		fclose(fp);

		if (n == 0)
			continue;

		buffer[n] = '\0';

		if (strcmp(name, buffer) == 0) {
			pid = (pid_t)val;
			break;
		}
	}

	closedir(dir);

	return pid;
}

/* ether_atoe() helper */
static int hexval(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return -1;
}

/*
 * Convert Ethernet address string representation to binary data
 * @param a  string in xx:xx:xx:xx:xx:xx notation
 * @param e  binary data
 * @return   TRUE if conversion was successful and FALSE otherwise
 */
int ether_atoe(const char *a, unsigned char *e)
{
	int i;

	if (!a || !e)
		return 0;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		int hi = hexval((unsigned char)a[0]);
		int lo = hexval((unsigned char)a[1]);

		if (hi < 0 || lo < 0)
			goto fail;

		e[i] = (unsigned char)((hi << 4) | lo);
		a += 2;

		if (i < ETHER_ADDR_LEN - 1) {
			if (*a++ != ':')
				goto fail;
		}
		else if (*a) {
			goto fail;
		}
	}

	return 1;

fail:
	memset(e, 0, ETHER_ADDR_LEN);

	return 0;
}

/*
 * Convert Ethernet address binary data to string representation
 * @param e       binary Ethernet address
 * @param a       output buffer
 * @return        string
 */
char *ether_etoa(const unsigned char *e, char *a)
{
	static const char hex[] = "0123456789ABCDEF";
	char *p = a;
	int i;

	if (!a) return NULL;

	if (!e) {
		a[0] = '\0';
		return a;
	}

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (i)
			*p++ = ':';

		*p++ = hex[e[i] >> 4];
		*p++ = hex[e[i] & 0x0F];
	}

	*p = '\0';

	return a;
}

void cprintf(const char *format, ...)
{
	FILE *f;
	int nfd;
	va_list args;

#ifdef DEBUG_NOISY
	{
#else
	if (nvram_match("debug_cprintf", "1")) {
#endif
		if((nfd = open("/dev/console", O_WRONLY | O_NONBLOCK)) > 0){
			if((f = fdopen(nfd, "w")) != NULL){
				va_start(args, format);
				vfprintf(f, format, args);
				va_end(args);
				fclose(f);
			}
			close(nfd);
		}
	}

	if (nvram_match("debug_cprintf_file", "1")) {
		if ((f = fopen("/tmp/cprintf", "a")) != NULL) {
			va_start(args, format);
			vfprintf(f, format, args);
			va_end(args);
			fclose(f);
		}
	}
}

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108
// xref (all): nas, wlconf
#if 0
/*
 * Get the ip configuration index if it exists given the
 * eth name.
 *
 * @param	wl_ifname 	pointer to eth interface name
 * @return	index or -1 if not found
 */
int
get_ipconfig_index(char *eth_ifname)
{
	char varname[64];
	char varval[64];
	char *ptr;
	char wl_ifname[NVRAM_MAX_PARAM_LEN];
	int index;

	/* Bail if we get a NULL or empty string */

	if (!eth_ifname) return -1;
	if (!*eth_ifname) return -1;

	/* Look up wl name from the eth name */
	if (osifname_to_nvifname(eth_ifname, wl_ifname, sizeof(wl_ifname)) != 0)
		return -1;

	snprintf(varname, sizeof(varname), "%s_ipconfig_index", wl_ifname);

	ptr = nvram_get(varname);

	if (ptr) {
	/* Check ipconfig_index pointer to see if it is still pointing
	 * the correct lan config block
	 */
		if (*ptr) {
			int index;
			char *ifname;
			char buf[64];
			index = atoi(ptr);

			snprintf(buf, sizeof(buf), "lan%d_ifname", index);

			ifname = nvram_get(buf);

			if (ifname) {
				if  (!(strcmp(ifname, wl_ifname)))
					return index;
			}
			nvram_unset(varname);
		}
	}

	/* The index pointer may not have been configured if the
	 * user enters the variables manually. Do a brute force search
	 *  of the lanXX_ifname variables
	 */
	for (index = 0; index < MAX_NVPARSE; index++) {
		snprintf(varname, sizeof(varname), "lan%d_ifname", index);
		if (nvram_match(varname, wl_ifname)) {
			/* if a match is found set up a corresponding index pointer for wlXX */
			snprintf(varname, sizeof(varname), "%s_ipconfig_index", wl_ifname);
			snprintf(varval, sizeof(varval), "%d", index);
			nvram_set(varname, varval);
			nvram_commit();
			return index;
		};
	}
	return -1;
}

/*
 * Set the ip configuration index given the eth name
 * Updates both wlXX_ipconfig_index and lanYY_ifname.
 *
 * @param	eth_ifname 	pointer to eth interface name
 * @return	0 if successful -1 if not.
 */
int
set_ipconfig_index(char *eth_ifname, int index)
{
	char varname[255];
	char varval[16];
	char wl_ifname[NVRAM_MAX_PARAM_LEN];

	/* Bail if we get a NULL or empty string */

	if (!eth_ifname) return -1;
	if (!*eth_ifname) return -1;

	if (index >= MAX_NVPARSE) return -1;

	/* Look up wl name from the eth name only if the name contains
	   eth
	*/

	if (osifname_to_nvifname(eth_ifname, wl_ifname, sizeof(wl_ifname)) != 0)
		return -1;

	snprintf(varname, sizeof(varname), "%s_ipconfig_index", wl_ifname);
	snprintf(varval, sizeof(varval), "%d", index);
	nvram_set(varname, varval);

	snprintf(varname, sizeof(varname), "lan%d_ifname", index);
	nvram_set(varname, wl_ifname);

	nvram_commit();

	return 0;
}

/*
 * Get interfaces belonging to a specific bridge.
 *
 * @param	bridge_name 	pointer to bridge interface name
 * @return	list of interfaces belonging to the bridge or NULL
 *              if not found/empty
 */
char *
get_bridged_interfaces(char *bridge_name)
{
	static char interfaces[255];
	char *ifnames = NULL;
	char bridge[64];

	if (!bridge_name) return NULL;

	memset(interfaces, 0, sizeof(interfaces));
	snprintf(bridge, sizeof(bridge), "%s_ifnames", bridge_name);

	ifnames = nvram_get(bridge);

	if (ifnames)
		strncpy(interfaces, ifnames, sizeof(interfaces));
	else
		return NULL;

	return  interfaces;

}

#endif	// 0

/*
 * Search a string backwards for a set of characters
 * This is the reverse version of strspn()
 *
 * @param  s       string to search backwards
 * @param  accept  set of chars for which to search
 * @return         number of characters in the trailing segment of s
 *                 which consist only of characters from accept
 */
static size_t sh_strrspn(const char *s, const char *accept)
{
	const char *p;
	size_t accept_len, count;

	if (s == NULL || accept == NULL)
		return 0;

	accept_len = strlen(accept);
	p = s + strlen(s);
	count = 0;

	while (p != s) {
		--p;
		if (memchr(accept, (unsigned char)*p, accept_len) == NULL)
			break;

		++count;
	}

	return count;
}

/*
 * Parse the unit and subunit from an interface string such as wlXX or wlXX.YY
 *
 * @param  ifname   interface string to parse
 * @param  unit     pointer to return the unit number, may pass NULL
 * @param  subunit  pointer to return the subunit number, may pass NULL
 * @return          Returns 0 if the string ends with digits or digits.digits, -1 otherwise.
 *                  If ifname ends in digits.digits, then unit and subuint are set
 *                  to the first and second values respectively. If ifname ends
 *                  in just digits, unit is set to the value, and subunit is set
 *                  to -1. On error both unit and subunit are -1. NULL may be passed
 *                  for unit and/or subuint to ignore the value.
 */
int get_ifname_unit(const char *ifname, int *unit, int *subunit)
{
	const char digits[] = "0123456789";
	char str[64];
	char *p;
	size_t ifname_len, len;
	unsigned long val;
	int u, su;

	u = -1;
	su = -1;

	if (unit)
		*unit = -1;
	if (subunit)
		*subunit = -1;

	if (ifname == NULL)
		return -1;

	if (strlcpy(str, ifname, sizeof(str)) >= sizeof(str))
		return -1;

	ifname_len = strlen(str);

	/* find the trailing digit chars */
	len = sh_strrspn(str, digits);

	/* fail if there were no trailing digits */
	if (len == 0)
		return -1;

	/* point to the beginning of the last integer and convert */
	p = str + (ifname_len - len);

	errno = 0;
	val = strtoul(p, NULL, 10);
	if (errno == ERANGE || val > INT_MAX)
		return -1;

	/* no ".subunit" suffix: trailing number is the unit */
	if (p == str || p[-1] != '.') {
		u = (int)val;

		if (unit)
			*unit = u;
		if (subunit)
			*subunit = su;

		return 0;
	}

	/* trailing number is subunit */
	su = (int)val;

	/* chop off the ".NNN" and get the unit number */
	p--;
	*p = '\0';

	/* find the trailing digit chars before the dot */
	len = sh_strrspn(str, digits);
	if (len == 0)
		return -1;

	p = p - len;

	errno = 0;
	val = strtoul(p, NULL, 10);
	if (errno == ERANGE || val > INT_MAX)
		return -1;

	u = (int)val;

	if (unit)
		*unit = u;
	if (subunit)
		*subunit = su;

	return 0;
}

/* In the space-separated/null-terminated list(haystack), try to
 * locate the string "needle"
 */
char *find_in_list(const char *haystack, const char *needle)
{
	const char *ptr;
	size_t needle_len, len;

	if (!haystack || !needle || !*haystack || !*needle)
		return NULL;

	needle_len = strlen(needle);
	ptr = haystack;

	while (*ptr) {
		/* consume leading spaces */
		ptr += strspn(ptr, " ");
		if (!*ptr)
			break;

		/* what's the length of the next word */
		len = strcspn(ptr, " ");

		if (needle_len == len && strncmp(needle, ptr, len) == 0)
			return (char *)ptr;

		ptr += len;
	}

	return NULL;
}

/*
 * Remove the specified word from the list.

 * @param  name      word to be removed from the list
 * @param  list      Space separated list to modify
 * @param  listsize  Max size the list can occupy
 * @return           error code
 */
int remove_from_list(const char *name, char *list, int listsize)
{
	char *occurrence;
	size_t namelen, tail_len;

	if (list == NULL || name == NULL || listsize <= 0 || *name == '\0')
		return EINVAL;

	/* ensure list is NUL-terminated within listsize */
	if (strnlen(list, (size_t)listsize) == (size_t)listsize)
		return EINVAL;

	namelen = strlen(name);

	occurrence = find_in_list(list, name);
	if (occurrence == NULL)
		return EINVAL;

	/* last item in list */
	if (occurrence[namelen] == '\0') {
		if (occurrence != list)
			occurrence--;

		*occurrence = '\0';
		return 0;
	}

	/* first or middle item, followed by a space */
	if (occurrence[namelen] == ' ') {
		tail_len = strlen(&occurrence[namelen + 1]) + 1;
		memmove(occurrence, &occurrence[namelen + 1], tail_len);
		return 0;
	}

	return EINVAL;
}

/*
 * Add the specified interface(string) to the list as long as
 * it will fit in the space left in the list.
 *
 * NOTE: If item is already in list, it won't be added again.
 *
 * @param  name      Name of interface to be added to the list
 * @param  list      List to modify
 * @param  listsize  Max size the list can occupy
 * @return           error code
 */
int add_to_list(const char *name, char *list, int listsize)
{
	size_t listlen;
	size_t namelen;
	size_t need_space;

	if (list == NULL || name == NULL || listsize <= 0 || *name == '\0')
		return EINVAL;

	listlen = strnlen(list, (size_t)listsize);
	if (listlen == (size_t)listsize)
		return EINVAL;

	namelen = strlen(name);

	if (find_in_list(list, name))
		return 0;

	need_space = (listlen != 0 && list[listlen - 1] != ' ') ? 1 : 0;

	if (namelen >= (size_t)listsize - listlen - need_space)
		return EMSGSIZE;

	if (need_space)
		list[listlen++] = ' ';

	memcpy(&list[listlen], name, namelen + 1);

	return 0;
}

/*
 * Remove duplicate words from a space-separated list in place.
 *
 * The function walks through the input list and builds a temporary output
 * list containing only the first occurrence of each word. Duplicate entries
 * are skipped. The resulting list is then copied back into the original
 * buffer.
 *
 * @param  inlist       Space-separated list to deduplicate in place
 * @param  inlist_size  Max size the list can occupy
 * @return              Pointer to the modified input list on success, NULL on error
 */
char *remove_dups(char *inlist, int inlist_size)
{
	char name[256];
	char *next, *outlist;
	int ret;

	if (inlist == NULL || inlist_size <= 0)
		return NULL;

	if (strnlen(inlist, (size_t)inlist_size) == (size_t)inlist_size)
		return NULL;

	outlist = (char *)malloc((size_t)inlist_size);
	if (outlist == NULL)
		return NULL;

	memset(outlist, 0, (size_t)inlist_size);

	next = NULL;

	foreach(name, inlist, next) {
		if (name[0] == '\0')
			continue;

		ret = add_to_list(name, outlist, inlist_size);
		if (ret != 0) {
			free(outlist);
			return NULL;
		}
	}

	strlcpy(inlist, outlist, (size_t)inlist_size);

	free(outlist);

	return inlist;
}

/*
 * Find the lexicographically smallest word in a space-separated list.
 *
 * The function scans a NUL-terminated, space-separated list and returns
 * a pointer to the first occurrence of the smallest word. Leading and
 * repeated spaces are ignored. The returned pointer refers to the original
 * input buffer.
 *
 * @param  haystack  Space-separated list to search
 * @return           Pointer to the smallest word in haystack, NULL on error
 *                   or if the list is empty
 */
char *find_smallest_in_list(char *haystack)
{
	char *ptr;
	char *smallest;
	size_t len, smallest_len, cmp_len;
	int ret;

	if (haystack == NULL)
		return NULL;

	ptr = haystack + strspn(haystack, " ");
	if (*ptr == '\0')
		return NULL;

	smallest = ptr;
	smallest_len = strcspn(ptr, " ");
	ptr += smallest_len;

	while (*ptr != '\0') {
		/* consume leading spaces */
		ptr += strspn(ptr, " ");
		if (*ptr == '\0')
			break;

		/* what's the length of the next word */
		len = strcspn(ptr, " ");
		cmp_len = (len < smallest_len) ? len : smallest_len;

		ret = strncmp(ptr, smallest, cmp_len);
		if (ret < 0 || (ret == 0 && len < smallest_len)) {
			smallest = ptr;
			smallest_len = len;
		}

		ptr += len;
	}

	return smallest;
}

char *sort_list(char *inlist, int inlist_size) {
	char *tmplist;
	char tmp[IFNAMSIZ];

	if (!inlist_size) return NULL;
	if (!inlist) return NULL;

	tmplist = (char *) malloc(inlist_size);
	if (!tmplist) return NULL;
	memset(tmplist, 0, inlist_size);

	char *b;
	int len;
	while ((b = find_smallest_in_list(inlist)) != NULL) {
		len = strcspn(b, " ");
		snprintf(tmp, len + 1, "%s", b);

		add_to_list(tmp, tmplist, inlist_size);
		remove_from_list(tmp, inlist, inlist_size);

	}
	strncpy(inlist, tmplist, inlist_size);

	free(tmplist);
	return inlist;
}

/*
	 return true/false if any wireless interface has URE enabled.
*/
int
ure_any_enabled(void)
{
	char *temp;
	char nv_param[NVRAM_MAX_PARAM_LEN];

	sprintf(nv_param, "ure_disable");
	temp = nvram_safe_get(nv_param);
	if (temp && (strncmp(temp, "0", 1) == 0))
		return 1;
	else
		return 0;
}


#define WLMBSS_DEV_NAME	"wlmbss"
#define WL_DEV_NAME "wl"
#define WDS_DEV_NAME	"wds"

/**
 *	 nvifname_to_osifname()
 *  The intent here is to provide a conversion between the OS interface name
 *  and the device name that we keep in NVRAM.
 * This should eventually be placed in a Linux specific file with other
 * OS abstraction functions.

 * @param nvifname pointer to ifname to be converted
 * @param osifname_buf storage for the converted osifname
 * @param osifname_buf_len length of storage for osifname_buf
 */
int
nvifname_to_osifname(const char *nvifname, char *osifname_buf,
                     int osifname_buf_len)
{
	char varname[NVRAM_MAX_PARAM_LEN];
	char *ptr;

	/* Bail if we get a NULL or empty string */
	if ((!nvifname) || (!*nvifname) || (!osifname_buf)) {
		return -1;
	}

	memset(osifname_buf, 0, osifname_buf_len);

	if (strstr(nvifname, "eth") || strstr(nvifname, ".")) {
		strncpy(osifname_buf, nvifname, osifname_buf_len);
		return 0;
	}

	snprintf(varname, sizeof(varname), "%s_ifname", nvifname);
	ptr = nvram_get(varname);
	if (ptr) {
		/* Bail if the string is empty */
		if (!*ptr) return -1;
		strncpy(osifname_buf, ptr, osifname_buf_len);
		return 0;
	}

	return -1;
}


/* osifname_to_nvifname()
 * Convert the OS interface name to the name we use internally(NVRAM, GUI, etc.)
 * This is the Linux version of this function

 * @param osifname pointer to osifname to be converted
 * @param nvifname_buf storage for the converted ifname
 * @param nvifname_buf_len length of storage for nvifname_buf
 */
int
osifname_to_nvifname(const char *osifname, char *nvifname_buf,
                     int nvifname_buf_len)
{
	char varname[NVRAM_MAX_PARAM_LEN];
	int pri, sec;

	/* Bail if we get a NULL or empty string */

	if ((!osifname) || (!*osifname) || (!nvifname_buf))
	{
		return -1;
	}

	memset(nvifname_buf, 0, nvifname_buf_len);

	if (strstr(osifname, "wl") || strstr(osifname, "br") ||
	     strstr(osifname, "wds")) {
		strncpy(nvifname_buf, osifname, nvifname_buf_len);
		return 0;
	}

	/* look for interface name on the primary interfaces first */
	for (pri = 0; pri < MAX_NVPARSE; pri++) {
		snprintf(varname, sizeof(varname),
					"wl%d_ifname", pri);
		if (nvram_match(varname, (char *)osifname)) {
					snprintf(nvifname_buf, nvifname_buf_len, "wl%d", pri);
					return 0;
				}
	}

	/* look for interface name on the multi-instance interfaces */
	for (pri = 0; pri < MAX_NVPARSE; pri++)
		for (sec = 0; sec < MAX_NVPARSE; sec++) {
			snprintf(varname, sizeof(varname),
					"wl%d.%d_ifname", pri, sec);
			if (nvram_match(varname, (char *)osifname)) {
				snprintf(nvifname_buf, nvifname_buf_len, "wl%d.%d", pri, sec);
				return 0;
			}
		}

	return -1;
}

#endif	// #if WL_BSS_INFO_VERSION >= 108

/******************************************************************************/
/*
 *	Add a character to a string buffer
 */

static void put_char(strbuf_t *buf, char c)
{
	if (buf->count >= (buf->size - 1)) {
		if (! (buf->flags & STR_REALLOC)) {
			return;
		}
		buf->size += STR_INC;
		if (buf->size > buf->max && buf->size > STR_INC) {
/*
 *			Caller should increase the size of the calling buffer
 */
			buf->size -= STR_INC;
			return;
		}
		if (buf->s == NULL) {
			buf->s = balloc(B_L, buf->size * sizeof(char));
		} else {
			buf->s = brealloc(B_L, buf->s, buf->size * sizeof(char));
		}
	}
	buf->s[buf->count] = c;
	if (c != '\0') {
		++buf->count;
	}
}

/******************************************************************************/
/*
 *	Add a string to a string buffer
 */

static void put_string(strbuf_t *buf, char *s, int len, int width,
		int prec, enum flag f)
{
	int		i;

	if (len < 0) { 
		len = strnlen(s, (prec >= 0 ? (unsigned int) prec : ULONG_MAX));
	} else if (prec >= 0 && prec < len) { 
		len = prec; 
	}
	if (width > len && !(f & flag_minus)) {
		for (i = len; i < width; ++i) { 
			put_char(buf, ' '); 
		}
	}
	for (i = 0; i < len; ++i) { 
		put_char(buf, s[i]); 
	}
	if (width > len && f & flag_minus) {
		for (i = len; i < width; ++i) { 
			put_char(buf, ' '); 
		}
	}
}

/******************************************************************************/
/*
 *	Add a long to a string buffer
 */

static void put_ulong(strbuf_t *buf, unsigned long int value, int base,
		int upper, char *prefix, int width, int prec, enum flag f)
{
	unsigned long	x, x2;
	int				len, zeros, i;

	for (len = 1, x = 1; x < ULONG_MAX / base; ++len, x = x2) {
		x2 = x * base;
		if (x2 > value) { 
			break; 
		}
	}
	zeros = (prec > len) ? prec - len : 0;
	width -= zeros + len;
	if (prefix != NULL) { 
		width -= strnlen(prefix, ULONG_MAX); 
	}
	if (!(f & flag_minus)) {
		if (f & flag_zero) {
			for (i = 0; i < width; ++i) { 
				put_char(buf, '0'); 
			}
		} else {
			for (i = 0; i < width; ++i) { 
				put_char(buf, ' '); 
			}
		}
	}
	if (prefix != NULL) { 
		put_string(buf, prefix, -1, 0, -1, flag_none); 
	}
	for (i = 0; i < zeros; ++i) { 
		put_char(buf, '0'); 
	}
	for ( ; x > 0; x /= base) {
		int digit = (value / x) % base;
		put_char(buf, (char) ((digit < 10 ? '0' : (upper ? 'A' : 'a') - 10) +
			digit));
	}
	if (f & flag_minus) {
		for (i = 0; i < width; ++i) { 
			put_char(buf, ' '); 
		}
	}
}

/******************************************************************************/
/*
 *	Dynamic sprintf implementation. Supports dynamic buffer allocation.
 *	This function can be called multiple times to grow an existing allocated
 *	buffer. In this case, msize is set to the size of the previously allocated
 *	buffer. The buffer will be realloced, as required. If msize is set, we
 *	return the size of the allocated buffer for use with the next call. For
 *	the first call, msize can be set to -1.
 */

static int dsnprintf(char **s, int size, char *fmt, va_list arg, int msize)
{
	strbuf_t	buf;
	char		c;

	assert(s);
	assert(fmt);

	memset(&buf, 0, sizeof(buf));
	buf.s = *s;

	if (*s == NULL || msize != 0) {
		buf.max = size;
		buf.flags |= STR_REALLOC;
		if (msize != 0) {
			buf.size = max(msize, 0);
		}
		if (*s != NULL && msize != 0) {
			buf.count = strlen(*s);
		}
	} else {
		buf.size = size;
	}

	while ((c = *fmt++) != '\0') {
		if (c != '%' || (c = *fmt++) == '%') {
			put_char(&buf, c);
		} else {
			enum flag f = flag_none;
			int width = 0;
			int prec = -1;
			for ( ; c != '\0'; c = *fmt++) {
				if (c == '-') { 
					f |= flag_minus; 
				} else if (c == '+') { 
					f |= flag_plus; 
				} else if (c == ' ') { 
					f |= flag_space; 
				} else if (c == '#') { 
					f |= flag_hash; 
				} else if (c == '0') { 
					f |= flag_zero; 
				} else {
					break;
				}
			}
			if (c == '*') {
				width = va_arg(arg, int);
				if (width < 0) {
					f |= flag_minus;
					width = -width;
				}
				c = *fmt++;
			} else {
				for ( ; isdigit((int)c); c = *fmt++) {
					width = width * 10 + (c - '0');
				}
			}
			if (c == '.') {
				f &= ~flag_zero;
				c = *fmt++;
				if (c == '*') {
					prec = va_arg(arg, int);
					c = *fmt++;
				} else {
					for (prec = 0; isdigit((int)c); c = *fmt++) {
						prec = prec * 10 + (c - '0');
					}
				}
			}
			if (c == 'h' || c == 'l') {
				f |= (c == 'h' ? flag_short : flag_long);
				c = *fmt++;
			}
			if (c == 'd' || c == 'i') {
				long int value;
				if (f & flag_short) {
					value = (short int) va_arg(arg, int);
				} else if (f & flag_long) {
					value = va_arg(arg, long int);
				} else {
					value = va_arg(arg, int);
				}
				if (value >= 0) {
					if (f & flag_plus) {
						put_ulong(&buf, value, 10, 0, ("+"), width, prec, f);
					} else if (f & flag_space) {
						put_ulong(&buf, value, 10, 0, (" "), width, prec, f);
					} else {
						put_ulong(&buf, value, 10, 0, NULL, width, prec, f);
					}
				} else {
					put_ulong(&buf, -value, 10, 0, ("-"), width, prec, f);
				}
			} else if (c == 'o' || c == 'u' || c == 'x' || c == 'X') {
				unsigned long int value;
				if (f & flag_short) {
					value = (unsigned short int) va_arg(arg, unsigned int);
				} else if (f & flag_long) {
					value = va_arg(arg, unsigned long int);
				} else {
					value = va_arg(arg, unsigned int);
				}
				if (c == 'o') {
					if (f & flag_hash && value != 0) {
						put_ulong(&buf, value, 8, 0, ("0"), width, prec, f);
					} else {
						put_ulong(&buf, value, 8, 0, NULL, width, prec, f);
					}
				} else if (c == 'u') {
					put_ulong(&buf, value, 10, 0, NULL, width, prec, f);
				} else {
					if (f & flag_hash && value != 0) {
						if (c == 'x') {
							put_ulong(&buf, value, 16, 0, ("0x"), width, 
								prec, f);
						} else {
							put_ulong(&buf, value, 16, 1, ("0X"), width, 
								prec, f);
						}
					} else {
                  /* 04 Apr 02 BgP -- changed so that %X correctly outputs
                   * uppercase hex digits when requested.
						put_ulong(&buf, value, 16, 0, NULL, width, prec, f);
                   */
						put_ulong(&buf, value, 16, ('X' == c) , NULL, width, prec, f);
					}
				}

			} else if (c == 'c') {
				char value = va_arg(arg, int);
				put_char(&buf, value);

			} else if (c == 's' || c == 'S') {
				char *value = va_arg(arg, char *);
				if (value == NULL) {
					put_string(&buf, ("(null)"), -1, width, prec, f);
				} else if (f & flag_hash) {
					put_string(&buf,
						value + 1, (char) *value, width, prec, f);
				} else {
					put_string(&buf, value, -1, width, prec, f);
				}
			} else if (c == 'p') {
				void *value = va_arg(arg, void *);
				put_ulong(&buf,
					(unsigned long int) value, 16, 0, ("0x"), width, prec, f);
			} else if (c == 'n') {
				if (f & flag_short) {
					short int *value = va_arg(arg, short int *);
					*value = buf.count;
				} else if (f & flag_long) {
					long int *value = va_arg(arg, long int *);
					*value = buf.count;
				} else {
					int *value = va_arg(arg, int *);
					*value = buf.count;
				}
			} else {
				put_char(&buf, c);
			}
		}
	}
	if (buf.s == NULL) {
		put_char(&buf, '\0');
	}

/*
 *	If the user requested a dynamic buffer (*s == NULL), ensure it is returned.
 */
	if (*s == NULL || msize != 0) {
		*s = buf.s;
	}

	if (*s != NULL && size > 0) {
		if (buf.count < size) {
			(*s)[buf.count] = '\0';
		} else {
			(*s)[buf.size - 1] = '\0';
		}
	}

	if (msize != 0) {
		return buf.size;
	}
	return buf.count;
}

/******************************************************************************/
/*
 *	sprintf and vsprintf are bad, ok. You can easily clobber memory. Use
 *	fmtAlloc and fmtValloc instead! These functions do _not_ support floating
 *	point, like %e, %f, %g...
 */

int fmtAlloc(char **s, int n, char *fmt, ...)
{
	va_list	ap;
	int		result;

	assert(s);
	assert(fmt);

	*s = NULL;
	va_start(ap, fmt);
	result = dsnprintf(s, n, fmt, ap, 0);
	va_end(ap);
	return result;
}

/******************************************************************************/
/*
 *	A vsprintf replacement.
 */

int fmtValloc(char **s, int n, char *fmt, va_list arg)
{
	assert(s);
	assert(fmt);

	*s = NULL;
	return dsnprintf(s, n, fmt, arg, 0);
}

/*
 *  * description: parse va and do system
 *  */
int doSystem(char *fmt, ...)
{
	va_list		vargs;
	char		*cmd = NULL;
	int 		rc = 0;
	#define CMD_BUFSIZE 256
	va_start(vargs, fmt);
	if (fmtValloc(&cmd, CMD_BUFSIZE, fmt, vargs) >= CMD_BUFSIZE) {
		fprintf(stderr, "doSystem: lost data, buffer overflow\n");
	}
	va_end(vargs);

	if(cmd) {
		if (!strncmp(cmd, "iwpriv", 6))
			logmsg(LOG_DEBUG, "*** %s: %s", __FUNCTION__, cmd);
		rc = system(cmd);
		bfree(B_L, cmd);
	}	
	return rc;
}

int 
swap_check()
{
	struct sysinfo info;

	sysinfo(&info);

	if(info.totalswap > 0)
		return 1;
	else	return 0;
}

void killall_tk_period_wait(const char *name, int wait_ds) /* time in deciseconds (1/10 sec) */
{
	int n;

	if (killall(name, SIGTERM) == 0) {
		n = wait_ds;
		while ((killall(name, 0) == 0) && (n-- > 0)) {
			logmsg(LOG_DEBUG, "*** %s: waiting name=%s n=%d", __FUNCTION__, name, n);
			usleep(100 * 1000); /* 100 ms */
		}
		if (n < 0) {
			n = wait_ds * 2;
			while ((killall(name, SIGKILL) == 0) && (n-- > 0)) {
				logmsg(LOG_DEBUG, "*** %s: SIGKILL name=%s n=%d", __FUNCTION__, name, n);
				usleep(100 * 1000); /* 100 ms */
			}
		}
	}
}

void killall_and_waitfor(const char *name, int loop, int killtime)
{
	pid_t pid;

	killall_tk_period_wait(name, killtime); /* wait time in deciseconds (1/10 sec) */
	while ((pid = pidof(name)) > 0 && (loop-- > 0)) {
		logmsg(LOG_WARNING, "killing %s ...", name);
		/* Reap the zombie if it has terminated */
		waitpid(pid, NULL, WNOHANG);
		sleep(1);
	}
}

int kill_pidfile_s(char *pidfile, int sig)
{
	char tmp[100];
	int pid;

	if (f_read_string(pidfile, tmp, sizeof(tmp)) > 0) {
		if ((pid = atoi(tmp)) > 1)
			return kill(pid, sig);
	}

	return -1;
}

#if 0
int kill_pidfile_s_rm(char *pidfile, int sig)
{
	FILE *fp;
	char buf[256];

	if ((fp = fopen(pidfile, "r")) != NULL) {
		if (fgets(buf, sizeof(buf), fp)) {
			pid_t pid = strtoul(buf, NULL, 0);
			fclose(fp);
			unlink(pidfile);
			return kill(pid, sig);
		}
		fclose(fp);
	}
	return errno;
}
#endif /* 0 */

long uptime(void)
{
	struct sysinfo info;
	sysinfo(&info);
	
	return info.uptime;
}

int _vstrsep(char *buf, const char *sep, ...)
{
	va_list ap;
	char **p;
	int n;

	n = 0;
	va_start(ap, sep);
	while ((p = va_arg(ap, char **)) != NULL) {
		if ((*p = strsep(&buf, sep)) == NULL) break;
		++n;
	}
	va_end(ap);
	return n;
}

#ifdef CONFIG_BCMWL5
char *
wl_ether_etoa(const struct ether_addr *n)
{
	static char etoa_buf[ETHER_ADDR_LEN * 3];
	char *c = etoa_buf;
	int i;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (i)
			*c++ = ':';
		c += sprintf(c, "%02X", n->octet[i] & 0xff);
	}
	return etoa_buf;
}
#endif

/* Find partition with defined name and return partition number as an integer */
int getMTD(const char *name)
{
	char line[128], dev[32], size[32], esize[32], part_name[64];
	int mtdnum, device = -1;
	FILE *fp;

	if (!name)
		return -1;

	if (!(fp = fopen("/proc/mtd", "r")))
		return -1;

	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "%31s %31s %31s \"%63[^\"]\"", dev, size, esize, part_name) != 4)
			continue;

		if (strcmp(part_name, name) != 0)
			continue;

		if (sscanf(dev, "mtd%d:", &mtdnum) == 1) {
			device = mtdnum;
			break;
		}
	}
	fclose(fp);

	return device;
}
