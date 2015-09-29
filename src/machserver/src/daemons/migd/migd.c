/* 
 * migd.c --
 *
 *	Routines to manage duties of the host-specific load average daemon.
 *	This includes supporting the host-specific pdev, and tracking load
 *	average and idle time.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/daemons/migd/RCS/migd.c,v 1.2 92/04/29 22:29:11 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <bstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <fs.h>
#include <option.h>
#include <syslog.h>
#include <status.h>
#include <sysStats.h>
#include <host.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <kernel/procMigrate.h>
#include <bit.h>
#include <pwd.h>
#include "migd.h"
#include "migPdev.h"
#include "global.h"

/*
 * Indication of whether this process is the global master or
 * a per-host daemon.
 */

int migd_GlobalMaster = 0;


int migd_Debug = 0;		/* Enable debugging info? */
int migd_Quit = 0;		/* Flag to indicate we should quit */

Fs_TimeoutHandler migd_TimeoutToken; /* Token for current timeout. */

static int migdGlobalDesc = -1;	/* Descriptor for communications with
				   global server. */

static Mig_Info currentInfo;	/* Current load info, passed to server */
static Mig_LoadVector *curVecPtr; /* Pointer into part of currentInfo that
				   * is periodically updated. */

int migd_AlwaysAccept = 0;	/* Override defaults for accepting processes? */
int migd_NeverAccept = 0;	/* Likewise. */

/*
 * The load interval is the number of seconds between sampling the system
 * load, and the write interval is the maximum number of seconds that may
 * pass between updating the global daemon.  These constants are defined in
 * mig.h
 */

int migd_Verbose = 1;
int migd_LoadInterval = MIG_LOAD_INTERVAL;
int migd_WriteInterval = MIG_GLOBAL_UPDATE_INTERVAL;

/*
 * Define global variables used for debugging & statistics gathering.
 */
int migd_LogToFiles = 0;
int migd_DontFork = 0;
int migd_DoStats = 1;
int migd_NeverEvict = 0;
int migd_NeverRunGlobal = 0;

/*
 * Define arbitrary thresholds (see loadavg.h).
 *
 * All load values must be below their corresponding THRESHOLD_LOW* to
 * start accepting foreign processes, but once they are being accepted,
 * there are different thresholds (THRESHOLD_HIGH*) for each average: if
 * any one of them is exceeded, stop accepting foreign processes.  (This
 * really needs to be generalized to handle a multiprocessor.)
 *
 * INPUT_THRESHOLD is the number of seconds that a node must
 * be idle for it to accept migrated processes.  
 */

#define THRESHOLD_LOW0 0.75
#define THRESHOLD_LOW1 1.0
#define THRESHOLD_LOW2 1.5
#define THRESHOLD_HIGH0 2.5
#define THRESHOLD_HIGH1 2.75
#define THRESHOLD_HIGH2 3.0
#ifndef INPUT_THRESHOLD
#define INPUT_THRESHOLD 30
#endif

/* 
 * Initialize our parameters.  Fill in 0 for the things we have to get
 * at run time.
 */
Mig_SystemParms migd_Parms = {
    0,				   /* criteria, obtained from kernel */
    0,				   /* version, obtained from kernel  */
    INPUT_THRESHOLD,					/* noInput   */
    0,							/* pad       */
    {THRESHOLD_LOW0, THRESHOLD_LOW1, THRESHOLD_LOW2},   /* minThresh */
    {THRESHOLD_HIGH0, THRESHOLD_HIGH1, THRESHOLD_HIGH2}	/* maxThresh */
};

/*
 * Set the default timeout for eviction, in seconds.  
 */
#ifndef EVICT_TIMEOUT
#define EVICT_TIMEOUT 300
#endif /* EVICT_TIMEOUT */

/*
 * Set the default write timeout, in seconds.
 */
#ifndef WRITE_TIMEOUT
#define WRITE_TIMEOUT 10
#endif /* WRITE_TIMEOUT */

/*
 * Set the number of times we'll try to contact the global daemon before
 * giving up.
 */
#ifndef MAX_GLOBAL_CONTACTS
#define MAX_GLOBAL_CONTACTS 100
#endif /* MAX_GLOBAL_CONTACTS */


static int refuseMigration;	/* whether kernel is refusing migrations */

static int ignoreInput;		/* whether we should ignore input time when
				   deciding when to accept migration */
static int ignoreLoad;		/* ditto for load average */
static int writeRate;		/* number of iterations before updating global
				   server */

static int numLowPris = 0;	/* Number of low-priority processes on this
				   host.  XXX Set by searching process
				   table. */

static	jmp_buf	writejmp;		/* Used for timeouts on writes. */
static  struct itimerval timeOutTimer; 	/* Set to WRITE_TIME_OUT. */
static  struct itimerval noTimer;	/* Set to 0. */

static int Reap();
static int AlarmHandler();
static int WriteAlarm();
static void ExamineLoads();
static void GetStats();
static void OutputLoads();
static void ParseMigStatus();
static int  CheckMessages();

static int ContactGlobal();
static int CreateGlobal();
#ifdef FAST_SELECT
static void GetNewParms();
static void HandleException();
#endif /* FAST_SELECT */

/*
 *----------------------------------------------------------------------
 *
 * Migd_Init --
 *
 *	Initialize data structures for a per-host migration daemon.
 * 
 * Results:
 *	0 for success, or a sprite ReturnStatus for failure.
 *
 * Side effects:
 *	The pseudo-device to talk to the master is opened.  If no
 *	master exists, this process tries to create a master.
 *
 *----------------------------------------------------------------------
 */

