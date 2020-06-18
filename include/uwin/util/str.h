
#ifndef UWIN_STR_H
#define UWIN_STR_H

#include <stddef.h>

char* uw_ascii_strdown(const char* s, ptrdiff_t ssize);
int   uw_ascii_strcasecmp(const char* s1, const char* s2);

char* uw_strconcat(const char* str1, ...);
char* uw_strdup_printf(const char *format, ...);
char* uw_strdup(const char* str);
char* uw_strndup(const char* str, ptrdiff_t size);

char* uw_path_get_dirname(const char* file_name);
char* uw_path_get_basename(const char* file_name);

#endif
 
