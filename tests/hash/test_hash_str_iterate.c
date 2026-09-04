/* Iteration mechanics of str_htable, driven entirely through the
   str_htable_foreach()/ht_iter_* macros -- string-keyed counterpart
   of test_hash_int_iterate.c. Covers empty-table behaviour,
   insertion-order preservation before any rehash, overwrite-in-place
   not perturbing the list, and full set/data correctness after
   growth has forced several rehashes (which also exercises that
   rehash carries the duplicated key strings over correctly, not just
   the linked-list bookkeeping). Interaction between iteration and
   deletion is covered separately in test_hash_str_iterate_delete.c. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash/str_htable.h"
#include "libtest.h"

#define ORDER_N 4
#define EXTRA 200
#define TOTAL (ORDER_N + EXTRA)

static str_htable_t *table;

static const uint64_t order_idx[ORDER_N] = { 2, 0, 3, 1 };
static char overwritten[] = "overwritten";

static void
make_key (char *buf, size_t bufsize, uint64_t n)
{
    snprintf (buf, bufsize, "k%llu", (unsigned long long) n);
}

static int
before_all (void)
{
    table = str_htable_create (8);

    return assert_true (table != NULL, "failed to create the table");
}

static int
after_all (void)
{
    str_htable_free (table);

    return ASSERT_OK;
}

/* An empty table must iterate zero times. */

static int
test_hash_str_iterate_empty (void)
{
    size_t visited = 0;

    str_htable_foreach (table, it)
        visited++;

    return check_equal (visited, 0);
}

/* Insert a handful of keys in a deliberately unsorted order, staying
   well under the initial entry capacity (6, for an index_capacity of 8)
   so nothing forces a rehash here. */

static int
test_hash_str_iterate_insertion_order (void)
{
    char key[32];

    for (int i = 0; i < ORDER_N; i++)
    {
        make_key (key, sizeof (key), order_idx[i]);
        check_true (str_htable_set (table, key,
                                    (void *) (uintptr_t) (order_idx[i] + 1)));
    }

    check_equal (str_htable_count (table), ORDER_N);

    int pos = 0;

    str_htable_foreach (table, it)
    {
        assert_true (pos < ORDER_N, "iterated past the %d inserted entries",
                     ORDER_N);

        if (pos < ORDER_N)
        {
            make_key (key, sizeof (key), order_idx[pos]);
            check_equal (strcmp (it.entry->key, key), 0);
            check_true (it.entry->data
                        == (void *) (uintptr_t) (order_idx[pos] + 1));
            check_equal (strcmp (ht_iter_get_entry (table, it).key, key), 0);
        }

        pos++;
    }

    check_equal (pos, ORDER_N);

    return ASSERT_OK;
}

/* Overwriting an existing key's data must not move it within the
   iteration order or change the entry count. */

static int
test_hash_str_iterate_after_overwrite (void)
{
    char key[32];

    make_key (key, sizeof (key), order_idx[1]);
    check_true (str_htable_set (table, key, overwritten));
    check_equal (str_htable_count (table), ORDER_N);

    int pos = 0;

    str_htable_foreach (table, it)
    {
        if (pos < ORDER_N)
        {
            make_key (key, sizeof (key), order_idx[pos]);
            check_equal (strcmp (it.entry->key, key), 0);

            void *expected_data
                = order_idx[pos] == order_idx[1]
                      ? overwritten
                      : (void *) (uintptr_t) (order_idx[pos] + 1);

            check_true (it.entry->data == expected_data);
        }

        pos++;
    }

    check_equal (pos, ORDER_N);

    return ASSERT_OK;
}

/* Force the table through several grow()/rehash cycles and check that
   iteration still visits every live key exactly once with the right
   data, and that each duplicated key string survived the rehash intact.
   Order is intentionally not asserted here, for the same reason as in
   test_hash_int_iterate.c. */

static int
test_hash_str_iterate_after_growth (void)
{
    char key[32];

    /* Restore the formulaic data (idx + 1) that the scan below assumes
       for every key, undoing the overwrite from the previous case. */
    make_key (key, sizeof (key), order_idx[1]);
    check_true (
        str_htable_set (table, key, (void *) (uintptr_t) (order_idx[1] + 1)));

    for (uint64_t i = ORDER_N; i < TOTAL; i++)
    {
        make_key (key, sizeof (key), i);
        assert_true (str_htable_set (table, key, (void *) (uintptr_t) (i + 1)),
                     "set failed for key %s", key);
    }

    check_equal (str_htable_count (table), TOTAL);

    bool seen[TOTAL];
    memset (seen, 0, sizeof (seen));
    size_t steps = 0;

    str_htable_foreach (table, it)
    {
        assert_true (++steps <= TOTAL + 16,
                     "iteration exceeded the expected number of entries "
                     "(possible cycle in the entry list)");

        if (steps > TOTAL + 16)
            break;

        assert_equal (it.entry->key[0], 'k',
                      "iterated key %s does not look like ours",
                      it.entry->key);

        uint64_t idx = strtoull (it.entry->key + 1, NULL, 10);

        assert_true (idx < TOTAL, "iterated an unexpected key %s",
                     it.entry->key);

        if (idx < TOTAL)
        {
            assert_false (seen[idx], "key %s was visited more than once",
                          it.entry->key);
            seen[idx] = true;
            check_true (it.entry->data == (void *) (uintptr_t) (idx + 1));
            check_true (str_htable_get (table, it.entry->key)
                        == it.entry->data);
        }
    }

    for (uint64_t i = 0; i < TOTAL; i++)
        assert_true (seen[i], "key k%llu was never visited",
                     (unsigned long long) i);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_str_iterate_empty),
        define_test_case(test_hash_str_iterate_insertion_order),
        define_test_case(test_hash_str_iterate_after_overwrite),
        define_test_case(test_hash_str_iterate_after_growth),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
