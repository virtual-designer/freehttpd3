/* Soak test: repeatedly insert and delete a rotating set of keys many
   times over. This drives the table through many grow()/rehash cycles
   purely from tombstone accumulation (see htable_set()'s
   "(count + deleted_count + 1) * 4 >= index_capacity * 3" growth
   trigger) and exercises the insert/delete/get paths together rather
   than in isolation, without asserting on internal capacity (the
   table is opaque from this test's point of view; the entries[]
   array's lack of compaction on delete is a design property noted in
   the code review, not something a black-box functional test can
   observe directly). */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "libtest.h"

#define KEYSET 8
#define CYCLES 5000

static int_htable_t *table;

static int
before_all (void)
{
    table = int_htable_create (4);

    return assert_true (table != NULL, "failed to create the table");
}

static int
after_all (void)
{
    int_htable_free (table);

    return ASSERT_OK;
}

static int
test_hash_int_churn (void)
{
    for (int cycle = 0; cycle < CYCLES; cycle++)
    {
        for (uint64_t i = 0; i < KEYSET; i++)
            assert_true (int_htable_set (table, i, (void *) (uintptr_t) (i + 1)),
                         "set failed for key %llu in cycle %d",
                         (unsigned long long) i, cycle);

        check_equal (int_htable_count (table), KEYSET);

        for (uint64_t i = 0; i < KEYSET; i++)
            check_true (int_htable_get (table, i)
                        == (void *) (uintptr_t) (i + 1));

        /* Delete in a different order than insertion to avoid always
           exercising the same probe pattern. */
        for (uint64_t i = KEYSET; i-- > 0;)
            check_true (int_htable_delete (table, i)
                        == (void *) (uintptr_t) (i + 1));

        check_equal (int_htable_count (table), 0);

        for (uint64_t i = 0; i < KEYSET; i++)
            check_false (int_htable_has (table, i));
    }

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_int_churn),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
