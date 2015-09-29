/* 
 * procEnviron.c --
 *
 *	Routines to manage a process's environment.  The routines in
 *	this file manage a monitor for the environments.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/sprited/proc/RCS/procEnviron.c,v 1.5 91/12/01 21:59:32 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <ckalloc.h>
#include <status.h>
#include <string.h>

#include <proc.h>
#include <procInt.h>
#include <sync.h>
#include <sys.h>
#include <vm.h>

/*
 * The minimum size of the environment that is allocated.  This size
 * represents the number of environment variables.
 */

#define	MIN_ENVIRON_SIZE	5

/*
 * Structure to describe an internal version of an environment variable.
 */

typedef struct ProcEnvironVar {
    char	*name;
    int		nameLength;
    char	*value;
    int		valueLength;
} ProcEnvironVar;

/*
 * Monitor declarations.
 */

static	Sync_Lock environMonitorLock = 
    Sync_LockInitStatic("Proc:environMonitorLock");
#define	LOCKPTR   &environMonitorLock

static	void	DoCopyEnviron _ARGS_((int srcSize, 
			ProcEnvironVar *srcVarPtr, int destSize, 
			ProcEnvironVar *destVarPtr));
static	void	FreeEnviron _ARGS_((register Proc_EnvironInfo *environPtr,
			int size, Boolean freeEnvironInfo));
static	void	DecEnvironRefCount _ARGS_((Proc_EnvironInfo *environPtr));


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_InitMainEnviron --
 *
 *	Allocate an environment for the main process.  This sets things up
 *	so that we don't have to worry about NIL environments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Environment allocated for main process.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
