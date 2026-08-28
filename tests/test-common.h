#ifndef FHTTPD_TEST_COMMON_H
#define FHTTPD_TEST_COMMON_H

#include <stdio.h>
#include <stdlib.h>

static int test_failures = 0;

#define CHECK(cond)                                                         \
    do                                                                      \
    {                                                                       \
        if (!(cond))                                                        \
        {                                                                   \
            fprintf (stderr, "FAIL: %s:%d: CHECK(%s)\n", __FILE__, __LINE__, \
                      #cond);                                                \
            test_failures++;                                                \
        }                                                                   \
    }                                                                       \
    while (0)

#define CHECK_MSG(cond, ...)                                                \
    do                                                                      \
    {                                                                       \
        if (!(cond))                                                        \
        {                                                                   \
            fprintf (stderr, "FAIL: %s:%d: ", __FILE__, __LINE__);          \
            fprintf (stderr, __VA_ARGS__);                                  \
            fprintf (stderr, "\n");                                        \
            test_failures++;                                                \
        }                                                                   \
    }                                                                       \
    while (0)

static inline int
test_report (void)
{
    if (test_failures > 0)
    {
        fprintf (stderr, "%d check(s) failed\n", test_failures);
        return EXIT_FAILURE;
    }

    printf ("all checks passed\n");
    return EXIT_SUCCESS;
}

#endif /* FHTTPD_TEST_COMMON_H */
