/*
 * sysSyscall.c --
 *
 *	Routines and structs for system calls.  Contains information
 *	about each system call such as the number of arguments and how
 *	to invoke the call for migrated processes.  All local
 *	processes invoke system calls by copying in the arguments from
 *	the user's address space and passing them to the kernel
 *	routine uninterpreted.  When migrated processes invoke system
 *	calls, when possible the arguments are passed to a generic
 *	stub that packages the arguments and sends them to the home
 *	node of the process through RPC.  This file contains
 *	information about the sizes and types of each argument for
 *	those procedures.  The generic stub is called with information
 *	about which system call was invoked and what its arguments consist
 *	of.  The information stored for each argument is described below.
 *
 *	Many system calls, however, are handled exclusively on the
 *	current machine or are processed by special-purpose routines
 *	on the current machine before being sent to the home machine.
 *	In these cases no information about parameter types is kept,
 *	and the procedure is invoked in the same manner as for local
 *	processes.
 *
 *	NOTES ON ADDING SYSTEM CALLS:
 *	   Add an entry for the system call to the two arrays
 *	   declared below, sysCalls and paramsArray.  For sysCalls,
 *	   list the procedures to be invoked, whether to use the
 *	   generic stub (in which case special == FALSE), and the number
 *	   of words passed to the system call.  For paramsArray, if
 *	   special is FALSE, list the type and disposition of each
 *	   parameter.  If special is TRUE, just add a comment as a
 *	   placeholder within the array.  Finally, add an entry in
 *	   procRpc.c for the callback routine corresponding to the new
 *	   procedure (NIL if the call is not migrated).
 *
 * Copyright 1985, 1988 Regents of the University of California
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
#endif not lint

#include "sprite.h"
#include "fs.h"
#include "sys.h"
#include "dbg.h"
#include "proc.h"
#include "sync.h"
#include "sched.h"
#include "byte.h"
#include "vm.h"
#include "user/vm.h"
#include "rpc.h"
#include "prof.h"
#include "devVid.h"
#include "net.h"
#include "sysSysCall.h"
#include "sysSysCallParam.h"
#include "sysTestCall.h"
#include "status.h"

/*
 * Forward declarations to procedures defined in this file:
 */

extern	ReturnStatus	SysMigCall();
extern	ReturnStatus	Sys_StatsStub();

#ifndef CLEAN
Boolean sysTraceSysCalls = FALSE;
#endif CLEAN

/*
 * For each system call, keep track of:
 *	- which procedure to invoke if the process is local;
 *	- which to invoke if it is remote;
 *	- whether the remoteFunc is "special" and is to be invoked
 *	  without interpreting the arguments, or whether the generic
 *	  stub will be called (passing it information such as the
 *	  number of arguments and the type of system call);
 *	- a pointer to an array of parameter information, which includes
 *	  the types and dispositions of each argument.  For "special"
 *	  routines, this pointer is NIL.  Refer to sysSysCallParam.h for
 *	  documentation on the Sys_CallParam type.
 */

typedef struct {
    ReturnStatus (*localFunc)();  /* procedure to invoke for local processes */
    ReturnStatus (*remoteFunc)(); /* procedure to invoke for migrated procs */
    Boolean special;		  /* whether the remoteFunc is called without
				     passing it additional information */
    int numWords;		  /* The number of 4-byte quantities that must
				     be passed to the system call. */
    Sys_CallParam *paramsPtr;	  /* pointer to parameter information for
				     generic stub to use */
} SysCallEntry;

/*
 * This declaration will go away when all system calls have been set up
 * for migration.
 */
extern ReturnStatus Proc_RemoteDummy();

/*
 * Sys_ParamSizes is an array of sizes corresponding to
 * each system call argument type.  Sys_ParamSizesDecl is used to
 * assign the elements of the array at compile time.  Due to compiler
 * restrictions, sys_ParamSizes needs to be a "pointer" while
 * sys_ParamSizesDecl is an "array".  The argument types are documented
 * in sysSysCallParam.h.
 */

