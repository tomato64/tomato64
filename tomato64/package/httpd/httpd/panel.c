/*
 * panel.c
 *
 * The front-panel screen, from the web UI: a PNG of what the screen is
 * showing, and pointer input that drives it the way a finger does. Written
 * for developing against the panel without sitting in front of the device.
 *
 * Both go through one control channel, a unix datagram socket the panel UI
 * binds and reads inside its own event loop (remote.uc). That keeps this file
 * free of ubus - httpd links against neither libubus nor LVGL - and keeps the
 * cost of a pointer sample to a single sendto(), which matters when a drag
 * sends tens of them a second. The UI is the only thing that can draw the
 * panel anyway, so asking it is also the only way to capture one.
 *
 * The screen capture re-renders the widget tree rather than reading back the
 * display, so it cannot catch a half-drawn frame. It is written to a fixed
 * path under a name that is renamed into place, so a reply is never a partly
 * written file.
 *
 * Hooks into the rest of httpd: two rows in mime_handlers[] and the include
 * that declares them. BE14000 only - it is the one board with a screen.
 */

#include "tomato.h"
#include "panel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

/* needed by logmsg() */
#define LOGMSG_DISABLE		DISABLE_SYSLOG_OSM
#define LOGMSG_NVDEBUG		"panel_debug"

/* Bound by the panel UI; it exists only while the UI is running, which is
   what tells a request that the screen is off. */
#define PANEL_SOCK	"/var/run/panel_remote"

/* Where the UI is asked to leave a capture for us. */
#define PANEL_SHOT	"/var/run/panel-screen.png"

/* A capture costs a re-render plus a PNG encode of 320x240. Measured well
   under 100ms on this board; the wait is for a UI that is busy, not slow. */
#define SHOT_WAIT_MS	1000
#define SHOT_POLL_MS	5

/* Sane for any panel this could ever drive, and all that is needed here: the
   UI clamps a sample to its own display before using it. */
#define COORD_MAX	9999

static void panel_msleep(int ms)
{
	struct timespec ts;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000L;

	nanosleep(&ts, NULL);
}

/*
 * One datagram per command, so the UI reads each one whole however many
 * requests are in flight. Nothing is sent back: the reply to a capture is the
 * file appearing, and a pointer sample has no answer worth waiting for.
 *
 * Fails rather than blocks when the UI is not running - there is no socket to
 * send to - or when its queue is full, which a screen that has stopped
 * reading would cause.
 */
static int panel_command(const char *fmt, ...)
{
	struct sockaddr_un sa;
	char buf[128];
	va_list args;
	int fd, len, ok;

	va_start(args, fmt);
	len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if ((len <= 0) || (len >= (int)sizeof(buf)))
		return 0;

	if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
		return 0;

	/* Set here rather than as a socket() flag, which needs _GNU_SOURCE. */
	fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	strlcpy(sa.sun_path, PANEL_SOCK, sizeof(sa.sun_path));

	ok = (sendto(fd, buf, len, 0, (struct sockaddr *)&sa, sizeof(sa)) == len);

	if (!ok)
		logmsg(LOG_DEBUG, "*** [tomato] %s: %s: %s", __FUNCTION__, PANEL_SOCK, strerror(errno));

	close(fd);

	return ok;
}

static int panel_shot_ready(void)
{
	struct stat st;

	return ((stat(PANEL_SHOT, &st) == 0) && S_ISREG(st.st_mode) && (st.st_size > 0));
}

/*
 * GET panel/screen.png
 *
 * The row carries no mime type, so that a panel which is not running answers
 * 503 rather than an empty image the browser would cache as one.
 */
void wo_panel_screen(char *url)
{
	int waited;

	unlink(PANEL_SHOT);

	if (!panel_command("s %s\n", PANEL_SHOT)) {
		send_header(503, NULL, mime_plain, 0);
		web_puts("panel not running\n");

		return;
	}

	for (waited = 0; waited < SHOT_WAIT_MS; waited += SHOT_POLL_MS) {
		if (panel_shot_ready())
			break;

		panel_msleep(SHOT_POLL_MS);
	}

	if (!panel_shot_ready()) {
		send_header(503, NULL, mime_plain, 0);
		web_puts("no screen capture\n");

		return;
	}

	send_header(200, NULL, "image/png", 0);
	do_file(PANEL_SHOT);
}

/*
 * POST panel/input.cgi with x, y and down (1 while the pointer is held), or
 * with wake=1 for nothing but a wake.
 *
 * One sample per request: press, each step of a drag, and the release. The UI
 * turns the stream back into taps, drags and swipes, so nothing here has to
 * know what a gesture is.
 *
 * Waking is its own command rather than a tap in the middle of the screen,
 * which is what it would land on when the panel is already awake.
 */
void wo_panel_input(char *url)
{
	int x, y, down;

	if (atoi(webcgi_safeget("wake", "0"))) {
		send_header(200, NULL, mime_plain, 0);
		web_puts(panel_command("w\n") ? "ok\n" : "panel not running\n");

		return;
	}

	x = atoi(webcgi_safeget("x", "-1"));
	y = atoi(webcgi_safeget("y", "-1"));
	down = atoi(webcgi_safeget("down", "0")) ? 1 : 0;

	send_header(200, NULL, mime_plain, 0);

	if ((x < 0) || (y < 0) || (x > COORD_MAX) || (y > COORD_MAX)) {
		web_puts("bad point\n");

		return;
	}

	web_puts(panel_command("t %d %d %d\n", x, y, down) ? "ok\n" : "panel not running\n");
}
