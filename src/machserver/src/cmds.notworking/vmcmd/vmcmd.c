/* 
 * vmcmd.c --
 *
 *	User interface to the Vm_Cmd system call.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/cmds/vmcmd/RCS/vmcmd.c,v 1.1 92/04/17 20:15:21 kupfer Exp Locker: kupfer $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <cfuncproto.h>
#include <option.h>
#include <vm.h>

/*
 * Command line options.
 */
int	maxPageOutProcs = -1;
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

Option optionArray[] = {
#if 0
    {OPT_INT, "a", (Address)&alwaysRefuse, "\tSet the always refuse flag.\n"},
    {OPT_INT, "A", (Address)&alwaysSayYes, "\tSet the always say yes flag.\n"},
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
    {OPT_INT, "t", (Address)&tracesPerSecond, "\tNumber of times to trace per second"},
    {OPT_TRUE, "T", (Address)&endTracing, "\tEnd tracing"},
    {OPT_INT, "w", (Address)&writeablePageout, "\tSet the vm writeable page out flag"},
    {OPT_INT, "W", (Address)&writeableRefPageout, "\tSet the vm writeable ref page out flag"},
    {OPT_INT, "e", (Address)&extra1, "\tDo extra command, with the number specified\n"},
    {OPT_INT, "X", (Address)&extra2, "\tSecond argument for extra command\n"},
#endif
};
int numOptions = sizeof(optionArray) / sizeof(Option);

/* 
 * Forward references:
 */

void DoCmd _ARGS_ ((int command, int length, int arg, int *status));


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
	DoCmd(VM_FLUSH_SEGMENT, 0, arr, &status);
    }
    if (freeWhenClean != -1) {
	DoCmd(VM_SET_FREE_WHEN_CLEAN, 0, freeWhenClean, &status);
    }
    if (maxPageOutProcs != -1) {
	DoCmd(VM_SET_PAGEOUT_PROCS, 0, maxPageOutProcs, &status);
    }
    if (canCOW != -1) {
	DoCmd(VM_SET_COW, 0, canCOW, &status);
    }
    if (fsPenalty != -1) {
	DoCmd(VM_SET_FS_PENALTY, 0, fsPenalty, &status);
    }
    if (resetFsMinMax) {
	DoCmd(VM_RESET_FS_STATS, 0, 0, &status);
    }
    if (numPageGroups != -1) {
	DoCmd(VM_SET_NUM_PAGE_GROUPS, 0, numPageGroups, &status);
    }
    if (alwaysRefuse != -1) {
	DoCmd(VM_SET_ALWAYS_REFUSE, 0, alwaysRefuse, &status);
    }
    if (alwaysSayYes != -1) {
	DoCmd(VM_SET_ALWAYS_SAY_YES, 0, alwaysSayYes, &status);
    }
    if (corReadOnly != -1) {
	DoCmd(VM_SET_COR_READ_ONLY, 0, corReadOnly, &status);
    }
    if (useReadAhead != -1) {
	DoCmd(VM_SET_USE_FS_READ_AHEAD, 0, useReadAhead, &status);
    }
    if (prefetch != -1) {
	DoCmd(VM_SET_PREFETCH, 0, prefetch, &status);
    }
    if (tracesPerSecond != -1) {
	DoCmd(VM_START_TRACING, 0, tracesPerSecond, &status);
    }
    if (endTracing) {
	DoCmd(VM_END_TRACING, 0, 0, &status);
    }
    if (writeablePageout != -1) {
	DoCmd(VM_SET_WRITEABLE_PAGEOUT, 0, writeablePageout, &status);
    }
    if (writeableRefPageout != -1) {
	DoCmd(VM_SET_WRITEABLE_REF_PAGEOUT, 0, writeableRefPageout, &status);
    }
    if (extra1 != -1) {
	DoCmd(extra1, 0, extra2, &status);
    }

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
DoCmd(cmd, length, arg, statusPtr)
    int cmd;			/* argument to Vm_Cmd */
    int length;			/* ditto */
    int arg;			/* ditto */
    int *statusPtr;		/* program's exit status (out) */
{
    if (Vm_Cmd(cmd, length, arg) != SUCCESS) {
	*statusPtr = 1;
    }
}
