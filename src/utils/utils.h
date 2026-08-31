#ifndef FHTTPD_UTILS_H
#define FHTTPD_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include "types.h"

#define MIN_VALUE(v1, v2) (((v1) <= (v2)) ? (v1) : (v2))
#define MAX_VALUE(v1, v2) (((v1) >= (v2)) ? (v1) : (v2))

bool util_fd_set_nonblocking (fd_t fd);
uint64_t util_round_ceil2_ull(uint64_t value);

#endif /* FHTTPD_UTILS_H */