ProcInitMainEnviron(procPtr)
    register	Proc_ControlBlock	*procPtr;	/* Pointer to main's
							 * PCB. */
{
    int		i;

    LOCK_MONITOR;

    procPtr->environPtr =
		(Proc_EnvironInfo *) ckalloc(sizeof(Proc_EnvironInfo));
    procPtr->environPtr->refCount = 1;
    procPtr->environPtr->size = MIN_ENVIRON_SIZE;
    procPtr->environPtr->varArray = (ProcEnvironVar *) 
		ckalloc(sizeof(ProcEnvironVar) * MIN_ENVIRON_SIZE);
    for (i = 0; i < MIN_ENVIRON_SIZE; i++) {
	procPtr->environPtr->varArray[i].name = (char *) NIL;
	procPtr->environPtr->varArray[i].value = (char *) NIL;
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcSetupEnviron --
 *
 *	Give this process a pointer to the parents environment and increment
 *	the reference count.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	environPtr field of process is set and reference count incremented
 *	in the environment.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
ProcSetupEnviron(procPtr)
    register	Proc_ControlBlock	*procPtr;	/* Process to setup
							 * environment for. */
{
    Proc_ControlBlock	*parentProc;

    LOCK_MONITOR;

    parentProc = Proc_GetPCB(procPtr->parentID);
    procPtr->environPtr = parentProc->environPtr;
    procPtr->environPtr->refCount++;

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * DoCopyEnviron --
 *
 *	Copy the source environment into the destination.  It is assumed that
 *	the destination is at least as big as the source.  Any remaining
 *	variables in the destination environment are set to NIL.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destination gets copy of source environment with any left over
 *	entries in the destination set to NIL.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL static void
DoCopyEnviron(srcSize, srcVarPtr, destSize, destVarPtr)
    int					srcSize;	/* # of variables in
							 * source environ. */
    register	ProcEnvironVar	*srcVarPtr;	/* Pointer to source
							 * environ variables.*/
    int					destSize;	/* # of variables in
							 * dest environment. */
    register	ProcEnvironVar	*destVarPtr;	/* Pointer to dest
							 * environ variables. */
{
    int		i;

    for (i = 0; i < srcSize; srcVarPtr++, destVarPtr++, i++) {
	if (srcVarPtr->name != (char *) NIL) {
	    destVarPtr->name = 
		(char *)ckalloc((unsigned)srcVarPtr->nameLength + 1);
	    destVarPtr->nameLength = srcVarPtr->nameLength;
	    destVarPtr->value =
		(char *)ckalloc((unsigned)srcVarPtr->valueLength + 1);
	    destVarPtr->valueLength = srcVarPtr->valueLength;
	    bcopy((Address) srcVarPtr->name, (Address) destVarPtr->name,
				srcVarPtr->nameLength + 1);
	    bcopy((Address) srcVarPtr->value, (Address) destVarPtr->value,
				srcVarPtr->valueLength + 1);
	} else {
	    destVarPtr->name = (char *) NIL;
	    destVarPtr->value = (char *) NIL;
	}
    }

    for (; i < destSize; i++, destVarPtr++) {
	destVarPtr->name = (char *) NIL;
	destVarPtr->value = (char *) NIL;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * FreeEnviron --
 *
 *	Free up all allocated memory in the environment.  The environment
 *	structure itself is not freed up unless the free flag is set.
 *	The actual number of variables in the environment to free up is given.
 *	This allows the caller to free up an environment that has not 
 *	been totally initialized.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All memory allocated for the given environment is freed.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL static void
FreeEnviron(environPtr, size, freeEnvironInfo)
    register	Proc_EnvironInfo	*environPtr;	/* Environment 
							 * to free. */
    int					size;		/* Number of variables
						   	 * in the environ. */
    Boolean				freeEnvironInfo;/* TRUE if should free
							 * environ struct .*/
{
    int					i;
    register	ProcEnvironVar	*varPtr;

    for (i = 0, varPtr = environPtr->varArray; i < size; varPtr++, i++) {
	if (varPtr->name != (char *) NIL) {
	    ckfree((Address) varPtr->name);
	    ckfree((Address) varPtr->value);
	}
    }
    ckfree((Address) environPtr->varArray);

    if (freeEnvironInfo) {
	ckfree((Address) environPtr);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * DecEnvironRefCount --
 *
 *	Decrement the reference count on the given environment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reference count decremented for the environment.
 *
 * ----------------------------------------------------------------------------
 */

static INTERNAL void
DecEnvironRefCount(environPtr)
    register	Proc_EnvironInfo	*environPtr;
{
    environPtr->refCount--;
    if (environPtr->refCount == 0) {
	FreeEnviron(environPtr, environPtr->size, TRUE);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * ProcDecEnvironRefCount --
 *
 *	Decrement the reference count on the given environment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reference count decremented for the environment.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY void
ProcDecEnvironRefCount(environPtr)
    register	Proc_EnvironInfo	*environPtr;
{
    LOCK_MONITOR;

    DecEnvironRefCount(environPtr);

    UNLOCK_MONITOR;
}


#if 0
/*
 * ----------------------------------------------------------------------------
 *
 * Proc_GetEnvironRangeStub --
 *
 *	Return as many environment variables as possible in the given range.  
 *	Variables are numbered from 0.  The actual number of environment 
 *	variables returned is returned in numActualVarsPtr.  The null 
 *	string is returned for any environment variables that are not set.
 *
 * Results:
 *	Error status if some error occcurs.  SUCCESS otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY ReturnStatus
Proc_GetEnvironRangeStub(first, last, envArray, numActualVarsPtr)
    int				first;			/* First var to 
							 * retrieve. */
    int				last;			/* Last var to 
							 * retrieve. */
    register	Proc_EnvironVar	*envArray;		/* Where to 
							 * store vars.*/
    int				*numActualVarsPtr;	/* Number of vars
							 * retrieved. */
{
    Proc_EnvironVar			*saveEnvPtr;
    register	ProcEnvironVar	*varPtr;
    register	Proc_ControlBlock	*procPtr;
    ReturnStatus			status = SUCCESS;
    int					i;
    int					numBytes;
    int					numVars;
    char				nullChar = '\0';

    LOCK_MONITOR;

    if (last < first || first < 0) {
	UNLOCK_MONITOR;
	return(SYS_INVALID_ARG);
    }

    procPtr = Proc_GetEffectiveProc();

    if (first >= procPtr->environPtr->size) {
	numVars = 0;
	status = Vm_CopyOut(sizeof(numVars), 
			(Address) &numVars, (Address) numActualVarsPtr);
	if (status != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	}
	UNLOCK_MONITOR;
	return(status);
    }
    if (last >= procPtr->environPtr->size) {
	last = procPtr->environPtr->size - 1;
    }

    /*
     * Make the environment array accessible.
     */

    i = (last - first + 1) * sizeof(Proc_EnvironVar);
    Vm_MakeAccessible(VM_READONLY_ACCESS, i,
	    (Address) envArray, &numBytes, (Address *) &saveEnvPtr);
    envArray = saveEnvPtr;
    if (numBytes != i) {
	if (envArray != (Proc_EnvironVar *) NIL) {
	    Vm_MakeUnaccessible((Address) envArray, numBytes);
	}
	UNLOCK_MONITOR;
	return(SYS_ARG_NOACCESS);
    }

    /*
     * Copy out the environment variables.
     */
    for (i = first, varPtr = &(procPtr->environPtr->varArray[first]);
	 i <= last;
	 i++, varPtr++, envArray++) {

	if (varPtr->name == (char *) NIL) {
	    if (Vm_CopyOut(1, (Address) &nullChar, 
			   (Address) envArray->name) != SUCCESS) {
		status = SYS_ARG_NOACCESS;
		break;
	    } else {
		continue;
	    }
	}

	if (Vm_CopyOut(varPtr->nameLength + 1, (Address) varPtr->name, 
		(Address) envArray->name) != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	    break;
	}

	if (Vm_CopyOut(varPtr->valueLength + 1, (Address) varPtr->value, 
		(Address) envArray->value) != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	    break;
	}
    }

    Vm_MakeUnaccessible((Address) saveEnvPtr, numBytes);

    if (status == SUCCESS) {
	numVars = last - first + 1;
	if (Vm_CopyOut(sizeof(numVars), (Address) &numVars, 
		       (Address) numActualVarsPtr) != SUCCESS) {
	    status = SYS_ARG_NOACCESS;
	}
    }

    UNLOCK_MONITOR;
    return(status);
}
#endif /* 0 */


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_CopyEnvironStub --
 *
 *	Give the current process its own copy of the environment.
 *
 * Results:
 *	Always returns SUCCESS.
 *
 * Side effects:
 *	New environment allocated.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY ReturnStatus
Proc_CopyEnvironStub()
{
    register	Proc_EnvironInfo	*newEnvironPtr;
    register	Proc_ControlBlock	*procPtr;

    LOCK_MONITOR;

    procPtr = Proc_GetEffectiveProc();

    if (procPtr->environPtr->refCount == 1) {
	UNLOCK_MONITOR;
	return(SUCCESS);
    }

    newEnvironPtr = (Proc_EnvironInfo *) ckalloc(sizeof(Proc_EnvironInfo));
    newEnvironPtr->refCount = 1;
    newEnvironPtr->size = procPtr->environPtr->size;
    newEnvironPtr->varArray = (ProcEnvironVar *) 
	  ckalloc(sizeof(ProcEnvironVar) * (unsigned)newEnvironPtr->size);

    DoCopyEnviron(newEnvironPtr->size, procPtr->environPtr->varArray, 
		  newEnvironPtr->size, newEnvironPtr->varArray);

    DecEnvironRefCount(procPtr->environPtr);
    procPtr->environPtr = newEnvironPtr;

    UNLOCK_MONITOR;

    return(SUCCESS);
}
