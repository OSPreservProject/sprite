/*
 * dbgInt.h --
 *
 *     Internal types, constants,  and procedure headers for the debugger module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBGINT
#define _DBGINT

#include "dbgAsm.h"
#include "dbgRs232.h"
#include "vmMach.h"
#include "vmMachInt.h"

/*
 * There are sixteen registers: d0-d7 and a0-7.
 */

#define	NUMGENERALREGISTERS	16

/*
 * The following are the registers that are saved when the debugger is 
 * called.
 */

extern	int		dbgTermReason;		/* Reason why debugger was 
						   called */
extern	int		dbgSfcReg;		/* Source function code reg */
extern	int		dbgDfcReg;		/* Dest function code reg */
extern	int		dbgUserContext;		/* User context register */
extern	int		dbgKernelContext;	/* Kernel context register */

extern	Boolean	dbgTracing;		/* Flag to say whether we are being
					   traced by the debugger. */
			
/*
 * Entry point into the debugger from the monitor.  If want to enter the 
 * debugger from the monitor should continue execution at this location.
 */

extern	int	dbgMonPC;


extern	int	dbgTermReason;		/* Why the debugger was entered. */

extern	int	dbgInDebugger; 		/* How many levels deep we are in 
					   the debugger. */

extern	int	dbgTraceLevel; 		/* Our trace level. */

extern 	int	dbgIntPending; 		/* Whether there is an interrupt 
					   pending for the debugger. */
extern	int	dbgExcType;		/* The exception type when an 
					   interrupt was blocked */
extern	int	dbgSavedSP;		/* Contains the true kernel stack
					   pointer after return from 
					   the call to Dbg_Main */

#endif _DBGINT
