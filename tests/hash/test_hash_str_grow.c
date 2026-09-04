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
#include "libtest.h"

#define N 4000

static str_htable_t *table;

static void
make_key (char *buf, size_t bufsize, const char *prefix, uint64_t n)
{
    snprintf (buf, bufsize, "%s%llu", prefix, (unsigned long long) n);
}

static int
before_all (void)
{
    /* Deliberately tiny so the table is forced through several doublings
       while inserting N entries. */
    table = str_htable_create (4);

    return assert_true (table != NULL, "failed to create the table");
}

static int
after_all (void)
{
    str_htable_free (table);

    return ASSERT_OK;
}

static int
test_hash_str_insert_past_capacity (void)
{
    char key[32];

    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "k", i + 1);
        assert_true (str_htable_set (table, key, (void *) (uintptr_t) (i + 1)),
                     "set failed for key %s", key);
    }

    check_equal (str_htable_count (table), N);

    return ASSERT_OK;
}

static int
test_hash_str_read_back_after_growth (void)
{
    char key[32];

    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "k", i + 1);

        void *data = str_htable_get (table, key);

        assert_true (data == (void *) (uintptr_t) (i + 1),
                     "wrong data for key %s after growth", key);
    }

    return ASSERT_OK;
}

/* Delete every other key, forcing tombstones to accumulate across an
   already-grown table, then verify the survivors are still correct and
   the deleted keys are gone. */

static int
test_hash_str_delete_every_other (void)
{
    char key[32];

    for (uint64_t i = 0; i < N; i += 2)
    {
        make_key (key, sizeof (key), "k", i + 1);
        check_true (str_htable_delete (table, key)
                    == (void *) (uintptr_t) (i + 1));
    }

    check_equal (str_htable_count (table), N / 2);

    for (uint64_t i = 0; i < N; i++)
    {
        bool should_exist = (i % 2) != 0;

        make_key (key, sizeof (key), "k", i + 1);
        check_true (str_htable_has (table, key) == should_exist);
    }

    return ASSERT_OK;
}

/* Insert a fresh batch of keys that did not exist before, so the insert
   path is exercised again after tombstones and multiple rehashes have
   accumulated. */

static int
test_hash_str_insert_after_tombstones (void)
{
    char key[32];

    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "j", i + 1);
        check_true (str_htable_set (table, key,
                                    (void *) (uintptr_t) (N + i + 1)));
    }

    check_equal (str_htable_count (table), N / 2 + N);

    for (uint64_t i = 0; i < N; i++)
    {
        make_key (key, sizeof (key), "j", i + 1);
        check_true (str_htable_get (table, key)
                    == (void *) (uintptr_t) (N + i + 1));
    }

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_str_insert_past_capacity),
        define_test_case(test_hash_str_read_back_after_growth),
        define_test_case(test_hash_str_delete_every_other),
        define_test_case(test_hash_str_insert_after_tombstones),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
