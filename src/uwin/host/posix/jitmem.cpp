
#include "uwin/uwin.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>

// I attempted not to include any linux-isms, so this should be quite portable
static int create_tempfile(void)
{
    int fd;
    char tmp_name[256];
    size_t tmp_name_len = 0;
    char *dir;
    struct stat st;

    dir = getenv("TMPDIR");

    if (dir) {
        tmp_name_len = strlen(dir);
        if (tmp_name_len > 0 && tmp_name_len < sizeof(tmp_name)) {
            if ((stat(dir, &st) == 0) && S_ISDIR(st.st_mode))
                strcpy(tmp_name, dir);
        }
    }

#ifdef P_tmpdir
    if (!tmp_name_len) {
        tmp_name_len = strlen(P_tmpdir);
        if (tmp_name_len > 0 && tmp_name_len < sizeof(tmp_name))
            strcpy(tmp_name, P_tmpdir);
    }
#endif
    if (!tmp_name_len) {
        strcpy(tmp_name, "/tmp");
        tmp_name_len = 4;
    }

    assert(tmp_name_len > 0 && tmp_name_len < sizeof(tmp_name));

    if (tmp_name[tmp_name_len - 1] == '/')
        tmp_name[--tmp_name_len] = '\0';

    if (tmp_name_len + 7 >= sizeof(tmp_name))
        return -1;

    strcpy(tmp_name + tmp_name_len, "/XXXXXX");
    fd = mkstemp(tmp_name);
    fchmod(fd, 0);

    if (fd == -1)
        return -1;

    if (unlink(tmp_name)) {
        close(fd);
        return -1;
    }

    return fd;
}

int uw_jitmem_alloc(void** rw_addr, void** exec_addr, size_t size)
{
    int fd;

    fd = create_tempfile();
    if (fd == -1)
        return -1;

    if (ftruncate(fd, size)) {
        close(fd);
        return -1;
    }

    *rw_addr = (struct chunk_header *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (*rw_addr == MAP_FAILED) {
        close(fd);
        return -1;
    }

    *exec_addr = mmap(NULL, size, PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);

    if (*exec_addr == MAP_FAILED) {
        munmap((void *)*rw_addr, size);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

void uw_jitmem_free(void* rw_addr, void* exec_addr, size_t size)
{
    munmap(rw_addr, size);
    munmap(exec_addr, size);
}
