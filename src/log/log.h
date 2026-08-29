#ifndef FH_LOG_H
#define FH_LOG_H

enum fh_log_level
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERR,
};

void __attribute__((format(printf, 2, 3))) fh_log (enum fh_log_level level, const char *format, ...);

#ifdef FH_LOG_MODULE_NAME
#define FH_LOG_PREFIX FH_LOG_MODULE_NAME ": "
#else /* not FH_LOG_MODULE_NAME */
#define FH_LOG_PREFIX
#endif /* FH_LOG_MODULE_NAME */

#define fh_pr_debug(format, ...) fh_log (LOG_DEBUG, FH_LOG_PREFIX format "\n", ##__VA_ARGS__)
#define fh_pr_info(format, ...) fh_log (LOG_INFO, FH_LOG_PREFIX format "\n", ##__VA_ARGS__)
#define fh_pr_warn(format, ...) fh_log (LOG_WARN, FH_LOG_PREFIX format "\n", ##__VA_ARGS__)
#define fh_pr_err(format, ...) fh_log (LOG_ERR, FH_LOG_PREFIX format "\n", ##__VA_ARGS__)

#endif /* FH_LOG_H */
