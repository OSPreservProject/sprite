/* 
 * lock.c--
 *
 *	Simple locking functions for Jaquith archive package.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Quote:
 *      "The goal of all inanimate objects is to resist man
 *      and ultimately defeat him."
 *      -- Russell Baker
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/lock.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <sys/file.h>
#include <errno.h>
#include "jaquith.h"

extern int jDebug;
extern int syserr;

static char printBuf[T_MAXSTRINGLEN];

#define LOCKSUFFIX ".LOCK"
#define LOCKFILES "*.LOCK"
#define MAXSLEEPSECS 120      /* arbitrary max timeout value */


/*
 *----------------------------------------------------------------------
 *
 * Lock_Acquire --
 *
 *	Get exclusive lock on file.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Lock file is created with name fileName.LOCK if flock unavailable
 *
 * Note:
 *      The flock routine is not used because it requires that the
 *      processes share a file descriptor.  If the sys admin intervenes,
 *      (restarts a cleaner process that died for example) there's
 *      no way for the newly created process to join the flock crowd.
 *
 *----------------------------------------------------------------------
 */

int
Lock_Acquire(fileName, blockFlag, handlePtr)
    char *fileName;           /* name of file to open */
    int blockFlag;            /* if lock is unavailable, return */
    Lock_Handle *handlePtr;   /* receiving structure */
{
    int retCode = T_FAILURE;
    static char pidChar[12];
    static int pidLen = 0;
    int mode = O_RDWR+O_CREAT+O_EXCL;
    int sleepSecs = 1;

    strcpy(handlePtr->lockName, fileName);
    strcat(handlePtr->lockName, LOCKSUFFIX);

    handlePtr->lockFd = open(handlePtr->lockName, mode, 0600);

    if ((handlePtr->lockFd == -1) && (blockFlag == LOCK_BLOCK)) {
	while (errno == EEXIST) {
	    sleep(sleepSecs);
	    if (sleepSecs < MAXSLEEPSECS) {
		sleepSecs *= 2;
	    }
	    if ((handlePtr->lockFd=open(handlePtr->lockName, mode, 0600))
		!= -1) {
		break;
	    }
	}
    }

    if (handlePtr->lockFd != -1) {
	retCode = T_SUCCESS;
	if (pidLen == 0) {
	    sprintf(pidChar, "0x%x\n\0", getpid());
	    pidLen = strlen(pidChar);
	}
	write(handlePtr->lockFd, pidChar, pidLen);
    }
/*
    if (jDebug) {
	fprintf(stderr,"Lock_Acquire: %s: retcode %d\n",
		handlePtr->lockName, retCode);
    }
*/
    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * Lock_Release --
 *
 *	Release exclusive lock on file
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	removes lock file, if not using flock.
 *
 *----------------------------------------------------------------------
 */

int
Lock_Release(handlePtr)
    Lock_Handle *handlePtr;   /* item to release */
{
/*
    if (jDebug) {
	fprintf(stderr,"Lock_Release: %s\n", handlePtr->lockName);
    }
*/
    if (close(handlePtr->lockFd) == -1) {
	syserr = errno;
	fprintf(stderr, "Lock_Release: close failed on %s. errno %d\n",
		handlePtr->lockName, syserr);
    }

    if (unlink(handlePtr->lockName) == -1) {
	syserr = errno;
	fprintf(stderr, "Lock_Release: unlink failed on %s. errno %d\n",
		handlePtr->lockName, syserr);
    }

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * Lock_RemoveAll --
 *
 *	Remove all lock files in directory.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Lock file is (conditionally) removed.
 *
 *----------------------------------------------------------------------
 */

void
Lock_RemoveAll(dirPath, confirmFlag)
    char *dirPath;            /* directory to clean */
    int confirmFlag;          /* 1 == request confirmation */
{
    DIR *dirPtr;
    DirObject *entryPtr;
    struct stat unixStatBuf;
    char pathName[T_MAXPATHLEN];
    char answer[T_MAXLINELEN];

    *answer = 'y';

    if ((dirPtr=(DIR *)opendir(dirPath)) == (DIR *) NULL) {
	return;
    }

    while ((entryPtr=readdir(dirPtr)) != (DirObject *)NULL) {
	if (Str_Match(entryPtr->d_name, LOCKFILES)) {
	    strcpy(pathName, dirPath);
	    strcat(pathName, "/");
	    strcat(pathName, entryPtr->d_name);
	    stat(pathName, &unixStatBuf);
	    if (S_ISREG(unixStatBuf.st_mode)) {
		if (confirmFlag) {
		    sprintf(printBuf,"Delete old lock file: %s ? ", pathName);
		    if (Utils_GetOk(printBuf)) {
			unlink(pathName);
		    }
		}
	    }
	}
    }

    closedir(dirPtr);
}

