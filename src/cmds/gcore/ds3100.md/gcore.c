/* 
 * gcore.c --
 *
 *	This file contains a program that will produce a Unix
 *	style core dump of a process.
 *	See the man page for details on what it does.
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
static char rcsid[] = "$Header: /sprite/src/cmds/gcore/RCS/gcore.c,v 1.3 91/10/25 10:28:35 jhh Exp $ SPRITE (Berkeley)";
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
#include <kernel/mach.h>
#include <kernel/proc.h>
#include <kernel/procMach.h>
#include <a.out.h>
#include <errno.h>

extern char *sys_errlist[];

#include "gcore.h"


Boolean	debug = FALSE;			/* Output debugging info. */
char	*outFilePrefix = "core";	/* Coredump output file. */
#ifdef SAFETY
Boolean	dumpRunningProcess = FALSE;	/* If true, an attempt is made to
					 * dump process not in the debugger.
					 */
#else
Boolean	dumpRunningProcess = TRUE;	/* If true, an attempt is made to
					 * dump process not in the debugger.
					 */
#endif	

int	debugSignal = SIGTRAP;		/* Signal to bump process into the
					 * debugger with.
					 */
Boolean killProcess = FALSE;		/* Kill process after dump. */

Option optionArray[] = {
   {OPT_DOC,	(char *) NULL,	(char *) NULL,
	    "This program generates a core dump of a process.\n Synopsis:  \"gcore [options] pid [pid pid...]\"\nOptions are:"},
   {OPT_STRING, "o", (char *) &outFilePrefix, "Prefix string for dump files."},
#ifdef SAFETY
   {OPT_TRUE,   "f", (char *) &dumpRunningProcess, 
	"The dump the process evening if it is running."},
#endif
   {OPT_TRUE,   "k", (char *) &killProcess, "Kill processes after the dump."},  
   {OPT_INT,	"s", (char *) &debugSignal, "Signal number to stop processes."},
   {OPT_TRUE,	"d", (char *) &debug, "Output program debugging info."},
};

/*
 * Forward procedure declarations.
 */
static Boolean	DumpCore();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	The main program for gcore.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes output to sepcified file.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;		/* Number of command-line arguments. */
    char **argv;	/* Values of command-line arguments. */
{
    int numCantDump;
    int	i;

    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray),0);
    if (argc < 2) {
	(void) fprintf(stderr,"Usage: %s [options] pid ...\n",PROGRAM_NAME);
	exit(1);
    } 
    numCantDump = 0;
    for (i = 1; i < argc; i++) {
        int	pid;
	char	*pidString;
	char	*outFileName = malloc(strlen(outFilePrefix)+16);

	pidString = argv[i];
        /*
	 * Convert the string pidString into an integer pid.
	 */
	{
	    char	*endPtr;

	    pid = strtoul(pidString, &endPtr, 16);
	    if (debug) {
	      fprintf(stderr, "%s %d\n", pidString, pid);
	    }
	    if (endPtr == pidString) {
		(void) fprintf(stderr,"%s: Bad process id \"%s\"; ignoring\n", 
			PROGRAM_NAME,  pidString);
		numCantDump++;
		continue;
	    }
	}
	(void) sprintf(outFileName,"%s.%s",outFilePrefix,pidString);
        if (DumpCore(pid, outFileName, pidString)) {
	    (void) printf("%s: %s dumped\n",PROGRAM_NAME,outFileName);
	} else {
	    numCantDump++;
	}
    }
    exit(numCantDump);
}



