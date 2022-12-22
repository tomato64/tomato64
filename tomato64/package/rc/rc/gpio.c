/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/


#include "rc.h"


int gpio_main(int argc, char *argv[])
{
	const char hex[] = "0123456789ABCDEF0123456789ABCDEF"; /* 16 pins + 16 pins (0 - 31) for visual help */
	long v;
	int bit;
	int i;
	char s[40];
	int f;

	if ((argc == 3) && ((strncmp(argv[1], "en", 2) == 0) || (strncmp(argv[1], "di", 2) == 0))) {
		bit = atoi(argv[2]);
		if ((bit >= TOMATO_GPIO_MIN) && (bit <= TOMATO_GPIO_MAX)) {
			bit = 1 << bit;
			{
				gpio_write(bit, argv[1][0] == 'e');

				return 0;
			}
		}
	}
	else if (argc >= 2) {
		if (strncmp(argv[1], "po", 2) == 0) {
			if (argc >= 3)
				bit = atoi(argv[2]);
			else
				bit = 0;

			printf("Enable gpio mask: 0x%08X\n", bit);

			if ((f = gpio_open(bit)) < 0) {
				printf("Failed to open gpio\n");

				return 0;
			}
			while ((v = _gpio_read(f)) != ~0) {
				memset(s, 0, sizeof(s)); /* clean-up */
				for (i = TOMATO_GPIO_MAX; i >= TOMATO_GPIO_MIN; --i) {
					s[i] = (v & (1 << i)) ? hex[i] : '.';
				}
				s[sizeof(s)-1] = '\0';
				printf("%08lX %s (GPIO 0 - 31)\n", v, s);
				sleep(1);
			}
			close(f);

			return 0;
		}
	}

	usage_exit(argv[0], "<enable|disable|poll> <pin|[poll_mask]>\n");
}