int sys_ParamSizesDecl[] = {
    sizeof(int),			/* SYS_PARAM_INT		*/
    sizeof(char),			/* SYS_PARAM_CHAR		*/
    NIL,				/* SYS_PARAM_STRING		*/
    sizeof(Proc_PID),			/* SYS_PARAM_PROC_PID		*/
    sizeof(Proc_ResUsage),		/* SYS_PARAM_PROC_RES		*/
    sizeof(Sync_Lock),			/* SYS_PARAM_SYNC_LOCK		*/
    sizeof(Fs_Attributes),		/* SYS_PARAM_FS_ATT		*/
    FS_MAX_PATH_NAME_LENGTH,		/* SYS_PARAM_FS_NAME		*/
    sizeof(Time),			/* SYS_PARAM_TIMEPTR		*/
    sizeof(Time) / 2,			/* SYS_PARAM_TIME1		*/
    sizeof(Time) / 2,			/* SYS_PARAM_TIME2		*/
    sizeof(int),			/* SYS_PARAM_VM_CMD		*/
    PROC_MAX_ENVIRON_NAME_LENGTH,	/* SYS_PARAM_PROC_ENV_NAME  	*/
    PROC_MAX_ENVIRON_VALUE_LENGTH, 	/* SYS_PARAM_PROC_ENV_VALUE 	*/
    0, 					/* SYS_PARAM_DUMMY	 	*/
    sizeof(int),			/* SYS_PARAM_RANGE1	 	*/
    sizeof(int),			/* SYS_PARAM_RANGE2		*/
    sizeof(Proc_ControlBlock),		/* SYS_PARAM_PCB		*/
    sizeof(Fs_Device),			/* SYS_PARAM_FS_DEVICE		*/
    sizeof(Proc_PCBArgString),		/* SYS_PARAM_PCBARG		*/

};

int *sys_ParamSizes = sys_ParamSizesDecl;

static int ErrorProc()
{
    Sys_Panic(SYS_WARNING, "Obsolete system call.\n");
    return(GEN_FAILURE);
}

/*
 * sysCalls --
 *
 *	This table is used during a system call trap to branch to the
 *	correct procedure for each system call.  There are two functions,
 *	one if the process is local, the other if the process is an immigrant.
 *	The number of integers (parameters) on the user's stack that have
 *	to be copied to the kernel stack is also listed here.  The last field
 *	of each record is filled in dynamically but initialized here to NIL.
 *
 *	N.B.: The format of this table is relied on to generate the file
 *	Dummy.c, a file with just the declarations of the system calls
 *	and no body.  See the CreateDummy script in src/lib/libc.
 */

#define NILPARM ((Sys_CallParam *) NIL)
#define CAST	(ReturnStatus (*) ())

