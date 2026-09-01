/* Interaction between int_htable_foreach() and deletion: whether
   removing a middle entry, the head, the tail, every remaining
   entry, or a tombstoned-then-reinserted key leaves the iteration
   list (head_idx/tail_idx/prev_idx/next_idx) correctly linked. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "test-common.h"

static void
check_order (int_htable_t *table, const uint64_t *expected, size_t n)
{
    size_t pos = 0;

    int_htable_foreach (table, it)
    {
        CHECK_MSG (pos < n, "iteration produced more entries than expected "
                             "(possible cycle)");

        if (pos < n)
            CHECK_MSG (it.entry->key == expected[pos],
                       "position %zu: got key %llu, expected %llu", pos,
                       (unsigned long long) it.entry->key,
                       (unsigned long long) expected[pos]);

        pos++;
    }

    CHECK_MSG (pos == n, "expected %zu entries, iterated %zu", n, pos);
}

int
main (void)
{
    /* Deleting a middle entry: the relink branch for two live
       neighbours does not touch head_idx/tail_idx. */
    {
        int_htable_t *table = int_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 10, 20, 30, 40 };
        for (size_t i = 0; i < 4; i++)
            CHECK (int_htable_set (table, keys[i],
                                   (void *) (uintptr_t) (keys[i] + 1)));

        CHECK (int_htable_delete (table, 20) == (void *) (uintptr_t) 21);
        CHECK (int_htable_count (table) == 3);

        static const uint64_t expected[] = { 10, 30, 40 };
        check_order (table, expected, 3);

        int_htable_free (table);
    }

    /* A tombstoned slot reused by a later insert must be re-appended
       at the new tail, not resurrected in its old position. */
    {
        int_htable_t *table = int_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 1, 2, 3 };
        for (size_t i = 0; i < 3; i++)
            CHECK (int_htable_set (table, keys[i],
                                   (void *) (uintptr_t) (keys[i] + 1)));

        CHECK (int_htable_delete (table, 2) == (void *) (uintptr_t) 3);
        CHECK (int_htable_set (table, 4, (void *) (uintptr_t) 5));
        CHECK (int_htable_count (table) == 3);

        static const uint64_t expected[] = { 1, 3, 4 };
        check_order (table, expected, 3);

        int_htable_free (table);
    }

    /* Deleting the head of a multi-entry table, then deleting the
       new head too, so a stale head_idx cannot hide behind a single
       lucky pass. */
    {
        int_htable_t *table = int_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 1, 2, 3 };
        for (size_t i = 0; i < 3; i++)
            CHECK (int_htable_set (table, keys[i],
                                   (void *) (uintptr_t) (keys[i] + 1)));

        CHECK (int_htable_delete (table, 1) == (void *) (uintptr_t) 2);
        CHECK (int_htable_count (table) == 2);

        static const uint64_t expected1[] = { 2, 3 };
        check_order (table, expected1, 2);

        CHECK (int_htable_delete (table, 2) == (void *) (uintptr_t) 3);
        CHECK (int_htable_count (table) == 1);

        static const uint64_t expected2[] = { 3 };
        check_order (table, expected2, 1);

        int_htable_free (table);
    }

    /* Deleting the tail of a multi-entry table, then inserting a
       fresh key, so a stale tail_idx cannot hide behind a single
       lucky pass: if tail_idx still pointed at the removed node,
       htable_set()'s append-at-tail logic would link the new entry
       onto a dead node instead of the real (new) tail, orphaning it
       from iteration even though it stays reachable via get(). */
    {
        int_htable_t *table = int_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 1, 2, 3 };
        for (size_t i = 0; i < 3; i++)
            CHECK (int_htable_set (table, keys[i],
                                   (void *) (uintptr_t) (keys[i] + 1)));

        CHECK (int_htable_delete (table, 3) == (void *) (uintptr_t) 4);
        CHECK (int_htable_count (table) == 2);

        static const uint64_t expected[] = { 1, 2 };
        check_order (table, expected, 2);

        CHECK (int_htable_set (table, 4, (void *) (uintptr_t) 5));
        CHECK (int_htable_count (table) == 3);
        CHECK (int_htable_get (table, 4) == (void *) (uintptr_t) 5);

        static const uint64_t expected2[] = { 1, 2, 4 };
        check_order (table, expected2, 3);

        int_htable_free (table);
    }

    /* Deleting every entry from the head, one at a time, must drain
       the list to empty -- also exercises the single-remaining-entry
       case (head_idx == tail_idx). */
    {
        int_htable_t *table = int_htable_create (8);
        CHECK (table != NULL);

        static const uint64_t keys[] = { 1, 2, 3 };
        for (size_t i = 0; i < 3; i++)
            CHECK (int_htable_set (table, keys[i],
                                   (void *) (uintptr_t) (keys[i] + 1)));

        for (size_t i = 0; i < 3; i++)
            CHECK (int_htable_delete (table, keys[i])
                   == (void *) (uintptr_t) (keys[i] + 1));

        CHECK (int_htable_count (table) == 0);
        check_order (table, NULL, 0);

        int_htable_free (table);
    }

    return test_report ();
}
