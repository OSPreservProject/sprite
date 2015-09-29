#ifndef lint
static char sccsid[] = "@(#)scandir.c	4.2 (Berkeley) 7/1/83";
static char RCSid[] =
"$Header: Scandir.c,v 1.4 84/04/24 22:53:33 lepreau Exp $";
#endif

/*
 * Hacked up version for ded.  Called Scandir() now, note cap.
 * Returns -1 if can't read dir, -2 if out of memory.
 */

/*
 * Scan the directory dirname calling select to make a list of selected
 * directory entries then sort using qsort and compare routine dcomp.
 * Returns the number of entries and a pointer to a list of pointers to
 * struct direct (through namelist). Returns -1 if there were any errors.
 */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef BSD42
# include <sys/dir.h>
# define MINSIZE 12
# define ESTSIZE 24
#else
# include <dir.h>		/* will be ndir.h on some systems */
# define MINSIZE 16		/* min length of a dir entry with inode # */
# define ESTSIZE 20		/* take into acct dir usually not full */
#endif
#include <stdio.h>

#include "ded.h"

/*VARARGS3*/ /* Only way to get lint to shut up about 4th arg == 0 in ded.c */
Scandir(dirname, namelist, select, dcomp)
	char *dirname;
	struct lbuf **namelist;
	int (*select)(), (*dcomp)();
{
	register struct direct *d;
	char *p;
	register struct lbuf *names;
	register int nitems;
	struct stat stb;
	long arraysz;
	DIR *dirp;

	if ((dirp = opendir(dirname)) == NULL)
		return(-1);
	if (fstat(dirp->dd_fd, &stb) < 0)
		return(-1);

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry. 
	 */
	arraysz = (stb.st_size / ESTSIZE);
	names = ALLOC(struct lbuf, arraysz);
	if (names == NULL)
		return(-2);

	nitems = 0;
	while ((d = readdir(dirp)) != NULL) {
		if (select != NULL && !(*select)(d))
			continue;	/* just selected names */
		/*
		 * Make a minimum size copy of the data
		 */
		p = ALLOC(char, d->d_namlen+1);
		if (p == NULL)
			return(-2);
		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */
		if (++nitems >= arraysz) {
			if (fstat(dirp->dd_fd, &stb) < 0)
				return(-1);	/* just might have grown */
			arraysz = stb.st_size/MINSIZE;
			names = RALLOC(struct lbuf, names, arraysz);
			if (names == NULL)
				return(-2);
		}
		names[nitems-1].lino = d->d_ino;
		names[nitems-1].namlen = d->d_namlen;
		bcopy(d->d_name, p, d->d_namlen+1);
		names[nitems-1].namep = p;
		if (nitems % 10 == 0)
  			(void) putchar('.');
	}  
	closedir(dirp);
	if (nitems && dcomp != NULL)
		qsort((char *)names, nitems, sizeof(struct lbuf), dcomp);
	*namelist = names;
	return(nitems);
}
