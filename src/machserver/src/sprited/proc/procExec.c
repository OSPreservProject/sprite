/* procExec.c --
 *
 *	Routines to exec a program.  There is no monitor required in this
 *	file.  Access to the proc table is synchronized by locking the PCB
 *	when modifying the genFlags field.
 *
 * Copyright (C) 1985, 1988, 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procExec.c,v 1.23 92/07/16 18:07:39 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <spriteTime.h>
#include <byte.h>
#include <ckalloc.h>
#include <ctype.h>
#include <list.h>
#include <mach.h>
#include <mach_error.h>
#include <mach/machine/vm_param.h>
#include <status.h>
#include <string.h>
#include <user/fs.h>

#include <fs.h>
#include <main.h>		/* main_DebugFlag */
#include <proc.h>
#include <procInt.h>
#include <procMach.h>
#include <procMachInt.h>
#include <rpc.h>
#include <sig.h>
#include <spriteSrvServer.h>
#include <sys.h>		/* sys_CallProfiling */
#include <timer.h>		/* Timer_GetTimeOfDay */
#include <utils.h>
#include <vm.h>
#include <user/vmStat.h>

/*
 * This will go away when libc is changed.
 */
#ifndef PROC_MAX_ENVIRON_LENGTH
#define PROC_MAX_ENVIRON_LENGTH (PROC_MAX_ENVIRON_NAME_LENGTH + \
				 PROC_MAX_ENVIRON_VALUE_LENGTH)
#endif /*  PROC_MAX_ENVIRON_LENGTH */

/* 
 * Maximum number of argument to an interpreter: script name, interpreter 
 * flags, script name (again), and nil pointer to end the array.
 */
#define MAX_INTERP_ARGS		4

/* 
 * When copying arguments and environment variables in from user space, we 
 * temporarily put them on a linked list.  Once we have the strings all on 
 * the list, we can figure out how much space they'll take, etc.
 */

typedef struct {
    List_Links	links;
    Address	stringPtr;
    int		stringLen;
} ArgListElement;

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
     * argv pointers go here
     * envp pointers go here
     * argv strings go here
     * envp strings go here
     */
} ExecEncapState;

/*
 * Define a structure to hold all the information about arguments
 * and environment variables (pointer and length).  This makes it easier
 * to pass a NIL pointer for the whole thing, and to keep them in one
 * place.  The number of arguments/environment pointers includes the 
 * nil pointer at the end of the array.
 */

typedef struct {
    Boolean	userMode;	/* TRUE if the arguments are in user space */
    char	**argPtrArray;	/* The array of argument pointers. */
    int		numArgs;	/* The number of arguments in the argArray. */
    int		argLength;	/* actual bytes in argArray */
    char	**envPtrArray;	/* The array of environment pointers. */
    int		numEnvs;	/* The number of arguments in the envArray. */
    int		envLength;	/* actual bytes in envArray */
} UserArgs;

/* 
 * This flag controls debug printf's for the argc, argv, and the 
 * environment strings.  Higher numbers yield more printfs.
 */
static int argDebug = 0;

/* 
 * When a user process starts, it has three regions of memory: code, 
 * heap, and stack.  The size and location of the code are specified 
 * by the object file.  The location of the heap is partially 
 * specified by the object file.  We will create large heap and stack 
 * segments, and then initially only map portions of them into the 
 * process address space.
 */

/* 
 * Instrumentation.
 */

static Time doExecTime;		/* time spent in DoExec */
static Time partATime;
static Time partBTime;		/* XXX actually partA + partB */
static Time setupCodeTime;	/* time spent in SetupCode */
static Time setupArgsTime;	/* time in SetupArgs */
static Time setupVmTime;	/* time spent in SetupVM */
static Time doCodeTime;
static Time doHeapTime;
static Time doStackTime;
static Time stackCopyOutTime;

/* GrabArgArray instrumentation */
static Time grabArgsTime;
static Time argsAccessTime;
static int numStrings;		/* number of strings copied in */
static unsigned long stringBytes; /* sum of string lengths */

/*
 * Forward declarations.
 */
static ReturnStatus DoCode _ARGS_((Proc_LockedPCB *procPtr,
		Proc_ObjInfo *execInfoPtr, Vm_Segment *codeSegPtr));
static ReturnStatus DoExec _ARGS_((Proc_ControlBlock *procPtr,
		char *execPath, UserArgs *userArgsPtr, Boolean kernExec,
		Boolean debugMe));
static ReturnStatus DoHeap _ARGS_((Proc_LockedPCB *procPtr,
		Proc_ObjInfo *execInfoPtr, Fs_Stream *objFilePtr,
		char *objFileName));
static ReturnStatus DoStack _ARGS_((Proc_LockedPCB *procPtr,
		Proc_ObjInfo *execInfoPtr, ExecEncapState *encapPtr));
static ReturnStatus GrabArgArray _ARGS_((int maxLength, Boolean userProc,
		char **extraArgArray, char **argPtrArray, int *numArgsPtr,
		List_Links *argList, int *realLengthPtr,
		int *paddedLengthPtr));
static Address HeapStart _ARGS_((Proc_ObjInfo *execInfoPtr));
static void InstrumentSwapSegments _ARGS_((Proc_LockedPCB *procPtr,
		Proc_ObjInfo *execInfoPtr));
static ReturnStatus SetupCode _ARGS_((char *execName, UserArgs *userArgsPtr,
		Fs_Stream **filePtrPtr, Vm_Segment **codeSegPtrPtr,
		Proc_ObjInfo *execInfoPtr, ExecEncapState **encapPtrPtr,
		char **argStringPtr));
static ReturnStatus SetupInterpret _ARGS_((char *buffer, int sizeRead,
		Fs_Stream **interpPtrPtr, char *interpName,
		char **argStringPtr, Proc_ObjInfo *objInfoPtr));
static ReturnStatus SetupVM _ARGS_((Proc_LockedPCB *procPtr,
		Vm_Segment *codeSegPtr, Proc_ObjInfo *execInfoPtr,
		ExecEncapState *encapPtr));
static Address StackStart _ARGS_((Proc_ObjInfo *execInfoPtr));


/*
 *----------------------------------------------------------------------
 *
 * Proc_OldExecEnvStub --
 *
 *	Do an exec on the local host.
 *	Note: this function is deprecated.  Remove it when you no longer 
 *	wish to support binaries that use the old exec call.
 *
 * Results:
 *	MIG_NO_REPLY: the exec was successful, so no reply message should 
 *	be sent.
 *	KERN_SUCCESS: there was an error; the Sprite code for the error is 
 *	returned in *statusPtr, and the "pending signals" flag is filled 
 *	in.
 *
 * Side effects:
 *	The process's PROC_NEEDS_WAKEUP flag is set if successful.
 *
 *----------------------------------------------------------------------
 */
    
