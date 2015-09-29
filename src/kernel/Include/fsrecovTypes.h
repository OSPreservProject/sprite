/*
 * fsrecovTypes.h --
 *
 *	Declarations of file handle recovery from recovery box.
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
 * $Header: /sprite/src/kernel/Cvsroot/kernel/fsrecov/fsrecovTypes.h,v 1.1 92/08/10 17:32:58 mgbaker Exp $ SPRITE (Berkeley)
 */

#ifndef _FSRECOV_TYPES
#define _FSRECOV_TYPES


/* constants */

/* data structures */
/*
 * The state in this structure corresponds mostly to the Fsio_FileReopenParams
 * that was passed from clients to server during recovery.  It includes
 * the stuff in the other types of reopen params as well (such as
 * FsRmtDeviceReopenParams and StreamReopenParams).  If we keep this
 * current, we should be able to recover without talking to the clients.
 * The reopen params also included prefixID and flags fields that I
 * don't think are necessary when doing it this way.
 */
typedef	struct	Fsrecov_HandleState {
    Fs_FileID			fileID;		/* File or stream ID. */
    Fs_FileID			otherID;	/* If the object is a stream,
						 * then this is the id it
						 * points to. */
    Fsio_UseCounts		use;		/* How many references this
						 * client has to the file. */
    int				info;		/* For files: file version.
						 * For pdevs: serverID.
					         * For streams: all useFlags.
						 * Otherwise unused. */
    int				clientData;	/* For files: whether client
   						 * can cache or not.
						 * For pdevs: seed.
						 * For streams: offset.
						 * Otherwise unused. */
} Fsrecov_HandleState;

extern	Boolean	fsrecov_AlreadyInit;	/* We had a fast reboot. */
extern	int	fsrecov_DebugLevel;	/* Print debugging info. */
extern	Boolean	fsrecov_FromBox;	/* Do recovery from what's in the
					 * recov box, even for old clients
					 * that are doing reopens. */

/*
 * Stuff for testing and timing recovery and the recovery box operations.
 */
extern	Time	fsrecov_StartRebuild;
extern	Time	fsrecov_ReturnedObjs;
extern	Time	fsrecov_BuiltHashTable;
extern	Time	fsrecov_BuiltIOHandles;
extern	Time	fsrecov_EndRebuild;
extern	int	fsrecov_RebuildNumObjs;

typedef struct Fsrecov_DirLog {
    int		nextLogSeqNum;
    int		oldestEntry;
} Fsrecov_DirLog;

typedef struct	DirEntryMap {
    int		objectNum;
    int		timeStamp;
} DirEntryMap;


#endif /* _FSRECOV_TYPES */
