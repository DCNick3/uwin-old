//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include "kobj.h"
#include "uwin/util/log.h"

#include <cstring>
#include <cstdlib>

#include <sys/types.h>
#include <dirent.h>


DIR* uw_file_opendir(const char* file_name);

namespace uwin {
    class kdir : public kobj {
    public:
        kdir(const char* path) : _dir(uw_file_opendir(path)) {}

        struct destroyer {
            void operator()(DIR* dp) {
                closedir(dp);
            }
        };

        inline DIR* get() { return _dir.get(); }

        inline int32_t read(char* buffer, uint32_t size) {
            struct dirent* entry = readdir(get());

            uw_log("g_dir_read_name() -> %p (%s)\n", entry, entry ? entry->d_name : "<NULL>");

            if (entry == nullptr)
                return -1;

            uint32_t l = strlen(entry->d_name) + 1;
            if (l < size)
                l = size;
            memcpy(buffer, entry->d_name, l);
            buffer[l-1] = '\0';

            return 0;
        }

    private:
        std::unique_ptr<DIR, destroyer> _dir;
    };
}