/*
 * proc.h --
 *
 *	External declarations of routines 
 *	for managing processes.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * rcsid $Header$ SPRITE (Berkeley)
 */

#ifndef _PROC
#define _PROC

#ifdef KERNEL
#include <procTypes.h>
#include <user/sync.h>
#include <syncLock.h>
#include <list.h>
#include <timer.h>
#include <sig.h>
#include <mach.h>
#include <sysSysCallParam.h>
#include <rpc.h>
#else
#include <kernel/procTypes.h>
#include <sync.h>
#include <kernel/syncLock.h>
#include <list.h>
#include <kernel/timer.h>
#include <kernel/sig.h>
#include <kernel/mach.h>
#include <kernel/rpc.h>
#endif /* */

/*
 *  proc_RunningProcesses points to an array of pointers to
 *  Proc_ControlBlock structures of processes currently running on each
 *  CPU.  It is initialized by Proc_Init at boot time to the appropriate
 *  size, which depends on the workstation configuration.
 */

extern Proc_ControlBlock  **proc_RunningProcesses;


/*
 *  proc_PCBTable is the array of all valid PCB's in the system.
 *  It is initialized by Proc_Init at boot time to the appropriate size,
 *  which depends on the workstation configuration.
 */

extern Proc_ControlBlock **proc_PCBTable;


/*
 *   Keep track of the maximum number of processes at any given time.
 */

extern int proc_MaxNumProcesses;

/*
 * set to TRUE to disallow all migrations to this machine.
 */
extern Boolean proc_RefuseMigrations;

/*
 *  Macros to manipulate process IDs.
 */

#define Proc_ComparePIDs(p1, p2) (p1 == p2)

#define Proc_GetPCB(pid) (proc_PCBTable[pid & PROC_INDEX_MASK])

#define Proc_ValidatePID(pid) \
    (((pid & PROC_INDEX_MASK) < proc_MaxNumProcesses) && \
     ((pid == proc_PCBTable[pid & PROC_INDEX_MASK]->processID)))

#define PROC_GET_VALID_PCB(pid, procPtr) \
    if ((pid & PROC_INDEX_MASK) >= proc_MaxNumProcesses) { \
	procPtr = (Proc_ControlBlock *) NIL; \
    } else { \
	procPtr = proc_PCBTable[pid & PROC_INDEX_MASK]; \
	if (pid != procPtr->processID) { \
	    procPtr = (Proc_ControlBlock *) NIL; \
	} \
    }

#define	Proc_GetHostID(pid) ((pid & PROC_ID_NUM_MASK) >> PROC_ID_NUM_SHIFT)

/*
 * Macros to determine and set the "actual" currently running process.
 */

#define	Proc_GetActualProc() \
	proc_RunningProcesses[Mach_GetProcessorNumber()]
#define Proc_SetActualProc(processPtr) \
	proc_RunningProcesses[Mach_GetProcessorNumber()] = processPtr

#define Proc_GetCurrentProc() Proc_GetActualProc()
#define Proc_SetCurrentProc(processPtr)	Proc_SetActualProc(processPtr)

/*
 * Various routines use Proc_IsMigratedProcess to decide whether the
 * effective process is different from the actual process (i.e.,
 * migrated).  This macro bypasses the procedure call, since
 * proc_RunningProcesses[processor] must be non-NIL.
 */

#define	Proc_IsMigratedProcess() \
    (proc_RunningProcesses[Mach_GetProcessorNumber()]->rpcClientProcess != \
		((Proc_ControlBlock *) NIL))

/*
 * Used to get the lock at the top of the lock stack without popping it off.
 */
#define Proc_GetCurrentLock(pcbPtr, typePtr, lockPtrPtr) \
    { \
	if ((pcbPtr)->lockStackSize <= 0) { \
	    *(typePtr) = -1; \
	    *(lockPtrPtr) = (Address) NIL; \
	} else { \
	    *(typePtr) = (pcbPtr)->lockStack[(pcbPtr)->lockStackSize-1].type; \
	    *(lockPtrPtr) = \
		(pcbPtr)->lockStack[(pcbPtr)->lockStackSize-1].lockPtr; \
	} \
    }
