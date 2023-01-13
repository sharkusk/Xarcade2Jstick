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

#ifdef DEBUG_X2J
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif

UINP_KBD_DEV uinp_kbd;
UINP_GPAD_DEV uinp_gpads[GPADSNUM];
INP_XARC_DEV xarcdev;
int use_syslog = 0;

enum key_state {
    key_up = 0,
    key_down = 1,
    key_repeat = 2,
    key_down_filtered,
    shifted_key_sent
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
	key_map_t shiftkey_map[KEY_MAP_SIZE];

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

    initialize_default_keymap(key_map, shiftkey_map);
    initialize_shiftkeys(key_map, shiftkey_map);

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

	if (uinput_gpad_open(&uinp_gpads[0], UINPUT_GPAD_TYPE_XARCADE, 1) == -1) {
        printf("Could not open gamepad 1. Exiting\n");
        SYSLOG(LOG_ERR, "Could not open gamepad 1. Exiting");
		exit(EXIT_FAILURE);
    }
	if (uinput_gpad_open(&uinp_gpads[1], UINPUT_GPAD_TYPE_XARCADE, 2) == -1) {
        printf("Could not open gamepad 2. Exiting\n");
        SYSLOG(LOG_ERR, "Could not open gamepad 2. Exiting");
		exit(EXIT_FAILURE);
    }
	if (uinput_kbd_open(&uinp_kbd) == -1) {
        printf("Could not open virtual keyboard. Exiting\n");
        SYSLOG(LOG_ERR, "Could not open virtual keyboard. Exiting");
		exit(EXIT_FAILURE);
    }

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
			key_map_t *p_shift_entry = NULL;
            struct input_event *p_ev = &xarcdev.ev[ctr];

			if (p_ev->type == 0)
				continue;
			if (p_ev->type == EV_MSC)
				continue;
			if (EV_KEY == p_ev->type) {

				p_map_entry = &key_map[p_ev->code];
				p_shift_entry = &shiftkey_map[p_ev->code];

				if (p_map_entry->shift_code == 0 || 
                        (p_map_entry->shift_code != 1 && keyStates[p_map_entry->shift_code] == key_up)) {
                    if (p_ev->value < 2) {
                        /* Only print for keyup and keydown */
                        DEBUG_PRINT("Normal key %d -> %d detected with event %d, shift: %d, value: %d\n", p_ev->code, p_map_entry->code, p_ev->value, p_map_entry->shift_code, p_ev->value);
                    }
                    /* There is no shift_code associated with this key so we can
                     * blindly pass through all events */
                    send_key(p_map_entry, p_ev->value);
                    if (p_ev->value > 1) {
                        /* Don't track states other than up and down */
                        keyStates[p_ev->code] = key_down;
                    } else {
                        keyStates[p_ev->code] = key_up;
                    }
                } else {
                    if (p_ev->value < 2) {
                        /* Only print for keyup and keydown */
                        if (p_map_entry->shift_code == 1) {
                            DEBUG_PRINT("Shift key %d detected with event %d\n", p_ev->code, p_ev->value);
                        } else {
                            DEBUG_PRINT("Potenial Shifted key %d -> %d:%d (%d:%d) detected with event %d\n",
                                    p_ev->code, p_map_entry->code, keyStates[p_map_entry->code],
                                    p_map_entry->shift_code, keyStates[p_map_entry->shift_code], p_ev->value);
                        }
                    }
                    if (p_ev->value == 1) { /* down event */
                        DEBUG_PRINT("  Key down detected with shift. Filtering keydown.\n");
                        keyStates[p_ev->code] = key_down_filtered;
                    /* end of ... if (p_ev->value == 1) */
                    } else if (p_ev->value == 0) { /* up event */
                        DEBUG_PRINT("  Key up detected\n");
                        key_map_t *p_entry = NULL;

                        /* First check if this is a shiftkey, in which case we
                         * need to check if a shiftkey had been sent, in which
                         * case we don't need to do anything.
                         */
                        switch (keyStates[p_ev->code]) {
                        case key_down:
                            DEBUG_PRINT("  No shift events, sending key\n");
                            p_entry = p_map_entry;
                            break;
                        case shifted_key_sent:
                            DEBUG_PRINT("  Shift key(s) sent, filtering key_up\n");
                            keyStates[p_ev->code] = key_up;
                            continue;
                        case key_up:
                            /* This key is tied to a shiftkey and this trigger
                             * means we should send the special key as long as
                             * the shift key is still depressed
                             */
                            if (keyStates[p_map_entry->shift_code] != key_up) {
                                DEBUG_PRINT("  Sending shifted key\n");
                                keyStates[p_map_entry->shift_code] = shifted_key_sent;
                                p_entry = p_shift_entry;
                            } else {
                                DEBUG_PRINT("  Sending non-shifted key\n");
                                p_entry = p_map_entry;
                            }
                            break;
                        case key_down_filtered:
                            /* We filtered this key, but never got a paired
                             * key, so we can send the normal key
                             */
                            if (p_map_entry->shift_code == 1) {
                                DEBUG_PRINT("  No shift keys sent, sending original shift key\n");
                                p_entry = p_map_entry;
                            } else if (keyStates[p_map_entry->shift_code] != key_up) {
                                DEBUG_PRINT("  Sending shifted key\n");
                                keyStates[p_map_entry->shift_code] = shifted_key_sent;
                                p_entry = p_shift_entry;
                            }
                            break;
                        default:
                            DEBUG_PRINT("  Unknown key state! %d\n", keyStates[p_ev->code]);
                            continue;
                        }
                        keyStates[p_ev->code] = key_up;
						send_key(p_entry, 1);
						if (p_entry->destination == game_pad) {
							uinput_gpad_sleep();
						} else {
							uinput_kbd_sleep();
						}
						send_key(p_entry, 0);
                    /* end of ... else if (p_ev->value == 0) */
                    } else {
                        DEBUG_PRINT("Ignored event for Shift key %d (%d) with event: %d\n", p_map_entry->code, p_map_entry->shift_code, p_ev->value);
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
