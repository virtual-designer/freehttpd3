#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "chain.h"
#include "compat.h"
#include "pool.h"

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

struct fh_chain *
fh_chain_new_with_buf_mem (struct fh_pool *pool, size_t capacity)
{
	struct fh_chain *chain = fh_pool_alloc (
		pool, sizeof (*chain) + sizeof (*chain->buf) + capacity);

	if (!chain)
		return NULL;

	chain->buf = (struct fh_buf *) (chain + 1);
	chain->next = NULL;
	chain->buf->mem_ptr = (uint8_t *) (chain->buf + 1);
	chain->buf->mem_cap = capacity;
	chain->buf->mem_size = 0;
	chain->buf->type = FH_BUF_MEM;
	chain->buf->flags = 0;

	return chain;
}

bool
fh_chain_read (struct fh_pool *pool, fd_t fd, struct fh_chain **head,
			   struct fh_chain **tail)
{
	struct fh_chain *current = *tail;
	struct fh_buf *buf = current ? current->buf : NULL;
	const size_t READ_BUF_SIZE = 4096;

	for (;;)
	{
		if (!current || buf->mem_size >= buf->mem_cap)
		{
		    current = fh_chain_new_with_buf_mem (pool, READ_BUF_SIZE);

			if (!current)
				return false;

			buf = current->buf;

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

bool
fh_http1x_chain_memchr (struct fh_chain_cur *dest_cur,
						const struct fh_chain_cur *begin, int delim,
						size_t max_len)
{
	struct fh_chain *current = begin->chain;
	size_t total_len = 0;

	while (current)
	{
		const bool is_begin = current == begin->chain;
		const size_t begin_off = is_begin ? begin->off : 0;
		uint8_t *ptr = current->buf->mem_ptr + begin_off;
		size_t size = current->buf->mem_size - begin_off;
		size = size + total_len >= max_len ? max_len - total_len : size;

		uint8_t *chr = memchr (ptr, delim, size);

		if (chr)
		{
			size_t rel_off = (size_t) (chr - ptr);
			size_t off = rel_off + begin_off;

			dest_cur->chain = current;
			dest_cur->off = off;

			return true;
		}

		total_len += size;

		if (total_len >= max_len || !current->next)
		{
			dest_cur->chain = current;
			dest_cur->off = current->buf->mem_size;
			return false;
		}

		current = current->next;
	}

	return false;
}
