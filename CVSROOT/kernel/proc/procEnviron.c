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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "proc.h"
#include "procInt.h"
#include "sync.h"
#include "sched.h"
#include "sys.h"
#include "mem.h"
#include "status.h"
#include "vm.h"

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

static	Sync_Lock environMonitorLock = SYNC_LOCK_INIT_STATIC();
#define	LOCKPTR   &environMonitorLock


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
		(Proc_EnvironInfo *) malloc(sizeof(Proc_EnvironInfo));
    procPtr->environPtr->refCount = 1;
    procPtr->environPtr->size = MIN_ENVIRON_SIZE;
    procPtr->environPtr->varArray = (ProcEnvironVar *) 
		malloc(sizeof(ProcEnvironVar) * MIN_ENVIRON_SIZE);
    for (i = 0; i < MIN_ENVIRON_SIZE; i++) {
	procPtr->environPtr->varArray[i].name = (char *) NIL;
	procPtr->environPtr->varArray[i].value = (char *) NIL;
    }

    UNLOCK_MONITOR;
}


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_SetupEnviron --
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
	    destVarPtr->name = (char *) malloc(srcVarPtr->nameLength + 1);
	    destVarPtr->nameLength = srcVarPtr->nameLength;
	    destVarPtr->value = (char *) malloc(srcVarPtr->valueLength + 1);
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
	    free((Address) varPtr->name);
	    free((Address) varPtr->value);
	}
    }
    free((Address) environPtr->varArray);

    if (freeEnvironInfo) {
	free((Address) environPtr);
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * FindVar --
 *
 *	Search the given environment for a variable with the given name.
 *	This assumes that the given name is null terminated.
 *
 * Results:
 *	A pointer to the environment variable if found, NULL otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL static ProcEnvironVar *
FindVar(environPtr, name)
    Proc_EnvironInfo	*environPtr;	/* Environment to search. */
    char		*name;		/* Name to search for. */
{
    register	ProcEnvironVar	*varPtr;
    int					i;

    for (i = 0, varPtr = environPtr->varArray; 
	 i < environPtr->size; 
	 varPtr++, i++) {
	if (varPtr->name != (char *) NIL && 
	    strcmp(varPtr->name, name) == 0) {
	    return(varPtr);
	}
    }

    return((ProcEnvironVar *) NIL);
}


/*
 * ----------------------------------------------------------------------------
 *
 * InitializeVar --
 *
 *	Put the given name for the environment variable into the given 
 *	environment.  The environment will be expanded if necessary.
 *
 * Results:
 *	A pointer to the initialized environment variable.
 *
 * Side effects:
 *	Environment may be expanded.
 *
 * ----------------------------------------------------------------------------
 */

INTERNAL static ProcEnvironVar *
InitializeVar(environPtr, name, nameLength)
    register Proc_EnvironInfo *environPtr;	/* Environment to put
					       	 * the variable in. */
    char		      *name;	    	/* Name of variable. */
    int			      nameLength;   	/* Length of null terminated 
						 * name not including null 
						 * character. */
{
    register	ProcEnvironVar	*varPtr;
    int					i;
    int					newSize;

    for (i = 0, varPtr = environPtr->varArray; 
	 i < environPtr->size; 
	 varPtr++, i++) {
	if (varPtr->name == (char *) NIL) {
	    break;
	}
    }

    /*
     * If there was no empty space in the environment then
     * make the environment twice as big and set varPtr to point to
     * the first null entry in the environment.
     */

    if (i == environPtr->size) {
	if (environPtr->size == PROC_MAX_ENVIRON_SIZE) {
	    return((ProcEnvironVar *) NIL);
	}
	newSize = environPtr->size * 2;
	if (newSize > PROC_MAX_ENVIRON_SIZE) {
	    newSize = PROC_MAX_ENVIRON_SIZE;
	}
	varPtr = (ProcEnvironVar *) 
			malloc(sizeof(ProcEnvironVar) * newSize);
	DoCopyEnviron(environPtr->size, environPtr->varArray, 
		      newSize, varPtr);
	FreeEnviron(environPtr, environPtr->size, FALSE);
	environPtr->varArray = varPtr;
	varPtr += environPtr->size;
	environPtr->size = newSize;
    }

    /*
     * Store the name in the environment variable.
     */

    varPtr->name = (char *) malloc(nameLength + 1);
    varPtr->nameLength = nameLength;
    bcopy((Address) name, (Address) varPtr->name, nameLength + 1);

    return(varPtr);
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

INTERNAL void
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


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_SetEnvironStub --
 *
 *	Add the given environment variable to the current process's 
 *	environment.
 *
 * Results:
 *	Return SYS_ARG_NOACCESS if the name or value are inaccessible.
 *
 * Side effects:
 *	The enviroment of the current process is modified.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY ReturnStatus
Proc_SetEnvironStub(environVar)
    Proc_EnvironVar	environVar;	/* Variable to add to environment. */
{
    register	ProcEnvironVar	*varPtr;
    register	Proc_ControlBlock	*procPtr;
    char				*namePtr;
    char				*valuePtr;
    int					nameLength;
    int					valueLength;
    int					nameAccLength;
    int					valueAccLength;
    ReturnStatus			status;

    LOCK_MONITOR;

    procPtr = Proc_GetEffectiveProc();

    /*
     * Make variable name accessible.
     */

    namePtr = environVar.name;
    status = Proc_MakeStringAccessible(PROC_MAX_ENVIRON_NAME_LENGTH,
				       &namePtr, &nameAccLength, &nameLength);
    if (status != SUCCESS) {
	UNLOCK_MONITOR;
	return(status);
    }

    /*
     * Make variable value accessible.
     */

    valuePtr = environVar.value;
    status = Proc_MakeStringAccessible(PROC_MAX_ENVIRON_VALUE_LENGTH,
				       &valuePtr, &valueAccLength, 
				       &valueLength);
    if (status != SUCCESS) {
	Proc_MakeUnaccessible((Address) namePtr, nameAccLength);
	UNLOCK_MONITOR;
	return(status);
    }

    /*
     * See if the variable already exists.  If not then put it in the
     * environment.
     */

    varPtr = FindVar(procPtr->environPtr, namePtr);
    if (varPtr == (ProcEnvironVar *) NIL) {
	varPtr = InitializeVar(procPtr->environPtr, namePtr, nameLength);
	if (varPtr == (ProcEnvironVar *) NIL) {
	    Proc_MakeUnaccessible((Address) namePtr, nameAccLength);
	    Proc_MakeUnaccessible((Address) valuePtr, valueAccLength);
	    UNLOCK_MONITOR;
	    return(PROC_ENVIRON_FULL);
	}
    }
    Proc_MakeUnaccessible((Address) namePtr, nameAccLength);

    /*
     * Put the value of the variable into the environment.
     */

    varPtr->value = (char *) malloc(valueLength + 1);
    varPtr->valueLength = valueLength;
    bcopy((Address) valuePtr, (Address) varPtr->value, valueLength + 1);

    Proc_MakeUnaccessible((Address) valuePtr, valueAccLength);

    UNLOCK_MONITOR;
    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_UnsetEnvironStub --
 *
 *	Remove the given environment variable from the current process's 
 *	environment.
 *
 * Results:
 *	Return PROC_NOT_SET_ENVIRON_VAR if the variable is not set and
 *	SYS_ARG_NOACCESS if the name is not accessible.
 *
 * Side effects:
 *	The enviroment of the current process is modified.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY ReturnStatus
Proc_UnsetEnvironStub(environVar)
    Proc_EnvironVar	environVar;	/* Variable to remove. */
{
    register	ProcEnvironVar	*varPtr;
    register	Proc_ControlBlock	*procPtr;
    char				*namePtr;
    int					nameLength;
    int					nameAccLength;
    ReturnStatus			status;

    LOCK_MONITOR;

    procPtr = Proc_GetEffectiveProc();

    /*
     * Make variable name accessible.
     */

    namePtr = environVar.name;
    status = Proc_MakeStringAccessible(PROC_MAX_ENVIRON_NAME_LENGTH,
				       &namePtr, &nameAccLength, 
				       &nameLength);
    if (status != SUCCESS) {
	UNLOCK_MONITOR;
	return(status);
    }

    /*
     * Find the variable in the environment.
     */

    varPtr = FindVar(procPtr->environPtr, namePtr);
    Proc_MakeUnaccessible((Address) namePtr, nameAccLength);

    if (varPtr == (ProcEnvironVar *) NIL) {
	UNLOCK_MONITOR;
	return(PROC_NOT_SET_ENVIRON_VAR);
    }

    /*
     * Unset the variable by freeing up the space that was allocated
     * for it.
     */

    free((Address) varPtr->name);
    varPtr->name = (char *) NIL;
    free((Address) varPtr->value);
    varPtr->value = (char *) NIL;

    UNLOCK_MONITOR;

    return(SUCCESS);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Proc_GetEnvironVarStub --
 *
 *	Return the value of the given environment variable in the current 
 *	process's environment.
 *
 * Results:
 *	SYS_ARG_NOACCESS if place to store value is a bad address or 
 *	PROC_NOT_SET_ENVIRON_VAR if the environment variable doesn't 
 *	exist.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */

ReturnStatus
Proc_GetEnvironVarStub(environVar)
    Proc_EnvironVar	environVar;	/* Variable to retrieve. */
{
    register	ProcEnvironVar	*varPtr;
    register	Proc_ControlBlock	*procPtr;
    char				*namePtr;
    int					nameLength;
    int					nameAccLength;
    ReturnStatus			status;

    LOCK_MONITOR;

    procPtr = Proc_GetEffectiveProc();

    /*
     * Make name accessible.
     */

    namePtr = environVar.name;
    status = Proc_MakeStringAccessible(PROC_MAX_ENVIRON_NAME_LENGTH,
				       &namePtr, &nameAccLength, &nameLength);
    if (status != SUCCESS) {
	UNLOCK_MONITOR;
	return(status);
    }

    /*
     * Find the variable.  If not found then return an error.
     */

    varPtr = FindVar(procPtr->environPtr, namePtr);
    Proc_MakeUnaccessible((Address) namePtr, nameAccLength);

    if (varPtr == (ProcEnvironVar *) NIL) {
	UNLOCK_MONITOR;
	return(PROC_NOT_SET_ENVIRON_VAR);
    }

    /*
     * Copy out the value of the variable.
     */

    if (Proc_ByteCopy(FALSE, varPtr->valueLength + 1, (Address) varPtr->value, 
		    (Address) environVar.value) != SUCCESS) {
	UNLOCK_MONITOR;
	return(SYS_ARG_NOACCESS);
    }

    UNLOCK_MONITOR;
    return(SUCCESS);
}


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

    newEnvironPtr = (Proc_EnvironInfo *) malloc(sizeof(Proc_EnvironInfo));
    newEnvironPtr->refCount = 1;
    newEnvironPtr->size = procPtr->environPtr->size;
    newEnvironPtr->varArray = (ProcEnvironVar *) 
		malloc(sizeof(ProcEnvironVar) * newEnvironPtr->size);

    DoCopyEnviron(newEnvironPtr->size, procPtr->environPtr->varArray, 
		  newEnvironPtr->size, newEnvironPtr->varArray);

    DecEnvironRefCount(procPtr->environPtr);
    procPtr->environPtr = newEnvironPtr;

    UNLOCK_MONITOR;

    return(SUCCESS);
}

/*
 * ----------------------------------------------------------------------------
 *
 * Proc_InstallEnvironStub --
 *
 *	Install the given environment as the environment of the current
 *	process.
 *
 * Results:
 *	Error if args invalid or inaccessible.  SUCCESS otherwise.
 *
 * Side effects:
 *	A new enviroment is allocated.
 *
 * ----------------------------------------------------------------------------
 */

ENTRY ReturnStatus
Proc_InstallEnvironStub(environ, numVars)
    Proc_EnvironVar	environ[];	/* Environment to install. */
    int			numVars;	/* Number of variables in the
					 * environment. */
{
    register	Proc_EnvironVar		*userEnvironPtr;
    register	Proc_EnvironInfo	*newEnvironPtr;
    register	ProcEnvironVar	*varPtr;
    Proc_EnvironVar			*saveEnvironPtr;
    int					environLength;
    int					i;
    int					nameAccLength;
    int					nameLength;
    char				*namePtr;
    int					valueAccLength;
    int					valueLength;
    char				*valuePtr;
    ReturnStatus			status;
    Proc_ControlBlock			*procPtr;	

    LOCK_MONITOR;

    if (numVars > PROC_MAX_ENVIRON_SIZE) {
	UNLOCK_MONITOR;
	return(SYS_INVALID_ARG);
    }

    /*
     * Make the users environment array accessible.
     */

    if (numVars > 0) {
	Vm_MakeAccessible(VM_READONLY_ACCESS, numVars * sizeof(Proc_EnvironVar),
	    		  (Address) environ, &environLength, 
			  (Address *) &saveEnvironPtr);
	userEnvironPtr = saveEnvironPtr;
	if (environLength != numVars * sizeof(Proc_EnvironVar)) {
	    if (userEnvironPtr != (Proc_EnvironVar *) NIL) {
		Vm_MakeUnaccessible((Address) userEnvironPtr, environLength);
	    }
	    UNLOCK_MONITOR;
	    return(SYS_ARG_NOACCESS);
	}
    }

    /*
     * Allocate a new environment of size at least MIN_ENVIRON_SIZE.
     */

    newEnvironPtr = (Proc_EnvironInfo *) malloc(sizeof(Proc_EnvironInfo));
    newEnvironPtr->refCount = 1;
    if (numVars < MIN_ENVIRON_SIZE) {
	newEnvironPtr->size = MIN_ENVIRON_SIZE;
    } else {
	newEnvironPtr->size = numVars;
    }
    newEnvironPtr->varArray = (ProcEnvironVar *) 
		malloc(sizeof(ProcEnvironVar) * newEnvironPtr->size);

    /*
     * Read in the users environment variables and store them in the
     * new environment.
     */

    for (i = 0, varPtr = newEnvironPtr->varArray; 
	 i < numVars;
	 userEnvironPtr++, varPtr++, i++) {
	if (userEnvironPtr->name != (char *) USER_NIL) {
	    namePtr = userEnvironPtr->name;
	    status = Proc_MakeStringAccessible(PROC_MAX_ENVIRON_NAME_LENGTH,
				       &namePtr, &nameAccLength, &nameLength);
	    if (status != SUCCESS) {
		Vm_MakeUnaccessible((Address) saveEnvironPtr, environLength);
		FreeEnviron(newEnvironPtr, i - 1, TRUE);
		UNLOCK_MONITOR;
		return(SYS_ARG_NOACCESS);
	    }
	    valuePtr = userEnvironPtr->value;
	    status = Proc_MakeStringAccessible(PROC_MAX_ENVIRON_VALUE_LENGTH,
				   &valuePtr, &valueAccLength, &valueLength);
	    if (status != SUCCESS) {
		Vm_MakeUnaccessible((Address) saveEnvironPtr, environLength);
		Proc_MakeUnaccessible((Address) namePtr, nameAccLength);
		FreeEnviron(newEnvironPtr, i - 1, TRUE);
		UNLOCK_MONITOR;
		return(SYS_ARG_NOACCESS);
	    }

	    varPtr->name = (char *) malloc(nameLength + 1);
	    varPtr->nameLength = nameLength;
	    bcopy(namePtr, varPtr->name, nameLength + 1);

	    varPtr->value = (char *) malloc(valueLength + 1);
	    varPtr->valueLength = valueLength;
	    bcopy(valuePtr, varPtr->value, valueLength + 1);

	    Proc_MakeUnaccessible((Address) namePtr, nameAccLength);
	    Proc_MakeUnaccessible((Address) valuePtr, valueAccLength);
	} else {
	    varPtr->name = (char *) NIL;
	    varPtr->value = (char *) NIL;
	}
    }

    if (numVars > 0) {
	Vm_MakeUnaccessible((Address) saveEnvironPtr, environLength);
    }

    /*
     * If we allocated more than the user requested then set the excess
     * variables to NIL.
     */

    for (; i < newEnvironPtr->size; i++, varPtr++) {
	varPtr->name = (char *) NIL;
	varPtr->value = (char *) NIL;
    }

    /*
     * Make this new environment the environment of the current process.
     */

    procPtr = Proc_GetEffectiveProc();
    DecEnvironRefCount(procPtr->environPtr);
    procPtr->environPtr = newEnvironPtr;

    UNLOCK_MONITOR;
    return(SUCCESS);
}
