#ifndef lint
static char RCSid[] =
"$Header: subr.c,v 1.3 84/05/01 18:16:12 lepreau Exp $";
#endif

/*
 * Miscellaneous functions. Currently (almost) none of them
 * reference any globals.
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "ded.h"

extern int nprint;
extern int errno;

    /* this needs cleaning up */
rm(arg)
char *arg;
{
    struct stat buf;

    if (lstat(arg, &buf)) {
	printf("%s not deleted.\n", arg);
	return;
    }
    if ((buf.st_mode & S_IFMT) == S_IFDIR) {
	if (access(arg, 02) < 0) {
	    printf("%s not deleted.\n", arg);
	    return;
	}
	rmrf(arg);
	return;
    }

    if (unlink(arg))
	printf("%s not deleted.\n", arg);
}

rmrf(f)
char   *f;
{
    int	    status,
	    pid,
	    i;

    if (dotname(f))
	return 0;
    while ((pid = vfork()) == -1)
	sleep(3);
    if (pid == 0) {
    	execl("/bin/rm", "rm", "-rf", f, (char *) 0);
    	execl("/usr/bin/rm", "rm", "-rf", f, (char *) 0);
    	printf("Can't find rm\n");
    	_exit(1);
    }
    while ((i = wait(&status)) != pid && i != -1)
	continue;
}

int
dotname(s)
char   *s;
{
    if (s[0] == '.')
	if (s[1] == '.')
	    if (s[2] == '\0')
		return 1;
	    else
		return 0;
	else if (s[1] == '\0')
	    return 1;
    return 0;
}

int	m1[] = {1, S_IREAD >> 0, 'r', '-'};
int	m2[] = {1, S_IWRITE >> 0, 'w', '-'};
int	m3[] = {2, S_ISUID, 's', S_IEXEC >> 0, 'x', '-'};
int	m4[] = {1, S_IREAD >> 3, 'r', '-'};
int	m5[] = {1, S_IWRITE >> 3, 'w', '-'};
int	m6[] = {2, S_ISGID, 's', S_IEXEC >> 3, 'x', '-'};
int	m7[] = {1, S_IREAD >> 6, 'r', '-'};
int	m8[] = {1, S_IWRITE >> 6, 'w', '-'};
int	m9[] = {2, S_ISVTX, 't', S_IEXEC >> 6, 'x', '-'};
int    *m[] =  {m1, m2, m3, m4, m5, m6, m7, m8, m9};
int	flags;			/* Gets flags */

pmode(aflag)
{
    register int  **mp;

    flags = aflag;
    for (mp = &m[0]; mp < &m[sizeof (m) / sizeof (m[0])];)
	mselect(*mp++);
	
}

mselect(pairp)
register int *pairp;
{
    register int n;

    n = *pairp++;
    while (--n >= 0 && (flags & *pairp++) == 0)
	pairp++;
    (void) putchar(*pairp);
    nprint++;
}

outch(c)
{
    (void) putchar(c);
}

putpad(str)
char   *str;
{
    if (str)
	tputs(str, 0, outch);
}

/* 
 * Print a filename intelligently.
 * lp		ptr to the lbuf entry
 * maxprt	max number of chars allowed to print
 * typeflg	display the file's type symbolically by appending *, /, @, >
 * symexpand	interpret symbolic links via the src -> target notation
 */
pname(lp, maxprt, typeflg, symexpand)
struct lbuf *lp;
{
    register char *p, *q;
    register int i;
    register int len;
    char buf[MAXPATHLEN];
    char buf2[MAXPATHLEN];
    int truncflg;
    int nprt;
    static struct target nulltarg = {0, ""};
    int expanded = 0;

    for (p = lp->namep, q = buf; *p; p++, q++)
    	if (isprint(*p))
	    *q = *p;
	else
	    *q = '?';
    *q = '\0';

