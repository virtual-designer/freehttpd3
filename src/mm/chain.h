#ifndef FHTTPD_MM_CHAIN_H
#define FHTTPD_MM_CHAIN_H

#include <stdint.h>
#include <stddef.h>
#include "types.h"
#include "pool.h"

enum fh_buf_type
{
    FH_BUF_FILE,
    FH_BUF_MEM,
};

enum fh_buf_flags
{
    FH_BUF_RO,
};

struct fh_buf
{
    enum fh_buf_type type : 2;
    enum fh_buf_flags flags : 4;

    union
    {
        struct
        {
            uint8_t *mem_ptr;
            size_t mem_size, mem_cap;
        };

        struct
        {
            fd_t file_fd;
        };
    };
};

struct fh_chain
{
    struct fh_buf *buf;
    struct fh_chain *next;
};

struct fh_chain *fh_chain_new (struct fh_pool *pool, struct fh_buf *buf);
struct fh_buf *fh_buf_new_file (struct fh_pool *pool, fd_t fd);
struct fh_buf *fh_buf_new_mem (struct fh_pool *pool, uint8_t *mem, size_t capacity, size_t size);

#endif /* FHTTPD_MM_CHAIN_H */
