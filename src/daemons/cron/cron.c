/* 
 * cron.c --
 *
 *	Cron daemon.  Reads /hosts/{hostname}/crontab.  Wakes
 *      up periodically to run the commands.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/daemons/cron/RCS/cron.c,v 1.5 90/02/16 12:54:50 douglis Exp Locker: rab $";
#endif /* not lint */


#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <pwd.h>
#include <fcntl.h>

#define	LISTS	(2*BUFSIZ)
#define	MAXLIN	BUFSIZ

#ifndef CRONTAB
#define CRONTAB "/sprite/lib/cron/crontab"
#endif

#ifndef CRONTABLOC
#define CRONTABLOC  "/usr/lib/crontab.local"
#endif

#define	EXACT	100
#define	ANY	101
#define	LIST	102
#define	RANGE	103
#define	EOS	104

static char	crontab[]	= CRONTAB;
static char	loc_crontab[256]   = CRONTABLOC;
static time_t	itime;
static struct	tm *loct;
extern struct	tm *localtime();
static int	flag;
static char	*list;
static char	*listend;
static unsigned listsize;

static FILE	*debug;
#define dprintf if (debug) fprintf

static char *cmp();
static void slp();
static void ex();
static void init();
static void append();
static void reapchild();
static void untty();
static int number();
static void Fs_GetPrivateName();

void
main(argc, argv)
	int argc;
	char **argv;
{
    register char *cp;
    char *cmp();
    time_t filetime = 0;
    time_t lfiletime = 0;
    char c;
    extern char *optarg;
    extern int errno;

     /*
      * Check command line arguments
      */
     c = getopt(argc, argv, "d:");
     if (c == 'd') {
	 if ((debug = fopen(optarg, "w")) == NULL) {
	     fprintf(stderr, "Can't open %s: %s\n", optarg, strerror(errno));
	     exit(1);
	 }   
	 fcntl(fileno(debug), F_SETFL, FAPPEND);
     }
    /*
     * Detach from parent.
     */
     if (!debug && fork()) {
	 exit(0);
     }

     chdir("/");
     freopen("/", "r", stdout);
     freopen("/", "r", stderr);
     untty();

     /*
      * Ignore signals
      */
     signal(SIGHUP, SIG_IGN);
     signal(SIGINT, SIG_IGN);
#ifndef sprite
     signal(SIGQUIT, SIG_IGN);
#endif
     signal(SIGCHLD, reapchild);
     time(&itime);
     itime -= localtime(&itime)->tm_sec;

     Fs_GetPrivateName("crontab", loc_crontab);

     for (;; itime+=60, slp()) {
	 struct stat cstat, lcstat;
	 int newcron, newloc;

	 newcron = 0;
	 if (stat(crontab, &cstat) < 0) {
	     cstat.st_mtime = 1;
	 }   
	 if (cstat.st_mtime != filetime) {
	     filetime = cstat.st_mtime;
	     newcron++;
	 }
	 newloc  = 0;
	 if (stat(loc_crontab, &lcstat) < 0) {
	     lcstat.st_mtime = 1;
	 }
	 if (lcstat.st_mtime != lfiletime) {
	     lfiletime = lcstat.st_mtime;
	     newloc++;
	 }
	 if (newcron || newloc) {
	     dprintf(debug, "%x: reading crontab files\n", getpid()),
	         fflush(debug);
	     init();
	     append(crontab);
	     append(loc_crontab);
	     *listend++ = EOS;
	     *listend++ = EOS;
	 }
	 loct = localtime(&itime);
	 loct->tm_mon++;		 /* 1-12 for month */
	 if (loct->tm_wday == 0) {
	     loct->tm_wday = 7;	/* sunday is 7, not 0 */
	 }
	 for(cp = list; *cp != EOS;) {
	     flag = 0;
	     cp = cmp(cp, loct->tm_min);
	     cp = cmp(cp, loct->tm_hour);
	     cp = cmp(cp, loct->tm_mday);
	     cp = cmp(cp, loct->tm_mon);
	     cp = cmp(cp, loct->tm_wday);
	     if(flag == 0) {
		 ex(cp);
	     }   
	     while(*cp++ != 0) {
		 ;
	     }
	 }
     }   
}

static char *
cmp(p, v)
    char *p;
{
    register char *cp;

    cp = p;
    switch(*cp++) {

    case EXACT:
	if (*cp++ != v) {
	    flag++;
	}
	return cp;

    case ANY:
	return cp;

    case LIST:
	while(*cp != LIST) {
	    if(*cp++ == v) {
		while(*cp++ != LIST) {
		    ;
		}
		return cp;
	    }
	}
	flag++;
	return cp+1;

    case RANGE:
	if(*cp > v || cp[1] < v) {
	    flag++;
	}
	return cp+2;
    }
    if(cp[-1] != v) {
	flag++;
    }
    return cp;
}

static void
slp()
{
    register i;
    time_t t;

    time(&t);
    i = itime - t;
    if(i < -60 * 60 || i > 60 * 60) {
	itime = t;
	i = 60 - localtime(&itime)->tm_sec;
	itime += i;
    }
    if(i > 0) {
	sleep(i);
    }
    return;
}

