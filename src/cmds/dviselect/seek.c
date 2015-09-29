/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: seek.c,v 1.1 88/02/11 17:08:56 jim Exp $";
#endif

/*
 * Seekable is a predicate which returns true iff a Unix fd is seekable.
 *
 * MakeSeekable forces an input stdio file to be seekable, by copying to
 * a temporary file if necessary.
 */

#include <stdio.h>
#ifdef sys5
#include <sys/types.h>
#include <sys/fcntl.h>
#else
#include <sys/param.h>
#endif
#include <sys/file.h>
#include <sys/stat.h>

long	lseek();
char	*getenv();

int
Seekable(fd)
	int fd;
{

	return (lseek(fd, 0L, 1) >= 0 && !isatty(fd));
}

/*
 * We use the despicable trick of unlinking an open temporary file.
 * The alternatives are too painful.  If it becomes necessary to
 * do this another way, however, here is a method suggested by Fred
 * Blonder: fork, and have the parent wait for the child to exit.
 * (The parent must avoid being killed during this phase.)  When
 * the child exits, the parent should then remove the temporary file,
 * then exit with the same status, or send itself the same signal.
 */
int
MakeSeekable(f)
	register FILE *f;
{
	register int tf, n;
	int mypid, tries, blksize;
	char *tmpdir;
#ifdef MAXBSIZE
	char buf[MAXBSIZE];
	struct stat st;
#else
	char buf[BUFSIZ];
#endif

	if (Seekable(fileno(f)))
		return (0);

	if ((tmpdir = getenv("TMPDIR")) == 0)
		tmpdir = "/tmp";

	/* compose a suitable temporary file name, and get an r/w descriptor */
	mypid = getpid();
	n = 0;
	tries = 0;
	do {
		sprintf(buf, "%s/#%d.%d", tmpdir, mypid, n++);
		(void) unlink(buf);
#ifdef O_CREAT			/* have three-argument open syscall */
		tries++;
		tf = open(buf, O_RDWR | O_CREAT | O_EXCL, 0666);
#else
		if (access(buf, 0) == 0) {
			/*
			 * Skip existing files.  Note that tf might
			 * not be set yet.
			 */
			tf = -1;
			continue;
		}
		tries++;
		tf = creat(buf, 0666);
		if (tf >= 0) {
			(void) close(tf);
			tf = open(buf, 2);
			if (tf < 0)
				(void) unlink(buf);
		}
#endif
	} while (tf < 0 && tries < 20);
	if (tf < 0)		/* probably unrecoverable user error */
		return (-1);

	(void) unlink(buf);

	/* copy from input file to temp file */
#ifdef MAXBSIZE
	if (fstat(tf, &st))	/* how can this ever fail? */
		blksize = MAXBSIZE;
	else
		blksize = MIN(MAXBSIZE, st.st_blksize);
#else
	blksize = BUFSIZ;
#endif
	while ((n = fread(buf, 1, blksize, f)) > 0) {
		if (write(tf, buf, n) != n) {
			(void) close(tf);
			return (-1);
		}
	}
	/* ferror() is broken in Ultrix 1.2; hence the && */
	if (ferror(f) && !feof(f)) {
		(void) close(tf);
		return (-1);
	}

	/*
	 * Now switch f to point at the temp file.  Since we hit EOF, there
	 * is nothing in f's stdio buffers, so we can play a dirty trick: 
	 */
	clearerr(f);
	if (dup2(tf, fileno(f))) {
		(void) close(tf);
		return (-1);
	}
	(void) close(tf);
	return (0);
}
