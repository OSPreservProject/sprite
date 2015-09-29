/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)lpd.c	5.5 (Berkeley) 5/5/88";
#endif /* not lint */

/*
 * lpd -- line printer daemon.
 *
 * Listen for a connection and perform the requested operation.
 * Operations are:
 *	\1printer\n
 *		check the queue for jobs and print any found.
 *	\2printer\n
 *		receive a job from another machine and queue it.
 *	\3printer [users ...] [jobs ...]\n
 *		return the current state of the queue (short form).
 *	\4printer [users ...] [jobs ...]\n
 *		return the current state of the queue (long form).
 *	\5printer person [users ...] [jobs ...]\n
 *		remove jobs from the queue.
 *
 * Strategy to maintain protected spooling area:
 *	1. Spooling area is writable only by daemon and spooling group
 *	2. lpr runs setuid root and setgrp spooling group; it uses
 *	   root to access any file it wants (verifying things before
 *	   with an access call) and group id to know how it should
 *	   set up ownership of files in the spooling area.
 *	3. Files in spooling area are owned by root, group spooling
 *	   group, with mode 660.
 *	4. lpd, lpq and lprm run setuid daemon and setgrp spooling group to
 *	   access files and printer.  Users can't get to anything
 *	   w/o help of lpq and lprm programs.
 */

#include "lp.h"
#ifdef sprite
#include <fs.h>
#include <sprite.h>
#include <pdev.h>
#include <status.h>
#endif

int	lflag;				/* log requests flag */

int	reapchild();
int	mcleanup();

#ifdef sprite
static ReturnStatus P_Open();
static ReturnStatus P_Close();
static ReturnStatus P_Write();
static int client;
static void RemoteWrite();
Pdev_CallBacks pdevHandlers = {
    P_Open,
    NULL,
    P_Write,
    NULL,
    P_Close,
    NULL,
    NULL
};
#define FINET_ID    47
int debug;
#endif