/* 
 * External procedures.
 */

extern ReturnStatus 	Proc_ByteCopy _ARGS_((Boolean copyIn, int numBytes,
				Address sourcePtr, Address destPtr));
extern void		Proc_CallFunc _ARGS_((void (*func)(
						   ClientData clientData, 
						   Proc_CallInfo *callInfoPtr), 
				ClientData clientData, unsigned int interval));
extern ClientData 	Proc_CallFuncAbsTime _ARGS_((void (*func)(
						   ClientData clientData, 
						   Proc_CallInfo *callInfoPtr), 
				ClientData clientData, Timer_Ticks time));
extern void 		Proc_CancelCallFunc _ARGS_((ClientData token));
extern	ReturnStatus	Proc_Debug _ARGS_((Proc_PID pid, 
				Proc_DebugReq request, int numBytes,
				Address srcAddr, Address destAddr));
extern	void		Proc_DestroyMigratedProc _ARGS_((ClientData pidData,
				Proc_CallInfo *callInfoPtr));
extern ReturnStatus 	Proc_Detach _ARGS_((int status));
extern void 		Proc_DetachInt _ARGS_((register 
				Proc_ControlBlock *procPtr));
extern	ReturnStatus	Proc_DoForEveryProc _ARGS_((Boolean (*booleanFuncPtr)
						(Proc_ControlBlock *pcbPtr),
				ReturnStatus (*actionFuncPtr)(Proc_PID pid), 
				Boolean ignoreStatus, int *numMatchedPtr));
extern ReturnStatus 	Proc_DoRemoteCall _ARGS_((int callNumber, int numWords,
				ClientData *argsPtr, Sys_CallParam *specsPtr));
extern	ReturnStatus	Proc_Dump _ARGS_((void));
extern	void		Proc_DumpPCB _ARGS_((Proc_ControlBlock *procPtr));
extern	ReturnStatus	Proc_EvictForeignProcs _ARGS_((void));
extern	ReturnStatus	Proc_EvictProc _ARGS_((Proc_PID pid));
extern	int		Proc_Exec _ARGS_((char *fileName, 
				char **argsPtrArray, char **envPtrArray,
				Boolean debugMe, int host));
extern int 		Proc_ExecEnv _ARGS_((char *fileName, 
				char **argPtrArray, char **envPtrArray, 
				Boolean debugMe));
extern void 		Proc_Exit _ARGS_((int status));
extern void 		Proc_ExitInt _ARGS_((int reason, int status, int code));
extern	void		Proc_FlagMigration _ARGS_((Proc_ControlBlock *procPtr,
				int hostID, Boolean exec));
extern	int		Proc_Fork _ARGS_((Boolean shareHeap, Proc_PID *pidPtr));
extern	Proc_ControlBlock *Proc_GetEffectiveProc _ARGS_((void));
extern	ReturnStatus	Proc_GetFamilyID _ARGS_((Proc_PID pid,
				Proc_PID *familyIDPtr));
extern	ReturnStatus	Proc_GetGroupIDs _ARGS_((int numGIDs, int *gidArrayPtr,
				int *trueNumGIDsPtr));
extern	ReturnStatus	Proc_GetHostIDs _ARGS_((int *virtualHostPtr, 
				int *physicalHostPtr));
extern	ReturnStatus	Proc_GetIDs _ARGS_((Proc_PID *procIDPtr,
				Proc_PID *parentIDPtr, int *userIDPtr,
				int *effUserIDPtr));
extern	ReturnStatus	Proc_GetIntervalTimer _ARGS_((int timerType,
				Proc_TimerInterval *userTimerPtr));
extern	ReturnStatus	Proc_GetPCBInfo _ARGS_((Proc_PID firstPid,
				Proc_PID lastPid, int hostID, int infoSize,
				Address bufferPtr, Proc_PCBArgString *argsPtr,
				int *trueNumBuffersPtr));
