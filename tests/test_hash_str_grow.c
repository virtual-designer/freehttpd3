/* Correctness of htable_grow()/rehash across many insertions, well
   beyond the initial capacity, plus survival of a bulk delete/reinsert
   pass after growth has happened -- str_htable counterpart of
   test_hash_int_grow.c. Also exercises key duplication under growth:
   every rehash copies cached_hash but must still be able to look
   entries up by content afterwards, which only works if the
   duplicated key strings themselves survived the rehash intact. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash/str_htable.h"
#include "test-common.h"

#define N 4000

static void
make_key (char *buf, size_t bufsize, const char *prefix, uint64_t n)
{
    snprintf (buf, bufsize, "%s%llu", prefix, (unsigned long long) n);
}

int
main (void)
{
    /* Deliberately tiny so the table is forced through several
       doublings while inserting N entries. */
    str_htable_t *table = str_htable_create (4);
    CHECK (table != NULL);

    char key[32];

    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "k", i + 1);
        CHECK_MSG (str_htable_set (table, key, (void *) (uintptr_t) (i + 1)),
                   "set failed for key %s", key);
    }

    CHECK (str_htable_count (table) == N);

    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "k", i + 1);
        void *data = str_htable_get (table, key);
        CHECK_MSG (data == (void *) (uintptr_t) (i + 1),
                   "wrong data for key %s after growth", key);
    }

    /* Delete every other key, forcing tombstones to accumulate across
       an already-grown table, then verify the survivors are still
       correct and the deleted keys are gone. */
    for (uint64_t i = 0; i < N; i += 2)
    {
        make_key (key, sizeof (key), "k", i + 1);
        CHECK (str_htable_delete (table, key) == (void *) (uintptr_t) (i + 1));
    }

    CHECK (str_htable_count (table) == N / 2);

    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "k", i + 1);
        bool should_exist = (i % 2) != 0;
        CHECK (str_htable_has (table, key) == should_exist);
    }

    /* Insert a fresh batch of keys that did not exist before, so the
       insert path is exercised again after tombstones and multiple
       rehashes have accumulated. */
    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "j", i + 1);
        CHECK (str_htable_set (table, key, (void *) (uintptr_t) (N + i + 1)));
    }

    CHECK (str_htable_count (table) == N / 2 + N);

    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "j", i + 1);
        CHECK (str_htable_get (table, key) == (void *) (uintptr_t) (N + i + 1));
    }

    str_htable_free (table);

    return test_report ();
}
