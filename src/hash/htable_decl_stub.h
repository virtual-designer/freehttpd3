struct HT_NAME (htable);
struct HT_NAME (ht_entry);
typedef struct HT_NAME (htable) HT_NAME (htable_t);
typedef struct HT_NAME (ht_entry) HT_NAME (ht_entry_t);
struct HT_NAME (htable)
    * HT_NAME (htable_create) (size_t initial_cap);
void *HT_NAME (htable_get) (const struct HT_NAME (htable) * table,
                            const HT_KEY_TYPE key);
void *HT_NAME (htable_get_with_flag) (const struct HT_NAME (htable) * table,
                                      const HT_KEY_TYPE key, bool *flag);
bool HT_NAME (htable_has) (const struct HT_NAME (htable) * table,
                           const HT_KEY_TYPE key);
bool HT_NAME (htable_set) (struct HT_NAME (htable) * table,
                           const HT_KEY_TYPE key, void *data);
size_t HT_NAME (htable_count) (const struct HT_NAME (htable) * table);
void *HT_NAME (htable_delete_with_flag) (struct HT_NAME (htable) * table,
                                         const HT_KEY_TYPE key, bool *flag);
void *HT_NAME (htable_delete) (struct HT_NAME (htable) * table,
                               const HT_KEY_TYPE key);
void HT_NAME (htable_free_with_cleanup) (struct HT_NAME (htable) * table,
                                         ht_data_free_cb_t data_free_cb);
void HT_NAME (htable_free) (struct HT_NAME (htable) * table);
