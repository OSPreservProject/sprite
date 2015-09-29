/* 
 * semop.c --
 *
 *	Procedures to map from Unix semaphore system calls to Sprite.
 *
 * Copyright 1989 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/lib/c/unixSyscall/RCS/mmap.c,v 1.1 90/01/11 14:34:37 shirriff Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "compatInt.h"


/*
 *----------------------------------------------------------------------
 *
 * semop --
 *
 *	Procedure to map from Unix semop system call to Sprite Sync_Semop.
 *
 * Results:
 *	Returns semaphore value.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	May modify a semaphore.
 *
 *----------------------------------------------------------------------
 */

int
semop(semid, sops, nsops)
    int			semid;	/* Semaphore specifier. */
    struct sembuf	*sops;	/* Array of semaphore operations. */
    int			nsops;	/* Number of semaphore operations.  */
{
    ReturnStatus status;	/* Result returned by Sync_Semop. */
    int semVal;			/* Returned semaphore value. */

    status = Sync_Semop(semid, sops, nsops, &semVal);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (semVal);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * semget --
 *
 *	Procedure to map from Unix semget system call to Sprite Sync_Semget.
 *
 * Results:
 *	Returns semaphore identifier.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	May create semaphore data structures.
 *
 *----------------------------------------------------------------------
 */

int
semget(key, nsems, semflag)
    key_t	key;		/* Semaphore key. */
    int 	nsems;		/* Number of semaphores. */
    int		semflag;	/* Creation flag.  */
{
    ReturnStatus status;	/* Result returned by Sync_Semget. */
    int semID;			/* Identifier returned by Sync_Semget. */

    status = Sync_Semget((long)key, nsems, semflag, &semID);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (semID);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * semctl --
 *
 *	Procedure to map from Unix semctl system call to Sprite Sync_Semctl.
 *
 * Results:
 *	Returns function-dependent value.
 *	Returns -1 if an error occurs.
 *
 * Side effects:
 *	May modify semaphores.
 *
 *----------------------------------------------------------------------
 */

int
semctl(semid, semnum, cmd, arg)
    int			semid;		/* Semaphore specifier. */
    int			semnum;		/* Semaphore number. */
    int			cmd;		/* Semaphore command. */
    union semun		arg;		/* Command argument. */
{
    int		result;		/* Result returned by Sync_Semctl. */
    ReturnStatus status;	/* Status returned by Sync_Semctl. */

    status = Sync_Semctl(semid, semnum, cmd, arg, &result);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	return(-1);
    } else {
	return (result);
    }
}