int
Migd_Init()
{
    int realErrno;
    char fileName[FS_MAX_NAME_LENGTH];
    int status;
    struct stat atts;
    Time	period;

    if (migd_Debug > 0) {
	fprintf(stderr, "Migd_Init -\n");
    }

    signal(SIGCHLD, Reap);

    ParseMigStatus();
    
    /*
     * Set up the initial Mig_Info structure, and variables we're going
     * to access periodically.  By default, we're active when we
     * start up, but later there's a check for idleTime being less
     * than the threshold if it's also equal to the time since we booted.
     */
    curVecPtr = &currentInfo.loadVec;
    bzero((char *) &currentInfo, sizeof(Mig_Info));
    currentInfo.hostID = migd_HostID;
    currentInfo.migVersion = migd_Parms.version;
    currentInfo.maxProcs = 1;	/* XXX number of processors */
    curVecPtr->allowMigration = migd_AlwaysAccept;
    currentInfo.state = refuseMigration ?  MIG_HOST_REFUSES : MIG_HOST_ACTIVE;

    curVecPtr->timestamp = time((time_t *)0);

    /*
     * Get bootstamp.  Note, this should be a kernel call instead of a
     * check on the file, but this will do for the "moment".
     */
    (void) sprintf(fileName, "/hosts/%s/boottime", migd_HostName);

    status = stat(fileName, &atts);
    if (status == 0) {
	currentInfo.bootTime = atts.st_mtime;
    } else {
	SYSLOG2(LOG_ERR, "Error getting boot time from %s: %s\n", fileName,
	       strerror(errno));
	currentInfo.bootTime = 0;
    }


    /*
     * Check to see if the global daemon is prohibited.
     */
    if (!migd_NeverRunGlobal) {
	(void) sprintf(fileName, 
		       "/hosts/%s/global-migd-prohibited", migd_HostName);
	status = stat(fileName, &atts);
	if (status == 0) {
	    syslog(LOG_INFO, "We will not run the global master.\n");
	    migd_NeverRunGlobal = 1;
	}
    }


    writeRate = migd_WriteInterval / migd_LoadInterval;
    
    GetStats(curVecPtr->lengths, &curVecPtr->noInput,
	     &curVecPtr->foreignProcs);
    curVecPtr->timestamp = time((time_t *)0);
    
    if (ContactGlobal() < 0) {
	realErrno = errno;
	fprintf(stderr, "Migd_Init - Unable to contact global daemon: %s\n",
	       strerror(errno));
	errno = realErrno;
	return(-1);
    }
    if (MigPdev_OpenMaster() < 0) {
	realErrno = errno;
	fprintf(stderr, "Migd_Init - Unable to initialize pdev %s: %s\n",
	       migd_LocalPdevName, strerror(errno));
	errno = realErrno;
	return(-1);
    }

    syslog(LOG_INFO, "running.");
    period.seconds = migd_LoadInterval;
    period.microseconds = 0;

    /*
     * Set up timeout for writes.
     */
    timeOutTimer.it_interval.tv_sec = 0;
    timeOutTimer.it_interval.tv_usec = 0;
    timeOutTimer.it_value.tv_sec = WRITE_TIMEOUT;
    timeOutTimer.it_value.tv_usec = 0;
    noTimer.it_interval.tv_sec = 0;
    noTimer.it_interval.tv_usec = 0;
    noTimer.it_value.tv_sec = 0;
    noTimer.it_value.tv_usec = 0;


    migd_TimeoutToken = Fs_TimeoutHandlerCreate(period, TRUE, Migd_GatherLoad,
						(ClientData) NULL);


    if (migd_Debug > 0) {
	fprintf(stderr, "Migd_Init - returning 0\n");
    }

    return(0);
}



/*
 *----------------------------------------------------------------------
 *
 * Migd_End --
 *
 *	Terminate service of the master end of a host-specific pdev.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The pdev is removed and closed.
 *
 *----------------------------------------------------------------------
 */

void
Migd_End()
{
    if (migd_Debug > 0) {
	fprintf(stderr, "Migd_End -\n");
    }

    MigPdev_End();

}


/*
 *----------------------------------------------------------------------
 *
 * ParseMigStatus --
 *
 *	Parse the kernel status by checking individual bits.  This
 *	is performed at start-up and anytime the migd_Parms structure
 *	is updated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	File-global variables are updated.
 *
 *----------------------------------------------------------------------
 */

static void
ParseMigStatus()
{
    int kernelState = migd_Parms.criteria;
    
    if ((kernelState & PROC_MIG_IMPORT_ALL) != PROC_MIG_IMPORT_ALL) {
	refuseMigration = 1;
    } else {
	refuseMigration = 0;
    }
    ignoreLoad = kernelState & PROC_MIG_IMPORT_ANYLOAD;
    ignoreInput = kernelState & PROC_MIG_IMPORT_ANYINPUT;
}




/*
 *----------------------------------------------------------------------
 *
 * ContactGlobal --
 *
 *	Open up the global pseudo-device as a client, and inform
 *	the master that we are a daemon.  If we can't open it, we sleep
 *	a random period of time and try again before returning an error.
 *	This reduces the likelihood of many hosts trying to open the
 *	pdev as a master when one fails.
 *
 * Results:
 *	0 for successful completion, -1 for error, in which case
 *	errno indicates the nature of the error.  The special errno
 *	ENODEV indicates that the master is nonexistent.
 *
 * Side effects:
 *	Opens pseudo-device as client.
 *
 *----------------------------------------------------------------------
 */

