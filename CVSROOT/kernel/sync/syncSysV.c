/* 
 * syncSysV.c --
 *
 *	Do system V compatibility synchronization functions:
 *		semctl, semget, semop
 *
 * Copyright (C) 1990 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "sync.h"
#include "sys/types.h"
#include "sys/ipc.h"
#include "sys/sem.h"
#include "spriteTime.h"
#include "fs.h"

Sync_SysVSem semTable[SEMMNI];
static int totalSems = 0;

Sync_Lock semLock;
#define LOCKPTR (&semLock)

/*
 * Debugging print statement declaration.
 */
#define dprintf if (vmShmDebug) printf
extern int vmShmDebug;

/*
 * Compat. defs.
 */
#define EPERM	GEN_EPERM
#define ENOENT	GEN_ENOENT
#define EINTR	GEN_EINTR
#define EACCES	GEN_EACCES
#define EEXIST	GEN_EEXIST
#define EINVAL	GEN_EINVAL
#define ENOSPC	GEN_ENOSPC

#define EFBIG	GEN_EFBIG
#define	E2BIG	GEN_E2BIG
#define EAGAIN	GEN_EAGAIN
#define EFAULT	GEN_EFAULT
#define ERANGE	GEN_ERANGE
#define EIDRM	GEN_EIDRM

#define DEFAULTGID 115

static int semInit = 0;

/*
 *----------------------------------------------------------------------
 *
 * Sync_SemInit --
 *
 *	Perform semaphore initialization functions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes datastructures.
 *
 *----------------------------------------------------------------------
 */
void
Sync_SemInit()
{
    int i;
    if (semInit == 1234) {
	return;
    }
    semInit = 1234;
    LOCK_MONITOR;
    for (i=0;i<SEMMNI;i++) {
	semTable[i].sem_base = (struct sem *)NIL;
    }
    UNLOCK_MONITOR;
}

