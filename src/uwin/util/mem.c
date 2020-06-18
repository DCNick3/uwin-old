
#include "uwin/util/mem.h"
#include "uwin/util/log.h"

#include <stdlib.h>

void* uw_malloc(size_t sz)
{
    void* r = malloc(sz);
    if (r == NULL) {
        uw_log("out of memory while trying to allocate 0x%lx bytes\n", (unsigned long)sz);
        abort();
    }
    return r;
}

void* uw_malloc0(size_t sz)
{
    void* r = calloc(1, sz);
    if (r == NULL) {
        uw_log("out of memory while trying to allocate 0x%lx zero bytes\n", (unsigned long)sz);
        abort();
    }
    return r;
}
 