    truncflg = (lp->namlen + typeflg) > maxprt;
    printf("%.*s%s", maxprt - truncflg - typeflg, buf, truncflg ? "-" : "");

#ifdef S_IFLNK
    if (!truncflg && symexpand && (lp->ltype == 'l' || lp->ltype == 'L')) {
	if (!(lp->flg & RDLINKDONE)) {
	    buf[0] = '\0';
	    (void) bldnam(buf, -1, lp);
	    if ((len = readlink(buf, buf2, sizeof buf2)) < 0) {
		(void) sprintf(buf2, "ERRNO%3d", errno);
		len = 8;
	    }
	    buf2[len] = '\0';
	    if ((lp->linkto = (struct target *)
	      malloc((unsigned) (sizeof linklen(lp) + len + 1))) == NULL) {
		telluser("Out of memory, not expanding symlink %s",lp->namep);
		lp->linkto = &nulltarg;
	    }
	    else {
		bcopy(buf2, linkname(lp), len+1);
		linklen(lp) = len;
	    }
	    lp->flg |= RDLINKDONE;
	}
	if (lp->linkto != &nulltarg) {		/* means was out of mem */
	    /* nprt will == how many printed so far */
	    nprt = min(lp->namlen, maxprt);
	    len = linklen(lp);
	    truncflg = (nprt + 4 + len + typeflg) > maxprt;
	    /* 4 == strlen(" -> ") */
	    if (nprt+4 < maxprt-truncflg-typeflg) {
		expanded++;
		printf(" -> ");
		for (i = nprt + 4, p = linkname(lp);
		     len-- && i < maxprt-truncflg-typeflg;
		     i++)
		    if (isprint(*p))
			(void) putchar(*p++);
		    else {
			(void) putchar('?');
			p++;
		    }
	    }
	    if (truncflg)
		(void) putchar('-');
	}
    }
#endif S_IFLNK
    if (typeflg) {
	i = symtype(lp);
	if (expanded && i == '>')   /* we can already tell cause of ->  */
	    i = '/';
	(void) putchar(i);
    }
}

    /* return a file`s symbolic "type" character */
int
symtype(lp)
register struct lbuf *lp;
{
    register int c;

    switch(lp->ltype) {
	case 'd':
	    c = '/';
	    break;
#ifdef S_IFLNK
	case 'l':
	    c = '@';
	    break;
	case 'L':
	    c = '>';			/* symlink to directory */
	    break;
#endif
#ifdef S_IFSOCK
	case 's':
	    c = '=';
	    break;
#endif
	default:
	    if (lp->lmode & S_IEXEC)	/* check owner only */
		c = '*';
	    else c = ' ';
	    break;
    }
    return c;
}

#ifndef UTAH
char *
skipto(string, charset)
register char *string;
char *charset;
{
	register char *setp;
	register int found = 0;		/* not found yet */

	while (*string && !found) {	/* until null or found */
	    /* find first char in charset matching *string */
	    for (setp = charset; *setp && (*setp != *string); setp++)
		continue;
	    if (*setp)
		found = 1;		/* matches a char */
	    else
		string++;		/* else keep looking */
	}
	return string;
}

char *
skipover(string, charset)
register char *string;
char *charset;
{
	register char *setp;
	register int found = 0;		/* not found yet */

	while (*string && !found) {	/* until null or found */
	    /* find first char in charset matching *string */
	    for (setp = charset; *setp && (*setp != *string); setp++)
		continue;
	    if (*setp)
		string++;		/* not found yet */
	    else
		found = 1;
	}
	return string;
}
#endif UTAH

#ifdef notyet
#ifndef xBSD42
/*
 * Convert size to number of "standard blocks".  Do NOT necessarily
 * convert to number of kilobytes (which is itself cleaner), but try to
 * match "ls"s behavior on a given version of Unix.
 */
long
sztob(size)
long	size;
{
    return (size + BUFSIZ-1) / BUFSIZ; /* guess as to blocksize from stdio.h*/
}

#else xBSD42
/*
 * Same as sztob, but uses st_blocks field in 4.2 for a
 * more accurate indication. We know 4.2 uses KB, so hardwire it.
 */
#define	kbytes(size)	(((size) + 1023) / 1024)

long
stbtok(nblks)
long nblks;
{
    return kbytes(dbtob(nblks));
}
#endif xBSD42
#endif notyet

#ifdef unneeded
char *
catargs(str, argv)
char *str;
char *argv[];
{
    for (++argv; *argv; argv++)
	if (**argv == '-') {
	    (void) strcat(str, *argv);
	    (void) strcat(str, " ");
	}
    return str;
}

overflow()
{
    printf("\n?Too many files\007\n");
    sleep(1);		/* So user can see it - don't worry if less */
    exit(1);
}
#endif unneeded
