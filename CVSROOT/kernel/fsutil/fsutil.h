/*
 * fsutil.h --
 *
 *	Declarations of utility routines used by all the file system modules.
 *	This file defines handle types and some sub-structures
 *	that are embedded in various types of handles.  A
 *	"handle" is a data structure that corresponds one-for-one
 *	with a file system object, i.e. a particular file, a device,
 *	a pipe, or a pseudo-device.  A handle is not always one-for-one
 *	with a file system name.  Devices can have more than one name,
 *	and pseudo-devices have many handles associated with one name.
 *	Each handle is identfied by a unique Fs_FileID, and has a standard
 *	header for manipulation by generic routines.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSUTIL
#define _FSUTIL

#include <stdlib.h>

#ifdef KERNEL
#include <fs.h>
#include <fsconsist.h>
#include <hash.h>
#include <proc.h>
#include <vm.h>
#include <rpc.h>
#include <timer.h>
#else
#include <kernel/fs.h>
#include <kernel/fsconsist.h>
#include <kernel/hash.h>
#include <kernel/proc.h>
#include <kernel/vm.h>
#include <kernel/rpc.h>
#include <kernel/timer.h>
#endif
/* constants */
/*
 * Define the types of files that we care about in the kernel, for such
 * things as statistics gathering, write-through policy, etc.  There is not
 * necessarily a one-to-one mapping between these and the types defined
 * in user/fs.h as FS_USER_TYPE_*; for example, FS_USER_TYPE_BINARY and
 * FS_USER_TYPE_OBJECT were mapped into FSUTIL_FILE_TYPE_DERIVED before they
 * were separated into two categories.  It would be possible to flag other
 * derived files (text formatting output, for example) to be in the DERIVED
 * category as well.  
 */
#define FSUTIL_FILE_TYPE_TMP 0
#define FSUTIL_FILE_TYPE_SWAP 1
#define FSUTIL_FILE_TYPE_DERIVED 2
#define FSUTIL_FILE_TYPE_BINARY 3
#define FSUTIL_FILE_TYPE_OTHER 4

/*
 * How often to try rewriting a file.
 */
#define	FSUTIL_WRITE_RETRY_INTERVAL	30

/* data structures */

/*
 * Files or devices with remote I/O servers need to keep some recovery
 * state to handle recovery after their server reboots.
 */

typedef struct Fsutil_RecoveryInfo {
    Sync_Lock		lock;		/* This struct is monitored */
    Sync_Condition	reopenComplete;	/* Notified when the handle has been
					 * re-opened at the I/O server */
    int			flags;		/* defined in fsRecovery.c */
    ReturnStatus	status;		/* Recovery status */
    Fsio_UseCounts		use;		/* Client's copy of use state */
} Fsutil_RecoveryInfo;

/*
 * Statistics for testing recovery.
 */
typedef	struct	Fsutil_FsRecovNamedStats {
    Fs_FileID		fileID;			/* Unique id for object. */
    Boolean		streamHandle;		/* Is this a stream handle? */
    int			mode;			/* Mode of stream. */
    int			refCount;		/* Ref count on IO handle. */
    int			streamRefCount;		/* Ref count on stream. */
    int			numBlocks;		/* Number of blocks in cache. */
    int			numDirtyBlocks;		/* Number dirty cache blocks. */
    char		name[50];		/* Name of object. */
} Fsutil_FsRecovNamedStats;


extern Boolean fsconsist_Debug;
/*
 * Whether or not to flush the cache at regular intervals.
 */

extern Boolean fsutil_ShouldSyncDisks;

extern	int	fsutil_NumRecovering;

/* procedures */
/*
 * Fsutil_StringNCopy
 *
 *	Copy the null terminated string in srcStr to destStr and return the
 *	actual length copied in *strLengthPtr.  At most numBytes will be
 *	copied if the string is not null-terminated.
 */

#define	Fsutil_StringNCopy(numBytes, srcStr, destStr, strLengthPtr) \
    (Proc_IsMigratedProcess() ? \
	    Proc_StringNCopy(numBytes, srcStr, destStr, strLengthPtr) : \
	    Vm_StringNCopy(numBytes, srcStr, destStr, strLengthPtr))

