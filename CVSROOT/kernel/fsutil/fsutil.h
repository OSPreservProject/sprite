/*
 * fsInt.h --
 *
 *      Filesystem wide internal types and definitions for the fs module.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
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

#ifndef _FSINT
#define _FSINT

#include "status.h"
#include "list.h"
#include "timer.h"
#include "mem.h"
#include "byte.h"
#include "string.h"

/*
 * Stream Types:
 *	FS_STREAM		Top level type for stream with offset.  Streams
 *				have an ID and are in the handle table to
 *				support migration and shared stream offsets.
 * The remaining types are for I/O handles
 *	FS_LCL_FILE_STREAM	For a regular disk file stored locally.
 *	FS_RMT_FILE_STREAM	For a remote Sprite file.
 *	FS_LCL_DEVICE_STREAM	For a device on this host.
 *	FS_RMT_DEVICE_STREAM	For a remote device.
 *	FS_LCL_PIPE_STREAM	For an anonymous pipe buffered on this host.
 *	FS_RMT_PIPE_STREAM	For an anonymous pipe bufferd on a remote host.
 *	FS_LCL_NAMED_PIPE_STREAM	For a named pipe cached on this host.
 *	FS_RMT_NAMED_PIPE_STREAM	For a named pipe cached elsewhere.
 *	FS_CONTROL_STREAM	This is the stream used by the server for
 *		a pseudo device to listen for new client arrivals and the
 *		departure of old clients.
 *	FS_SERVER_STREAM	This is the stream used by the server for
 *		the request/reponse channel to a particular client.
 *	FS_LCL_PSEUDO_STREAM	This is the client's end of the stream
 *		between it and the server process for the pseudo device.
 *	FS_RMT_PSUEDO_STREAM	As above, but when the server is remote.
 *	FS_REMOTE_UNIX_STREAM	For files on the old hybrid unix/sprite server.
 *	FS_REMOTE_NFS_STREAM	(unimplemented) For NFS access.
 */
#define FS_STREAM			0
#define FS_LCL_FILE_STREAM		1
#define FS_RMT_FILE_STREAM		2
#define FS_LCL_DEVICE_STREAM		3
#define FS_RMT_DEVICE_STREAM		4
#define FS_LCL_PIPE_STREAM		5
#define FS_RMT_PIPE_STREAM		6
#define FS_CONTROL_STREAM		7
#define FS_SERVER_STREAM		8
#define FS_LCL_PSEUDO_STREAM		9
#define FS_RMT_PSEUDO_STREAM		10
#define FS_LCL_NAMED_PIPE_STREAM	11
#define FS_RMT_NAMED_PIPE_STREAM	12
#define FS_RMT_UNIX_STREAM		13
#define FS_RMT_NFS_STREAM		14


/*
 * The following structures are subfields of the various I/O handles.
 * First we define a use count structure to handle common book keeping needs.
 */

typedef struct FsUseCounts {
    int		ref;		/* The number of referneces to handle */
    int		write;		/* The number of writers on handle */
    int		exec;		/* The number of executors of handle */
} FsUseCounts;			/* 12 BYTES */

/*
 * There is a system call to lock a file.  The lock state is kept in
 * the I/O handle on the I/O server so that the lock has network-wide effect.
 */

typedef struct FsLockState {
    int		flags;		/* Bits defined below */
    List_Links	waitList;	/* List of processes to wakeup when the
				 * file gets unlocked */
    int		numShared;	/* Number of shared lock holders */
} FsLockState;			/* 16 BYTES */

/*
 * (The following lock bits are defined in user/fs.h)
 * IOC_LOCK_EXCLUSIVE - only one process may hold an exclusive lock.
 * IOC_LOCK_SHARED    - many processes may hold shared locks as long as
 *	there are no exclusive locks held.  Exclusive locks have to
 *	wait until all shared locks go away.
 */

/*
 * Files or devices with remote I/O servers need to keep some recovery
 * state to handle recovery after their server reboots.
 */

typedef struct FsRecoveryInfo {
    Sync_Lock		lock;		/* This struct is monitored */
    Sync_Condition	reopenComplete;	/* Notified when the handle has been
					 * re-opened at the I/O server */
    int			flags;		/* WANT_RECOVERY, RECOVERY_FAILED. */
    FsUseCounts		use;		/* Client's copy of use state */
} FsRecoveryInfo;			/* 28 BYTES */

/*
 * Values for the recovery info flags field.
 *	FS_WANT_RECOVERY	The handle needs to be re-opened at the server.
 *	FS_RECOVERY_FAILED	The last re-open attempt failed.
 */
#define FS_WANT_RECOVERY	0x1
#define FS_RECOVERY_FAILED	0x2


/*
 * Cache information for each file.
 */

