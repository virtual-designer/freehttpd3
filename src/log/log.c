#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#include "log.h"
#include "utils/datetime.h"
#include "utils/compat.h"

static uint64_t start_time = 0;
static bool supports_color = false;

static void __attribute__ ((constructor))
fh_log_init (void)
{
    start_time = time_now_ms ();
    supports_color = isatty (STDOUT_FILENO) && isatty (STDERR_FILENO);
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
    
    if (likely (supports_color))
        fprintf (stream, "\033[%sm[%12.7lf]\033[0m ", level >= LOG_WARN ? "31" : "32", diff_s);
    else
        fprintf (stream, "[%12.7lf] ", diff_s);
    
    vfprintf (stream, format, args);
    va_end (args);
}
