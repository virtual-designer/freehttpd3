#include <fcntl.h>

#include "utils.h"

bool 
util_fd_set_nonblocking (fd_t fd)
{
    int flags = fcntl (fd, F_GETFL);
    
    if (flags < 0)
        return false;

    return fcntl (fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}