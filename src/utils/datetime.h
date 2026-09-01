#ifndef FHTTPD_DATETIME_H
#define FHTTPD_DATETIME_H

#include <stdint.h>
#include <time.h>

uint64_t time_now_ms (void);
uint64_t time_now_us (void);
uint64_t time_now_ns (void);
long double time_now_seconds (void);

#endif /* FHTTPD_DATETIME_H */
