#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "utils/compat.h"
#include "utils/datetime.h"

static uint64_t start_time = 0;
static bool supports_color = false;

static const char *log_level_lut[] = {
    [FH_LOG_DEBUG] = "debug",
    [FH_LOG_INFO] = "info",
    [FH_LOG_WARN] = "warn",
    [FH_LOG_ERR] = "error",
};

static const char *log_level_color_lut[] = {
    [FH_LOG_DEBUG] = "2",
    [FH_LOG_INFO] = "0",
    [FH_LOG_WARN] = "1;33",
    [FH_LOG_ERR] = "1;31",
};

void
fh_log_init (void)
{
    start_time = time_now_ns ();
    fh_log_refresh ();
}

void
fh_log_refresh (void)
{
    supports_color = isatty (STDOUT_FILENO) && isatty (STDERR_FILENO);
}

void
fh_log_set_color_support (bool value)
{
    supports_color = value;
}

void
fh_log_set_start_time (uint64_t value)
{
    start_time = value;
}

void ATTRIBUTE_FORMAT_PRINTF (2, 3)
    fh_log (enum fh_log_level level, const char *format, ...)
{
    va_list args;
    va_start (args, format);
    const uint64_t now = time_now_ns ();
    const double diff = (now - start_time) / 1000000000.0;
    const char *log_level_str = log_level_lut[level];
    FILE *stream = level >= FH_LOG_WARN ? stderr : stdout;
    char format_processed[256 + strlen (format)];

    if (likely (supports_color))
        snprintf (format_processed, sizeof format_processed,
                  "\033[%sm[%16.7f]\033[0m \033[%sm%5s:\033[0m %s\n",
                  level == FH_LOG_WARN  ? "33"
                  : level >= FH_LOG_ERR ? "31"
                                        : "32",
                  diff, log_level_color_lut[level], log_level_str, format);
    else
        snprintf (format_processed, sizeof format_processed,
                  "[%16.7f] %5s: %s\n", diff, log_level_str, format);

    vfprintf (stream, format_processed, args);
    va_end (args);
}
