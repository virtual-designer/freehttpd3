#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "htable.h"
#include "utils/utils.h"

#define HT_DELETED (UINT32_MAX)
#define HT_EMPTY (0)

#define htable_t struct HT_NAME (htable)
#define ht_entry_t struct HT_NAME (ht_entry)

struct HT_NAME (ht_entry)
{
    HT_KEY_TYPE key;
    void *data;
};

struct HT_NAME (htable)
{
    ht_entry_t *entries;
    uint32_t *index_list;
    uint32_t *deleted_index_list;
    size_t count;
    uint32_t entry_next_insert;
    size_t index_capacity;
    size_t entry_capacity;
    size_t deleted_count;
    size_t deleted_capacity;
};

htable_t *
HT_NAME (htable_create) (size_t initial_cap)
{
    assert (initial_cap >= 2);

    htable_t *table = malloc (sizeof (*table));

    if (!table)
        return NULL;

    table->count = 0;
    table->entry_next_insert = 0;
    table->deleted_index_list = NULL;
    table->deleted_capacity = 0;
    table->deleted_count = 0;
    table->index_capacity = util_round_ceil2_ull (initial_cap);
    table->entry_capacity = (table->index_capacity * 3) / 4;
    table->index_list
        = calloc (table->index_capacity, sizeof (*table->index_list));

    if (!table->index_list)
    {
        free (table);
        return NULL;
    }

    table->entries = malloc (table->entry_capacity * sizeof (*table->entries));

    if (!table->entries)
    {
        free (table->index_list);
        free (table);
        return NULL;
    }

    return table;
}

static bool
HT_NAME (htable_rehash) (htable_t *table, uint32_t *new_index_list,
                         size_t new_index_capacity, ht_entry_t *new_entries,
                         size_t new_entry_capacity)
{
    const uint64_t mask = new_index_capacity - 1;
    uint32_t new_idx_counter = 0;

    for (size_t i = 0; i < table->index_capacity; i++)
    {
        if (table->index_list[i] == HT_EMPTY
            || table->index_list[i] == HT_DELETED)
            continue;

        const uint32_t idx = table->index_list[i] - 1;
        const uint32_t new_idx = new_idx_counter++;
        const ht_entry_t *prev_entry = &table->entries[idx];
        uint64_t hash = HT_KEY_HASH_CB (prev_entry->key) & mask;
        uint64_t insert_target_hash = UINT64_MAX;

        new_entries[new_idx] = *prev_entry;

        for (size_t n = 0; n < new_index_capacity; n++)
        {
            const uint32_t idx_element = new_index_list[hash];

            if (idx_element == HT_EMPTY)
            {
                insert_target_hash = hash;
                break;
            }

            hash = (hash + 1) & mask;
        }

        if (insert_target_hash == UINT64_MAX)
            return false;

        new_index_list[insert_target_hash] = new_idx + 1;
    }

    table->entry_next_insert = new_idx_counter;
    table->deleted_count = 0;

    return true;
}

static bool
HT_NAME (htable_grow) (htable_t *table)
{
    size_t new_index_capacity = table->index_capacity << 1;
    size_t new_entry_capacity = (new_index_capacity * 3) / 4;

    uint32_t *new_index_list
        = calloc (new_index_capacity, sizeof (*table->index_list));

    if (!new_index_list)
        return false;

    ht_entry_t *new_entries
        = malloc (new_entry_capacity * sizeof (*table->entries));

    if (!new_entries)
    {
        free (new_index_list);
        return false;
    }

    const bool ret
        = HT_NAME (htable_rehash) (table, new_index_list, new_index_capacity,
                                   new_entries, new_entry_capacity);

    if (!ret)
    {
        free (new_index_list);
        free (new_entries);
        return false;
    }

    free (table->entries);
    free (table->index_list);

    table->index_list = new_index_list;
    table->index_capacity = new_index_capacity;
    table->entries = new_entries;
    table->entry_capacity = new_entry_capacity;

    return true;
}

bool
HT_NAME (htable_set) (htable_t *table, const HT_KEY_TYPE key, void *data)
{
    if ((table->entry_next_insert >= table->entry_capacity
         || (table->count + 1) * 4 >= table->index_capacity * 3)
        && !HT_NAME (htable_grow) (table))
    {
        return false;
    }

    const uint64_t mask = table->index_capacity - 1;
    uint64_t hash = HT_KEY_HASH_CB (key) & mask;
    uint64_t insert_target_hash = UINT64_MAX;

    for (size_t n = 0; n < table->index_capacity; n++)
    {
        const uint32_t idx_element = table->index_list[hash];

        switch (idx_element)
        {
            case HT_DELETED:
                if (insert_target_hash == UINT64_MAX)
                    insert_target_hash = hash;

                goto hash_loop_next;

            case HT_EMPTY:
                if (insert_target_hash == UINT64_MAX)
                    insert_target_hash = hash;

                goto hash_loop_end;
        }

        const uint32_t idx = idx_element - 1;
        ht_entry_t *entry = &table->entries[idx];

        if (HT_KEY_EQUAL_CB (entry->key, key))
        {
            entry->data = data;
            return true;
        }

    hash_loop_next:
        hash = (hash + 1) & mask;
    }

hash_loop_end:
    if (insert_target_hash == UINT64_MAX)
        return false;

    HT_KEY_TYPE dup_key = HT_KEY_DUP_CB (key);
    HT_KEY_DUP_CHECK (dup_key);

    uint32_t idx;

    if (table->deleted_count)
    {
        idx = table->deleted_index_list[--table->deleted_count];

        if (table->deleted_capacity > 16
            && table->deleted_count < (table->deleted_capacity >> 2))
        {
            const size_t new_deleted_capacity
                = table->deleted_count < 4 ? 4 : (table->deleted_capacity >> 1);
            uint32_t *new_deleted_index_list = realloc (
                table->deleted_index_list,
                sizeof (*table->deleted_index_list) * (new_deleted_capacity));

            if (new_deleted_index_list)
            {
                table->deleted_capacity = new_deleted_capacity;
                table->deleted_index_list = new_deleted_index_list;
            }
        }
    }
    else
    {
        idx = table->entry_next_insert++;
    }

    table->index_list[insert_target_hash] = idx + 1;
    table->entries[idx].key = dup_key;
    table->entries[idx].data = data;
    table->count++;

    return true;
}

