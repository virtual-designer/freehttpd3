/*
 * This file is part of OSN freehttpd.
 *
 * Copyright (C) 2025-2026  OSN Developers.
 *
 * OSN freehttpd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OSN freehttpd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with OSN freehttpd.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FHTTPD_MM_CHAIN_H
#define FHTTPD_MM_CHAIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "pool.h"
#include "types.h"

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

struct fh_chain_list
{
	struct fh_chain *head;
	struct fh_chain *tail;
};

struct fh_chain_cur
{
	struct fh_chain *chain;
	size_t off;
};

struct fh_chain_cpbuf
{
	uint8_t *raw_buf;
	size_t len;
	bool incomplete;
};

struct fh_chain *fh_chain_new (struct fh_pool *pool, struct fh_buf *buf);
struct fh_buf *fh_buf_new_file (struct fh_pool *pool, fd_t fd);
struct fh_buf *fh_buf_new_mem (struct fh_pool *pool, uint8_t *mem,
							   size_t capacity, size_t size);
bool fh_chain_read (struct fh_pool *pool, fd_t fd, struct fh_chain **head,
					struct fh_chain **tail);
struct fh_chain *fh_chain_new_with_buf_mem (struct fh_pool *pool,
											size_t capacity);
bool fh_chain_find_char (struct fh_chain_cur *dest_cur,
						 const struct fh_chain_cur *begin, int delim,
						 size_t max_len);
bool fh_chain_copy_range (struct fh_pool *pool, struct fh_chain_cur *start_cur,
						  struct fh_chain_cur *end_cur, size_t max_probable_len,
						  struct fh_chain_cpbuf *cpbuf);

#endif /* FHTTPD_MM_CHAIN_H */
