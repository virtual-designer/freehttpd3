#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "rapidhash.h"

#define HT_IMPLEMENTATION
#include "str_htable.h"

static inline uint64_t
str_htable_key_hash (const char *key)
{
    return rapidhashMicro (key, strlen(key));
}

#include "htable.c"
