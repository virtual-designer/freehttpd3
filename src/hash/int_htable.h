#ifndef FHTTPD_INT_HTABLE_H
#define FHTTPD_INT_HTABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define HT_PREFIX int_
#define HT_KEY_TYPE uint64_t
#define HT_KEY_HASH_CB int_htable_key_hash
#define HT_KEY_EQUAL_CB(key1, key2) ((key1) == (key2))
#define HT_KEY_DUP_CB(key) (key)
#define HT_KEY_FREE_CB(key)
#define HT_KEY_DUP_CHECK(key)

#ifdef HT_IMPLEMENTATION
#    define HT_IMPLEMENTATION_ALREADY_DEFINED
#else /* not HT_IMPLEMENTATION */
#    define HT_IMPLEMENTATION
#endif /* HT_IMPLEMENTATION */

#include "htable.h"
#include "htable_decl_stub.h"

#define int_htable_foreach(table, it) ht_foreach_template(struct int_ht_entry, table, it)

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
#    undef HT_IMPLEMENTATION_ALREADY_DEFINED
#endif /* HT_IMPLEMENTATION_ALREADY_DEFINED */

#endif /* FHTTPD_INT_HTABLE_H */
