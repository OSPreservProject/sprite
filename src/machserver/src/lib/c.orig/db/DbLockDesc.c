/* 
 * DbLockDesc.c --
 *
 *	Source code for the DbLockDesc procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/db/RCS/DbLockDesc.c,v 1.7 89/07/31 17:42:10 douglis Exp $ SPRITE (Berkeley)";
#endif not lint


#include <db.h>
#include "dbInt.h"
#include <sys/signal.h>
#include <sys/time.h>
#include <stdio.h>

#ifndef CLEAN
int db_Contention = 0;
#endif /* CLEAN */

/*
 * Set the default timeout, in seconds.
 */
#ifndef DB_LOCK_TIMEOUT
#define DB_LOCK_TIMEOUT 10
#endif /* DB_LOCK_TIMEOUT */

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
 *	The alarm is disabled.
 *
 *----------------------------------------------------------------------
 */
static int
AlarmHandler()
{
    
    alarm(0);
#ifdef DEBUG
    syslog(LOG_INFO, "Database lock timed out.");
#endif /* DEBUG */
    (void) signal (SIGALRM, SIG_IGN);
}


/*
 *----------------------------------------------------------------------
 *
 * DbLockDesc --
 *
 *	Lock a file descriptor, polling if necessary, and potentially
 *	breaking the lock if timeout occurs.
 *
 * Results:
 *	If the file is locked, by hook or by crook,
 *	0 is returned.  If the file is not successfully locked,
 *	-1 indicates an error and errno gives a more specific error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
DbLockDesc(handlePtr)
    Db_Handle *handlePtr;
{
    int streamID;			/* file descriptor */
    int type;				/* type of lock */
    Db_LockHow action;		/* DB_LOCK_{POLL,BLOCK,NO_BLOCK,WAIT,NONE} */
    int status;
    struct itimerval itimer, oldItimer;
    int (*oldHandler) ();
    int error;
    static int syslogDone = 0;


    streamID = handlePtr->streamID;
    action = handlePtr->lockHow;
    type = handlePtr->lockType;
    
    if (action == DB_LOCK_NONE) {
	return(0);
    }
    if (action != DB_LOCK_WAIT) {
	type |= LOCK_NB;
    }
    /*
     * Try the lock, then set a timer and try again.
     *
     * If the lock call times out in the RPC, return SUCCESS to keep the caller
     * from aborting.
     */
    
    status = flock(streamID, type);
    if (status == -1) {
	/*
	 * It is possible to get a sprite RPC_TIMEOUT condition, or other
	 * obscure error, which will map to EINVAL by default.  We're mostly
	 * interested in the EWOULDBLOCK condition, so if we get something
	 * we know is likely to be innocuous, return 0.  In particular,
	 * we don't want our caller to abort because the lock timed out, since
	 * it can wait for recovery.
	 */
	if (errno == EINVAL) {
	    return(0);
	} 
	if ((errno != EWOULDBLOCK) || (action == DB_LOCK_NO_BLOCK)) {
	    return(-1);
	}
    } else {
	return(0);
    }

    /*
     * Now lock in blocking mode, with an interval timer around to wake
     * us up if need be.
     */
#ifndef CLEAN
    db_Contention++;
#endif

    type &= ~LOCK_NB;
    itimer.it_interval.tv_sec = 0;
    itimer.it_interval.tv_usec = 0;
    itimer.it_value.tv_sec = DB_LOCK_TIMEOUT;
    itimer.it_value.tv_usec = 0;

    oldHandler = (int (*)()) signal(SIGALRM, AlarmHandler);
    if (oldHandler == (int (*)()) -1) {
	syslog(LOG_ERR, "Error setting signal handler.");
	return(-1);
    }
    if (setitimer(ITIMER_REAL, &itimer, &oldItimer) == -1) {
	error = errno;
	(void) signal(SIGALRM, oldHandler);
	errno = error;
	return(-1);
    }
#ifdef LOCK_DEBUG
    syslog(LOG_INFO, "Debug msg: DB doing blocking lock with timeout.");
#endif

    status = flock(streamID, type);

#ifdef LOCK_DEBUG
    syslog(LOG_INFO, "Debug msg: blocking lock with timeout returned %d, errno %d.", status, errno);
#endif

    error = errno;
    if (setitimer(ITIMER_REAL, &oldItimer, (struct itimerval *) NULL) == -1) {
	return(-1);
    }
    (void) signal(SIGALRM, oldHandler);
    if (status == -1) {
	errno = error;
	if (errno == EINVAL || errno == 0) {
	    return(0);
	}
	if (errno != EINTR) {
	    return(status);
	}
	if (action == DB_LOCK_POLL) {
	    errno = ETIMEDOUT;
	    return(-1);
	}
#ifndef CLEAN
	if (!syslogDone) {
	    syslog(LOG_INFO, "DbLockDesc: lock timed out (file %s).\n",
		   handlePtr->fileName);
	    syslogDone = 1;
	}
#endif /* CLEAN */

	/*
	 * Break the lock if required.  Try to break an exclusive lock.  If
	 * that fails, and we were trying to get an exclusive lock, then try
	 * releasing a shared lock since that could have been the problem.
	 * Then try to get the lock again.
	 * Return EWOULDBLOCK if breaking the lock wasn't successful.
	 *
	 * This has been commented out because one may no longer break
	 * locks on behalf of other processes.
	 */
	if (action == DB_LOCK_BREAK) {
	    status = flock(streamID, LOCK_EX | LOCK_NB | LOCK_UN);
	    if ((status != 0) && (type & LOCK_EX)) {
		(void) flock(streamID, LOCK_SH | LOCK_NB | LOCK_UN);
	    }
	}
	status = flock(streamID, type | LOCK_NB);
	if (status == 0) {
	    return(0);
	} else if (errno != EWOULDBLOCK) {
	    return(status);
	} else {
#ifdef CLEAN
	    syslog(LOG_WARNING, "DbLockDesc: unable to lock stream %d in %s mode",
		   streamID, (type & LOCK_EX) ? "exclusive" : "shared");
#else /* CLEAN */
	    syslog(LOG_WARNING, "DbLockDesc: unable to lock file %s in %s mode",
		   handlePtr->fileName,
		   (type & LOCK_EX) ? "exclusive" : "shared");
#endif /* CLEAN */
	}

/*
 * Once this thing is really production quality, it should return an error
 * if the lock times out.  For now, especially as long as processes can go
 * into the debugger with a lock held, just return success anyway.  We
 * tried.
 */

#ifdef INSTALLED
	errno = EWOULDBLOCK;
	return (-1);
#else /* INSTALLED */
	return(0);
#endif /* INSTALLED */
    }
    return(0);
}


