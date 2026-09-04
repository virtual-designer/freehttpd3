/* Soak test: repeatedly insert and delete a rotating set of string
   keys many times over. str_htable counterpart of
   test_hash_int_churn.c; unlike the int version, every insert here
   duplicates a key string (strdup) and every delete frees it, so this
   also functions as a stress test of that allocation lifecycle
   (KEYSET * CYCLES * 2 dup/free pairs) without leaking or
   double-freeing. */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hash/str_htable.h"
#include "libtest.h"

#define KEYSET 8
#define CYCLES 5000

static str_htable_t *table;

static int
before_all (void)
{
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
test_hash_str_churn (void)
{
    char key[16];

    for (int cycle = 0; cycle < CYCLES; cycle++)
    {
        for (uint64_t i = 0; i < KEYSET; i++)
        {
            snprintf (key, sizeof (key), "k%llu", (unsigned long long) i);
            assert_true (str_htable_set (table, key,
                                         (void *) (uintptr_t) (i + 1)),
                         "set failed for key %s in cycle %d", key, cycle);
        }

        check_equal (str_htable_count (table), KEYSET);

        for (uint64_t i = 0; i < KEYSET; i++)
        {
            snprintf (key, sizeof (key), "k%llu", (unsigned long long) i);
            check_true (str_htable_get (table, key)
                        == (void *) (uintptr_t) (i + 1));
        }

        /* Delete in a different order than insertion to avoid always
           exercising the same probe pattern. */
        for (uint64_t i = KEYSET; i-- > 0;)
        {
            snprintf (key, sizeof (key), "k%llu", (unsigned long long) i);
            check_true (str_htable_delete (table, key)
                        == (void *) (uintptr_t) (i + 1));
        }

        check_equal (str_htable_count (table), 0);

        for (uint64_t i = 0; i < KEYSET; i++)
        {
            snprintf (key, sizeof (key), "k%llu", (unsigned long long) i);
            check_false (str_htable_has (table, key));
        }
    }

    return ASSERT_OK;
}

const struct libtest_config LIBTEST_CONFIG_SYMBOL = {
    .test_cases = (const struct libtest_test_case *[]) {
        define_test_case(test_hash_str_churn),
        NULL,
    },
    .before_all = &before_all,
    .after_all = &after_all,
};