static SysCallEntry sysCalls[] = {
/*
 *	localFunc		  remoteFunc	   special numWords  NILPARM
 */
/* DON'T DELETE THIS LINE - CreateDummy depends on it */
    Proc_Fork,		       Proc_Fork,	   TRUE,	2,   NILPARM,
    Proc_Exec,		       Proc_Exec,	   TRUE,	3,   NILPARM,
    CAST Proc_Exit,       CAST Proc_Exit,	   TRUE,	1,   NILPARM,
    Sync_WaitTime,	       Sync_WaitTime,	   TRUE,	2,   NILPARM,
    Test_PrintOut,	       Test_PrintOut,      TRUE,       10,   NILPARM,
    Test_GetLine,	       Test_GetLine,       TRUE,	2,   NILPARM,
    Test_GetChar,	       Test_GetChar,  	   TRUE,	1,   NILPARM,
    Fs_OpenStub,	       Fs_OpenStub,  	   TRUE,	4,   NILPARM,
    Fs_ReadStub,	       Fs_ReadStub,  	   TRUE, 	4,   NILPARM,
    Fs_WriteStub,	       Fs_WriteStub,  	   TRUE, 	4,   NILPARM,
    Fs_UserClose,	       Fs_UserClose,  	   TRUE, 	1,   NILPARM,
    Fs_RemoveStub,	       Fs_RemoveStub,  	   TRUE,	1,   NILPARM,
    Fs_RemoveDirStub,	       Fs_RemoveDirStub,   TRUE,	1,   NILPARM,
    Fs_MakeDirStub,	       Fs_MakeDirStub,     TRUE,	2,   NILPARM,
    Fs_ChangeDirStub,	       Fs_ChangeDirStub,   TRUE,	1,   NILPARM,
    Proc_Wait,		       Proc_Wait,   	   TRUE,	8,   NILPARM,
    Proc_Detach,	       Proc_DoRemoteCall,  FALSE,	1,   NILPARM,
    Proc_GetIDs,	       Proc_DoRemoteCall,  FALSE,	4,   NILPARM,
    Proc_SetIDs,	       Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    Proc_GetGroupIDs,	       Proc_GetGroupIDs,   TRUE,	3,   NILPARM,
    Proc_SetGroupIDs,	       Proc_SetGroupIDs,   TRUE,	2,   NILPARM,
    Proc_GetFamilyID,	       Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    Proc_SetFamilyID,	       Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    Test_RpcStub,	       Test_RpcStub,   	   TRUE,	4,   NILPARM,
    Sys_StatsStub,	       Sys_StatsStub,      TRUE,	4,   NILPARM,
    Vm_CreateVA,	       Vm_CreateVA,	   TRUE, 	2,   NILPARM,
    Vm_DestroyVA,	       Vm_DestroyVA,	   TRUE, 	2,   NILPARM,
    Sig_UserSend,	       Proc_DoRemoteCall,  FALSE,	3,   NILPARM,
    Sig_Pause,		       Sig_Pause,          TRUE,	1,   NILPARM,
    Sig_SetHoldMask,	       Sig_SetHoldMask,    TRUE,	2,   NILPARM,
    Sig_SetAction,	       Sig_SetAction,      TRUE,	3,   NILPARM,
    Prof_Start,		       Proc_DoRemoteCall,  TRUE,	0,   NILPARM,
    Prof_End,		       Proc_DoRemoteCall,  TRUE,	0,   NILPARM,
    Prof_DumpStub,	       Proc_DoRemoteCall,  FALSE,	1,   NILPARM,
    Vm_Cmd,		       Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    Sys_GetTimeOfDay,	       Proc_DoRemoteCall,  FALSE,	3,   NILPARM,
    Sys_SetTimeOfDay,	       Proc_DoRemoteCall,  FALSE,	3,   NILPARM,
    Sys_DoNothing,	       Sys_DoNothing,      TRUE,	0,   NILPARM,
    Proc_GetPCBInfo,	       Proc_DoRemoteCall,  FALSE,	5,   NILPARM,
    Vm_GetSegInfo,	       Proc_RemoteDummy,   TRUE,	3,   NILPARM,
    Proc_GetResUsage,	       Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    Proc_GetPriority,	       Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    CAST Proc_SetPriority,     Proc_DoRemoteCall,  FALSE,	3,   NILPARM,
    Proc_Debug,		       Proc_RemoteDummy,   TRUE,	5,   NILPARM,
    Proc_Profile,	       Proc_RemoteDummy,   TRUE,	6,   NILPARM,
    ErrorProc,		       ErrorProc,   	   TRUE,	2,   NILPARM,
    ErrorProc,	   	       ErrorProc, 	   TRUE,	2,   NILPARM,
    Fs_GetNewIDStub,	       Fs_GetNewIDStub,    TRUE,	2,   NILPARM,
    Fs_GetAttributesStub,      Fs_GetAttributesStub, TRUE,	3,   NILPARM,
    Fs_GetAttributesIDStub,    Fs_GetAttributesIDStub, TRUE,	2,   NILPARM,
    Fs_SetAttributesStub,      Fs_SetAttributesStub, TRUE,	3,   NILPARM,
    Fs_SetAttributesIDStub,    Fs_SetAttributesIDStub, TRUE,	2,   NILPARM,
    Fs_SetDefPermStub,	       Fs_SetDefPermStub,  TRUE,	2,   NILPARM,
    Fs_IOControlStub,	       Fs_IOControlStub,   TRUE,	6,   NILPARM,
    Dev_VidEnable,	       Proc_DoRemoteCall,  FALSE,	1,   NILPARM,
    Proc_SetEnvironStub,       Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    Proc_UnsetEnvironStub,     Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    Proc_GetEnvironVarStub,    Proc_DoRemoteCall,  FALSE,	2,   NILPARM,
    Proc_GetEnvironRangeStub,  Proc_RemoteDummy,   TRUE,	4,   NILPARM,
    Proc_InstallEnvironStub,   Proc_RemoteDummy,   TRUE,	2,   NILPARM,
    Proc_CopyEnvironStub,      Proc_DoRemoteCall,  FALSE,	0,   NILPARM,
    Sync_SlowLockStub,	       Sync_SlowLockStub,  TRUE,	1,   NILPARM,
    Sync_SlowWaitStub,	       Sync_SlowWaitStub,  TRUE,	3,   NILPARM,
    Sync_SlowBroadcastStub,    Sync_SlowBroadcastStub,  TRUE,	2,   NILPARM,
    Vm_PageSize,		Vm_PageSize,  	   TRUE,	1,   NILPARM,
    Fs_HardLinkStub,		Fs_HardLinkStub,   TRUE,	2,   NILPARM,
    Fs_RenameStub,		Fs_RenameStub, 	   TRUE,	2,   NILPARM,
    Fs_SymLinkStub,		Fs_SymLinkStub,    TRUE,	3,   NILPARM,
    Fs_ReadLinkStub,		Fs_ReadLinkStub,   TRUE,	4,   NILPARM,
    Fs_CreatePipeStub,		Fs_CreatePipeStub, TRUE,	2,   NILPARM,
    VmMach_MapKernelIntoUser,	Proc_RemoteDummy, FALSE,	4,   NILPARM,
    Fs_AttachDiskStub,		Proc_DoRemoteCall, FALSE,	3,   NILPARM,
    Fs_SelectStub,		Fs_SelectStub, 	   TRUE,	6,   NILPARM,
    CAST Sys_Shutdown,		Proc_DoRemoteCall, FALSE,	2,   NILPARM,
    Proc_Migrate,		Proc_DoRemoteCall, FALSE,	2,   NILPARM,
    Fs_MakeDeviceStub,		Proc_DoRemoteCall, FALSE,	3,   NILPARM,
    Fs_CommandStub,		Proc_DoRemoteCall, FALSE,	3,   NILPARM,
    ErrorProc,       		ErrorProc, 	   TRUE,	2,   NILPARM,
    Sys_GetMachineInfo,       	Proc_DoRemoteCall, FALSE,	3,   NILPARM,
    Net_InstallRouteStub, 	Net_InstallRouteStub, TRUE, 	6,   NILPARM,
    Fs_ReadVectorStub, 		Fs_ReadVectorStub, TRUE, 	4,   NILPARM,
    Fs_WriteVectorStub, 	Fs_WriteVectorStub, TRUE, 	4,   NILPARM,
    Fs_CheckAccess,     	Fs_CheckAccess, 	TRUE,	3,   NILPARM,
    Proc_GetIntervalTimer,	Proc_GetIntervalTimer, 	TRUE,	2,   NILPARM,
    Proc_SetIntervalTimer,	Proc_SetIntervalTimer, 	TRUE,	3,   NILPARM,
    Fs_FileWriteBackStub,	Fs_FileWriteBackStub, TRUE,	4,   NILPARM,
    Proc_ExecEnv,		Proc_ExecEnv,	   TRUE,	4,   NILPARM,
    Fs_SetAttrStub,		Fs_SetAttrStub,	   TRUE,	4,   NILPARM,
    Fs_SetAttrIDStub,		Fs_SetAttrIDStub,   TRUE,	3,   NILPARM,
};


