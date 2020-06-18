/*
 *  implementation of small subset of standard C library
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

#include <stddef.h>
#include <limits.h>
#include <rpmalloc.h>
#include <ksvc.h>

void KSVC_DLL *memmove(void *dest, const void *src, size_t len)
{
  char *d = dest;
  const char *s = src;
  if (d < s)
    while (len--)
      *d++ = *s++;
  else
    {
      const char *lasts = s + (len-1);
      char *lastd = d + (len-1);
      while (len--)
        *lastd-- = *lasts--;
    }
  return dest;
} 
void KSVC_DLL *memcpy(void* dest, const void* src, size_t cnt)
{
    char *d = dest;
    const char *s =(const char*)src;

    if((d != NULL) && (s != NULL))
    {
        while(cnt)
        {
            *(d++)= *(s++);
            --cnt;
        }
    }

    return dest;
}
void KSVC_DLL *memset(void *b, int c, size_t len)
{
  unsigned char *p = b;
  while(len > 0)
    {
      *p = c;
      p++;
      len--;
    }
  return b;
}

char* KSVC_DLL strdup(const char *s)
{
    size_t sz = strlen(s) + 1;
    char* buf = rpmalloc(sz);
    if (buf == NULL) return NULL;
    memcpy(buf, s, sz);
    return buf;
}

size_t KSVC_DLL strlen(const char *s)
{
    size_t r = 0;
    while (*s != '\0') s++, r++;
    return r;
}

int KSVC_DLL strcmp(const char *s1, const char *s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// based on https://stackoverflow.com/questions/7457163/what-is-the-implementation-of-strtol
long KSVC_DLL strtol(const char *restrict nptr, char **restrict endptr, int base) {
    const char *p = nptr, *endp;
    _Bool is_neg = 0, overflow = 0;
    /* Need unsigned so (-LONG_MIN) can fit in these: */
    unsigned long n = 0UL, cutoff;
    int cutlim;
    if (base < 0 || base == 1 || base > 36) {
        return 0L;
    }
    endp = nptr;
    while (isspace(*p))
        p++;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        is_neg = 1, p++;
    }
    if (*p == '0') {
        p++;
        /* For strtol(" 0xZ", &endptr, 16), endptr should point to 'x';
         * pointing to ' ' or '0' is non-compliant.
         * (Many implementations do this wrong.) */
        endp = p;
        if (base == 16 && (*p == 'X' || *p == 'x')) {
            p++;
        } else if (base == 0) {
            if (*p == 'X' || *p == 'x') {
                base = 16, p++;
            } else {
                base = 8;
            }
        }
    } else if (base == 0) {
        base = 10;
    }
    cutoff = (is_neg) ? -(LONG_MIN / base) : LONG_MAX / base;
    cutlim = (is_neg) ? -(LONG_MIN % base) : LONG_MAX % base;
    while (1) {
        int c;
        if (*p >= 'A')
            c = ((*p - 'A') & (~('a' ^ 'A'))) + 10;
        else if (*p <= '9')
            c = *p - '0';
        else
            break;
        if (c < 0 || c >= base) break;
        endp = ++p;
        if (overflow) {
            /* endptr should go forward and point to the non-digit character
             * (of the given base); required by ANSI standard. */
            if (endptr) continue;
            break;
        }
        if (n > cutoff || (n == cutoff && c > cutlim)) {
            overflow = 1; continue;
        }
        n = n * base + c;
    }
    if (endptr) *endptr = (char *)endp;
    if (overflow) {
        return ((is_neg) ? LONG_MIN : LONG_MAX);
    }
    return (long)((is_neg) ? -n : n);
}

int KSVC_DLL isspace(int c) { return c == ' ' || c == '\t'; }

char* KSVC_DLL strcat(char* dst, const char* src) {
    char* sdst = dst;
    while (*dst != '\0')
        dst++;
    while (*src != '\0') {
        *dst = *src;
        src++;
        dst++;
    }
    return sdst;
}
char* KSVC_DLL strchr(const char *s, int c) {
    char ch;

    ch = c;
    for (;; ++s) {
        if (*s == ch)
            return (char *)s;
        if (*s == '\0')
            return NULL;
    }
}
char* KSVC_DLL strrchr(const char *s, int c) {
    char *save;
    char ch;

    ch = c;
    for (save = NULL;; ++s) {
        if (*s == ch)
            save = (char *)s;
        if (*s == '\0')
            return save;
    }
}
int KSVC_DLL tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
        return 'a' + c - 'A';
    else
        return c;
}
int KSVC_DLL stricmp(const char *s1, const char *s2) {
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;
    int result;
    if (p1 == p2)
    return 0;
    while ((result = tolower(*p1) - tolower(*p2++)) == 0)
    if (*p1++ == '\0')
        break;
    return result;
}
