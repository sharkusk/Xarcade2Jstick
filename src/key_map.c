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
/*                           (c) 2023, Sharkus                              */
/* ======================================================================== */

#include <string.h>
#include <linux/input.h>
#include <stdint.h>
#include "key_map.h"

void set_keymap(key_map_t key_map[], uint8_t key, enum dest_type destination, int game_pad_index, uint16_t code, uint16_t value_low, uint16_t value_high, uint16_t evtype, uint8_t shift_code)
{
    key_map[key].destination = destination;
    key_map[key].game_pad_index = game_pad_index;
    key_map[key].code = code;
    key_map[key].value_high = value_high;
    key_map[key].value_low = value_low;
    key_map[key].evtype = evtype;
    key_map[key].shift_code = shift_code;
}

void initialize_default_keymap(key_map_t key_map[], key_map_t shiftkey_map[])
{
	memset(key_map, 0, sizeof(key_map_t)*KEY_MAP_SIZE);
	memset(shiftkey_map, 0, sizeof(key_map_t)*KEY_MAP_SIZE);

	/* Player 1 */
    set_keymap(key_map, KEY_LEFTCTRL, game_pad, 0, BTN_WEST, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_LEFTALT, game_pad, 0, BTN_NORTH, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_SPACE, game_pad, 0, BTN_TL, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_LEFTSHIFT, game_pad, 0, BTN_SOUTH, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_Z, game_pad, 0, BTN_EAST, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_X, game_pad, 0, BTN_TR, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_C, game_pad, 0, BTN_EXTRA, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_3, game_pad, 0, BTN_MODE, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_V, game_pad, 0, BTN_MODE, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_1, game_pad, 0, BTN_START, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_5, game_pad, 0, BTN_SELECT, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_KP4, game_pad, 0, ABS_X, 2, 0, EV_ABS, 0);
	set_keymap(key_map, KEY_LEFT, game_pad, 0, ABS_X, 2, 0, EV_ABS, 0);
	set_keymap(key_map, KEY_KP6, game_pad, 0, ABS_X, 2, 4, EV_ABS, 0);
	set_keymap(key_map, KEY_RIGHT, game_pad, 0, ABS_X, 2, 4, EV_ABS, 0);
	set_keymap(key_map, KEY_KP8, game_pad, 0, ABS_Y, 2, 0, EV_ABS, 0);
	set_keymap(key_map, KEY_UP, game_pad, 0, ABS_Y, 2, 0, EV_ABS, 0);
	set_keymap(key_map, KEY_KP2, game_pad, 0, ABS_Y, 2, 4, EV_ABS, 0);
	set_keymap(key_map, KEY_DOWN, game_pad, 0, ABS_Y, 2, 4, EV_ABS, 0);

	/* Player 2 */
	set_keymap(key_map, KEY_A, game_pad, 1, BTN_WEST, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_S, game_pad, 1, BTN_NORTH, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_Q, game_pad, 1, BTN_TL, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_W, game_pad, 1, BTN_SOUTH, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_E, game_pad, 1, BTN_EAST, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_I, game_pad, 1, BTN_EAST, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_LEFTBRACE, game_pad, 1, BTN_TR, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_K, game_pad, 1, BTN_TR, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_RIGHTBRACE, game_pad, 1, BTN_EXTRA, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_J, game_pad, 1, BTN_EXTRA, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_4, game_pad, 1, BTN_MODE, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_L, game_pad, 1, BTN_MODE, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_2, game_pad, 1, BTN_START, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_6, game_pad, 1, BTN_SELECT, 0, 1, EV_KEY, 0);
	set_keymap(key_map, KEY_D, game_pad, 1, ABS_X, 2, 0, EV_ABS, 0);
	set_keymap(key_map, KEY_G, game_pad, 1, ABS_X, 2, 4, EV_ABS, 0);
	set_keymap(key_map, KEY_R, game_pad, 1, ABS_Y, 2, 0, EV_ABS, 0);
	set_keymap(key_map, KEY_F, game_pad, 1, ABS_Y, 2, 4, EV_ABS, 0);

	/* Hotkeys */
	set_keymap(shiftkey_map, KEY_5, keyboard, 0, KEY_TAB, 0, 1, EV_KEY, KEY_1);
	set_keymap(shiftkey_map, KEY_6, keyboard, 0, KEY_ESC, 0, 1, EV_KEY, KEY_2);
}

void initialize_shiftkeys(key_map_t key_map[], key_map_t shiftkey_map[])
{
    int i = 0;
	/* 
	   We only require the user to program the shiftkey info in one place,
	   but we want the tables to be consistent so fill this in ourselves.
	*/
	for (i=0; i<KEY_MAP_SIZE; i++) {
		if (shiftkey_map[i].shift_code != 0) {
            /* Shifted keys have the value of the associated shift key */
			key_map[i].shift_code = shiftkey_map[i].shift_code; 
            /* Shift keys have value of 1 */
			key_map[shiftkey_map[i].shift_code].shift_code = 1;
		}
		if (key_map[i].destination == pass_through) {
			key_map[i].code = i;
			key_map[i].evtype = EV_KEY;
		}
	}
}