/*
 * paramsArray is a static array of parameter information.  The array is
 * one gigantic array so that it may be initialized at compile time, but
 * conceptually it is distinct arrays, one per system call.  SysInitSysCall,
 * called at system initialization time, maps points within this array
 * to paramsPtr fields within the sysCalls array.  ParamsPtr is initialized
 * to NIL at compile time, but for procedures that are not flagged as
 * "special", paramsPtr is reset to point into paramsArray at the point of
 * the first Sys_CallParam structure corresponding to that procedure.
 *
 * For system calls that are "special", there is no entry in paramsArray.
 * However, a comment with the system call number and " special" is useful
 * to keep track of the correspondence between parameter information and
 * the rest of the sysCall struct.  Note that "special" is equivalent to
 * "local": "special" usually means a special-purpose routine is called,
 * while "local" means the same routine is used for both local and migrated
 * processes.
 *
 * The format of paramsArray is as follows.  For each non-special
 * SysCallEntry, there should be -numWords- Sys_CallParam structures.
 * A number of defined constants are given to simplify the information
 * given for each one.  Essentially, the crucial things to consider are
 * whether a given parameter is passed IN to a procedure, OUT of it, or
 * both.  At the same time, is the parameter passed into the system call
 * in its entirety, such as an integer; or is the parameter a pointer to
 * something that needs to be copied into or out of the kernel address
 * space, or made accessible?  Finally, if the parameter is a pointer, is
 * it a pointer to an object of fixed size or does it point to an array
 * of objects?  The generic stub will handle arrays if the size of the
 * array is an IN parameter that is passed in just before the pointer to
 * the array, in the argument list.  It will also handle arrays with
 * a range of numbers that indicates the size of the array; for example,
 * if the preceding arguments were 2 and 5, the size of the array would
 * be (5 - 2 + 1) * sizeof(...).
 *
 * Note that multi-word parameters must be treated in this array as
 * *separate* arguments.  For example, Time structures are given as
 * TIME1 and TIME2.  This is because the procedures & structures in this
 * file do not know the actual number of arguments, but rather the number
 * of words that a system call is passed.
 */