typedef struct FsCachedAttributes {
    int		firstByte;	/* Cached version of desc. firstByte */
    int		lastByte;	/* Cached version of desc. lastByte */
    int		accessTime;	/* Cached version of access time */
    int		modifyTime;	/* Cached version of modify time */
    int		createTime;	/* Create time (won't change, but passed
				 * to clients for use in
				 * statistics-gathering) */
    int		userType;	/* user advisory file type, defined in
				 * user/fs.h */
    /*
     * The following fields are needed by Proc_Exec.
     */
    int		permissions;	/* File permissions */
    int		uid;		/* User ID of owner */
    int		gid;		/* Group Owner ID */
} FsCachedAttributes;		/* 36 BYTES */

typedef struct FsCacheFileInfo {
    List_Links	   links;	   /* Links for the list of dirty files.
				      THIS MUST BE FIRST in the struct */
    List_Links	   dirtyList;	   /* List of dirty blocks for this file.
				    * THIS MUST BE SECOND, see the macro
				    * in fsBlockCache.c that depends on it. */
    List_Links	   blockList;      /* List of blocks for the file */
    List_Links	   indList;	   /* List of indirect blocks for the file */
    Sync_Lock	   lock;	   /* This is used to serialize cache access */
    int		   flags;	   /* Flags to indicate the state of the
				      file, defined in fsBlockCache.h */
    int		   version;	   /* Used to verify validity of cached data */
    struct FsHandleHeader *hdrPtr; /* Back pointer to I/O handle */
    int		   blocksInCache;  /* The number of blocks that this file has
				      in the cache. */
    int		   blocksWritten;  /* The number of blocks that have been
				    * written in a row without requiring a 
				    * sync of the servers cache. */
    int		   numDirtyBlocks; /* The number of dirty blocks in the cache.*/
    Sync_Condition noDirtyBlocks;  /* Notified when all write backs done. */
    int		   lastTimeTried;  /* Time that last tried to see if disk was
				    * available for this block. */
    FsCachedAttributes attr;	   /* Local version of descriptor attributes. */
} FsCacheFileInfo;		   /* 84 BYTES */

/*
 * The client use state needed to allow remote client access and to
 * enforce network cache consistency.  A list of state for each client
 * using the file is kept, including the name server itself.  This
 * is used to determine what cache consistency actions to take.
 * There is synchronization over this list between subsequent open
 * operations and the cache consistency actions themselves.
 */

typedef struct FsConsistInfo {
    Sync_Lock	lock;		/* Monitor lock used to synchronize access
				 * to cache consistency routines and the
				 * consistency list. */
    int		flags;		/* Flags defined in fsCacheConsist.c */
    int		lastWriter;	/* Client id of last client to have it open
				   for write caching. */
    int		openTimeStamp;	/* Generated on the server when the file is
				 * opened.  Checked on clients to catch races
				 * between open replies and consistency
				 * messages */
    FsHandleHeader *hdrPtr;	/* Back pointer to handle header needed to
				 * identify the file. */
    List_Links	clientList;	/* List of clients of this file.  Scanned
				 * to determine cachability conflicts */
    List_Links	msgList;	/* List of outstanding cache
				 * consistency messages. */
    Sync_Condition consistDone;	/* Opens block on this condition
				 * until ongoing cache consistency
				 * actions have completed */
    Sync_Condition repliesIn;	/* This condition is notified after
				 * all the clients told to take
				 * consistency actions have replied. */
    List_Links migList;		/* List header for clients migrating the file */
} FsConsistInfo;		/* 48 BYTES */

/* 
 * The I/O descriptor for remote streams.  This is all that is needed for
 *	remote devices, remote pipes, and named pipes that are not cached
 *	on the local machine.  This structure is also embedded into the
 *	I/O descriptor for remote files.  These stream types share some
 *	common remote procedure stubs, and this structure provides
 *	a common interface.
 *	FS_RMT_DEVICE_STREAM, FS_RMT_PIPE_STREAM, FS_RMT_NAMED_PIPE_STREAM.
 */

typedef struct FsRemoteIOHandle {
    FsHandleHeader	hdr;		/* Standard handle header.  The server
					 * ID field in the hdr is used to
					 * forward the I/O operation. */
    FsRecoveryInfo	recovery;	/* For I/O server recovery */
} FsRemoteIOHandle;			/* 56 BYTES */


/*
 * The current time in seconds and the element used to schedule the update to
 * it.
 */

extern	int			fsTimeInSeconds;
extern	Timer_QueueElement	fsTimeOfDayElement;

/*
 * These record the maximum transfer size supported by the RPC system.
 */
extern int fsMaxRpcDataSize;
extern int fsMaxRpcParamSize;

/*
 * Whether or not to flush the cache at regular intervals.
 */

extern Boolean fsShouldSyncDisks;

/*
 * TRUE once the file system has been initialized, so we
 * know we can sync the disks safely.
 */
