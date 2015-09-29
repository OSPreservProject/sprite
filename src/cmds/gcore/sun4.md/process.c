/* 
 * process.c --
 *
 *	Routines for finding and manipulating processes for the
 *	gcore program.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/gcore/RCS/process.c,v 1.4 91/10/25 10:28:36 jhh Exp $ SPRITE (Berkeley)";
#endif not lint


#include <ctype.h>
#include <option.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <proc.h>
#include <vm.h>

#include "gcore.h"


/*
 *----------------------------------------------------------------------
 *
 *  FindProcess --
 *
 *      Find the process and argument string of the specified process.
 *
 * Results:
 *	The process state.
 *
 *
 * Side effects:
 *      The none.
 *
 *----------------------------------------------------------------------
 */

int
FindProcess(pid,argString,segSizePtr,sigStatePtr)
    int		pid;		/* Process ID to locate. */
    char	*argString;	/* Area to place argument string of
				 * the specified process. */
    int		*segSizePtr;	/* Array to store segment size in. */
    int		*sigStatePtr;   /* Out: State of the signal masks. 
				 * In: The signal to check. */
{
    int  pcbsUsed;
    ReturnStatus status;
    Proc_PCBInfo	pcb;
    Proc_PCBInfo	*pcbPtr;
    Proc_PCBArgString	pcbArgString;
    int			procState;

    /*
     * Find the PCB of the specifed process.
     */
    procState = NOT_FOUND_STATE;
    pcbPtr = &pcb;
    status = Proc_GetPCBInfo(Proc_PIDToIndex(pid),
				 Proc_PIDToIndex(pid), PROC_MY_HOSTID,
				 sizeof(Proc_PCBInfo),
				 pcbPtr, &pcbArgString, &pcbsUsed);
    if (status != SUCCESS) {
	if (debug) {
	    (void) fprintf(stderr, "Proc_GetPCBInfo for pid 0x%x error: %s",pid,
			Stat_GetMsg(status));
	}
	return procState;
    }

    /*
     * The process we got info for may not be the one that was
     * requested (different generation numbers);  check to be sure.
     */
    if (pid != pcbPtr->processID) {
	if (debug) {
	    (void) fprintf(stderr,
		    "Proc_GetPCBInfo: looking for 0x%x but found 0x%x\n",
		   pid,pcbPtr->processID);	
        }
        return procState;
    }
    /*
     * Check the process state.
     */
    switch (pcbPtr->state) {
    case PROC_SUSPENDED: {
	procState = (pcbPtr->genFlags & (PROC_DEBUGGED | PROC_ON_DEBUG_LIST)) ?
				DEBUG_STATE : 
				SUSPEND_STATE;
	break;
        }
    case PROC_RUNNING:
    case PROC_READY:
    case PROC_WAITING: {
	procState = RUN_STATE;
	break;
        }
    default:
	procState = UNKNOWN_STATE;
    }

    /*
     * Copy the argument string to return it.
     */
    if (argString != NULL) {
	char	*ap = pcbArgString.argString;
	(void) strncpy(argString,ap,MAX_ARG_STRING_SIZE-1);
	argString[MAX_ARG_STRING_SIZE-1] = 0;
	if (debug) {
	    (void) fprintf(stderr,"FindProcess: Argument string \"%s\"\n",ap);
	}
   }
   /*
    * Lookup the segment size from the virtual memory system.
    */
    if (segSizePtr != (int *) 0) {
	  Vm_SegmentInfo 	segBuf[VM_NUM_SEGMENTS];
	  int	     		pagesize = getpagesize();

	  status = Vm_GetSegInfo(pcbPtr, 0, sizeof(Vm_SegmentInfo),
			         &(segBuf[1]));
	  if (status != SUCCESS) {
	    if(debug) {
		(void) fprintf(stderr, 
		    "Couldn't read segment info for pid %x: %s\n",
		    pcbPtr->processID, Stat_GetMsg(status));
	    }
	  }
	  segSizePtr[TEXT_SEG] = segBuf[VM_CODE].numPages*pagesize;
	  segSizePtr[DATA_SEG] = segBuf[VM_HEAP].numPages*pagesize;
	  segSizePtr[STACK_SEG] = segBuf[VM_STACK].numPages*pagesize;
    }

   if (sigStatePtr != (int *) 0) {
	int	spriteSig;
	(void) Compat_UnixSignalToSprite(*sigStatePtr,&spriteSig);
	if (pcbPtr->sigActions[spriteSig] == SIG_IGNORE_ACTION) {
		*sigStatePtr = SIG_IGNORING;
	} else if (pcbPtr->sigActions[spriteSig] > SIG_NUM_ACTIONS) {
		*sigStatePtr = SIG_HANDLING;
	} else if (pcbPtr->sigHoldMask & spriteSig) {
		*sigStatePtr = SIG_HOLDING;
	} else {
		*sigStatePtr = 0;
	}

   }
   return (procState);

}

/*
 *----------------------------------------------------------------------
 *
 *  XferSegmentFromProcess --
 *
 *      Transfer a memory segment from a process into a file. 
 *
 * 	This routines transfers the memory image from the process 
 *	specified by pid into the file coreFile. The transfer starts
 *	at the address specified by startAddress and ends on the first
 *	unreadable page. 
 *
 * Results:
 *	Number of bytes written to file. -1 if an error occured.
 *
 * Side effects:
 *      The file is written.
 *
 *----------------------------------------------------------------------
 */