/*
 *----------------------------------------------------------------------
 *
 * GetProgramName --
 *
 *	Find the program name in the command string.
 *
 *	The routine takes a command string and returns the name of the 
 *	program used in the command.  A command is assume to be a string
 *	of the form:
 *		commandPathname	arg1 arg2 arg3 ....
 *	The program name is the file name of the exec file in the command.
 *
 * Results:
 *	Nothing.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void 
GetProgramName(argString,programName)
    char	*argString;	/* Argument string of process. */
    char	*programName;	/* Area to place program name. */
{
    register char	*endPtr, *startPtr;

    /*
     * Strip any leading blanks.
     */
    while (*argString == ' ') {
	argString++;
    }
    /*
     * Locate the end of the program string. This is either the first blank
     * or end of string.
     */
    endPtr = strchr(argString,' ');
    if (endPtr == NULL) {
	endPtr = argString + strlen(argString);
    }
    if (endPtr == argString) {
	*programName = 0;
	return;
    }
    /*
     * From the end of the program string, find the beginning by working 
     * backwards until the first '/' or beginning of string is found.
     */
    for (startPtr = endPtr - 1; startPtr > argString; startPtr--) {
	if (*startPtr == '/') {
		startPtr++;
		break;
	}
    }
    { 
	int	len = endPtr - startPtr;
	bcopy(startPtr, programName, len);
	programName[len] = 0;
    }
}



/*
 *----------------------------------------------------------------------
 *
 *  DumpCore --
 *
 *      Dump the core image of the specified process.
 *
 * Results:
 *	True if dump succeeded, false otherwise.
 *
 * Side effects:
 *      Core image is dumped to a file.
 *
 *----------------------------------------------------------------------
 */
