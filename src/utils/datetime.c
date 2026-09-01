#include "datetime.h"

uint64_t
time_now_ms (void)
{
    struct timespec ts = { 0 };
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000ULL + (ts.tv_nsec / 1000000ULL);
}

uint64_t
time_now_us (void)
{
    struct timespec ts = { 0 };
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000ULL + (ts.tv_nsec / 1000ULL);
}

uint64_t
time_now_ns (void)
{
    struct timespec ts = { 0 };
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

long double
time_now_seconds (void)
{
    struct timespec ts = { 0 };
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ((long double) ts.tv_sec) + (((long double) ts.tv_nsec) / 1000000000.0L);
}