#define PARM 		0
#define PARM_I 		SYS_PARAM_IN
#define PARM_O 		SYS_PARAM_OUT
#define PARM_IO		(SYS_PARAM_IN | SYS_PARAM_OUT)
#define PARM_IA 	(SYS_PARAM_IN | SYS_PARAM_ACC)
#define PARM_OA 	(SYS_PARAM_OUT | SYS_PARAM_ACC)
#define PARM_IOA	(PARM_IO | SYS_PARAM_ACC)
#define PARM_IC 	(SYS_PARAM_IN | SYS_PARAM_COPY)
#define PARM_OC		(SYS_PARAM_OUT | SYS_PARAM_COPY)
#define PARM_IOC	(PARM_IO | SYS_PARAM_COPY)
#define PARM_ICR 	(SYS_PARAM_IN | SYS_PARAM_COPY | SYS_PARAM_ARRAY)
#define PARM_OCR	(SYS_PARAM_OUT | SYS_PARAM_COPY | SYS_PARAM_ARRAY)

static Sys_CallParam paramsArray[] = {
    /* special */				/* SYS_PROC_FORK	0 */
    /* special */			     	/* SYS_PROC_EXEC	1 */
    /* special */ 				/* SYS_PROC_EXIT	2 */
    /* local */		      			/* SYS_SYNC_WAITTIME	3 */
    /* local */					/* SYS_TEST_PRINTOUT	4 */
    /* local */					/* SYS_TEST_GETLINE	5 */
    /* local */					/* SYS_TEST_GETCHAR	6 */
    /* local */					/* SYS_FS_OPEN		7 */
    /* local */					/* SYS_FS_READ		8 */
    /* local */					/* SYS_FS_WRITE		9 */
    /* local */					/* SYS_FS_CLOSE		10 */
    /* local */					/* SYS_FS_REMOVE	11 */
    /* local */					/* SYS_FS_REMOVE_DIR	12 */
    /* local */					/* SYS_FS_MAKE_DIR	13 */
    /* local */					/* SYS_FS_CHANGE_DIR	14 */
    /* special */			     	/* SYS_PROC_WAIT	15 */
    SYS_PARAM_INT,	      PARM_I,		/* SYS_PROC_DETACH	16 */
    SYS_PARAM_PROC_PID,	      PARM_OC,	     	/* SYS_PROC_GETIDS	17 */
    SYS_PARAM_PROC_PID,	      PARM_OC,
    SYS_PARAM_INT,	      PARM_OC,
    SYS_PARAM_INT,	      PARM_OC,
    SYS_PARAM_INT,	      PARM_I,		/* SYS_PROC_SETIDS	18 */
    SYS_PARAM_INT,	      PARM_I,
    /* local */				     	/* SYS_PROC_GETGROUPIDS 19 */
    /* local */				     	/* SYS_PROC_SETGROUPIDS 20 */
    SYS_PARAM_PROC_PID,	      PARM_I,		/* SYS_PROC_GETFAMILYID 21 */
    SYS_PARAM_PROC_PID,	      PARM_OC,		
    SYS_PARAM_PROC_PID,	      PARM_I,		/* SYS_PROC_SETFAMILYID 22 */
    SYS_PARAM_INT,	      PARM_I,
    /* test */					/* SYS_TEST_RPC		23 */
    /* test */					/* SYS_SYS_STATS	24 */
    /* local */					/* SYS_VM_CREATEVA	25 */
    /* local */					/* SYS_VM_DESTROYVA	26 */
    SYS_PARAM_INT,	      PARM_I,		/* SYS_SIG_SEND		27 */
    SYS_PARAM_PROC_PID,	      PARM_I,
    SYS_PARAM_INT,	      PARM_I,
    /* local */ 				/* SYS_SIG_PAUSE	28 */
    /* local */ 				/* SYS_SIG_SETHOLDMASK	29 */
    /* local */				     	/* SYS_SIG_SETACTION	30 */

    /* no args */			     	/* SYS_PROF_START	31 */
    /* no args */			     	/* SYS_PROF_END		32 */
    SYS_PARAM_FS_NAME,	      PARM_IA,		/* SYS_PROF_DUMP	33 */
    SYS_PARAM_VM_CMD,	      PARM_I,		/* SYS_VM_CMD		34 */
    SYS_PARAM_INT,	      PARM_I,
    SYS_PARAM_TIMEPTR,	      PARM_OC,		/* SYS_SYS_GETTIMEOFDAY 35 */
    SYS_PARAM_INT,	      PARM_OC,
    SYS_PARAM_INT,	      PARM_OC,
    SYS_PARAM_TIMEPTR,	      PARM_IC,		/* SYS_SYS_SETTIMEOFDAY 36 */
    SYS_PARAM_INT,	      PARM_I,
    SYS_PARAM_INT,	      PARM_I,
    /* local */				     	/* SYS_SYS_DONOTHING	37 */
    SYS_PARAM_RANGE1,	      PARM_I,		/* SYS_PROC_GETPCBINFO	38 */
    SYS_PARAM_RANGE2,	      PARM_I,
    SYS_PARAM_PCB,	      PARM_OCR,
    SYS_PARAM_PCBARG,         PARM_OCR,
    SYS_PARAM_INT,            PARM_OC,
    /* special (don't migrate?) */		/* SYS_VM_GETSEGINFO	39 */
    SYS_PARAM_PROC_PID,	      PARM_I,		/* SYS_PROC_GETRESUSAGE 40 */
    SYS_PARAM_PROC_RES,	      PARM_OC,
    SYS_PARAM_PROC_PID,	      PARM_I,		/* SYS_PROC_GETPRIORITY 41 */
    SYS_PARAM_INT,	      PARM_OC,
    SYS_PARAM_PROC_PID,	      PARM_I,		/* SYS_PROC_SETPRIORITY 42 */
    SYS_PARAM_INT,	      PARM_I,
    SYS_PARAM_INT,	      PARM_I,
    /* special (don't migrate?) */	     	/* SYS_PROC_DEBUG	43 */
    /* local case not implemented */		/* SYS_PROC_PROFILE	44 */
    /* local */					/* SYS_FS_TRUNC		45 */
    /* local */					/* SYS_FS_TRUNC_ID	46 */
    /* local */ 				/* SYS_FS_GET_NEW_ID	47 */
    /* local */					/* SYS_FS_GET_ATTRIBUTES 48 */
    /* local */ 				/* SYS_FS_GET_ATTR_ID	49 */
    /* local */					/* SYS_FS_SET_ATTRIBUTES 50 */
    /* local */ 				/* SYS_FS_SET_ATTR_ID	51 */
    /* local */					/* SYS_FS_SET_DEF_PERM	52 */
    /* local */			     		/* SYS_FS_IO_CONTROL	53 */
    SYS_PARAM_INT,	      PARM_I,		/* SYS_SYS_ENABLEDISPLAY 54 */
    SYS_PARAM_PROC_ENV_NAME,  PARM_IA,	     	/* SYS_PROC_SET_ENVIRON 55 */
    SYS_PARAM_PROC_ENV_VALUE, PARM_IA,
    SYS_PARAM_PROC_ENV_NAME,  PARM_IA,	     	/* SYS_PROC_UNSET_ENVIRON 56 */
    SYS_PARAM_DUMMY,  	      PARM,
    SYS_PARAM_PROC_ENV_NAME,  PARM_IA,	     	/* ..._GET_ENVIRON_VAR	57 */
    SYS_PARAM_PROC_ENV_VALUE, PARM_OC,
    /* special */				/* ..._GET_ENVIRON_RANGE 58 */
    /* special */				/* ..._INSTALL_ENVIRON	59 */
    /* no args */			     	/* SYS_PROC_COPY_ENVIRON 60 */
    /* local */					/* SYS_SYNC_SLOWLOCK	61 */
    /* local */					/* SYS_SYNC_SLOWWAIT	62 */
    /* local */					/* SYS_SYNC_SLOWBROADCAST 63 */
    /* local */					/* SYS_VM_PAGESIZE	64 */
    /* local */					/* SYS_FS_HARDLINK	65 */
    /* local */					/* SYS_FS_RENAME	66 */
    /* local */					/* SYS_FS_SYMLINK	67 */
    /* local */					/* SYS_FS_READLINK	68 */
    /* local */					/* SYS_FS_CREATEPIPE	69 */
    SYS_PARAM_INT,	      PARM_I,		/* ..VM_MAPKERNELINTOUSER 70 */
    SYS_PARAM_INT,	      PARM_I,
    SYS_PARAM_INT,	      PARM_I,
    SYS_PARAM_INT,	      PARM_OC,
    SYS_PARAM_FS_NAME,	      PARM_IA,		/* SYS_FS_ATTACH_DISK	71 */
    SYS_PARAM_FS_NAME,	      PARM_IA,
    SYS_PARAM_INT,	      PARM_I,
    /* local */			     		/* SYS_FS_SELECT	72 */
    SYS_PARAM_INT,	      PARM_I,		/* SYS_SYS_SHUTDOWN	73 */
    SYS_PARAM_STRING,	      PARM_IC,	
    SYS_PARAM_PROC_PID,	      PARM_I,		/* SYS_PROC_MIGRATE	74 */
    SYS_PARAM_INT,	      PARM_I,
    SYS_PARAM_FS_NAME, 	      PARM_IA,		/* SYS_FS_MAKE_DEVICE	75 */
    SYS_PARAM_FS_DEVICE,      PARM_IC,
    SYS_PARAM_INT,	      PARM_I,
    SYS_PARAM_INT,	      PARM_I,		/* SYS_FS_COMMAND	76 */
    SYS_PARAM_INT,	      PARM_I,
    SYS_PARAM_CHAR,           PARM_ICR,
    /* local */					/* SYS_FS_LOCK		77 */
    SYS_PARAM_INT,	      PARM_OC,		/* SYS_GETMACHINEINFO	78 */
    SYS_PARAM_INT,	      PARM_OC,
    SYS_PARAM_INT,	      PARM_OC,
    /* special */				/* SYS_NET_INSTALL_ROUTE 79 */
    /* local */					/* SYS_FS_READVECTOR	80 */
    /* local */					/* SYS_FS_WRITEVECTOR	81 */
    /* local */					/* SYS_FS_CHECKACCESS	82 */
    /* local */				/* SYS_PROC_GETINTERVALTIMER	83 */
    /* local */				/* SYS_PROC_SETINTERVALTIMER	84 */
    /* local */				/* SYS_FS_WRITEBACKID		85 */
    /* special */			     	/* SYS_PROC_EXEC_ENV	86 */
    /* local */				/* SYS_FS_SET_ATTR_NEW		87 */
    /* local */ 			/* SYS_FS_SET_ATTR_ID_NEW	88 */
    /*
     * Insert new system call information above this line.
     */
    NIL,		      NIL		/* array compatibility check */
};

