/*
 * syslog.c --
 *
 * 	Sprite version of 4.3BSD's syslog facilty.
 *
 */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)syslog.c	5.10 (Berkeley) 4/20/87";
#endif LIBC_SCCS and not lint

/*
 * SYSLOG -- print message on log file
 *
 * This routine looks a lot like printf, except that it
 * outputs to the log file instead of the standard output.
 * Also:
 *	adds a timestamp,
 *	prints the module name in front of the message,
 *	has some other formatting types (or will sometime),
 *	adds a newline on the end of the message.
 *
 * The output of this routine is intended to be read by /etc/syslogd.
 *
 * Author: Eric Allman
 * (Modified to use UNIX domain IPC by Ralph Campbell)
 *	Modified to use the Sprite /dev/syslog, fall 1987
 */

#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/syslog.h>
#include <netdb.h>
#include <strings.h>
#include <stdio.h>
#include <sys/wait.h>

#if defined(__STDC__) && !defined(spur) && !defined(sun4)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#define	MAXLINE	1024			/* max message size */

#define PRIFAC(p)	(((p) & LOG_FACMASK) >> 3)
#define IMPORTANT 	LOG_ERR

static char	logname[] = "/dev/syslog";
static char	ctty[] = "/dev/console";

static int	LogFile = -1;		/* fd for log */
static int	LogStat	= 0;		/* status bits, set by openlog() */
static char	*LogTag = "syslog";	/* string to tag the entry with */
static int	LogMask = 0xff;		/* mask of priorities to be logged */
static int	LogFacility = LOG_USER;	/* default facility code */


extern	int errno, sys_nerr;
extern	char *sys_errlist[];
extern	long time();
extern	char *ctime();

extern int setlogmask();
extern void closelog();
extern void openlog();

#ifndef lint
#if defined(__STDC__) && !defined(spur) && !defined(sun4)
void
syslog(int pri, const char *fmt, ...)
{
#else
void
syslog(va_alist)
        va_dcl
{
        int pri;
	char *fmt;
#endif
	char buf[MAXLINE + 1], outline[MAXLINE + 1];
	register char *b, *o;
#ifdef __STDC__
	register const char *f;
#else
	register char *f;
#endif
	register int c;
	long now;
	int pid, olderrno = errno;
	va_list args;

#if defined(__STDC__) && !defined(spur) && !defined(sun4)
	va_start(args, fmt);
#else
	va_start(args);
	pri = va_arg(args, int);
	fmt = va_arg(args, char *);
#endif
	/* see if we should just throw out this message */
	if (pri <= 0 || PRIFAC(pri) >= LOG_NFACILITIES ||
	    (LOG_MASK(pri) & LogMask) == 0)
		return;
	if (LogFile < 0)
		openlog(LogTag, LogStat | LOG_NDELAY, 0);

	/* set default facility if none specified */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* build the message */
	o = outline;
	sprintf(o, "<%d>", pri);
	o += strlen(o);
	time(&now);
	sprintf(o, "%.15s ", ctime(&now) + 4);
	o += strlen(o);
	if (LogTag) {
		strcpy(o, LogTag);
		o += strlen(o);
	}
	if (LogStat & LOG_PID) {
		sprintf(o, "[%x]", getpid());
		o += strlen(o);
	}
	if (LogTag) {
		strcpy(o, ": ");
		o += 2;
	}

	b = buf;
	f = fmt;
	while ((c = *f++) != '\0' && c != '\n' && b < &buf[MAXLINE]) {
		if (c != '%') {
			*b++ = c;
			continue;
		}
		if ((c = *f++) != 'm') {
			*b++ = '%';
			*b++ = c;
			continue;
		}
		if ((unsigned)olderrno > sys_nerr)
			sprintf(b, "error %d", olderrno);
		else
			strcpy(b, sys_errlist[olderrno]);
		b += strlen(b);
	}
	*b++ = '\n';
	*b = '\0';
	vsprintf(o, buf, args);
	va_end(args);
	c = strlen(outline);
	if (c > MAXLINE)
		c = MAXLINE;

	/* output the message to the local logger */
	if (write(LogFile, outline, c) >= 0)
		return;
	if (!(LogStat & LOG_CONS))
		return;

	/* output the message to the console */
	pid = vfork();
	if (pid == -1)
		return;
	if (pid == 0) {
		int fd;

		sigsetmask(sigblock(0));
		fd = open(ctty, O_WRONLY);
		strcat(o, "\r");
		o = index(outline, '>') + 1;
		write(fd, o, c + 1 - (o - outline));
		close(fd);
		_exit(0);
	}
	if (!(LogStat & LOG_NOWAIT))
		while ((c = wait((union wait *)0)) > 0 && c != pid)
			;
}
#else /* lint */
/*VARARGS2*/
/*ARGSUSED*/
void
syslog(pri, fmt)
    int pri;
    char *fmt;
{
    return;
}
#endif /* !lint */

/*
 * OPENLOG -- open system log
 */

void
openlog(ident, logstat, logfac)
	char *ident;
	int logstat, logfac;
{
	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	if (logfac != 0)
		LogFacility = logfac & LOG_FACMASK;
	if (LogFile >= 0)
		return;

	if (LogStat & LOG_NDELAY) {
		LogFile = open(logname, O_WRONLY, 0);
		fcntl(LogFile, F_SETFD, 1);
	}
	return;
}

/*
 * CLOSELOG -- close the system log
 */

void
closelog()
{

	(void) close(LogFile);
	LogFile = -1;
	return;
}

/*
 * SETLOGMASK -- set the log mask level
 */
int
setlogmask(pmask)
	int pmask;
{
	int omask;

	omask = LogMask;
	if (pmask != 0)
		LogMask = pmask;
	return (omask);
}
