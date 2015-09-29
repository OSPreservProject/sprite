#ifndef F_DUPFD
#include <fcntl.h>
#endif

int
dup2 (old, new)
int old, new;
{
	(void) close(new);

	return fcntl(old, F_DUPFD, new);
}
