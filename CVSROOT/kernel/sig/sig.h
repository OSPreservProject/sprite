/*
 * sig.h --
 *
 *     Data structures and procedure headers exported by the
 *     the signal module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _SIG
#define _SIG

#include "user/sig.h"
#include "mach.h"

/*
 * The signal context that is used to restore the state after a signal.
 */
typedef struct {
    int			oldHoldMask;	/* The signal hold mask that was in
					 * existence before this signal
					 * handler was called.  */
    Mach_SigContext	machContext;	/* The machine dependent context
					 * to restore the process from. */
} Sig_Context;

/*
 * Structure that user sees on stack when a signal is taken.
 */
typedef struct {
    int		sigNum;		/* The number of this signal. */
    int		sigCode;    	/* The code of this signal. */
    Sig_Context	*contextPtr;	/* Pointer to structure used to restore the
				 * state before the signal. */
} Sig_Stack;

/*
 *----------------------------------------------------------------------
 *
 * Sig_Pending --
 *
 *	Return TRUE if a signal is pending and FALSE if not.  This routine
 *	does not impose any synchronization.
 *
 * Results:
 *	TRUE if a signal is pending and FALSE if not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#define Sig_Pending(procPtr) \
    ((Boolean) (procPtr->sigPendingMask & ~procPtr->sigHoldMask))

/*
 * Procedures for the signal module.
 */

extern	ReturnStatus	Sig_Send();
extern	ReturnStatus 	Sig_SendProc();
extern	ReturnStatus	Sig_UserSend();
extern	ReturnStatus	Sig_SetHoldMask();
extern	ReturnStatus	Sig_SetAction();
extern	ReturnStatus	Sig_Pause();

extern	void		Sig_Init();
extern	void		Sig_ProcInit();
extern	void		Sig_Fork();
extern	void		Sig_Exec();
extern	void		Sig_ChangeState();
extern	Boolean		Sig_Handle();
extern	void		Sig_Return();
extern	void		Sig_AllowMigration();

#endif _SIG

