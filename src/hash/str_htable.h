#ifndef FHTTPD_STR_HTABLE_H
#define FHTTPD_STR_HTABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define HT_PREFIX str_
#define HT_KEY_TYPE char *
#define HT_KEY_HASH_CB str_htable_key_hash
#define HT_KEY_EQUAL_CB(key1, key2) (!strcmp(key1, key2))
#define HT_KEY_DUP_CB strdup
#define HT_KEY_FREE_CB free
#define HT_KEY_DUP_CHECK(key)                                                  \
    do                                                                         \
    {                                                                          \
        if (!(key))                                                              \
            return false;                                                       \
    } while (false);

#ifdef HT_IMPLEMENTATION
#    define HT_IMPLEMENTATION_ALREADY_DEFINED
#else /* not HT_IMPLEMENTATION */
#    define HT_IMPLEMENTATION
#endif /* HT_IMPLEMENTATION */

#include "htable.h"
#include "htable_decl_stub.h"

#ifndef HT_IMPLEMENTATION_ALREADY_DEFINED
#    undef HT_IMPLEMENTATION
#    include "htable.h"

#    undef HT_PREFIX
#    undef HT_KEY_TYPE
#    undef HT_KEY_HASH_CB
#    undef HT_KEY_EQUAL_CB
#    undef HT_KEY_DUP_CB
#    undef HT_KEY_FREE_CB
#    undef HT_KEY_DUP_CHECK
#endif /* HT_IMPLEMENTATION_ALREADY_DEFINED */

#endif /* FHTTPD_STR_HTABLE_H */
