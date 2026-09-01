/* Key-ownership semantics that are unique to str_htable: unlike
   int_htable (whose HT_KEY_TYPE is a plain scalar), str_htable's keys
   are heap-duplicated on insert (HT_KEY_DUP_CB is strdup) and freed on
   removal (HT_KEY_FREE_CB is free). None of this is exercised by
   int_htable's tests at all, since its dup/free callbacks are no-ops,
   so this file is not a mirror of anything in the int suite. */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hash/str_htable.h"
#include "test-common.h"

int
main (void)
{
    str_htable_t *table = str_htable_create (8);
    CHECK (table != NULL);

    /* The table must copy the key at insert time rather than alias
       the caller's buffer: mutating the buffer after set() must not
       change what is stored. */
    char mutable_key[16];
    strcpy (mutable_key, "original");
    static char payload_a[] = "a";
    CHECK (str_htable_set (table, mutable_key, payload_a));

    strcpy (mutable_key, "mutated!");
    CHECK (str_htable_has (table, "original"));
    CHECK (str_htable_get (table, "original") == payload_a);
    CHECK (str_htable_has (table, "mutated!") == false);

    /* Two distinct allocations holding equal content must be treated
       as the same key (equality is by content via strcmp, not by
       pointer identity). */
    char *heap_key1 = strdup ("shared");
    char *heap_key2 = strdup ("shared");
    CHECK (heap_key1 != NULL && heap_key2 != NULL);
    CHECK (heap_key1 != heap_key2);

    static char payload_b[] = "b";
    static char payload_c[] = "c";
    CHECK (str_htable_set (table, heap_key1, payload_b));
    CHECK (str_htable_count (table) == 2);

    /* Setting via a different pointer with the same content overwrites
       the existing entry instead of creating a second one. */
    CHECK (str_htable_set (table, heap_key2, payload_c));
    CHECK (str_htable_count (table) == 2);
    CHECK (str_htable_get (table, heap_key1) == payload_c);
    CHECK (str_htable_get (table, "shared") == payload_c);

    /* The caller's own buffers were never taken ownership of; freeing
       them here must not corrupt the table's independent copy. */
    free (heap_key1);
    free (heap_key2);
    CHECK (str_htable_get (table, "shared") == payload_c);

    /* Deleting by a freshly-built key (yet another allocation, equal
       by content) must still find and remove the entry. */
    char *heap_key3 = strdup ("shared");
    CHECK (heap_key3 != NULL);
    CHECK (str_htable_delete (table, heap_key3) == payload_c);
    free (heap_key3);
    CHECK (str_htable_has (table, "shared") == false);

    /* The empty string is a legal, distinct key. */
    static char payload_empty[] = "empty";
    CHECK (str_htable_has (table, "") == false);
    CHECK (str_htable_set (table, "", payload_empty));
    CHECK (str_htable_has (table, ""));
    CHECK (str_htable_get (table, "") == payload_empty);
    CHECK (str_htable_delete (table, "") == payload_empty);
    CHECK (str_htable_has (table, "") == false);

    /* Keys spanning rapidhash's various internal length branches
       (empty, short, medium, and long-past-112-bytes) must all
       round-trip correctly once duplicated and hashed. */
    static const char *lengths[] = {
        "",
        "a",
        "abcd",
        "0123456789abcdef",                       /* 16 bytes */
        "0123456789abcdef0123456789abcdef012345",  /* 39 bytes */
        /* 130 bytes, past the 112-byte long-input threshold */
        ("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
         "0123456789abcdef0123456789abcdef0123456789abcdef0123456789ab"),
    };
    static char length_payload[6];

    for (size_t i = 0; i < sizeof (lengths) / sizeof (lengths[0]); i++)
    {
        length_payload[i] = (char) ('A' + i);
        CHECK_MSG (str_htable_set (table, lengths[i], &length_payload[i]),
                   "set failed for length-%zu key", strlen (lengths[i]));
    }

    for (size_t i = 0; i < sizeof (lengths) / sizeof (lengths[0]); i++)
        CHECK (str_htable_get (table, lengths[i]) == &length_payload[i]);

    str_htable_free (table);

    return test_report ();
}
