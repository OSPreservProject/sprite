/*
 * procMigrate.h --
 *
 *	Declarations of procedures and constants for process migration. 
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
 * $ProcMigrate: proto.h,v 1.4 86/03/20 14:00:21 andrew Exp $ SPRITE (Berkeley)
 */

#ifndef _PROCMIGRATE
#define _PROCMIGRATE

#include "proc.h"
#include "trace.h"
#include "sys.h"

extern int proc_MigDebugLevel;
extern Boolean proc_DoTrace;
extern Boolean proc_DoCallTrace;


#define PROC_MIG_ANY 0

/*
 * Define a macro to get a valid PCB for a migrated process.  This gets the
 * PCB corresponding to the process ID, and if it is a valid PCB the macro
 * then checks to make sure the process is migrated and from the specified
 * host.
 */

#define PROC_GET_MIG_PCB(pid, procPtr, hostID) 		    	\
	procPtr = Proc_LockPID(pid);	 			\
	if (procPtr != (Proc_ControlBlock *) NIL && 		\
	        (!(procPtr->genFlags & PROC_FOREIGN) || 	\
	        procPtr->peerHostID != remoteHostID)) {		\
	    Proc_Unlock(procPtr);				\
 	    procPtr = (Proc_ControlBlock *) NIL; 		\
	}

/*
 * Information returned upon first transferring a process state.
 */

typedef struct {
    ReturnStatus	status;
    Proc_PID		remotePID;
    unsigned int	filler;
} Proc_MigrateReply;

/*
 * Types of information passed during a migration:
 *
 * PROC_MIGRATE_PROC		- Process control block.
 * PROC_MIGRATE_VM		- Vm segment.
 * PROC_MIGRATE_FILES		- All files.
 * PROC_MIGRATE_FS_STREAM 	- A particular Fs_Stream.
 * PROC_MIGRATE_USER_INFO 	- User information that may change, such as
 *		  		  priorities or IDs.
 *
 * Another command that may be sent via RPC:
 *
 * PROC_MIGRATE_RESUME	- Resume execution of a migrated process.
 */

typedef enum {
    PROC_MIGRATE_PROC,
    PROC_MIGRATE_VM,
    PROC_MIGRATE_FILES,
    PROC_MIGRATE_FS_STREAM,
    PROC_MIGRATE_USER_INFO,
    PROC_MIGRATE_RESUME
} Proc_MigrateCommandType;
    
/* 
 * Information sent when transferring a segment.
 */

typedef struct {
    Proc_PID			remotePID;
    Proc_MigrateCommandType  	command;
    unsigned int		filler;
} Proc_MigrateCommand;
    
/*
 * Structure to contain information for the arguments to a system call
 * for a migrated process.    The size is the size of the argument passed
 * to or from the other node.  The disposition is SYS_PARAM_IN and/or
 * SYS_PARAM_OUT.
 */

typedef struct {
    int size;
    int disposition;
} Proc_ParamInfo;

/* 
 * Generic information sent when migrating a system call back to the
 * home machine.  The processID transferred is the ID of the process
 * on the machine servicing the RPC.
 */
	
typedef struct {
    Proc_PID			processID;
    int			 	callNumber;
    Boolean			parseArgs;
    int				numArgs;
    int				replySize;
    Proc_ParamInfo		info[SYS_MAX_ARGS];
} Proc_RemoteCall;

/*
 * Declare variables and constants for instrumentation.  First,
 * declare variables for the trace package.  These are followed by
 * structures that are passed into the trace package.  Each trace
 * record contains the process ID of the process being operated upon,
 * whether the operation is done for a process on its home node or a
 * remote one, and either a system call number and ReturnStatus or a
 * migration meta-command such as transferring state.
 */

extern Trace_Header proc_TraceHeader;
extern Trace_Header *proc_TraceHdrPtr;
#define PROC_NUM_TRACE_RECS 500

/*
 * "Events" for the trace package.
 *
 *	PROC_MIGTRACE_BEGIN_MIG 	- starting to transfer a process
 *	PROC_MIGTRACE_END_MIG 		- completed transferring a process
 *	PROC_MIGTRACE_TRANSFER  	- a particular transfer operation
 *	PROC_MIGTRACE_CALL		- a migrated system call
 *	
 */

#define PROC_MIGTRACE_BEGIN_MIG 	0
#define PROC_MIGTRACE_END_MIG 		1
#define PROC_MIGTRACE_TRANSFER  	2
#define PROC_MIGTRACE_CALL		3
#define PROC_MIGTRACE_MIGTRAP		4

typedef struct {
    int callNumber;
    ReturnStatus status;
} Proc_SysCallTrace;

typedef struct {
    Proc_MigrateCommandType type;
    ClientData data;
} Proc_CommandTrace;


typedef struct {
    Proc_PID processID;
    int flags;
    union {
	Proc_SysCallTrace call;
	Proc_CommandTrace command;
	int filler;
    } info;
} Proc_TraceRecord;
	
/*
 * Flags for Proc_TraceRecords:
 *
 * 	PROC_MIGTRACE_START - start of an RPC
 *	PROC_MIGTRACE_HOME  - operation is for a process on its home machine
 *
 * Both of these flags are boolean, so absence of a flag implies its
 * opposite (end of an RPC, or that the operation is for a foreign process).
 */

#define PROC_MIGTRACE_START	0x01
#define PROC_MIGTRACE_HOME	0x02

/*
 * Define a structure for passing information via callback for killing
 * a migrated process.  [Not used yet, but potentially.]
 */
typedef struct Proc_DestroyMigProcData {
    Proc_ControlBlock *procPtr;		/* local copy of process to kill */
    ReturnStatus status;		/* status to return when it exits */
} Proc_DestroyMigProcData;

/*
 * Functions for process migration.  [Others should be moved here.]
 */
extern void Proc_ResumeMigProc();
extern void Proc_DestroyMigratedProc();

extern void Proc_RemoveMigDependency();
extern void Proc_AddMigDependency();



#endif /* _PROCMIGRATE */
