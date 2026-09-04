/* Correctness of htable_grow()/rehash across many insertions, well
   beyond the initial capacity, plus survival of a bulk delete/reinsert
   pass after growth has happened. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "libtest.h"

#define N 4000

static int_htable_t *table;

static int
before_all (void)
{
    /* Deliberately tiny so the table is forced through several doublings
       while inserting N entries. */
    table = int_htable_create (4);

    return assert_true (table != NULL, "failed to create the table");
}

static int
after_all (void)
{
    int_htable_free (table);

    return ASSERT_OK;
}

/* Keys are offset from 1 so none of them collide with the reserved
   sentinel values (HT_EMPTY == 0) at the value level; that reservation
   lives in the index list, not the key space, but starting at 1 keeps
   this test's intent obvious. */

static int
test_hash_int_insert_past_capacity (void)
{
    for (uint64_t i = 0; i < N; i++)
        assert_true (int_htable_set (table, i + 1, (void *) (uintptr_t) (i + 1)),
                     "set failed for key %llu", (unsigned long long) (i + 1));

    check_equal (int_htable_count (table), N);

    return ASSERT_OK;
}

static int
test_hash_int_read_back_after_growth (void)
{
    for (uint64_t i = 0; i < N; i++)
    {
        void *data = int_htable_get (table, i + 1);

        assert_true (data == (void *) (uintptr_t) (i + 1),
                     "wrong data for key %llu after growth",
                     (unsigned long long) (i + 1));
    }

    return ASSERT_OK;
}

/* Delete every other key, forcing tombstones to accumulate across an
   already-grown table, then verify the survivors are still correct and
   the deleted keys are gone. */

static int
test_hash_int_delete_every_other (void)
{
    for (uint64_t i = 0; i < N; i += 2)
        check_true (int_htable_delete (table, i + 1)
                    == (void *) (uintptr_t) (i + 1));

    check_equal (int_htable_count (table), N / 2);

    for (uint64_t i = 0; i < N; i++)
    {
        bool should_exist = (i % 2) != 0;

        check_true (int_htable_has (table, i + 1) == should_exist);
    }

    return ASSERT_OK;
}

/* Insert a fresh batch of keys that did not exist before, so the insert
   path is exercised again after tombstones and multiple rehashes have
   accumulated. */

static int
test_hash_int_insert_after_tombstones (void)
{
    for (uint64_t i = 0; i < N; i++)
    {
        uint64_t key = N + i + 1;

        check_true (int_htable_set (table, key, (void *) (uintptr_t) key));
    }

    check_equal (int_htable_count (table), N / 2 + N);

    for (uint64_t i = 0; i < N; i++)
    {
        uint64_t key = N + i + 1;

        check_true (int_htable_get (table, key) == (void *) (uintptr_t) key);
    }

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_int_insert_past_capacity),
        define_test_case(test_hash_int_read_back_after_growth),
        define_test_case(test_hash_int_delete_every_other),
        define_test_case(test_hash_int_insert_after_tombstones),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
