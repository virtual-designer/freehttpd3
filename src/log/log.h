#ifndef FH_LOG_H
#define FH_LOG_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "utils/compat.h"

enum fh_log_level
{
    FH_LOG_DEBUG,
    FH_LOG_INFO,
    FH_LOG_WARN,
    FH_LOG_ERR,
};

void ATTRIBUTE_FORMAT_PRINTF (2, 3)
    fh_log (enum fh_log_level level, const char *format, ...);
void fh_log_init (void);
void fh_log_refresh (void);
void fh_log_set_color_support (bool value);
void fh_log_set_start_time (uint64_t value);

#ifdef FH_LOG_MODULE_NAME
#    define FH_LOG_PREFIX FH_LOG_MODULE_NAME ": "
#else /* not FH_LOG_MODULE_NAME */
#    define FH_LOG_PREFIX
#endif /* FH_LOG_MODULE_NAME */

#ifdef NDEBUG
#    define fh_pr_debug(...)
#else /* not NDEBUG */
#    define fh_pr_debug(...) fh_log (FH_LOG_DEBUG, FH_LOG_PREFIX __VA_ARGS__)
#endif /* NDEBUG */

#define fh_pr_info(...) fh_log (FH_LOG_INFO, FH_LOG_PREFIX __VA_ARGS__)
#define fh_pr_warn(...) fh_log (FH_LOG_WARN, FH_LOG_PREFIX __VA_ARGS__)
#define fh_pr_err(...) fh_log (FH_LOG_ERR, FH_LOG_PREFIX __VA_ARGS__)

#endif /* FH_LOG_H */
