/* procExec.c --
 *
 *	Routines to exec a program.  There is no monitor required in this
 *	file.  Access to the proc table is synchronized by locking the PCB
 *	when modifying the genFlags field.
 *
 * Copyright (C) 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <procMach.h>
#include <mach.h>
#include <proc.h>
#include <procInt.h>
#include <sync.h>
#include <sched.h>
#include <fs.h>
#include <fsio.h>
#include <stdlib.h>
#include <sig.h>
#include <spriteTime.h>
#include <list.h>
#include <vm.h>
#include <sys.h>
#include <procMigrate.h>
#include <procUnixStubs.h>
#include <status.h>
#include <string.h>
#include <byte.h>
#include <rpc.h>
#include <prof.h>
#include <fsutil.h>
#include <ctype.h>
#include <stdio.h>
#include <bstring.h>
#include <vmMach.h>
#include <file.h>
/*
 * This will go away when libc is changed.
 */
#ifndef PROC_MAX_ENVIRON_LENGTH
#define PROC_MAX_ENVIRON_LENGTH (PROC_MAX_ENVIRON_NAME_LENGTH + \
				 PROC_MAX_ENVIRON_VALUE_LENGTH)
#endif /*  PROC_MAX_ENVIRON_LENGTH */

#define UNIX_CODE 1

typedef struct {
    List_Links	links;
    Address	stringPtr;
    int		stringLen;
} ArgListElement;

extern int debugProcStubs;

/*
 * Define the information needed to perform an exec: the user's stack,
 * where to put it, and whether to debug on startup.  We define a structure
 * containing the "meta-info" that isn't actually copied onto the user's
 * stack, and then another structure that also includes argc.  (By
 * separating the header into a separate structure, it's easier to take its
 * size.)
 * 
 * Note that the actual values for argv and envp passed to main() are
 * calculated by _start() based on the address of argc and are not actually
 * put on the stack given to the exec'ed process.  The size of the structure
 * to be copied onto the user's stack is the size in the 'size' field
 * minus the size of the header.
 *
 * The fileName and argString fields are used only when doing a remote exec.
 */
typedef struct {
    Address base;		/* base of the structure in user space */
    int size;			/* size of the entire structure */
    char *fileName;		/* pointer to buffer containing name of file
				 * to exec */
    int fileNameLength;		/* length of file name buffer */
    char *argString;		/* pointer to buffer containing full list of
				 * arguments, for ps listing */
    int argLength;		/* length of argString buffer */
} ExecEncapHeader;

typedef struct {
    ExecEncapHeader hdr;	/* meta-information; see above */
    /*
     * User stack data starts here.
     */
    int argc;			/* Number of arguments */
    /*
     * argv[] goes here
     * envp[] goes here
     * *argv[] goes here
     * *envp[] goes here
     */
} ExecEncapState;

/*
 * Define a structure to hold all the information about arguments
 * and environment variables (pointer and length).  This makes it easier
 * to pass a NIL pointer for the whole thing, and to keep them in one
 * place.  The number of arguments/environment pointers includes the 
 * null pointer at the end of the array.
 */

typedef struct {
    Boolean	userMode;	/* TRUE if the arguments are in user space */
    char	**argPtrArray;	/* The array of argument pointers. */
    int		numArgs;	/* The number of arguments in the argArray. */
    int		argLength;	/* actual size of argArray */
    char	**envPtrArray;	/* The array of environment pointers. */
    int		numEnvs;	/* The number of arguments in the envArray. */
    int		envLength;	/* actual size of envArray */
} UserArgs;

/*
 * Forward declarations.
 */
static ReturnStatus 	DoExec _ARGS_((char fileName[], 
			    UserArgs *userArgsPtr, 
			    ExecEncapState **encapPtrPtr, Boolean debugMe));
static ReturnStatus 	SetupInterpret _ARGS_((register char *buffer, 
			    int sizeRead, register Fs_Stream **filePtrPtr, 
			    char **argPtrPtr, int *extraArgsPtr, 
			    ProcObjInfo *objInfoPtr));
static ReturnStatus 	SetupArgs _ARGS_((UserArgs *userArgsPtr, 
			    char **extraArgArray, Address *argStackPtr, 
			    char **argStringPtr));
static ReturnStatus 	GrabArgArray _ARGS_((int maxLength, Boolean userProc, 
			    char **extraArgArray, char **argPtrArray, 
			    int *numArgsPtr, List_Links *argList, 
			    int *realLengthPtr, int *paddedLengthPtr));
static Boolean 		SetupVM _ARGS_((register Proc_ControlBlock *procPtr, 
			    register ProcObjInfo *objInfoPtr, 
			    Fs_Stream *codeFilePtr, Boolean usedFile, 
			    Vm_Segment **codeSegPtrPtr, 
			    register Vm_ExecInfo *execInfoPtr));
static Boolean		ZeroHeapEnd _ARGS_ ((Vm_ExecInfo *execInfoPtr));

#ifdef notdef
/*
 * Define a type to include the information that is passed from
 * the local setup routine to the routine that performs the actual
 * exec.
 */
typedef struct {
    Proc_AOUT				*aoutPtr;
    Vm_ExecInfo				*execInfoPtr;
    Proc_AOUT				aout;
    Vm_Segment				*codeSegPtr = (Vm_Segment *) NIL;
    char				*argString = (char *) NIL;
    Address				argBuffer = (Address) NIL;
    Fs_Stream				*filePtr;
    int					entry;
    Boolean				usedFile;
    int					uid;
    int					gid;
    ExecEncapState			*hdrPtr;
}
#endif    

/*
 * Define entry points for exec.  They are distinct due to compatibility
 * considerations.  We can deal with them better when we convert the
 * system calls to be more unix-like.
 */

/*
 *----------------------------------------------------------------------
 *
 * Proc_RemoteExec --
 *
 *	Process the Exec system call on a remote host.
 *	This does the same setup as Proc_Exec, and then initiates a migration
 *	with the stack of the new process contained in a buffer reachable
 *	from the process control block.
 *
 * Results:
 *	SUCCESS indicates that the process has been signalled to cause it
 *	to migrate before it exits the kernel.  Any other status is
 *	an error that should be returned to the process as usual.
 *
 * Side effects:
 *	Memory is allocated for the buffer containing the exec info.
 *	This should be freed by the migration encapsulation routine.
 *
 *----------------------------------------------------------------------
 */

