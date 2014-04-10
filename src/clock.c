//  Time related functions

#define _POSIX_C_SOURCE 199309L

#include <assert.h>
#include <time.h>
#include <stdio.h>

#include "clock.h"

//  Return current time in ms

uint64_t
clock_now ()
{
    struct timespec tv;
    const int rc = clock_gettime (CLOCK_MONOTONIC, &tv);
    assert (rc == 0);
    return (uint64_t) tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
}