/*
 * Macros to handle type casting when dealing with handles.
 */
#define Fsutil_HandleFetchType(type, fileIDPtr) \
    (type *)Fsutil_HandleFetch(fileIDPtr)

#define Fsutil_HandleDupType(type, handlePtr) \
    (type *)Fsutil_HandleDup((Fs_HandleHeader *)handlePtr)

#define Fsutil_HandleLock(handlePtr) \
    Fsutil_HandleLockHdr((Fs_HandleHeader *)handlePtr)

#define Fsutil_HandleUnlock(handlePtr) \
    (void)Fsutil_HandleUnlockHdr((Fs_HandleHeader *)handlePtr)

#define Fsutil_HandleRelease(handlePtr, locked) \
    Fsutil_HandleReleaseHdr((Fs_HandleHeader *)handlePtr, locked)

#define Fsutil_HandleRemove(handlePtr) \
    Fsutil_HandleRemoveHdr((Fs_HandleHeader *)handlePtr)

#define Fsutil_HandleName(handlePtr) \
    ((((Fs_HandleHeader *)handlePtr) == (Fs_HandleHeader *)NIL) ? \
	    "(no handle)": \
      ((((Fs_HandleHeader *)handlePtr)->name == (char *)NIL) ? "(no name)" : \
	((Fs_HandleHeader *)handlePtr)->name) )

#define	Fsutil_TimeInSeconds()	(Timer_GetUniversalTimeInSeconds())

#define mnew(type)	(type *)malloc(sizeof(type))

#ifdef SOSP91
/*
 * We are borrowing a couple of bits from the handle to record the read/write
 * status of the stream.  Each stream has a handle so we can get away with
 * this.
 */

#define FSUTIL_RW_FLAGS 	0x300
#define FSUTIL_RW_READ		0x100
#define FSUTIL_RW_WRITE		0x200

#endif

extern void Fsutil_RecoveryInit _ARGS_((Fsutil_RecoveryInfo *recovPtr));
extern void Fsutil_RecoverySyncLockCleanup _ARGS_((
		Fsutil_RecoveryInfo *recovPtr));
extern void Fsutil_WantRecovery _ARGS_((Fs_HandleHeader *hdrPtr));
extern void Fsutil_AttemptRecovery _ARGS_((ClientData data, 
		Proc_CallInfo *callInfoPtr));
extern ReturnStatus Fsutil_WaitForRecovery _ARGS_((Fs_HandleHeader *hdrPtr, 
		ReturnStatus rpcStatus));
extern Boolean Fsutil_RecoveryNeeded _ARGS_((Fsutil_RecoveryInfo *recovPtr));
extern void Fsutil_Reopen _ARGS_((int serverID, ClientData clientData));
extern Boolean Fsutil_RemoteHandleScavenge _ARGS_((Fs_HandleHeader *hdrPtr));
extern void Fsutil_ClientCrashed _ARGS_((int spriteID, ClientData clientData));
extern void Fsutil_ClientCrashed _ARGS_((int spriteID, ClientData clientData));
extern void Fsutil_RemoveClient _ARGS_((int clientID));


/*
 * Wait list routines.  Waiting lists for various conditions are kept
 * hanging of I/O handles.
 */
extern void Fsutil_WaitListInsert _ARGS_((List_Links *list, 
		Sync_RemoteWaiter *waitPtr));
extern void Fsutil_WaitListNotify _ARGS_((List_Links *list));
extern void Fsutil_FastWaitListInsert _ARGS_((List_Links *list, 
		Sync_RemoteWaiter *waitPtr));
extern void Fsutil_FastWaitListNotify _ARGS_((List_Links *list));
extern void Fsutil_WaitListDelete _ARGS_((List_Links *list));
extern void Fsutil_WaitListRemove _ARGS_((List_Links *list, 
		Sync_RemoteWaiter *waitPtr));

/*
 * File handle routines.
 */