/*
 * Define an array to count the number of system calls performed for local
 * and foreign processes, as well as subscripts and a macro to reset it.
 */

#define LOCAL_CALL 0
#define FOREIGN_CALL 1
int sys_NumCalls[SYS_NUM_SYSCALLS];
#define RESET_NUMCALLS() Byte_Zero(SYS_NUM_SYSCALLS * sizeof(int), \
				   (Address) sys_NumCalls)


/*
 *----------------------------------------------------------------------
 *
 * SysInitSysCall --
 *
 *	Initialize the data structures for performing a system call.
 *	Make sure the last entry in the array of parameters is (NIL, NIL)
 * 	(serving as a cross check on the total number of parameters to
 *	be initialized).  Initialize the count of the number of system
 *	calls performed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The mapping between system calls and their argument types is
 *	established.
 *
 *----------------------------------------------------------------------
 */

void
SysInitSysCall()
{
    int sysCallNum;
    SysCallEntry *entryPtr;
    Sys_CallParam *paramPtr;

    paramPtr = paramsArray;
    entryPtr = sysCalls;
    for (sysCallNum = 0; sysCallNum < SYS_NUM_SYSCALLS; sysCallNum++) {
	if (!entryPtr->special) {
	    entryPtr->paramsPtr = paramPtr;
	    paramPtr += entryPtr->numWords;
	/*
	 * Won't lint due to cast of function pointer.
	 */
#ifndef lint
	    Mach_InitSyscall(sysCallNum, entryPtr->numWords,
		    entryPtr->localFunc, SysMigCall);
#endif /* lint */
	} else {
	/*
	 * Won't lint due to cast of function pointer.
	 */
#ifndef lint
	    Mach_InitSyscall(sysCallNum, entryPtr->numWords,
		    entryPtr->localFunc, entryPtr->remoteFunc);
#endif /* lint */
	}
	entryPtr++;
    }
    if (paramPtr->type != NIL || paramPtr->disposition != NIL) {
	Sys_Panic(SYS_FATAL,
		  "SysInitSysCall: error initializing parameter array.\n");
    }
    RESET_NUMCALLS();
}

