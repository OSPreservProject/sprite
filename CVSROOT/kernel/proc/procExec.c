/* procExec.c --
 *
 *	Routines to exec a program.  There is no monitor required in this
 *	file.  Access to the proc table is synchronized by locking the PCB
 *	when modifying the genFlags field.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "../vm/vm.h"
#include "proc.h"
#include "fs.h"
#include "mem.h"
#include "sync.h"
#include "sched.h"
#include "sig.h"
#include "time.h"
#include "list.h"
#include "char.h"
#include "vm.h"
#include "sys.h"
#include "machine.h"
#include "procAOUT.h"
#include "status.h"
#include "byte.h"
#include "string.h"

extern	ReturnStatus	DoExec();

static	char execFileName[FS_MAX_PATH_NAME_LENGTH];


/*
 *----------------------------------------------------------------------
 *
 * Proc_Exec --
 *
 *	Process the Exec system call.
 *
 * Results:
 *	This process will not return unless an error occurs in which case it
 *	returns the error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Proc_Exec(fileName, argPtrArray, debugMe)
    char	*fileName;	/* The name of the file to exec. */
    char	**argPtrArray;	/* The array of arguments to the exec'd 
				 * program. */
    Boolean	debugMe;	/* TRUE means that the process is 
				 * to be sent a SIG_DEBUG signal before
    				 * executing its first instruction. */
{
    char		**newArgPtrArray;
    int			newArgPtrArrayLength;
    int			strLength;
    int			accessLength;
    ReturnStatus	status;

    /*
     * Make the file name accessible. 
     */

    status = Proc_MakeStringAccessible(FS_MAX_PATH_NAME_LENGTH, &fileName,
				       &accessLength, &strLength);
    if (status != SUCCESS) {
	return(status);
    }

    String_Copy(fileName, execFileName);
    Proc_MakeUnaccessible((Address) fileName, accessLength);

    /*
     * Make the arguments array accessible.
     */

    if (argPtrArray != (char **) USER_NIL) {
	Vm_MakeAccessible(VM_READONLY_ACCESS,
			  (PROC_MAX_EXEC_ARGS + 1) * sizeof(Address),
			  (Address) argPtrArray, 
		          &newArgPtrArrayLength, (Address *) &newArgPtrArray);
	if (newArgPtrArrayLength == 0) {
	    return(SYS_ARG_NOACCESS);
	}
    } else {
	newArgPtrArray = (char **) NIL;
	newArgPtrArrayLength = 0;
    }

    status = DoExec(execFileName, strLength, newArgPtrArray, 
			   newArgPtrArrayLength / 4, TRUE, debugMe);
    if (status == SUCCESS) {
	Sys_Panic(SYS_FATAL, "Proc_Exec: DoExec returned SUCCESS!!!\n");
    }
    if (newArgPtrArray != (char **) NIL) {
	Vm_MakeUnaccessible((Address) newArgPtrArray, newArgPtrArrayLength);
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_KernExec --
 *
 *	Do an exec from a kernel process.  This will exec the program and
 *	change the type of process to a user process.
 *
 * Results:
 *	This routine does not return unless an error occurs from DoExec.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Proc_KernExec(fileName, argPtrArray)
    char *fileName;
    char **argPtrArray;
{
    register	Proc_ControlBlock	*procPtr;
    ReturnStatus			status;

    procPtr = Proc_GetCurrentProc(Sys_GetProcessorNumber());

    /*
     * Set up dummy segments so that DoExec can work properly.
     */

    procPtr->segPtrArray[VM_CODE] = Vm_SegmentNew(VM_CODE, (Fs_Stream *) NIL, 0,
					          1, 0, procPtr);
    if (procPtr->segPtrArray[VM_CODE] == (Vm_Segment *) NIL) {
	return(PROC_NO_SEGMENTS);
    }
    Vm_InitPageTable(procPtr->segPtrArray[VM_CODE], (Vm_PTE *) NIL, 
		     0, -1, FALSE);

    procPtr->segPtrArray[VM_HEAP] = Vm_SegmentNew(VM_HEAP, (Fs_Stream *) NIL, 
					          0, 1, 1, procPtr);
    if (procPtr->segPtrArray[VM_HEAP] == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(procPtr->segPtrArray[VM_CODE], procPtr);
	return(PROC_NO_SEGMENTS);
    }

    procPtr->segPtrArray[VM_STACK] = Vm_SegmentNew(VM_STACK, (Fs_Stream *) NIL, 
				   0 , 1, MACH_LAST_USER_STACK_PAGE, procPtr);
    if (procPtr->segPtrArray[VM_STACK] == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(procPtr->segPtrArray[VM_CODE], procPtr);
	Vm_SegmentDelete(procPtr->segPtrArray[VM_HEAP], procPtr);
	return(PROC_NO_SEGMENTS);
    }

    /*
     * Change this process to a user process.
     */

    procPtr->context = VM_INV_CONTEXT;

    Proc_Lock(procPtr);
    procPtr->genFlags &= ~PROC_KERNEL;
    procPtr->genFlags |= PROC_USER;
    Proc_Unlock(procPtr);

    Vm_SetupContext(procPtr);

    status = DoExec(fileName, String_Length(fileName),
			argPtrArray, PROC_MAX_EXEC_ARGS, FALSE, FALSE);

    /*
     * If the exec failed, then delete the extra segments and fix up the
     * proc table entry to put us back into kernel mode.
     */

    Proc_Lock(procPtr);
    procPtr->genFlags &= ~PROC_USER;
    procPtr->genFlags |= PROC_KERNEL;
    Proc_Unlock(procPtr);

    Vm_ReinitContext(procPtr);

    Vm_SegmentDelete(procPtr->segPtrArray[VM_CODE], procPtr);
    Vm_SegmentDelete(procPtr->segPtrArray[VM_HEAP], procPtr);
    Vm_SegmentDelete(procPtr->segPtrArray[VM_STACK], procPtr);

    return(status);
}

typedef struct {
    List_Links	links;
    Address	stringPtr;
    int		stringLen;
} ArgListElement;

ReturnStatus	SetupInterpret();
Boolean		CopyInArgs();


/*
 *----------------------------------------------------------------------
 *
 * DoExec --
 *
 *	Exec a new program.  The current process image is overlayed by the 
 *	newly exec'd program.
 *
 * Results:
 *	This routine does not return unless an error occurs in which case the
 *	error code is returned.
 *
 * Side effects:
 *	The state of the calling process is modified for the new image.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DoExec(fileName, fileNameLength, argPtrArray, numArgs, userProc, debugMe)
    char	fileName[];	/* The name of the file that is to be exec'd */
    int		fileNameLength;	/* The length of the file name. */
    char	**argPtrArray;	/* The array of argument pointers. */
    int		numArgs;	/* The number of arguments in the argArray. */
    Boolean	userProc;	/* Set if the calling process is a user 
				 * process. */
    Boolean	debugMe;	/* TRUE means that the process is to be 
				 * sent a SIG_DEBUG signal before
    				 * executing its first instruction. */
{
    register	Proc_ControlBlock	*procPtr;
    register	ArgListElement		*argListPtr;
    register	Proc_AOUT		*aoutPtr;
    register	char			**argPtr;
    register	int			argNumber;
    int					origNumArgs;
    Vm_ExecInfo				*execInfoPtr;
    char				**newArgPtrArray;
    int					tArgNumber;
    Proc_AOUT				aout;
    List_Links				argList;
    Vm_Segment				*codeSegPtr = (Vm_Segment *) NIL;
    char				*copyAddr;
    int					copyLength;
    Fs_Stream				*filePtr;
    ReturnStatus			status;
    char				buffer[PROC_MAX_INTERPRET_SIZE];
    int					extraArgs = 0;
    char				*shellArgPtr;
    int					argBytes;
    int					userStackPointer;
    int					usp;
    int					entry;
    Boolean				usedFile;
    int					uid = -1;
    int					gid = -1;

    origNumArgs = numArgs;

    procPtr = Proc_GetActualProc(Sys_GetProcessorNumber());
    List_Init(&argList);

    /*
     * Open the file that is to be exec'd.
     */
    filePtr = (Fs_Stream *) NIL;
    status =  Fs_Open(fileName, (FS_READ | FS_EXECUTE), FS_FILE, 0,
		      &filePtr);
    if (status != SUCCESS) {
	return(status);
    }

    /*
     * Determine if this file has the set uid or set gid bits set.
     */
    Fs_CheckSetID(filePtr, &uid, &gid);

    /*
     * See if this file is already cached by the virtual memory system.
     */
    codeSegPtr = Vm_FindCode(filePtr, procPtr, &execInfoPtr, &usedFile);
    if (codeSegPtr == (Vm_Segment *) NIL) {
	int	sizeRead;

	/*
	 * Read the file header.
	 */
	sizeRead = PROC_MAX_INTERPRET_SIZE;
	status = Fs_Read(filePtr, buffer, 0, &sizeRead);
	if (status != SUCCESS) {
	    goto execError;
	}
	if (sizeRead >= 2 && buffer[0] == '#' && buffer[1] == '!') {
	    Vm_InitCode(filePtr, (Vm_Segment *) NIL, (Vm_ExecInfo *) NIL);
	    /*
	     * See if this is an interpreter file.
	     */
	    status = SetupInterpret(buffer, sizeRead, &filePtr, 
				    &shellArgPtr, &extraArgs, &aout); 
	    if (status != SUCCESS) {
		return(status);
	    }
	    sizeRead = sizeof(Proc_AOUT);
	    aoutPtr = &aout;
	    codeSegPtr = Vm_FindCode(filePtr, procPtr, &execInfoPtr, &usedFile);
	} else {
	    aoutPtr = (Proc_AOUT *) buffer;
	}
	if (codeSegPtr == (Vm_Segment *) NIL && 
	    (sizeRead < sizeof(Proc_AOUT) || PROC_BAD_MAGIC_NUMBER(*aoutPtr))) {
	    status = PROC_BAD_AOUT_FORMAT;
	    goto execError;
	}
    }

    /*
     * The total number of arguments is the number passed in plus the
     * number of extra arguments because is an interpreter file.
     */
    if (argPtrArray == (char **) NIL) {
	numArgs = extraArgs;
    } else {
	numArgs += extraArgs;
    }

    /*
     * Copy in all of the arguments.  If we are executing a normal
     * program then we just copy in the arguments as is.  If this is
     * an interpreter file then we do the following:
     *
     *	1) If the user passed in arguments, then arg[0] is left alone.
     *	2) If arguments were passed to the interpreter program in the
     *	   interpreter file then they are placed in arg[1].
     *  3) The name of the interpreter file is placed in arg[1] or arg[2]
     *	   depending if there were interpreter arguments from step 2.
     *  3) Original arguments arg[1] through arg[n] are shifted over.
     */
    userStackPointer = MACH_MAX_USER_STACK_ADDR;
    for (argNumber = 0, argPtr = argPtrArray; 
	 argNumber < numArgs;
	 argNumber++) {
	char	*stringPtr;
	int	stringLength;
        int	realLength;
	Boolean accessible;

	accessible = FALSE;

	if ((argNumber > 0 || argPtrArray == (char **) NIL) && extraArgs > 0) {
	    if (extraArgs == 2) {
		stringPtr = shellArgPtr;
		realLength = String_Length(shellArgPtr);
	    } else {
		stringPtr = fileName;
		realLength = fileNameLength;
	    }
	    extraArgs--;
	} else {
	    if (!userProc) {
		if (*argPtr == (char *) NIL) {
		    break;
		}
		stringPtr = *argPtr;
		stringLength = PROC_MAX_EXEC_ARG_LENGTH;
	    } else {
		if (*argPtr == (char *) USER_NIL) {
		    break;
		}
		Vm_MakeAccessible(VM_READONLY_ACCESS,
				  PROC_MAX_EXEC_ARG_LENGTH + 1, 
				  (Address) *argPtr, 
				  &stringLength,
				  (Address *) &stringPtr);
		if (stringLength == 0) {
		    status = SYS_ARG_NOACCESS;
		    goto execError;
		}
		accessible = TRUE;
	    }
	    /*
	     * Find out the length of the argument.
	     */
	    realLength = String_NLength(stringLength, stringPtr);
	    /*
	     * Move to the next argument.
	     */
	    argPtr++;
	}

	/*
	 * Put this string onto the argument list.
	 */
	argListPtr = (ArgListElement *) Mem_Alloc(sizeof(ArgListElement));
	argListPtr->stringPtr = (char *) Mem_Alloc(realLength);
	argListPtr->stringLen = realLength;
	List_InitElement((List_Links *) argListPtr);
	List_Insert((List_Links *) argListPtr, LIST_ATREAR(&argList));
	/*
	 * Make room on the stack for this string.  Make it 4 byte aligned.
	 */
	userStackPointer -= ((realLength + 1) + 3) & ~3;
	/*
	 * Copy over the argument.
	 */
	Byte_Copy(realLength, (Address) stringPtr, 
		  (Address) argListPtr->stringPtr);
	/*
	 * Clean up 
	 */
	if (accessible) {
	    Vm_MakeUnaccessible((Address) stringPtr, stringLength);
	}
    }

    /*
     * We no longer need access to the old arguments. 
     */
    if (userProc && argPtrArray != (char **) NIL) {
	Vm_MakeUnaccessible((Address) argPtrArray, origNumArgs * 4);
    }

    /*
     * Set up virtual memory for the new image.
     */
    if (!SetupVM(procPtr, aoutPtr, filePtr, usedFile, &codeSegPtr, 
		 execInfoPtr, &entry)) {
	/*
	 * Setup VM will make sure that the file is closed and that
	 * all new segments are freed up.
	 */
	filePtr = (Fs_Stream *) NIL;
	status = VM_NO_SEGMENTS;
	goto execError;
    }

    /*
     * Now copy all of the arguments onto the stack.
     */
    argBytes = MACH_MAX_USER_STACK_ADDR - userStackPointer;
    (void) Vm_MakeAccessible(VM_READWRITE_ACCESS,
			  argBytes, (Address) userStackPointer,
			  &copyLength, (Address *) &copyAddr);
    newArgPtrArray = (char **) Mem_Alloc((argNumber + 1) * sizeof(Address));
    argNumber = 0;
    usp = userStackPointer;
    while (!List_IsEmpty(&argList)) {
	argListPtr = (ArgListElement *) List_First(&argList);
	/*
	 * Copy over the argument.
	 */
	Byte_Copy(argListPtr->stringLen, 
		  (Address) argListPtr->stringPtr, 
		  (Address) copyAddr);
	copyAddr[argListPtr->stringLen] = '\0';
	newArgPtrArray[argNumber] = (char *) usp;
	copyAddr += ((argListPtr->stringLen + 1) + 3) & ~3;
	usp += ((argListPtr->stringLen + 1) + 3) & ~3;
	/*
	 * Clean up 
	 */
	List_Remove((List_Links *) argListPtr);
	Mem_Free((Address) argListPtr->stringPtr);
	Mem_Free((Address) argListPtr);
	argNumber++;
    }
    Vm_MakeUnaccessible(copyAddr - argBytes, argBytes);
    /*
     * Now copy the array of arguments plus argc and the address of argv 
     * onto the stack.
     */
    copyLength = (argNumber + 1) * sizeof(Address) + sizeof(int);
    newArgPtrArray[argNumber] = (char *) USER_NIL;
    userStackPointer -= copyLength;
    tArgNumber = argNumber;
    (void) Vm_CopyOut(sizeof(int), (Address) &tArgNumber,
		      (Address) userStackPointer);
    (void) Vm_CopyOut(copyLength - sizeof(int), (Address) newArgPtrArray,
	      (Address) (userStackPointer + sizeof(int)));
    /*
     * Free the array of arguments.
     */
    Mem_Free((Address) newArgPtrArray);
    
    /*
     * Finally we are actually ready to start the new process.
     */
    procPtr->genRegs[SP] = userStackPointer;
    procPtr->genFlags |= PROC_DONT_MIGRATE;
    if (debugMe) {
	procPtr->progCounter = entry - 2;
	procPtr->genFlags |= PROC_DEBUG_ON_EXEC;
    } else {
	procPtr->progCounter = entry;
    }

    /*
     * Close any streams that have been marked close-on-exec.
     */
    Fs_CloseOnExec(procPtr);

    /*
     * Do set uid and set gid here.
     */
    if (uid != -1) {
	procPtr->effectiveUserID = uid;
    }
    if (gid != -1) {
	ProcAddToGroupList(procPtr, gid);
    }

    /*
     * Save the name of the file, for use during process migration,
     * and generally usefull for debugging and for the procstat program.
     */
    String_Copy(fileName, procPtr->codeFileName);

    /*
     * Set up signal hold masks and default actions, then unlock
     * the process table entry so signals can come in.
     */
    Sig_ProcInit(procPtr);
    Proc_Unlock(procPtr);

    /*
     * Disable interrupts.  Note that we don't use the macro DISABLE_INTR 
     * because there is an implicit enable interrupts when we return to user 
     * mode.
     */
    Sys_DisableIntr();
    Proc_RunUserProc(procPtr->genRegs, procPtr->progCounter, Proc_Exit,
		    procPtr->stackStart + MACH_EXEC_STACK_OFFSET);
    Sys_Panic(SYS_FATAL, "DoExec: Proc_RunUserProc returned.\n");

execError:
    /*
     * The exec failed after or while copying in the arguments.  Free any
     * virtual memory allocated and free the arguments that were copied in.
     */
    if (filePtr != (Fs_Stream *) NIL) {
	if (codeSegPtr != (Vm_Segment *) NIL) {
	    Vm_SegmentDelete(codeSegPtr, procPtr);
	} else {
	    Vm_InitCode(filePtr, (Vm_Segment *) NIL, (Vm_ExecInfo *) NIL);
	}
	Fs_Close(filePtr);
    }
    while (!List_IsEmpty(&argList)) {
	argListPtr = (ArgListElement *) List_First(&argList);
	List_Remove((List_Links *) argListPtr);
	Mem_Free((Address) argListPtr->stringPtr);
	Mem_Free((Address) argListPtr);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * SetupInterpret --
 *
 *	Read in the interpreter name, arguments and object file header.
 *
 * Results:
 *	Error if for some reason could not parse the interpreter name
 *	and arguments are could not open the interpreter object file.
 *
 * Side effects:
 *	*filePtrPtr contains pointer to the interpreter object file, 
 *	*argPtrPtr points to interpreter argument string, *extraArgsPtr
 *	contains number of arguments that have to be prepended to the 
 *	argument vector passed to the interpreter and *aoutPtr contains the
 *	interpreter files a.out header.
 *
 *----------------------------------------------------------------------
 */ 

static ReturnStatus
SetupInterpret(buffer, sizeRead, filePtrPtr, argPtrPtr, 
	       extraArgsPtr, aoutPtr)
    register	char	*buffer;	/* Bytes read in from file.*/
    int			sizeRead;	/* Number of bytes in buffer. */	
    register	Fs_Stream **filePtrPtr;	/* IN/OUT parameter: Exec'd file as 
					 * input, interpreter file as output. */
    char		**argPtrPtr;	/* Pointer to shell argument string. */
    int			*extraArgsPtr;	/* Number of arguments that have to be
					 * added for the intepreter. */
    Proc_AOUT		*aoutPtr;	/* Place to read a.out header into. */
{
    register	char	*strPtr;
    char		*shellNamePtr;
    int			i;
    ReturnStatus	status;

    Fs_Close(*filePtrPtr);

    /*
     * Make sure the interpreter name and arguments are terminated by a 
     * carriage return.
     */
    for (i = 2, strPtr = &(buffer[2]);
	 i < sizeRead && *strPtr != '\n';
	 i++, strPtr++) {
    }
    if (i == sizeRead) {
	return(PROC_BAD_FILE_NAME);
    }
    *strPtr = '\0';

    /*
     * Get a pointer to the name of the file to exec.
     */

    for (strPtr = &(buffer[2]); Char_IsSpace(*strPtr); strPtr++) {
    }
    if (*strPtr == '\0') {
	return(PROC_BAD_FILE_NAME);
    }
    shellNamePtr = strPtr;
    while (!Char_IsSpace(*strPtr) && *strPtr != '\0') {
	strPtr++;
    }
    *extraArgsPtr = 1;

    /*
     * Get a pointer to the arguments if there are any.
     */

    if (*strPtr != '\0') {
	*strPtr = '\0';
	strPtr++;
	while (Char_IsSpace(*strPtr)) {
	    strPtr++;
	}
	if (*strPtr != '\0') {
	    *argPtrPtr = strPtr;
	    *extraArgsPtr = 2;
	}
    }

    /*
     * Open the interpreter to exec and read the a.out header.
     */

    status =  Fs_Open(shellNamePtr, (FS_READ | FS_EXECUTE), FS_FILE,
		      0, filePtrPtr);
    if (status != SUCCESS) {
	return(status);
    }

    sizeRead = sizeof(Proc_AOUT);
    status = Fs_Read(*filePtrPtr, (Address) aoutPtr, 0, &sizeRead);
    if (status == SUCCESS && sizeRead != sizeof(Proc_AOUT)) {
	status = PROC_BAD_AOUT_FORMAT;
    }
    if (status != SUCCESS) {
	Fs_Close(*filePtrPtr);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * SetupVM --
 *
 *	Setup virtual memory for this process.
 *
 * Results:
 *	TRUE if could set up VM, false otherwise.
 *
 * Side effects:
 *	*filePtrPtr contains pointer to the interpreter object file, 
 *	*argPtrPtr points to interpreter argument string, *extraArgsPtr
 *	contains number of arguments that have to be prepended to the 
 *	argument vector passed to the interpreter and *aoutPtr contains the
 *	interpreter files a.out header.
 *
 *----------------------------------------------------------------------
 */ 
static Boolean
SetupVM(procPtr, aoutPtr, codeFilePtr, usedFile, codeSegPtrPtr, execInfoPtr, 
	entryPtr)
    register	Proc_ControlBlock	*procPtr;
    register	Proc_AOUT		*aoutPtr;
    Fs_Stream				*codeFilePtr;
    Boolean				usedFile;
    Vm_Segment				**codeSegPtrPtr;
    register	Vm_ExecInfo		*execInfoPtr;
    int					*entryPtr;
{
    register	Vm_Segment	*segPtr;
    Vm_PTE			pte;
    int				numPages;
    int				fileOffset;
    int				pageOffset;
    Boolean			notFound;
    Vm_Segment			*heapSegPtr;
    Vm_ExecInfo			execInfo;
    Fs_Stream			*heapFilePtr;

    pte = vm_ZeroPTE;
    pte.validPage = 1;
    pte.zeroFill = 0;
    if (*codeSegPtrPtr == (Vm_Segment *) NIL) {
	execInfoPtr = &execInfo;
	execInfoPtr->entry = aoutPtr->entry;
	if (aoutPtr->data != 0) {
	    execInfoPtr->heapPages = (aoutPtr->data - 1) / VM_PAGE_SIZE + 1;
	} else { 
	    execInfoPtr->heapPages = 0;
	}
	if (aoutPtr->bss != 0) {
	    execInfoPtr->heapPages += (aoutPtr->bss - 1) / VM_PAGE_SIZE + 1;
	}
	execInfoPtr->heapPageOffset = 
			PROC_DATA_LOAD_ADDR(*aoutPtr) / VM_PAGE_SIZE;
	execInfoPtr->heapFileOffset = 
			(int) PROC_DATA_FILE_OFFSET(*aoutPtr);
	execInfoPtr->bssFirstPage = 
			PROC_BSS_LOAD_ADDR(*aoutPtr) / VM_PAGE_SIZE;
	if (aoutPtr->bss > 0) {
	    execInfoPtr->bssLastPage = (int) (execInfoPtr->bssFirstPage + 
				       (aoutPtr->bss - 1) / VM_PAGE_SIZE);
	} else {
	    execInfoPtr->bssLastPage = 0;
	}
	/* 
	 * Set up the code image.
	 */
	numPages = (aoutPtr->code - 1) / VM_PAGE_SIZE + 1;
	fileOffset = PROC_CODE_FILE_OFFSET(*aoutPtr);
	pageOffset = PROC_CODE_LOAD_ADDR(*aoutPtr) / VM_PAGE_SIZE;
	segPtr = Vm_SegmentNew(VM_CODE, codeFilePtr, fileOffset,
			       numPages, pageOffset, procPtr);
	if (segPtr == (Vm_Segment *) NIL) {
	    Vm_InitCode(codeFilePtr, (Vm_Segment *) NIL, (Vm_ExecInfo *) NIL);
	    Fs_Close(codeFilePtr);
	    return(FALSE);
	}
	pte.protection = VM_UR_PROT;
	Vm_InitPageTable(segPtr, &pte, pageOffset,
			 pageOffset + numPages - 1, TRUE);
	Vm_InitCode(codeFilePtr, segPtr, execInfoPtr);
	*codeSegPtrPtr = segPtr;
	notFound = TRUE;
    } else {
	notFound = FALSE;
    }

    if (usedFile || notFound) {
	Fs_StreamCopy(codeFilePtr, &heapFilePtr);
    } else {
	heapFilePtr = codeFilePtr;
    }
    /* 
     * Set up the heap image.
     */
    segPtr = Vm_SegmentNew(VM_HEAP, heapFilePtr, execInfoPtr->heapFileOffset,
			       execInfoPtr->heapPages, 
			       execInfoPtr->heapPageOffset, procPtr);
    if (segPtr == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(*codeSegPtrPtr, procPtr);
	Fs_Close(heapFilePtr);
	return(FALSE);
    }
    pte.protection = VM_URW_PROT;
    Vm_InitPageTable(segPtr, &pte, execInfoPtr->heapPageOffset,
		     execInfoPtr->bssFirstPage - 1, TRUE);
    pte.zeroFill = 1;
    if (execInfoPtr->bssLastPage > 0) {
	Vm_InitPageTable(segPtr, &pte, execInfoPtr->bssFirstPage,
			 execInfoPtr->bssLastPage, TRUE);
    }
    heapSegPtr = segPtr;
    /*
     * Set up a new stack.
     */
    segPtr = Vm_SegmentNew(VM_STACK, (Fs_Stream *) NIL, 0, 
			   1, MACH_LAST_USER_STACK_PAGE, procPtr);
    if (segPtr == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(*codeSegPtrPtr, procPtr);
	Vm_SegmentDelete(heapSegPtr, procPtr);
	return(FALSE);
    }
    Vm_InitPageTable(segPtr, &pte, MACH_LAST_USER_STACK_PAGE, 
		     MACH_LAST_USER_STACK_PAGE, TRUE);

    Proc_Lock(procPtr);
    procPtr->genFlags |= PROC_NO_VM;
    Vm_SegmentDelete(procPtr->segPtrArray[VM_CODE], procPtr);
    Vm_SegmentDelete(procPtr->segPtrArray[VM_HEAP], procPtr);
    Vm_SegmentDelete(procPtr->segPtrArray[VM_STACK], procPtr);
    procPtr->segPtrArray[VM_CODE] = *codeSegPtrPtr;
    procPtr->segPtrArray[VM_HEAP] = heapSegPtr;
    procPtr->segPtrArray[VM_STACK] = segPtr;
    procPtr->genFlags &= ~PROC_NO_VM;
    Vm_ReinitContext(procPtr);

    *entryPtr = execInfoPtr->entry;

    return(TRUE);
}
