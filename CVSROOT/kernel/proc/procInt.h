/*
 * procInt.h --
 *
 *	Declarations internal to the proc module.
 *
 * Copyright (C) 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $ProcInt: proto.h,v 1.4 86/03/20 14:00:21 andrew Exp $ SPRITE (Berkeley)
 */

#ifndef _PROCINT
#define _PROCINT
#include "procMigrate.h"
#include "migrate.h"
#include "proc.h"
#include "fs.h"
#include "procMach.h"
/*
 * Information used by the Proc_Wait command for child termination information.
 */

typedef struct {
    Proc_PID 			processID;
    int 			termReason;
    int 			termStatus;
    int 			termCode;
    int				numQuantumEnds;
    int				numWaitEvents;
    Timer_Ticks			kernelCpuUsage;
    Timer_Ticks			userCpuUsage;
    Timer_Ticks			childKernelCpuUsage;
    Timer_Ticks			childUserCpuUsage;
} ProcChildInfo;

/*
 * Machine independent object file information.
 */
typedef struct {
    Address	codeLoadAddr;	/* Address in user memory to load code. */
    unsigned	codeFileOffset;	/* Offset in obj file to load code from.*/
    unsigned	codeSize;	/* Size of code segment. */
    Address	heapLoadAddr;	/* Address in user memory to load heap. */
    unsigned	heapFileOffset;	/* Offset in obj file to load initialized heap
				 * from . */
    unsigned	heapSize;	/* Size of heap segment. */
    Address	bssLoadAddr;	/* Address in user memory to load bss. */
    unsigned	bssSize;	/* Size of bss segment. */
    Address	entry;		/* Entry point to start execution. */
} ProcObjInfo;

/*
 * Procedures internal to the proc module.
 */

extern void 		ProcRemoteExit _ARGS_((register 
				Proc_ControlBlock *procPtr, int reason, 
				int exitStatus, int code));
extern 	void 		ProcRemoteSuspend _ARGS_((Proc_ControlBlock *procPtr,
				int exitFlags));
extern void 		ProcExitProcess _ARGS_((register 
				Proc_ControlBlock *exitProcPtr, int reason, 
				int status, int code, Boolean thisProcess));
extern void 		ProcFamilyHashInit _ARGS_((void));
extern void 		ProcDebugInit _ARGS_((void));
extern	void		ProcDebugWakeup _ARGS_((void));
extern void 		ProcRecovInit _ARGS_((void));
extern void 		ProcFamilyRemove _ARGS_((Proc_ControlBlock *procPtr));
extern ReturnStatus 	ProcFamilyInsert _ARGS_((Proc_ControlBlock *procPtr, 
				int familyID));
extern	ReturnStatus	ProcChangeTimer _ARGS_((int timerType, 
				Proc_TimerInterval *newTimerPtr,
				Proc_TimerInterval *oldTimerPtr,
				Boolean userMode));
extern	void		ProcDeleteTimers _ARGS_((Proc_ControlBlock *procPtr));
extern ReturnStatus 	ProcExecGetEncapSize _ARGS_((Proc_ControlBlock *procPtr,
				int hostID, Proc_EncapInfo *infoPtr));
extern ReturnStatus 	ProcExecEncapState _ARGS_((register 
				Proc_ControlBlock *procPtr, int hostID, 
				Proc_EncapInfo *infoPtr, Address bufPtr));
extern ReturnStatus	ProcExecDeencapState _ARGS_((register 
				Proc_ControlBlock *procPtr, 
				Proc_EncapInfo *infoPtr, Address bufPtr));
extern ReturnStatus 	ProcExecFinishMigration _ARGS_((register 
				Proc_ControlBlock *procPtr, int hostID, 
				Proc_EncapInfo *infoPtr, Address bufPtr, 
				Boolean failure));
extern void 		ProcDoRemoteExec _ARGS_((register 
				Proc_ControlBlock *procPtr));
extern void 		ProcRemoteExec _ARGS_((register 
				Proc_ControlBlock *procPtr, int uid));
extern  void		ProcRecordUsage _ARGS_((Timer_Ticks ticks,
				ProcRecordUsageType type));
extern  ReturnStatus	ProcRemoteWait _ARGS_((Proc_ControlBlock *procPtr,
				int flags, int numPids, Proc_PID pidArray[],
				ProcChildInfo *childInfoPtr));
