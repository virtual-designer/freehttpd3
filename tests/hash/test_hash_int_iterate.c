/* Iteration mechanics of int_htable, driven entirely through the
   int_htable_foreach()/ht_iter_* macros: empty-table behaviour,
   insertion-order preservation on a table that has not yet been
   rehashed, overwrite-in-place not perturbing the iteration list,
   and full set/data correctness after growth has forced several
   rehashes. Interaction between iteration and deletion (which
   mutates the head/tail links) is covered separately in
   test_hash_int_iterate_delete.c. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hash/int_htable.h"
#include "libtest.h"

#define ORDER_N 4
#define EXTRA 200
#define TOTAL (ORDER_N + EXTRA)

static int_htable_t *table;

static const uint64_t order_keys[ORDER_N] = { 2, 0, 3, 1 };
static char overwritten[] = "overwritten";

static int
before_all (void)
{
    table = int_htable_create (8);

    return assert_true (table != NULL, "failed to create the table");
}

static int
after_all (void)
{
    int_htable_free (table);

    return ASSERT_OK;
}

/* An empty table must iterate zero times. */

static int
test_hash_int_iterate_empty (void)
{
    size_t visited = 0;

    int_htable_foreach (table, it)
        visited++;

    return check_equal (visited, 0);
}

/* Insert a handful of keys in a deliberately unsorted order, staying
   well under the initial entry capacity (6, for an index_capacity of 8)
   so nothing forces a rehash here -- the linked list is expected to
   preserve insertion order, not key order or hash order. */

static int
test_hash_int_iterate_insertion_order (void)
{
    for (int i = 0; i < ORDER_N; i++)
        check_true (int_htable_set (table, order_keys[i],
                                    (void *) (uintptr_t) (order_keys[i] + 1)));

    check_equal (int_htable_count (table), ORDER_N);

    int pos = 0;

    int_htable_foreach (table, it)
    {
        assert_true (pos < ORDER_N, "iterated past the %d inserted entries",
                     ORDER_N);

        if (pos < ORDER_N)
        {
            check_equal (it.entry->key, order_keys[pos]);
            check_true (it.entry->data
                        == (void *) (uintptr_t) (order_keys[pos] + 1));

            /* ht_iter_get_entry() must agree with the iterator's own
               entry pointer. */
            check_equal (ht_iter_get_entry (table, it).key, it.entry->key);
            check_true (ht_iter_get_entry (table, it).data == it.entry->data);
        }

        pos++;
    }

    check_equal (pos, ORDER_N);

    return ASSERT_OK;
}

/* Overwriting an existing key's data must not move it within the
   iteration order or change the entry count. */

static int
test_hash_int_iterate_after_overwrite (void)
{
    check_true (int_htable_set (table, order_keys[1], overwritten));
    check_equal (int_htable_count (table), ORDER_N);

    int pos = 0;

    int_htable_foreach (table, it)
    {
        if (pos < ORDER_N)
        {
            uint64_t expected_key = order_keys[pos];
            void *expected_data = expected_key == order_keys[1]
                                      ? overwritten
                                      : (void *) (uintptr_t) (expected_key + 1);

            check_equal (it.entry->key, expected_key);
            check_true (it.entry->data == expected_data);
        }

        pos++;
    }

    check_equal (pos, ORDER_N);

    return ASSERT_OK;
}

/* Force the table through several grow()/rehash cycles and check that
   iteration still visits every live key exactly once with the right
   data. Order is intentionally not asserted here: a rehash rebuilds the
   list from the old table's hash-slot scan order rather than replaying
   insertion order, so only set membership and per-entry correctness are
   part of the contract. */

static int
test_hash_int_iterate_after_growth (void)
{
    /* Restore the formulaic data (key + 1) that the scan below assumes
       for every key, undoing the overwrite from the previous case. */
    check_true (int_htable_set (table, order_keys[1],
                                (void *) (uintptr_t) (order_keys[1] + 1)));

    for (uint64_t key = ORDER_N; key < TOTAL; key++)
        assert_true (int_htable_set (table, key,
                                     (void *) (uintptr_t) (key + 1)),
                     "set failed for key %llu", (unsigned long long) key);

    check_equal (int_htable_count (table), TOTAL);

    bool seen[TOTAL];
    memset (seen, 0, sizeof (seen));
    size_t steps = 0;

    int_htable_foreach (table, it)
    {
        assert_true (++steps <= TOTAL + 16,
                     "iteration exceeded the expected number of entries "
                     "(possible cycle in the entry list)");

        if (steps > TOTAL + 16)
            break;

        uint64_t key = it.entry->key;

        assert_true (key < TOTAL, "iterated an unexpected key %llu",
                     (unsigned long long) key);

        if (key < TOTAL)
        {
            assert_false (seen[key], "key %llu was visited more than once",
                          (unsigned long long) key);
            seen[key] = true;
            check_true (it.entry->data == (void *) (uintptr_t) (key + 1));
            check_true (int_htable_get (table, key) == it.entry->data);
        }
    }

    for (uint64_t key = 0; key < TOTAL; key++)
        assert_true (seen[key], "key %llu was never visited",
                     (unsigned long long) key);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_int_iterate_empty),
        define_test_case(test_hash_int_iterate_insertion_order),
        define_test_case(test_hash_int_iterate_after_overwrite),
        define_test_case(test_hash_int_iterate_after_growth),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
