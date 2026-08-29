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
#include "test-common.h"

#define KEYSET 8
#define CYCLES 5000

int
main (void)
{
    str_htable_t *table = str_htable_create (4);
    CHECK (table != NULL);

    char key[16];

    for (int cycle = 0; cycle < CYCLES; cycle++)
    {
        for (uint64_t i = 0; i < KEYSET; i++)
        {
            snprintf (key, sizeof (key), "k%llu", (unsigned long long) i);
            CHECK_MSG (
                str_htable_set (table, key, (void *) (uintptr_t) (i + 1)),
                "set failed for key %s in cycle %d", key, cycle);
        }

        CHECK (str_htable_count (table) == KEYSET);

        for (uint64_t i = 0; i < KEYSET; i++)
        {
            snprintf (key, sizeof (key), "k%llu", (unsigned long long) i);
            CHECK (str_htable_get (table, key) == (void *) (uintptr_t) (i + 1));
        }

        /* Delete in a different order than insertion to avoid always
           exercising the same probe pattern. */
        for (uint64_t i = KEYSET; i-- > 0;)
        {
            snprintf (key, sizeof (key), "k%llu", (unsigned long long) i);
            CHECK (str_htable_delete (table, key)
                   == (void *) (uintptr_t) (i + 1));
        }

        CHECK (str_htable_count (table) == 0);

        for (uint64_t i = 0; i < KEYSET; i++)
        {
            snprintf (key, sizeof (key), "k%llu", (unsigned long long) i);
            CHECK (str_htable_has (table, key) == false);
        }
    }

    str_htable_free (table);

    return test_report ();
}
