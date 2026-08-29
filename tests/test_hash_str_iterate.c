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
#include "test-common.h"

#define ORDER_N 4
#define EXTRA 200
#define TOTAL (ORDER_N + EXTRA)

static void
make_key (char *buf, size_t bufsize, uint64_t n)
{
    snprintf (buf, bufsize, "k%llu", (unsigned long long) n);
}

int
main (void)
{
    str_htable_t *table = str_htable_create (8);
    CHECK (table != NULL);

    /* An empty table must iterate zero times. */
    size_t visited = 0;
    str_htable_foreach (table, it)
        visited++;
    CHECK (visited == 0);

    /* Insert a handful of keys in a deliberately unsorted order,
       staying well under the initial entry capacity (6, for an
       index_capacity of 8) so nothing forces a rehash here. */
    static const uint64_t order_idx[ORDER_N] = { 2, 0, 3, 1 };
    char key[32];

    for (int i = 0; i < ORDER_N; i++)
    {
        make_key (key, sizeof (key), order_idx[i]);
        CHECK (str_htable_set (table, key,
                               (void *) (uintptr_t) (order_idx[i] + 1)));
    }

    CHECK (str_htable_count (table) == ORDER_N);

    int pos = 0;
    str_htable_foreach (table, it)
    {
        CHECK_MSG (pos < ORDER_N,
                   "iterated past the %d inserted entries", ORDER_N);

        if (pos < ORDER_N)
        {
            make_key (key, sizeof (key), order_idx[pos]);
            CHECK (strcmp (it.entry->key, key) == 0);
            CHECK (it.entry->data
                   == (void *) (uintptr_t) (order_idx[pos] + 1));
            CHECK (strcmp (ht_iter_get_entry (table, it).key, key) == 0);
        }

        pos++;
    }
    CHECK (pos == ORDER_N);

    /* Overwriting an existing key's data must not move it within the
       iteration order or change the entry count. */
    static char overwritten[] = "overwritten";
    make_key (key, sizeof (key), order_idx[1]);
    CHECK (str_htable_set (table, key, overwritten));
    CHECK (str_htable_count (table) == ORDER_N);

    pos = 0;
    str_htable_foreach (table, it)
    {
        if (pos < ORDER_N)
        {
            make_key (key, sizeof (key), order_idx[pos]);
            CHECK (strcmp (it.entry->key, key) == 0);

            void *expected_data = order_idx[pos] == order_idx[1]
                ? overwritten
                : (void *) (uintptr_t) (order_idx[pos] + 1);
            CHECK (it.entry->data == expected_data);
        }

        pos++;
    }
    CHECK (pos == ORDER_N);

    /* Restore the formulaic data (idx + 1) that the growth-phase scan
       below assumes for every key, undoing the overwrite above. */
    make_key (key, sizeof (key), order_idx[1]);
    CHECK (str_htable_set (table, key,
                           (void *) (uintptr_t) (order_idx[1] + 1)));

    /* Force the table through several grow()/rehash cycles and check
       that iteration still visits every live key exactly once with
       the right data, and that each duplicated key string survived
       the rehash intact. Order is intentionally not asserted here,
       for the same reason as in test_hash_int_iterate.c. */
    for (uint64_t i = ORDER_N; i < TOTAL; i++)
    {
        make_key (key, sizeof (key), i);
        CHECK_MSG (str_htable_set (table, key, (void *) (uintptr_t) (i + 1)),
                   "set failed for key %s", key);
    }

    CHECK (str_htable_count (table) == TOTAL);

    bool seen[TOTAL];
    memset (seen, 0, sizeof (seen));
    size_t steps = 0;

    str_htable_foreach (table, it)
    {
        CHECK_MSG (++steps <= TOTAL + 16,
                   "iteration exceeded the expected number of entries "
                   "(possible cycle in the entry list)");

        if (steps > TOTAL + 16)
            break;

        CHECK_MSG (it.entry->key[0] == 'k',
                   "iterated key %s does not look like ours", it.entry->key);

        uint64_t idx = strtoull (it.entry->key + 1, NULL, 10);
        CHECK_MSG (idx < TOTAL, "iterated an unexpected key %s",
                   it.entry->key);

        if (idx < TOTAL)
        {
            CHECK_MSG (!seen[idx], "key %s was visited more than once",
                       it.entry->key);
            seen[idx] = true;
            CHECK (it.entry->data == (void *) (uintptr_t) (idx + 1));
            CHECK (str_htable_get (table, it.entry->key) == it.entry->data);
        }
    }

    for (uint64_t i = 0; i < TOTAL; i++)
        CHECK_MSG (seen[i], "key k%llu was never visited",
                   (unsigned long long) i);

    str_htable_free (table);

    return test_report ();
}
