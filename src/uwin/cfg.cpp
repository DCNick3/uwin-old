/*
 *  uwin layer registry helper
 *
 *  Copyright (c) 2020 Nikita Strygin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "uwin/uwin.h"
#include "uwin/util/mem.h"
#include "ini.h"

#include <stdio.h>
#include <string.h>

// based on mattiasgustavsson's ini.h

// no need for mutexes here, as config is accessed only from the main thread
static ini_t* config_file;


void uw_cfg_initialize(void)
{
    FILE* f = fopen(UW_CONFIG_PATH, "r");
    if (f == NULL)
        config_file = ini_create(NULL);
    else {

        fseek(f, 0L, SEEK_END);
        size_t size = ftell(f);
        fseek(f, 0L, SEEK_SET);
        auto *contents = (char*)uw_malloc(size + 1);
        fread(contents, 1, size, f);
        fclose(f);

        contents[size] = 0;

        config_file = ini_load(contents, NULL);

        uw_free(contents);
    }
}

void uw_cfg_finalize(void)
{
    uw_cfg_commit();
    ini_destroy(config_file);
}

void uw_cfg_write(const char* key, const char* value)
{
    int section_index = ini_find_section(config_file, UW_TARGET_CONFIG_GROUP, 0);
    if (section_index == INI_NOT_FOUND) {
        section_index = ini_section_add(config_file, UW_TARGET_CONFIG_GROUP, 0);
    }
    int property_index = ini_find_property(config_file, section_index, key, 0);
    if (property_index == INI_NOT_FOUND) {
        ini_property_add(config_file, section_index, key, 0, value, 0);
    } else {
        ini_property_value_set(config_file, section_index, property_index, value, 0);
    }
}
int32_t uw_cfg_read(const char* key, char* buffer, uint32_t size)
{
    int section_index = ini_find_section(config_file, UW_TARGET_CONFIG_GROUP, 0);
    if (section_index == INI_NOT_FOUND)
        return -1;
    int property_index = ini_find_property(config_file, section_index, key, 0);
    if (property_index == INI_NOT_FOUND)
        return -1;

    const char* val = ini_property_value(config_file, section_index, property_index);

    uint32_t ret = strlen(val) + 1;
    if (size != 0) {
        *buffer = '\0';
        strncat(buffer, val, size - 1);
    }
    return ret;
}

void uw_cfg_commit(void)
{
    int size = ini_save(config_file, NULL, 0);
    auto* data = (char*)uw_malloc(size);
    ini_save(config_file, data, size);


    FILE* f = fopen(UW_CONFIG_PATH, "w");
    fwrite(data, 1, size, f);
    fclose(f);

    uw_free(data);
}
