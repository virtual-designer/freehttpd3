/* Interaction between int_htable_foreach() and deletion: whether
   removing a middle entry, the head, the tail, every remaining
   entry, or a tombstoned-then-reinserted key leaves the iteration
   list (head_idx/tail_idx/prev_idx/next_idx) correctly linked. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "libtest.h"

static void
check_order (int_htable_t *table, const uint64_t *expected, size_t n)
{
    size_t pos = 0;

    int_htable_foreach (table, it)
    {
        assert_true (pos < n, "iteration produced more entries than expected "
                              "(possible cycle)");

        if (pos < n)
            assert_equal (it.entry->key, expected[pos],
                          "position %zu: got key %llu, expected %llu", pos,
                          (unsigned long long) it.entry->key,
                          (unsigned long long) expected[pos]);

        pos++;
    }

    assert_equal (pos, n, "expected %zu entries, iterated %zu", n, pos);
}

static int_htable_t *
make_table (const uint64_t *keys, size_t n)
{
    int_htable_t *table = int_htable_create (8);

    check_true (table != NULL);

    for (size_t i = 0; i < n; i++)
        check_true (
            int_htable_set (table, keys[i], (void *) (uintptr_t) (keys[i] + 1)));

    return table;
}

/* Deleting a middle entry: the relink branch for two live neighbours
   does not touch head_idx/tail_idx. */

static int
test_hash_int_delete_middle (void)
{
    static const uint64_t keys[] = { 10, 20, 30, 40 };
    static const uint64_t expected[] = { 10, 30, 40 };

    int_htable_t *table = make_table (keys, 4);

    check_true (int_htable_delete (table, 20) == (void *) (uintptr_t) 21);
    check_equal (int_htable_count (table), 3);
    check_order (table, expected, 3);

    int_htable_free (table);

    return ASSERT_OK;
}

/* A tombstoned slot reused by a later insert must be re-appended at the
   new tail, not resurrected in its old position. */

static int
test_hash_int_reinsert_appends_at_tail (void)
{
    static const uint64_t keys[] = { 1, 2, 3 };
    static const uint64_t expected[] = { 1, 3, 4 };

    int_htable_t *table = make_table (keys, 3);

    check_true (int_htable_delete (table, 2) == (void *) (uintptr_t) 3);
    check_true (int_htable_set (table, 4, (void *) (uintptr_t) 5));
    check_equal (int_htable_count (table), 3);
    check_order (table, expected, 3);

    int_htable_free (table);

    return ASSERT_OK;
}

/* Deleting the head of a multi-entry table, then deleting the new head
   too, so a stale head_idx cannot hide behind a single lucky pass. */

static int
test_hash_int_delete_head (void)
{
    static const uint64_t keys[] = { 1, 2, 3 };
    static const uint64_t expected1[] = { 2, 3 };
    static const uint64_t expected2[] = { 3 };

    int_htable_t *table = make_table (keys, 3);

    check_true (int_htable_delete (table, 1) == (void *) (uintptr_t) 2);
    check_equal (int_htable_count (table), 2);
    check_order (table, expected1, 2);

    check_true (int_htable_delete (table, 2) == (void *) (uintptr_t) 3);
    check_equal (int_htable_count (table), 1);
    check_order (table, expected2, 1);

    int_htable_free (table);

    return ASSERT_OK;
}

/* Deleting the tail of a multi-entry table, then inserting a fresh key,
   so a stale tail_idx cannot hide behind a single lucky pass: if
   tail_idx still pointed at the removed node, htable_set()'s
   append-at-tail logic would link the new entry onto a dead node instead
   of the real (new) tail, orphaning it from iteration even though it
   stays reachable via get(). */

static int
test_hash_int_delete_tail (void)
{
    static const uint64_t keys[] = { 1, 2, 3 };
    static const uint64_t expected[] = { 1, 2 };
    static const uint64_t expected2[] = { 1, 2, 4 };

    int_htable_t *table = make_table (keys, 3);

    check_true (int_htable_delete (table, 3) == (void *) (uintptr_t) 4);
    check_equal (int_htable_count (table), 2);
    check_order (table, expected, 2);

    check_true (int_htable_set (table, 4, (void *) (uintptr_t) 5));
    check_equal (int_htable_count (table), 3);
    check_true (int_htable_get (table, 4) == (void *) (uintptr_t) 5);
    check_order (table, expected2, 3);

    int_htable_free (table);

    return ASSERT_OK;
}

/* Deleting every entry from the head, one at a time, must drain the list
   to empty -- also exercises the single-remaining-entry case (head_idx
   == tail_idx). */

static int
test_hash_int_delete_all (void)
{
    static const uint64_t keys[] = { 1, 2, 3 };

    int_htable_t *table = make_table (keys, 3);

    for (size_t i = 0; i < 3; i++)
        check_true (int_htable_delete (table, keys[i])
                    == (void *) (uintptr_t) (keys[i] + 1));

    check_equal (int_htable_count (table), 0);
    check_order (table, NULL, 0);

    int_htable_free (table);

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_int_delete_middle),
        define_test_case(test_hash_int_reinsert_appends_at_tail),
        define_test_case(test_hash_int_delete_head),
        define_test_case(test_hash_int_delete_tail),
        define_test_case(test_hash_int_delete_all),
        NULL,
    },
};
