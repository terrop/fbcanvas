#include <linux/input.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "document.h"
#include "keymap.h"

static bool active_console = true;
static bool need_repaint = false;

static void handle_signal(int s)
{
	int fd = open("/dev/tty", O_RDWR);
	if (fd)
	{
		if (s == SIGUSR1)
		{
			/* Release display */
			active_console = false;
			ioctl(fd, VT_RELDISP, 1);
		} else if (s == SIGUSR2) {
			/* Acquire display */
			ioctl(fd, VT_RELDISP, VT_ACKACQ);
			need_repaint = true;
			active_console = true;
		}

		close(fd);
	}
}

static int keyboard_fd = -1;
static int mouse_fd = -1;
static struct pollfd pfd[2];
static sigset_t sigs;
static int modifiers = 0;

int read_key(struct document *doc)
{
	for (;;)
	{
		int ret = ppoll(pfd, 2, NULL, &sigs);
		if (ret == -1) /* error, most likely EINTR. */
		{
			/* This will be handled later */
		} else if (ret == 0) { /* Timeout */
			/* Nothing to do */
		} else {
			/* Mouse input available */
			if (pfd[0].revents)
			{
				struct input_event ev;
				read(mouse_fd, &ev, sizeof(ev));

				if (ev.type == EV_REL)
				{
					if (ev.code == REL_X)
					{
						if (ev.value < 0)
							return KEY_LEFT | SHIFT;
						else if (ev.value > 0)
							return KEY_RIGHT | SHIFT;

					} else if (ev.code == REL_Y) {
						if (ev.value < 0)
							return KEY_UP | SHIFT;
						else if (ev.value > 0)
							return KEY_DOWN | SHIFT;
					}
				}
			}

			/* Keyboard input available */
			if (pfd[1].revents)
			{
				int key = 0;
				struct input_event ev;
				read(keyboard_fd, &ev, sizeof(ev));

				if (ev.type == EV_KEY)
				{
					unsigned int m = 0;
					if (ev.code == KEY_LEFTSHIFT || ev.code == KEY_RIGHTSHIFT)
						m = SHIFT;
					if (ev.code == KEY_LEFTCTRL || ev.code == KEY_RIGHTCTRL)
						m = CONTROL;
					if (ev.code == KEY_LEFTALT || ev.code == KEY_RIGHTALT)
						m = ALT;

					if (ev.value == 1 || ev.value == 2)
						modifiers |= m;
					else if (ev.value == 0)
						modifiers &= ~m;

					if (m)
						continue;

					if (ev.value == 1 || ev.value == 2)
					{
						/*
						 * Ei päästetä ALT+konsolinvaihtoa
						 * pääohjelmaan asti.
						 */
						switch (ev.code)
						{
							case KEY_F1 ... KEY_F10:
							case KEY_F11 ... KEY_F12:
							case KEY_LEFT:
							case KEY_RIGHT:
								if (!(modifiers & ALT))
									key = ev.code;
								break;
							default:
								key = ev.code;
								break;
						}
					}

					if (key && active_console)
						return modifiers | key;
				}

				if (need_repaint)
				{
					need_repaint = false;
					return KEY_L | CONTROL;
				}
			}
		}
	}
}
