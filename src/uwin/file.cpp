/*
 *  file helper for uwin
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

extern "C" {
#include "uwin/uwin.h"
#include "uwin/util/str.h"
#include "uwin/util/mem.h"

#include "uwin/uwinerr.h"
#include "fcaseopen.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

// hopefully posix...
// needed for opendir and stat
// more or less portable
#include <sys/statvfs.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
}

#include <string>

// should fopen or open be used? dunno...

struct uw_file {
    FILE* f;
};

static char* current_directory;
static pthread_mutex_t curdir_mutex;
static char* host_base_path = NULL;

void uw_file_initialize(void) {
    current_directory = uw_strdup(UW_GUEST_PROG_PATH);
    
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&curdir_mutex, &attr);
}
void uw_file_finalize(void) {
    pthread_mutex_destroy(&curdir_mutex);
    uw_free(current_directory);
    uw_free(host_base_path);
}

// Do I want C++? I don't know... But this function requires it
// Dragons be here
static char* target_realpath(const char* file_name) {
    uw_log("target_realpath(%s)\n", file_name);
    pthread_mutex_lock(&curdir_mutex);
    char* copy = uw_strdup(file_name);

    // I would like to work only with forward slashes
    for (char* c = copy; *c != '\0'; c++)
        if (*c == '\\')
            *c = '/';

    if (copy[1] != ':' || copy[2] != '/')
    {
        if (copy[0] == '/') {
            char* new_copy = uw_strconcat(current_directory, copy, NULL);
            uw_free(copy);
            copy = new_copy;
        } else {
            char* new_copy = uw_strconcat(current_directory, "/", copy, NULL);
            uw_free(copy);
            copy = new_copy;
        }
    }

    for (char* c = copy; *c != '\0'; c++)
        if (*c == '\\')
            *c = '/';

    assert(copy[0] == 'C' || copy[0] == 'c');
    
    std::string out_buffer = "C:/";//(char*)uw_malloc(strlen(copy) + 1); // it can't get bigger
    const char* pcopy = copy + 3;


    //out_buffer[0] = 0;
    //strcat(out_buffer, "C:/");
    //int out_buffer_length = 3;

    std::string component_buffer;//(char*)uw_malloc(strlen(copy) + 1); // it can't get bigger
    //int component_length = 0;

    int end = 0;
    while (!end) {
        while (*pcopy != '\0' && *pcopy != '/') {
            component_buffer += *pcopy;
            pcopy++;
        }
        
        if (*pcopy == '\0') {
            // we parsed the final component - file name (assuming it is a file name indeed)
            //g_string_append(norm_path_str, norm_path_component->str);
            end = 1;
        }
        
        // deal with "some/./path" and "some//path"
        if (component_buffer == "." || component_buffer.length() == 0) {
            // do nothing with it
            pcopy++;
            component_buffer.clear();
            continue;
        }
        
        if (component_buffer == "..") {
            pcopy++;
            component_buffer.clear();
            int p = out_buffer.size() - 2;
            if (p != 1) // we have only C:/ left
            {
                while (out_buffer[p] != '/')
                    p--;
                out_buffer.erase(p + 1);
                //out_buffer[p + 1] = 0;
            }
            continue;
        }
        
        // nothing special, just append it
        //out_buffer[out_buffer_length] = 0;
        out_buffer += component_buffer;
        out_buffer += '/';
        //memcpy(out_buffer + out_buffer_length, component_buffer, component_length);
        //out_buffer_length += component_length;
        //out_buffer[out_buffer_length++] = '/';

        component_buffer.clear();
        //component_length = 0;
        pcopy++;

        assert(*out_buffer.rbegin() == '/');
    }
    //uw_free(component_buffer);

    //out_buffer[out_buffer_length - 1] = 0;

    out_buffer.erase(out_buffer.size() - 1);

    //uw_free(copy);
    copy = uw_strdup(out_buffer.c_str());
    //uw_free(out_buffer);
    
    pthread_mutex_unlock(&curdir_mutex);
    
    return copy;
}

static char* translate_path(const char* file_name) {
    uw_log("translate_path(%s)\n", file_name);
    
    pthread_mutex_lock(&curdir_mutex);
    
    char* copy = target_realpath(file_name);

    if (copy[0] != 'C' && copy[0] != 'c') {
        uw_free(copy);
        pthread_mutex_unlock(&curdir_mutex);
        return NULL; // nothing besides drive C, I swear
    }
    
    // now normalize the path (remove . and .. and stuff)
    uw_log("normalized path is %s\n", copy);
    
    const char* pvpath = UW_GUEST_PROG_PATH;
    const char* pcopy = copy;
    while (*pvpath != '\0' && *pcopy != '\0') {
        if (*pvpath != *pcopy && (*pvpath != '\\' || *pcopy != '/'))
            break;
        pvpath++;
        pcopy++;
    }
    if (*pvpath != '\0') // something doesn't match. Tell the game that there is nothing besides it's directory =)
    {
        uw_free(copy);
        pthread_mutex_unlock(&curdir_mutex);
        return NULL;
    }

    uw_log("path relative to current (virtual) directory is %s\n", pcopy);
    
    char* host_path = uw_strconcat(host_base_path, pcopy, NULL);
    uw_free(copy);
    pthread_mutex_unlock(&curdir_mutex);
    
    return host_path;
}

static const char* mode_to_fopen_mode(int mode) {
    // TODO: support for creation disposition is kinda lacking with this...
    switch (mode) {
        case UW_FILE_READ:                                                          return "rb";
        case UW_FILE_WRITE | UW_FILE_CREATE | UW_FILE_TRUNCATE:                     return "wb";
        case UW_FILE_WRITE | UW_FILE_CREATE | UW_FILE_APPEND:                       return "ab";
        case UW_FILE_READ  | UW_FILE_WRITE:                                         return "r+b";
        case UW_FILE_READ  | UW_FILE_WRITE  | UW_FILE_CREATE | UW_FILE_TRUNCATE:    return "w+b";
        case UW_FILE_READ  | UW_FILE_WRITE  | UW_FILE_CREATE | UW_FILE_APPEND:      return "a+b";
        default:  
            fprintf(stderr, "Unsupported mode: %d\n", mode);
            abort();
    }
}

static uint32_t errno_to_win32_error(void) {
    return ERROR_FILE_NOT_FOUND;
}

static int seek_origin_to_fseek_whence(int origin) {
    switch (origin) {
        case UW_FILE_SEEK_BGN: return SEEK_SET;
        case UW_FILE_SEEK_CUR: return SEEK_CUR;
        case UW_FILE_SEEK_END: return SEEK_END;
        default: 
            fprintf(stderr, "Unsupported origin: %d\n", origin);
            abort();
    }
}

void uw_file_set_host_directory(const char* path) {
    //assert(g_file_test(path, G_FILE_TEST_IS_DIR));

    DIR* dir = opendir(path);
    assert(dir);
    if (dir)
        closedir(dir);

    host_base_path = uw_strdup(path);
    
    size_t l = strlen(path);
    while (host_base_path[l - 1] == '/') {
        host_base_path[l - 1] = '\0';
        l--;
    }

    // Heroes of Might and Magic III specific:
    // create 'random_maps' and 'games' directories (homm3 does not create it automatically, but GoG distribution (apparently) does not contain it
    char* p = uw_strconcat(host_base_path, "/random_maps", NULL);
    dir = opendir(p);
    if (dir)
        closedir(dir);
    else
        mkdir(p, 0755);
    uw_free(p);

    p = uw_strconcat(host_base_path, "/games", NULL);
    dir = opendir(p);
    if (dir)
        closedir(dir);
    else
        mkdir(p, 0755);
    uw_free(p);
}

uw_file_t* uw_file_open(const char* file_name, int mode)
{
    //qemu_log("uw_file_open(%s, %d)\n", file_name, mode);
    
    char* host_path = translate_path(file_name);
    if (host_path == NULL) {
        win32_err = ERROR_FILE_NOT_FOUND;
        return NULL;
    }
    
    uw_log("fcaseopen(%s)\n", host_path);
    
    FILE* f = fcaseopen(host_path, mode_to_fopen_mode(mode));
    uw_free(host_path);
    
    if (f == NULL) {
        win32_err = errno_to_win32_error();
        return NULL;
    }
    
    win32_err = ERROR_SUCCESS;
    
    uw_file_t* r = uw_new(uw_file_t, 1);
    r->f = f;
    return r;
}

int32_t uw_file_read(uw_file_t* file, char* buffer, uint32_t length)
{
    int32_t r = fread(buffer, 1, length, file->f);
    if (r == 0) {
        if (ferror(file->f)) {
            assert("IO error occured" == 0);
        }
    }
    
    win32_err = ERROR_SUCCESS;
    
    // TODO: error handling
    return r;
}

int32_t uw_file_write(uw_file_t* file, const char* buffer, uint32_t length)
{
    int32_t r = fwrite(buffer, 1, length, file->f);
    win32_err = ERROR_SUCCESS;
    // TODO: error handling
    return r;
}

int32_t uw_file_seek(uw_file_t* file, int64_t seek, uint32_t origin)
{
    int whence = seek_origin_to_fseek_whence(origin);
    win32_err = ERROR_SUCCESS;
    //qemu_log("fseek(%p, %lx, %d)\n", file->f, seek, whence);
    return fseek(file->f, seek, whence);
}

int64_t uw_file_tell(uw_file_t* file)
{
    win32_err = ERROR_SUCCESS;
    return ftell(file->f);
}

void uw_file_close(uw_file_t* file)
{
    fclose(file->f);
    uw_free(file);
}

void uw_file_get_free_space(uint64_t* free_bytes, uint64_t* total_bytes) {
    struct statvfs buf;
    int r = statvfs(host_base_path, &buf);
    assert(!r);
    *free_bytes  = (uint64_t)buf.f_frsize * buf.f_bavail;
    *total_bytes = (uint64_t)buf.f_frsize * buf.f_blocks;
}

int32_t uw_file_set_current_directory(const char* new_current_directory)
{
    pthread_mutex_lock(&curdir_mutex);
    
    char* real_new = target_realpath(new_current_directory);
    
    char* host_path = translate_path(real_new);
    uw_log("host_path = %s\n", host_path);
    
    DIR* dir = caseopendir(host_path);
    if (dir)
        closedir(dir);
    else {
        uw_log("directory does not exist, returning error\n");
        win32_err = ERROR_FILE_NOT_FOUND;
        pthread_mutex_unlock(&curdir_mutex);
        return -1;
    }
    
    uw_free(host_path);
    uw_free(current_directory);
    current_directory = real_new;
    
    uw_log("cd '%s'\n", real_new);
    
    pthread_mutex_unlock(&curdir_mutex);
    return 0;
}

void uw_file_get_current_directory(char* buffer, uint32_t size)
{
    pthread_mutex_lock(&curdir_mutex);
    
    uint32_t l = strlen(current_directory) + 1;
    if (l > size)
        l = size;
    memcpy(buffer, current_directory, l);
    buffer[l - 1] = '\0';
    
    pthread_mutex_unlock(&curdir_mutex);
}

static void stat_buf_to_uw_file_stat(struct stat *buf, uw_file_stat_t* stat_res)
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

void uw_file_stat(const char* file_name, uw_file_stat_t* stat_res)
{
    struct stat buf;
    char* path = translate_path(file_name);
    
    int r = casestat(path, &buf);
    uw_free(path);
    assert(!r);
    
    stat_buf_to_uw_file_stat(&buf, stat_res);
}

void uw_file_fstat(uw_file_t* file, uw_file_stat_t* stat_res) 
{
    struct stat buf;
    
    int r = fstat(fileno(file->f), &buf); // meh. glib sucks here
    assert(!r);
    
    stat_buf_to_uw_file_stat(&buf, stat_res);
}

struct uw_dir {
    DIR* dir;
};

uw_dir_t* uw_file_opendir(const char* file_name)
{
    char* path = translate_path(file_name);
    
    uw_log("g_dir_caseopen(%s)\n", path);
    
    DIR* dir = caseopendir(path);
    uw_free(path);
    assert(dir);

    uw_dir_t* res = uw_new(uw_dir_t, 1);
    res->dir = dir;

    return res;
}

int32_t uw_file_readdir(uw_dir_t* dir, char* buffer, uint32_t size)
{
    struct dirent* entry = readdir(dir->dir);
    
    uw_log("g_dir_read_name() -> %p (%s)\n", entry, entry ? entry->d_name : "<NULL>");
    
    if (entry == NULL)
        return -1;
    
    uint32_t l = strlen(entry->d_name) + 1;
    if (l < size)
        l = size;
    memcpy(buffer, entry->d_name, l);
    buffer[l-1] = '\0';
    
    return 0;
}

void uw_file_closedir(uw_dir_t* dir) {
    closedir(dir->dir);
    uw_free(dir);
}