static Boolean
DumpCore(pid,coreFileName,pidString)
    int 	pid;	        /* Process id to dump. */
    char	 *coreFileName;	/* Name of core file to create. */
    char	*pidString;	/* String name of pid - use only for message. */
{
    char		*argString;	/* Argument string of process. */
    char		*argStringProgram;
    struct user		coreHeader;	/* Header written to coreFile. */
    int			segSize[NUM_SEGMENTS];
    FILE		*coreFile;	/* Core file. */
    int			procState, procOrigState;
    int			sigState;
    Boolean		retVal = FALSE;
    ProcExecHeader      header;
    /*
     * Find the status and argument string of the process. 
     */
    argString = malloc(MAX_ARG_STRING_SIZE);
    /*
     * Find and record the original state of this process. 
     */
    sigState = debugSignal;
    procOrigState = FindProcess(pid,argString,segSize,&sigState);
    /*
     * Get pcb for process.
     */
    if (procOrigState == NOT_FOUND_STATE) {
	 (void) fprintf(stderr, "%s: Process id %s not found.\n",
			PROGRAM_NAME, pidString );
	return (FALSE);
    }
    /*
     * Insure that we can open the coreFile before signalling the process.
     */
     coreFile = fopen(coreFileName,"w");
     if (coreFile == (FILE *) NULL) {
	 (void) fprintf(stderr,
		 "%s: Can't open %s: %s\n",PROGRAM_NAME,coreFileName,
		sys_errlist[errno]);
	 return (FALSE);
     }
    /*
     * If the process is not already in the DEBUG state, put it there.
     */
    if (procOrigState != DEBUG_STATE) {
	/*
	 * Check to see if SAFETY is on.
	 */
	if (!dumpRunningProcess) {
	    (void) fprintf(stderr,
		"%s: Process id %s is running, must use -f to dump.\n",
		PROGRAM_NAME, pidString);
	    (void) fclose(coreFile); (void) unlink(coreFileName);
	    return (FALSE);
	}
	/*
	 * See if the debugSignal is being held, ignored, or handled.
	 */
	if (sigState != 0) {
	    char	*problem;

	    if (sigState & SIG_IGNORING) {
		problem = "ignoring";
	    } else if (sigState & SIG_HANDLING) {
		problem = "handling";
	    } else if (sigState & SIG_HOLDING) {
		problem = "holding";
	    } else {
		problem = NULL;
	    }
	    if (problem) { 
		(void) fprintf(stderr,
		    "%s: Process %s is %s signal %d; can't dump.\n",
		    PROGRAM_NAME, pidString, problem, debugSignal);
		(void) fclose(coreFile); (void) unlink(coreFileName);
		return (FALSE);
	    }
	}
	/*
	 * Try bumping the process into the debugger. This will fail if the
	 * user can't signal the process.
	 */
	if(kill(pid,debugSignal) < 0) {
	    (void) fprintf(stderr,
		    "%s: Can't signal process %s with signal %d: %s\n",
		    PROGRAM_NAME, pidString, debugSignal,sys_errlist[errno]);
	    (void) fclose(coreFile); (void) unlink(coreFileName);
	    return (FALSE);
	 } 
	 (void) sleep(1);
	 procState = FindProcess(pid,argString,segSize,(int *) 0);
	 /* 
	  * If process is still not in the debug state, trying giving it a
	  * SIGCONT.  This appears to be necessary to get suspended process
	  * into the debug state.
	  */
	 if (procState == SUSPEND_STATE) {
		(void) kill(pid,SIGCONT);
	        (void) sleep(1);
		procState = FindProcess(pid,argString,segSize,(int *) 0);
	 }
	 if (procState != DEBUG_STATE) {
	     if (procState == NOT_FOUND_STATE) {
		(void) fprintf(stderr,
		    "%s: Process id %s disappeared after signal %d, sorry.\n",
		    PROGRAM_NAME, pidString,debugSignal);
	    } else {
		(void) fprintf(stderr,
			"%s: Process %s wont stop for signal %d.\n",
			PROGRAM_NAME, pidString, debugSignal);
	    }
	    (void) fclose(coreFile); (void) unlink(coreFileName);
	    return (FALSE);
	}
    }
    /*
     * Attach the process using the debugger interface. 
     */
    if (!AttachProcess(pid)) {
	(void) fprintf(stderr,
	       "%s: Can't attach process %s\n",PROGRAM_NAME, pidString);
	(void) fclose(coreFile); (void) unlink(coreFileName);
	return(FALSE);
    }

    /* Find the program name from the argument string. */

    argStringProgram = malloc(strlen(argString)+1);
    GetProgramName(argString,argStringProgram);

    /*
     * Fill in the core file header with our best guess.
     */
/*     coreHeader.c_magic = CORE_MAGIC;
       coreHeader.c_len = sizeof(struct user);*/
    if (!ReadStopInfoFromProcess(pid,&coreHeader.u_error,&coreHeader.u_pcb)) {
       (void) fprintf(stderr,"%s: Can't read regs of process %s\n",PROGRAM_NAME,
                        pidString);
			(void) fclose(coreFile); 
       goto errout;
     }
    /*
     * Fill in the exec file header with our best guesses.
     */
    header.fileHeader = *((ProcFileHeader *)malloc(sizeof(ProcFileHeader)));
    header.aoutHeader = *((ProcAOUTHeader *)malloc(sizeof(ProcAOUTHeader)));
    bzero((char *)&header,sizeof(header));
    header.fileHeader.magic = ZMAGIC;
    header.fileHeader.numSections = 3;
    header.aoutHeader.magic = ZMAGIC;
    header.aoutHeader.codeSize = segSize[TEXT_SEG];
    header.aoutHeader.heapSize = segSize[DATA_SEG];
    header.aoutHeader.verStamp = 23;
    
    coreHeader.u_tsize = segSize[TEXT_SEG]/PAGSIZ;
    coreHeader.u_ar0 = 0;
      (void) strncpy(coreHeader.u_comm,argStringProgram,MAXCOMLEN);

     /*
      * Write empty coreHeader to be filled in later. This reserve space
      * in the output file for the header.
      */
     if (fwrite((char *)&coreHeader,sizeof(coreHeader),1,coreFile) != 1) {
	 (void) fprintf(stderr,"%s: Can't write file %s: %s\n",PROGRAM_NAME,
		coreFileName,sys_errlist[errno]);
	 (void) fclose(coreFile);
	 goto errout;
     }

    /*
     * Follow the coreHeader with the data segment
     */
     {
/*       bcopy((char *)&coreHeader.c_aouthdr,(char *)&header, sizeof(header));*/
       if (fseek(coreFile,8192,0) < 0) {
	 (void) fprintf(stderr,
		"%s: Can't find start of data segment: %s\n",PROGRAM_NAME,
		sys_errlist[errno]);
       }
       coreHeader.u_dsize = (XferSegmentFromProcess(pid,(unsigned int)(1<<28),coreFile))/PAGSIZ;
     }
     if (coreHeader.u_dsize <= 0) {
	 (void) fprintf(stderr,"%s: Can't read data segment.\n",PROGRAM_NAME);
	 (void) fclose(coreFile);
	 goto errout;
     }

    /*
     * And then the stack segment starting with at the stack pointer.
     */
     coreHeader.u_ssize = (XferSegmentFromProcess(pid,
			       (unsigned int) coreHeader.u_pcb.pcb_regs[29], coreFile))/PAGSIZ;
     if (coreHeader.u_ssize <= 0) {
	 (void) fprintf(stderr,"%s: Can't read stack segment.\n",PROGRAM_NAME);
	 (void) fclose(coreFile);
	 goto errout;
     }
     /*
      * Now rewrite the coreFile header with the new data and stack segment
      * sizes.
      */
     if (fseek(coreFile,0,0) < 0) {
	 (void) fprintf(stderr,
		"%s: Can't rewrite file %s header: %s\n",PROGRAM_NAME,
		coreFileName,sys_errlist[errno]);
     }
     if (fwrite((char *)&coreHeader,sizeof(coreHeader),1,coreFile) != 1) {
	 (void) fprintf(stderr,"%s: Can't write %s : %s\n",PROGRAM_NAME,
		coreFileName,sys_errlist[errno]);
	 (void) fclose(coreFile);
	 goto errout;
     }
     if (fclose(coreFile) != 0)  {
	 (void) fprintf(stderr,"%s: Error closing %s : %s\n",PROGRAM_NAME,
		coreFileName,sys_errlist[errno]);
     }
     retVal = TRUE;
     /* 
      * Did the user want the process dead?
      */
     if (killProcess && (kill(pid,SIGKILL) < 0)) {
	 (void) fprintf(stderr,
		    "%s: Can't kill process %s: %s\n",
		    PROGRAM_NAME, pidString, sys_errlist[errno]);
     }
 errout:
     /*
      * Detach the debugger from the process. If the process was in
      * the suspend state the detach will start it running again. 
      * The current thinking is that this is not good so we leave
      * suspended processes in the debugger.
      */
     if (killProcess || ! ( (procOrigState == SUSPEND_STATE) ||
			    (procOrigState == DEBUG_STATE))) {
	 static char *procStateNames[] = STATE_NAMES;
	 (void) DetachProcess(pid);
	 (void) sleep(1);
	 procState = FindProcess(pid,argString,(int *) 0,(int *) 0);
	 /*
	  * If we didn't want it dead and it died?
	  */
	 if (!killProcess && (procState == NOT_FOUND_STATE)) {
	    (void) fprintf(stderr,
		"%s: Warning: Process %s disappeared after dump, sorry.\n",
		PROGRAM_NAME, pidString);
	 } else if (killProcess && (procState != NOT_FOUND_STATE)) {
#ifdef notdef
	/*
	 * This doesn't appear to work.
	 */
	    (void) fprintf(stderr,
		"%s: Warning: Process %s didn't die.\n",PROGRAM_NAME, 
		 pidString);
#endif
	 } else if (procState != procOrigState) {
	     (void) fprintf(stderr,
		"%s: Warning: Process %s new state is %s was %s\n",
		PROGRAM_NAME, pidString,procStateNames[procState],
		procStateNames[procOrigState]);
	 } 
     }
     if (!retVal) {
	 (void) unlink(coreFileName);
	 (void) fclose(coreFile);
     }
     return (retVal);
}
