#ifndef lint
static char rcsid[] = "$Id: subr.c,v 1.14.1.2 91/01/29 19:46:20 berliner Exp $";
#endif !lint

/*
 *    Copyright (c) 1989, Brian Berliner
 *
 *    You may distribute under the terms of the GNU General Public License
 *    as specified in the README file that comes with the CVS 1.0 kit.
 *
 * Various useful functions for the CVS support code.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <varargs.h>
#include "cvs.h"

/*
 * Send a "printf" format string to stderr and die, calling the
 * defined exit function first, if necessary
 */
error(doperror, fmt, va_alist)
    int doperror;
    char *fmt;
    va_dcl
{
    extern int errno;
    va_list x1;
    int err = errno;

    va_start(x1);
    (void) fprintf(stderr, "%s: ", progname);
    (void) vfprintf(stderr, fmt, x1);
    if (doperror) {
	(void) fprintf(stderr, ": ");
	errno = err;
	perror("");
	errno = 0;
    } else
	(void) fprintf(stderr, "\n");
    va_end(x1);
    Lock_Cleanup(0);
    exit(1);
}

/*
 * Like error() above, but just display the message to stderr,
 * without dying or running the exit function.
 */
warn(doperror, fmt, va_alist)
    int doperror;
    char *fmt;
    va_dcl
{
    extern int errno;
    va_list x1;
    int err = errno;

    va_start(x1);
    (void) fprintf(stderr, "%s: ", progname);
    (void) vfprintf(stderr, fmt, x1);
    if (doperror) {
	(void) fprintf(stderr, ": ");
	errno = err;
	perror("");
	errno = 0;
    } else
	(void) fprintf(stderr, "\n");
    va_end(x1);
}

/*
 * Copies "from" to "to".
 * mallocs a buffer large enough to hold the entire file and
 * does one read/one write to do the copy.  This is reasonable,
 * since source files are typically not too large.
 */
copy_file(from, to)
    char *from;
    char *to;
{
    struct stat sb;
    int fdin, fdout;
    char *buf;

    if ((fdin = open(from, O_RDONLY)) < 0)
	error(1, "cannot open %s for copying", from);
    if (fstat(fdin, &sb) < 0)
	error(1, "cannot fstat %s", from);
    if ((fdout = creat(to, (int) sb.st_mode & 07777)) < 0)
	error(1, "cannot create %s for copying", to);
    if (sb.st_size > 0) {
	buf = xmalloc((int)sb.st_size);
	if (read(fdin, buf, (int)sb.st_size) != (int)sb.st_size)
	    error(1, "cannot read file %s for copying", from);
	if (write(fdout, buf, (int)sb.st_size) != (int)sb.st_size)
	    error(1, "cannot write file %s for copying", to);
	free(buf);
    }
    (void) close(fdin);
    (void) close(fdout);
}

/*
 * Returns non-zero if the argument file is a directory, or
 * is a symbolic link which points to a directory.
 */
isdir(file)
    char *file;
{
    struct stat sb;

    if (stat(file, &sb) < 0)
	return (0);
    return ((sb.st_mode & S_IFMT) == S_IFDIR);
}

/*
 * Returns non-zero if the argument file is a symbolic link.
 */
islink(file)
    char *file;
{
    struct stat sb;

#ifdef S_IFLNK
    if (lstat(file, &sb) < 0)
	return (0);
    return ((sb.st_mode & S_IFMT) == S_IFLNK);
#else
    return (0);
#endif
}

/*
 * Returns non-zero if the argument file exists.
 */
isfile(file)
    char *file;
{
    struct stat sb;

    if (stat(file, &sb) < 0)
	return (0);
    return (1);
}

/*
 * Returns non-zero if the argument file is readable.
 * XXX - muct be careful if "cvs" is ever made setuid!
 */
isreadable(file)
    char *file;
{
    return (access(file, R_OK) != -1);
}

/*
 * Returns non-zero if the argument file is writable
 * XXX - muct be careful if "cvs" is ever made setuid!
 */
iswritable(file)
    char *file;
{
    return (access(file, W_OK) != -1);
}

/*
 * Open a file and die if it fails
 */
FILE *
open_file(name, mode)
    char *name;
    char *mode;
{
    FILE *fp;

    if ((fp = fopen(name, mode)) == NULL)
	error(1, "cannot open %s", name);
    return (fp);
}

/*
 * Make a directory and die if it fails
 */
make_directory(name)
    char *name;
{
    if (mkdir(name, 0777) < 0)
	error(1, "cannot make directory %s", name);
}

/*
 * malloc some data and die if it fails
 */
char *
xmalloc(bytes)
    int bytes;
{
    extern char *malloc();
    char *cp;

    if (bytes <= 0)
	error(0, "bad malloc size %d", bytes);
    if ((cp = malloc((unsigned)bytes)) == NULL)
	error(0, "malloc failed");
    return (cp);
}


/*
 * ppstrcmp() is a front-end for strcmp() when the arguments
 * are pointers to pointers to chars.
 */
ppstrcmp(pp1, pp2)
    register char **pp1, **pp2;
{
    return (strcmp(*pp1, *pp2));
}

