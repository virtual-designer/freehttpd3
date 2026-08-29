#ifndef FHTTPD_HTABLE_H
#define FHTTPD_HTABLE_H

typedef void (*ht_data_free_cb_t)(void *);

#ifndef HT_PREFIX
#error "Please define HT_* macros"
#endif

#ifndef HT_KEY_TYPE
#error "Please define HT_* macros"
#endif

#ifndef HT_KEY_HASH_CB
#error "Please define HT_* macros"
#endif

#ifndef HT_KEY_EQUAL_CB
#error "Please define HT_* macros"
#endif

#ifndef HT_KEY_DUP_CB
#error "Please define HT_* macros"
#endif

#ifndef HT_KEY_DUP_CHECK
#error "Please define HT_* macros"
#endif

#ifndef HT_KEY_FREE_CB
#error "Please define HT_* macros"
#endif

/* Temporary macros */

#define HT_NAME_0(prefix, name) prefix##name
#define HT_NAME_1(prefix, name) HT_NAME_0 (prefix, name)
#define HT_NAME(name) HT_NAME_1 (HT_PREFIX, name)

#endif /* FHTTPD_HTABLE_H */

#ifndef HT_IMPLEMENTATION
#undef HT_NAME_0
#undef HT_NAME_1
#undef HT_NAME
#undef htable_t
#undef ht_entry_t
#endif /* HT_IMPLEMENTATION */