static int
ContactGlobal()
{
    int sleepTime;
    int status;
    int retries;
    int ioctlRetries;
    int realErrno;
    int success = 0;
    static int firstContact = 1; /* First time we are trying to reach the
				    global daemon? */
    int t;
    
    t = time(0);
    if (migd_Debug > 1) {
	fprintf(stderr, "ContactGlobal - %s\n", ctime(&t));
    }

    /*
     * Set a temporary variable to track firstContact, and reset it so
     * any subsequent calls have the updated value.  This avoids the
     * need to reset it before every return statement.
     */
    if (firstContact) {
	/*
	 * First time we've been called. Set up seed for random
	 * numbers.
	 */
	srandom(getpid());
	firstContact = 0;
    } 

    /*
     * Clean up any old descriptor.
     */
    if (migdGlobalDesc >= 0) {
	Fs_EventHandlerDestroy(migdGlobalDesc);
	(void) close(migdGlobalDesc);
    }
    
    sleepTime = (random() & 07) + 1;
    for (retries = 1;
	 retries <= MAX_GLOBAL_CONTACTS && !migd_Quit && !success;
	 retries++) {
	migdGlobalDesc = open(migd_GlobalPdevName, O_RDWR, 0);
	if (migdGlobalDesc < 0) {
	    if (migd_Debug > 2) {
		fprintf(stderr, "ContactGlobal - sleeping %d seconds\n",
		       sleepTime);
	    }
	    sleep(sleepTime);
	    sleepTime *= 2;
	    migdGlobalDesc = open(migd_GlobalPdevName, O_RDWR, 0);
	}
	if (migdGlobalDesc < 0) {
	    if (migd_Debug > 0) {
		fprintf(stderr,
		       "ContactGlobal: couldn't open %s: %s\n",
		       migd_GlobalPdevName, strerror(errno));
	    }
	    /*
	     * If errno is ENOENT, there is not currently a master, anywhere.
	     * (When the master exits it removes the pdev.)  EIO 
	     * may mean the daemon crashed.  EINVAL may mean the daemon's host
	     * crashed.  We special case EIO due to a race condition
	     * between recovery and starting daemons.
	     */
	    if (errno == ENOENT || errno == EIO || errno == EINVAL) {
		if (retries == MAX_GLOBAL_CONTACTS - 1 && errno != ENOENT) {
		    /*
		     * We're getting desperate here.  We can't open
		     * the file, but we should be able to.  Remove
		     * the pdev and try one last time to create the
		     * master, since it may be that the host running
		     * the master has crashed and the name server
		     * is continually returning a bad status to us.
		     * We risk clobbering someone else who has successfully
		     * opened the pdev just before us, but there's a small
		     * window of vulnerability and by this time we're sleeping
		     * a long time.  
		     */
		    (void) unlink(migd_GlobalPdevName);
		}
		if (!migd_NeverRunGlobal) {
		    if (CreateGlobal() < 0) {
			return(-1);
		    }
		}
		/*
		 * Go to start of for loop, trying to open pdev.
		 */
		continue;
	    } else {
		realErrno = errno;
		fprintf(stderr, "Migd_Init - Unable to contact master of global pdev: %s\n",
		       strerror(errno));
		errno = realErrno;
		return(-1);
	    }
	} else {
	    /*
	     * We've successfully opened the pdev.
	     * Try to tell the global master that we're a daemon.  It may say
	     * DEV_BUSY, which means that there is already a daemon.  In that
	     * case, it tells the other daemon to go away, and we will keep
	     * trying.  In some cases we may get an error doing the ioctl,
	     * such as a stale handle, in which case we close the file and
	     * go to the top again.
	     */
	    for (ioctlRetries = 1; ioctlRetries <= MAX_GLOBAL_CONTACTS;
		 ioctlRetries++) {
		status = Fs_IOControl(migdGlobalDesc, IOC_MIG_DAEMON, sizeof(Mig_Info),
				      (char *) &currentInfo,
				      0, (char *) NULL);
		if (status == DEV_BUSY) {
		    if (migd_Debug > 0) {
			fprintf(stderr, "ContactGlobal - ioctl returned busy.\n");
		    }
		    sleepTime = ((random() & 07) + 1) * ioctlRetries;
		    if (migd_Debug > 2) {
			fprintf(stderr, "ContactGlobal - sleeping %d seconds\n",
			       sleepTime);
		    }
		    sleep(sleepTime);
		} else {
		    /*
		     * An error we can't deal with, or SUCCESS.
		     */
		    break;
		}
	    }
	    if (status != SUCCESS) {
		SYSLOG1(LOG_ERR,
		       "ContactGlobal: warning: error during ioctl to global master: %s\n",
		       Stat_GetMsg(status));
		errno = Compat_MapCode(status);
		close(migdGlobalDesc);
	    } else {
		/*
		 * We did it!  Break out of the inner for loop, and the
		 * success flag will break us out of the outer loop.
		 */
		success = 1;
		break;
	    }
	}
    }
    if (!success) {
	realErrno = errno;
	SYSLOG0(LOG_ERR, "unable to contact master; giving up.\n");
	errno = realErrno;
	return(-1);
    }

#ifdef FAST_SELECT
    Fs_EventHandlerCreate(migdGlobalDesc, FS_READ|FS_EXCEPTION,
			  HandleException, (ClientData) NULL);
#endif /* FAST_SELECT */

    if (migd_Debug > 1) {
	fprintf(stderr, "ContactGlobal - completed successfully\n");
    }

    return (0);
}


/*
 *----------------------------------------------------------------------
 *
 * Reap --
 *
 *	Reap any exited child processes to make sure they don't stick
 *	around.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
Reap()
{
    union wait status;

    if (migd_Debug > 3) {
	fprintf(stderr, "Reap - \n");
    }
	
    while (wait3(&status, WNOHANG, (struct rusage *) NULL) > 0) {
    }
    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * WriteAlarm --
 *
 *	Routine to service a SIGALRM signal during a write, which
 *	requires that we longjmp to interrupt the write.
 *	This routine also disables the alarm, letting the caller reenable
 *	it when appropriate.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The alarm is disabled.
 *
 *----------------------------------------------------------------------
 */
static int
WriteAlarm()
{
    
    alarm(0);
    syslog(LOG_INFO, "Write to global daemon timed out.");
    (void) signal (SIGALRM, SIG_IGN);
    longjmp(writejmp, 1);
    /* THIS DOES NOT RETURN. */
}