int
Proc_RemoteExec(fileName, argPtrArray, envPtrArray, host)
    char	*fileName;	/* The name of the file to exec. */
    char	**argPtrArray;	/* The array of arguments to the exec'd 
				 * program. */
    char	**envPtrArray;	/* The array of environment variables for
				 * the exec'd program. */
    int		host;		/* ID of host on which to exec. */
{
    int status;
    
    /*
     * XXX need to check permission to migrate.
     */

    status = Proc_Exec(fileName, argPtrArray, envPtrArray, FALSE, host);
    /*
     * XXX on failure, need to clean up.
     */
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ExecEnv --
 *
 *	Here for backward compatibility.  It does an exec on the
 *	local host.
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
Proc_ExecEnv(fileName, argPtrArray, envPtrArray, debugMe)
    char	*fileName;	/* The name of the file to exec. */
    char	**argPtrArray;	/* The array of arguments to the exec'd 
				 * program. */
    char	**envPtrArray;	/* The array of environment variables
				 * of the form NAME=VALUE. */ 
    Boolean	debugMe;	/* TRUE means that the process is 
				 * to be sent a SIG_DEBUG signal before
    				 * executing its first instruction. */
{
    return(Proc_Exec(fileName, argPtrArray, envPtrArray, debugMe, 0));
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_Exec --
 *
 *	The ultimate entry point for the Exec system call.  This
 * 	handles both local and remote exec's.  It calls SetupExec to
 * 	initialize the file pointer, a.out info, user's stack image, etc.
 *	The a.out information is used if the exec is
 * 	performed on this machine, but for a remote exec, the file
 *	is reopened in case of different machine types.  The stack is
 * 	used locally or copied to the remote host.
 *
 * Results:
 *	For local exec's, this procedure will not return unless an error
 *	occurs, in which case it returns the error.  For remote exec's,
 *	SUCCESS is returned, and the calling routine arranges for
 *	the process to hit a migration signal before continuing.
 *
 * Side effects:
 *	The argument & environment arrays are made accessible.  Memory
 *	is allocated for the file name.  
 *	The DoExec routine makes the arrays unaccessible.  It frees the
 *	space for the file name, unless the name is used for a remote
 *	exec, in which case it is left around until after migration.
 *
 *----------------------------------------------------------------------
 */

int
Proc_Exec(fileName, argPtrArray, envPtrArray, debugMe, host)
    char	*fileName;	/* The name of the file to exec. */
    char	**argPtrArray;	/* The array of arguments to the exec'd 
				 * program. */
    char	**envPtrArray;	/* The array of environment variables
				 * of the form NAME=VALUE. */ 
    Boolean	debugMe;	/* TRUE means that the process is 
				 * to be sent a SIG_DEBUG signal before
    				 * executing its first instruction. */
    int		host;		/* Host to which to do remote exec, or 0
				 * for local host. */
{
    char		**newArgPtrArray;
    int			newArgPtrArrayLength;
    char		**newEnvPtrArray;
    int			newEnvPtrArrayLength;
    UserArgs		userArgs;
    int			strLength;
    int			accessLength;
    ReturnStatus	status;
    char 		*execFileName;
    ExecEncapState	*encapPtr;
    ExecEncapState	**encapPtrPtr;
    Proc_ControlBlock 	*procPtr;
    


    /*
     * Make the file name accessible. 
     */

    status = Proc_MakeStringAccessible(FS_MAX_PATH_NAME_LENGTH, &fileName,
				       &accessLength, &strLength);
    if (status != SUCCESS) {
	return(status);
    }

    execFileName = malloc(accessLength);
    (void) strncpy(execFileName, fileName, accessLength);
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

    /*
     * Make the environments array accessible.
     */

    if (envPtrArray != (char **) USER_NIL) {
	Vm_MakeAccessible(VM_READONLY_ACCESS,
			  (PROC_MAX_ENVIRON_SIZE + 1) * sizeof(Address),
			  (Address) envPtrArray, 
		          &newEnvPtrArrayLength, (Address *) &newEnvPtrArray);
	if (newEnvPtrArrayLength == 0) {
	    return(SYS_ARG_NOACCESS);
	}
    } else {
	newEnvPtrArray = (char **) NIL;
	newEnvPtrArrayLength = 0;
    }
    /*
     * Perform the exec, if local, or setup the user's stack if remote.
     */
    userArgs.userMode = TRUE;
    userArgs.argPtrArray = newArgPtrArray;
    userArgs.numArgs = newArgPtrArrayLength / sizeof(Address);
    userArgs.argLength = newArgPtrArrayLength;
    userArgs.envPtrArray = newEnvPtrArray;
    userArgs.numEnvs = newEnvPtrArrayLength / sizeof(Address);
    userArgs.envLength = newEnvPtrArrayLength;

    /*
     * Check for explicit remote exec onto this host, in which case it's
     * a local exec.
     */
    if (host == rpc_SpriteID) {
	host = 0;
    }

    if (host != 0) {
	encapPtrPtr = &encapPtr;
    } else {
	encapPtrPtr = (ExecEncapState **) NIL;
    }
    status = DoExec(execFileName, &userArgs, encapPtrPtr, debugMe);
    if (status == SUCCESS) {
	if (host != 0) {
	    /*
	     * Set up the process to migrate.
	     */
	    procPtr = Proc_GetCurrentProc();
	    Proc_Lock(procPtr);
	    status = ProcInitiateMigration(procPtr, host);
	    if (status == SUCCESS) {
		procPtr->remoteExecBuffer = (Address) encapPtr;
		Proc_FlagMigration(procPtr, host, TRUE);
	    } else {
		free((Address) encapPtr);
	    }
	    Proc_Unlock(procPtr);
	} else {
	    panic("Proc_Exec: DoExec returned SUCCESS!!!\n");
	}
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

/* 
 * Use the old Proc_KernExec until this gets installed as the sole procExec.c.
 */
int
Proc_KernExec(fileName, argPtrArray)
    char *fileName;
    char **argPtrArray;
{
    register	Proc_ControlBlock	*procPtr;
    ReturnStatus			status;
    UserArgs				userArgs;

    procPtr = Proc_GetCurrentProc();

    /*
     * Set up dummy segments so that DoExec can work properly.
     */

    procPtr->vmPtr->segPtrArray[VM_CODE] = 
				Vm_SegmentNew(VM_CODE, (Fs_Stream *) NIL, 0,
					          1, 0, procPtr);
    if (procPtr->vmPtr->segPtrArray[VM_CODE] == (Vm_Segment *) NIL) {
	return(PROC_NO_SEGMENTS);
    }

    procPtr->vmPtr->segPtrArray[VM_HEAP] =
		    Vm_SegmentNew(VM_HEAP, (Fs_Stream *) NIL, 0, 1, 1, procPtr);
    if (procPtr->vmPtr->segPtrArray[VM_HEAP] == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_CODE], procPtr);
	return(PROC_NO_SEGMENTS);
    }

    procPtr->vmPtr->segPtrArray[VM_STACK] =
		    Vm_SegmentNew(VM_STACK, (Fs_Stream *) NIL, 
				   0 , 1, mach_LastUserStackPage, procPtr);
    if (procPtr->vmPtr->segPtrArray[VM_STACK] == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_CODE], procPtr);
	Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_HEAP], procPtr);
	return(PROC_NO_SEGMENTS);
    }

    /*
     * Change this process to a user process.
     */

    Proc_Lock(procPtr);
    procPtr->genFlags &= ~PROC_KERNEL;
    procPtr->genFlags |= PROC_USER;
    Proc_Unlock(procPtr);

    VmMach_ReinitContext(procPtr);

    userArgs.userMode = FALSE;
    userArgs.argPtrArray = argPtrArray;
    userArgs.numArgs = PROC_MAX_EXEC_ARGS;
    userArgs.argLength = PROC_MAX_EXEC_ARGS * sizeof(Address);
    userArgs.envPtrArray = (char **) NIL;
    userArgs.numEnvs = 0;
    userArgs.envLength = 0;
    status = DoExec(fileName, &userArgs, (ExecEncapState **) NIL, FALSE);
    /*
     * If the exec failed, then delete the extra segments and fix up the
     * proc table entry to put us back into kernel mode.
     */

    Proc_Lock(procPtr);
    procPtr->genFlags &= ~PROC_USER;
    procPtr->genFlags |= PROC_KERNEL;
    Proc_Unlock(procPtr);

    VmMach_ReinitContext(procPtr);

    Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_CODE], procPtr);
    Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_HEAP], procPtr);
    Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_STACK], procPtr);

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * SetupArgs --
 *
 *	Chase through arrays of character strings (usually in user space)
 *	and copy them into a contiguous array.  This array is later copied
 *	onto the stack of the exec'd program, and it may be used to
 *	pass the arguments to another host for a remote exec.  It
 *	contains argc, argv, envp, and the strings referenced by argv and
 *	envp.  All values in argv and envp are relative to the presumed
 *	start of the data in user space, which is normally set up to end at
 *	mach_MaxUserStackAddr.  If
 *	the exec is performed on a different machine, the pointers in argv and
 *	envp must be adjusted by the relative values of mach_MaxUserStackAddr.
 *
 *	The format of the structure is defined by ExecEncapState above.
 *		
 *
 * Results:
 *	A ReturnStatus is returned.  Any non-SUCCESS result indicates a failure
 *	that should be returned to the user.
 *	In addition, a pointer to the encapsulated stack is returned,
 *	as well as its size.
 *
 * Side effects:
 *	Memory is allocated for the argument stack.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
