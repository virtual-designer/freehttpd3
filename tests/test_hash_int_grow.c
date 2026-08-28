/* Correctness of htable_grow()/rehash across many insertions, well
   beyond the initial capacity, plus survival of a bulk delete/reinsert
   pass after growth has happened. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "test-common.h"

#define N 4000

int
main (void)
{
    /* Deliberately tiny so the table is forced through several
       doublings while inserting N entries. */
    int_htable_t *table = int_htable_create (4, NULL);
    CHECK (table != NULL);

    /* Keys are offset from 1 so none of them collide with the
       reserved sentinel values (HT_EMPTY == 0) at the value level;
       that reservation lives in the index list, not the key space,
       but starting at 1 keeps this test's intent obvious. */
    for (uint64_t i = 0; i < N; i++)
        CHECK_MSG (int_htable_set (table, i + 1, (void *) (uintptr_t) (i + 1)),
                   "set failed for key %llu", (unsigned long long) (i + 1));

    CHECK (int_htable_count (table) == N);

    for (uint64_t i = 0; i < N; i++)
    {
        void *data = int_htable_get (table, i + 1);
        CHECK_MSG (data == (void *) (uintptr_t) (i + 1),
                   "wrong data for key %llu after growth",
                   (unsigned long long) (i + 1));
    }

    /* Delete every other key, forcing tombstones to accumulate across
       an already-grown table, then verify the survivors are still
       correct and the deleted keys are gone. */
    for (uint64_t i = 0; i < N; i += 2)
        CHECK (int_htable_delete (table, i + 1) == (void *) (uintptr_t) (i + 1));

    CHECK (int_htable_count (table) == N / 2);

    for (uint64_t i = 0; i < N; i++)
    {
        bool should_exist = (i % 2) != 0;
        CHECK (int_htable_has (table, i + 1) == should_exist);
    }

    /* Insert a fresh batch of keys that did not exist before, so the
       insert path is exercised again after tombstones and multiple
       rehashes have accumulated. */
    for (uint64_t i = 0; i < N; i++)
    {
        uint64_t key = N + i + 1;
        CHECK (int_htable_set (table, key, (void *) (uintptr_t) key));
    }

    CHECK (int_htable_count (table) == N / 2 + N);

    for (uint64_t i = 0; i < N; i++)
    {
        uint64_t key = N + i + 1;
        CHECK (int_htable_get (table, key) == (void *) (uintptr_t) key);
    }

    int_htable_free (table);

    return test_report ();
}
