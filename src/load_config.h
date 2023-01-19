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
#ifndef LOAD_CONFIG_H
#define LOAD_CONFIG_H

#include "key_map.h"

#define MAX_DEVICE_PATH_SIZE 256

struct k2j_config {
    char device_path[MAX_DEVICE_PATH_SIZE];
    int num_virtual_game_pads;
    key_map_t key_map[KEY_MAP_SIZE];
    key_map_t shiftkey_map[KEY_MAP_SIZE];
};

int load_config(const char *config_path, struct k2j_config *p_config);

#endif