extern  Boolean fsInitialized;		

/*
 * The directory that temporary file will live in.
 */
extern	int	fsTmpDirNum;

/*
 * Define the types of files that we care about in the kernel, for such
 * things as statistics gathering, write-through policy, etc.  There is not
 * necessarily a one-to-one mapping between these and the types defined
 * in user/fs.h as FS_USER_TYPE_*; for example, FS_USER_TYPE_BINARY and
 * FS_USER_TYPE_OBJECT were mapped into FS_FILE_TYPE_DERIVED before they
 * were separated into two categories.  It would be possible to flag other
 * derived files (text formatting output, for example) to be in the DERIVED
 * category as well.  
 */
#define FS_FILE_TYPE_TMP 0
#define FS_FILE_TYPE_SWAP 1
#define FS_FILE_TYPE_DERIVED 2
#define FS_FILE_TYPE_BINARY 3
#define FS_FILE_TYPE_OTHER 4

/*
 * Whether or not to keep information about file I/O by user file type.
 */
extern Boolean fsKeepTypeInfo;

/*
 * Fs_StringNCopy
 *
 *	Copy the null terminated string in srcStr to destStr and return the
 *	actual length copied in *strLengthPtr.  At most numBytes will be
 *	copied if the string is not null-terminated.
 */

#define	Fs_StringNCopy(numBytes, srcStr, destStr, strLengthPtr) \
    (Proc_IsMigratedProcess() ? \
	    Proc_StringNCopy(numBytes, srcStr, destStr, strLengthPtr) : \
	    Vm_StringNCopy(numBytes, srcStr, destStr, strLengthPtr))

/*
 *	The FS_NUM_GROUPS constant limits the number of group IDs that
 *	are used even though the proc table supports a variable number.
 */
#define FS_NUM_GROUPS	8

typedef struct FsUserIDs {
    int user;			/* Indicates effective user ID */
    int numGroupIDs;		/* Number of valid entries in groupIDs */
    int group[FS_NUM_GROUPS];	/* The set of groups the user is in */
} FsUserIDs;



/*
 * Wait list routines.  Waiting lists for various conditions are kept
 * hanging of I/O handles.
 */
extern	void		FsWaitListInsert();
extern	void		FsWaitListNotify();
extern	void		FsFastWaitListInsert();
extern	void		FsFastWaitListNotify();
extern	void		FsWaitListDelete();
extern	void		FsWaitListRemove();

/*
 * Name lookup routines.
 */
extern	ReturnStatus	FsLookupOperation();
extern	ReturnStatus	FsTwoNameOperation();

/*
 * Cache size control.
 */
extern	void		FsSetMinSize();
extern	void		FsSetMaxSize();

/*
 * File handle routines.
 */
extern	void 	 	FsHandleInit();
extern	Boolean     	FsHandleInstall();
extern	FsHandleHeader 	*FsHandleFetch();
extern	FsHandleHeader	*FsHandleDup();
extern  FsHandleHeader	*FsGetNextHandle();
extern	void 	 	FsHandleLockHdr();
extern	void	 	FsHandleInvalidate();
extern	Boolean		FsHandleValid();
extern	void		FsHandleIncRefCount();
extern	void		FsHandleDecRefCount();
extern	void 	 	FsHandleUnlockHdr();
extern	void 	 	FsHandleReleaseHdr();
extern	void 	 	FsHandleRemoveHdr();
extern	void 	 	FsHandleAttemptRemove();
extern	void 	 	FsHandleRemoveInt();

/*
 * Macros to handle type casting when dealing with handles.
 */
#define FsHandleFetchType(type, fileIDPtr) \
    (type *)FsHandleFetch(fileIDPtr)

#define FsHandleDupType(type, handlePtr) \
    (type *)FsHandleDup((FsHandleHeader *)handlePtr)

#define FsHandleLock(handlePtr) \
    FsHandleLockHdr((FsHandleHeader *)handlePtr)

#define FsHandleUnlock(handlePtr) \
    FsHandleUnlockHdr((FsHandleHeader *)handlePtr)

#define FsHandleRelease(handlePtr, locked) \
    FsHandleReleaseHdr((FsHandleHeader *)handlePtr, locked)

#define FsHandleRemove(handlePtr) \
    FsHandleRemoveHdr((FsHandleHeader *)handlePtr)

/*
 * Routines for use by the name component hash table.  They increment the
 * low-level reference count on a handle when it is in the cache.
 */
extern	void	 	FsHandleIncRefCount();
extern	void	 	FsHandleDecRefCount();

/*
 * Miscellaneous.
 */
extern	void		FsFileError();
extern	void		FsUpdateTimeOfDay();
extern	void		FsClearStreamID();
extern	void		FsAssignAttrs();
extern  int	 	FsFindFileType();
extern  void	 	FsRecordDeletionStats();

#endif _FSINT