extern void Fsutil_HandleInit _ARGS_((int fileHashSize));
extern Boolean Fsutil_HandleInstall _ARGS_((Fs_FileID *fileIDPtr, 
	int size, char *name, Boolean cantBlock, Fs_HandleHeader **hdrPtrPtr));
extern Fs_HandleHeader *Fsutil_HandleFetch _ARGS_((Fs_FileID *fileIDPtr));
extern Fs_HandleHeader *Fsutil_HandleFetchNoWait _ARGS_((Fs_FileID *fileIDPtr,
						Boolean *wouldWaitPtr));
extern Fs_HandleHeader *Fsutil_HandleDup _ARGS_((Fs_HandleHeader *hdrPtr));
extern Fs_HandleHeader *Fsutil_GetNextHandle _ARGS_((Hash_Search *hashSearchPtr));
extern void Fsutil_HandleLockHdr _ARGS_((Fs_HandleHeader *hdrPtr));
extern void Fsutil_HandleIncRefCount _ARGS_((Fs_HandleHeader *hdrPtr,
		int amount));
extern void Fsutil_HandleDecRefCount _ARGS_((Fs_HandleHeader *hdrPtr));
extern void Fsutil_HandleInvalidate _ARGS_((Fs_HandleHeader *hdrPtr));
extern Boolean Fsutil_HandleValid _ARGS_((Fs_HandleHeader *hdrPtr));
extern Boolean Fsutil_HandleUnlockHdr _ARGS_((Fs_HandleHeader *hdrPtr));
extern void Fsutil_HandleReleaseHdr _ARGS_(( Fs_HandleHeader *hdrPtr, 
		Boolean locked));
extern void Fsutil_HandleRemoveHdr _ARGS_((Fs_HandleHeader *hdrPtr));
extern Boolean Fsutil_HandleAttemptRemove _ARGS_((Fs_HandleHeader *hdrPtr));
extern void Fsutil_HandleRemoveInt _ARGS_((Fs_HandleHeader *hdrPtr));
/*
 * Miscellaneous.
 */
extern void Fsutil_FileError _ARGS_((Fs_HandleHeader *hdrPtr, char *string, 
		int status));
extern void Fsutil_PrintStatus _ARGS_((int status));
extern char *Fsutil_FileTypeToString _ARGS_((int type));

extern ReturnStatus Fsutil_DomainInfo _ARGS_((Fs_FileID *fileIDPtr, 
		Fs_DomainInfo *domainInfoPtr));

extern int Fsutil_HandleDescWriteBack _ARGS_((Boolean shutdown, int domain));
extern void Fsutil_SyncProc _ARGS_((ClientData data, 
		Proc_CallInfo *callInfoPtr));
extern void Fsutil_Sync _ARGS_((unsigned int writeBackTime, Boolean shutdown));
extern void Fsutil_SyncStub _ARGS_((ClientData data));
extern ReturnStatus Fsutil_WaitForHost _ARGS_((Fs_Stream *streamPtr, int flags,
		ReturnStatus rpcStatus));
extern int Fsutil_TraceInit _ARGS_((void));
extern int Fsutil_PrintTraceRecord _ARGS_((ClientData clientData, int event,
		Boolean printHeaderFlag));
extern void Fsutil_PrintTrace _ARGS_((ClientData clientData));
extern ReturnStatus Fsutil_RpcRecovery _ARGS_((ClientData srvToken, 
		int clientID, int command, Rpc_Storage *storagePtr));


extern void Fsutil_HandleScavengeStub _ARGS_((ClientData data));
extern void Fsutil_HandleScavenge _ARGS_((ClientData data, 
		Proc_CallInfo *callInfoPtr));

extern char *Fsutil_GetFileName _ARGS_((Fs_Stream *streamPtr));

extern ReturnStatus Fsutil_FsRecovInfo _ARGS_((int length, 
		Fsutil_FsRecovNamedStats *resultPtr, int *lengthNeededPtr));

extern int Fsutil_TestForHandles _ARGS_((int serverID));
#endif /* _FSUTIL */
