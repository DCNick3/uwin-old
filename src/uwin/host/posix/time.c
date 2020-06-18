/*
 *  time functions
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

#include "uwin/uwin.h"

#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>


uint64_t uw_get_time_us(void) {
    struct timeval tv;
    int r = gettimeofday(&tv, NULL);
    assert(r == 0);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void uw_sleep(uint32_t time_ms) {
    useconds_t t = (useconds_t)time_ms * 1000;
    usleep(t);
}

uint64_t uw_get_monotonic_time(void) {
    struct timespec tp;
    int r = clock_gettime(CLOCK_MONOTONIC, &tp);
    assert(!r);
    
    return (uint64_t)tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
}

uint64_t uw_get_real_time(void) {
    struct timespec tp;
    int r = clock_gettime(CLOCK_REALTIME, &tp);
    assert(!r);

    return (uint64_t)tp.tv_sec * 1000000 + tp.tv_nsec / 1000;
}

uint32_t uw_get_timezone_offset(void) {
    tzset();
    return timezone;
}