extern	ReturnStatus	Proc_GetPriority _ARGS_((Proc_PID pid, 
				int *priorityPtr));
extern	ReturnStatus	Proc_GetRemoteSegInfo _ARGS_((int hostID, int segNum,
				Vm_SegmentInfo *segInfoPtr));
extern	ReturnStatus	Proc_GetResUsage _ARGS_((Proc_PID pid, 
				Proc_ResUsage *bufferPtr));
extern	Boolean		Proc_HasPermission _ARGS_((int userID));
extern void 		Proc_InformParent _ARGS_((register 
				Proc_ControlBlock *procPtr, int childStatus));
extern void 		Proc_Init _ARGS_((void));
extern	void		Proc_InitMainEnviron _ARGS_((
				Proc_ControlBlock *procPtr));
extern void 		Proc_InitMainProc _ARGS_((void));
extern	Boolean		Proc_IsMigratedProc _ARGS_((
				Proc_ControlBlock *procPtr));
extern	int		Proc_KillAllProcesses _ARGS_((Boolean userProcsOnly));
extern	void		Proc_KDump _ARGS_((void));
extern	void		Proc_Lock _ARGS_((Proc_ControlBlock *procPtr));
extern ReturnStatus 	Proc_LockFamily _ARGS_((int familyID, 
				List_Links **familyListPtr, int *userIDPtr));
extern	Proc_ControlBlock *Proc_LockPID _ARGS_((Proc_PID pid));
extern ReturnStatus 	Proc_MakeStringAccessible _ARGS_((int maxLength,
				char **stringPtrPtr, int *accessLengthPtr,
				int *newLengthPtr));
extern void 		Proc_MakeUnaccessible _ARGS_((Address addr, 
				int numBytes));
extern	void		Proc_MigAddToCounter _ARGS_((int value, 
				unsigned int *intPtr, 
				unsigned int *squaredPtr));
extern	ReturnStatus	Proc_MigGetStats _ARGS_((Address addr));
extern	void		Proc_MigInit _ARGS_((void));
extern	ReturnStatus	Proc_Migrate _ARGS_((Proc_PID pid, int hostID));
extern	void		Proc_MigrateStartTracing _ARGS_((void));
extern	void		Proc_MigrateTrap _ARGS_((Proc_ControlBlock *procPtr));
extern	ReturnStatus	Proc_MigResetStats _ARGS_((void));
extern ReturnStatus	Proc_MigUpdateInfo _ARGS_((Proc_ControlBlock *procPtr));
extern	void		Proc_NeverMigrate _ARGS_((Proc_ControlBlock *procPtr));
extern ReturnStatus	Proc_NewProc _ARGS_((Address PC, int procType,
				Boolean shareHeap, Proc_PID *pidPtr,
				char *procName));
extern	void		Proc_NotifyMigratedWaiters _ARGS_((Proc_PID pid,
				Proc_CallInfo *callInfoPtr));
extern	ReturnStatus	Proc_Profile _ARGS_((int shiftSize, int lowPC,
				int highPC, Time interval, int counterArray[]));
extern	void		Proc_PushLockStack _ARGS_((Proc_ControlBlock *pcbPtr,
				int type, Address lockPtr));
extern void 		Proc_Reaper _ARGS_((register 
				Proc_ControlBlock *procPtr, 
				Proc_CallInfo *callInfoPtr));
extern void		Proc_ResumeProcess _ARGS_((Proc_ControlBlock *procPtr,
				Boolean killingProc));
extern	ReturnStatus	Proc_RemoteDummy _ARGS_((int callNumber, 
				int numWords, Sys_ArgArray *argsPtr,
				Sys_CallParam *specsPtr));
extern int 		Proc_RemoteExec _ARGS_((char *fileName, 
				char **argPtrArray,char **envPtrArray, 
				int host));
extern	void		Proc_RemoveFromLockStack _ARGS_((
				Proc_ControlBlock *pcbPtr, Address lockPtr));
