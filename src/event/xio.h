#ifndef FH_XIO_H
#define FH_XIO_H

#include <sys/types.h>

#ifdef HAVE_LIBURING
    #include <liburing.h>
#endif /* HAVE_LIBURING */

#include "config.h"
#include "utils/utils.h"

struct fh_xio;
struct fh_xio_result
{
    uint32_t flags;
    int32_t res;
    void *buf;
    void *udata;
};

static inline signed int
fh_xio_result_status (const struct fh_xio_result *result)
{
    return result->res;
}

static inline void *
fh_xio_result_get_buf (const struct fh_xio_result *result)
{
    return result->buf;
}

static inline void *
fh_xio_result_get_udata (const struct fh_xio_result *result)
{
    return result->udata;
}

struct fh_xio *fh_xio_create (void);
void fh_xio_free (struct fh_xio *xio);
int fh_xio_request_read (struct fh_xio *xio, void *udata, fd_t fd, void *buf,
                         size_t size, off_t offset);
bool fh_xio_submit_requests (struct fh_xio *xio);
ssize_t fh_xio_wait (struct fh_xio *xio, struct fh_xio_result *results,
                     size_t max_results, uint64_t timeout_ms);
void fh_xio_result_free (struct fh_xio *xio,
                         const struct fh_xio_result *result);
void fh_xio_results_free (struct fh_xio *xio, struct fh_xio_result *results,
                          size_t count);

#endif /* FH_XIO_H */
