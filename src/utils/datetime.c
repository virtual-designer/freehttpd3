#include "datetime.h"

uint64_t
time_now_ms (void)
{
    struct timespec ts = {0};
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000ULL + (ts.tv_nsec / 1000000ULL);
}
