#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <shared.h>


int main(int argc, char *argv[])
{
	struct sysinfo si;
	time_t uptime;
	char wantime_file[64];
	char prefix[] = "wanXX";

	if (argc > 1)
		strlcpy(prefix, argv[1], sizeof(prefix));
	else
		strlcpy(prefix, "wan", sizeof(prefix));

	memset(wantime_file, 0, sizeof(wantime_file));
	snprintf(wantime_file, sizeof(wantime_file), "/var/lib/misc/%s_time", prefix);

	if (sysinfo(&si) == -1)
		return 1;

	if (check_wanup(prefix) && f_read(wantime_file, &uptime, sizeof(time_t)) == sizeof(uptime))
		printf("%ld\n", si.uptime - uptime);
	else
		printf("0\n");

	return 0;
}