SetupArgs(userArgsPtr, extraArgArray, argStackPtr, argStringPtr)
    UserArgs *userArgsPtr;	/* Information for taking args
				 * and environment. */ 
    char     **extraArgArray;	/* Any arguments that should be
	     		 	 * inserted prior to argv[1] */
    Address  *argStackPtr; 	/* Pointer to contain address of encapsulated
				 * stack. */
    char     **argStringPtr;	/* Pointer to allocated buffer for argument
				 * list as a single string (for ps) */
{
    int	     numArgs;		/* The number of arguments in the argArray. */
    int	     numEnvs;		/* The number of arguments in the envArray. */
    register	ArgListElement		*argListPtr;
    register	int			argNumber;
    char				**newArgPtrArray;
    char				**newEnvPtrArray;
    List_Links				argList;
    List_Links				envList;
    char				*argBuffer;
    char				*envBuffer;
    register	char			*argString;
    int					argStringLength;
    ReturnStatus			status;
    int					usp;
    int					paddedArgLength;
    int					paddedEnvLength;
    int					bufSize;
    Address				buffer;
    int					stackSize;
    ExecEncapHeader			*hdrPtr;
    ExecEncapState			*encapPtr;

    /*
     * Initialize variables so when if we abort we know what to clean up.
     */
    *argStringPtr = (char *) NIL;
    
    List_Init(&argList);
    List_Init(&envList);

    /*
     * Copy in the arguments.  argStringLength is an upper bound on
     * the total length permitted.
     */
    numArgs = userArgsPtr->numArgs;
    argStringLength = PROC_MAX_EXEC_ARG_LENGTH;
    status = GrabArgArray(PROC_MAX_EXEC_ARG_LENGTH + 1,
			  userArgsPtr->userMode, extraArgArray,
			  userArgsPtr->argPtrArray, &numArgs,
			  &argList, &argStringLength,
			  &paddedArgLength);


    if (status != SUCCESS) {
	goto execError;
    }
    /*
     * Copy in the environment.
     */

    numEnvs = userArgsPtr->numEnvs;
    status = GrabArgArray(PROC_MAX_ENVIRON_LENGTH + 1,
			  userArgsPtr->userMode, (char **) NIL,
			  userArgsPtr->envPtrArray, &numEnvs,
			  &envList, (int *) NIL,
			  &paddedEnvLength);

    if (status != SUCCESS) {
	goto execError;
    }

    /*
     * Now copy all of the arguments and environment variables into a buffer.
     * Allocate the buffer and initialize pointers into it.
     * The stack ends up in the following state:  the top word is argc.
     * Right below this is the array of pointers to arguments (argv).  Right
     * below this is the array of pointers to environment stuff (envp).  So,
     * to figure out the address of argv, one simply adds a word to the address
     * of the top of the stack.  To figure out the address of envp, one
     * looks at argc and skips over the appropriate amount of space to jump
     * over argc and argv = (1 + (argc + 1)) * 4 bytes.  The extra "1" is for
     * the null argument at the end of argv.  Below all that stuff on the
     * stack come the environment and argument strings themselves.
     */
    bufSize = sizeof(ExecEncapState) + (numArgs + numEnvs + 2) * sizeof(Address)
	+ paddedArgLength + paddedEnvLength;
    buffer = malloc(bufSize);
    *argStackPtr = buffer;
    encapPtr = (ExecEncapState *) buffer;
    hdrPtr = &encapPtr->hdr;
    hdrPtr->size = bufSize;
    stackSize = Byte_AlignAddr((bufSize - sizeof(ExecEncapHeader)));
    hdrPtr->base = mach_MaxUserStackAddr - stackSize;
    encapPtr->argc = numArgs;
    newArgPtrArray = (char **) (buffer + sizeof(ExecEncapState));
    newEnvPtrArray = (char **) ((int) newArgPtrArray +
				(numArgs + 1) * sizeof(Address));
    argBuffer = (Address) ((int) newEnvPtrArray +
			   (numEnvs + 1) * sizeof(Address));
    envBuffer =  (argBuffer + paddedArgLength);
				
    argNumber = 0;
    usp = (int)hdrPtr->base + (int) argBuffer - (int) &encapPtr->argc;
    argString = malloc(argStringLength + 1);
    *argStringPtr = argString;

    while (!List_IsEmpty(&argList)) {
	argListPtr = (ArgListElement *) List_First(&argList);
	/*
	 * Copy the argument.
	 */
	bcopy((Address) argListPtr->stringPtr, (Address) argBuffer,
		    argListPtr->stringLen);
	newArgPtrArray[argNumber] = (char *) usp;
	argBuffer += Byte_AlignAddr(argListPtr->stringLen);
	usp += Byte_AlignAddr(argListPtr->stringLen);
	bcopy((Address) argListPtr->stringPtr, argString,
		    argListPtr->stringLen - 1);
	argString[argListPtr->stringLen - 1] = ' ';
	argString += argListPtr->stringLen;
	/*
	 * Clean up
	 */
	List_Remove((List_Links *) argListPtr);
	free((Address) argListPtr->stringPtr);
	free((Address) argListPtr);
	argNumber++;
    }
    argString[0] = '\0';
    newArgPtrArray[argNumber] = (char *) USER_NIL;
    
    argNumber = 0;
    while (!List_IsEmpty(&envList)) {
	argListPtr = (ArgListElement *) List_First(&envList);
	/*
	 * Copy the environment variable.
	 */
	bcopy((Address) argListPtr->stringPtr, (Address) envBuffer,
		    argListPtr->stringLen);
	newEnvPtrArray[argNumber] = (char *) usp;
	envBuffer += Byte_AlignAddr(argListPtr->stringLen);
	usp += Byte_AlignAddr(argListPtr->stringLen);
	/*
	 * Clean up
	 */
	List_Remove((List_Links *) argListPtr);
	free((Address) argListPtr->stringPtr);
	free((Address) argListPtr);
	argNumber++;
    }
    newEnvPtrArray[argNumber] = (char *) USER_NIL;

    /*
     * We're done here.  Leave it to the caller to free the copy of the
     * stack after copying it to user space.
     */
    return(SUCCESS);
    
execError:
    /*
     * The exec failed while copying in the arguments.  Free any
     * arguments or environment variables that were copied in.
     */
    while (!List_IsEmpty(&argList)) {
	argListPtr = (ArgListElement *) List_First(&argList);
	List_Remove((List_Links *) argListPtr);
	free((Address) argListPtr->stringPtr);
	free((Address) argListPtr);
    }
    while (!List_IsEmpty(&envList)) {
	argListPtr = (ArgListElement *) List_First(&envList);
	List_Remove((List_Links *) argListPtr);
	free((Address) argListPtr->stringPtr);
	free((Address) argListPtr);
    }
    return(status);

}


