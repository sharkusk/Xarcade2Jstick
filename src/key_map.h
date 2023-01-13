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

#ifndef KEY_MAP_H
#define KEY_MAP_H

#include <stdint.h>

typedef struct key_map_t {
	/* char key, -- defined by position in key_map table */
	enum dest_type { pass_through, keyboard, game_pad } destination;
	int game_pad_index; /* ignored for keyboard destination */
	uint16_t code;		/* code we are going to send*/
	uint16_t value_low; /* value to send when ev == 0 */
	uint16_t value_high;/* value to send when ev > 0 */
	uint16_t evtype;    /* type of event to send */

	/* In the main key map shift_code will be a 0 for normal key,
	   and a 1 for a shift_code.  This will autofill based on the shift_code map.

	   In the shiftkey map, this will be the keycode for the associated
	   shift_key that must be already pressed for the code to be sent.
	*/
	uint8_t shift_code; 
} key_map_t;

# define KEY_MAP_SIZE 256

void set_keymap(key_map_t key_map[], uint8_t key, enum dest_type destination, int game_pad_index, uint16_t code, uint16_t value_low, uint16_t value_high, uint16_t evtype, uint8_t shift_code);
void initialize_default_keymap(key_map_t key_map[], key_map_t shiftkey_map[]);
void initialize_shiftkeys(key_map_t key_map[], key_map_t shiftkey_map[]);

#endif /* KEY_MAP_H */
