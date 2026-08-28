#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define HT_IMPLEMENTATION
#include "int_htable.h"

static uint64_t
int_htable_key_hash (uint64_t key)
{
    const uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325;
    const uint64_t FNV_PRIME = 0x100000001b3;
    uint64_t hash = FNV_OFFSET_BASIS;

    const uint64_t ull_key = (uint64_t) key;
    const unsigned char *data = (const unsigned char *) &ull_key;

    for (size_t i = 0; i < sizeof (ull_key); i++)
    {
        hash ^= data[i];
        hash *= FNV_PRIME;
    }

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