main(argc, argv)
	int argc;
	char **argv;
{
#ifdef sprite
	int f, finet, lfd;
	Pdev_Token pdev;
#else
	int f, funix, finet, options, defreadfds, fromlen;
	static struct sockaddr_un sun, fromunix;
	int omask, lfd;
#endif
	static struct sockaddr_in sin, frominet;
	int err;

	gethostname(host, sizeof(host));
#ifdef sprite
	sprintf(pdevName, "/hosts/%s/dev/printer", host);
#endif
	name = argv[0];
	while (--argc > 0) {
		argv++;
		if (argv[0][0] == '-')
			switch (argv[0][1]) {
			case 'd':
#ifdef sprite
			        debug = 1;
#else
				options |= SO_DEBUG;
#endif
				break;
			case 'l':
				lflag++;
				break;
			}
	}

#ifndef DEBUG
	/*
	 * Set up standard environment by detaching from the parent.
	 * Regular file descriptors are closed, stderr is hooked
	 * to /dev/syslog.
	 */
	if (fork())
		exit(0);
	for (f = 0; f < 5; f++)
		(void) close(f);
	(void) open("/dev/null", O_RDONLY);
	(void) open("/dev/null", O_WRONLY);
	err = open("/dev/syslog", O_WRONLY);
	if (err >= 0 && err != 2) {
	    dup2(err, 2);
	    close(err);
	}
	f = open("/dev/tty", O_RDWR);
	if (f > 0) {
		ioctl(f, TIOCNOTTY, 0);
		(void) close(f);
	}
#endif

	openlog("lpd", LOG_PID, LOG_LPR);
	if (err < 0) {
	    syslog(LOG_ERR, "%s: %m", "/dev/syslog open failed");
	}
	(void) umask(0);
	sprintf(masterLock, "/sprite/spool/lpd/%s.lock", host);
	lfd = open(masterLock, O_WRONLY|O_CREAT, 0644);
	if (lfd < 0) {
		syslog(LOG_ERR, "%s: %m", masterLock);
		exit(1);
	}
	if (flock(lfd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)	/* active deamon present */
			exit(0);
		syslog(LOG_ERR, "%s: %m", masterLock);
		exit(1);
	}
	ftruncate(lfd, 0);
	/*
	 * write process id for others to know
	 */
	sprintf(line, "%u\n", getpid());
	f = strlen(line);
	if (write(lfd, line, f) != f) {
		syslog(LOG_ERR, "%s: %m", masterLock);
		exit(1);
	}
	signal(SIGCHLD, reapchild);
	/*
	 * Restart all the printers.
	 */
	startup();
#ifdef sprite
	if ((pdev = Pdev_Open(pdevName, NULL, 0, 0, &pdevHandlers, 0)) == NULL) {
	    syslog(LOG_ERR, "Pdev_Open: %s", pdev_ErrorMsg);
	    exit(1);
	}
	signal(SIGHUP, mcleanup);
	signal(SIGINT, mcleanup);
	signal(SIGQUIT, mcleanup);
	signal(SIGTERM, mcleanup);
#else
	(void) unlink(SOCKETNAME);
	funix = socket(AF_UNIX, SOCK_STREAM, 0);
	if (funix < 0) {
		syslog(LOG_ERR, "socket: %m");
		exit(1);
	}
#define	mask(s)	(1 << ((s) - 1))
	omask = sigblock(mask(SIGHUP)|mask(SIGINT)|mask(SIGQUIT)|mask(SIGTERM));
	signal(SIGHUP, mcleanup);
	signal(SIGINT, mcleanup);
	signal(SIGQUIT, mcleanup);
	signal(SIGTERM, mcleanup);
	sun.sun_family = AF_UNIX;
	strcpy(sun.sun_path, SOCKETNAME);
	if (bind(funix, &sun, strlen(sun.sun_path) + 2) < 0) {
		syslog(LOG_ERR, "ubind: %m");
		exit(1);
	}
	sigsetmask(omask);
	defreadfds = 1 << funix;
	listen(funix, 5);
#endif
	finet = socket(AF_INET, SOCK_STREAM, 0);
	if (finet >= 0) {
		struct servent *sp;
#ifndef sprite
		if (options & SO_DEBUG)
			if (setsockopt(finet, SOL_SOCKET, SO_DEBUG, 0, 0) < 0) {
				syslog(LOG_ERR, "setsockopt (SO_DEBUG): %m");
				mcleanup();
			}
#endif
		sp = getservbyname("printer", "tcp");
		if (sp == NULL) {
			syslog(LOG_ERR, "printer/tcp: unknown service");
			mcleanup();
		}
		sin.sin_family = AF_INET;
		sin.sin_port = sp->s_port;
		if (bind(finet, &sin, sizeof(sin), 0) < 0) {
			syslog(LOG_ERR, "bind: %m");
			mcleanup();
		}
#ifdef sprite
		Fs_EventHandlerCreate(finet, FS_READABLE, RemoteWrite,FINET_ID);
#else
		defreadfds |= 1 << finet;
#endif
		listen(finet, 5);
	}

#ifdef sprite
	for (;;)
	    Fs_Dispatch();
#else

	/*
	 * Main loop: accept, do a request, continue.
	 */
	for (;;) {
		int domain, nfds, s, readfds = defreadfds;

		nfds = select(20, &readfds, 0, 0, 0);
		if (nfds <= 0) {
			if (nfds < 0 && errno != EINTR)
				syslog(LOG_WARNING, "select: %m");
			continue;
		}
		if (readfds & (1 << funix)) {
			domain = AF_UNIX, fromlen = sizeof(fromunix);
			s = accept(funix, &fromunix, &fromlen);
		} else if (readfds & (1 << finet)) {
			domain = AF_INET, fromlen = sizeof(frominet);
			s = accept(finet, &frominet, &fromlen);
		}
		if (s < 0) {
			if (errno != EINTR)
				syslog(LOG_WARNING, "accept: %m");
			continue;
		}
		if (fork() == 0) {
			signal(SIGCHLD, SIG_IGN);
			signal(SIGHUP, SIG_IGN);
			signal(SIGINT, SIG_IGN);
			signal(SIGQUIT, SIG_IGN);
			signal(SIGTERM, SIG_IGN);
			(void) close(funix);
			(void) close(finet);
			dup2(s, 1);
			(void) close(s);
			if (domain == AF_INET)
				chkhost(&frominet);
			doit();
			exit(0);
		}
		(void) close(s);
	}
#endif
}

