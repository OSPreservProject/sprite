/* 
* procEnviron.c --
*
*	Routines to map from the original environment-related kernel
*	calls into the standard unix operations on the stack.  The
*	old kernel calls are still available by changing the name of the
*	call to Proc_OLD...
*
*	If a process's environment on the stack is nonexistent, then it
*	assumes it has been called by Proc_Exec rather than Proc_ExecEnv,
*	and getenv, et al., call Proc_OLD*.  This will go away once all
*	programs pass the environment on the stack.
*
* Copyright 1987 Regents of the University of California
* All rights reserved.
*/

#ifndef lint
static char rcsid[] = "$Header: procEnviron.c,v 1.5 88/07/29 17:08:48 ouster Exp $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <bstring.h>
#include <proc.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Library imports:
 */

extern char **environ;
extern void unsetenv();


/*
*----------------------------------------------------------------------
*
* Proc_GetEnvironVar --
*
*	Routine to map from the original Sprite call into getenv.
*
* Results:
*	None.
*
* Side effects:
*	The process's environment is modified.
*
*----------------------------------------------------------------------
*/

ReturnStatus
Proc_GetEnvironVar(environVar)
Proc_EnvironVar	environVar;	/* Variable to add to environment. */
{
    char *value;
    extern char *getenv();

    value = getenv(environVar.name);
    if (value == NULL) {
	return(FAILURE);
    }
    strncpy(environVar.value, value, PROC_MAX_ENVIRON_VALUE_LENGTH);
    return(SUCCESS);
}

/*
* ----------------------------------------------------------------------------
*
* Proc_GetEnvironRange --
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

ReturnStatus
Proc_GetEnvironRange(first, last, envArray, numActualVarsPtr)
    int				first;			/* First var to 
							 * retrieve. */
    int 			last;			/* Last var to 
							 * retrieve. */
    register	Proc_EnvironVar	*envArray;		/* Where to 
							 * store vars.*/
    int				*numActualVarsPtr;	/* Number of vars
							 * retrieved. */
{
    ReturnStatus			status = SUCCESS;
    int					i;
    char 				**envPtr;
    char				*varPtr;
    char				*namePtr;

    /*
     * We might not be set up with a valid environment on the stack. In
     * this case, punt and make the kernel call.
     */
    if (environ == NULL || *environ == NULL) {
	status = Proc_OLDGetEnvironRange(first, last, envArray,
					 numActualVarsPtr);
	return(status);
    }

    if (last < first || first < 0) {
	return(SYS_INVALID_ARG);
    }

    /*
     * Copy out the environment variables.
     */
    envPtr = environ;
    for (i = 0; i < first; i++) {
	if (*envPtr == NULL) {
	    *numActualVarsPtr = 0;
	    return(SUCCESS);
	}
	envPtr++;
    }
    for (i = first; i <= last && *envPtr != NULL; i++, envPtr++, envArray++) {
	varPtr = *envPtr;
	if (*varPtr == NULL) {
	    envArray->name[0] = NULL;
	    envArray->value[0] = NULL;
	} else {
	    char *value;
	    
	    value = index(varPtr, '=');
	    if (value != NULL) {
		(void) strcpy(envArray->value, (char *) (value + 1));
	    } else {
		envArray->value[0] = NULL;
	    }

	    namePtr = envArray->name;
	    while ((*varPtr != '\0') && (*varPtr != '=')) {
		*namePtr = *varPtr;
		varPtr++, namePtr++;
	    }
	    *namePtr = '\0';
	}
    }
    *numActualVarsPtr = i - first;
    return(status);
}



