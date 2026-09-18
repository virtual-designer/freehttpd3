#ifndef FH_XIO_H
#define FH_XIO_H

#include <sys/socket.h>
#include <sys/types.h>

#ifdef FH_PLATFORM_LINUX
    #include <sys/epoll.h>
#endif /* HAVE_LIBURING */

#ifdef HAVE_LIBURING
    #include <liburing.h>
#endif /* HAVE_LIBURING */

#include "config.h"
#include "utils/utils.h"
#include "xpoll.h"

struct fh_xio;

enum fh_xio_op
{
    XIO_OP_READ = 1,
    XIO_OP_WRITE,
    XIO_OP_RECV,
    XIO_OP_SEND,
    XIO_OP_ACCEPT,
};

enum xio_task_flag
{
    XIO_TASK_F_BUFFER = 0x1,
    XIO_TASK_F_MORE = 0x2,
    XIO_TASK_F_INTERNAL_ERROR = 0x4
};

struct fh_xio_result
{
    enum fh_xio_op op;
    uint32_t flags;
    ssize_t res;
    void *buf;
    void *udata;
};

/* These functions set errno if an error has occurred and it can be
   handled. */
struct fh_xio *fh_xio_create (void);
void fh_xio_free (struct fh_xio *xio);
bool fh_xio_submit_requests (struct fh_xio *xio);
ssize_t fh_xio_wait (struct fh_xio *xio, struct fh_xio_result *results,
                     size_t max_results, uint64_t timeout_ms);
void fh_xio_result_free (struct fh_xio *xio,
                         const struct fh_xio_result *result);
void fh_xio_results_free (struct fh_xio *xio, struct fh_xio_result *results,
                          size_t count);

/* These functions return -errno on failure. */
int fh_xio_request_read (struct fh_xio *xio, void *udata, fd_t fd, void *buf,
                         size_t size, off_t offset);
int fh_xio_request_write (struct fh_xio *xio, void *udata, fd_t fd, void *buf,
                          size_t size, off_t offset);
int fh_xio_request_recv (struct fh_xio *xio, void *udata, fd_t fd, void *buf,
                         size_t size, int flags);
int fh_xio_request_send (struct fh_xio *xio, void *udata, fd_t fd, void *buf,
                         size_t size, int flags);
int fh_xio_request_accept (struct fh_xio *xio, void *udata, fd_t fd,
                           struct sockaddr *addr, socklen_t *addr_len,
                           int flags);

#endif /* FH_XIO_H */
