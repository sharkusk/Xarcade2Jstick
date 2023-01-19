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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "load_config.h"
#include "key_map.h"

static int parse_config_line(char *line, struct k2j_config *p_config)
{
    int result = 0;
    char token[64];
    char value[MAX_DEVICE_PATH_SIZE];

    char *p;

    sscanf(line, "%s = %s", token, value);
    if (strcmp(token, "num_virt_game_pads") == 0) {
        p_config->num_virtual_game_pads = strtol(value, &p, 10);
    } else if (strcmp(token, "input_device_path") == 0) {
        strncpy(p_config->device_path, value, MAX_DEVICE_PATH_SIZE);
    } else {
        fprintf(stderr, "Unknown config parameter: '%s'\n", token);
        result = -1;
    }

    return result;
}

static int parse_key_map_line(char *line, key_map_t key_map[], int shiftkey)
{
    int result = 0;

    uint16_t key_value;
    char key[64];
    char dest[64];
    int index;
    char code[64];
    int val_high;
    int val_low;
    char evtype[64];
    char shift_code[64];

    key_map_t *p_cur_map = NULL;

    if (sscanf(line, "%64s, %64s, %d, %64s, %d, %d, %64s, %64s", key, dest, &index, code,
            &val_high, &val_low, evtype, shift_code) < 7) {
        fprintf(stderr, "Unknown error reading line: '%s'\n", line);
        return -1;
    }

    result = name_to_code(key, &key_value);
    if (result != 0) {
        fprintf(stderr, "Unknown key: '%s'\n", key);
    } else {
        p_cur_map = &key_map[key_value];
        if (shiftkey == 0) {
            p_cur_map->shift_code = 0;
        } else {
            result = name_to_code(shift_code, &p_cur_map->shift_code);
            if (result != 0) {
                fprintf(stderr, "Unknown shift_code: '%s'\n", shift_code);
            }
        }
    }

    /* TODO:
    p_cur_map->destination = destination;
    */
    p_cur_map->game_pad_index = index;
    result = name_to_code(code, &p_cur_map->code);
    if (result != 0) {
        fprintf(stderr, "Unknown code: '%s'\n", code);
    }
    p_cur_map->value_high = val_high;
    p_cur_map->value_low = val_low;
    result = name_to_code(evtype, &p_cur_map->evtype);
    if (result != 0) {
        fprintf(stderr, "Unknown evtype: '%s'\n", evtype);
    }

    return result;
}

int load_config(const char *config_path, struct k2j_config *p_config)
{
    int result = 0;

    char line[256];

    enum section_type {
        config_section = 0,
        key_map_section,
        shift_key_section
    };
    enum section_type section = 0;

    memset(p_config, 0, sizeof(struct k2j_config));

    FILE *fp = fopen(config_path, "r");
    if (!fp) {
        fprintf(stderr, "Unable to open config file '%s'.\n", config_path);
        return -1;
    }

    while (fgets(line, 255, fp)) {
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        } else if (line[0] == '[') {
            if (strncmp( &line[1], "config", 6) == 0) {
                section = config_section;
            } else if (strncmp( &line[1], "keymap", 6) == 0) {
                section = key_map_section;
            } else if (strncmp( &line[1], "shiftkeys", 9) == 0) {
                section = shift_key_section;
            } else {
                fprintf(stderr, "Unknown section type found '%s'.\n", line);
                result = -1;
                fprintf(stderr, "Aborting config file parsing\n");
                break;
            }
            continue;
        }
        switch (section) {
            case config_section:
                result = parse_config_line(line, p_config);
                break;
            case key_map_section:
                result = parse_key_map_line(line, p_config->key_map, 0);
                break;
            case shift_key_section:
                result = parse_key_map_line(line, p_config->shiftkey_map, 1);
                break;
        }
        if (result != 0) {
            fprintf(stderr, "Aborting config file parsing\n");
            break;
        }
    }

    fclose(fp);
    return result;
}

