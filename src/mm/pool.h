#ifndef FHTTPD_POOL_H
#define FHTTPD_POOL_H

struct fh_pool;
typedef struct fh_pool pool_t;

#define fh_pool_calloc(pool, n, size) fh_pool_zalloc(pool, n * size)

struct fh_pool *fh_pool_create (size_t initial_capacity);
void fh_pool_free (struct fh_pool *pool);
void *fh_pool_alloc (struct fh_pool *pool, size_t size);
void *fh_pool_zalloc (struct fh_pool *pool, size_t size);

#endif /* FHTTPD_POOL_H */
