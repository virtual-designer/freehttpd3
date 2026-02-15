#include <stdlib.h>
#include <string.h>

#include "pool.h"

const size_t POOL_LARGE_THRESHOLD = 4096;
const size_t POOL_REALLOC_SIZE = 4096;

struct fh_pool_chunk
{
	void *start;
	struct fh_pool_chunk *prev;
	size_t capacity, offset;
};

struct fh_pool
{
	struct fh_pool_chunk *current;
};

static struct fh_pool_chunk *
fh_pool_new_chunk (size_t capacity)
{
	struct fh_pool_chunk *chunk = malloc (sizeof (*chunk) + capacity);

	if (!chunk)
		return NULL;

	chunk->start = (void *) (chunk + 1);
	chunk->offset = 0;
	chunk->prev = NULL;
	chunk->capacity = capacity;

	return chunk;
}

struct fh_pool *
fh_pool_create (size_t initial_capacity)
{
	struct fh_pool *pool = malloc (sizeof (*pool));

   	if (!pool)
   		return NULL;

   	pool->current = initial_capacity ? fh_pool_new_chunk (initial_capacity) : NULL;

	if (initial_capacity && !pool->current)
   	{
   		free (pool);
   		return NULL;
   	}

	return pool;
}

void
fh_pool_free (struct fh_pool *pool)
{
	struct fh_pool_chunk *chunk = pool->current;

	while (chunk)
	{
		struct fh_pool_chunk *prev = chunk->prev;
		free (chunk);
		chunk = prev;
	}

	free (pool);
}

void *
fh_pool_alloc (struct fh_pool *pool, size_t size)
{
	if (size >= POOL_LARGE_THRESHOLD)
	{
		/* malloc */
		return NULL;
	}

	struct fh_pool_chunk *chunk = pool->current;

	if (!chunk || chunk->offset + size >= chunk->capacity)
	{
		struct fh_pool_chunk *new_chunk = fh_pool_new_chunk (POOL_REALLOC_SIZE);

		if (!new_chunk)
			return NULL;

		new_chunk->prev = pool->current;
		chunk = pool->current = new_chunk;
	}

	void *ptr = (void *) ((char *) chunk->start + chunk->offset);
	chunk->offset += size;
	return ptr;
}

void *
fh_pool_zalloc (struct fh_pool *pool, size_t size)
{
	void *ptr = fh_pool_alloc (pool, size);

	if (!ptr)
		return NULL;

	memset (ptr, 0, size);
	return ptr;
}