kern_return_t
Proc_OldExecEnvStub(serverPort, fileName, nameLength, argPtrArray,
		 envPtrArray, debugMe, statusPtr, sigPendingPtr)
    mach_port_t	serverPort;	/* the server request port */
    Fs_PathName fileName;	/* The name of the file to exec. */
    mach_msg_type_number_t nameLength; /* extra parameter to appease MIG */
    vm_address_t argPtrArray;	/* The array of arguments to the exec'd 
				 * program. (user address) */
    vm_address_t envPtrArray;	/* The array of environment variables
				 * of the form NAME=VALUE. (user address) */ 
    Boolean	debugMe;	/* TRUE means that the process is 
				 * to be sent a SIG_DEBUG signal before
    				 * executing its first instruction. */
    ReturnStatus *statusPtr;	/* OUT: Sprite status code */
    boolean_t *sigPendingPtr;	/* OUT: is there a signal pending */
{
#ifdef lint
    serverPort = serverPort;
    nameLength = nameLength;
#endif

    *statusPtr = Proc_Exec(fileName, (char **)argPtrArray,
			   (char **)envPtrArray, debugMe, 0);
    if (*statusPtr != SUCCESS) {
	*sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
	return KERN_SUCCESS;
    } else {
	return MIG_NO_REPLY;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_ExecEnvStub --
 *
 *	Do an exec on the local host.
 *	Note: this function is deprecated.  It can be ifdef'd out when you
 *	no longer wish to support binaries that use the old exec call.
 *	However, you should hold onto the migration-related code.
 *
 * Results:
 *	MIG_NO_REPLY: the exec was successful, so no reply message should 
 *	be sent.
 *	KERN_SUCCESS: there was an error; the Sprite code for the error is 
 *	returned in *statusPtr, and the "pending signals" flag is filled 
 *	in.
 *
 * Side effects:
 *	The process's PROC_NEEDS_WAKEUP flag is set if successful.
 *
 *----------------------------------------------------------------------
 */

kern_return_t
Proc_ExecEnvStub(serverPort, fileName, nameLength, argOffsetArray, numArgs,
		    argStrings, argStringsSize, envOffsetArray, numEnvs,
		    envStrings, envStringsSize, debugMe, statusPtr,
		    sigPendingPtr)
    mach_port_t		serverPort;	/* the server request port */
    Fs_PathName		fileName;	/* The name of the file to exec. */
    mach_msg_type_number_t nameLength;	/* extra parameter to appease MIG */
    Proc_OffsetTable	argOffsetArray;	/* offsets into argument strings */
    mach_msg_type_number_t numArgs;	/* number of argument strings */
    Proc_Strings 	argStrings;	/* argument strings */
    mach_msg_type_number_t argStringsSize; /* number of bytes in arg strings */
    Proc_OffsetTable	envOffsetArray;	/* offsets into environment strings */
    mach_msg_type_number_t numEnvs;	/* number of environment strings */
    Proc_Strings	envStrings;	/* environment strings */
    mach_msg_type_number_t envStringsSize; /* number of bytes in env strings */
    Boolean		debugMe;	/* TRUE means that the process is 
					 * to be sent a SIG_DEBUG signal before
					 * executing its first instruction. */
    ReturnStatus	*statusPtr;	/* OUT: Sprite status code */
    boolean_t		*sigPendingPtr;	/* OUT: is there a signal pending */
{
    char **argPtrArray;			/* pointers into arg strings */
    char **envPtrArray;			/* pointers into env strings */
    UserArgs userArgs;
    Proc_ControlBlock *procPtr = Proc_GetCurrentProc();
    int i;

#ifdef lint
    serverPort = serverPort;
    nameLength = nameLength;
    argStringsSize = argStringsSize;
    envStringsSize = envStringsSize;
#endif

    /* 
     * Convert the string offsets into actual string pointers.  Allow room 
     * for a null pointer at the end.
     */
    argPtrArray = (char **)ckalloc((numArgs + 1) * sizeof(char *));
    envPtrArray = (char **)ckalloc((numEnvs + 1) * sizeof(char *));
    for (i = 0; i < numArgs; i++) {
	argPtrArray[i] = argStrings + argOffsetArray[i];
    }
    argPtrArray[numArgs] = (char *)NIL;
    for (i = 0; i < numEnvs; i++) {
	envPtrArray[i] = envStrings + envOffsetArray[i];
    }
    envPtrArray[numEnvs] = (char *)NIL;

    userArgs.userMode = FALSE;
    userArgs.argPtrArray = argPtrArray;
    userArgs.numArgs = numArgs;
    userArgs.argLength = (numArgs+1) * sizeof(char *);
    userArgs.envPtrArray = envPtrArray;
    userArgs.numEnvs = numEnvs;
    userArgs.envLength = (numEnvs+1) * sizeof(char *);

    *statusPtr = DoExec(procPtr, fileName, &userArgs, FALSE, debugMe);

    if (*statusPtr != SUCCESS) {
	*sigPendingPtr = Sig_Pending(Proc_GetCurrentProc());
	return KERN_SUCCESS;
    } else {
	return MIG_NO_REPLY;
    }
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
 *	For local exec's, returns the usual Sprite status code.
 *	For remote exec's, SUCCESS is returned, and the calling routine
 *	arranges for the process to hit a migration signal before
 *	continuing.
 *
 * Side effects:
 *	The argument & environment arrays are made accessible.
 *	The DoExec routine makes the arrays unaccessible again.  The 
 *	process's PROC_NEEDS_WAKEUP flag is set if successful.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_Exec(fileName, argPtrArray, envPtrArray, debugMe, host)
    char	*fileName;	/* The name of the file to exec. */
    char	**argPtrArray;	/* The array of arguments to the exec'd 
				 * program. (user address) */
    char	**envPtrArray;	/* The array of environment variables
				 * of the form NAME=VALUE. (user addr.) */ 
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
    ReturnStatus	status;
    ExecEncapState	**encapPtrPtr;
    Proc_ControlBlock 	*procPtr;
    
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
#ifdef SPRITED_MIGRATION	
	encapPtrPtr = &encapPtr;
#else
	return PROC_MIGRATION_REFUSED;
#endif
    } else {
	encapPtrPtr = (ExecEncapState **) NIL;
    }
    procPtr = Proc_GetCurrentProc();
    status = DoExec(procPtr, fileName, &userArgs, FALSE, debugMe);
#ifdef SPRITED_MIGRATION
    if (status == SUCCESS && host != 0) {
	/*
	 * Set up the process to migrate.
	 */
	Proc_Lock(procPtr);
	status = ProcInitiateMigration(procPtr, host);
	if (status == SUCCESS) {
	    procPtr->remoteExecBuffer = (Address) encapPtr;
	    Proc_FlagMigration(procPtr, host, TRUE);
	} else {
	    ckfree((Address) encapPtr);
	}
	Proc_Unlock(procPtr);
    }
#endif /* SPRITED_MIGRATION */

#ifdef lint
#ifndef SPRITED_MIGRATION
    encapPtrPtr = encapPtrPtr;
#endif
#endif
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * Proc_KernExec --
 *
 *	Facade over DoExec for exec'ing the first user process.
 *
 * Results:
 *	Return status.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Proc_KernExec(procPtr, execPath, argPtrArray)
    Proc_ControlBlock *procPtr;	/* process to set up */
    char *execPath;		/* name of program to exec */
    char **argPtrArray;		/* argv argument array to pass to process */
{
    UserArgs userArgs;

    /* 
     * Set up the argument/environment arrays.  We don't have to be 
     * accurate about the argument count or space, because the end of the 
     * argument array is marked with a NIL pointer.  There are no 
     * environment variables to set.
     */
    userArgs.userMode = FALSE;
    userArgs.argPtrArray = argPtrArray;
    userArgs.numArgs = PROC_MAX_EXEC_ARGS;
    userArgs.argLength = PROC_MAX_EXEC_ARGS * sizeof(Address);
    userArgs.envPtrArray = (char **) NIL;
    userArgs.numEnvs = 0;
    userArgs.envLength = 0;

    return DoExec(procPtr, execPath, &userArgs, TRUE, FALSE);
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
 *	procMach_MaxUserStackAddr - 1.  If the exec is performed on a 
 *	different machine, the pointers in argv and envp must be adjusted
 *	by the relative values of procMach_MaxUserStackAddr.
 *
 *	The format of the structure is defined by ExecEncapState above.
 *		
 *
 * Results:
 *	A ReturnStatus is returned.  Any non-SUCCESS result indicates a failure
 *	that should be returned to the user.
 *	In addition, a pointer to the encapsulated stack is returned,
 *	as well as its size.  The caller is responsible for freeing the 
 *	encapsulated stack and the "ps" argument string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
SetupArgs(userArgsPtr, extraArgArray, encapPtrPtr, argStringPtr)
    UserArgs *userArgsPtr;	/* Information for taking args
				 * and environment. */ 
    char     **extraArgArray;	/* Any arguments that should be
	     		 	 * inserted prior to argv[1] */
    ExecEncapState  **encapPtrPtr; /* OUT: the encapsulated stack. */
    char     **argStringPtr;	/* OUT: argument list as string (for ps) */
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
    int		usp;		/* simulated user stack ptr; for copying 
				 * strings  */
    int					paddedArgLength;
    int					paddedEnvLength;
    int					bufSize;
    Address				buffer;
    int					stackSize;
    ExecEncapHeader			*hdrPtr;
    ExecEncapState			*encapPtr;
    Proc_ControlBlock			*procPtr; /* for debugging */
    Time startTime, endTime;	/* instrumentation */

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    procPtr = (argDebug > 0 ? Proc_GetCurrentProc() : NULL);

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
     * over argc and argv = (1 + (argc + 1)) * sizeof(Address).  The extra
     * "1" is for the null argument at the end of argv.  Below all that
     * stuff on the stack come the environment and argument strings
     * themselves.
     */
    bufSize = sizeof(ExecEncapState)
	+ (numArgs + numEnvs + 2) * sizeof(Address)
	+ paddedArgLength + paddedEnvLength;
    buffer = ckalloc((unsigned)bufSize);
    encapPtr = (ExecEncapState *) buffer;
    *encapPtrPtr = encapPtr;
    hdrPtr = &encapPtr->hdr;
    hdrPtr->size = bufSize;
    hdrPtr->fileName = NULL;
    hdrPtr->fileNameLength = 0;
    hdrPtr->argString = NULL;
    hdrPtr->argLength = 0;
    stackSize = Byte_AlignAddr((bufSize - sizeof(ExecEncapHeader)));
    hdrPtr->base = procMach_MaxUserStackAddr - stackSize;
    if (argDebug > 0) {
	printf("pid %x: argc @ 0x%x = %d\n", procPtr->processID,
	       hdrPtr->base, numArgs);
    }
    encapPtr->argc = numArgs;
    newArgPtrArray = (char **) (buffer + sizeof(ExecEncapState));
    newEnvPtrArray = (char **) ((int) newArgPtrArray +
				(numArgs + 1) * sizeof(Address));
    argBuffer = (Address) ((int) newEnvPtrArray +
			   (numEnvs + 1) * sizeof(Address));
    envBuffer =  (argBuffer + paddedArgLength);
				
    argNumber = 0;

    /* 
     * Make usp contain the user address of a string on the stack.
     */
    usp = (int)hdrPtr->base + (int) argBuffer - (int) &encapPtr->argc;

    /*
     * Create the buffer for the "ps" argument string. 
     */
    argString = ckalloc((unsigned)argStringLength + 1);
    *argStringPtr = argString;

    while (!List_IsEmpty(&argList)) {
	argListPtr = (ArgListElement *) List_First(&argList);
	/*
	 * Copy the argument to the stack and set the argv pointer.
	 */
	bcopy((Address) argListPtr->stringPtr, (Address) argBuffer,
		    argListPtr->stringLen);
	newArgPtrArray[argNumber] = (char *) usp;
	if (argDebug > 0) {
	    printf("pid %x: argv[%d] = 0x%x", procPtr->processID,
		   argNumber, usp);
	    if (argDebug > 1) {
		printf(" (%s)", argListPtr->stringPtr);
	    }
	    printf("\n");
	}
	argBuffer += Byte_AlignAddr(argListPtr->stringLen);
	usp += Byte_AlignAddr(argListPtr->stringLen);
	/* 
	 * Copy the argument to the "ps" argument string.
	 */
	bcopy((Address) argListPtr->stringPtr, argString,
		    argListPtr->stringLen - 1);
	argString[argListPtr->stringLen - 1] = ' ';
	argString += argListPtr->stringLen;
	/*
	 * Clean up
	 */
	List_Remove((List_Links *) argListPtr);
	ckfree((Address) argListPtr->stringPtr);
	ckfree((Address) argListPtr);
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
	if (argDebug > 0) {
	    printf("pid %x: envp[%d] = 0x%x", procPtr->processID,
		   argNumber, usp);
	    if (argDebug > 1) {
		printf(" (%s)", argListPtr->stringPtr);
	    }
	    printf("\n");
	}
	envBuffer += Byte_AlignAddr(argListPtr->stringLen);
	usp += Byte_AlignAddr(argListPtr->stringLen);
	/*
	 * Clean up
	 */
	List_Remove((List_Links *) argListPtr);
	ckfree((Address) argListPtr->stringPtr);
	ckfree((Address) argListPtr);
	argNumber++;
    }
    newEnvPtrArray[argNumber] = (char *) USER_NIL;

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, setupArgsTime, &setupArgsTime);
    }

    /*
     * We're done here.  Leave it to the caller to free the copy of the
     * stack after copying it to user space.
     */
    return(SUCCESS);
    
execError:
    printf("SetupArgs failed: %s\n", Stat_GetMsg(status)); /* DEBUG */
    /*
     * The exec failed while copying in the arguments.  Free any
     * arguments or environment variables that were copied in.
     */
    while (!List_IsEmpty(&argList)) {
	argListPtr = (ArgListElement *) List_First(&argList);
	List_Remove((List_Links *) argListPtr);
	ckfree((Address) argListPtr->stringPtr);
	ckfree((Address) argListPtr);
    }
    while (!List_IsEmpty(&envList)) {
	argListPtr = (ArgListElement *) List_First(&envList);
	List_Remove((List_Links *) argListPtr);
	ckfree((Address) argListPtr->stringPtr);
	ckfree((Address) argListPtr);
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
    register	ArgListElement		*argListPtr;
    register	char	**argPtr; /* element of argPtrArray */
    register	int	argNumber;
    ReturnStatus	status = SUCCESS;
    char		*stringPtr = NULL; /* single string when building 
					    * array */
    int			stringLength; /* length of string that stringPtr
				       * refers to, including trailing null */
    int			mappedLength = 0; /* length of region returned by 
					   * Vm_MakeAccessible */ 
    Time startTime, endTime;	/* instrumentation */
    Time startAccessTime, endAccessTime; /* instrumentation */
    
    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    /* 
     * Count up the number of extra arguments to stick at the front of the 
     * list. 
     */
    if (extraArgArray != (char **) NIL) {
	for (extraArgs = 0; extraArgArray[extraArgs] != (char *)NIL;
	     extraArgs++) {
	}
    } else {
	extraArgs = 0;
    }
    *numArgsPtr += extraArgs;

    for (argNumber = 0, argPtr = argPtrArray; 
	 argNumber < *numArgsPtr;
	 argNumber++) {

	/* 
	 * Make stringPtr and stringLength refer to the next string to put 
	 * into the array.
	 */
	
	if ((argNumber > 0 || argPtrArray == (char **) NIL) && extraArgs > 0) {
	    stringPtr = extraArgArray[0];
	    stringLength = strlen(stringPtr) + 1;
	    extraArgArray++;
	    extraArgs--;
	} else {
	    if (!userProc) {
		if (*argPtr == (char *) NIL) {
		    break;
		}
		stringPtr = *argPtr;
		stringLength = strlen(stringPtr) + 1;
	    } else {
		if (*argPtr == (char *) USER_NIL) {
		    break;
		}

		if (sys_CallProfiling) {
		    Timer_GetTimeOfDay(&startAccessTime, (int *)NULL,
				       (Boolean *)NULL);
		} else {
		    startAccessTime = time_ZeroSeconds;
		}
		Vm_MakeAccessible(VM_READONLY_ACCESS, maxLength, *argPtr,
				  &mappedLength, &stringPtr);
		if (sys_CallProfiling && !Time_EQ(startAccessTime,
						  time_ZeroSeconds)) {
		    Timer_GetTimeOfDay(&endAccessTime, (int *)NULL,
				       (Boolean *)NULL);
		    Time_Subtract(endAccessTime, startAccessTime,
				  &endAccessTime);
		    Time_Add(endAccessTime, argsAccessTime, &argsAccessTime);
		}

		if (stringPtr == NULL) {
		    if (argDebug > 1) {
			printf("GrabArgArray: couldn't map string at 0x%x\n",
			       *argPtr);
		    }
		    status = SYS_ARG_NOACCESS;
		    goto bailOut;
		}
		stringLength = strlen(stringPtr) + 1;
		numStrings++;	/* DEBUG */
		if (stringBytes + stringLength < stringBytes) {
		    printf("GrabArgArray: string length overflow.\n");
		}
		stringBytes += stringLength; /* DEBUG */
	    }
	    /*
	     * Move to the next argument.
	     */
	    argPtr++;
	}
	
	/*
	 * Put this string onto the argument list.
	 */
	argListPtr = (ArgListElement *)ckalloc(sizeof(ArgListElement));
	argListPtr->stringPtr = ckalloc((unsigned)stringLength);
	argListPtr->stringLen = stringLength;
	List_InitElement((List_Links *) argListPtr);
	List_Insert((List_Links *) argListPtr, LIST_ATREAR(argList));

	/*
	 * Calculate the room on the stack needed for this string.
	 * Make it double-word aligned to make access efficient on
	 * all machines.  Increment the amount needed to save the argument
	 * list (the same total length, but without the padding).
	 */
	paddedTotalLength += Byte_AlignAddr(stringLength);
	totalLength += stringLength;
	bcopy((Address) stringPtr, (Address) argListPtr->stringPtr,
	      stringLength);

	/* 
	 * Deallocate the original string if we had to get it from user 
	 * space. 
	 */
	if (mappedLength != 0) {
	    Vm_MakeUnaccessible(stringPtr, mappedLength);
	    mappedLength = 0;
	}
    }
    
    
    if (realLengthPtr != (int *) NIL) {
	if (totalLength > *realLengthPtr) {
	    /*
	     * Would really like to flag "argument too long" here.
	     * Also, should we check after every argument?
	     */
	    status = GEN_INVALID_ARG;
	    goto bailOut;
	}
	*realLengthPtr = totalLength;
    }
    if (paddedLengthPtr != (int *) NIL) {
	*paddedLengthPtr = paddedTotalLength;
    }
    *numArgsPtr = argNumber;
    
 bailOut:
    if (status != SUCCESS) {
	/*
	 * We hit an error while copying in the arguments.  Free any
	 * arguments that were copied in.
	 */
	while (!List_IsEmpty(argList)) {
	    argListPtr = (ArgListElement *) List_First(argList);
	    List_Remove((List_Links *) argListPtr);
	    ckfree((Address) argListPtr->stringPtr);
	    ckfree((Address) argListPtr);
	}
    }

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, grabArgsTime, &grabArgsTime);
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * DoExec --
 *
 *	Set up a user process with a binary.  Flushes the old VM for the
 *	process, copies in the executable, and sets the thread state.
 *
 * Results:
 *	status code.
 *
 * Side effects:
 *	The UserArgs arrays are freed if the flag is set saying that they
 *	were copied in from user space.  If successful and the exec was for 
 *	a user request, sets the PROC_NEEDS_WAKEUP flag.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DoExec(procPtr, execPath, userArgsPtr, kernExec, debugMe)
    Proc_ControlBlock *procPtr;	/* process to set up */
    char *execPath;		/* path to program to run */
    UserArgs *userArgsPtr;	/* Information for taking args
				 * and environment, or NIL. */
    Boolean kernExec;		/* doing (initial) exec on behalf of 
				 * "kernel" process */
    Boolean	debugMe;	/* TRUE means that the process is to be 
				 * sent a SIG_DEBUG signal before
    				 * executing its first instruction. */
{
    Fs_Stream *filePtr = NULL;		/* the program to run */
    Proc_ObjInfo execInfo;		/* a.out header information */
    char *argStringSave = NULL;		/* the process's current arg string */
    char *argString = NULL;		/* the process's new arg string */
    Vm_Segment *codeSegPtr = NULL;	/* new code segment */
    ExecEncapState *encapPtr = NULL;	/* stack information & contents */
    int uid = -1;			/* user ID to use; -1 if not setuid */
    int gid = -1;			/* group ID to use; -1 if not setgid */
    Boolean noReturn = FALSE;		/* can't do an error return 'cuz the 
					 * process has been trashed */
    Time startTime, endTime;		/* instrumentation */
    Time pointATime, pointBTime; /* and more */
    ReturnStatus status = SUCCESS;
    kern_return_t kernStatus;

    /* Return through "execError", so that cleanup happens. */

#ifdef SPRITED_PROFILING
    /* Turn off profiling */
    if (procPtr->Prof_Scale != 0) {
	Prof_Disable(procPtr);
    }
#endif

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, NULL, NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    /*
     * Save the argString away, because if we hit an error we always
     * set procPtr->argString back to this value.  We could nil out the 
     * string pointer in the PCB, which would simplify management of the 
     * strings.  However, this would reduce the amount of information 
     * available via ps or (in case of trouble) the debugger.
     */
    argStringSave = procPtr->argString;

    /*
     * Open the file that is to be exec'd.  Check for setuid or setgid.
     */
    status = Fs_Open(execPath, FS_EXECUTE | FS_FOLLOW, FS_FILE, 0,
		     &filePtr);
    if (status != SUCCESS) {
	goto execError;
    }
    Fs_CheckSetID(filePtr, &uid, &gid);

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&pointATime, (int *)NULL, (Boolean *)NULL);
    }

    /* 
     * Get the right code segment to exec, possibly an interpreter (if the 
     * named file was a script).
     */
    status = SetupCode(execPath, userArgsPtr, &filePtr, &codeSegPtr,
		       &execInfo, &encapPtr, &argString);
    if (status != SUCCESS) {
	goto execError;
    }

    /* 
     * The initial user process doesn't have an FS state yet, so don't 
     * bother trying to close "close-on-exec" files.
     */
    if (!kernExec) {
	Fs_CloseOnExec((Proc_ControlBlock *)procPtr);
    }

    /*
     * Change the argument string and pass ownership off to the PCB.
     */
    procPtr->argString = argString;
    argString = NULL;
    /*
     * Do set uid here.  This way, the uid will be set before a remote
     * exec.
     */
    if (uid != -1) {
	procPtr->effectiveUserID = uid;
    }

    /* 
     * This is the point of no return.  Start doing operations that can't 
     * be backed out.  Note: we also use noReturn as a flag that the 
     * process is locked (XXX).
     */
    
    noReturn = TRUE;
    Proc_Lock(procPtr);

    /* 
     * In the normal case the user process is sitting in mach_msg, waiting
     * for a reply.  Freeze it and abort the RPC.  When the exec has
     * completed and we've reset the process's state, we can unfreeze the 
     * process.  
     * 
     * Note that this needs to be done before calling SetupVM.  When we did 
     * it after SetupVM, we were getting on sun3's an EXC_EMULATION message
     * before we could resume the user process.
     */
    if (!kernExec) {
	kernStatus = thread_suspend(procPtr->thread);
	if (kernStatus != KERN_SUCCESS) {
	    printf("DoExec: couldn't suspend thread: %s\n",
		   mach_error_string(kernStatus));
	    goto execError;
	}
	kernStatus = thread_abort(procPtr->thread);
	if (kernStatus != KERN_SUCCESS) {
	    printf("DoExec: couldn't abort thread: %s\n",
		   mach_error_string(kernStatus));
	    goto execError;
	}
    }

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&pointBTime, (int *)NULL, (Boolean *)NULL);
    }

    /* 
     * Set up virtual memory and machine registers.  If there are any 
     * problems in this section the user process is trashed badly enough 
     * that there's nothing left to return to.
     */
    status = SetupVM(Proc_AssertLocked(procPtr), codeSegPtr, &execInfo,
		     encapPtr);
    if (status != SUCCESS) {
	goto execError;
    }
    status = ProcMachSetRegisters(procPtr, encapPtr->hdr.base,
				  execInfo.entry);
    if (status != SUCCESS) {
	goto execError;
    }
    if (!kernExec) {
	procPtr->genFlags |= PROC_NEEDS_WAKEUP;
    }

    /* 
     * This is now past the point of no return, but we still need the pcb 
     * locked.  Fortunately, there are no error exits in the next set of 
     * calls, so it's okay to leave noReturn TRUE until we unlock the pcb.
     */

    /*
     * Set-gid only needs to be done on the host running the process.
     */
    if (gid != -1) {
	ProcAddToGroupList(Proc_AssertLocked(procPtr), gid);
    }

    /*
     * Take signal actions for execed process.
     */
    Sig_Exec(Proc_AssertLocked(procPtr));
    if (debugMe) {
	/*
	 * Debugged processes get a SIG_DEBUG at start up.
	 */
	(void)Sig_SendProc(Proc_AssertLocked(procPtr), SIG_DEBUG, FALSE,
			   SIG_NO_CODE, (Address)0);
    }

    noReturn = FALSE;
    Proc_Unlock(Proc_AssertLocked(procPtr));

 execError:
    /* 
     * If there's an error starting up a user process from a kernel thread,
     * let the caller take care of cleaning up.  We might be able to do the
     * kill here, but only if we turn off the paranoia check in
     * Proc_SetState.
     * 
     * Note that we don't just exit right here, because we need to free up 
     * the request/reply buffers from the MIG request (not to mention all 
     * the other resources obtained in this function).
     */
    if (status != SUCCESS && noReturn && !kernExec) {
	printf("DoExec killing process %x: failed exec: %s\n",
	       procPtr->processID, Stat_GetMsg(status));
	(void)Sig_SendProc(Proc_AssertLocked(procPtr), SIG_KILL, TRUE,
			   PROC_EXEC_FAILED, (Address)0);
    }
    if (noReturn) {
	Proc_Unlock(Proc_AssertLocked(procPtr));
	noReturn = FALSE;
    }
    if (filePtr != NULL) {
	(void)Fs_Close(filePtr);
    }
    if (codeSegPtr != NULL) {
	Vm_SegmentRelease(codeSegPtr);
    }
    if (encapPtr != NULL) {
	ckfree(encapPtr);
    }
    if (status == SUCCESS) {
	if (argStringSave != NULL) {
	    ckfree(argStringSave);
	    argStringSave = NULL;
	}
    } else {
	if (argString != NULL) {
	    ckfree(argString);
	    argString = NULL;
	}
	/* 
	 * Back out the arg string, being careful that the pcb's copy is 
	 * never garbage.
	 */
	if (procPtr->argString != argStringSave) {
	    char *newString = procPtr->argString;
	    procPtr->argString = argStringSave;
	    ckfree(newString);
	}
	argStringSave = NULL;
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

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, NULL, NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(doExecTime, endTime, &doExecTime);
	Time_Subtract(pointATime, startTime, &pointATime);
	Time_Add(partATime, pointATime, &partATime);
	Time_Subtract(pointBTime, startTime, &pointBTime);
	Time_Add(partBTime, pointBTime, &partBTime);
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * SetupCode --
 *
 *	Get a code segment to exec.  Do whatever changes are necessary if 
 *	the program is an interpreted script.
 *
 * Results:
 *	Returns a status code.  If the named program is a script, the file
 *	pointer is changed to refer to the interpreter.  If the return
 *	status is SUCCESS, fills in the code segment, the object file
 *	information for the actual program to execute, the strings to put
 *	on the stack for the program (arguments and environment), and
 *	creates a string suitable for storing in the PCB (e.g., for the ps
 *	command).  The caller is responsible for freeing the string array
 *	(one big buffer) and the "ps" string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
SetupCode(execName, userArgsPtr, filePtrPtr, codeSegPtrPtr, execInfoPtr,
	  encapPtrPtr, argStringPtr)
    char *execName;		/* name of the file given to Exec */
    UserArgs *userArgsPtr;	/* user arguments & env., possibly NIL */
    Fs_Stream **filePtrPtr;	/* IN: stream for execName; OUT: either the 
				 * same stream, or a stream for an 
				 * interpreter */
    Vm_Segment **codeSegPtrPtr;	/* OUT: code segment for final stream */
    Proc_ObjInfo *execInfoPtr;	/* OUT: a.out info for final stream */
    ExecEncapState **encapPtrPtr; /* OUT: stuff to copy to process's stack,
				   * including the program's argument  */
    char **argStringPtr;	/* OUT: argument string for ps */
{
    ReturnStatus status;
    Fs_Stream *filePtr;		/* local copy of file handle; this is the
				 * file that actually gets loaded */
    int sizeRead;		/* bytes to read from file */
    char *buffer = NULL;	/* buffer to hold first bit of file */
    Boolean interpreted = FALSE; /* is the program actually a script */
    char interpName[FS_MAX_PATH_NAME_LENGTH]; /* name of interpreter, if any */
    char *interpFlags;		/* if a script, arguments (usually flags) to
				 * pass to the interpreter */ 
    char *interpArgs[MAX_INTERP_ARGS]; /* actual arguments to pass to 
					* interpreter  */
    Time startTime, endTime;	/* instrumentation */

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    filePtr = *filePtrPtr;
    status = Vm_GetCodeSegment(filePtr, execName, (Proc_ObjInfo *)NULL,
			       TRUE, codeSegPtrPtr);
    if (status == SUCCESS) {
	bcopy((Address)((*codeSegPtrPtr)->typeInfo.codeInfo.execInfoPtr),
	      (Address)execInfoPtr, sizeof(Proc_ObjInfo));
    } else if (status == VM_SEG_NOT_FOUND) {
	/*
	 * We don't already have a segment for the given file.  Read the 
	 * file header, to see if it's an interpreter script.
	 */
	sizeRead = PROC_MAX_INTERPRET_SIZE;
	if (sizeRead < sizeof(ProcExecHeader)) {
	    sizeRead = sizeof(ProcExecHeader);
	}
	buffer = ckalloc((unsigned)sizeRead);
	if (buffer == NULL) {
	    panic("SetupCode: out of memory.\n");
	}
	status = Fs_Read(filePtr, buffer, 0, &sizeRead);
	if (status != SUCCESS) {
	    goto execError;
	}
	if (sizeRead >= 2 && buffer[0] == '#' && buffer[1] == '!') {
	    /* 
	     * It's definitely not a regular object file, so get rid of it 
	     * right now.
	     */
	    (void)Fs_Close(filePtr);
	    filePtr = NULL;
	    /*
	     * See if this is an interpreter script.
	     */
	    status = SetupInterpret(buffer, sizeRead, &filePtr, interpName,
				    &interpFlags, execInfoPtr); 
	    if (status != SUCCESS) {
		goto execError;
	    }
	    interpreted = TRUE;
	    status = Vm_GetCodeSegment(filePtr, interpName, execInfoPtr,
				       FALSE, codeSegPtrPtr);
	} else if (sizeRead < sizeof(ProcExecHeader)) {
	    status = PROC_BAD_AOUT_FORMAT;
	} else {
	    status = ProcGetObjInfo(filePtr, (ProcExecHeader *)buffer,
				    execInfoPtr);
	    if (status != SUCCESS) {
		/*
		 * XXX Native Sprite tries to give a useful diagnostic in
		 * case the object file has a recognizable but wrong object
		 * format.  We should do the same.
		 */
		goto execError;
	    }
	    status = Vm_GetCodeSegment(filePtr, execName, execInfoPtr,
				       FALSE, codeSegPtrPtr);
	}
    }
    if (status != SUCCESS) {
	goto execError;
    }

    /*
     * Set up whatever special arguments we might have due to an
     * interpreter file.
     */
    if (interpreted) {
	int index = 0;		/* index into array of interpreter args */
	
	if (userArgsPtr->argPtrArray == (char **) NIL) {
	    interpArgs[index] = execName;
	    ++index;
	}
	if (interpFlags != NULL) {
	    interpArgs[index] = interpFlags;
	    ++index;
	}
	interpArgs[index] = execName;
	++index;
	interpArgs[index] = (char *) NIL;
	if (index >= MAX_INTERP_ARGS) {
	    panic("SetupCode: need bigger array for interpreter arguments.\n");
	}
    }
    /*
     * Copy in the argument list and environment into a single contiguous
     * buffer.
     */
    status = SetupArgs(userArgsPtr, (interpreted ? interpArgs : (char **)NIL),
		       encapPtrPtr, argStringPtr);
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

 execError:
    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, setupCodeTime, &setupCodeTime);
    }

    *filePtrPtr = filePtr;
    if (buffer != NULL) {
	ckfree(buffer);
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * SetupVM --
 *
 *	Set up the virtual memory for the process.  The process's VM is 
 *	cleared, and new mappings are made for code, heap, stack, etc.
 *
 * Results:
 *	Returns a status code.
 *
 * Side effects:
 *	Make additional private references to the code segment, as necessary.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
SetupVM(procPtr, codeSegPtr, execInfoPtr, encapPtr)
    Proc_LockedPCB *procPtr;
    Vm_Segment *codeSegPtr;	/* the code segment to use */
    Proc_ObjInfo *execInfoPtr;	/* layout of the program */
    ExecEncapState *encapPtr;	/* encapsulated exec info with initial stack 
				 * contents */
{
    ReturnStatus status;
    Time startTime, endTime;	/* instrumentation */

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    /* 
     * Do some instrumentation on the heap and stack, then clear out the 
     * user process's address space.
     */
    InstrumentSwapSegments(procPtr, execInfoPtr);
    Vm_ReleaseMappedSegments(procPtr);
    (void)vm_deallocate(procPtr->pcb.taskInfoPtr->task, VM_MIN_ADDRESS,
			(vm_size_t)(VM_MAX_ADDRESS - VM_MIN_ADDRESS));

    status = DoCode(procPtr, execInfoPtr, codeSegPtr);
    if (status != SUCCESS) {
	printf("SetupVM: Can't set up text: %s\n", Stat_GetMsg(status));
	goto done;
    }
    status = DoHeap(procPtr, execInfoPtr, codeSegPtr->swapFilePtr,
		    codeSegPtr->swapFileName);
    if (status != SUCCESS) {
	printf("SetupVM: Can't set up heap: %s\n", Stat_GetMsg(status));
	status = VM_SWAP_ERROR;
	goto done;
    }
    status = DoStack(procPtr, execInfoPtr, encapPtr);
    if (status != SUCCESS) {
	printf("SetupVM: Can't set up stack: %s\n", Stat_GetMsg(status));
	status = VM_SWAP_ERROR;
	goto done;
    }

    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, setupVmTime, &setupVmTime);
    }

 done:
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * DoCode --
 *
 *	Set up the text part of the child process, by mapping the code 
 *	segment into the process's address space.
 *
 * Results:
 *	Sprite status code.  
 *
 * Side effects:
 *	Creates a private reference to the code segment if successful.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DoCode(procPtr, execInfoPtr, codeSegPtr)
    Proc_LockedPCB *procPtr;	/* the locked process to set up */
    Proc_ObjInfo *execInfoPtr;	/* where to find things in the file */
    Vm_Segment *codeSegPtr;	/* the code segment to use */
{
    kern_return_t kernStatus;
    Address textStart;		/* start of text region in user VM */
    ReturnStatus status = SUCCESS;
    Time startTime, endTime;	/* instrumentation */

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    textStart = execInfoPtr->codeLoadAddr;
    Vm_SegmentAddRef(codeSegPtr);

    /* 
     * XXX Should allow for object files that don't have page-aligned 
     * sections. 
     */
    if (textStart != Vm_TruncPage(textStart)) {
	status = PROC_BAD_AOUT_FORMAT;
	goto done;
    }
    kernStatus = Vm_MapSegment(procPtr, codeSegPtr, TRUE, FALSE,
			       (vm_offset_t)execInfoPtr->codeFileOffset,
			       (vm_size_t)execInfoPtr->codeSize,
			       &textStart, &status);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (textStart != execInfoPtr->codeLoadAddr) {
	/* 
	 * XXX Really should set an error status and deallocate the 
	 * region. 
	 */
	panic("DoCode: Didn't get requested text address.\n");
    }

 done:
    if (status != SUCCESS) {
	Vm_SegmentRelease(codeSegPtr);
    }
    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, doCodeTime, &doCodeTime);
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * DoHeap --
 *
 *	Set up the heap part of the child process.
 *
 * Results:
 *	status code.
 *
 * Side effects:
 * 	Creates a swap segment for the heap.  Maps the segment into 
 * 	the process's address space.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DoHeap(procPtr, execInfoPtr, objFilePtr, objFileName)
    Proc_LockedPCB *procPtr;	/* the locked process to set up */
    Proc_ObjInfo *execInfoPtr;	/* where to find things in the file */
    Fs_Stream *objFilePtr;	/* the object file */
    char *objFileName;		/* and its name */
{
    kern_return_t kernStatus;
    Address heapStart;		/* start of heap region in user VM */
    vm_size_t heapSize;		/* bytes in heap segment */
    vm_size_t heapMappedSize;	/* initial number of bytes actually mapped */
    ReturnStatus status = SUCCESS;
    Vm_Segment *segPtr = NULL;	/* the heap segment */
    Time startTime, endTime;	/* instrumentation */

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    /* 
     * Verify that the program is laid out like we expect, with 
     * initialized data coming before uninitialized data.
     */
    if (execInfoPtr->heapLoadAddr > execInfoPtr->bssLoadAddr) {
	printf("Can't deal with heap (0x%x) above bss (0x%x)\n",
	       execInfoPtr->heapLoadAddr, execInfoPtr->bssLoadAddr);
	return PROC_BAD_AOUT_FORMAT;
    }

    /* 
     * Make the swap file big enough to consume all of the remaining 
     * memory.  Of course, most of the segment will never get mapped.
     */
    heapStart = HeapStart(execInfoPtr);
    if (heapStart != Vm_TruncPage(heapStart)) {
	printf("DoHeap: heap segment isn't page aligned.\n");
	return PROC_BAD_AOUT_FORMAT;
    }
    /* 
     * The paging code assumes that each page resides entirely in the 
     * object file or in the swap file.  If we want to remove this 
     * restriction, we have to fix the paging code to zero-fill the 
     * portion of the last page that's not in the object file.
     */
    if (execInfoPtr->heapSize != trunc_page(execInfoPtr->heapSize)) {
	printf("DoHeap: heap segment contains fraction of page.\n");
	return PROC_BAD_AOUT_FORMAT;
    }

    /* 
     * From here on, return through "bailOut", so that the segment 
     * gets cleaned up correctly.
     */

    heapSize = procMach_MaxUserStackAddr - heapStart;
    status = Vm_GetSwapSegment(VM_HEAP, heapSize, &segPtr);
    if (status != SUCCESS) {
	goto bailOut;
    }
    Vm_AddHeapInfo(segPtr, objFilePtr, objFileName, execInfoPtr);

    heapMappedSize = (int)Vm_RoundPage(execInfoPtr->bssLoadAddr -
				       execInfoPtr->heapLoadAddr +
				       execInfoPtr->bssSize);
    kernStatus = Vm_MapSegment(procPtr, segPtr, FALSE, FALSE, (vm_offset_t)0,
			       heapMappedSize, &heapStart, &status);
    if (kernStatus != KERN_SUCCESS) {
	printf("DoHeap: couldn't map heap segment: %s\n",
	       mach_error_string(kernStatus));
	status = Utils_MapMachStatus(kernStatus);
    }
    if (heapStart != execInfoPtr->heapLoadAddr) {
	/* 
	 * XXX Really should set an error status and deallocate the 
	 * region. 
	 */
	panic("DoHeap: Didn't get requested start address.\n");
    }

 bailOut:
    if (status != SUCCESS && segPtr != NULL) {
	Vm_SegmentRelease(segPtr);
    }
    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, doHeapTime, &doHeapTime);
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * DoStack --
 *
 *	Set up the stack part of the child process.  Creates a segment for
 *	the stack and maps it into the process's address space.
 *
 * Results:
 *	Returns a status code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static ReturnStatus
