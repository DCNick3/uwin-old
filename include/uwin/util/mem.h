 
#ifndef MEM_UTIL_H
#define MEM_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

void* uw_malloc(size_t sz);
void* uw_malloc0(size_t sz);

#define uw_new(type, cnt) ((type*)uw_malloc(sizeof(type) * cnt))
#define uw_new0(type, cnt) ((type*)uw_malloc0(sizeof(type) * cnt))

#define uw_free free


#ifdef __cplusplus
}
#endif

#endif
 
