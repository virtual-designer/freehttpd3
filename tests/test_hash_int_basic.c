/* Basic create/set/get/has/count behaviour of int_htable. */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hash/int_htable.h"
#include "test-common.h"

int
main (void)
{
    int_htable_t *table = int_htable_create (8, NULL);
    CHECK (table != NULL);
    CHECK (int_htable_count (table) == 0);

    /* Missing key on an empty table. */
    bool flag = true;
    void *data = int_htable_get_with_flag (table, 42, &flag);
    CHECK (data == NULL);
    CHECK (flag == false);
    CHECK (int_htable_has (table, 42) == false);

    /* Insert a handful of keys and read them back. */
    static char payload_a[] = "a";
    static char payload_b[] = "b";
    static char payload_c[] = "c";

    CHECK (int_htable_set (table, 1, payload_a));
    CHECK (int_htable_set (table, 2, payload_b));
    CHECK (int_htable_set (table, 3, payload_c));
    CHECK (int_htable_count (table) == 3);

    CHECK (int_htable_get (table, 1) == payload_a);
    CHECK (int_htable_get (table, 2) == payload_b);
    CHECK (int_htable_get (table, 3) == payload_c);
    CHECK (int_htable_has (table, 1));
    CHECK (int_htable_has (table, 2));
    CHECK (int_htable_has (table, 3));
    CHECK (int_htable_has (table, 999) == false);

    /* Setting an existing key overwrites the data without changing
       count. */
    CHECK (int_htable_set (table, 2, payload_c));
    CHECK (int_htable_count (table) == 3);
    CHECK (int_htable_get (table, 2) == payload_c);

    /* A key stored with NULL data is present (has() is true) even
       though get() alone cannot distinguish it from "missing"; only
       the _with_flag variant can. */
    CHECK (int_htable_set (table, 4, NULL));
    CHECK (int_htable_count (table) == 4);
    CHECK (int_htable_has (table, 4));
    CHECK (int_htable_get (table, 4) == NULL);

    flag = false;
    data = int_htable_get_with_flag (table, 4, &flag);
    CHECK (data == NULL);
    CHECK (flag == true);

    int_htable_free (table);

    return test_report ();
}