DoStack(procPtr, execInfoPtr, encapPtr)
    Proc_LockedPCB *procPtr;	/* the locked process to set up */
    Proc_ObjInfo *execInfoPtr;	/* info about how to lay out the binary */
    ExecEncapState *encapPtr;	/* arguments to put on stack & other info */
{
    kern_return_t kernStatus;
    ReturnStatus status = SUCCESS;
    vm_size_t stackSize;	/* number of bytes allowed for stack */
    Address firstPageToMap;	/* address of first stack page to map */
    Address firstPageMapped;	/* address of page actually mapped */
    vm_size_t bytesToMap;	/* number of stack bytes to map initially */
    Address stackStart = 0;	/* where stack would start in memory, if 
				 * we let it get that big */
    Vm_Segment *segPtr = NULL;	/* the stack segment */
    Time startTime, endTime;	/* instrumentation */
    Time startCopyTime, endCopyTime; /* more instrumentation */

    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startTime = time_ZeroSeconds;
    }

    if (!PROCMACH_STACK_GROWS_DOWN) {
	/* 
	 * Need to fix the low-end address for the stack, rather than 
	 * letting it grown down.
	 */
	panic("DoStack: can't handle stacks that grow up.\n");
    }

    /* 
     * Verify that the program is laid out like we expect it, 
     * with uninitialized data coming after code and initialized data.
     */
    if (execInfoPtr->codeLoadAddr > execInfoPtr->bssLoadAddr ||
		execInfoPtr->heapLoadAddr > execInfoPtr->bssLoadAddr) {
	printf("DoStack: unexpected program layout.\n");
	status = PROC_BAD_AOUT_FORMAT;
	goto done;
    }

    stackStart = StackStart(execInfoPtr);
    stackSize = procMach_MaxUserStackAddr - stackStart;
    status = Vm_GetSwapSegment(VM_STACK, stackSize, &segPtr);
    if (status != SUCCESS) {
	goto done;
    }
    segPtr->typeInfo.stackInfo.baseAddr = stackStart;

    /* 
     * Map as many pages as are needed for the initial stack Setup 
     * (arguments, environment).
     */
    bytesToMap = round_page(encapPtr->hdr.size - sizeof(ExecEncapHeader));
    firstPageToMap = procMach_MaxUserStackAddr - bytesToMap;
    firstPageMapped = firstPageToMap;
    kernStatus = Vm_MapSegment(procPtr, segPtr, FALSE, FALSE,
			       (vm_offset_t)(firstPageToMap - stackStart),
			       bytesToMap, &firstPageMapped, &status);
    if (kernStatus != KERN_SUCCESS) {
	status = Utils_MapMachStatus(kernStatus);
    }
    if (status == SUCCESS && firstPageMapped != firstPageToMap) {
	panic("DoStack: didn't get requested stack address.\n");
    }
    if (status != SUCCESS) {
	goto done;
    }

    /* 
     * Copy the arguments and environment to the stack.
     */
    if (sys_CallProfiling) {
	Timer_GetTimeOfDay(&startCopyTime, (int *)NULL, (Boolean *)NULL);
    } else {
	startCopyTime = time_ZeroSeconds;
    }
    status = Vm_CopyOutProc(encapPtr->hdr.size - sizeof(ExecEncapHeader),
			    (Address)&encapPtr->argc, TRUE, procPtr,
			    (Address)encapPtr->hdr.base);
    if (status != SUCCESS) {
	panic("DoStack: couldn't copy arguments to user process: %s\n",
	      Stat_GetMsg(status));
    }
    if (sys_CallProfiling && !Time_EQ(startCopyTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endCopyTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endCopyTime, startCopyTime, &endCopyTime);
	Time_Add(endCopyTime, stackCopyOutTime, &stackCopyOutTime);
    }

 done:
    if (status != SUCCESS && segPtr != NULL) {
	Vm_SegmentRelease(segPtr);
	segPtr = NULL;
    }
    if (sys_CallProfiling && !Time_EQ(startTime, time_ZeroSeconds)) {
	Timer_GetTimeOfDay(&endTime, (int *)NULL, (Boolean *)NULL);
	Time_Subtract(endTime, startTime, &endTime);
	Time_Add(endTime, doStackTime, &doStackTime);
    }
    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * SetupInterpret --
 *
 *	Read in the interpreter name, arguments and object file header.
 *
 * Results:
 *	Error if for some reason could not parse the interpreter name and
 *	arguments, or could not open the interpreter object file.  If
 *	successful, fills in the stream for the interpreter object file 
 *	(which the caller is responsible for closing); fills in the name of 
 *	the interpreter; fills in the argument string to pass to the
 *	interpreter; fills in object file information for the interpreter.
 *	Non-NULL string pointers point into the given buffer.
 *
 * Side effects:
 *	Puts some nulls into the given buffer to separate out strings. 
 *
 *----------------------------------------------------------------------
 */ 

