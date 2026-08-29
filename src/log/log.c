#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "log.h"
#include "utils/datetime.h"

static uint64_t start_time = 0;

static void __attribute__((constructor))
fh_log_init (void)
{
    start_time = time_now_ms ();
}

void __attribute__((format(printf, 2, 3)))
fh_log (enum fh_log_level level, const char *format, ...)
{
    va_list args;
    va_start (args, format);
    uint64_t time = time_now_ms ();
    uint64_t diff = time - start_time;
    double diff_s = diff / 1000.0;
    FILE *stream = level >= LOG_WARN ? stderr : stdout;
    fprintf (stream, "[%4.7lf] ", diff_s);
    vfprintf (stream, format, args);
    va_end (args);
}