/*
 *----------------------------------------------------------------------
 *
 * Sync_SemctlStub --
 *
 *	Perform control functions on semaphores.
 *
 * Results:
 *	Returns result of the function.
 *
 * Side effects:
 *	Performs a function on the semaphore.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Sync_SemctlStub(semid, semnum, cmd, arg, retValOut)
    int		semid;		/* Semaphore group identifier. */
    int		semnum;		/* Semaphore number. */
    int		cmd;		/* Command to perform. */
    union semun	arg;		/* Command argument. */
    int		*retValOut;	/* Result of command. */
{

    ReturnStatus	status;
    int			perm;
    Sync_SysVSem	*semPtr;
    int			uid;
    struct semid_ds	semBuf;
    int			i;
    int			nsems;
    struct sem		*curSem;
    ushort		array[SEMMSL];
    Time		timeVal;
    int			retVal=0;

    Sync_SemInit();
    LOCK_MONITOR;
    if (cmd != GETKEYS) {
	status = Sync_SemStruct(semid, &perm, &semPtr);
	if (status != SUCCESS) {
	    UNLOCK_MONITOR;
	    return status;
	}
    }
    switch (cmd) {

	case SETVAL:
	case SETALL:
	    if (!(perm & SEM_A)) {
		UNLOCK_MONITOR;
		dprintf("Access denied\n");
		return EACCES;
	    }
	    break;
	case IPC_SET:
	case IPC_RMID:
	    uid=Proc_GetEffectiveProc()->effectiveUserID;
	    if (uid != 0 && uid != semPtr->sem_perm.uid &&
		    uid != semPtr->sem_perm.cuid) {
		UNLOCK_MONITOR;
		dprintf("Permission denied\n");
		return EPERM;
	    }
	    break;
	case GETKEYS:
	    break;
	case GETVAL:
	case GETPID:
	case GETNCNT:
	case GETZCNT:
	case GETALL:
	case IPC_STAT:
	    if (!(perm & SEM_R)) {
		UNLOCK_MONITOR;
		dprintf("Access denied\n");
		return EACCES;
	    }
	    break;
	default:
	    UNLOCK_MONITOR;
	    dprintf("invalid command\n");
	    return EINVAL;
    }
	    
    if (cmd == GETVAL || cmd == SETVAL || cmd == GETPID || cmd == GETNCNT ||
	    cmd == GETZCNT) {
	if (semnum < 0 || semnum >= semPtr->sem_nsems) {
	    UNLOCK_MONITOR;
	    dprintf("Invalid semnum: %d\n",semnum);
	    return EINVAL;
	}
	curSem = &semPtr->sem_base[semnum];
    }
    switch (cmd) {
	case GETVAL:
	    retVal = curSem->semval;
	    break;
	case SETVAL:
	    if (arg.val < 0 || arg.val > SEMVMX) {
		UNLOCK_MONITOR;
		dprintf("Invalid value %d\n",arg.val);
		return ERANGE;
	    }
	    curSem->semval = arg.val;
	    if ( (arg.val == 0 && curSem->semzcnt > 0) ||
		    (arg.val > 0 && curSem->semncnt > 0)) {
		Sync_Broadcast(&curSem->semLock);
	    }
	    break;
	case GETPID:
	    retVal = curSem->sempid;
	    break;
	case GETNCNT:
	    retVal = curSem->semncnt;
	    break;
	case GETZCNT:
	    retVal = curSem->semzcnt;
	    break;
	case GETALL:
	    for (i=0;i<semPtr->sem_nsems;i++) {
		array[i] = semPtr->sem_base[i].semval;
	    }
	    status = Vm_CopyOut(sizeof(ushort)*semPtr->sem_nsems,
		    (Address) array, (Address) arg.array);
	    if (status != SUCCESS) {
		UNLOCK_MONITOR;
		dprintf("Copy-out fault\n");
		return EFAULT;
	    }
	    break;
	case GETKEYS:
	    bzero((Address)&semBuf, sizeof(struct semid_ds));
	    for (i=0;i<SEMMNI;i++) {
		semBuf.sem_perm.mode = semTable[i].sem_perm.mode;
		semBuf.sem_perm.key = semTable[i].sem_perm.key;
		semBuf.sem_perm.seq = semTable[i].sem_perm.seq;
		semBuf.sem_perm.uid = semTable[i].sem_perm.uid;
		semBuf.sem_perm.gid = semTable[i].sem_perm.gid;
		semBuf.sem_nsems =
			(semTable[i].sem_base == (struct sem *)NIL) ? 0 : 1;
		status = Vm_CopyOut(sizeof(struct semid_ds), (Address) &semBuf,
			(Address) (arg.buf+i));
		if (status != SUCCESS) {
		    UNLOCK_MONITOR;
		    dprintf("Copy-out fault\n");
		    return EFAULT;
		}
	    }
	    break;
	case IPC_STAT:
	    status = Vm_CopyOut(sizeof(struct semid_ds), (Address) semPtr,
		    (Address) arg.buf);
	    if (status != SUCCESS) {
		UNLOCK_MONITOR;
		dprintf("Copy-out fault\n");
		return EFAULT;
	    }
	    break;
	case SETALL:
	    status = Vm_CopyIn(sizeof(ushort)*semPtr->sem_nsems,
		    (Address) arg.array, (Address) array);
	    if (status != SUCCESS) {
		UNLOCK_MONITOR;
		dprintf("Copy-in fault\n");
		return EFAULT;
	    }

	    for (i=0;i<semPtr->sem_nsems;i++) {
		if (array[i] < 0 || array[i] > SEMVMX) {
		    UNLOCK_MONITOR;
		    dprintf("Invalid value: %d\n",array[i]);
		    return ERANGE;
		}
	    }
	    for (i=0;i<semPtr->sem_nsems;i++) {
		semPtr->sem_base[i].semval = array[i];
	    }
	    for (i=0;i<semPtr->sem_nsems;i++) {
		curSem = &semPtr->sem_base[i];
		if ( (array[i] == 0 && curSem->semzcnt > 0) ||
			(array[i] > 0 && curSem->semncnt > 0)) {
		    Sync_Broadcast(&curSem->semLock);
		}
	    }
	    break;
	case IPC_SET:
	    status = Vm_CopyIn(sizeof(struct semid_ds), (Address) arg.buf,
		    (Address) &semBuf);
	    if (status != SUCCESS) {
		UNLOCK_MONITOR;
		dprintf("Copy-in fault\n");
		return EFAULT;
	    }
	    semPtr->sem_perm.uid = semBuf.sem_perm.uid;
	    semPtr->sem_perm.gid = semBuf.sem_perm.gid;
	    semPtr->sem_perm.mode = semBuf.sem_perm.mode & 0666;
	    break;
	case IPC_RMID:
	    semPtr->sem_perm.seq++;
	    nsems = semPtr->sem_nsems;
	    if (nsems > SEMMNI) nsems = SEMMNI;
	    curSem = semPtr->sem_base;
	    semPtr->sem_base = (struct sem *)NIL;
	    for (i=0;i<nsems;i++) {
		if (curSem[i].semzcnt != 0 || curSem[i].semncnt != 0) {
		    Sync_Broadcast(&curSem->semLock);
		}
	    }
	    free(curSem);
	    break;
    }
    if (cmd == SETVAL || cmd == SETALL || cmd == IPC_SET) {
	Timer_GetRealTimeOfDay(&timeVal,NIL,NIL);
	semPtr->sem_ctime = timeVal.seconds;
    }
    UNLOCK_MONITOR;
    status = Vm_CopyOut(sizeof(int), (Address) &retVal, (Address) retValOut);
    if (status != SUCCESS) {
	dprintf("Copy-out fault\n");
	return EFAULT;
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Sync_SemgetStub --
 *
 *	Gets a set of semaphores.
 *
 * Results:
 *	Returns result of the function.
 *
 * Side effects:
 *	May create semaphore datastructures.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Sync_SemgetStub(key, nsems, semflg, retValOut)
    long	key;		/* Semaphore key. */
    int		nsems;		/* Number of semaphores requested. */
    int		semflg;		/* Creation flag. */
    int		*retValOut;	/* Result of operation. */
    
{
    int			i;
    Sync_SysVSem	*semPtr;
    long		searchKey;
    Proc_ControlBlock	*procPtr;
    int			uid;
    Time		timeVal;
    int			retVal;
    ReturnStatus	status;
    Fs_ProcessState	*fsPtr;

    Sync_SemInit();
    if (nsems < 0 || nsems > SEMMSL) {
	UNLOCK_MONITOR;
	dprintf("Invalid number of semaphores: %d\n",nsems);
	return EINVAL;
    }
    LOCK_MONITOR;
    if (key != IPC_PRIVATE) {
	semPtr = NULL;
	for (i=0;i<SEMMNI;i++) {
	    if ((long)semTable[i].sem_perm.key == key) {
		semPtr = &semTable[i];
		break;
	    }
	}
	if (semPtr == NULL && !(semflg & IPC_CREAT)) {
	    UNLOCK_MONITOR;
	    dprintf("No semaphore %d\n",key);
	    return ENOENT;
	}
    }
    if (key == IPC_PRIVATE || semPtr == NULL) {
	semPtr = NULL;
	for (i=0;i<SEMMNI;i++) {
	    if (semTable[i].sem_base == (struct sem *)NIL) {
		semPtr = &semTable[i];
		break;
	    }
	}
	if (semPtr == NULL) {
		UNLOCK_MONITOR;
		dprintf("Semaphore table full\n");
		return ENOSPC;
	}
	if (totalSems+nsems > SEMMNS) {
	    UNLOCK_MONITOR;
	    dprintf("Semaphore table full\n");
	    return ENOSPC;
	}
	procPtr=Proc_GetEffectiveProc();
	uid = procPtr->effectiveUserID;
	semPtr->sem_perm.uid = procPtr->effectiveUserID;
	semPtr->sem_perm.cuid = procPtr->effectiveUserID;
	fsPtr = procPtr->fsPtr;
	if (fsPtr->numGroupIDs > 0) {
	    semPtr->sem_perm.gid = fsPtr->groupIDs[0];
	} else {
	    semPtr->sem_perm.gid = DEFAULTGID;
	}
	semPtr->sem_perm.cgid = semPtr->sem_perm.gid;
	semPtr->sem_perm.mode = semflg&0666;
	semPtr->sem_perm.key = key;
	semPtr->sem_base = (struct sem *)malloc(sizeof(struct sem)*nsems);
	Timer_GetRealTimeOfDay(&timeVal,NIL,NIL);
	semPtr->sem_nsems = nsems;
	semPtr->sem_otime = 0;
	semPtr->sem_ctime = timeVal.seconds;
	for (i=0;i<nsems;i++) {
	    semPtr->sem_base[i].semval = 0;
	    semPtr->sem_base[i].sempid = 0;
	    semPtr->sem_base[i].semncnt = 0;
	    semPtr->sem_base[i].semzcnt = 0;
	}
    } else {
	if (semPtr->sem_nsems < nsems) {
	    UNLOCK_MONITOR;
	    dprintf("Not enough semaphores created\n");
	    return EINVAL;
	}
	if ((semflg & IPC_CREAT) && (semflg & IPC_EXCL)) {
	    UNLOCK_MONITOR;
	    dprintf("Semaphore set already exists\n");
	    return EEXIST;
	}
	if (((semflg & 0333) & semPtr->sem_perm.mode) != (semflg & 0333)) {
	    UNLOCK_MONITOR;
	    dprintf("Permission denied: mode %o\n",semPtr->sem_perm.mode);
	    return EACCES;
	}
    }
    retVal = semPtr->sem_perm.seq*SEMMNI+(semPtr-semTable);
    UNLOCK_MONITOR;
    status = Vm_CopyOut(sizeof(int), (Address) &retVal, (Address) retValOut);
    if (status != SUCCESS) {
	dprintf("Copy-out failure\n");
	return EFAULT;
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * Sync_SemopStub --
 *
 *	Perform semaphore operations on semaphores.
 *
 * Results:
 *	Returns result of the function.
 *
 * Side effects:
 *	Performs a function on the semaphore.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Sync_SemopStub(semid, sopsIn, nsops, retValOut)
    int			semid;		/* Semaphore group identifier. */
    struct sembuf	*sopsIn;	/* Operations to perform. */
    int			nsops;		/* Number of operations. */
    int			retValOut;	/* Result of operation. */
    
{
    int			perms;
    Sync_SysVSem	*semPtr;
    ReturnStatus	status;
    int			i,j;
    struct sembuf	sops[SEMOPM];
    int			seq;
    int			semmod[SEMMSL];
    Time		timeVal;
    int			pid = Proc_GetEffectiveProc()->processID;
    struct sem		*curSem;
    int			retVal;
    Boolean		sig;

    if (nsops <= 0 || nsops > SEMOPM) {
	dprintf("Too many semaphore operations\n");
	return E2BIG;
    }

    Sync_SemInit();

    status = Vm_CopyIn(sizeof(struct sembuf)*nsops, (Address) sopsIn,
	    (Address) sops);
    if (status != SUCCESS) {
	dprintf("Copy-in fault\n");
	return EFAULT;
    }
    LOCK_MONITOR;
    status = Sync_SemStruct(semid, &perms, &semPtr);
    if (status != SUCCESS) {
	UNLOCK_MONITOR;
	return status;
    }
    for (i=0;i<nsops;i++) {
	if (sops[i].sem_num < 0 || sops[i].sem_num >= semPtr->sem_nsems) {
	    UNLOCK_MONITOR;
	    dprintf("Semaphore value too big\n");
	    return EFBIG;
	}
	if ((sops[i].sem_op != 0 && !(perms & SEM_A)) ||
		(sops[i].sem_op == 0 && !(perms & SEM_R))) {
	    UNLOCK_MONITOR;
	    dprintf("Semaphore access denied\n");
	    return EACCES;
	}
	if (sops[i].sem_flg & SEM_UNDO) {
	    printf("semop: SEM_UNDO not implemented\n");
	    UNLOCK_MONITOR;
	    return EINVAL;
	}
    }

retry:

    status = SUCCESS;
    for (i=0;i<nsops;i++) {
	int sem_op = sops[i].sem_op;
	curSem = &semPtr->sem_base[sops[i].sem_num];
	retVal = curSem->semval;
	if (sem_op < 0) {
	    if (curSem->semval >= -sem_op) {
		curSem->semval -= -sem_op;
	    } else {
		break;
	    }
	} else if (sops[i].sem_op > 0) {
	    if (curSem->semval + sem_op > SEMVMX) {
		status = ERANGE;
		break;
	    }
	    curSem->semval += sem_op;
	} else {
	    if (curSem->semval != 0) {
		break;
	    }
	}
    }

    if (i==nsops) {
	/*
	 * Success.
	 */
	for (i=0;i<semPtr->sem_nsems;i++) {
	    semmod[i] = 0;
	}
	for (i=0;i<nsops;i++) {
	    if (sops[i].sem_num != 0) {
		semmod[sops[i].sem_num]++;
	    }
	}
	Timer_GetRealTimeOfDay(&timeVal,NIL,NIL);
	semPtr->sem_otime = timeVal.seconds;
	for (i=0;i<semPtr->sem_nsems;i++) {
	    if (semmod[i] != 0) {
		curSem = &semPtr->sem_base[i];
		curSem->sempid = pid;
		if ((curSem->semzcnt != 0 && curSem->semval == 0) ||
			(curSem->semncnt != 0 && curSem->semval != 0)) {
		    Sync_Broadcast(&curSem->semLock);
		}
	    }
	}
	UNLOCK_MONITOR;
	status = Vm_CopyOut(sizeof(int), (Address) &retVal,
		(Address) retValOut);
	if (status != SUCCESS) {
	    dprintf("Copy-out fault\n");
	    return EFAULT;
	}
	return SUCCESS;
    } else {
	/*
	 * Failure;
	 */
	/*
	 * Undo any changes.
	 */
	for (j=0;j<i;j++) {
	    semPtr->sem_base[sops[j].sem_num].semval -= sops[j].sem_op;
	}
	if (status != SUCCESS) {
	    UNLOCK_MONITOR;
	    return status;
	}
	/*
	 * Wait if necessary.  Then try again.
	 */
	if (!(sops[i].sem_flg & IPC_NOWAIT)) {
	    if (sops[i].sem_op == 0) {
		curSem->semzcnt++;
	    } else {
		curSem->semncnt++;
	    }
	    seq = semPtr->sem_perm.seq;
	    sig = Sync_Wait(&curSem->semLock, TRUE);
	    if (seq != semPtr->sem_perm.seq) {
		UNLOCK_MONITOR;
		dprintf("Semaphore removed\n");
		return EIDRM;
	    }
	    if (sops[i].sem_op == 0) {
		curSem->semzcnt--;
	    } else {
		curSem->semncnt--;
	    }
	    if (sig) {
		UNLOCK_MONITOR;
		dprintf("Semaphore interrupted\n");
		return EINTR;
	    } else {
		goto retry;
	    }
	} else {
	    UNLOCK_MONITOR;
	    dprintf("Semaphore would block\n");
	    return EAGAIN;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Sync_SemStruct --
 *
 *	Get the semaphore structure, given the identifier.
 *	This routine checks the associated permissions.
 *
 * Results:
 *	Returns SUCCESS or error condition.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Sync_SemStruct(id, perm, retPtr)
    int			id;		/* Semaphore id value. */
    int			*perm;		/* (Return) permissions. */
    Sync_SysVSem	**retPtr;	/* (Return) Semaphore structure. */

{
    Sync_SysVSem	*semPtr;
    Proc_ControlBlock	*procPtr;
    int uid, gid;
    int uperm;
    Fs_ProcessState	*fsPtr;

    if (id < 0) return EINVAL;

    semPtr = &semTable[id%SEMMNI];
    *retPtr = semPtr;
    if (semPtr->sem_perm.seq != id/SEMMNI ||
	    semPtr->sem_base == (struct sem *) NIL ) {
	/*
	 * No such semaphore.
	 */
	dprintf("No semaphore %d\n",id);
	return EINVAL;
    }
    /*
     * Check permissions.
     */
    procPtr=Proc_GetEffectiveProc();
    uid = procPtr->effectiveUserID;
    fsPtr = procPtr->fsPtr;
    if (fsPtr->numGroupIDs > 0) {
	gid = fsPtr->groupIDs[0];
    } else {
	gid = DEFAULTGID;
	printf("Warning: process has no gid\n");
    }
    if (uid == 0) {
	*perm = SEM_A | SEM_R;
	return SUCCESS;
    }
    if (uid == semPtr->sem_perm.uid || uid == semPtr->sem_perm.cuid) {
	uperm = semPtr->sem_perm.mode;
    } else if (gid == semPtr->sem_perm.gid || gid == semPtr->sem_perm.cgid) {
	uperm = semPtr->sem_perm.mode<<3;
    } else {
	uperm = semPtr->sem_perm.mode<<6;
    }
    *perm = uperm & (SEM_A | SEM_R);
    return SUCCESS;
}