static ReturnStatus
SetupInterpret(buffer, sizeRead, interpPtrPtr, interpName, argStringPtr, 
	       objInfoPtr)
    register char *buffer;	/* Bytes read in from start of file */
    int sizeRead;		/* Number of bytes in buffer. */
    register Fs_Stream **interpPtrPtr; /* OUT: interpreter file */
    char *interpName;		/* MAXPATHLEN buffer to hold the name of 
				 * the interpreter file */
    char **argStringPtr;	/* OUT: single argument string to pass to 
				 * interpreter, possibly NULL */
    Proc_ObjInfo *objInfoPtr;	/* OUT: a.out info for the interpreter */
{
    register	char	*strPtr;
    int			i;
    ReturnStatus	status;
    ProcExecHeader	execHeader;
    char *		givenInterpName; /* interpreter name from script */

    /*
     * Make sure the interpreter name and arguments are terminated by a 
     * carriage return, and make sure there's no garbage in the file.
     */

    for (i = 2, strPtr = &(buffer[2]);
	     i < sizeRead && *strPtr != '\n';
	     i++, strPtr++) {
	if (!isascii(*strPtr)) {
	    return PROC_BAD_FILE_NAME;
	}
    }
    if (i == sizeRead) {
	return(PROC_BAD_FILE_NAME);
    }
    *strPtr = '\0';

    /*
     * Get the name of the interpreter from the script.  
     */
    
    for (strPtr = &(buffer[2]); isspace(*strPtr); strPtr++) {
    }
    if (*strPtr == '\0') {
	return(PROC_BAD_FILE_NAME);
    }
    givenInterpName = strPtr;
    while (!isspace(*strPtr) && *strPtr != '\0') {
	strPtr++;
    }

    /*
     * Terminate the interpreter name, and get a pointer to the arguments 
     * if there are any. 
     */
    
    *argStringPtr = NULL;

    if (*strPtr != '\0') {
	*strPtr = '\0';
	strPtr++;
	while (isspace(*strPtr)) {
	    strPtr++;
	}
	if (*strPtr != '\0') {
	    *argStringPtr = strPtr;
	}
    }

    strcpy(interpName, givenInterpName);

    /*
     * Open the interpreter to exec and read the a.out header.
     */

    status = Fs_Open(interpName, FS_EXECUTE | FS_FOLLOW, FS_FILE, 0,
		     interpPtrPtr);
    if (status != SUCCESS) {
	return(status);
    }

    sizeRead = sizeof(ProcExecHeader);
    status = Fs_Read(*interpPtrPtr, (Address)&execHeader, 0, &sizeRead);
    if (status == SUCCESS && sizeRead != sizeof(ProcExecHeader)) {
	status = PROC_BAD_AOUT_FORMAT;
    } else {
	status = ProcGetObjInfo(*interpPtrPtr, &execHeader, objInfoPtr);
    }
    if (status != SUCCESS) {
	/*
	 * XXX Native Sprite tries to give a useful diagnostic in
	 * case the object file has a recognizable but wrong object
	 * format.  We should do the same.
	 */
	(void)Fs_Close(*interpPtrPtr);
	*interpPtrPtr = NULL;
    }

    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * InstrumentSwapSegments --
 *
 *	Check whether the process's current heap and stack could simply be 
 *	reused in an exec().  Also, record the current heap and stack 
 *	sizes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Increments the vmStat counter for each segment that could be 
 *	reused.  Updates the heap and stack sizes in the PCB.
 *
 *----------------------------------------------------------------------
 */

static void
InstrumentSwapSegments(procPtr, execInfoPtr)
    Proc_LockedPCB *procPtr;	/* the process to look at */
    Proc_ObjInfo *execInfoPtr;	/* info about the new object file */
{
    Address newStackStart;
    Address newHeapStart;
    ProcTaskInfo *taskInfoPtr;
    vm_size_t currHeapSize;	/* current heap size  */
    vm_size_t currStackSize;	/* current stack size */

    taskInfoPtr = procPtr->pcb.taskInfoPtr;

    if (taskInfoPtr != NULL) {

	/*
	 * Record the number of pages currently mapped by the heap and
	 * stack.
	 */
	currHeapSize = (taskInfoPtr->vmInfo.heapInfoPtr == NULL
			? 0
			: taskInfoPtr->vmInfo.heapInfoPtr->length);
	currStackSize = (taskInfoPtr->vmInfo.stackInfoPtr == NULL
			 ? 0
			 : taskInfoPtr->vmInfo.stackInfoPtr->length);
	taskInfoPtr->vmInfo.execHeapPages = Vm_ByteToPage(currHeapSize);
	taskInfoPtr->vmInfo.execStackPages = Vm_ByteToPage(currStackSize);

	/* 
	 * If the heap starts at the same location, then it could
	 * theoretically be reused.  For the stack to be reused, the BSS
	 * must end at the same place.  In either case, it might first be
	 * necessary to bzero the currently mapped pages, then deallocate
	 * pages that aren't initially allocated by the new object file.
	 * On the other hand, it might not be worth reusing any segments
	 * that have pages out on the swap server.
	 */
	newHeapStart = HeapStart(execInfoPtr);
	if (taskInfoPtr->vmInfo.heapInfoPtr != NULL
	    && (newHeapStart ==
		procPtr->pcb.taskInfoPtr->vmInfo.heapInfoPtr->mappedAddr)) {
	    vmStat.segmentsNeedlesslyDestroyed++;
	}
	newStackStart = StackStart(execInfoPtr);
	if (taskInfoPtr->vmInfo.stackInfoPtr != NULL
	    && (newStackStart ==
		(procPtr->pcb.taskInfoPtr->vmInfo.stackInfoPtr->
		 segPtr->typeInfo.stackInfo.baseAddr))) {
	    vmStat.segmentsNeedlesslyDestroyed++;
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * StackStart --
 *
 *	Calculate the stack start address for the given object file.
 *
 * Results:
 *	Returns the stack start address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Address
StackStart(execInfoPtr)
    Proc_ObjInfo *execInfoPtr;	/* info about the new object file */
{
    return Vm_RoundPage(execInfoPtr->bssLoadAddr + execInfoPtr->bssSize);
}


/*
 *----------------------------------------------------------------------
 *
 * HeapStart --
 *
 *	Return the starting heap address for the given object file.
 *
 * Results:
 *	Returns the heap start address.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Address
HeapStart(execInfoPtr)
    Proc_ObjInfo *execInfoPtr;	/* info about the new object file */
{
    return execInfoPtr->heapLoadAddr;
}
