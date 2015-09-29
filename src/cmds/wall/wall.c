/* 
 * wall.c --
 *
 * Write to all.  Sends a message to the /dev/syslog window of
 * each machine, or the local host if specified.
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
static char rcsid[] = "$Header: /sprite/src/cmds/wall/RCS/wall.c,v 1.13 92/04/22 14:21:28 kupfer Exp $";
#endif /* not lint */

#define NDEBUG

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mig.h>
#include <host.h>
#include <sys/time.h>
#include <pwd.h>
#include <option.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <rloginPseudoDev.h>
#include <setjmp.h>
#include <ulog.h>


#define MAX_NUM_HOSTS   0x100
#define MAX_MSG_LEN     0x400
#define TIME_OUT        180
#define HOST_NAME_SIZE  64

#ifdef __STDC__
static void SendMsg(char *msg, int msize, char *host, int doRlogin);
static void FillAlert(void);
static void Down(int n);
static void Prompt(void);
#else
static void SendMsg();
static void FillAlert();
static void Down();
static void Prompt();
#endif

static Mig_Info infoArray[MAX_NUM_HOSTS];
static char buf[MAX_MSG_LEN];
static int verbose = 0;
static int debug = 0;
static int local = 0;
static int wallRlogins = 1;
static int consoles = 1;

static char alert[0x100];

extern int errno;

Option optionArray[] = {
    {OPT_TRUE, "l", (Address)&local,
     "Run on local host only."},
    {OPT_TRUE, "v", (Address)&verbose,
     "Print as hosts are accessed."},
    {OPT_TRUE, "d", (Address)&debug,
     "Enable debugging."},
    {OPT_FALSE, "r", (Address)&wallRlogins,
     "Don't write to rlogins (useful if the swap server is down)."},
    {OPT_FALSE, "C", (Address)&consoles,
     "Don't write to consoles (useful just for debugging."},
};

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Prompts the invoker for a message, and then prints the
 *      message to the /dev/syslog of all sprite machines.
 *
 * Results:
 *	Zero exit code if there are no errors.
 *
 * Side effects:
 *	Print a message to everybody's /dev/syslog.
 *
 *----------------------------------------------------------------------
 */

void
main(argc, argv)
    int argc;
    char **argv;
{
    register int n;
    register Mig_Info *infoPtr;
    register Host_Entry *hostPtr;
    register char *b;
    register int r;
    register int size;
    register int numRecs;
    int doPrompt;
    int loopIndex;
    int myID;

    (void) Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray),
		     OPT_ALLOW_CLUSTERING);

    FillAlert();
    if ((numRecs = Mig_GetAllInfo(infoArray, MAX_NUM_HOSTS)) <= 0) {
	perror("Error in Mig_GetAllInfo");
	exit(1);
    }
    if ((doPrompt = isatty(0)) != 0) {
	Prompt();
    }
    for (b = buf, size = sizeof(buf); (r = read(0, b, size)) > 0; b += r) {
	if ((size -= r) == 0) {
	    (void) fprintf(stderr, "WARNING: max message length exceeded\n");
	    break;
	}
	/*
	 * stop if a `.' appears at the beginning of a line.
	 * This assumes that stdin is line buffered.
	 */
	if (*b == '.' && b[-1] == '\n') {
	    break;
	}
	if (doPrompt) {
	    Prompt();
	}
    }
    size = sizeof(buf) - size;
    if (local) {
	(void) Proc_GetHostIDs(&myID, (int *) NULL);
    }
    for (loopIndex = !consoles; loopIndex <= wallRlogins; loopIndex++) {
	for (n = 0, infoPtr = &infoArray[0]; n < numRecs; ++infoPtr, ++n) {
	    if (infoPtr->loadVec.timestamp == 0) {
		continue;
	    }
	    if (local && infoPtr->hostID != myID) {
		continue;
	    }
	    if (infoPtr->state == MIG_HOST_DOWN) {
		if (verbose) {
		    Down(infoPtr->hostID);
		}
		continue;
	    }
	    if (infoPtr->loadVec.timestamp - infoPtr->bootTime < 0) {
		if (verbose) {
		    Down(infoPtr->hostID);
		}
		continue;
	    }
	    if ((hostPtr = Host_ByID(infoPtr->hostID)) == NULL) {
		(void) fprintf(stderr, "Error in Host_ByID(%d)\n", infoPtr->hostID);
		continue;
	    }
	    SendMsg(buf, size, hostPtr->name, loopIndex);
	}
    }
    exit(0);
}   /* main */


/*
 *----------------------------------------------------------------------
 *
 * Down --
 *
 *      Print a list of all machines that are down.
 *
 * Results:
 *      None.
 * 
 * Side effects:
 *      Prints a list to stdout.
 *  
 *----------------------------------------------------------------------
 */ 

static void
Down(n)
    int n;
{
    Host_Entry *hostPtr;

    if ((hostPtr = Host_ByID(n)) == NULL) {
	(void) fprintf(stderr, "Error in Host_ByID(%d)\n", n);
	return;
    }
    (void) printf("%s is down\n", hostPtr->name);
}


/*
 * Set the default timeout, in seconds.
 */
#ifndef TIMEOUT
#define TIMEOUT 10
#endif /* TIMEOUT */
char currentFile[0x400];
int exited;
static jmp_buf	OpenTimeout;