/*
 *----------------------------------------------------------------------
 *
 * GrabArgArray --
 *
 *	Copy a an array of strings from user space and put it in a
 *	linked list of strings.  The terminology for "args" refers
 *	to argv, but the same routine is used for environment variables
 *	as well.
 *
 * Results:
 *	A ReturnStatus indicates any sort of error, indicating immediate
 *	failure that should be reported to the user.  Otherwise, the
 *	arguments and their lengths are returned in the linked list
 *	referred to by argListPtr, and the total length is returned
 *	in *totalLengthPtr.
 *
 * Side effects:
 *	Memory is allocated to hold the strings and the structures
 *	pointing to them.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
GrabArgArray(maxLength, userProc, extraArgArray, argPtrArray, numArgsPtr,
	     argList, realLengthPtr, paddedLengthPtr)
    int	     maxLength;		/* The maximum length of one argument */
    Boolean  userProc;		/* Set if the calling process is a user 
	     			 * process. */
    char     **extraArgArray;	/* Any arguments that should be
	     		 	 * inserted prior to argv[1] */
    char     **argPtrArray;	/* The array of argument pointers. */
    int	     *numArgsPtr;	/* Pointer to the number of arguments in the
				 * argArray. This is updated with the
				 * actual number of arguments. */
    List_Links *argList;	/* Pointer to header of list containing
				 * copied data. Assumed to be initialized by
				 * caller. */
    int      *realLengthPtr; 	/* Pointer to contain combined size, without
				 * padding.   Value passed in contains
				 * maximum. */
    int      *paddedLengthPtr; 	/* Pointer to contain combined size, including
				 * padding. */
{
    int 				totalLength = 0;
    int 				paddedTotalLength = 0;
    int 				extraArgs;
    Boolean 				accessible;
    register	ArgListElement		*argListPtr;
    register	char			**argPtr;
    register	int			argNumber;
    ReturnStatus			status;
    char				*stringPtr;
    int					stringLength;
    int					realLength;
    
    if (extraArgArray != (char **) NIL) {
	for (extraArgs = 0; extraArgArray[extraArgs] != (char *)NIL;
	     extraArgs++) {
	}
    } else {
	extraArgs = 0;
    }
    
    for (argNumber = 0, argPtr = argPtrArray; 
	 argNumber < *numArgsPtr;
	 argNumber++) {

	accessible = FALSE;

	if ((argNumber > 0 || argPtrArray == (char **) NIL) && extraArgs > 0) {
	    stringPtr = extraArgArray[0];
	    realLength = strlen(stringPtr) + 1;
	    extraArgArray++;
	    extraArgs--;
	} else {
	    if (!userProc) {
		if (*argPtr == (char *) NIL) {
		    break;
		}
		stringPtr = *argPtr;
		stringLength = maxLength;
	    } else {
		if (*argPtr == (char *) USER_NIL) {
		    break;
		}
		Vm_MakeAccessible(VM_READONLY_ACCESS,
				  maxLength + 1, 
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
	     * Find out the length of the argument.  Because of accessibility
	     * limitations the whole string may not be available.
	     */
	    {
		register char *charPtr;
		for (charPtr = stringPtr, realLength = 0;
		     (realLength < stringLength) && (*charPtr != '\0');
		     charPtr++, realLength++) {
		}
		realLength++;
	    }
	    /*
	     * Move to the next argument.
	     */
	    argPtr++;
	}

	/*
	 * Put this string onto the argument list.
	 */
	argListPtr = (ArgListElement *)
		malloc(sizeof(ArgListElement));
	argListPtr->stringPtr =  malloc(realLength);
	argListPtr->stringLen = realLength;
	List_InitElement((List_Links *) argListPtr);
	List_Insert((List_Links *) argListPtr, LIST_ATREAR(argList));
	/*
	 * Calculate the room on the stack needed for this string.
	 * Make it double-word aligned to make access efficient on
	 * all machines.  Increment the amount needed to save the argument
	 * list (the same total length, but without the padding).
	 */
	paddedTotalLength += Byte_AlignAddr(realLength);
	totalLength += realLength;
	/*
	 * Copy over the argument and ensure null termination.
	 */
	bcopy((Address) stringPtr, (Address) argListPtr->stringPtr, realLength);
	argListPtr->stringPtr[realLength-1] = '\0';
	/*
	 * Clean up 
	 */
	if (accessible) {
	    Vm_MakeUnaccessible((Address) stringPtr, stringLength);
	}
	if (realLength > maxLength+1) {
	    status = GEN_E2BIG;
	    goto execError;
	}
    }
    if (realLengthPtr != (int *) NIL) {
	if (totalLength > *realLengthPtr) {
	    /*
	     * Would really like to flag "argument too long" here.
	     * Also, should we check after every argument?
	     */
	    status = GEN_INVALID_ARG;
	    goto execError;
	}
	*realLengthPtr = totalLength;
    }
    if (paddedLengthPtr != (int *) NIL) {
	*paddedLengthPtr = paddedTotalLength;
    }
    *numArgsPtr = argNumber;
    return(SUCCESS);
    
execError:
    /*
     * We hit an error while copying in the arguments.  Free any
     * arguments that were copied in.
     */
    while (!List_IsEmpty(argList)) {
	argListPtr = (ArgListElement *) List_First(argList);
	List_Remove((List_Links *) argListPtr);
	free((Address) argListPtr->stringPtr);
	free((Address) argListPtr);
    }
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DoExec --
 *
 *	Exec a new program.  If the exec is to be done on this host, the
 *	current process image is overlayed by the 
 *	newly exec'd program.  If not, then set up the image of the
 *	user stack and set *encapPtrPtr to point to it.
 *
 * Results:
 *	In the local case, this routine does not return unless an
 *	error occurs in which case the
 *	error code is returned.  In the remote case, SUCCESS indicates
 *	the remote exec may continue.
 *
 * Side effects:
 *	The state of the calling process is modified for the new image and
 *	the argPtrArray and envPtrArray are made unaccessible if this
 *	is a user process.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DoExec(fileName, userArgsPtr, encapPtrPtr, debugMe)
    char	fileName[];	/* The name of the file that is to be exec'd */
    UserArgs *userArgsPtr;	/* Information for taking args
				 * and environment, or NIL. */
    ExecEncapState
	**encapPtrPtr;		/* User stack state, either for us to use,
				 * for us to set, or NIL */
    Boolean	debugMe;	/* TRUE means that the process is to be 
				 * sent a SIG_DEBUG signal before
    				 * executing its first instruction. */
{
    register	Proc_ControlBlock	*procPtr;
    Vm_ExecInfo				*execInfoPtr;
    Vm_ExecInfo				execInfo;
    Vm_Segment				*codeSegPtr = (Vm_Segment *) NIL;
    char				*argString = (char *) NIL;
    char				*argStringSave;
    Address				argBuffer = (Address) NIL;
    Fs_Stream				*filePtr;
    ReturnStatus			status;
    char				buffer[PROC_MAX_INTERPRET_SIZE];
    int					extraArgs = 0;
    char				*shellArgPtr;
    char				*extraArgsArray[2];
    char				**extraArgsPtrPtr;
    int					argBytes;
    Address				userStackPointer;
    Boolean				usedFile;
    int					uid = -1;
    int					gid = -1;
    ExecEncapState			*encapPtr;
    int					importing = 0;
    int					exporting = 0;
    ProcObjInfo				objInfo;

#ifdef notdef
    DBG_CALL;
#endif

    /*
     * Use the encapsulation buffer and arguments arrays to determine
     * whether everything is local, or we're starting a remote exec,
     * or finishing one from another host.
     */
    if (encapPtrPtr != (ExecEncapState **) NIL) {
	if (userArgsPtr != (UserArgs *) NIL) {
	    exporting = TRUE;
	} else {
	    importing = TRUE;
	}
    }
    procPtr = Proc_GetActualProc();

    /*
     * objInfo.unixCompat is set if the header of the file indicates
     * it is a Unix binary.
     */
    objInfo.unixCompat = 0;
    procPtr->unixProgress = PROC_PROGRESS_NOT_UNIX;

    /* Turn off profiling */
    if (procPtr->Prof_Scale != 0) {
	Prof_Disable(procPtr);
    }

    /*
     * Save the argString away, because if we hit an error we always
     * set procPtr->argString back to this value.
     */
    argStringSave = procPtr->argString;

    /*
     * Open the file that is to be exec'd.
     */
    filePtr = (Fs_Stream *) NIL;
    status =  Fs_Open(fileName, FS_EXECUTE | FS_FOLLOW, FS_FILE, 0, &filePtr);
    if (status != SUCCESS) {
	filePtr = (Fs_Stream *) NIL;
	goto execError;
    }

    /*
     * Determine if this file has the set uid or set gid bits set.
     */
    Fs_CheckSetID(filePtr, &uid, &gid);

    /*
     * See if this file is already cached by the virtual memory system.
     */
    execInfoPtr = (Vm_ExecInfo *)NIL;
    codeSegPtr = Vm_FindCode(filePtr, procPtr, &execInfoPtr, &usedFile);
    if (codeSegPtr == (Vm_Segment *) NIL) {
	int	sizeRead;

	/*
	 * Read the file header.
	 */
	sizeRead = PROC_MAX_INTERPRET_SIZE > sizeof(ProcExecHeader) ?
						PROC_MAX_INTERPRET_SIZE :
						sizeof(ProcExecHeader);
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
				    &shellArgPtr, &extraArgs, &objInfo); 
	    if (status != SUCCESS) {
		filePtr = (Fs_Stream *)NIL;
		goto execError;
	    }
	    codeSegPtr = Vm_FindCode(filePtr, procPtr, &execInfoPtr, &usedFile);
	} else {
	    if (sizeRead < sizeof(ProcExecHeader) ||
		ProcGetObjInfo(filePtr, (ProcExecHeader *)buffer, &objInfo) != SUCCESS) {
		    if(ProcIsObj(filePtr,1)==SUCCESS) {
			status = FS_NO_ACCESS;
		    } else {
			status = PROC_BAD_AOUT_FORMAT;
		    }
		goto execError;
	    }
	}
    }

    if (!importing) {
	/*
	 * Set up whatever special arguments we might have due to an
	 * interpreter file.  If the
	 */
	if (extraArgs > 0) {
	    int i;
	    int index;

	    if (userArgsPtr->argPtrArray == (char **) NIL) {
		extraArgsArray[0] = fileName;
		index = 1;
	    } else {
		index = 0;
	    }
	    for (i = index; extraArgs > 0; i++, extraArgs--) {
		if (extraArgs == 2) {
		    extraArgsArray[i] = shellArgPtr;
		} else {
		    extraArgsArray[i] = fileName;
		}
	    }
	    extraArgsArray[i] = (char *) NIL;
	    extraArgsPtrPtr = extraArgsArray;
	} else {
	    extraArgsPtrPtr = (char **) NIL;
	}
	/*
	 * Copy in the argument list and environment into a single contiguous
	 * buffer.
	 */
	status = SetupArgs(userArgsPtr, extraArgsPtrPtr,
			   &argBuffer, &argString);

	if (status != SUCCESS) {
	    goto execError;
	}

	/*
	 * We no longer need access to the old arguments or the environment. 
	 */
	if (userArgsPtr->userMode) {
	    if (userArgsPtr->argPtrArray != (char **) NIL) {
		Vm_MakeUnaccessible((Address) userArgsPtr->argPtrArray,
				    userArgsPtr->argLength);
		userArgsPtr->argPtrArray = (char **)NIL;
		userArgsPtr->argLength = 0;
	    }
	    if (userArgsPtr->envPtrArray != (char **) NIL) {
		Vm_MakeUnaccessible((Address) userArgsPtr->envPtrArray,
				    userArgsPtr->envLength);
		userArgsPtr->envPtrArray = (char **)NIL;
		userArgsPtr->envLength = 0;
	    }
	}

	/*
	 * Close any streams that have been marked close-on-exec.
	 */
	Fs_CloseOnExec(procPtr);
    } else {
	/*
	 * We're "importing" this process.  Use the stack copied over from
	 * its former host.
	 */
	argBuffer = procPtr->remoteExecBuffer;
	procPtr->remoteExecBuffer = (Address) NIL;
	encapPtr = (ExecEncapState *) argBuffer;
	argString = encapPtr->hdr.argString;
    }
    
    /*
     * Change the argument string.
     */
    procPtr->argString = argString;
    /*
     * Do set uid here.  This way, the uid will be set before a remote
     * exec.
     */
    if (uid != -1) {
	procPtr->effectiveUserID = uid;
    }
    /*
     * If we're doing the initial part of a remote exec, time to
     * return to our caller.  Free up whatever virtual memory resources
     * we had set up.
     */
    encapPtr = (ExecEncapState *) argBuffer;
    if (exporting) {
	if (filePtr != (Fs_Stream *) NIL) {
	    if (codeSegPtr != (Vm_Segment *) NIL) {
		Vm_SegmentDelete(codeSegPtr, procPtr);
		if (!usedFile) {
		    /*
		     * If usedFile is TRUE then the file has already been closed
		     * by Vm_SegmentDelete.
		     */
		    (void) Fs_Close(filePtr);
		}
	    } else {
		/*
		 * We're not setting up the segment after all, so let vm
		 * clean up state and wake up anyone waiting for us to
		 * set up the segment.
		 */
		Vm_InitCode(filePtr, (Vm_Segment *) NIL, (Vm_ExecInfo *) NIL);
		(void) Fs_Close(filePtr);
	    }
	}
	*encapPtrPtr = encapPtr;
	encapPtr->hdr.fileName = fileName;
	encapPtr->hdr.argString = argString;
	return(SUCCESS);
    }
    /*
     * The file name has been dynamically allocated if it was copied in
     * on this host from user space.
     */
    if (!importing && userArgsPtr->userMode) {
	free(fileName);
	fileName = (char *) NIL;
    }
    /*
     * Set up virtual memory for the new image.
     */
    if (execInfoPtr == (Vm_ExecInfo *)NIL) {
	execInfoPtr = &execInfo;
    }
    if (!SetupVM(procPtr, &objInfo, filePtr, usedFile, &codeSegPtr, 
		 execInfoPtr)) {
	/*
	 * Setup VM will make sure that the file is closed and that
	 * all new segments are freed up.
	 */
	filePtr = (Fs_Stream *) NIL;
	status = VM_NO_SEGMENTS;
	goto execError;
    }

    /*
     * Now copy all of the arguments and environment variables onto the stack.
     */
    argBytes = encapPtr->hdr.size - sizeof(ExecEncapHeader);
    userStackPointer = encapPtr->hdr.base;
    status = Vm_CopyOut(argBytes, (Address) &encapPtr->argc,
		      userStackPointer);

    if (status != SUCCESS) {
	goto execError;
    }

    /*
     * Free original argString (kept in argStringSave, in case we
     * needed it) here, after last chance to goto execError.
     */
    if (argStringSave != (char *)NIL) {
	free(argStringSave);
    }

    /*
     * Set-gid only needs to be done on the host running the process.
     */
    if (gid != -1) {
	ProcAddToGroupList(procPtr, gid);
    }

    /*
     * Take signal actions for execed process.
     */
    Sig_Exec(procPtr);
    if (debugMe) {
	/*
	 * Debugged processes get a SIG_DEBUG at start up.
	 */
	Sig_SendProc(procPtr, SIG_DEBUG, SIG_NO_CODE, (Address)0);
    }
    if (!importing && (procPtr->genFlags & PROC_FOREIGN)) {
	ProcRemoteExec(procPtr, uid);
    }

    /* If this is a vfork process, wake the parent now */
    if (procPtr->genFlags & PROC_VFORKCHILD) {
	Proc_VforkWakeup(procPtr);
    }

    Proc_Unlock(procPtr);

    free(argBuffer);
    argBuffer = (Address) NIL;
    
    /*
     * Move the stack pointer on the sun4.
     */
    if (execInfoPtr->flags & UNIX_CODE) {
#ifdef sun4
	/*
	 * Unix on the sun4 has the stack in a different location from
	 * Sprite.
	 */
	userStackPointer += 32;
	if (debugProcStubs) {
	    printf("Moving stack pointer for Unix binary.\n");
	}
#endif
	procPtr->unixProgress = PROC_PROGRESS_UNIX;
    }

    /*
     * Disable interrupts.  Note that we don't use the macro DISABLE_INTR 
     * because there is an implicit enable interrupts when we return to user 
     * mode.
     */
    Mach_ExecUserProc(procPtr, userStackPointer, (Address) execInfoPtr->entry);
    panic("DoExec: Proc_RunUserProc returned.\n");

execError:
    /*
     * The exec failed after or while copying in the arguments.  Free any
     * virtual memory allocated and free any arguments or environment
     * variables that were copied in.
     */
    if (filePtr != (Fs_Stream *) NIL) {
	if (codeSegPtr != (Vm_Segment *) NIL) {
	    Vm_SegmentDelete(codeSegPtr, procPtr);
	    if (!usedFile) {
		/*
		 * If usedFile is TRUE then the file has already been closed
		 * by Vm_SegmentDelete.
		 */
		(void) Fs_Close(filePtr);
	    }
	} else {
	    Vm_InitCode(filePtr, (Vm_Segment *) NIL, (Vm_ExecInfo *) NIL);
	    (void) Fs_Close(filePtr);
	}
    }
    if (userArgsPtr != (UserArgs *) NIL && userArgsPtr->userMode) {
	if (userArgsPtr->argPtrArray != (char **) NIL) {
	    Vm_MakeUnaccessible((Address) userArgsPtr->argPtrArray,
				userArgsPtr->argLength);
	    userArgsPtr->argPtrArray = (char **)NIL;
	    userArgsPtr->argLength = 0;
	}
	if (userArgsPtr->envPtrArray != (char **) NIL) {
	    Vm_MakeUnaccessible((Address) userArgsPtr->envPtrArray,
				userArgsPtr->envLength);
	    userArgsPtr->envPtrArray = (char **)NIL;
	    userArgsPtr->envLength = 0;
	}
    }
    if (argBuffer != (Address) NIL) {
	free(argBuffer);
    }
    if (argString != (Address) NIL) {
	free(argString);
    }
    /*
     * Restore original arg string.  If we don't do this, then when
     * DoFork() tries to free() the arg string (after this process
     * exits, when some other process gets the then-empty process
     * slot), free() will panic because the original argString was
     * just freed above.
     */
    procPtr->argString = argStringSave;

    if (!importing && (fileName != (char *) NIL) && userArgsPtr->userMode) {
	free(fileName);
	fileName = (char *) NIL;
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
	       extraArgsPtr, objInfoPtr)
    register	char	*buffer;	/* Bytes read in from file.*/
    int			sizeRead;	/* Number of bytes in buffer. */	
    register	Fs_Stream **filePtrPtr;	/* IN/OUT parameter: Exec'd file as 
					 * input, interpreter file as output. */
    char		**argPtrPtr;	/* Pointer to shell argument string. */
    int			*extraArgsPtr;	/* Number of arguments that have to be
					 * added for the intepreter. */
    ProcObjInfo		*objInfoPtr;	/* Place to put obj file info. */
{
    register	char	*strPtr;
    char		*shellNamePtr;
    int			i;
    ReturnStatus	status;
    ProcExecHeader	execHeader;

    (void) Fs_Close(*filePtrPtr);

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

    for (strPtr = &(buffer[2]); isspace(*strPtr); strPtr++) {
    }
    if (*strPtr == '\0') {
	return(PROC_BAD_FILE_NAME);
    }
    shellNamePtr = strPtr;
    while (!isspace(*strPtr) && *strPtr != '\0') {
	strPtr++;
    }
    *extraArgsPtr = 1;

    /*
     * Get a pointer to the arguments if there are any.
     */

    if (*strPtr != '\0') {
	*strPtr = '\0';
	strPtr++;
	while (isspace(*strPtr)) {
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

    status = Fs_Open(shellNamePtr, FS_EXECUTE | FS_FOLLOW, FS_FILE, 0,
		     filePtrPtr);
    if (status != SUCCESS) {
	return(status);
    }

    sizeRead = sizeof(ProcExecHeader);
    status = Fs_Read(*filePtrPtr, (Address)&execHeader, 0, &sizeRead);
    if (status == SUCCESS && sizeRead != sizeof(ProcExecHeader)) {
	status = PROC_BAD_AOUT_FORMAT;
    } else {
	status = ProcGetObjInfo(*filePtrPtr, &execHeader, objInfoPtr);
    }
    if (status != SUCCESS) {
	if(ProcIsObj(*filePtrPtr,1)==SUCCESS) {
	    status = FS_NO_ACCESS;
	} else {
	    status = PROC_BAD_AOUT_FORMAT;
	}
	(void) Fs_Close(*filePtrPtr);
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
SetupVM(procPtr, objInfoPtr, codeFilePtr, usedFile, codeSegPtrPtr, execInfoPtr)
    register	Proc_ControlBlock	*procPtr;
    register	ProcObjInfo		*objInfoPtr;
    Fs_Stream				*codeFilePtr;
    Boolean				usedFile;
    Vm_Segment				**codeSegPtrPtr;
    register	Vm_ExecInfo		*execInfoPtr;
{
    register	Vm_Segment	*segPtr;
    int				numPages;
    int				fileOffset;
    int				pageOffset;
    Boolean			notFound;
    Vm_Segment			*heapSegPtr;
    Fs_Stream			*heapFilePtr;
    Address			heapEnd = (Address) NIL;
    int				realCode = 1;

    if (*codeSegPtrPtr == (Vm_Segment *) NIL) {
	if (objInfoPtr->unixCompat) {
	    execInfoPtr->flags = UNIX_CODE;
	} else {
	    execInfoPtr->flags = 0;
	}
	execInfoPtr->entry = (int)objInfoPtr->entry;
	if (objInfoPtr->heapSize != 0) {
	    execInfoPtr->heapPages = 
			(objInfoPtr->heapSize - 1) / vm_PageSize + 1;
	} else { 
	    execInfoPtr->heapPages = 0;
	}
	if (objInfoPtr->bssSize != 0) {
	    execInfoPtr->heapPages += 
			(objInfoPtr->bssSize - 1) / vm_PageSize + 1;
	}
	execInfoPtr->heapPageOffset = 
			(unsigned)objInfoPtr->heapLoadAddr / vm_PageSize;
	execInfoPtr->heapFileOffset = objInfoPtr->heapFileOffset;
	heapEnd = objInfoPtr->heapLoadAddr + objInfoPtr->heapSize;
	execInfoPtr->heapExcess =
		vm_PageSize - ((unsigned)heapEnd&(vm_PageSize-1));
	if (execInfoPtr->heapExcess == vm_PageSize) {
	    execInfoPtr->heapExcess = 0;
	}
	execInfoPtr->bssFirstPage = 
			(unsigned)objInfoPtr->bssLoadAddr / vm_PageSize;
	if ((unsigned)(heapEnd-1) / vm_PageSize >= execInfoPtr->bssFirstPage) {
	    /*
	     * End of heap, start of bss in same page, so move bss.
	     */
	    execInfoPtr->bssFirstPage = (unsigned)(heapEnd-1)/vm_PageSize + 1;
	}

	if (objInfoPtr->bssSize != 0) {
	    execInfoPtr->bssLastPage = (int) (execInfoPtr->bssFirstPage + 
				   (objInfoPtr->bssSize - 1) / vm_PageSize);
	} else {
	    execInfoPtr->bssLastPage = 0;
	}
	/* 
	 * Set up the code image.
	 */
	if (objInfoPtr->codeSize == 0) {
	    /*
	     * Things work better if we have a code segment.
	     * I'm not sure setting realCode=1 is the right thing, but
	     * I'll try it.  If realCode=0 we may have problems when we
	     * try to clean up the file handle.
	     */
	    realCode = 1;
	    objInfoPtr->codeSize = vm_PageSize;
	}
	numPages = (objInfoPtr->codeSize - 1) / vm_PageSize + 1;
	fileOffset = objInfoPtr->codeFileOffset;
	pageOffset = (unsigned)objInfoPtr->codeLoadAddr / vm_PageSize;
	segPtr = Vm_SegmentNew(VM_CODE, codeFilePtr, fileOffset,
			       numPages, pageOffset, procPtr);
	if (segPtr == (Vm_Segment *) NIL) {
	    Vm_InitCode(codeFilePtr, (Vm_Segment *) NIL, (Vm_ExecInfo *) NIL);
	    (void) Fs_Close(codeFilePtr);
	    return(FALSE);
	}
	Vm_ValidatePages(segPtr, pageOffset, pageOffset + numPages - 1,
			 FALSE, TRUE);
	if (realCode) {
	    Vm_InitCode(codeFilePtr, segPtr, execInfoPtr);
	} else {
	    Vm_InitCode(codeFilePtr, (Vm_Segment *) NIL, (Vm_ExecInfo *) NIL);
	}
	*codeSegPtrPtr = segPtr;
	notFound = TRUE;
    } else {
	notFound = FALSE;
    }

    if (usedFile || notFound) {
	Fsio_StreamCopy(codeFilePtr, &heapFilePtr);
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
	(void) Fs_Close(heapFilePtr);
	return(FALSE);
    }
    Vm_ValidatePages(segPtr, execInfoPtr->heapPageOffset,
		     execInfoPtr->bssFirstPage - 1, FALSE, TRUE);
    if (execInfoPtr->bssLastPage > 0) {
	Vm_ValidatePages(segPtr, execInfoPtr->bssFirstPage,
			 execInfoPtr->bssLastPage, TRUE, TRUE);
    }
    heapSegPtr = segPtr;
    /*
     * Set up a new stack.
     */
    segPtr = Vm_SegmentNew(VM_STACK, (Fs_Stream *) NIL, 0, 
			   1, mach_LastUserStackPage, procPtr);
    if (segPtr == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(*codeSegPtrPtr, procPtr);
	Vm_SegmentDelete(heapSegPtr, procPtr);
	return(FALSE);
    }
    Vm_ValidatePages(segPtr, mach_LastUserStackPage, 
		    mach_LastUserStackPage, TRUE, TRUE);

    Proc_Lock(procPtr);
    procPtr->genFlags |= PROC_NO_VM;
#ifdef sun4
    Mach_FlushWindowsToStack();
    VmMach_FlushCurrentContext();
#endif
    Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_CODE], procPtr);
    Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_HEAP], procPtr);
    Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_STACK], procPtr);
    procPtr->vmPtr->segPtrArray[VM_CODE] = *codeSegPtrPtr;
    procPtr->vmPtr->segPtrArray[VM_HEAP] = heapSegPtr;
    procPtr->vmPtr->segPtrArray[VM_STACK] = segPtr;
    procPtr->genFlags &= ~PROC_NO_VM;
    VmMach_ReinitContext(procPtr);

    /*
     * If heap does not match page boundary, prefetch the partial page
     * if necessary, and zero the rest.
     */
    if (execInfoPtr->heapExcess != 0) {
	if (realCode && (execInfoPtr->flags & UNIX_CODE) == 0) {
	    printf("SetupVM: Warning: Program %s has unaligned heap %s\n",
		    Fsutil_HandleName(&codeFilePtr->hdr),
		    "and should be relinked");
	}
	return ZeroHeapEnd(execInfoPtr);
    }
    return(TRUE);
}