/*
 *----------------------------------------------------------------------
 *
 * CreateGlobal --
 *
 *	Fork off a process to try and become the global master.  
 *
 * Results:
 *	0 for successful completion, -1 for error, in which case
 *	errno indicates the nature of the error.  The parent
 *	will return success as long as it can successfully fork.  That
 * 	doesn't necessarily mean the master side of the global pdev
 *	has been opened, at least by the child, but the parent will go
 * 	ahead and try to contact the master once again in any case.
 *
 * Side effects:
 *	A new process is created.
 *
 *----------------------------------------------------------------------
 */

static int
CreateGlobal()
{
    int pid;
    
    if (migd_Debug > 2) {
	fprintf(stderr, "CreateGlobal -\n");
    }

    if (migd_DontFork) {
	fprintf(stderr, "CreateGlobal - becoming master; start a new child.\n");
	pid = 0;
    } else {
	pid = fork();
	if (pid < 0) {
	    fprintf(stderr, "CreateGlobal - couldn't fork\n");
	    return(-1);
	}
    }
    if (pid > 0) {
	/*
	 * Give the child a chance to set things up.  We should be awakened
	 * by a signal if the child should exit prematurely.
	 */
	sleep(5);
	return(0);
    }

    /*
     * We are the child, and will try to become the global master.
     */
    (void) signal(SIGCHLD, SIG_DFL);
    MigPdev_CloseAllStreams();

    if (Global_Init() < 0) {
	exit(1);
    }
	
    if (migd_Debug > 0) {
	fprintf(stderr, "CreateGlobal - we are the global master, pid %x\n",
	       getpid());
    }

    Migd_HandleRequests();

    /*
     * If we reach this point we were signalled or in some other way
     * told to exit.
     */

    Global_End();
    DATE();
    exit(0);

#ifdef lint
    return(0); /* keep lint happy */
#endif
}



/*
 *----------------------------------------------------------------------
 *
 * Migd_HandleRequests --
 *
 *	Main loop to call Fs_Dispatch; used by both per-host daemons and
 * 	global master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls to Fs_Dispatch invoke callbacks.
 *
 *----------------------------------------------------------------------
 */