/*
 * ppstrcmp_files() is a front-end for strcmp() when the arguments
 * are pointers to pointers to chars.
 * For some reason, the ppstrcmp() above sorts in reverse order when
 * called from Entries2Files().
 */
ppstrcmp_files(pp1, pp2)
    register char **pp1, **pp2;
{
    /*
     * Reversing the arguments here cause for the
     * correct alphabetical order sort, as we desire.
     */
    return (strcmp(*pp2, *pp1));
}

/* Some UNIX distributions don't include these in their stat.h */
#ifndef S_IWRITE
#define	S_IWRITE	0000200		/* write permission, owner */
#endif !S_IWRITE
#ifndef S_IWGRP
#define	S_IWGRP		0000020		/* write permission, grougroup */
#endif !S_IWGRP
#ifndef S_IWOTH
#define	S_IWOTH		0000002		/* write permission, other */
#endif !S_IWOTH

/*
 * Change the mode of a file, either adding write permissions, or
 * removing all write permissions.  Adding write permissions honors
 * the current umask setting.
 */
xchmod(fname, writable)
    char *fname;
    int writable;
{
    struct stat sb;
    int mode, oumask;
    int		fd;
    uid_t	euid;

    if (stat(fname, &sb) < 0) {
	warn(1, "cannot stat %s", fname);
	return;
    }
    euid = geteuid();
    if (sb.st_uid == euid) { 
	if (writable) {
	    oumask = umask(0);
	    (void) umask(oumask);
	    mode = sb.st_mode | ((S_IWRITE|S_IWGRP|S_IWOTH) & ~oumask);
	} else {
	    mode = sb.st_mode & ~(S_IWRITE|S_IWGRP|S_IWOTH);
	}
	if (chmod(fname, mode) < 0)
	    warn(1, "cannot change mode of file %s", fname);
    } else {
	fd = open(fname, writable ? O_WRONLY : O_RDONLY);
	if (fd < 0) {
	    warn(1, "no %s access to file %s", 
		writable ? "write" : "read", fname);
	} else {
	    close(fd);
	}
    }
}

/*
 * Rename a file and die if it fails
 */
rename_file(from, to)
    char *from;
    char *to;
{
    if (rename(from, to) < 0)
	error(1, "cannot rename file %s to %s", from, to);
}

/*
 * Compare "file1" to "file2".
 * Return non-zero if they don't compare exactly.
 *
 * mallocs a buffer large enough to hold the entire file and
 * does two reads to load the buffer and calls bcmp to do the cmp.
 * This is reasonable, since source files are typically not too large.
 */
xcmp(file1, file2)
    char *file1;
    char *file2;
{
    register char *buf1, *buf2;
    struct stat sb;
    off_t size;
    int ret, fd1, fd2;

    if ((fd1 = open(file1, O_RDONLY)) < 0)
	error(1, "cannot open file %s for comparing", file1);
    if ((fd2 = open(file2, O_RDONLY)) < 0)
	error(1, "cannot open file %s for comparing", file2);
    if (fstat(fd1, &sb) < 0)
	error(1, "cannot fstat %s", file1);
    size = sb.st_size;
    if (fstat(fd2, &sb) < 0)
	error(1, "cannot fstat %s", file2);
    if (size == sb.st_size) {
	if (size == 0)
	    ret = 0;
	else {
	    buf1 = xmalloc((int)size);
	    buf2 = xmalloc((int)size);
	    if (read(fd1, buf1, (int)size) != (int)size)
		error(1, "cannot read file %s cor comparing", file1);
	    if (read(fd2, buf2, (int)size) != (int)size)
		error(1, "cannot read file %s for comparing", file2);
	    ret = bcmp(buf1, buf2, (int)size);
	    free(buf1);
	    free(buf2);
	}
    } else
	ret = 1;
    (void) close(fd1);
    (void) close(fd2);
    return (ret);
}

/*
 * Recover the space allocated by Find_Names() and line2argv()
 */
free_names(pargc, argv)
    int *pargc;
    char *argv[];
{
    register int i;

    for (i = 0; i < *pargc; i++) {	/* only do through *pargc */
	free(argv[i]);
    }
    *pargc = 0;				/* and set it to zero when done */
}

/*
 * Convert a line into argc/argv components and return the result in
 * the arguments as passed.  Use free_names() to return the memory
 * allocated here back to the free pool.
 */
line2argv(pargc, argv, line)
    int *pargc;
    char *argv[];
    char *line;
{
    char *cp;

    *pargc = 0;
    for (cp = strtok(line, " \t"); cp; cp = strtok((char *)NULL, " \t")) {
	argv[*pargc] = xmalloc(strlen(cp) + 1);
	(void) strcpy(argv[*pargc], cp);
	(*pargc)++;
    }
}

/*
 * Returns the number of dots ('.') found in an RCS revision number
 */
numdots(s)
    char *s;
{
    char *cp;
    int dots = 0;

    for (cp = s; *cp; cp++) {
	if (*cp == '.')
	    dots++;
    }
    return (dots);
}