reapchild()
{
	union wait status;

	while (wait3(&status, WNOHANG, 0) > 0)
		;
}

mcleanup()
{
	if (lflag)
		syslog(LOG_INFO, "exiting");
#ifdef sprite
	unlink(pdevName);
#else
	unlink(SOCKETNAME);
#endif
	exit(0);
}

/*
 * Stuff for handling job specifications
 */
char	*user[MAXUSERS];	/* users to process */
int	users;			/* # of users in user array */
int	requ[MAXREQUESTS];	/* job number of spool entries */
int	requests;		/* # of spool requests */
char	*person;		/* name of person doing lprm */

char	fromb[32];	/* buffer for client's machine name */
char	cbuf[BUFSIZ];	/* command line buffer */
char	*cmdnames[] = {
	"null",
	"printjob",
	"recvjob",
	"displayq short",
	"displayq long",
	"rmjob"
};

#ifdef sprite

static void
RemoteWrite(clientData, streamID, eventMask)
    ClientData clientData;
    int streamID;
    int eventMask;
{
    int r;
    struct sockaddr_in frominet;
    int fromlen;
    int s;

    if ((int) clientData != FINET_ID) {
	syslog(LOG_ERR, "RemoteWrite: bad clientData (%d)", (int) clientData);
	return;
    }
    fromlen = sizeof(frominet);
    if ((s = accept(streamID, &frominet, &fromlen)) < 0) {
	syslog(LOG_ERR, "accept: %m");
	return;
    }
    if ((r = read(s, cbuf, sizeof(cbuf))) < 0) {
	syslog(LOG_ERR, "finet read: %m");
	return;
    }
    cbuf[r] = '\0';
    if (cbuf[r - 1] == '\n')
	cbuf[r - 1] = '\0';
    if (debug)
	syslog(LOG_ERR, "RemoteWrite: (%s)", cbuf);
    if (fork() == 0) {
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	close(streamID);
	dup2(s, 1);
	close(s);
	chkhost(&frominet);
	doit();
	/*NOTREACHED*/
    }
    close(s);
    return;
}

static ReturnStatus
P_Open(token, streamPtr, readBuffer, flags, pid, hostID, uid, selectBitsPtr)
    ClientData token;		/* Was passed to Pdev_Open call */
    Pdev_Stream *streamPtr;
    char *readBuffer;
    int flags;
    int pid;
    int hostID;
    int uid;
    int *selectBitsPtr;
{
    if (debug) {
        syslog(LOG_ERR,
        "<%d> Proc %x at host %x, uid %x opened with flags %x\n",
	    client, pid, hostID, uid, flags);
    }
    streamPtr->clientData = client;
    client++;
    *selectBitsPtr = FS_WRITABLE;
    return SUCCESS;
}

static ReturnStatus
P_Close(streamPtr)
    Pdev_Stream *streamPtr;
{
    if (debug) {
        syslog(LOG_ERR, "<%d> client closed\n",
        (int) streamPtr->clientData);
    }
    return SUCCESS;
}