static void
ex(s)
    char *s;
{
    int st;
    register struct passwd *pwd;
    char user[BUFSIZ];
    char *c = user;
    int pid;

    if (fork()) {
	return;
    }

    pid = getpid();
    while(*s != ' ' && *s != '\t') {
	*c++ = *s++;
    }
    *c = '\0';
    s++;
    if ((pwd = getpwnam(user)) == NULL) {
	dprintf(debug, "%x: cannot find %s\n", pid, user),
	    fflush(debug);
	    exit(1);
    }   
    (void) setgid(pwd->pw_gid);
    initgroups(pwd->pw_name, pwd->pw_gid);
    (void) setuid(pwd->pw_uid);
    freopen("/", "r", stdin);
    dprintf(debug, "%x: executing %s", pid, s), fflush (debug);
    execl("/bin/sh", "sh", "-c", s, 0);
    dprintf(debug, "%x: cannot execute sh\n", pid), fflush (debug);
    exit(0);
}

static void
init()
{
    /*
     * Don't free in case was longer than LISTS.  Trades off
     * the rare case of crontab shrinking vs. the common case of
     * extra realloc's needed in append() for a large crontab.
     */
    if (list == 0) {
	list = malloc(LISTS);
	listsize = LISTS;
    }
    listend = list;
    return;
}

static void
append(fn)
    char *fn;
{
    register i, c;
    register char *cp;
    register char *ocp;
    register int n;

    if (freopen(fn, "r", stdin) == NULL) {
	return;
    }
    cp = listend;
loop:
    if(cp > list+listsize-MAXLIN) {
	int length = cp - list;

	listsize += LISTS;
	list = realloc(list, listsize);
	cp = list + length;
    }
    ocp = cp;
    for(i=0;; i++) {
	do {
	    c = getchar();
	} while(c == ' ' || c == '\t');
	if(c == EOF || c == '\n') {
	    goto ignore;
	}
	if(i == 5) {
	    break;
	}
	if(c == '*') {
	    *cp++ = ANY;
	    continue;
	}
	if ((n = number(c)) < 0) {
	    goto ignore;
	}
	c = getchar();
	if(c == ',') {
	    goto mlist;
	}
	if(c == '-') {
	    goto mrange;
	}
	if(c != '\t' && c != ' ') {
	    goto ignore;
	}
	*cp++ = EXACT;
	*cp++ = n;
	continue;

    mlist:
	*cp++ = LIST;
	*cp++ = n;
	do {
	    if ((n = number(getchar())) < 0) {
		goto ignore;
	    }
	    *cp++ = n;
	    c = getchar();
	} while (c==',');
	if(c != '\t' && c != ' ') {
	    goto ignore;
	}
	*cp++ = LIST;
	continue;

    mrange:
	*cp++ = RANGE;
	*cp++ = n;
	if ((n = number(getchar())) < 0) {
			goto ignore;
	}
	c = getchar();
	if(c != '\t' && c != ' ') {
	    goto ignore;
	}
	*cp++ = n;
    }
    while(c != '\n') {
	if(c == EOF) {
	    goto ignore;
	}
	if(c == '%') {
	    c = '\n';
	}
	*cp++ = c;
	c = getchar();
    }
    *cp++ = '\n';
    *cp++ = 0;
    goto loop;

ignore:
    cp = ocp;
    while(c != '\n') {
	if(c == EOF) {
	    fclose(stdin);
	    listend = cp;
	    return;
	}
	c = getchar();
    }
    goto loop;
}

static int
number(c)
    register c;
{
    register n = 0;

    while (isdigit(c)) {
	n = n*10 + c - '0';
	c = getchar();
    }
    ungetc(c, stdin);
    if (n>=100) {
	return -1;
    }
    return n;
}

static void
reapchild()
{
    union wait status;
    int pid;

    while ((pid = wait3(&status, WNOHANG, 0)) > 0) {
	dprintf(debug, "%x: child exits with signal %d status %d\n",
	    pid, status.w_termsig, status.w_retcode),
	    fflush (debug);
    }
    return;
}

static void
untty()
{
    int i;

    i = open("/dev/tty", O_RDWR);
    if (i >= 0) {
	ioctl(i, TIOCNOTTY, (char *)0);
	(void) close(i);
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Fs_GetPrivateName --
 *
 *      Fill a buffer with the path name of a file private to the local
 *      machine. The file is located in /hosts/<hostname>/<file>
 *
 * Results:
 *      The path of the file.
 *
 * Side Effects:
 *      The passed buffer is overwritten.
 *
 *----------------------------------------------------------------------
 */
static void
Fs_GetPrivateName (fileName, bufPtr)
    char                *fileName;      /* Short name of private file */
    char                *bufPtr;        /* Pointer to buffer for storage */
{
    static int      	initialized = 0;
    static char         hostname[64];   /* Name of local host */

#define PRIVATE_DIR     "/hosts"

    if (!initialized) {
        gethostname (hostname, sizeof(hostname));
	hostname[sizeof(hostname)-1] = 0;
        initialized = 1;
    }

    sprintf (bufPtr, "%s/%s/%s", PRIVATE_DIR, hostname, fileName);
    return;
}
