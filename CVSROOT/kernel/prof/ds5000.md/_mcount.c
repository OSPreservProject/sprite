/* 
 * _mcount.c --
 *
 *	This is the code for the routine mcount.  mcount is the routine
 *	called at the beginning of each procedure if it the code has
 *	been compiled with the -p option to cc.
 *
 *	NB: The compiler compiles this procedure into something called
 *	"_mcount" which we massage back into "mcount" (see the Makefile).
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
static char rcsid[] = "$Header$ SPRITE (DECWRL)";
#endif


#include "sprite.h"
#include "prof.h"
#include "profInt.h"
#include "sync.h"
#include "sys.h"
#include "dbg.h"

/*
 * Boolean to prevent recursion in mcount.  This would only work
 * on a uniprocessor, and can be eliminated if you never compile
 * this file with -p.
 *
static Boolean inMcount = FALSE;
 *  
 */

/*
 * There is a critical section when mcount does a pseudo-alloc
 * of the storage for its arcs.
 */

#ifndef lint
static int	mcountMutex;
#endif


/*
 *----------------------------------------------------------------------
 *
 * mcount --
 *
 *	A call to this routine is inserted by the compiler at the
 *	beginning of every routine. (Use the -p option to cc.)
 *	This looks up the call stack a bit to determine which arc
 *	of the call graph is being executed.  A call graph arc represents
 *	one routine calling another.  The routine with the call to mcount
 *	is the callee of the arc, its caller (mcount's "grandparent")
 *	is the caller of the arc.  An execution count is kept for each
 *	arc.  The counts are dumped out and analyzed by the gprof program.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increment a counter corresponding to the call graph arc.
 *
 *----------------------------------------------------------------------
 */

void
mcount()
{
#ifdef notdef
    register unsigned int calleePC;	/* PC of instr. that called mcount */
    register unsigned int callerPC;	/* PC of instr. that called mcount's 
					 * caller */
    register unsigned int instructionNumber;	/* Index into profArcIndex */
    register ProfRawArc *arcPtr;	/* Pointer to arc data storage */

    if (!profEnabled) {
	return;
    }

/*
    if (inMcount) {
	return;
    } else {
	inMcount = TRUE;
    }
*/

    /*
     * Get the PC that was saved after the jsr mcount instruction.
     * This is done by getting our frame pointer and then looking
     * next to it on the stack for the saved PC.
     * The saved PC identifies the caller of mcount and the callee
     * of the call graph arc.
     */

    calleePC  = Prof_ThisPC( Prof_ThisFP() );


    /*
     * Get the PC that was saved after the jsr foo instruction.
     * This PC identifies the caller of foo and the caller in
     * the call graph arc.
     */

    callerPC = Prof_ThisPC( Prof_CallerFP() );

    /*
     * Use the PC of the jsr foo instruction as an index into the
     * index of stored arcs.  There should only be one call instruction
     * that corresponds to the index.
     *
     * Go from PC to instruction number by subracting off the base
     * PC and dividing by the instruction size (2 bytes).
     */

    instructionNumber =  (callerPC) >> PROF_ARC_SHIFT;
    if (instructionNumber > profArcIndexSize) {
	printf(
		  "_mcount: Index (%d) exceeds bounds (%d) of index array.\n",
		  instructionNumber, profArcIndexSize);
	goto exit;
    }

    /*
     * Check to see if arcPtr equals an unused value (which is 0 because
     * profArcIndex is initialized with Byte_Zero in Prof_Start).
     */

    arcPtr = profArcIndex[instructionNumber];
    if (arcPtr == (ProfRawArc *) 0) {

#ifdef DEBUG
	printf( "mcount: 1 callerPC = %x(%d), calleePC = %x\n",
			    callerPC, instructionNumber, calleePC);
	/* DBG_CALL; */
#endif DEBUG

	/*
	 * First time call graph arc has been traversed.  Allocate arc
	 * storage from the arcList and initialize it.  This is locked
	 * to prevent the scheduler from interrupting the allocation
	 * and initialization.
	 */

	if (profArcListFreePtr >= profArcListEndPtr) {
	    profEnabled = FALSE;
	    printf( 
			"_mcount: No more arcs, stopping profiling\n");
	} else {

	    MASTER_LOCK(*mcountMutex);

	    arcPtr = profArcListFreePtr;
	    profArcListFreePtr++;
	    profArcIndex[instructionNumber] = arcPtr;
	    arcPtr->calleePC = calleePC;
	    arcPtr->count    = 1;
	    arcPtr->link     = (ProfRawArc *)NIL;

	    MASTER_UNLOCK(mcountMutex);
	}
	goto exit;
    }

    while (arcPtr->calleePC != calleePC) {
	/*
	 * Loop through the list of callee's for this caller.
	 */

	if (arcPtr->link == (ProfRawArc *)NIL) {

	    /*
	     *  Allocate, link, and initialize another arc storage unit.
	     */
#ifdef DEBUG
	    printf( 
			"mcount 2 callerPC = %x(%d), calleePC = %x\n",
			callerPC, instructionNumber, calleePC);
	/* DBG_CALL; */
#endif DEBUG

	    if (profArcListFreePtr >= profArcListEndPtr) {
		printf( "_mcount: No more arcs\n");
	    } else {
		MASTER_LOCK(mcountMutex);

		arcPtr->link = profArcListFreePtr;
		profArcListFreePtr++;

		arcPtr = arcPtr->link;
		arcPtr->calleePC	= calleePC;
		arcPtr->count		= 1;
		arcPtr->link		= (ProfRawArc *) NIL;

		MASTER_UNLOCK(mcountMutex);
	    }
	    goto exit;
	}
	arcPtr = arcPtr->link;
    }
    arcPtr->count++;

exit:
#endif
    /* inMcount = FALSE; */
    return;
}