/*
 *----------------------------------------------------------------------
 *
 * AlarmHandler --
 *
 *	Routine to service a SIGALRM signal.  This routine disables
 *	the alarm (letting the caller reenable it when appropriate).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The alarm is disabled, and a warning message is printed. A
 * 	global variable is set to indicate that the parent should give up.
 *
 *----------------------------------------------------------------------
 */
static int
AlarmHandler()
{
    exited = 1;
    alarm(0);
    fprintf(stderr, "Warning: couldn't write to %s; removing file.\n",
	    currentFile);
    fflush(stderr);
    if (unlink(currentFile) < 0) {
	perror("unlink");
    }
    (void) signal (SIGALRM, SIG_IGN);
    longjmp(OpenTimeout, 1);
}

/*
 *----------------------------------------------------------------------
 *
 * SendMsg --
 *
 *      Write the message to a specific host.
 *
 * Results:
 *      None.
 * 
 * Side effects:
 *      Writes the message to the /dev/syslog of the specified host.
 *  
 *----------------------------------------------------------------------
 */ 

static void
SendMsg(msg, msize, host, doRlogin)
    char *msg;
    int msize;
    char *host;
    int doRlogin;
{
    int fd;
    int i;
    struct stat statBuf;
    char *s = currentFile;
    int pid;
    

    if (!doRlogin) {
	(void) sprintf(s, "/hosts/%s/dev/syslog", host);
	if (debug) {
	    printf("would write to %s\n", s);
	    return;
	} 
	fd = open(s, O_WRONLY);
	if (fd < 0) {
	    perror(s);
	    return;
	}
	if (verbose) {
	    (void) printf("sending to %s\n", host);
	}
	(void) write(fd, alert, strlen(alert));
	(void) write(fd, msg, msize);
	(void) write(fd, "\7\7\7", 3);
	(void) close(fd);
    } else {
	for (i = 1; i < ULOG_MAX_PORTS; ++i) {
	    (void) sprintf(s, "/hosts/%s/%s%d", host, RLOGIN_PDEV_NAME, i);
	    if (debug) {
		printf("would write to %s\n", s);
		return;
	    } 
	    if (stat(s, &statBuf) < 0) {
		continue;
	    }
	    pid = fork();
	    if (pid) {
		union wait status;
		int error;
		int exitCode;
		struct itimerval itimer;

		itimer.it_interval.tv_sec = 0;
		itimer.it_interval.tv_usec = 0;
		itimer.it_value.tv_sec = TIMEOUT;
		itimer.it_value.tv_usec = 0;
		(void) signal(SIGALRM, AlarmHandler);
		(void) setitimer(ITIMER_REAL, &itimer, NULL);

		exited = 0;
		do {
		    if (setjmp(OpenTimeout) != 0) {
			break;
		    }
		    error = wait(&status);
		    if (error == -1) {
			if (errno == EINTR) {
			    /*
			     * This will unfortunately never be reached
			     * until wait can actually return EINTR (not
			     * possible now due to migration interactions
			     * with signals).  Hence the setjmp.
			     */
			    break;
			} else {
			    perror("wait");
			    exited = 1;
			}
		    } else if (error == pid) {
			if (status.w_stopval != WSTOPPED) {
			    exited = 1;
			}
		    }
		} while (!exited);
		itimer.it_value.tv_sec = 0;
		(void) setitimer(ITIMER_REAL, &itimer, NULL);
	    } else {
		/*
		 * Child.
		 */
		if (verbose) {
		    (void) printf("Opening %s.\n", s);
		}
		fd = open(s, O_WRONLY | O_APPEND);
		if (fd < 0) {
		    exit(1);
		}
		if (verbose) {
		    (void) printf("Writing to %s.\n", s);
		}
		if (write(fd, alert, strlen(alert)) < 0) {
		    exit(1);
		}
		(void) write(fd, msg, msize);
		(void) write(fd, "\7\7\7", 3);
		(void) close(fd);
		exit(0);
	    } 
		
	}
    }
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * FillAlert --
 *
 *      Creates the `alert' message that is sent as a prelude
 *      to the main message.
 *
 * Results:
 *      None.
 * 
 * Side effects:
 *      Prints a string into the `alert' buffer.
 *  
 *----------------------------------------------------------------------
 */ 

static void
FillAlert()
{
    char *me;
    char hostname[32];
    long clock;
    char *timestamp;
#ifdef notdef
    struct tm *localtime();
    struct tm *localclock;
#endif
    char *ttyname();

    me = getpwuid(getuid())->pw_name;
    (void) gethostname(hostname, sizeof(hostname));
    (void) time(&clock);
    timestamp = ctime(&clock);
#ifdef notdef
    localclock = localtime(&clock);
    (void) sprintf(alert,
	"\7\7\7%s broadcast message from %s@%s at %d:%02d ...\n",
	local ? "Local" : "Network-wide", me, hostname,
		   localclock->tm_hour, localclock->tm_min);
#endif
    (void) sprintf(alert,
	"\7\7\7%s broadcast message from %s@%s at %s ...\n",
	local ? "Local" : "Network-wide", me, hostname, timestamp);
    return;
}


/*
 *----------------------------------------------------------------------
 *
 * Prompt --
 *
 *      Print a prompt.
 *
 * Results:
 *      None.
 * 
 * Side effects:
 *      Prints a prompt to stdout.
 *  
 *----------------------------------------------------------------------
 */ 

static void
Prompt()
{

    (void) write(1, "> ", 2);
    return;
}

