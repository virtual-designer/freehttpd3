#include <stdbool.h>
#include <unistd.h>

#include "utils.h"

_noreturn void
freeze (void)
{
	while (true)
		sleep (1000);
}
