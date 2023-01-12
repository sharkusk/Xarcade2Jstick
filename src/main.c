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

enum key_state {
    key_up = 0,
    key_down = 1,
    key_down_filtered,
    hot_key_sent
};

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
	enum key_state keyStates[KEY_MAP_SIZE];
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

	printf("Got exclusive access to Xarcade.\n");
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
			key_map_t *p_map_entry = NULL;
			key_map_t *p_hk_entry = NULL;

			if (xarcdev.ev[ctr].type == 0)
				continue;
			if (xarcdev.ev[ctr].type == EV_MSC)
				continue;
			if (EV_KEY == xarcdev.ev[ctr].type) {

				p_map_entry = &key_map[xarcdev.ev[ctr].code];
				p_hk_entry = &hotkey_map[xarcdev.ev[ctr].code];

				if (!p_map_entry->hot_key) {
                    printf("Normal key %d detected: %d\n", p_map_entry->code, xarcdev.ev[ctr].code);
                    /* There is no hot_key associated with this key so we can
                     * blindly pass through all events */
                    send_key(p_map_entry, xarcdev.ev[ctr].value);
                    keyStates[xarcdev.ev[ctr].code] = xarcdev.ev[ctr].value;
                } else {
                    printf("Hot key %d (%d) detected: %d\n", p_map_entry->code, p_map_entry->hot_key, xarcdev.ev[ctr].code);
                    if (xarcdev.ev[ctr].value == 1) { /* down event */
                        printf("  Key down detected");
                        /* if this is a hotkey we always filter the downstroke.
                         * if this key can be assigned to a hotkey, we need to filter
                         * all down presses if the associated hotkey is depressed.
                         */
                        if (p_map_entry->hot_key == 1 || keyStates[p_map_entry->hot_key] == key_down) {
                            /* filter down stroke .. (don't send anything for now)
                             * We want to track that the hotkey is depressed,
                             * the assigned key state does not change
                             */
                            if (p_map_entry->hot_key == 1) {
                                printf("  Main hot_key detected, marking as filtered\n");
                                keyStates[xarcdev.ev[ctr].code] = key_down_filtered;
                            }
                        } else {
                            printf("  Hotkey not depressed, sending key.\n");
                            send_key(p_map_entry, xarcdev.ev[ctr].value);
                            keyStates[xarcdev.ev[ctr].code] = key_down;
                        }
                    } else if (xarcdev.ev[ctr].value == 0) { /* up event */
                        printf("  Key up detected\n");
                        key_map_t *p_entry = NULL;

                        /* First check if this is a hotkey, in which case we
                         * need to check if a hotkey had been sent, in which
                         * case we don't need to do anything.
                         */
                        switch (keyStates[xarcdev.ev[ctr].code]) {
                        case key_down:
                            send_key(p_map_entry, xarcdev.ev[ctr].value);
                            keyStates[xarcdev.ev[ctr].code] = key_up;
                            continue;
                        case hot_key_sent:
                            keyStates[xarcdev.ev[ctr].code] = key_up;
                            continue;
                        case key_up:
                            /* This key is tied to a hot key and this trigger
                             * means we should send the special key as long as
                             * the hot key is still depressed
                             */
                            if (keyStates[p_map_entry->hot_key] != key_up) {
                                keyStates[p_map_entry->hot_key] = hot_key_sent;
                                p_entry = p_hk_entry;
                            } else {
                                p_entry = p_map_entry;
                            }
                            break;
                        case key_down_filtered:
                            /* We filtered this key, but never got a paired
                             * key, so we can send the normal key
                             */
                            p_entry = p_map_entry;
                            break;
                        default:
                            break;
                        }
						send_key(p_entry, 1);
						if (p_entry->destination == game_pad) {
							uinput_gpad_sleep();
						} else {
							uinput_kbd_sleep();
						}
						send_key(p_entry, 0);
                        keyStates[p_entry->hot_key] = key_up;
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