/*
 *----------------------------------------------------------------------
 *
 * ZeroHeapEnd --
 *
 *	Zero out the end of the heap (from the given address to the 
 *	next page boundary).  This routine exists for compatibility 
 *	with oddly-linked binaries.
 *
 * Results:
 *	TRUE if we were successful, FALSE if not (e.g., couldn't bring 
 *	in the last heap page).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
    
static Boolean
ZeroHeapEnd(execInfoPtr)
    Vm_ExecInfo	*execInfoPtr;	/* info about the exec file */
{
    ReturnStatus	status;
    Address	heapEnd;	/* kernel address of end of heap */
    int		bytesToZero;	/* number of bytes to zero out */
    int		bytesAvail;	/* number of bytes accessible */
    int		userHeapEnd;
    

    status = Vm_PageIn((Address) ((execInfoPtr->bssFirstPage-1)*vm_PageSize),
		       FALSE);
    if (status != SUCCESS) {
	printf("SetupVM: heap prefetch failure\n");
	return FALSE;
    }
    bytesToZero = execInfoPtr->heapExcess;
    userHeapEnd = execInfoPtr->bssFirstPage*vm_PageSize-bytesToZero;
    if (debugProcStubs) {
	printf("ZeroHeapEnd: zeroing %x at %x\n", bytesToZero, userHeapEnd);
    }
    Vm_MakeAccessible(VM_READWRITE_ACCESS, bytesToZero, (Address)userHeapEnd,
		      &bytesAvail, (Address *)&heapEnd);
    if (bytesAvail != bytesToZero) {
	printf("SetupVM: can't map heap\n");
	return FALSE;
    }
    bzero((char *)heapEnd, bytesToZero);
    Vm_MakeUnaccessible(heapEnd, bytesToZero);

    return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcExecGetEncapSize --
 *
 *	Return the size of the encapsulated exec state.
 *
 * Results:
 *	SUCCESS is returned directly; the size of the encapsulated state
 *	is returned in infoPtr->size.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
ProcExecGetEncapSize(procPtr, hostID, infoPtr)
    Proc_ControlBlock *procPtr;			/* process being migrated */
    int hostID;					/* host to which it migrates */
    Proc_EncapInfo *infoPtr;			/* area w/ information about
						 * encapsulated state */
{
    ExecEncapState *encapPtr;

    encapPtr = (ExecEncapState *) procPtr->remoteExecBuffer;
    encapPtr->hdr.fileNameLength =
	Byte_AlignAddr(strlen(encapPtr->hdr.fileName) + 1);
    encapPtr->hdr.argLength =
	Byte_AlignAddr(strlen(encapPtr->hdr.argString) + 1);
    infoPtr->size = encapPtr->hdr.size + encapPtr->hdr.fileNameLength +
	encapPtr->hdr.argLength;
    return(SUCCESS);	
}


/*
 *----------------------------------------------------------------------
 *
 * ProcExecEncapState --
 *
 *	Encapsulate the information needed to perform a remote exec,
 *	and return it in the buffer provided.
 *
 * Results:
 *	SUCCESS.  The buffer is filled.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
ProcExecEncapState(procPtr, hostID, infoPtr, bufPtr)
    register Proc_ControlBlock 	*procPtr;  /* The process being migrated */
    int hostID;				   /* host to which it migrates */
    Proc_EncapInfo *infoPtr;		   /* area w/ information about
					    * encapsulated state */
    Address bufPtr;			   /* Pointer to allocated buffer */
{
    ExecEncapState *encapPtr;

    encapPtr = (ExecEncapState *) procPtr->remoteExecBuffer;

    bcopy((Address) encapPtr, bufPtr, encapPtr->hdr.size);
    bufPtr += encapPtr->hdr.size;
    (void) strncpy(bufPtr, encapPtr->hdr.fileName, encapPtr->hdr.fileNameLength);
    bufPtr += encapPtr->hdr.fileNameLength;
    (void) strncpy(bufPtr, encapPtr->hdr.argString, encapPtr->hdr.argLength);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcExecDeencapState --
 *
 *	Get remote exec information from a Proc_ControlBlock from another host.
 *	The information is contained in the parameter ``buffer''.
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Memory is allocated for argString, which is kept around while the
 *	process is alive.
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
ProcExecDeencapState(procPtr, infoPtr, bufPtr)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    Proc_EncapInfo *infoPtr;		  /* information about the buffer */
    Address bufPtr;			  /* buffer containing data */
{
    ExecEncapState *encapPtr;
    char *argString;

    procPtr->remoteExecBuffer = malloc(infoPtr->size);
    bcopy(bufPtr, procPtr->remoteExecBuffer, infoPtr->size);

    encapPtr = (ExecEncapState *) procPtr->remoteExecBuffer;
    encapPtr->hdr.fileName = procPtr->remoteExecBuffer + encapPtr->hdr.size;
    argString = encapPtr->hdr.fileName + encapPtr->hdr.fileNameLength;
    encapPtr->hdr.argString = malloc(encapPtr->hdr.argLength);
    (void) strncpy(encapPtr->hdr.argString, argString, encapPtr->hdr.argLength);
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcExecFinishMigration --
 *
 *	Free up resources after a remote exec.  This includes buffers
 *      used to store information about a remote exec
 *	between the time of the system call and the time the migration
 *	completes: namely, the buffer containing the user's stack,
 *	and the file name to exec (reached via that buffer).  Also,
 *	free the virtual memory segments used by the process. 
 *
 * Results:
 *	SUCCESS.
 *
 * Side effects:
 *	Memory is freed. The segments are freed.
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
ReturnStatus
ProcExecFinishMigration(procPtr, hostID, infoPtr, bufPtr, failure)
    register Proc_ControlBlock 	*procPtr; /* The process being migrated */
    int hostID;				  /* Host to which it migrated */
    Proc_EncapInfo *infoPtr;		  /* Information about the buffer */
    Address bufPtr;			  /* Buffer containing data */
    Boolean failure;			  /* Whether a failure occurred */
{
    ExecEncapState *encapPtr;
    int i;

    encapPtr = (ExecEncapState *) procPtr->remoteExecBuffer;
    free(encapPtr->hdr.fileName);
    free(procPtr->remoteExecBuffer);
    Proc_Lock(procPtr);
    procPtr->remoteExecBuffer = (Address) NIL;
    procPtr->genFlags |= PROC_NO_VM;
    Proc_Unlock(procPtr);
    for (i = VM_CODE; i <= VM_STACK; i++) {
	Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[i], procPtr);
    }
    return(SUCCESS);
}


/*
 *----------------------------------------------------------------------
 *
 * ProcDoRemoteExec --
 *
 *	Do an exec of a process that's come to this machine from another
 *	one.  This is the PC at which the process resumes after migration,
 *	at which point it has no VM set up.
 *
 * Results:
 *	None.  This routine doesn't return, since upon error the process
 *	exits.
 *
 * Side effects:
 *	An exec is performed.
 *
 *----------------------------------------------------------------------
 */

void
ProcDoRemoteExec(procPtr)
    register Proc_ControlBlock *procPtr; /* Process control block, locked
					  * on entry */
{
    char *fileName = (char *) NIL;
    ReturnStatus status;
    ExecEncapState *encapPtr;

    /*
     * Set up dummy segments so that DoExec can work properly.
     */

    procPtr->genFlags &= ~PROC_REMOTE_EXEC_PENDING;
    procPtr->vmPtr->segPtrArray[VM_CODE] = 
				Vm_SegmentNew(VM_CODE, (Fs_Stream *) NIL, 0,
					          1, 0, procPtr);
    if (procPtr->vmPtr->segPtrArray[VM_CODE] == (Vm_Segment *) NIL) {
	status = PROC_NO_SEGMENTS;
	Proc_Unlock(procPtr);
	goto failure;
    }

    procPtr->vmPtr->segPtrArray[VM_HEAP] =
		    Vm_SegmentNew(VM_HEAP, (Fs_Stream *) NIL, 0, 1, 1, procPtr);
    if (procPtr->vmPtr->segPtrArray[VM_HEAP] == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_CODE], procPtr);
	status = PROC_NO_SEGMENTS;
	Proc_Unlock(procPtr);
	goto failure;
    }

    procPtr->vmPtr->segPtrArray[VM_STACK] =
		    Vm_SegmentNew(VM_STACK, (Fs_Stream *) NIL, 
				   0 , 1, mach_LastUserStackPage, procPtr);
    if (procPtr->vmPtr->segPtrArray[VM_STACK] == (Vm_Segment *) NIL) {
	Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_CODE], procPtr);
	Vm_SegmentDelete(procPtr->vmPtr->segPtrArray[VM_HEAP], procPtr);
	status = PROC_NO_SEGMENTS;
	Proc_Unlock(procPtr);
	goto failure;
    }

    VmMach_ReinitContext(procPtr);

    encapPtr = (ExecEncapState *) procPtr->remoteExecBuffer;
    fileName = encapPtr->hdr.fileName;
    Proc_Unlock(procPtr);
    status = DoExec(fileName, (UserArgs *) NIL, &encapPtr, FALSE);
    /*
     * If the exec failed, then exit.  
     */
    failure:
    if (proc_MigDebugLevel > 0) {
	printf("Remote exec of %s failed: %s\n", fileName,
	       Stat_GetMsg(status));
    }
    Proc_ExitInt(PROC_TERM_DESTROYED, SIG_KILL, 0);
    /*
     * NOTREACHED
     */
}


