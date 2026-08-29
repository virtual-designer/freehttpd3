#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define HT_IMPLEMENTATION
#include "int_htable.h"

/* SplitMix64 finalizer */
static inline uint64_t
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

#include "htable.c"
