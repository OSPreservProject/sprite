/* 
 * profMigrate.c --
 *
 *	Routines to handle profiling for migrated procedures.
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */


#include "sprite.h"
#include "proc.h"
#include "procMigrate.h"
#include "status.h"
#include "prof.h"

/* 
 * Information sent for profiling.
 */

typedef struct {
    short *Prof_Buffer;
    int Prof_BufferSize;
    int Prof_Offset;
    int Prof_Scale;
} EncapState;


/*
 *----------------------------------------------------------------------
 *
 * Prof_GetEncapSize --
 *
 *	Returns the size of the storage area used to record profiling
 *      information that is sent with a migrated process.
 *
 * Results:
 *	SUCCESS is returned directly; the size of the encapsulated state
 *	is returned in infoPtr->size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Prof_GetEncapSize(procPtr, hostID, infoPtr)
    Proc_ControlBlock *procPtr;			/* process being migrated */
    int hostID;					/* host to which it migrates */
    Proc_EncapInfo *infoPtr;			/* area w/ information about
						 * encapsulated state */
{
    infoPtr->size = sizeof(EncapState);
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Prof_EncapState --
 *
 *	Encapsulate the profiling information to be sent with
 *      a migrated process.  If the processes is being profiled,
 *      then turn if off.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Disables profiling of the process.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Prof_EncapState(procPtr, hostID, infoPtr, ptr)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    int hostID;				   /* host to which it migrates */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address ptr;			   /* Pointer to allocated buffer */
{
    EncapState *encapPtr;

    encapPtr = (EncapState *) ptr;
    encapPtr->Prof_Buffer = procPtr->Prof_Buffer;
    encapPtr->Prof_BufferSize = procPtr->Prof_BufferSize;
    encapPtr->Prof_Offset = procPtr->Prof_Offset;
    encapPtr->Prof_Scale = procPtr->Prof_Scale;
    if (procPtr->Prof_Scale) {
	Prof_Disable(procPtr);
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Prof_DeencapState
 *
 *	De-encapsulate information that arrived with a migrated process.
 *      If the process was being profiled at home, then turn profiling
 *      on here.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	May enable profiling of the process.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
Prof_DeencapState(procPtr, infoPtr, ptr)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    Proc_EncapInfo *infoPtr;		  /* information about the buffer */
    Address ptr;			  /* buffer containing data */
{
    EncapState *encapPtr;

    encapPtr = (EncapState *) ptr;
    if (infoPtr->size != sizeof(EncapState)) {
	if (proc_MigDebugLevel > 0) {
	    printf("Prof_DeencapState: warning: host %d tried to migrate onto this host with wrong structure size.  Ours is %d, theirs is %d.\n",
		   procPtr->peerHostID, sizeof(EncapState),
		   infoPtr->size);
	}
	return(PROC_MIGRATION_REFUSED);
    }
    procPtr->Prof_Scale = 0;
    Prof_Enable(procPtr, encapPtr->Prof_Buffer, encapPtr->Prof_BufferSize,
	encapPtr->Prof_Offset, encapPtr->Prof_Scale);
    return SUCCESS;
}