/*
 *----------------------------------------------------------------------
 *
 * Proc_SetEnviron --
 *
 *	Routine to map from the original Sprite call into setenv.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process's environment is modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_SetEnviron(environVar)
    Proc_EnvironVar	environVar;	/* Variable to add to environment. */
{
    setenv(environVar.name, environVar.value);
    return(SUCCESS);
}


 /*
 *----------------------------------------------------------------------
 *
 * Proc_UnsetEnviron --
 *
 *	Routine to map from the original Sprite call into unsetenv.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process's environment is modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_UnsetEnviron(environVar)
    Proc_EnvironVar	environVar;	/* Variable to add to environment. */
{
    unsetenv(environVar.name);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_InstallEnviron --
 *
 *	Routine to create a new environment, from scratch.  Formerly,
 *	this would be a system call, but now we just set up the global
 *	variable 'environ'.  Note, this doesn't free the old environ or
 *	the data it pointed to, since the data might have been allocated
 *	on the stack rather than with malloc.  However, this routine is not
 * 	likely to be called more than once, if that.
 *
 *	It's possible that the caller will be installing its current
 *	environment, in which case no changes would be necessary (since
 *	each process now gets its own copy anyway, making a private copy
 *	via this routine is unnecessary).  However, let's just create
 *	the new environment anyway: this is a temporary placeholder after
 *	all.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The process's environment is modified.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_InstallEnviron(newEnviron, numVars)
    Proc_EnvironVar	newEnviron[];	/* Array of new environment variables */
    int 		numVars; 	/* Size of array */
{
    int i;
    char *newVar;
    extern char *malloc();
    Proc_EnvironVar *envPtr;	/* pointer into array of vars */

    environ = (char **) malloc((unsigned) ((numVars + 1) * sizeof(char *)));
    for (i = 0, envPtr = newEnviron; i < numVars; i++, envPtr++) {
	newVar = malloc ((unsigned) (strlen (envPtr->name) +
				     strlen (envPtr->value) + 2));
	if (newVar == NULL) {
	    return (FAILURE);
	}
	(void) sprintf (newVar, "%s=%s", envPtr->name, envPtr->value);
	environ[i] = newVar;
    }
    environ[numVars] = NULL;
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_CopyEnviron --
 *
 *	Routine to copy the environment. If environ is set, then do nothing.
 *	otherwise, we're using the system-wide environment, so call
 *	the original kernel routine.
 *
 * Results:
 *	Propagated from the kernel call, or SUCCESS.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_CopyEnviron()
{
#define COMPAT_ENV
#ifndef COMPAT_ENV
    if (environ == NULL || *environ == NULL) {
#endif COMPAT_ENV
	return(Proc_OLDCopyEnviron());
#ifndef COMPAT_ENV
    }
    return(SUCCESS);
#endif COMPAT_ENV
}

/*
 *----------------------------------------------------------------------
 *
 * Proc_FetchGlobalEnv --
 *
 *	Reads the process's environment from the kernel into new storage
 *	allocated on the heap (to be used to pass environment into 
 *	Proc_ExecEnv).
 *
 *	The strings are done in the traditional fashion of 'name=value', though
 *	they appear on the stack in reverse order.
 *
 * Results:
 *	The address of the start of the vector. 
 *
 * Side Effects:
 *	Memory is allocated to hold the environment.
 *
 *----------------------------------------------------------------------
 */
char **
Proc_FetchGlobalEnv()
{
    register char	**vecPtr;
    register int	numVariables;
    register int	varNum;

    char		value[PROC_MAX_ENVIRON_VALUE_LENGTH];
    char		name[PROC_MAX_ENVIRON_NAME_LENGTH];
    Proc_EnvironVar	var;
    char		*tempEnv[PROC_MAX_ENVIRON_SIZE];
    int		i;

    vecPtr = tempEnv;
    varNum = 0;
    var.name = name; var.value = value;

    while ((Proc_OLDGetEnvironRange(varNum, varNum, &var, &i) == SUCCESS) && i) {
	varNum += 1;
	if (name[0]){
	    *vecPtr = malloc((unsigned) (strlen(name)+1+
				strlen(value)+1));
	    (void) strcpy(*vecPtr, name);
	    (void) strcat(*vecPtr, "=");
	    (void) strcat(*vecPtr, value);
	    vecPtr++;
	}
    }
    *vecPtr = (char *)NULL;
    numVariables = vecPtr - tempEnv + 1;
    vecPtr = (char **) malloc((unsigned) (numVariables*sizeof(char *)));
    bcopy((char *) tempEnv, (char *) vecPtr, numVariables*sizeof(char *));

    return (vecPtr);
}
