#include <stdlib.h>
#include <string.h>

#include "pool.h"

static const size_t POOL_LARGE_THRESHOLD = 4096;
static const size_t POOL_REALLOC_SIZE = 4096;

struct fh_pool_large_chunk
{
	void *start;
	size_t capacity;
	void (*cleanup_cb)(void *);
	struct fh_pool_large_chunk *prev;
};

struct fh_pool_chunk
{
	void *start;
	struct fh_pool_chunk *prev;
	size_t capacity, offset;
};

struct fh_pool
{
	struct fh_pool_chunk *last;
	struct fh_pool_large_chunk *last_lg;
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

	pool->last_lg = NULL;
   	pool->last = initial_capacity ? fh_pool_new_chunk (initial_capacity) : NULL;

	if (initial_capacity && !pool->last)
   	{
   		free (pool);
   		return NULL;
   	}

	return pool;
}

void
fh_pool_free (struct fh_pool *pool)
{
	struct fh_pool_chunk *chunk = pool->last;

	while (chunk)
	{
		struct fh_pool_chunk *prev = chunk->prev;
		free (chunk);
		chunk = prev;
	}

	struct fh_pool_large_chunk *lg_chunk = pool->last_lg;

	while (lg_chunk)
	{
		struct fh_pool_large_chunk *prev = lg_chunk->prev;

		if (lg_chunk->cleanup_cb)
			lg_chunk->cleanup_cb (lg_chunk->start);
		else
			free (lg_chunk);
		
		lg_chunk = prev;
	}

	free (pool);
}

void *
fh_pool_alloc (struct fh_pool *pool, size_t size)
{
	if (size > POOL_LARGE_THRESHOLD)
	{
		struct fh_pool_large_chunk *lg_chunk = malloc (sizeof (*lg_chunk) + size);

		if (!lg_chunk)
			return NULL;

		lg_chunk->capacity = size;
		lg_chunk->cleanup_cb = NULL;
		lg_chunk->prev = pool->last_lg;
		lg_chunk->start = (void *) (lg_chunk + 1);

		pool->last_lg = lg_chunk;
		return lg_chunk;
	}

	struct fh_pool_chunk *chunk = pool->last;

	if (!chunk || chunk->offset + size >= chunk->capacity)
	{
		struct fh_pool_chunk *new_chunk = fh_pool_new_chunk (POOL_REALLOC_SIZE);

		if (!new_chunk)
			return NULL;

		new_chunk->prev = pool->last;
		chunk = pool->last = new_chunk;
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
