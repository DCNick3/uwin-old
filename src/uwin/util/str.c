
#include "uwin/util/str.h"
#include "uwin/util/mem.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

char* uw_ascii_strdown(const char* s, ptrdiff_t ssize) {
    char* r = uw_strndup(s, ssize);
    int len = strlen(r);
    for (int i = 0; i < len; i++) {
        r[i] = tolower(r[i]);
    }
    return r;
}
int uw_ascii_strcasecmp(const char* s1, const char* s2) {
    int result;
    
    if (s1 == s2)
        return 0;
    while ((result = tolower(*s1) - tolower(*s2++)) == 0)
        if (*s1++ == '\0')
            break;
    return result;
}

char* uw_strconcat(const char* str1, ...) {
    va_list args;
    va_list tmpa;

    va_start(args, str1);
    va_copy(tmpa, args);
    
    size_t size = strlen(str1);
    while (1) {
        const char* a = va_arg(tmpa, const char*);
        if (a == NULL)
            break;
        size += strlen(a);
    }
    
    va_end(tmpa);
    
    char* r = uw_malloc(size + 1);
    char* p = r;
    
    size_t l = strlen(str1);
    memcpy(p, str1, l);
    p += l;
    
    while (1) {
        const char* a = va_arg(args, const char*);
        if (a == NULL)
            break;
        l = strlen(a);
        memcpy(p, a, l);
        p += l;
    }
    *p = 0;

    va_end(args);
    
    return r;
}

char* uw_strdup_printf(const char *format, ...) {
    va_list args;
    va_list tmpa;

    va_start(args, format);
    va_copy(tmpa, args);
    
    int size = vsnprintf(NULL, 0, format, tmpa);
      
    va_end(tmpa);
    
    if (size < 0) abort();
    
    char* r = uw_malloc(size + 1);
    vsprintf(r, format, args);
    
    return r;
}

char* uw_strdup(const char* str) {
    return uw_strndup(str, strlen(str));
}

char* uw_strndup(const char* str, ptrdiff_t size) {
    if (size == -1)
        return uw_strdup(str);
    char* r = uw_malloc(size + 1);
    memcpy(r, str, size);
    r[size] = 0;
    return r;
}

char* uw_path_get_dirname(const char* file_name) {
    int l = strlen(file_name);
    for (int i = l - 1; i >= 0; i--) {
        if (file_name[i] == '/')
            return uw_strndup(file_name, i);
    }
    return uw_strdup(file_name);
}

char* uw_path_get_basename(const char* file_name) {
    int l = strlen(file_name);
    for (int i = l - 1; i >= 0; i--) {
        if (file_name[i] == '/')
            return uw_strdup(file_name + i + 1);
    }
    return uw_strdup(file_name);
}
 
