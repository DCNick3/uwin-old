//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include "kobj.h"

#include <cassert>

// hopefully posix...
// needed for opendir and stat
// more or less portable
#include <sys/statvfs.h>
#include <sys/stat.h>

#include <fstream>
#include <memory>


FILE* uw_file_open(const char* file_name, int mode);

namespace uwin {
    class kfile : public kobj {
    public:
        inline kfile(FILE* f) : _file(f) {}
        inline kfile(const char* filename, int mode) : _file(uw_file_open(filename, mode)) {}

        struct destroyer {
            void operator()(FILE* fp) {
                fclose(fp);
            }
        };

        inline FILE* get() { return _file.get(); }

        inline int64_t tell() { return ftell(get()); }
        inline int32_t seek(int64_t seek, uint32_t origin) { return fseek(get(), seek, seek_origin_to_fseek_whence(origin)); }
        inline int32_t write(const char* buffer, uint32_t length) { return fwrite(buffer, 1, length, get()); }
        inline int32_t read(char* buffer, uint32_t length) {
            int32_t res = fread(buffer, 1, length, get());
            if (res == 0) {
                if (ferror(get()))
                    std::terminate();
            }
            return res;
        }
        inline void stat(uw_file_stat_t* stat_res) {
            struct stat buf;

            int r = fstat(fileno(get()), &buf);
            assert(!r);

            stat_buf_to_uw_file_stat(&buf, stat_res);
        }

    private:

        inline static int seek_origin_to_fseek_whence(uint32_t origin) {
            switch (origin) {
                case UW_FILE_SEEK_BGN: return SEEK_SET;
                case UW_FILE_SEEK_CUR: return SEEK_CUR;
                case UW_FILE_SEEK_END: return SEEK_END;
                default:
                    fprintf(stderr, "Unsupported origin: %d\n", origin);
                    abort();
            }
        }


        inline static void stat_buf_to_uw_file_stat(struct stat *buf, uw_file_stat_t* stat_res)
        {
            if ((buf->st_mode & S_IFMT) == S_IFREG)
                stat_res->mode = UW_FILE_MODE_FILE;
            else if ((buf->st_mode & S_IFMT) == S_IFDIR)
                stat_res->mode = UW_FILE_MODE_DIR;
            else
                assert("Unknown file mode" == 0);

            stat_res->size = buf->st_size;

            stat_res->atime = buf->st_atime;
            stat_res->mtime = buf->st_mtime;
            stat_res->ctime = buf->st_ctime;
        }

        std::unique_ptr<FILE, destroyer> _file;
    };
}
