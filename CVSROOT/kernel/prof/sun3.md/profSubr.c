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
#include "profInt.h"
#include "dbg.h"
#include "sys.h"
#include "timer.h"
#include "mach.h"
#include "fs.h"
#include "byte.h"
#include "vm.h"

extern	int	etext;

/*
 * An on/off profiling switch.
 */

Boolean profEnabled = FALSE;


/*
 * A histogram of PC samples is kept for use by gprof. Each sample is a
 * counter that gets incremented when the PC is in the range for the counter.
 */

typedef struct {
    Address lowpc;
    Address highpc;
    int	    size;
} SampleHdr;

static int	pcSampleSize;
static short	*pcSamples;

/*
 * PC sampling data structures (shared with _mcount.c).
 */

int		profArcListSize;
ProfRawArc	*profArcList;
ProfRawArc	*profArcListFreePtr;
ProfRawArc	*profArcListEndPtr;

int		profArcIndexSize;
ProfRawArc	**profArcIndex;

/*
 * Flag to indicate if Prof_Init has been called.
 */
static Boolean	init = FALSE;


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
 *	Uses Vm_RawAlloc.  Each structure is order(textSize).
 *	Sets a flag indicating it has been called.
 *
 *----------------------------------------------------------------------
 */

void
Prof_Init()
{
    int numInstructions;

    /*
     * We estimate the number of instructions in the text
     * by dividing the address range in half...  This determines
     * to PC to index calculations done in mcount and Prof_CollectInfo.
     */

    numInstructions = ((unsigned)&etext - (unsigned)&spriteStart) >>
						    PROF_INSTR_SIZE_SHIFT;
    printf("Prof_Init: # instructions in kernel = %d\n", numInstructions);

    /*
     * The size of the sample array reflects a compression down
     * by the group size.
     */

    pcSampleSize = numInstructions / PROF_PC_GROUP_SIZE;
    pcSamples = (short *) Vm_RawAlloc(pcSampleSize * sizeof(short));

    /*
     * Allocate an array indexed by PC and containing a pointer
     * to the call graph arc that starts at that PC.  This array is
     * compressed by the arc group size.
     */
    profArcIndexSize = numInstructions >> PROF_ARC_GROUP_SHIFT;
    profArcIndex = 
	(ProfRawArc **) Vm_RawAlloc(profArcIndexSize * sizeof(ProfRawArc *));

    /*
     * The arcList needs an element for every distinct call instruction
     * that gets executed in the kernel.  The size is just a guess.
     */

    profArcListSize = numInstructions / PROF_CALL_RATIO;
    profArcList = 
	    (ProfRawArc *) Vm_RawAlloc(profArcListSize * sizeof(ProfRawArc));

    init = TRUE;
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
 *	The interval between profile timer interrupts is defined in the
 *	timer module.
 *
 * Results:
 *	Return status.
 *
 * Side effects:
 *	Profiling is enabled and the data structures are zeroed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Prof_Start()
{
    if (!init) {
	Prof_Init();
    }

    printf("Starting Profiling.\n");

    /*
     * Reset the PC sample counters.
     */

    bzero((Address) pcSamples, pcSampleSize * sizeof(short));

    /*
     * Reset the arc pointer list indexed by caller PC.
     */

    bzero((Address) profArcIndex, profArcIndexSize * sizeof(ProfRawArc *));

    /*
     * Set the free pointers into the arc storage.  Don't have to
     * initialize the arc storage itself because that is done
     * as arc storage is allocated by mcount.
     */

    profArcListFreePtr = &profArcList[0];
    profArcListEndPtr = &profArcList[profArcListSize-1];

    profEnabled = TRUE;
    Timer_TimerInit(TIMER_PROFILE_TIMER);
    Timer_TimerStart(TIMER_PROFILE_TIMER);

    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * Prof_CollectInfo --
 *
 *	Collect profiling information from the stack.
 *	Look at the Status register to make sure we are
 *	running in the kernel, then sample the PC.
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
Prof_CollectInfo(stackPtr)
    Mach_IntrStack	*stackPtr;
{
    if (!profEnabled) {
	return;
    }
    if (stackPtr->excStack.statusReg & MACH_SR_SUPSTATE) {
	register int pc;	/* The program counter. */
	register int index;	/* Index into the array of counters */

	pc = stackPtr->excStack.pc;
	if (pc >= (int) &spriteStart && pc <= (int) &etext) {
	    index = (pc - (int) &spriteStart) >> PROF_PC_SHIFT;
	    if (index < pcSampleSize) {
		pcSamples[index]++;
	    }
	}
    }
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
    Timer_TimerInactivate(TIMER_PROFILE_TIMER);
    profEnabled = FALSE;
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
 *	SUCCESS		- the information was dumped to the file.
 *	?		- return codes from Fs module.
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
    ReturnStatus	status;
    Fs_Stream		*streamPtr;
    int			fileOffset;
    int			writeLen;
    int			index;
    ProfArc		arc;
    ProfRawArc		*rawArcPtr;
    SampleHdr		sampleHdr;

    status = Fs_Open(dumpName, FS_WRITE|FS_CREATE, FS_FILE, 0666, &streamPtr);
    if (streamPtr == (Fs_Stream *) NIL || status != SUCCESS) {
	return(status);
    }

    /*
     * Write out the PC sampling counters.  Note they are preceeded
     * by a header that indicates the PC range and the size of the
     * sampling buffer.  (The size includes the header size...)
     */

    sampleHdr.lowpc	= (Address) &spriteStart;
    sampleHdr.highpc	= (Address) &etext;
    sampleHdr.size	= (pcSampleSize * sizeof(short)) + sizeof(sampleHdr);

    fileOffset = 0;
    writeLen = sizeof(sampleHdr);
    status = Fs_Write(streamPtr, (Address) &sampleHdr, fileOffset, &writeLen);
    if (status != SUCCESS) {
	printf(
		    "Prof_Dump: Fs_Write(1) failed, status = %x\n",status);
	goto dumpError;
    }
    printf("Prof_Dump: pc sample size = %d\n", pcSampleSize);

    fileOffset += writeLen;
    writeLen = pcSampleSize * sizeof(short);
    status = Fs_Write(streamPtr, (Address) pcSamples, fileOffset, &writeLen);
    if (status != SUCCESS) {
	printf(
		    "Prof_Dump: Fs_Write(2) failed, status = %x\n",status);
	goto dumpError;
    }

    fileOffset += writeLen;

    /*
     * Write out instantiated arcs.  Loop through the arcIndex index
     * and for each one that has arc storage figure out the PC that
     * corresponds to the arcIndex.  Then dump out an entry for
     * each routine called from that PC.
     */

    for (index = 0 ; index < profArcIndexSize ; index++) {
	rawArcPtr = profArcIndex[index];

	/* 
	 * Check if rawArcPtr equals an unused value (which is 0 because 
	 * profArcIndex is initialized with bzero in Prof_Start).
	 */
	if (rawArcPtr == (ProfRawArc *) 0) {
	    continue;
	}

	/*
	 * Reverse the PC to index calculation done in mcount.
	 */
	arc.callerPC = (int)&spriteStart + (index << PROF_ARC_SHIFT);

	do {
	    arc.calleePC = rawArcPtr->calleePC;
	    arc.count = rawArcPtr->count;

	    writeLen = sizeof(ProfArc);
	    status = Fs_Write(streamPtr, (Address)&arc, fileOffset, &writeLen);
	    if (status != SUCCESS) {
		printf(
		    "Prof_Dump: Fs_Write(3) failed, status = %x, index = %d\n",
		    status, index);
		goto dumpError;
	    }
	    fileOffset += writeLen;

	    rawArcPtr = rawArcPtr->link;
	    /*
	     * Check against NIL pointer here because of initialization
	     * in mcount.
	     */
	} while (rawArcPtr != (ProfRawArc *)NIL);
    }

    status = Fs_Close(streamPtr);
    if (status != SUCCESS) {
	printf(
		"Prof_Dump: Fs_Close failed, status = %x\n", status);
    }
    return(status);

dumpError:
    (void) Fs_Close(streamPtr);
    return(status);
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
 *	SUCCESS		- the file was dumped.
 *	?		- error returned by Fs module.
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
    char	newName[FS_MAX_PATH_NAME_LENGTH];
    int		pathNameLength;

    /*
     * Copy the name in from user space to the kernel stack.
     */
    if ((Proc_IsMigratedProcess() ?
	    Proc_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength) :
	    Vm_StringNCopy(FS_MAX_PATH_NAME_LENGTH, pathName, newName,
		       &pathNameLength)) != SUCCESS) {
	return(SYS_ARG_NOACCESS);
    }
    if (pathNameLength == FS_MAX_PATH_NAME_LENGTH) {
	return(FS_INVALID_ARG);
    }
    return(Prof_Dump(newName));
}
