/* Basic create/set/get/has/count behaviour of str_htable. */

#include <stdbool.h>
#include <stdlib.h>

#include "hash/str_htable.h"
#include "test-common.h"

int
main (void)
{
    str_htable_t *table = str_htable_create (8);
    CHECK (table != NULL);
    CHECK (str_htable_count (table) == 0);

    /* Missing key on an empty table. */
    bool flag = true;
    void *data = str_htable_get_with_flag (table, "missing", &flag);
    CHECK (data == NULL);
    CHECK (flag == false);
    CHECK (str_htable_has (table, "missing") == false);

    /* Insert a handful of keys and read them back. */
    static char payload_a[] = "a";
    static char payload_b[] = "b";
    static char payload_c[] = "c";

    CHECK (str_htable_set (table, "one", payload_a));
    CHECK (str_htable_set (table, "two", payload_b));
    CHECK (str_htable_set (table, "three", payload_c));
    CHECK (str_htable_count (table) == 3);

    CHECK (str_htable_get (table, "one") == payload_a);
    CHECK (str_htable_get (table, "two") == payload_b);
    CHECK (str_htable_get (table, "three") == payload_c);
    CHECK (str_htable_has (table, "one"));
    CHECK (str_htable_has (table, "two"));
    CHECK (str_htable_has (table, "three"));
    CHECK (str_htable_has (table, "nine-hundred-ninety-nine") == false);

    /* Setting an existing key overwrites the data without changing
       count. */
    CHECK (str_htable_set (table, "two", payload_c));
    CHECK (str_htable_count (table) == 3);
    CHECK (str_htable_get (table, "two") == payload_c);

    /* A key stored with NULL data is present (has() is true) even
       though get() alone cannot distinguish it from "missing"; only
       the _with_flag variant can. */
    CHECK (str_htable_set (table, "four", NULL));
    CHECK (str_htable_count (table) == 4);
    CHECK (str_htable_has (table, "four"));
    CHECK (str_htable_get (table, "four") == NULL);

    flag = false;
    data = str_htable_get_with_flag (table, "four", &flag);
    CHECK (data == NULL);
    CHECK (flag == true);

    str_htable_free (table);

    return test_report ();
}
