/*
 * gcore.h --
 *
 *	Interfile declartions for the gcore program.
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
 * $Header: /a/newcmds/gcore/RCS/gcore.h,v 1.1 89/02/05 20:50:58 mendel Exp Locker: mendel $ SPRI
TE (Berkeley)
 */

#ifndef _GCORE_H
#define _GCORE_H

/*
 * Program name used in error messages.
 */
#define	PROGRAM_NAME	"gcore"

/*
 * MAX_ARG_STRING_SIZE - Maximum size of a argument string as return 
 *			  by FindProcess.
 */
#define	MAX_ARG_STRING_SIZE	1024

/*
 * Number of and index assignments in to the segSize array return by 
 * FindProcess.
 */
#define	TEXT_SEG	0
#define	DATA_SEG	1
#define	STACK_SEG	2
#define	NUM_SEGMENTS	3


/*
 * Sig mask state as returned by FindProcess.
 */

#define	SIG_IGNORING 1
#define	SIG_HANDLING 2
#define	SIG_HOLDING  4

extern Boolean debug;

#include <sys/dir.h>
#include <sys/param.h>
#include <machine/sys/user.h>

/*
 * Possible states return by FindProcess.
 */
#define	NOT_FOUND_STATE	0	/* Could find the process. */
#define	DEBUG_STATE	1	/* Process is in the debug state. */
#define	SUSPEND_STATE	2	/* Process is in the suspend state. */
#define	WAIT_STATE	3	/* Process is in the wait state. */
#define	RUN_STATE	4	/* Process is in the run state. */
#define	UNKNOWN_STATE	5	/* Process in the unknown state. */

#define	STATE_NAMES	{\
	"Not found", "Debug", "Suspended", "Wait", "Running", "Unknown"}

/*
 * Routine exported from modules.
 */

Boolean	AttachProcess();
Boolean DetachProcess();
Boolean ReadStopInfoFromProcess();
int XferSegmentFromProcess();
int FindProcessStatus();
void GetSigMask();

#endif