void *
HT_NAME (htable_get_with_flag) (const htable_t *table, const HT_KEY_TYPE key,
                                bool *flag)
{
    const uint64_t mask = table->index_capacity - 1;
    uint64_t hash = HT_KEY_HASH_CB (key) & mask;

    for (size_t n = 0; n < table->index_capacity; n++)
    {
        const uint32_t idx_element = table->index_list[hash];

        switch (idx_element)
        {
            case HT_DELETED:
                break;

            case HT_EMPTY:
                *flag = false;
                return NULL;

            default:
                {
                    const ht_entry_t *entry = &table->entries[idx_element - 1];

                    if (HT_KEY_EQUAL_CB (entry->key, key))
                    {
                        *flag = true;
                        return entry->data;
                    }

                    break;
                }
        }

        hash = (hash + 1) & mask;
    }

    *flag = false;
    return NULL;
}

void *
HT_NAME (htable_get) (const htable_t *table, const HT_KEY_TYPE key)
{
    bool flag;
    return HT_NAME (htable_get_with_flag) (table, key, &flag);
}

bool
HT_NAME (htable_has) (const htable_t *table, const HT_KEY_TYPE key)
{
    bool flag = false;
    HT_NAME (htable_get_with_flag) (table, key, &flag);
    return flag;
}

void *
HT_NAME (htable_delete_with_flag) (htable_t *table, const HT_KEY_TYPE key,
                                   bool *flag)
{
    const uint64_t mask = table->index_capacity - 1;
    uint64_t hash = HT_KEY_HASH_CB (key) & mask;

    for (size_t n = 0; n < table->index_capacity; n++)
    {
        const uint32_t idx_element = table->index_list[hash];

        switch (idx_element)
        {
            case HT_DELETED:
                break;

            case HT_EMPTY:
                *flag = false;
                return NULL;

            default:
                {
                    const ht_entry_t *entry = &table->entries[idx_element - 1];

                    if (HT_KEY_EQUAL_CB (entry->key, key))
                    {
                        if (table->deleted_count >= table->deleted_capacity)
                        {
                            const size_t new_deleted_capacity
                                = table->deleted_capacity
                                      ? table->deleted_capacity << 1
                                      : 16;

                            uint32_t *new_deleted_index_list
                                = realloc (table->deleted_index_list,
                                           sizeof (*table->deleted_index_list)
                                               * (new_deleted_capacity));

                            if (!new_deleted_index_list)
                            {
                                *flag = false;
                                return NULL;
                            }

                            table->deleted_index_list = new_deleted_index_list;
                            table->deleted_capacity = new_deleted_capacity;
                        }

                        HT_KEY_FREE_CB (entry->key);
                        table->deleted_index_list[table->deleted_count++]
                            = idx_element - 1;

                        table->index_list[hash] = HT_DELETED;
                        table->count--;
                        *flag = true;
                        return entry->data;
                    }

                    break;
                }
        }

        hash = (hash + 1) & mask;
    }

    *flag = false;
    return NULL;
}

void *
HT_NAME (htable_delete) (htable_t *table, const HT_KEY_TYPE key)
{
    bool flag = false;
    return HT_NAME (htable_delete_with_flag) (table, key, &flag);
}

size_t
HT_NAME (htable_count) (const htable_t *table)
{
    return table->count;
}

void
HT_NAME (htable_free_with_cleanup) (htable_t *table,
                                    ht_data_free_cb_t data_free_cb)
{
    for (size_t i = 0; i < table->index_capacity; i++)
    {
        const uint32_t idx_element = table->index_list[i];

        if (idx_element == HT_EMPTY || idx_element == HT_DELETED)
            continue;

        ht_entry_t *entry = &table->entries[idx_element - 1];
        HT_KEY_FREE_CB (entry->key);

        if (data_free_cb)
            data_free_cb (entry->data);
    }

    free (table->deleted_index_list);
    free (table->index_list);
    free (table->entries);
    free (table);
}

void
HT_NAME (htable_free) (htable_t *table)
{
    HT_NAME (htable_free_with_cleanup) (table, NULL);
}

#undef htable_t
#undef ht_entry_t
