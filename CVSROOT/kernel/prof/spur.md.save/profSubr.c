/* 
 * prof.c --
 *
 *	Routines for initializing and collecting profile information.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "prof.h"


/*
 * An on/off profiling switch.
 */

Boolean profEnabled = FALSE;

/*
 *----------------------------------------------------------------------
 *
 * Prof_Init --
 *
 *	Allocate the profile data structures and initialize the profile timer.
 *	The timer is initialized to automatically start ticking again
 *	once its interrupt line is reset.  The array of counters
 *	for sampling the PC is allocated, as is the table of call
 *	graph arc counts.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

void
Prof_Init()
{

}

/*
 *----------------------------------------------------------------------
 *
 * Prof_Start --
 *
 *	Initialize the profile data structures and the profile timer.
 *	This clears the PC sample counters, the call graph arc counters,
 *	and the index into the list of call graph arc counters.
 *
 *	The interval between profile timer interrupts is defined in devTimer.c.
 *
 * Results:
 *	Return status.
 *
 * Side effects:
 *
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Prof_Start()
{

    Sys_Printf("Profiling Not Implmented.\n");

    return(FAILURE);
}


/*
 *----------------------------------------------------------------------
 *
 * Prof_CollectInfo --
 *
 *	Collect profiling information from the stack.
 *
 *	The interval between calls to this routine is defined
 *	by the profile timer's interrupt interval, which is
 *	defined in devTimer.c.
 *
 *	Note: This is an interrupt-level routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increment the counter associated with the PC value.
 *
 *----------------------------------------------------------------------
 */

void
Prof_CollectInfo()
{
}


/*
 *----------------------------------------------------------------------
 *
 * Prof_End --
 *
 *	Stop the profiling.
 *
 * Results:
 *	Return status.
 *
 * Side effects:
 *	Profiling is disabled.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Prof_End()
{
    return(SUCCESS);
}

/*
 *----------------------------------------------------------------------
 *
 * Prof_Dump --
 *
 *	Dump out the profiling data to the specified file.
 *
 * Results:
 *	FAILURE		- return codes from Fs module.
 *
 * Side effects:
 *	Write the profiling data to a file.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Prof_Dump(dumpName)
    char *dumpName;		/* Name of the file to dump to. */
{
    return(FAILURE);
}


/*
 *----------------------------------------------------------------------
 *
 * Prof_DumpStub --
 *
 *	This system call dumps profiling information into the specified file.
 *	This is done by making the name of the file accessible, then calling 
 *	Prof_Dump.
 *
 * Results:
 *	FAILURE		- error returned by Fs module.
 *
 * Side effects:
 *	A file is written.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Prof_DumpStub(pathName)
    char *pathName;		/* The name of the file to write. */
{
	return (FAILURE);
}
