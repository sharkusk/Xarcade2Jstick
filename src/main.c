/* ======================================================================== */
/*  This program is free software; you can redistribute it and/or modify    */
/*  it under the terms of the GNU General Public License as published by    */
/*  the Free Software Foundation; either version 2 of the License, or       */
/*  (at your option) any later version.                                     */
/*                                                                          */
/*  This program is distributed in the hope that it will be useful,         */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       */
/*  General Public License for more details.                                */
/*                                                                          */
/*  You should have received a copy of the GNU General Public License       */
/*  along with this program; if not, write to the Free Software             */
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.               */
/* ======================================================================== */
/*                 Copyright (c) 2014, Florian Mueller                      */
/*                           (c) 2023, Sharkus                              */
/* ======================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <syslog.h>

#include "uinput_gamepad.h"
#include "uinput_kbd.h"
#include "input_xarcade.h"
#include "key_map.h"

// TODO Extract all magic numbers and collect them as defines in at a central location

#define GPADSNUM 2

UINP_KBD_DEV uinp_kbd;
UINP_GPAD_DEV uinp_gpads[GPADSNUM];
INP_XARC_DEV xarcdev;
int use_syslog = 0;

#define SYSLOG(...) if (use_syslog == 1) { syslog(__VA_ARGS__); }

static void teardown();
static void signal_handler(int signum);

static void send_key(const key_map_t *p_map_entry, uint16_t ev_value)
{
    int is_pressed = ev_value > 0;

	if (p_map_entry->destination == game_pad) {
		uinput_gpad_write(
			&uinp_gpads[p_map_entry->game_pad_index],
			p_map_entry->code,
			is_pressed ? p_map_entry->value_high:p_map_entry->value_low,
			p_map_entry->evtype);
	} else {
		uinput_kbd_write(
			&uinp_kbd,
			p_map_entry->code,
			ev_value,
			p_map_entry->evtype);
	}
}

int main(int argc, char* argv[]) {
	int result = 0;
	int rd, ctr;
	char keyStates[KEY_MAP_SIZE];
	char* evdev = NULL;

	key_map_t key_map[KEY_MAP_SIZE];
	key_map_t hotkey_map[KEY_MAP_SIZE];

	int detach = 0;
	int opt;
	while ((opt = getopt(argc, argv, "dse:")) != -1) {
		switch (opt) {
			case 'd':
				detach = 1;
				break;
			case 's':
				use_syslog = 1;
				break;
	  case 'e':
		evdev = optarg;
		break;
			default:
				fprintf(stderr, "Usage: %s [-d] [-s] [-e eventPath]\n", argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}

    initialize_default_keymap(key_map, hotkey_map);
    initialize_hotkeys(key_map, hotkey_map);

	SYSLOG(LOG_NOTICE, "Starting.");

	printf("[Xarcade2Joystick] Getting exclusive access: ");
	result = input_xarcade_open(&xarcdev, INPUT_XARC_TYPE_TANKSTICK, evdev);
	if (result != 0) {
		if (errno == 0) {
			printf("Not found.\n");
			SYSLOG(LOG_ERR, "Xarcade not found, exiting.");
		} else {
			printf("Failed to get exclusive access to Xarcade: %d (%s)\n", errno, strerror(errno));
			SYSLOG(LOG_ERR, "Failed to get exclusive access to Xarcade, exiting: %d (%s)", errno, strerror(errno));
		}
		exit(EXIT_FAILURE);
	}

	SYSLOG(LOG_NOTICE, "Got exclusive access to Xarcade.");

	uinput_gpad_open(&uinp_gpads[0], UINPUT_GPAD_TYPE_XARCADE, 1);
	uinput_gpad_open(&uinp_gpads[1], UINPUT_GPAD_TYPE_XARCADE, 2);
	uinput_kbd_open(&uinp_kbd);

	if (detach) {
		if (daemon(0, 1)) {
			perror("daemon");
			return 1;
		}
	}
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	SYSLOG(LOG_NOTICE, "Running.");

	while (1) {
		rd = input_xarcade_read(&xarcdev);
		if (rd < 0) {
			break;
		}

		for (ctr = 0; ctr < rd; ctr++) {
			int skip_key_up_count = 0;
			key_map_t *p_map_entry = NULL;
			key_map_t *p_hk_entry = NULL;

			if (xarcdev.ev[ctr].type == 0)
				continue;
			if (xarcdev.ev[ctr].type == EV_MSC)
				continue;
			if (EV_KEY == xarcdev.ev[ctr].type) {

				keyStates[xarcdev.ev[ctr].code] = xarcdev.ev[ctr].value;
				p_map_entry = &key_map[xarcdev.ev[ctr].code];
				p_hk_entry = &hotkey_map[xarcdev.ev[ctr].code];

				if (!p_map_entry->hot_key) {
                    send_key(p_map_entry, xarcdev.ev[ctr].value);
                } else {
					if (xarcdev.ev[ctr].value) {
						/* Ignore all key down events for hotkeys */
						continue;
					}
					if (keyStates[p_hk_entry->hot_key]) {
						/* The main hotkey associated with this key
						   is in keydown state so we have a hotkey hit! */
						send_key(p_hk_entry, 1);
						if (p_hk_entry->destination == game_pad) {
							uinput_gpad_sleep();
						} else {
							uinput_kbd_sleep();
						}
						send_key(p_hk_entry, 0);
						/* TODO: verify this needs to be 2, this current behavior
						is modeled after the existing code.
						*/
						skip_key_up_count = 2;
                    } else {
                        /* not a hotkey hit */
                        if (skip_key_up_count > 0) {
                            /* We are still in recovery from the last hotkey hit,
                                so we are going to ignore this keyup event. */
                            skip_key_up_count -= 1;
                        } else {
                            /* Since we filtered the key down event, we need to
                               create both down and up events.
                            */
                            send_key(p_map_entry, 1);
                            if (p_map_entry->destination == game_pad) {
                                uinput_gpad_sleep();
                            } else {
                                uinput_kbd_sleep();
                            }
                            send_key(p_map_entry, 0);
                        }
                    }
                }
            }
        }
    }

	teardown();
	return EXIT_SUCCESS;
}

static void teardown() {
	printf("Exiting.\n");
	SYSLOG(LOG_NOTICE, "Exiting.");
	
	input_xarcade_close(&xarcdev);
	uinput_gpad_close(&uinp_gpads[0]);
	uinput_gpad_close(&uinp_gpads[1]);
	uinput_kbd_close(&uinp_kbd);
}

static void signal_handler(int signum) {
	signal(signum, SIG_DFL);

	printf("Received signal %d (%s), exiting.\n", signum, strsignal(signum));
	SYSLOG(LOG_NOTICE, "Received signal %d (%s), exiting.", signum, strsignal(signum));
	teardown();
	exit(EXIT_SUCCESS);
}