static ReturnStatus
P_Write(streamPtr, async, writePtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;
    int async;
    Pdev_RWParam *writePtr;
    int *selectBitsPtr;
    Pdev_Signal *sigPtr;
{
    register int n;
    register char *buffer = writePtr->buffer;

    if (debug)
        syslog(LOG_ERR, "Pdev_Write");
    n = writePtr->length;
    buffer[n] = '\0';
    if (buffer[n - 1] == '\n')
        buffer[n - 1] = '\0';
    switch (buffer[0]) {

    case 'd':
        debug = 1;
        break;

    case 'n':
        debug = 0;
        break;

    default:
        if (fork() == 0) {
	    signal(SIGCHLD, SIG_IGN);
	    signal(SIGHUP, SIG_IGN);
	    signal(SIGINT, SIG_IGN);
	    signal(SIGQUIT, SIG_IGN);
	    signal(SIGTERM, SIG_IGN);
	    strcpy(cbuf, buffer);
	    doit();
	    /*NOTREACHED*/
	}
    }
    return SUCCESS;
}
#endif /* sprite */

doit()
{
	register char *cp;
	register int n;

#ifdef sprite
	Proc_Detach(0);
#else
	for (;;) {
		cp = cbuf;
		do {
			if (cp >= &cbuf[sizeof(cbuf) - 1])
				fatal("Command line too long");
			if ((n = read(1, cp, 1)) != 1) {
				if (n < 0)
					fatal("Lost connection");
				return;
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
#endif
		cp = cbuf;
		if (lflag) {
			if (*cp >= '\1' && *cp <= '\5')
				syslog(LOG_INFO, "%s requests %s %s",
					from, cmdnames[*cp], cp+1);
			else
				syslog(LOG_INFO, "bad request (%d) from %s",
					*cp, from);
		}
		switch (*cp++) {
		case '\1':	/* check the queue and print any jobs there */
			printer = cp;
			printjob();
			break;
		case '\2':	/* receive files to be queued */
			printer = cp;
			recvjob();
			break;
		case '\3':	/* display the queue (short form) */
		case '\4':	/* display the queue (long form) */
			printer = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			displayq(cbuf[0] - '\3');
			exit(0);
		case '\5':	/* remove a job from the queue */
			printer = cp;
			while (*cp && *cp != ' ')
				cp++;
			if (!*cp)
				break;
			*cp++ = '\0';
			person = cp;
			while (*cp) {
				if (*cp != ' ') {
					cp++;
					continue;
				}
				*cp++ = '\0';
				while (isspace(*cp))
					cp++;
				if (*cp == '\0')
					break;
				if (isdigit(*cp)) {
					if (requests >= MAXREQUESTS)
						fatal("Too many requests");
					requ[requests++] = atoi(cp);
				} else {
					if (users >= MAXUSERS)
						fatal("Too many users");
					user[users++] = cp;
				}
			}
			rmjob();
			break;
		}
		fatal("Illegal service request");
#ifndef sprite
	}
#endif
}

/*
 * Make a pass through the printcap database and start printing any
 * files left from the last time the machine went down.
 */
startup()
{
	char buf[BUFSIZ];
	register char *cp;
	int pid;

	printer = buf;

	/*
	 * Restart the daemons.
	 */
	while (getprent(buf) > 0) {
		for (cp = buf; *cp; cp++)
			if (*cp == '|' || *cp == ':') {
				*cp = '\0';
				break;
			}
		if ((pid = fork()) < 0) {
			syslog(LOG_WARNING, "startup: cannot fork");
			mcleanup();
		}
		if (!pid) {
			endprent();
#ifdef sprite
			Proc_Detach(0);
#endif
			printjob();
		}
	}
}

#define DUMMY ":nobody::"

/*
 * Check to see if the from host has access to the line printer.
 */
chkhost(f)
	struct sockaddr_in *f;
{
	register struct hostent *hp;
	register FILE *hostf;
	register char *cp, *sp;
	char ahost[50];
	int first = 1;
	extern char *inet_ntoa();
	int baselen = -1;

	f->sin_port = ntohs(f->sin_port);
	if (f->sin_family != AF_INET || f->sin_port >= IPPORT_RESERVED)
		fatal("Malformed from address");
	hp = gethostbyaddr(&f->sin_addr, sizeof(struct in_addr), f->sin_family);
	if (hp == 0)
		fatal("Host name for your address (%s) unknown",
			inet_ntoa(f->sin_addr));

	strcpy(fromb, hp->h_name);
	from = fromb;
	if (!strcmp(from, host))
		return;

	sp = fromb;
	cp = ahost;
	while (*sp) {
		if (*sp == '.') {
			if (baselen == -1)
				baselen = sp - fromb;
			*cp++ = *sp++;
		} else {
			*cp++ = isupper(*sp) ? tolower(*sp++) : *sp++;
		}
	}
	*cp = '\0';
	hostf = fopen("/etc/hosts.equiv", "r");
again:
	if (hostf) {
		if (!_validuser(hostf, ahost, DUMMY, DUMMY, baselen)) {
			(void) fclose(hostf);
			return;
		}
		(void) fclose(hostf);
	}
	if (first == 1) {
		first = 0;
		hostf = fopen("/etc/hosts.lpd", "r");
		goto again;
	}
	fatal("Your host does not have line printer access");
}
