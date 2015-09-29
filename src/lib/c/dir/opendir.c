/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)opendir.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <stdlib.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * open a directory.
 */
DIR *
opendir(name)
	char *name;
{
	register DIR *dirp;
	register int fd;
	struct stat statBuf;

	if (stat(name, &statBuf) == -1) {
		return NULL;
	}
        if (!S_ISDIR(statBuf.st_mode)) {
                errno = ENOTDIR;
		return NULL;
	}
	if ((fd = open(name, O_RDONLY, 0)) == -1) {
		return NULL;
	}
	if ((dirp = (DIR *)malloc(sizeof(DIR))) == NULL) {
		close (fd);
		return NULL;
	}
	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	return dirp;
}