extern	void		Proc_ResumeMigProc _ARGS_((int pc));
extern	ReturnStatus	Proc_RpcGetPCB _ARGS_((ClientData srvToken,
				int clientID, int command,
				Rpc_Storage *storagePtr));
extern	ReturnStatus	Proc_RpcMigCommand _ARGS_((ClientData srvToken,
				int hostID, int command, 
				Rpc_Storage *storagePtr));
extern	ReturnStatus	Proc_RpcRemoteCall _ARGS_((ClientData srvToken,
				int clientID, int command, 
				Rpc_Storage *storagePtr));
extern ReturnStatus 	Proc_RpcRemoteWait _ARGS_((ClientData srvToken,
				int clientID, int command, 
				Rpc_Storage *storagePtr));
extern void 		Proc_UnlockFamily _ARGS_((int familyID));
extern void 		Proc_ServerInit _ARGS_((void));
extern void 		Proc_ServerProc _ARGS_((void));
extern	void		Proc_SetEffectiveProc _ARGS_((
				Proc_ControlBlock *procPtr));
extern	ReturnStatus	Proc_SetFamilyID _ARGS_((Proc_PID pid, 
				Proc_PID familyID));
extern	ReturnStatus	Proc_SetGroupIDs _ARGS_((int numGIDs, 
				int *gidArrayPtr));
extern	ReturnStatus	Proc_SetIDs _ARGS_((int userID, int effUserID));
extern	ReturnStatus	Proc_SetIntervalTimer _ARGS_((int timerType,
				Proc_TimerInterval *newTimerPtr,
				Proc_TimerInterval *oldTimerPtr));
extern	ReturnStatus	Proc_SetPriority _ARGS_((Proc_PID pid, int priority,
				Boolean useFamily));
extern	void		Proc_SetServerPriority _ARGS_((Proc_PID pid));
extern	void		Proc_SetupEnviron _ARGS_((Proc_ControlBlock *procPtr));
extern	ReturnStatus	Proc_StringNCopy _ARGS_((int numBytes, char *srcStr,
				char *destStr, int *strLengthPtr));
extern	void		Proc_SuspendProcess _ARGS_((
				Proc_ControlBlock *procPtr, Boolean debug,
				int termReason, int termStatus, 
				int termCode));
extern	void		Proc_Unlock _ARGS_((Proc_ControlBlock *procPtr));
extern	ReturnStatus	Proc_Wait _ARGS_((int numPids, Proc_PID pidArray[],
				int flags, Proc_PID *procIDPtr, 
				int *reasonPtr, int *statusPtr, 
				int *subStatusPtr, Proc_ResUsage *usagePtr));
extern	ReturnStatus	Proc_WaitForHost _ARGS_((int hostID));
extern	ReturnStatus	Proc_WaitForMigration _ARGS_((Proc_PID processID));
extern	void		Proc_WakeupAllProcesses _ARGS_((void));
extern	int		proc_NumServers;
/*
 * The following are kernel stubs corresponding to system calls.  They
 * used to be known by the same name as the system call, but the C library
 * has replaced them at user level in order to use the stack environments.
 * The "Stub" suffix therefore avoids naming conflicts with the library.
 */

extern	ReturnStatus	Proc_SetEnvironStub _ARGS_((
				Proc_EnvironVar environVar));
extern	ReturnStatus	Proc_UnsetEnvironStub _ARGS_((
				Proc_EnvironVar environVar));
extern	ReturnStatus	Proc_GetEnvironVarStub _ARGS_((
				Proc_EnvironVar environVar));
extern	ReturnStatus	Proc_GetEnvironRangeStub _ARGS_((int first, int last,
				Proc_EnvironVar *envArray, 
				int *numActualVarsPtr));
extern	ReturnStatus	Proc_CopyEnvironStub _ARGS_((void));
extern	ReturnStatus	Proc_InstallEnvironRangeStub _ARGS_((
				Proc_EnvironVar environVar, int numVars));
extern 	int 		Proc_KernExec _ARGS_((char *fileName, 
				char **argPtrArray));

#endif /* _PROC */
