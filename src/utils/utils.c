#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils.h"
#include "types.h"

_noreturn void
freeze (void)
{
	while (true)
		sleep (1000);
}

bool
fd_set_nonblocking (fd_t fd)
{
	int flags = fcntl (fd, F_GETFL, 0);

	if (flags < 0)
		return false;

	return fcntl (fd, F_SETFL, flags | O_NONBLOCK) == 0;
}
