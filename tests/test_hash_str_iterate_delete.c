/* Interaction between str_htable_foreach() and deletion -- string-
   keyed counterpart of test_hash_int_iterate_delete.c. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash/str_htable.h"
#include "test-common.h"

static void
make_key (char *buf, size_t bufsize, uint64_t n)
{
    snprintf (buf, bufsize, "k%llu", (unsigned long long) n);
}

static void
check_order (str_htable_t *table, const uint64_t *expected, size_t n)
{
    size_t pos = 0;
    char key[32];

    str_htable_foreach (table, it)
    {
        CHECK_MSG (pos < n, "iteration produced more entries than expected "
                             "(possible cycle)");

        if (pos < n)
        {
            make_key (key, sizeof (key), expected[pos]);
            CHECK_MSG (strcmp (it.entry->key, key) == 0,
                       "position %zu: got key \"%s\", expected \"%s\"", pos,
                       it.entry->key, key);
        }

        pos++;
    }

    CHECK_MSG (pos == n, "expected %zu entries, iterated %zu", n, pos);
}

int
main (void)
{
    char key[32];

    /* Deleting a middle entry: the relink branch for two live
       neighbours does not touch head_idx/tail_idx. */
    {
        str_htable_t *table = str_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 10, 20, 30, 40 };
        for (size_t i = 0; i < 4; i++)
        {
            make_key (key, sizeof (key), keys[i]);
            CHECK (str_htable_set (table, key,
                                   (void *) (uintptr_t) (keys[i] + 1)));
        }

        make_key (key, sizeof (key), 20);
        CHECK (str_htable_delete (table, key) == (void *) (uintptr_t) 21);
        CHECK (str_htable_count (table) == 3);

        static const uint64_t expected[] = { 10, 30, 40 };
        check_order (table, expected, 3);

        str_htable_free (table);
    }

    /* A tombstoned slot reused by a later insert must be re-appended
       at the new tail, not resurrected in its old position. */
    {
        str_htable_t *table = str_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 1, 2, 3 };
        for (size_t i = 0; i < 3; i++)
        {
            make_key (key, sizeof (key), keys[i]);
            CHECK (str_htable_set (table, key,
                                   (void *) (uintptr_t) (keys[i] + 1)));
        }

        make_key (key, sizeof (key), 2);
        CHECK (str_htable_delete (table, key) == (void *) (uintptr_t) 3);
        make_key (key, sizeof (key), 4);
        CHECK (str_htable_set (table, key, (void *) (uintptr_t) 5));
        CHECK (str_htable_count (table) == 3);

        static const uint64_t expected[] = { 1, 3, 4 };
        check_order (table, expected, 3);

        str_htable_free (table);
    }

    /* Deleting the head of a multi-entry table, then deleting the
       new head too, so a stale head_idx cannot hide behind a single
       lucky pass. */
    {
        str_htable_t *table = str_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 1, 2, 3 };
        for (size_t i = 0; i < 3; i++)
        {
            make_key (key, sizeof (key), keys[i]);
            CHECK (str_htable_set (table, key,
                                   (void *) (uintptr_t) (keys[i] + 1)));
        }

        make_key (key, sizeof (key), 1);
        CHECK (str_htable_delete (table, key) == (void *) (uintptr_t) 2);
        CHECK (str_htable_count (table) == 2);

        static const uint64_t expected1[] = { 2, 3 };
        check_order (table, expected1, 2);

        make_key (key, sizeof (key), 2);
        CHECK (str_htable_delete (table, key) == (void *) (uintptr_t) 3);
        CHECK (str_htable_count (table) == 1);

        static const uint64_t expected2[] = { 3 };
        check_order (table, expected2, 1);

        str_htable_free (table);
    }

    /* Deleting the tail of a multi-entry table, then inserting a
       fresh key, so a stale tail_idx cannot hide behind a single
       lucky pass: if tail_idx still pointed at the removed node,
       htable_set()'s append-at-tail logic would link the new entry
       onto a dead node instead of the real (new) tail, orphaning it
       from iteration even though it stays reachable via get(). */
    {
        str_htable_t *table = str_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 1, 2, 3 };
        for (size_t i = 0; i < 3; i++)
        {
            make_key (key, sizeof (key), keys[i]);
            CHECK (str_htable_set (table, key,
                                   (void *) (uintptr_t) (keys[i] + 1)));
        }

        make_key (key, sizeof (key), 3);
        CHECK (str_htable_delete (table, key) == (void *) (uintptr_t) 4);
        CHECK (str_htable_count (table) == 2);

        static const uint64_t expected[] = { 1, 2 };
        check_order (table, expected, 2);

        make_key (key, sizeof (key), 4);
        CHECK (str_htable_set (table, key, (void *) (uintptr_t) 5));
        CHECK (str_htable_count (table) == 3);
        CHECK (str_htable_get (table, key) == (void *) (uintptr_t) 5);

        static const uint64_t expected2[] = { 1, 2, 4 };
        check_order (table, expected2, 3);

        str_htable_free (table);
    }

    /* Deleting every entry from the head, one at a time, must drain
       the list to empty -- also exercises the single-remaining-entry
       case (head_idx == tail_idx). */
    {
        str_htable_t *table = str_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 1, 2, 3 };
        for (size_t i = 0; i < 3; i++)
        {
            make_key (key, sizeof (key), keys[i]);
            CHECK (str_htable_set (table, key,
                                   (void *) (uintptr_t) (keys[i] + 1)));
        }

        for (size_t i = 0; i < 3; i++)
        {
            make_key (key, sizeof (key), keys[i]);
            CHECK (str_htable_delete (table, key)
                   == (void *) (uintptr_t) (keys[i] + 1));
        }

        CHECK (str_htable_count (table) == 0);
        check_order (table, NULL, 0);

        str_htable_free (table);
    }

    return test_report ();
}
