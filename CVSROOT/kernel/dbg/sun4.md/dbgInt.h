/*
 * dbgInt.h --
 *
 *     Internal types, constants, and procedure headers for the debugger module.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBGINT
#define _DBGINT



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


extern	int	dbgInDebugger; 		/* How many levels deep we are in
					   the debugger. */

extern	int	dbgTraceLevel; 		/* Our trace level. */


#endif /* _DBGINT */
