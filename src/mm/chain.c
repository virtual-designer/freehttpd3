#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "chain.h"
#include "pool.h"
#include "compat.h"

struct fh_chain *
fh_chain_new (struct fh_pool *pool, struct fh_buf *buf)
{
	struct fh_chain *chain = fh_pool_alloc (pool, sizeof (*chain));

	if (!chain)
		return NULL;

	chain->buf = buf;
	chain->next = NULL;

	return chain;
}

struct fh_buf *
fh_buf_new_file (struct fh_pool *pool, fd_t fd)
{
	struct fh_buf *buf = fh_pool_alloc (pool, sizeof (*buf));

	if (!buf)
		return NULL;

	buf->file_fd = fd;
	buf->flags = 0;
	buf->type = FH_BUF_FILE;

	return buf;
}

struct fh_buf *
fh_buf_new_mem (struct fh_pool *pool, uint8_t *mem, size_t capacity,
				size_t size)
{
	struct fh_buf *buf = fh_pool_alloc (pool, sizeof (*buf));

	if (!buf)
		return NULL;

	buf->mem_cap = capacity;
	buf->mem_size = size;
	buf->mem_ptr = mem;
	buf->flags = 0;
	buf->type = FH_BUF_MEM;

	return buf;
}

bool
fh_chain_read (struct fh_pool *pool, fd_t fd, struct fh_chain **head,
			   struct fh_chain **tail)
{
	struct fh_chain *current = *tail;
	struct fh_buf *buf = current ? current->buf : NULL;
	const size_t MEM_REALLOC_SIZE = 4096;

	for (;;)
	{
		if (!current || buf->mem_size >= buf->mem_cap)
		{
			uint8_t *mem = fh_pool_alloc (pool, MEM_REALLOC_SIZE);

			if (!mem)
				return false;

			buf = fh_buf_new_mem (pool, mem, MEM_REALLOC_SIZE, 0);

			if (!buf)
				return false;

			current = fh_chain_new (pool, buf);

			if (!current)
				return false;

			if (!*tail)
			{
				*head = *tail = current;
			}
			else
			{
				(*tail)->next = current;
				(*tail) = current;
			}
		}

		ssize_t read_bytes = recv (fd, buf->mem_ptr + buf->mem_size,
								   buf->mem_cap - buf->mem_size, 0);

        if (read_bytes == 0)
            return true;
        
		if (read_bytes < 0)
        {
            if (errno == EINTR)
                continue;
            
			return ERR_WOULD_BLOCK;
        }

		buf->mem_size += (size_t) read_bytes;
	}
}
