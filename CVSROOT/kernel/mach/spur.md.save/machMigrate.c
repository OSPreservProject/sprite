/* 
 * machMigrate.c --
 *
 *     	Machine dependent code to support process migration.  These routines
 *     	encapsulate and deencapsulate the machine-dependent state of a
 *	process and set up the state of the process on its new machine.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "machConst.h"
#include "machInt.h"
#include "mach.h"
#include "machMon.h"
#include "sched.h"
#include "procMigrate.h"

/*
 * The information that is transferred between two machines.
 */
typedef struct {
    Mach_UserState userState;	/* the contiguous machine-dependent
				 * user state. */
    int	pc[2];			/* PC's at which to resume execution. */
} MigratedState;


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_EncapState --
 *
 *	Copy the machine-dependent information for a process into
 *	a buffer.  The buffer passed to the routine must contain space for
 *	a MigratedState structure, the size of which is accessible via 
 *	another procedure.  
 *
 * Results:
 *	The buffer is filled with the user state and PC of the process.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Mach_EncapState(procPtr, hostID, infoPtr, buffer)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    int hostID;				   /* host to which it migrates */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address buffer;			   /* Pointer to allocated buffer */
{
    return(GEN_NOT_IMPLEMENTED);
}    
    

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_DeencapState --
 *
 *	Copy the machine-dependent information for a process from
 *	a buffer.  The buffer passed to the routine must contain
 *	a MigratedState structure created by Mach_EncapState on the
 *	machine starting a migration.  
 *
 * Results:
 *	The user state and PC of the process are initialized from the
 *	encapsulated information, and the other standard process
 *	initialization operations are performed (by the general initialization
 *	procedure).
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
Mach_DeencapState(procPtr, infoPtr, buffer)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    Proc_EncapInfo *infoPtr;		  /* information about the buffer */
    Address buffer;			  /* buffer containing data */
{
    return(GEN_NOT_IMPLEMENTED);
}    
    


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_GetEncapSize --
 *
 *	Return the size of the encapsulated machine-dependent data.
 *
 * Results:
 *	The size of the migration information structure.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 *
 */

int
Mach_GetEncapSize()
{
    return(sizeof(MigratedState));
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_CanMigrate --
 *
 *	Indicate whether a process's state is in a form suitable for
 *	starting a migration.
 *
 * Results:
 *	TRUE if we can migrate using this state, FALSE otherwise.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
Boolean
Mach_CanMigrate(procPtr)
    Proc_ControlBlock *procPtr;		/* pointer to process to check */
{
    return(FALSE);
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_GetLastSyscall --
 *
 *	Return the number of the last system call performed for the current
 *	process.
 *
 * Results:
 *	The system call number is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Mach_GetLastSyscall()
{
    return 0;
}

