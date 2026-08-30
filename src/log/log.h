#ifndef FH_LOG_H
#define FH_LOG_H

#include "config.h"

enum fh_log_level
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERR,
};

void __attribute__ ((format (printf, 2, 3))) fh_log (enum fh_log_level level,
                                                     const char *format, ...);

#ifdef FH_LOG_MODULE_NAME
#    define FH_LOG_PREFIX FH_LOG_MODULE_NAME ": "
#else /* not FH_LOG_MODULE_NAME */
#    define FH_LOG_PREFIX
#endif /* FH_LOG_MODULE_NAME */

#ifdef STDC_HAVE_VA_OPT
#    ifdef NDEBUG
#        define fh_pr_debug(format, ...)
#    else /* not NDEBUG */
#        define fh_pr_debug(format, ...)                                       \
            fh_log (LOG_DEBUG,                                                 \
                    FH_LOG_PREFIX format "\n" __VA_OPT__ (, ) __VA_ARGS__)
#    endif /* NDEBUG */

#    define fh_pr_info(format, ...)                                            \
        fh_log (LOG_INFO, FH_LOG_PREFIX format "\n" __VA_OPT__ (, ) __VA_ARGS__)
#    define fh_pr_warn(format, ...)                                            \
        fh_log (LOG_WARN, FH_LOG_PREFIX format "\n" __VA_OPT__ (, ) __VA_ARGS__)
#    define fh_pr_err(format, ...)                                             \
        fh_log (LOG_ERR, FH_LOG_PREFIX format "\n" __VA_OPT__ (, ) __VA_ARGS__)
#else /* not STDC_HAVE_VA_OPT */
#    ifdef NDEBUG
#        define fh_pr_debug(format, ...)
#    else /* not NDEBUG */
#        define fh_pr_debug(format, ...)                                       \
            fh_log (LOG_DEBUG, FH_LOG_PREFIX format "\n", ##__VA_ARGS__)
#    endif /* NDEBUG */

#    define fh_pr_info(format, ...)                                            \
        fh_log (LOG_INFO, FH_LOG_PREFIX format "\n", ##__VA_ARGS__)
#    define fh_pr_warn(format, ...)                                            \
        fh_log (LOG_WARN, FH_LOG_PREFIX format "\n", ##__VA_ARGS__)
#    define fh_pr_err(format, ...)                                             \
        fh_log (LOG_ERR, FH_LOG_PREFIX format "\n", ##__VA_ARGS__)
#endif /* STDC_HAVE_VA_OPT */

#endif /* FH_LOG_H */
