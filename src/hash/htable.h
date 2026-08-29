#ifndef FHTTPD_HTABLE_H
#    define FHTTPD_HTABLE_H

typedef void (*ht_data_free_cb_t) (void *);

#endif /* FHTTPD_HTABLE_H */

#define HT_ELEMENT(table, idx) (table)->entries[idx]

#ifndef HT_PREFIX
#    error "Please define HT_* macros"
#endif

#ifndef HT_KEY_TYPE
#    error "Please define HT_* macros"
#endif

#ifndef HT_KEY_HASH_CB
#    error "Please define HT_* macros"
#endif

#ifndef HT_KEY_EQUAL_CB
#    error "Please define HT_* macros"
#endif

#ifndef HT_KEY_DUP_CB
#    error "Please define HT_* macros"
#endif

#ifndef HT_KEY_DUP_CHECK
#    error "Please define HT_* macros"
#endif

#ifndef HT_KEY_FREE_CB
#    error "Please define HT_* macros"
#endif

#    define HT_NAME_0(prefix, name) prefix##name
#    define HT_NAME_1(prefix, name) HT_NAME_0 (prefix, name)
#    define HT_NAME(name) HT_NAME_1 (HT_PREFIX, name)

struct HT_NAME (ht_entry)
{
    HT_KEY_TYPE key;
    void *data;
    uint64_t cached_hash;
    uint32_t prev_idx, next_idx;
};

struct HT_NAME (htable)
{
    struct HT_NAME (ht_entry) * entries;
    uint32_t *index_list;
    uint32_t *deleted_index_list;
    size_t count;
    uint32_t entry_next_insert;
    uint32_t head_idx, tail_idx;
    size_t index_capacity;
    size_t entry_capacity;
    size_t deleted_count;
    size_t deleted_capacity;
};

#    define ht_iter_begin(table)                                               \
        { .idx = (table)->head_idx,                                            \
          .entry = (table)->entries + (table)->head_idx }
#    define ht_iter_has_next(it) ((it).idx != UINT32_MAX)
#    define ht_iter_next(table, it)                                            \
        (((it).idx = (it).entry->next_idx),                                    \
         ((it).entry = (table)->entries + (it).idx))
#    define ht_iter_get_entry(table, it) ((table)->entries[(it).idx])

#    define ht_foreach_template(ENT, table, it)                                \
        for (struct {                                                          \
                 uint32_t idx;                                                 \
                 ENT *entry;                                                   \
             } it = ht_iter_begin (table);                                     \
             ht_iter_has_next (it); ht_iter_next (table, it))
             