int
XferSegmentFromProcess(pid,startAddress,coreFile)
    int	  	pid;		/* Process ID to operate on. */
    unsigned int startAddress;	/* Address to start with. */
    FILE	*coreFile;	/* File to write image to. */
{
    ReturnStatus                status = SUCCESS;
    int				numPages, xferSize = getpagesize();
    char			*xferBuffer;
    unsigned int		nextAddress;

    /*
     * Round the starting address to point at the start of its page.
     */

    startAddress = startAddress & ~(xferSize-1);
    nextAddress = startAddress;
    if (debug) {
      fprintf (stderr,"Xfering memory from process %d...\n", pid);
      fprintf (stderr,"Start Address: %d\n", startAddress);
    }
    /*
     * Allocate the buffer to copy.
     */
    xferBuffer = malloc(xferSize);
    /*
     * Read from memory and write to file until we get an error.
     */
    numPages = 1;
    status = Proc_Debug((Proc_PID)pid,PROC_READ, xferSize, 
				(char *)nextAddress,xferBuffer);
    while (status == SUCCESS) {
        numPages++;
	if (fwrite(xferBuffer,xferSize,1,coreFile) != 1) {
	    perror(PROGRAM_NAME);
	    return (-1);
	}
	nextAddress += xferSize;
	status = Proc_Debug((Proc_PID) pid,PROC_READ,xferSize,
				(char *)nextAddress,xferBuffer);
    }
    if (debug) {
      fprintf (stderr,"%d pages transferred...\n", numPages);
      (void) fprintf(stderr,"XferSegment 0x%x - 0x%x from 0x%x\n",
		     startAddress,nextAddress, pid);
    }
    /*
     * Return the number of bytes transfered.
     */
    return (nextAddress-startAddress);
}


/*
 *----------------------------------------------------------------------
 *
 *  ReadStopInfoFromProcess --
 *
 *      Read the stop info (ie registers and fault code) from
 *	the specified process.
 *
 * Results:
 *	True if operation succeeded false otherwise.
 *
 * Side effects:
 *      The process is "attached" using the Proc_Debug call.
 *
 *----------------------------------------------------------------------
 */


Boolean
ReadStopInfoFromProcess(pid,signalNumPtr,regsPtr)
    int		pid;		/* Process ID to read. */
    int		*signalNumPtr;	/* Signal number of fault. */
    struct	regs *regsPtr;  /* Regs of process.	 */
{
    ReturnStatus    status;
    Proc_DebugState process_state;

    /*
     * Attach the process.
     */
    if (debug) {
      fprintf (stderr,"Getting regs and signal from process...\n");
    }
    status = Proc_Debug((Proc_PID)pid,PROC_GET_THIS_DEBUG,0,(char *)0,
						(char *)0);
    if (status != SUCCESS) {
	(void) fprintf(stderr, "%s: Can't attach process 0x%x error: %s", 
			PROGRAM_NAME, pid, Stat_GetMsg(status));

	return FALSE;
    }
    /*
     * Read the process's stop state.
     */
    status = Proc_Debug((Proc_PID)pid, PROC_GET_DBG_STATE, 0,
				    (char *)0,(char *)&process_state);
    if (status != SUCCESS) {
	(void) fprintf(stderr, "%s: Read state of process 0x%x error: %s", 
			PROGRAM_NAME, pid, Stat_GetMsg(status));

	return FALSE;
    }
    /*
     * Map the Sprite signal to a Unix style signal using a routine 
     * in libc.a.
     */
    (void)Compat_SpriteSignalToUnix(process_state.termStatus,signalNumPtr);
    /*
     * Convert the Mach_RegState structure return by Proc_Debug into a
     * Unix "struct regs".
     */
    if (debug) {
      fprintf (stderr,"Converting regs...\n");
    }
    ConvertSpriteRegsToUnixRegs(&process_state.regState,regsPtr);
    return (TRUE);
}

Boolean
AttachProcess(pid)
{
    ReturnStatus	status;
    /*
     * Attach the process.
     */
    status = Proc_Debug((Proc_PID)pid,PROC_GET_THIS_DEBUG,0,(char *)0,
						(char *)0);
    if (status != SUCCESS) {
	(void) fprintf(stderr, 
			"%s: Error attaching process %x: %s\n", PROGRAM_NAME,
			pid, Stat_GetMsg(status));

	return FALSE;
    }
    return TRUE;
}

Boolean
DetachProcess(pid)
{
    ReturnStatus	status;
    /*
     * Attach the process.
     */
    status = Proc_Debug((Proc_PID)pid,PROC_DETACH_DEBUGGER,0,(char *)0,
							     (char *)0);
    if (status != SUCCESS) {
	if (status != PROC_INVALID_PID) { 
	    (void) fprintf(stderr, 
			"%s: Error detaching process %x: %s\n", PROGRAM_NAME,
			pid, Stat_GetMsg(status));
	}
	return FALSE;
    }
    return TRUE;
}

ConvertSpriteRegsToUnixRegs(spriteRegsPtr,regsPtr)
Mach_RegState	*spriteRegsPtr;
struct	regs	*regsPtr;
{
    /* Mach_RegState defined in kernel/machTypes.h */
    bcopy((char *)spriteRegsPtr->ins,(char *)&(regsPtr->r_o0),
	  8*sizeof(unsigned int));
    regsPtr->r_y = spriteRegsPtr->y;
    bcopy((char *)spriteRegsPtr->globals,(char *)&(regsPtr->r_g1),
	  7*sizeof(unsigned int));
    /*assume r_sr changed to r_psr in struct regs and statusReg */
    /*changed to curPsr */
    regsPtr->r_psr = spriteRegsPtr->curPsr;
    regsPtr->r_pc = spriteRegsPtr->pc;
}
