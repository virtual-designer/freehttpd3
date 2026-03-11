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

#undef NDEBUG

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mm/chain.h"
#include "mm/pool.h"

static struct fh_chain *
create_chain (char *raw_buf[static 1], size_t buf_count)
{
	struct fh_chain *head = NULL;
	struct fh_chain *tail = NULL;

	for (size_t i = 0; i < buf_count; i++)
	{
		size_t buf_len = strlen (raw_buf[i]);
		struct fh_chain *chain
			= calloc (1, sizeof (*chain) + sizeof (*chain->buf));
		assert (chain != NULL);

		if (!head)
		{
			head = tail = chain;
		}
		else
		{
			tail->next = chain;
			tail = chain;
		}

		chain->buf = (struct fh_buf *) (chain + 1);
		chain->buf->type = FH_BUF_MEM;
		chain->buf->mem_ptr = (uint8_t *) (raw_buf[i]);
		chain->buf->mem_size = buf_len;
		chain->buf->mem_cap = buf_len;
	}

	return head;
}

static void
free_chain (struct fh_chain *chain)
{
	while (chain)
	{
		struct fh_chain *next = chain->next;
		free (chain);
		chain = next;
	}
}

static void
print_chain (struct fh_chain *chain)
{
	size_t id = 0;

	while (chain)
	{
		printf ("[***] Chain #%zu:\n", id);
		printf ("      Length: %zu\n", chain->buf->mem_size);
		printf ("      Buffer: |%s|\n", (char *) chain->buf->mem_ptr);
		printf ("\n");
		chain = chain->next;
		id++;
	}
}

static void
test_single_buffer (void)
{
	struct fh_chain *chain
		= create_chain ((char *[]) { "GET / HTTP/1.1\r\n" }, 1);

	struct fh_chain_cur end_cur = { 0 };
	struct fh_chain_cur start_cur = { .chain = chain, .off = 0 };

	print_chain (chain);

	assert (fh_chain_find_char (&end_cur, &start_cur, ' ', 16) == FIND_CHAR_OK);

	assert (end_cur.chain == chain);
	assert (end_cur.off == 3);

	free_chain (chain);
}

static void
test_split_buffers (void)
{
	struct fh_chain *chain
		= create_chain ((char *[]) { "GE", "T /", " HTTP/1.1\r\n" }, 3);

	struct fh_chain_cur end_cur = { 0 };
	struct fh_chain_cur start_cur = { .chain = chain, .off = 0 };

	print_chain (chain);

	assert (fh_chain_find_char (&end_cur, &start_cur, ' ', 16) == FIND_CHAR_OK);

	assert (end_cur.chain == chain->next);
	assert (end_cur.off == 1);

	free_chain (chain);
}

static void
test_chain_zero_copy (void)
{
	struct fh_chain *chain
		= create_chain ((char *[]) { "GET /", " HTTP/1.1\r\n" }, 2);
	struct fh_pool *pool = fh_pool_create (4096);

	struct fh_chain_cur end_cur = { 0 };
	struct fh_chain_cur start_cur = { .chain = chain, .off = 0 };

	print_chain (chain);

	assert (fh_chain_find_char (&end_cur, &start_cur, ' ', 16) == FIND_CHAR_OK);

	assert (end_cur.chain == chain);
	assert (end_cur.off == 3);

	struct fh_chain_cpbuf cpbuf = { 0 };

	assert (fh_chain_copy_range (pool, &start_cur, &end_cur, 8, &cpbuf));

	printf ("BUFFER: (%zu) |%.*s|\n", cpbuf.len, (int) cpbuf.len,
			(char *) cpbuf.raw_buf);

	assert (cpbuf.len == 3);
	assert (memcmp (cpbuf.raw_buf, "GET", 3) == 0);
	assert (cpbuf.raw_buf == chain->buf->mem_ptr);

	fh_pool_free (pool);
	free_chain (chain);
}

static void
test_chain_copy (void)
{
	struct fh_chain *chain
		= create_chain ((char *[]) { "PA", "T", "CH /", " HTTP/1.1\r\n" }, 4);
	struct fh_pool *pool = fh_pool_create (4096);

	struct fh_chain_cur end_cur = { 0 };
	struct fh_chain_cur start_cur = { .chain = chain, .off = 0 };

	print_chain (chain);

	assert (fh_chain_find_char (&end_cur, &start_cur, ' ', 16) == FIND_CHAR_OK);

	assert (end_cur.chain == chain->next->next);
	assert (end_cur.off == 2);

	struct fh_chain_cpbuf cpbuf = { 0 };

	assert (fh_chain_copy_range (pool, &start_cur, &end_cur, 16, &cpbuf));

	printf ("BUFFER: (%zu) |%.*s|\n", cpbuf.len, (int) cpbuf.len,
			(char *) cpbuf.raw_buf);

	assert (cpbuf.len == 5);
	assert (memcmp (cpbuf.raw_buf, "PATCH", 5) == 0);

	fh_pool_free (pool);
	free_chain (chain);
}

int
main (void)
{
	void (**fns) (void) = (void (*[]) (void)) {
		&test_single_buffer,
		&test_split_buffers,
		&test_chain_copy,
		&test_chain_zero_copy,
		NULL,
	};

	while (*fns)
	{
		(*fns) ();
		fns++;
	}

	return 0;
}
