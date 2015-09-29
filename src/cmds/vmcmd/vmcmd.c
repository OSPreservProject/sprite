/* 
 * vmcmd.c --
 *
 *	User interface to the Vm_Cmd system call.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/vmcmd/RCS/vmcmd.c,v 1.9 91/03/18 13:44:52 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <cfuncproto.h>
#include <option.h>
#include <vm.h>

/*
 * Command line options.
 */
int	maxPageOutProcs = -1;
int	pagesToCheck = -1;
int	clockSleep = -1;
int	canCOW = -1;
int	freeWhenClean = -1;
int	flushSeg = -1;
int	lowPage = -1;
int	highPage = -1;
int	fsPenalty = -1;
Boolean	resetFsMinMax = FALSE;
int	numPageGroups = -1;
int	alwaysRefuse = -1;
int	alwaysSayYes = -1;
int	corReadOnly = -1;
int	useReadAhead = -1;
int	prefetch = -1;
int	tracesPerSecond = -1;
Boolean	endTracing = FALSE;
int	writeablePageout = -1;
int	writeableRefPageout = -1;
int	extra1 = -1;
int	extra2 = 0;
#ifdef spur
int	flushOnRefBitClear = -1;
int	setUseHardRefBit = -1;
int	ownStackAndHeap = -1;
int	uniprocessorFlushPage = -1;
#endif

Option optionArray[] = {
    {OPT_INT, "a", (Address)&alwaysRefuse, "\tSet the always refuse flag.\n"},
    {OPT_INT, "A", (Address)&alwaysSayYes, "\tSet the always say yes flag.\n"},
    {OPT_INT, "c", (Address)&pagesToCheck, "\tNumber of pages to check each iteration of the clock algorithm"},
    {OPT_INT, "C", (Address)&canCOW, "\tSet the copy-on-write flag."},
    {OPT_INT, "f", (Address)&freeWhenClean, "\tSet free when clean flag."},
    {OPT_INT, "F", (Address)&fsPenalty,
		"\tPenalty in seconds to make FS pay."},
    {OPT_TRUE, "fsreset", (Address)&resetFsMinMax,
	 	"\tReset the FS cache minimum and maximum."},
    {OPT_INT, "x", (Address)&flushSeg, "\tSegment to flush."},
    {OPT_INT, "h", (Address)&highPage, "\tHighest page to flush."},
    {OPT_INT, "l", (Address)&lowPage, "\tLowest page to flush."},
    {OPT_INT, "n", (Address)&numPageGroups,
		"\tNumber of groups to divide available pages into.\n"},
    {OPT_INT, "p", (Address)&maxPageOutProcs, "\tMaximum number of page out procs"},
    {OPT_INT, "P", (Address)&prefetch, "\tSet prefetch flag"},
    {OPT_INT, "r", (Address)&corReadOnly, "\tSet copy-on-ref read-only flag"},
    {OPT_INT, "R", (Address)&useReadAhead, "\tSet use fs read ahead flag"},
    {OPT_INT, "s", (Address)&clockSleep, "\tNumber of seconds between iterations of the clock algorithm"},
    {OPT_INT, "t", (Address)&tracesPerSecond, "\tNumber of times to trace per second"},
    {OPT_TRUE, "T", (Address)&endTracing, "\tEnd tracing"},
    {OPT_INT, "w", (Address)&writeablePageout, "\tSet the vm writeable page out flag"},
    {OPT_INT, "W", (Address)&writeableRefPageout, "\tSet the vm writeable ref page out flag"},
    {OPT_INT, "e", (Address)&extra1, "\tDo extra command, with the number specified\n"},
    {OPT_INT, "X", (Address)&extra2, "\tSecond argument for extra command\n"},
#ifdef spur
    {OPT_INT, "z", (Address)&flushOnRefBitClear, "\tSet the flush page on ref bit clear flag\n"},
    {OPT_INT, "Z", (Address)&setUseHardRefBit, "\tSet the use hard ref bit flag\n"},
    {OPT_INT, "o", (Address)&ownStackAndHeap, "\tSet the coherency bit in the pte of stack and heap pages\n"},
    {OPT_INT, "u", (Address)&uniprocessorFlushPage, "\tSet the use uniprocessor page flush flag\n"},
#endif
};
int numOptions = sizeof(optionArray) / sizeof(Option);