void
Migd_HandleRequests()
{
    while(1) {
	if (migd_Quit && !migd_GlobalMaster) {
	    break;
	}
	Fs_Dispatch();
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Migd_GatherLoad --
 *
 *	Update the current load averages and report them to the master
 *	daemon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes to master daemon.
 *
 *----------------------------------------------------------------------
 */

void
Migd_GatherLoad()
{
    int oldAllow;
    int	oldInput;
    int	oldForeign;
    static int iteration = 0;
    int numWritten;
    int error;
    int status;
    
    
    oldAllow = curVecPtr->allowMigration;
    oldInput = curVecPtr->noInput;
    oldForeign = curVecPtr->foreignProcs;
    if (migd_Debug > 2) {
	fprintf(stderr, "Migd_GatherLoad - time %d, oldAllow %d, oldInput %d\n",
	       time((int *) NULL), oldAllow, oldInput);
    }
    GetStats(curVecPtr->lengths, &curVecPtr->noInput,
	     &curVecPtr->foreignProcs);
    curVecPtr->timestamp = time((time_t *)0);
    ExamineLoads(curVecPtr);

    if ((oldInput > migd_Parms.noInput) &&
	(curVecPtr->noInput < migd_Parms.noInput) &&
	!ignoreInput && !migd_NeverEvict && !refuseMigration) {
	Migd_Evict(TRUE);
    }

    /*
     * Send the new load vector to the global daemon periodically,
     * or if our migration status changes, or if the number of
     * foreign processes goes from zero to non-zero or vice-versa.
     * This way the global daemon can track things like the last use
     * of a machine by a process that won't release the host when it
     * finishes.
     */
    if (iteration == 0 || (oldAllow != curVecPtr->allowMigration) ||
	(oldForeign > 0 && curVecPtr->foreignProcs == 0) ||
	(oldForeign == 0 && curVecPtr->foreignProcs > 0)) {
	if (migd_Debug > 2) {
	    fprintf(stderr,
		   "Notifying global server, iteration %d, oldAllow %d, newAllow %d, oldForeign %d, newForeign %d.\n",
		   iteration, oldAllow, curVecPtr->allowMigration,
		   oldForeign, curVecPtr->foreignProcs);
	}
	iteration = 0;

	/*
	 * Get the kernel's variable determining whether to refuse
	 * migrations.  We keep rechecking periodically in case it changes.
	 */

	status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_STATE,
			   (Address) &migd_Parms.criteria);
	if (status != SUCCESS) {
	    SYSLOG1(LOG_ERR, "Error in Sys_Stats getting migration state: %s.\n",
		    Stat_GetMsg(status));
	    exit(Compat_MapCode(status));
	}
	ParseMigStatus();

	if (curVecPtr->lengths[1] >= 1.0)  {
	    struct timeval tv;
	    struct timeval curTime;
	    
	    /*
	     * The 5-minute load average is over 1.  This could
	     * happen if there is a long-running process but it
	     * also seems to happen without anything running.
	     * Sleep a short period of time to try to
	     * keep from being in lock-step with someone else.  There's
	     * nothing too magical about the number except that it's
	     * intended to be something that other processes are unlikely
	     * to sleep for.  
	     */
	    tv.tv_sec = 0;
	    tv.tv_usec = ((random() % 999) + 1) * 1000;
;
	    if (migd_Debug > 2) {
		if (gettimeofday(&curTime,
				 (struct timezone *) NULL) < 0) {
		    perror("Error in gettimeofday");
		    exit(1);
		}
		fprintf(stderr,
			"Sleeping %d usec to avoid lock step, time %d.%d.\n",
			tv.tv_usec, curTime.tv_sec, curTime.tv_usec);
	    }
	    if (select(0, (int *) NULL, (int *) NULL, (int *) NULL,
			  &tv) < 0) {
		if (migd_Debug > 2) {
		    perror("select");
		}
	    }
	    if (migd_Debug > 2) {
		if (gettimeofday(&curTime,
				 (struct timezone *) NULL) < 0) {
		    perror("Error in gettimeofday");
		    exit(1);
		}
		fprintf(stderr, "Time is now %d.%d.\n", curTime.tv_sec,
			curTime.tv_usec);
	    }
	}
	if (migd_Debug > 3) {
	    fprintf(stderr, "Writing vector to global daemon.\n");
	}

	/*
	 * OK, here's the tricky part.  We don't want our write to wait
	 * indefinitely, so we set an alarm.  But just waking up won't
	 * cause Fs_Write to return an error, so we have to longjmp.
	 * So we set the signal handler, set the timer, and setjmp, then
	 * after the write we reverse the process.
	 */
	if (setjmp(writejmp)) {
	    numWritten = -1;
	    errno = EIO;
	} else {
	    if ((int) signal(SIGALRM, WriteAlarm) < 0) {
		syslog(LOG_ERR, "Error setting signal handler: %s.\n",
		       strerror(errno));
		exit(1);
	    }
	    if (setitimer(ITIMER_REAL, &timeOutTimer,
			  (struct itimerval *) NULL) == -1) {
		syslog(LOG_ERR, "Error setting interval timer: %s.\n",
		       strerror(errno));
		exit(1);
	    }
	    numWritten = write(migdGlobalDesc, (char *) curVecPtr,
			       sizeof(Mig_LoadVector));
	}
	error = errno;
	if (setitimer(ITIMER_REAL, &noTimer,
		      (struct itimerval *) NULL) == -1) {
	    syslog(LOG_ERR, "Error disabling interval timer: %s.\n",
		   strerror(errno));
	    exit(1);
	}
	(void) signal(SIGALRM, SIG_IGN);
	errno = error;

	/*
	 * Now we're back to where we would be if all we'd done was
	 * write(), with errno and numWritten set to appropriate values.
	 */

	if (migd_Debug > 3) {
	    fprintf(stderr, "Write returned value %d.\n", numWritten);
	}
	if (numWritten < 0) {
	    if (migd_Debug > 0) {
		fprintf(stderr, "Error %d writing to global daemon: %s.\n",
		       error, strerror(error));
	    }
	    close(migdGlobalDesc);
	    if (migd_Quit || ContactGlobal() < 0) {
		fprintf(stderr, "Exiting.\n");
		exit(1);
	    }
	} else if (numWritten != sizeof(Mig_LoadVector)) {
	    SYSLOG2(LOG_WARNING, "short write to global daemon of %d/%d bytes.\n",
		   numWritten, sizeof(Mig_LoadVector));
	}
	iteration = 0;

	/*
	 * Check on currentInfo.state in case we have to reconnect to the
	 * global daemon or a user process reads the Mig_Info struct from
	 * us.
	 */
	if (curVecPtr->allowMigration &&
	    currentInfo.state == MIG_HOST_ACTIVE) {
	    currentInfo.state = MIG_HOST_IDLE;
	} else if (!curVecPtr->allowMigration &&
		   currentInfo.state == MIG_HOST_IDLE) {
	    currentInfo.state = refuseMigration ?
		MIG_HOST_REFUSES : MIG_HOST_ACTIVE;
	}

	if (CheckMessages() >= 0) {
	    if (migd_Debug > 0) {
		fprintf(stderr,
			"This host is being reclaimed by order of global migration daemon.\n");
	    }
	    Migd_Evict(FALSE);
	}

    }

    iteration = (iteration + 1) % writeRate;
}


/*
 *----------------------------------------------------------------------
 *
 * ExamineLoads --
 *
 *	Look at the current load value and make any state changes (e.g., 
 *	was accepting migrations, but not any more).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets currentInfo.state and curVecPtr->allowMigration.
 *
 *----------------------------------------------------------------------
 */

static void
ExamineLoads(curVecPtr)
    Mig_LoadVector *curVecPtr;
{
    int i;
    static int init = 0;

    if (!init) {
	init = 1;
	for (i = 0; i < MIG_NUM_LOAD_VALUES; i++) {
	    curVecPtr->utils[i] = 0;
	    curVecPtr->lengths[i] = 0;
	}
    }
    
    if (migd_Debug > 3) {
	(void)fprintf(stderr, "current loads:");
	for (i = 0; i < MIG_NUM_LOAD_VALUES; i++) {
	    (void)fprintf(stderr, " %6.2f", curVecPtr->lengths[i]);
	}
	(void) fprintf(stderr, "\nallowMigration %d noInput %d\n",
		       curVecPtr->allowMigration, curVecPtr->noInput);
	fflush(stderr);
    }

    /*
     * Check for our state changing from nonmigratable to migratable or
     * vice-versa.
     */

    if (!migd_AlwaysAccept) {
	if (curVecPtr->allowMigration) {
	    if ((((curVecPtr->noInput < migd_Parms.noInput) &&
		  (curVecPtr->noInput <
		   curVecPtr->timestamp - currentInfo.bootTime - migd_LoadInterval)) &&
		 !ignoreInput) ||
		refuseMigration) {
		curVecPtr->allowMigration = 0;
		if (refuseMigration) {
		    currentInfo.state = MIG_HOST_REFUSES;
		} else {
		    currentInfo.state = MIG_HOST_ACTIVE;
		}
	    } else if (!ignoreLoad){
		for (i = 0; i < MIG_NUM_LOAD_VALUES; i++) {
		    /*
		     * Compare the load to the threshold, ignoring
		     * low-priority processes.
		     */
		    if (curVecPtr->lengths[i] - numLowPris >
			migd_Parms.maxThresh[i]) {
			if (migd_Debug > 3) {
			    fprintf(stderr, "Load too high\n");
			}
			curVecPtr->allowMigration = 0;
			break;
		    } else if (migd_Debug > 3 && curVecPtr->lengths[i] >
			       migd_Parms.maxThresh[i]) {
			fprintf(stderr, "Load low enough because of %d low priority processes\n",
				numLowPris);
		    }
		}
	    }
	    /*
	     * Allow migration if we're ignoring input or we've been
	     * idle for a while or we've been idle as long as we've been up.
	     * Don't allow it if overridden by a flag, though.
	     */
	} else if ((! curVecPtr->allowMigration) &&
		   (ignoreInput || (curVecPtr->noInput >= migd_Parms.noInput) ||
		    (curVecPtr->noInput >=
		     curVecPtr->timestamp - currentInfo.bootTime - migd_LoadInterval)) &&
		   !refuseMigration) {
	    curVecPtr->allowMigration = 1;
	    currentInfo.state = MIG_HOST_IDLE;
	    if (!ignoreLoad) {
		for (i = 0; i < MIG_NUM_LOAD_VALUES; i++) {
		    if (curVecPtr->lengths[i] > migd_Parms.minThresh[i]) {
			curVecPtr->allowMigration = 0;
			currentInfo.state = MIG_HOST_ACTIVE;
		    }
		}
	    }
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * GetStats --
 *
 *	Get the system load information and idle time.
 *
 * Results:
 * 	Fills in the averaged system load values, time since last console 
 * 	input, and number of foreign processes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetStats(loadsPtr, noInputPtr, foreignPtr)
    double *loadsPtr;		/* OUT: load averages */
    int *noInputPtr;		/* OUT: idle time in seconds */
    int *foreignPtr;		/* OUT: number of foreign processes */
{
    Sched_Instrument stats;
    int status;
    ReturnStatus spriteStatus;
    Proc_MigStats migStats;

    spriteStatus = Sys_Stats(SYS_SCHED_STATS, sizeof(stats),
			     (Address)&stats);
    if (spriteStatus != SUCCESS) {
	fprintf(stderr, "GetStats: can't get sched stats: %s\n",
		Stat_GetMsg(spriteStatus));
	bzero(&stats, sizeof(stats));
    }

    if (MACH_NUM_LOAD_VALUES == MIG_NUM_LOAD_VALUES) {
	bcopy(stats.avenrun, loadsPtr,
	      MACH_NUM_LOAD_VALUES * sizeof(double)); 
    } else {
	fprintf(stderr,
		"GetStats: Mach returns %d load numbers, we want %d.\n",
		MACH_NUM_LOAD_VALUES, MIG_NUM_LOAD_VALUES);
	bzero(loadsPtr, MIG_NUM_LOAD_VALUES * sizeof(*loadsPtr));
    }

    *noInputPtr = stats.noUserInput.seconds;
    if (migd_Debug > 3) {
	fprintf(stderr, "GetStats: no input in %d seconds.\n",
	       stats.noUserInput.seconds);
    }
    /*
     * Check for bogus idle time.
     */
    if (*noInputPtr < 0) {
	*noInputPtr = 0;
    }

    /*
     * Get a copy of the proc statistics record.  Make sure it's zeroed in
     * case the kernel provides us with a shorter (older) structure.
     */

    bzero((Address) &migStats, sizeof(migStats));
    status = Sys_Stats(SYS_PROC_MIGRATION, SYS_PROC_MIG_GET_STATS,
		       (Address) &migStats);
    if (status != SUCCESS) {
	SYSLOG1(LOG_ERR, "Can't get migration stats: %s.\n",
		Stat_GetMsg(status));
	exit(1);
    }
    *foreignPtr = migStats.foreign;


}

/*
 *----------------------------------------------------------------------
 *
 * Migd_GetLocalLoad --
 *
 *	Return the current migInfo structure -- this is used by anyone
 *	contacting the local daemon and using read to get the local
 *	load.
 *
 * Results:
 *	Fills buffer with Mig_Info struct.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Migd_GetLocalLoad(buffer)
    char *buffer;
{
    Mig_Info *infoPtr;

    infoPtr = (Mig_Info *) buffer;

    *infoPtr = currentInfo;
}



/*
 *----------------------------------------------------------------------
 *
 * HandleException --
 *
 *	Do something when the migdGlobalDesc becomes selectable.  This
 *	can happen if the stream gets an error of some sort, or
 *	if it becomes readable (meaning we should contact the daemon about
 * 	something).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The daemon may be contacted, or the stream to it reopened.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
#ifdef FAST_SELECT
static void
HandleException(clientData, streamID, eventMask)
    ClientData clientData;
    int streamID;
    int eventMask;
{
    if (migd_Debug > 0) { 
	fprintf(stderr, "HandleException -\n");
    }
    if (eventMask & FS_EXCEPTION) {
	fprintf(stderr,
	       "Exception occurred talking to global daemon.  Reopening contact.\n");
	(void) close(streamID);
	if (migd_Quit || ContactGlobal() < 0) {
	    DATE();
	    SYSLOG0(LOG_ERR, "Exiting.\n");
	    exit(1);
	}
    }
    if (eventMask & FS_READ) {
	GetNewParms();
    }
}
#endif /* FAST_SELECT */


#ifdef FAST_SELECT
/*
 *----------------------------------------------------------------------
 *
 * GetNewParms --
 *
 *	Get new parameters from the global daemon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The migd_Parms structure is updated, and global variables
 *	are reset accordingly.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static void
GetNewParms(clientData, streamID, eventMask)
    ClientData clientData;
    int streamID;
    int eventMask;
{
    fprintf(stderr, "GetNewParms called, doing nothing yet.\n");
}
#endif /* FAST_SELECT */


/*
 *----------------------------------------------------------------------
 *
 * Migd_SetParms --
 *
 *	Set the parameters used by this daemon (only).
 *
 * Results:
 *	0 for success, or an errno indicating the error.
 *
 * Side effects:
 *	The migd_Parms structure is updated, and global variables
 *	are reset accordingly.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
int
Migd_SetParms(cltPtr, command, inBuffer, inBufSize, outBuffer,
	       outBufSizePtr)
    Migd_OpenStreamInfo *cltPtr;/* Information about the client making
				   the request. */
    int command;		/* Ignored. */
    char *inBuffer;		/* Buffer to get arguments from. */
    int inBufSize;		/* Size of the input buffer. */
    char *outBuffer;		/* Buffer to place results, not used. */
    int *outBufSizePtr;		/* Size of the output buffer, set to 0. */
{
    if (migd_Debug > 1) {
	fprintf(stderr, "Migd_SetParms called.\n");
    }

    /*
     * XXX need to add this here.
     */
    return(EINVAL);
}
    

/*
 *----------------------------------------------------------------------
 *
 * Migd_GetParms --
 *
 *	Get the parameters used by this daemon.
 *
 * Results:
 *	0 for success, or an errno indicating the error.
 *	The parameters structure, and the size of the buffer, are returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
int
Migd_GetParms(cltPtr, command, inBuffer, inBufSize, outBuffer,
	       outBufSizePtr)
    Migd_OpenStreamInfo *cltPtr;/* Information about the client making
				   the request. */
    int command;		/* Ignored. */
    char *inBuffer;		/* Not used. */
    int inBufSize;		/* Not used. */
    char *outBuffer;		/* Buffer to place results. */
    int *outBufSizePtr;		/* Size of the output buffer. */
{
    if (migd_Debug > 1) {
	fprintf(stderr, "Migd_GetParms called.\n");
    }

    /*
     * XXX need to add this here.
     */
    return(EINVAL);
}
    

/*
 *----------------------------------------------------------------------
 *
 * AlarmHandler --
 *
 *	Routine to service a SIGALRM signal for eviction.
 *	This routine disables
 *	the alarm (letting the caller reenable it when appropriate).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The alarm is disabled.
 *
 *----------------------------------------------------------------------
 */
static int
AlarmHandler()
{
    
    alarm(0);
    if (migd_Debug > 0) {
	SYSLOG0(LOG_INFO, "Eviction didn't finish in a timely fashion.");
    }
    (void) signal (SIGALRM, SIG_IGN);
}

/*
 *----------------------------------------------------------------------
 *
 * Migd_Evict --
 *
 *	Perform a system call to evict foreign processes, with an
 *	appropriate timeout to ensure we don't block forever.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forks a process to perform the eviction.  If the global
 *	migd_Verbose variable is set, information about each process
 *	evicted is printed on the syslog.  If notify is set, it does
 *	an ioctl to tell the global daemon; if not, then that means we're
 *	evicting on orders of the global daemon.
 *
 *----------------------------------------------------------------------
 */

void
Migd_Evict(notify)
    int notify;			/* Whether to notify global daemon  */
{
    ReturnStatus status;
    struct itimerval itimer;
#define NUM_PCBS 256
    Proc_PCBInfo infos[NUM_PCBS];
    register Proc_PCBInfo *infoPtr;
    int pcbsUsed;
    int i;
    int pid;
    Host_Entry *hostPtr;
    struct passwd *pwPtr;
    int state;
    int numEvicted = 0;


    if (migd_Debug > 1) {
	fprintf(stderr, "Migd_Evict -\n");
    }
    pid = fork();
    if (pid < 0) {
	fprintf(stderr, "Migd_Evict - couldn't fork\n");
	return;
    } else if (pid > 0) {
	return;
    }

    if (migd_Verbose) {
	/*
	 * Dump the entire process table into our memory.
	 */

	status = Proc_GetPCBInfo(0, NUM_PCBS-1, PROC_MY_HOSTID,
				 sizeof(Proc_PCBInfo),
				 infos, (Proc_PCBArgString *) NULL, &pcbsUsed);
	if (status != SUCCESS) {
	    fprintf(stderr, "Couldn't read process table: %s\n",
		   Stat_GetMsg(status));
	    exit(1);
	}

	for (i = 0, infoPtr = infos; i < pcbsUsed;
	     i++, infoPtr++) {
	    if ((infoPtr->genFlags & PROC_FOREIGN) &&
		!(infoPtr->genFlags & PROC_DONT_MIGRATE)) {
		switch (infoPtr->state) {
		    case PROC_RUNNING:
		    case PROC_READY:
		    case PROC_WAITING:
		    case PROC_SUSPENDED: {
			hostPtr = Host_ByID(infoPtr->peerHostID);
			pwPtr = getpwuid(infoPtr->userID);
			if (hostPtr != (Host_Entry *) NULL &&
			    pwPtr != (struct passwd *) NULL) {
			    syslog(LOG_INFO,
				   "Evicting process %x (%s@%s)",
				   infoPtr->peerProcessID, pwPtr->pw_name,
				   hostPtr->name);
			    numEvicted++;
			}
		    }
		    break;
		    default: {
			break;
		    }
		}
	    }
	}
	Host_End();
	endpwent();
    }
    /*
     * At this point, make the kernel call to get
     * all processes at this point in time.  They may be slightly
     * different from the ones we found using "ps"-like calls,
     * but that was just for information anyway -- this is the real
     * mccoy.  (For this reason, often there will be no processes evicted.)
     * We also set up a timer in case we block too long.
     */
    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = 0;
    itimer.it_value.tv_sec = EVICT_TIMEOUT;
    itimer.it_value.tv_usec = 0;

    if ((int) signal(SIGALRM, AlarmHandler) < 0) {
	syslog(LOG_ERR, "Error setting signal handler: %s.\n",
	       strerror(errno));
	exit(1);
    }
    if (setitimer(ITIMER_REAL, &itimer,
		  (struct itimerval *) NULL) == -1) {
	syslog(LOG_ERR, "Error setting interval timer: %s.\n",
	       strerror(errno));
	exit(1);
    }
    status = Proc_Migrate(PROC_ALL_PROCESSES, 0);
    if (status != SUCCESS && status != GEN_ABORTED_BY_SIGNAL) {
	syslog(LOG_ERR, "Error evicting foreign processes: %s\n",
	       Stat_GetMsg(status));
    } else if (migd_Verbose && numEvicted > 0) {
	syslog(LOG_INFO, "Evicted %d processes.\n", numEvicted);
    }
    (void) setitimer(ITIMER_REAL, &noTimer, (struct itimerval *) NULL);
    (void) signal(SIGALRM, SIG_IGN);

    if (notify) {
	/*
	 * Notify the global daemon that eviction has occurred.
	 */
	state = MIG_HOST_ACTIVE;
	status = Fs_IOControl(migdGlobalDesc, IOC_MIG_CHANGE, sizeof(int),
			      (char *) &state,
			      0, (char *) NULL);
	if (status != SUCCESS) {
	    if (migd_Debug > 0) {
		fprintf(stderr,
			"Error doing MIG_CHANGE ioctl with global daemon: %s.\n",
			Stat_GetMsg(status));
	    }
	}
    }


    exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * Migd_EvictIoctl --
 *
 *	Ioctl interface to Migd_Evict routine.  May be called by user
 *	program to force an eviction.
 *
 * Results:
 *	0 is returned as the value of the ioctl.  The number of foreign
 *	processes on the machine is returned in outBuffer.
 *
 * Side effects:
 *	Any foreign processes are evicted.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
int
Migd_EvictIoctl(cltPtr, command, inBuffer, inBufSize, outBuffer,
	       outBufSizePtr)
    Migd_OpenStreamInfo *cltPtr;/* Information about the client making
				   the request. */
    int command;		/* Ignored. */
    char *inBuffer;		/* Not used. */
    int inBufSize;		/* Not used. */
    char *outBuffer;		/* Buffer to place results. */
    int *outBufSizePtr;		/* Size of the output buffer. */
{
    int *intPtr;
    
    if (migd_Debug > 1) {
	fprintf(stderr, "Migd_EvictIoctl called.\n");
    }

    if (*outBufSizePtr < sizeof(int)) {
	if (migd_Debug > 0) {
	    SYSLOG2(LOG_WARNING,
		   "Migd_EvictIoctl: bad output buffer size (%d) from process %x\n",
		   *outBufSizePtr, cltPtr->processID);
	}
	return(EINVAL);
    }
    if (migd_GlobalMaster) {
	return(EINVAL);
    }

    intPtr = (int *) outBuffer;
    *intPtr = curVecPtr->foreignProcs;
    *outBufSizePtr = sizeof(int);
    
    Migd_Evict(TRUE);
    return(0);
}

	

/*
 *----------------------------------------------------------------------
 *
 * CheckMessages --
 *
 *	Check for messages from the global daemon.  
 *
 * Results:
 *	-1 if no messages or on error.  Otherwise returns the value of the
 *	message, which is an integer.
 *	Obtains at most one message per call.
 *
 * Side effects:
 *	May do ioctl to server.
 *
 *----------------------------------------------------------------------
 */
static int
CheckMessages()
{
    static int *bitArray = NULL;
    static int bitSize = 0;
    int numReady;
    int status;
    int msgHostID;
    struct timeval time;
    
    if (migd_Debug > 4) {
	fprintf(stderr, "CheckMessages called.\n");
    }

    if (migdGlobalDesc < 0) {
	if (migd_Debug > 4) {
	    fprintf(stderr, "CheckMessages: no pdev connection.\n");
	}
	return(-1);
    }
    if (bitSize <= migdGlobalDesc) {
	bitArray = Bit_Expand(migdGlobalDesc + 1, bitSize, bitArray);
	bitSize = migdGlobalDesc + 1;
    }
	    
    Bit_Set(migdGlobalDesc, bitArray);
    time.tv_sec = 0;
    time.tv_usec = 0;
    if (migd_Debug > 4) {
	fprintf(stderr, "CheckMessages: calling select.\n");
    }
    numReady = select(bitSize, bitArray, (int *) NULL, (int *) NULL,
		      &time);
    if (migd_Debug > 4) {
	fprintf(stderr, "CheckMessages: select returned %d.\n", numReady);
    }
    if (numReady <= 0) {
	return(-1);
    } else {
	if (migd_Debug > 4) {
	    fprintf(stderr, "CheckMessages: calling Fs_IOControl.\n");
	}
	if (MigSetAlarm() < 0) {
	    fprintf(stderr,
		    "Error setting alarm for contact with global migd.\n");
	    return(FALSE);
	}
	status = Fs_IOControl(migdGlobalDesc, IOC_MIG_GET_UPDATE,
			      0, (char *) NULL, sizeof(int),
			      (char *) &msgHostID);
	if (MigClearAlarm() < 0) {
	    fprintf(stderr,
		    "Error clearing alarm for contact with migd.\n");
	}
		
		
#ifdef DEBUG
	fprintf(stderr, "CheckMessages: Fs_IOControl returned %x.\n",
		status);
#endif				/* DEBUG */
	if (status != SUCCESS) {
	    if (migd_Debug >= 2) {
		fprintf(stderr,
			"CheckMessages: numReady = 0x%x, global desc=%d\n",
			numReady, migdGlobalDesc);
		fprintf(stderr,
			"  bitArray=0x%x\n",
			bitArray[0]); /* XXX what if size > 32? */
	    }
	    fprintf(stderr,
		    "CheckMessages: error during ioctl to global master: %s\n",
		    Stat_GetMsg(status));
	    return(-1);
	}
	return(msgHostID);
    }
}
