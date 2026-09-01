/* Deletion, tombstone reuse, and count bookkeeping for str_htable. */

#include <stdbool.h>
#include <stdlib.h>

#include "hash/str_htable.h"
#include "test-common.h"

int
main (void)
{
    str_htable_t *table = str_htable_create (8);
    CHECK (table != NULL);

    static char payload_a[] = "a";
    static char payload_b[] = "b";

    /* Deleting from an empty table is a well-defined no-op. */
    bool flag = true;
    void *data = str_htable_delete_with_flag (table, "missing", &flag);
    CHECK (data == NULL);
    CHECK (flag == false);
    CHECK (str_htable_delete (table, "missing") == NULL);

    CHECK (str_htable_set (table, "alpha", payload_a));
    CHECK (str_htable_set (table, "beta", payload_b));
    CHECK (str_htable_count (table) == 2);

    /* Deleting an existing key returns its data, marks it absent,
       and decrements count. */
    flag = false;
    data = str_htable_delete_with_flag (table, "alpha", &flag);
    CHECK (data == payload_a);
    CHECK (flag == true);
    CHECK (str_htable_count (table) == 1);
    CHECK (str_htable_has (table, "alpha") == false);
    CHECK (str_htable_get (table, "alpha") == NULL);

    /* The other key is unaffected. */
    CHECK (str_htable_has (table, "beta"));
    CHECK (str_htable_get (table, "beta") == payload_b);

    /* Deleting the same key twice is safe; the second call finds
       nothing. */
    CHECK (str_htable_delete (table, "alpha") == NULL);
    CHECK (str_htable_count (table) == 1);

    /* A deleted key can be reinserted (exercises tombstone reuse in
       htable_set) and is retrievable again afterwards. */
    static char payload_c[] = "c";
    CHECK (str_htable_set (table, "alpha", payload_c));
    CHECK (str_htable_count (table) == 2);
    CHECK (str_htable_get (table, "alpha") == payload_c);

    CHECK (str_htable_delete (table, "alpha") == payload_c);
    CHECK (str_htable_delete (table, "beta") == payload_b);
    CHECK (str_htable_count (table) == 0);

    str_htable_free (table);

    return test_report ();
}