/* 
 * Forward references:
 */

void DoCmd _ARGS_ ((int command, int arg, int *status));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Execute the commands.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls Vm_Cmd.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    int status = 0;		/* our exit status */

    (void)Opt_Parse(argc, argv, optionArray, numOptions, 0);
    if (flushSeg != -1)  {
	int	arr[3];
	arr[0] = flushSeg;
	arr[1] = lowPage;
	arr[2] = highPage;
	DoCmd(VM_FLUSH_SEGMENT, arr, &status);
    }
    if (freeWhenClean != -1) {
	DoCmd(VM_SET_FREE_WHEN_CLEAN, freeWhenClean, &status);
    }
    if (maxPageOutProcs != -1) {
	DoCmd(VM_SET_PAGEOUT_PROCS, maxPageOutProcs, &status);
    }
    if (pagesToCheck != -1) {
	DoCmd(VM_SET_CLOCK_PAGES, pagesToCheck, &status);
    }
    if (clockSleep != -1) { 
	DoCmd(VM_SET_CLOCK_INTERVAL, clockSleep, &status);
    }
    if (canCOW != -1) {
	DoCmd(VM_SET_COW, canCOW, &status);
    }
    if (fsPenalty != -1) {
	DoCmd(VM_SET_FS_PENALTY, fsPenalty, &status);
    }
    if (resetFsMinMax) {
	DoCmd(VM_RESET_FS_STATS, 0, &status);
    }
    if (numPageGroups != -1) {
	DoCmd(VM_SET_NUM_PAGE_GROUPS, numPageGroups, &status);
    }
    if (alwaysRefuse != -1) {
	DoCmd(VM_SET_ALWAYS_REFUSE, alwaysRefuse, &status);
    }
    if (alwaysSayYes != -1) {
	DoCmd(VM_SET_ALWAYS_SAY_YES, alwaysSayYes, &status);
    }
    if (corReadOnly != -1) {
	DoCmd(VM_SET_COR_READ_ONLY, corReadOnly, &status);
    }
    if (useReadAhead != -1) {
	DoCmd(VM_SET_USE_FS_READ_AHEAD, useReadAhead, &status);
    }
    if (prefetch != -1) {
	DoCmd(VM_SET_PREFETCH, prefetch, &status);
    }
    if (tracesPerSecond != -1) {
	DoCmd(VM_START_TRACING, tracesPerSecond, &status);
    }
    if (endTracing) {
	DoCmd(VM_END_TRACING, 0, &status);
    }
    if (writeablePageout != -1) {
	DoCmd(VM_SET_WRITEABLE_PAGEOUT, writeablePageout, &status);
    }
    if (writeableRefPageout != -1) {
	DoCmd(VM_SET_WRITEABLE_REF_PAGEOUT, writeableRefPageout, &status);
    }
    if (extra1 != -1) {
	DoCmd(extra1, extra2, &status);
    }
#ifdef spur
    if (flushOnRefBitClear != -1) {
	DoCmd(VM_SET_FLUSH_ON_REF_BIT_CLEAR, flushOnRefBitClear, &status);
    }
    if (setUseHardRefBit != -1) {
	DoCmd(VM_SET_USE_HARD_REF_BIT, setUseHardRefBit, &status);
    }
    if (ownStackAndHeap != -1) {
	DoCmd(VM_SET_COHERENCY_BIT, ownStackAndHeap, &status);
    }
    if (uniprocessorFlushPage != -1) {
	DoCmd(VM_SET_UP_FLUSH_PAGE, uniprocessorFlushPage, &status);
    }
#endif

    exit(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DoCmd --
 *
 *	Call Vm_Cmd and check for errors.
 *
 * Results:
 *	Sets *statusPtr to non-zero if there was an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
DoCmd(cmd, arg, statusPtr)
    int cmd;			/* argument to Vm_Cmd */
    int arg;			/* ditto */
    int *statusPtr;		/* program's exit status (out) */
{
    if (Vm_Cmd(cmd, arg) != SUCCESS) {
	*statusPtr = 1;
    }
}
