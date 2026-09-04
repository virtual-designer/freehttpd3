#ifndef LIBTEST_H
#define LIBTEST_H

#include <stdlib.h>
#include "assert.h"

#define LIBTEST_CONFIG_SYMBOL libtest_config

#define define_test_case(cb) & (struct libtest_test_case) { .name = (#cb), .callback = &(cb) }

typedef int (*libtest_test_callback_t)(void);

struct libtest_test_case
{
    const char *name;
    const libtest_test_callback_t callback;
};

struct libtest_config
{
    const struct libtest_test_case **test_cases;
    const libtest_test_callback_t before_all;
    const libtest_test_callback_t before_each;
    const libtest_test_callback_t after_all;
    const libtest_test_callback_t after_each;
};

int main (int argc, char **argv);

#endif /* LIBTEST_H */