/*
 * The following table contains the functions to check if an executable
 * is of a particular machine type.
 */
int	hostFmt = HOST_FMT;
char * (*machType[]) _ARGS_((int bufferSize, char *buffer, int *magic, 
	int *syms, char **other)) =  {
    machType68k,
    machTypeSparc,
    machTypeSpur,
    machTypeMips,
    machTypeSymm,
};
/*
 *----------------------------------------------------------------------
 *
 * ProcIsObj -
 *
 *	Check if the process is an a.out file.  If doErr is set, an
 *	error message will be printed if the file is an a.out file.
 *	This routine is to be called if the file cannot be execed, to
 *	see if it's the wrong a.out type.
 *	This code is based on the Sprite a.out checking routines for
 *	the file program.
 *
 * Results:
 *	SUCCESS if the file is an a.out file.
 *	FAILURE if the file is not an a.out file.
 *
 * Side effects:
 *	An error may printed in the syslog if doErr is set.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
ProcIsObj(streamPtr, doErr)
    Fs_Stream	*streamPtr;	/* Stream pointer for obj file. */
    int		doErr;		/* 1 if want error messages. */
{
    ReturnStatus	status;
    char		*buffer;
    int			hdrSize = BUFSIZ;
    int			i;
    int			magic;
    char		*name;
    int			syms;
    char		*other;

    buffer = (char *)malloc(hdrSize);
    status = Fs_Read(streamPtr, (Address)buffer, 0, &hdrSize);
    if (status != SUCCESS) {
	return FAILURE;
    }
    for (i=0; i < sizeof(machType)/sizeof(*machType); i++) {
	name = machType[i](hdrSize, (const char *) buffer, &magic, &syms,
		&other);
	if (name != NULL) {
	    if (doErr) {
		printf("Proc_Exec: Can't run %s ", name);
		switch (magic) {
		    case 0407:
			printf("OMAGIC");
			break;
		    case 0410:
			printf("NMAGIC");
			break;
		    case 0413:
			printf("ZMAGIC");
			break;
		    case 0414:
			printf("SPRITE_ZMAGIC");
			break;
		    case 0415:
			printf("UNIX_ZMAGIC");
			break;
		    case 0443:
			printf("LIBMAGIC");
			break;
		    default:
			printf("(0%03o)", magic);
			break;
		}
		if (*other != '\0') {
		    printf(" %s", other);
		}
		printf(" executable file on %s.\n", mach_MachineType);
	    }
	    return SUCCESS;
	}
    }
    return FAILURE;
}