extern	ReturnStatus	ProcRemoteFork _ARGS_((Proc_ControlBlock *parentProcPtr,
				Proc_ControlBlock *childProcPtr));
extern	ReturnStatus	ProcInitiateMigration _ARGS_((
				Proc_ControlBlock *procPtr, int hostID));
extern	ReturnStatus	ProcServiceRemoteWait _ARGS_((
				Proc_ControlBlock *curProcPtr,
				int flags, int numPids, Proc_PID pidArray[],
				int waitToken, ProcChildInfo *childInfoPtr));
extern	void		ProcDebugInit _ARGS_((void));
extern void 		ProcInitMainEnviron _ARGS_((register 
				Proc_ControlBlock *procPtr));
extern void 		ProcSetupEnviron _ARGS_((register 
				Proc_ControlBlock *procPtr));
extern void 		ProcDecEnvironRefCount _ARGS_((register 
				Proc_EnvironInfo	*environPtr));
extern	ReturnStatus	ProcIsObj _ARGS_((Fs_Stream *streamPtr,
				int doErr));
extern	void		ProcAddToGroupList _ARGS_((Proc_ControlBlock *procPtr,
				int gid));
extern	ReturnStatus	ProcMigReceiveProcess _ARGS_((ProcMigCmd *cmdPtr,
				Proc_ControlBlock *procPtr,
				Proc_MigBuffer *inBufPtr,
				Proc_MigBuffer *outBufPtr));
extern	ReturnStatus	ProcMigGetUpdate _ARGS_((ProcMigCmd *cmdPtr,
				Proc_ControlBlock *procPtr,
				Proc_MigBuffer *inBufPtr,
				Proc_MigBuffer *outBufPtr));
extern	ReturnStatus	ProcMigGetSupend _ARGS_((ProcMigCmd *cmdPtr,
				Proc_ControlBlock *procPtr,
				Proc_MigBuffer *inBufPtr, 
				Proc_MigBuffer *outBufPtr));
extern	ReturnStatus	ProcMigEncapCallback _ARGS_((ProcMigCmd *cmdPtr,
				Proc_ControlBlock *procPtr,
				Proc_MigBuffer *inBufPtr,
				Proc_MigBuffer *outBufPtr));
extern	void		ProcMigKillRemoteCopy _ARGS_((Proc_PID processID));
extern	ReturnStatus	ProcMigCommand _ARGS_((int host, ProcMigCmd *cmdPtr,
				Proc_MigBuffer *inBufPtr, 
				Proc_MigBuffer *outBufPtr));
extern	void		ProcMigWakeupWaiters _ARGS_((void));
extern	void		ProcMigEvictionComplete _ARGS_((void));
extern	void		ProcMigAddDependency _ARGS_((Proc_PID processID,
				Proc_PID peerProcessID));
extern	void		ProcMigRemoveDependency _ARGS_((Proc_PID processID,
				Boolean notified));
extern	ReturnStatus	ProcMigAcceptMigration _ARGS_((ProcMigCmd *cmdPtr,
				Proc_ControlBlock *procPtr,
				Proc_MigBuffer *inBufPtr,
				Proc_MigBuffer *outBufPtr));
extern	ReturnStatus	ProcMigDestroyCmd _ARGS_((ProcMigCmd *cmdPtr,
				Proc_ControlBlock *procPtr,
				Proc_MigBuffer *inBufPtr,
				Proc_MigBuffer *outBufPtr));
extern	ReturnStatus	ProcMigContinueProcess _ARGS_((ProcMigCmd *cmdPtr,
				Proc_ControlBlock *procPtr,
				Proc_MigBuffer *inBufPtr,
				Proc_MigBuffer *outBufPtr));
extern	ReturnStatus	ProcMigGetSuspend _ARGS_((ProcMigCmd *cmdPtr,
				Proc_ControlBlock *procPtr,
				Proc_MigBuffer *inBufPtr,
				Proc_MigBuffer *outBufPtr));
extern	void		ProcInitTable _ARGS_((void));
extern	Proc_ControlBlock *ProcGetUnusedPCB _ARGS_((void));
extern	void		ProcFreePCB _ARGS_((Proc_ControlBlock *procPtr));
extern	int		ProcTableMatch _ARGS_((int maxPids,
				Boolean (*booleanFuncPtr)(),
				Proc_PID *pidArray));
extern 	int		ProcGetObjInfo _ARGS_((ProcExecHeader *execPtr,
				ProcObjInfo *objInfoPtr));
#endif /* _PROCINT */
