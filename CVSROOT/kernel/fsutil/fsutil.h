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


#include "fs.h"
#include "fsio.h"

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
    Fsutil_UseCounts		use;		/* Client's copy of use state */
} Fsutil_RecoveryInfo;



/*
 * The current time in seconds and the element used to schedule the update to
 * it.
 */

extern	int			fsutil_TimeInSeconds;
extern	Timer_QueueElement	fsutil_TimeOfDayElement;

extern Boolean fsconsist_Debug;
/*
 * Whether or not to flush the cache at regular intervals.
 */

extern Boolean fsutil_ShouldSyncDisks;


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

#define mnew(type)	(type *)malloc(sizeof(type))

extern void		Fsutil_RecoveryInit();
extern void		Fsutil_RecoverySyncLockCleanup();
extern void		Fsutil_WantRecovery();
extern void		Fsutil_AttemptRecovery();
extern ReturnStatus	Fsutil_WaitForRecovery();
extern void		Fsutil_Reopen();
extern Boolean		Fsutil_RecoveryNeeded();
extern Boolean		Fsutil_RemoteHandleScavenge();
extern void		Fsutil_ClientCrashed();
extern void		Fsutil_RemoveClient();


/*
 * Wait list routines.  Waiting lists for various conditions are kept
 * hanging of I/O handles.
 */
extern	void		Fsutil_WaitListInsert();
extern	void		Fsutil_WaitListNotify();
extern	void		Fsutil_FastWaitListInsert();
extern	void		Fsutil_FastWaitListNotify();
extern	void		Fsutil_WaitListDelete();
extern	void		Fsutil_WaitListRemove();

/*
 * File handle routines.
 */
extern	void 	 	Fsutil_HandleInit();
extern	Boolean     	Fsutil_HandleInstall();
extern	Fs_HandleHeader 	*Fsutil_HandleFetch();
extern	Fs_HandleHeader	*Fsutil_HandleDup();
extern  Fs_HandleHeader	*Fsutil_GetNextHandle();
extern	void 	 	Fsutil_HandleLockHdr();
extern	void	 	Fsutil_HandleInvalidate();
extern	Boolean		Fsutil_HandleValid();
extern	void		Fsutil_HandleIncRefCount();
extern	void		Fsutil_HandleDecRefCount();
extern	Boolean	 	Fsutil_HandleUnlockHdr();
extern	void 	 	Fsutil_HandleReleaseHdr();
extern	void 	 	Fsutil_HandleRemoveHdr();
extern	Boolean	 	Fsutil_HandleAttemptRemove();
extern	void 	 	Fsutil_HandleRemoveInt();
/*
 * Miscellaneous.
 */
extern	void		Fsutil_FileError();
extern	void		Fsutil_PrintStatus();
extern	void		Fsutil_UpdateTimeOfDay();
extern	void		Fs_ClearStreamID();
extern  int	 	Fsdm_FindFileType();
extern	char *		Fsutil_FileTypeToString();

extern ReturnStatus	Fsutil_DomainInfo();

extern ReturnStatus Fsutil_HandleDescWriteBack();
extern	void	Fsutil_SyncProc();
extern	void	Fsutil_Sync();
extern void Fsutil_SyncStub();
extern ReturnStatus Fsutil_WaitForHost();
extern int Fsutil_TraceInit();
extern ReturnStatus Fsutil_RpcRecovery();
extern int Fsutil_PrintTraceRecord();
extern void Fsutil_PrintTrace();


extern	void		Fsutil_HandleScavengeStub();
extern	void		Fsutil_HandleScavenge();

extern  char		*Fsutil_GetFileName();

#endif /* _FSUTIL */
