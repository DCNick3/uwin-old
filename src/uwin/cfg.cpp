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
#include "inipp.h"

#include <cstdio>
#include <cstring>
#include <vector>
#include <fstream>

// no need for mutexes here, as config is accessed only from the main thread
static inipp::Ini<char> config_file;

#define UW_CONFIG_PATH "homm3.cfg"
#define UW_TARGET_CONFIG_GROUP "homm3_config"

void uw_cfg_initialize(void)
{
    std::ifstream f(UW_CONFIG_PATH);
    if (!f.is_open())
        config_file.clear();
    else {
        config_file.parse(f);
    }
}

void uw_cfg_finalize(void)
{
    uw_cfg_commit();
}

void uw_cfg_write(const char* key, const char* value)
{
    config_file.sections[UW_TARGET_CONFIG_GROUP][key] = value;
}
int32_t uw_cfg_read(const char* key, char* buffer, uint32_t size)
{
    auto const& section = config_file.sections[UW_TARGET_CONFIG_GROUP];
    auto it = section.find(key);
    if (it == section.end())
        return -1;
    auto const& data = it->second;
    int32_t ret = data.size() + 1;

    if (size != 0) {
        *buffer = '\0';
        strncat(buffer, data.c_str(), size - 1);
    }

    return ret;
}

void uw_cfg_commit(void)
{
    std::ofstream f(UW_CONFIG_PATH);

    config_file.generate(f);
}