/*
 *----------------------------------------------------------------------
 *
 * SysMigCall --
 *
 *	This procedure is invoked whenever a migrated process invokes
 *	a kernel call that doesn't have "special" set.  It arranges
 *	for the kernel call to be sent home in a standard fashion.
 *
 * Results:
 *	Returns the result of the kernel call, whatever that is.
 *
 * Side effects:
 *	Depends on the kernel call.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
SysMigCall(args)
    Sys_ArgArray args;			/* The arguments to the system call. */
{
    int sysCallType;
    ReturnStatus status;
    register SysCallEntry *entryPtr;

    status = Vm_CopyIn(sizeof(sysCallType), Mach_GetStackPointer(),
	    (Address) &sysCallType);
    if (status != SUCCESS) {
	return SYS_ARG_NOACCESS;
    }
    entryPtr = &sysCalls[sysCallType];
    return (*entryPtr->remoteFunc)(sysCallType, entryPtr->numWords,
	    (ClientData *) &args, entryPtr->paramsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Sys_OutputNumCalls --
 *
 *	Copy the number of invocations of system calls into user space.
 *	Takes an argument, the number of calls to copy, which indicates the
 *	size of the user's buffer.  This is protection against any
 *	inconsistency between the kernel and user program's ideas of how
 *	many system calls there are.  An argument of 0 calls indicates that
 *	the statistics should be reset to 0.
 *
 * Results:
 *	The return status from Vm_CopyOut is returned.
 *
 * Side effects:
 *	Data is copied into user space.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
Sys_OutputNumCalls(numToCopy, buffer)
    int numToCopy;	/* number of system calls statistics to copy */
    Address buffer;
{
    ReturnStatus status = SUCCESS;

    if (numToCopy == 0) {
	RESET_NUMCALLS();
    } else {
	/*
	 * Are arrays stored in row-major or column-major order???
	 */
	status = Vm_CopyOut(numToCopy * sizeof(int), (Address) sys_NumCalls,
			    buffer);
    }
    return(status);
}
