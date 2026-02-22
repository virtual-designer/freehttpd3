#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "chain.h"
#include "compat.h"
#include "pool.h"
#include "types.h"

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
fh_chain_find_char (struct fh_chain_cur *dest_cur,
					const struct fh_chain_cur *begin, int delim, size_t max_len)
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

bool
fh_chain_copy_range (struct fh_pool *pool, struct fh_chain_cur *start_cur,
					 struct fh_chain_cur *end_cur, size_t max_probable_len,
					 struct fh_chain_cpbuf *cpbuf)
{
	const bool is_same_chain = start_cur->chain == end_cur->chain;

	if (is_same_chain
		|| (start_cur->chain->next && start_cur->chain->next == end_cur->chain
			&& end_cur->off == 0))
	{
		cpbuf->raw_buf
			= (uint8_t *) (start_cur->chain->buf->mem_ptr + start_cur->off);
		cpbuf->len = is_same_chain && end_cur->off > start_cur->off
						 ? end_cur->off - start_cur->off
						 : start_cur->chain->buf->mem_size - start_cur->off;
		cpbuf->incomplete = cpbuf->len < max_probable_len;

		if (!cpbuf->incomplete)
			cpbuf->len = max_probable_len;

		return true;
	}

	uint8_t *buf = cpbuf->raw_buf;

	if (!buf)
	{
		buf = fh_pool_alloc (pool, max_probable_len);

		if (!buf)
			return false;

		cpbuf->raw_buf = buf;
	}

	size_t total_len = 0;
	struct fh_chain *current = start_cur->chain;

	while (current)
	{
		const bool is_begin = current == start_cur->chain;
		const bool is_end = current == end_cur->chain;
		const size_t begin_off = is_begin ? start_cur->off : 0;
		const size_t end_off = is_end ? end_cur->off : 0;
		const size_t eff_size = (end_off ? end_off <= current->buf->mem_size
											   ? end_off
											   : current->buf->mem_size
										 : current->buf->mem_size)
								- begin_off;
		const size_t bytes_to_copy = eff_size + total_len > max_probable_len
										 ? max_probable_len - total_len
										 : eff_size;

		memcpy (buf + total_len, current->buf->mem_ptr + begin_off,
				bytes_to_copy);
		total_len += bytes_to_copy;

		if (total_len > max_probable_len || is_end)
			break;

		current = current->next;
	}

	cpbuf->len = total_len;
	cpbuf->incomplete = total_len < max_probable_len;

	return true;
}
