
#ifndef UW_LOG_UTIL_H
#define UW_LOG_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define uw_log(...) fprintf(stderr, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
 
