#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define HT_IMPLEMENTATION
#include "int_htable.h"

/* FNV-1a */
static uint64_t
int_htable_key_hash (uint64_t key)
{
    uint64_t hash = key;

    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9ULL;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebULL;
    hash ^= hash >> 31;

    return hash;
}

static bool
int_htable_key_equal (uint64_t key1, uint64_t key2)
{
    return key1 == key2;
}

static uint64_t
int_htable_key_dup (uint64_t key)
{
    return key;
}

#include "htable.c"
