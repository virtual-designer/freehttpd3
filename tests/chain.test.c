#include "mm/pool.h"
#include <stdint.h>
#undef NDEBUG

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/http1x.h"
#include "mm/chain.h"

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

	assert (fh_http1x_chain_memchr (&end_cur, &start_cur, ' ', 16));

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

	assert (fh_http1x_chain_memchr (&end_cur, &start_cur, ' ', 16));

	assert (end_cur.chain == chain->next);
	assert (end_cur.off == 1);

	free_chain (chain);
}

int
main (void)
{
	void (**fns) (void) = (void (*[]) (void)) {
		&test_single_buffer,
		&test_split_buffers,
		NULL,
	};

	while (*fns)
	{
		(*fns) ();
		fns++;
	}

	return 0;
}
