/*
 * procInt.h.h --
 *
 *	Declarations internal to the proc module.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/proc/RCS/procInt.h,v 1.11 92/05/08 15:06:14 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _PROCINT
#define _PROCINT

#include <sprite.h>
#include <spriteTime.h>
#include <mach.h>

#include <fs.h>
#include <procMach.h>
#include <procTypes.h>

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
    Time			kernelCpuUsage;
    Time			userCpuUsage;
    Time			childKernelCpuUsage;
    Time			childUserCpuUsage;
} ProcChildInfo;

/* 
 * This is the process table index for the main thread.
 */
#define PROC_MAIN_PROC_SLOT 0


/*
 * Procedures internal to the proc module.
 */

extern void		ProcExitInit _ARGS_((void));
extern void 		ProcExitProcess _ARGS_((register 
				Proc_ControlBlock *exitProcPtr, int reason, 
				int status, int code, Boolean thisProcess));
extern void 		ProcFamilyHashInit _ARGS_((void));
extern void 		ProcDebugInit _ARGS_((void));
extern void		ProcDebugWakeup _ARGS_((void));
extern void 		ProcRecovInit _ARGS_((void));
extern void 		ProcFamilyRemove _ARGS_((Proc_ControlBlock *procPtr));
extern ReturnStatus 	ProcFamilyInsert _ARGS_((Proc_ControlBlock *procPtr, 
				int familyID));
extern	ReturnStatus	ProcChangeTimer _ARGS_((int timerType, 
				Proc_TimerInterval *newTimerPtr,
				Proc_TimerInterval *oldTimerPtr));
extern	void		ProcDeleteTimers _ARGS_((Proc_LockedPCB *procPtr));
extern  ReturnStatus	ProcRemoteWait _ARGS_((Proc_ControlBlock *procPtr,
				int flags, int numPids, Proc_PID pidArray[],
				ProcChildInfo *childInfoPtr));
#ifdef SPRITED_MIGRATION
extern	ReturnStatus	ProcRemoteFork
				_ARGS_((Proc_ControlBlock *parentProcPtr,
				Proc_ControlBlock *childProcPtr));
#else
#define ProcRemoteFork(a, b)	(FAILURE)
#endif
extern void 		ProcInitMainEnviron
    				_ARGS_((Proc_ControlBlock *procPtr));
extern void 		ProcSetupEnviron _ARGS_((Proc_ControlBlock *procPtr));
extern void 		ProcDecEnvironRefCount _ARGS_((
				Proc_EnvironInfo *environPtr));
extern	void		ProcAddToGroupList _ARGS_((Proc_LockedPCB *procPtr,
				int gid));
extern	void		ProcInitTable _ARGS_((void));
extern	Proc_LockedPCB	*ProcGetUnusedPCB _ARGS_((void));
extern	void		ProcFreePCB _ARGS_((Proc_LockedPCB *procPtr));
extern	int		ProcTableMatch _ARGS_((unsigned int maxPids,
				Boolean (*booleanFuncPtr)
					(Proc_ControlBlock *pcbPtr),
				Proc_PID *pidArray));
extern 	int		ProcGetObjInfo _ARGS_((Fs_Stream *filePtr,
				ProcExecHeader *execPtr,
				Proc_ObjInfo *objInfoPtr));

extern void		ProcDestroyServicePort _ARGS_((mach_port_t *portPtr));
extern void		ProcFreeTaskThread _ARGS_((
				Proc_LockedPCB *procPtr));
extern void		ProcKillThread _ARGS_((Proc_LockedPCB *procPtr));
extern mach_port_t	ProcMakeServicePort _ARGS_((mach_port_t name));
extern ReturnStatus	ProcMakeTaskThread _ARGS_((Proc_ControlBlock *procPtr,
				Proc_ControlBlock *parentProcPtr));
extern ProcTaskInfo	*ProcNewTaskInfo _ARGS_((void));
extern void		ProcReleaseTaskInfo _ARGS_((
				Proc_LockedPCB *procPtr));
extern void		ProcTaskThreadInit _ARGS_((void));

#endif /* _PROCINT */
