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
#include "test-common.h"

#define ORDER_N 4
#define EXTRA 200
#define TOTAL (ORDER_N + EXTRA)

int
main (void)
{
    int_htable_t *table = int_htable_create (8);
    CHECK (table != NULL);

    /* An empty table must iterate zero times. */
    size_t visited = 0;
    int_htable_foreach (table, it)
        visited++;
    CHECK (visited == 0);

    /* Insert a handful of keys in a deliberately unsorted order,
       staying well under the initial entry capacity (6, for an
       index_capacity of 8) so nothing forces a rehash here -- the
       linked list is expected to preserve insertion order, not key
       order or hash order. */
    static const uint64_t order_keys[ORDER_N] = { 2, 0, 3, 1 };

    for (int i = 0; i < ORDER_N; i++)
        CHECK (int_htable_set (table, order_keys[i],
                               (void *) (uintptr_t) (order_keys[i] + 1)));

    CHECK (int_htable_count (table) == ORDER_N);

    int pos = 0;
    int_htable_foreach (table, it)
    {
        CHECK_MSG (pos < ORDER_N,
                   "iterated past the %d inserted entries", ORDER_N);

        if (pos < ORDER_N)
        {
            CHECK (it.entry->key == order_keys[pos]);
            CHECK (it.entry->data
                   == (void *) (uintptr_t) (order_keys[pos] + 1));

            /* ht_iter_get_entry() must agree with the iterator's own
               entry pointer. */
            CHECK (ht_iter_get_entry (table, it).key == it.entry->key);
            CHECK (ht_iter_get_entry (table, it).data == it.entry->data);
        }

        pos++;
    }
    CHECK (pos == ORDER_N);

    /* Overwriting an existing key's data must not move it within the
       iteration order or change the entry count. */
    static char overwritten[] = "overwritten";
    CHECK (int_htable_set (table, order_keys[1], overwritten));
    CHECK (int_htable_count (table) == ORDER_N);

    pos = 0;
    int_htable_foreach (table, it)
    {
        if (pos < ORDER_N)
        {
            uint64_t expected_key = order_keys[pos];
            void *expected_data = expected_key == order_keys[1]
                ? overwritten
                : (void *) (uintptr_t) (expected_key + 1);

            CHECK (it.entry->key == expected_key);
            CHECK (it.entry->data == expected_data);
        }

        pos++;
    }
    CHECK (pos == ORDER_N);

    /* Restore the formulaic data (key + 1) that the growth-phase scan
       below assumes for every key, undoing the overwrite above. */
    CHECK (int_htable_set (table, order_keys[1],
                           (void *) (uintptr_t) (order_keys[1] + 1)));

    /* Force the table through several grow()/rehash cycles and check
       that iteration still visits every live key exactly once with
       the right data. Order is intentionally not asserted here: a
       rehash rebuilds the list from the old table's hash-slot scan
       order rather than replaying insertion order, so only set
       membership and per-entry correctness are part of the
       contract. */
    for (uint64_t key = ORDER_N; key < TOTAL; key++)
        CHECK_MSG (int_htable_set (table, key, (void *) (uintptr_t) (key + 1)),
                   "set failed for key %llu", (unsigned long long) key);

    CHECK (int_htable_count (table) == TOTAL);

    bool seen[TOTAL];
    memset (seen, 0, sizeof (seen));
    size_t steps = 0;

    int_htable_foreach (table, it)
    {
        CHECK_MSG (++steps <= TOTAL + 16,
                   "iteration exceeded the expected number of entries "
                   "(possible cycle in the entry list)");

        if (steps > TOTAL + 16)
            break;

        uint64_t key = it.entry->key;
        CHECK_MSG (key < TOTAL, "iterated an unexpected key %llu",
                   (unsigned long long) key);

        if (key < TOTAL)
        {
            CHECK_MSG (!seen[key], "key %llu was visited more than once",
                       (unsigned long long) key);
            seen[key] = true;
            CHECK (it.entry->data == (void *) (uintptr_t) (key + 1));
            CHECK (int_htable_get (table, key) == it.entry->data);
        }
    }

    for (uint64_t key = 0; key < TOTAL; key++)
        CHECK_MSG (seen[key], "key %llu was never visited",
                   (unsigned long long) key);

    int_htable_free (table);

    return test_report ();
}
