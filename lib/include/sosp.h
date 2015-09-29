/*
 * sospTrace.h --
 *
 *	Declarations of the data structures and constants necessary to
 *	post-process the SOSP 91 file system traces.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/sosp/RCS/sosp.h,v 1.2 92/06/21 17:51:19 jhh Exp Locker: mgbaker $ SPRITE (Berkeley)
 */

#ifndef _SOSPTRACE
#define _SOSPTRACE

/*
 * Relevant portions of the file ID.
 */
typedef struct {
    int		host;
    int		domain;
    int		inode;
} ID;

/*
 * File ID specified with relevant portions.
 */
typedef struct {
    int		notused;
    ID		id;
} FileID;

/*
 * Beginning of each trace record.
 */
typedef struct	RecordHeader {
    int		flags;		/* See below. */
    int		time[2];	/* (Timer_Ticks) Timestamp. */
    int		type;		/* Type of record that follows. */
} RecordHeader;

/*
 * Masks for the flags field in the RecordHeader. 
 */

#define TRACELOG_BYTEMASK 0x0000ffff	/* Use this to get the size of the
					 * record. */

/*
 * Definitions for the types of log records.  These are the events being
 * traced.
 */
#define SOSP_INVALID            0
#define SOSP_OPEN               1
#define SOSP_DELETE             2
#define SOSP_CREATE             3
#define SOSP_MKLINK             4
#define SOSP_SET_ATTR           5
#define SOSP_GET_ATTR           6
#define SOSP_LSEEK              7
#define SOSP_CLOSE              8
#define SOSP_MIGRATE            9
#define SOSP_TRUNCATE           10
#define SOSP_CONSIST_CHANGE     11
#define SOSP_READ               12
#define SOSP_LOOKUP             13
#define SOSP_CONSIST_ACTION     14
#define SOSP_PREFIX             15
#define SOSP_LOOKUP_OP          16
#define SOSP_DELETE_DESC        17

#define SOSP_NUM_EVENTS         18

#define	SOSP_LOST_TYPE		128

/*
 * Open trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				currentHostID;	/* Host doing open. */
    int				homeHostID;	/* Home host if migrated. */
    FileID			fid;		/* File ID being opened. */
    FileID			sid;		/* Stream ID being returned. */
    int				effID;		/* Effective user ID. */
    int				realID;		/* Real user ID. */
    int				mode;		/* Mode the to open the file. */
    int				numNowReading;	/* Number of clients with file
						 * open for reading after
						 * the open. */
    int				numNowWriting;	/* Number of clients with file
						 * open for writing after
						 * the open. */
    int				create;		/* Create time of file. */
    int				size;		/* File size in bytes at open */
    int				modify;		/* Modify time of file. */
    int				type;		/* FS_FILE, FS_DIRECTORY, etc.
						 * See /usr/include/fs.h. */
    int				cacheable;	/* File is cacheable or not. */
} SospOpen;

/*
 * Create trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				currentHostID;	/* Host doing create. */
    int				homeHostID;	/* Home host if migrated. */
    FileID			fid;		/* File ID created. */
} SospCreate;

/*
 * MkLink trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				currentHostID;	/* Host doing mklink. */
    int				homeHostID;	/* Home host if migrated. */
    FileID			fid;		/* File ID of link. */
} SospMkLink;

/*
 * SetAttr trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				currentHostID;	/* Host doing setattr. */
    int				homeHostID;	/* Home host if migrated. */
    FileID			fid;		/* File being set attr'd. */
    int				userID;		/* User doing setattr. */
} SospSetAttr;

/*
 * GetAttr trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				currentHostID;	/* Host doing getattr. */
    int				homeHostID;	/* Home host if migrated. */
    FileID			fid;		/* File being get attr'd. */
    int				userID;		/* User doing getattr. */
} SospGetAttr;



/*
 * Lseek trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    FileID			sid;		/* Stream ID. */
    int				start;		/* Starting offset. */
    int				end;		/* Ending offset. */
    int				rwflags;	/* R/W flags from stream. */
} SospLseek;

/*
 * Close trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    FileID			sid;		/* Stream ID. */
    int				offset;		/* Offset at close time. */
    int				size;		/* Size at close time. */
    int				flags;		/* Flags for stream. */
    int				rwflags;	/* R/W flags for file. */
    int				refCount;	/* Current refs to file. */
    int				consist;	/* Did consistency on close. */
} SospClose;

/*
 * Truncation trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    FileID			sid;		/* Stream ID. */
    int				oldLength;	/* Length before trunc. */
    int				newLength;	/* Length after trunc. */
    int				modify;		/* Modify time of file. */
    int				create;		/* Create time of file. */
} SospTrunc;

/*
 * Delete trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				currentHostID;	/* Host doing open. */
    int				homeHostID;	/* Home host if migrated. */
    FileID			fid;		/* File ID. */
    int				modify;		/* Modify time of file. */
    int				create;		/* Create time of file. */
    int				size;		/* Size of file to delete. */
} SospDelete;

/*
 * Consist trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				hostID;		/* Host causing consistency
						 * change. */
    FileID			fid;		/* File ID causing consist. */
    int				op;		/* Operation causing the
						 * consist change: SOSP_OPEN,
						 * etc. */
    int				writeOp;	/* Whether operation is for
						 * writing (TRUE) else FALSE. */
} SospConsist;

/*
 * Consistency action trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int			causingHostID;		/* Host causing consistency
						 * change. */
    int			affectedHostID;		/* Host affected by consistency
						 * change. */
    FileID			fid;		/* File ID causing consist. */
    int				action;		/* Whether the action to take
						 * on the client is
						 * FSCONSIST_WRITE_BACK_BLOCKS
						 * and/or
						 * FSCONSIST_INVALIDATE_BLOCKS,
						 * etc. */
} SospConsistAction;

/*
 * Read/write trace event on a concurrently write-shared file.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				hostID;		/* Host doing read/write. */
    FileID			fid;		/* File ID being read. */
    FileID			sid;		/* Stream ID being read. */
    int				readIt;		/* Whether it's a read (TRUE)
						 * or write (FALSE). */
    int				offset;		/* File offset to start
						 * operation. */
    int				numBytes;	/* Size in bytes to read or
						 * write. */
    int				numNowReading;	/* Number of clients with
						 * file open for reading. */
    int				numNowWriting;	/* Number of clients with
						 * file open for writing. */
} SospRead;

/*
 * Prefix operation trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				hostID;		/* ID of host. */
    int				rpcID;		/* RPC sequence number. */
} SospAddPrefix;


/*
 * Migration trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				fromHost;	/* Source host of migration. */
    int				toHost;		/* Dest. host of migration. */
    FileID			sid;		/* Stream ID migrated. */
    int				offset;		/* Current offset of stream. */
} SospMigrate;

/*
 * Lookup trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				currentHostID;	/* Host doing lookup. */
    int				homeHostID;	/* Home host if migrated. */
    FileID			result;		/* FileID result. */
    int				status;		/* Status of lookup:
						 * See FslclLookup. */
    int				size;		/* Number of fileIDs in
						 * buffer space following. */
    int				op;		/* Operation causing lookup:
						 * FS_DOMAIN_OPEN, etc.
						 * See kernel fsNameOps.h. */
    /*
     * Note: the file IDs follow as if in an array immediately after this
     * header.  The size field gives the number of the file IDs.
     */
} SospLookup;

/*
 * Delete descriptor trace event.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    FileID			fid;		/* FileID deleted. */
    int				modify;		/* Modify time for file. */
    int				create;		/* Create time for file. */
    int				size;		/* Size of the file. */
} SospDeleteDesc;

/*
 * To record number of lost records in the trace.
 */
typedef struct {
    RecordHeader		rec;		/* Record info. */
    int				count;		/* Number of records lost. */
} SospLost;



/***************************************************************************/

/*
 *  Structure definition for the trace library routines.
 */

typedef struct traceFile {
    FILE *stream;		/* Stream to read from. */
    int traceRecCount;		/* Counter of records read. */
    int numRecs;		/* Counter of number of records. */
    int version;		/* File type version. */
				/* 0 = old, 1 = new. */
    int swap;			/* 0 = don't, 1 = do. */
    char *name;			/* file name. */
} traceFile;

/*
 * Routines provided by the trace library.
 */

int initRead _ARGS_((char *name, int argc, char **argv,
	Sys_TracelogHeader **retHdr));
int getHeader _ARGS_((traceFile *file, Sys_TracelogHeader *hdr));
int getNextRecord _ARGS_((traceFile *file, char *buf));
int getNextRecordMerge _ARGS_((char **buf));
Boolean migrateChildren _ARGS_((Boolean flags));

/***************************************************************************/

/*
 * Definitions for sample post-processor.
 */

#define RUN_BUCKET	100
#define OPEN_BUCKET	100
#define SIZE_BUCKET	100
#define AGE_BUCKET	1
#define TPUT_BUCKET	100


#define WHOLE_FILE 0
#define SEQUENTIAL 1
#define RANDOM 2

#define LOW_LIMIT 1000000000

#define ID_CMP(a,b) 			\
    (!((a.inode == b.inode) &&		\
       (a.domain == b.domain) &&	\
       (a.host == b.host)))



typedef struct {
    int		rwflags;
    int		mig;
    int		histIndex;
    int		user;
} RunHistKey;

typedef struct {
    int		mig;
    int		index;
} OpenKey;

typedef struct {
    int		state;
    int		usage;
    int		mig;
    int		user;
    int		hour;
} XferKey;

typedef struct {
    int		rwflags;
    int		mig;
    int		type;
} TotalBytesKey;

typedef struct {
    int		intLength;
    int		index;
    int		user;
} TputKey;

typedef struct {
    int		intLength;
    int		index;
    int		user;
} UserKey;

typedef struct {
    int			count;
    unsigned int	bytes[2];
} SizeHist, AgeHist, XferHist, RunHist;

typedef struct {
    Boolean	mig;
    int		bytes;
} TputHist;

typedef struct {
    int		bytes[2];
} TotalBytesInfo;

#define ALLOC_VALUE(type, entryPtr) { 		\
    char	*__ptr;				\
    __ptr = (char *) malloc(sizeof(type));	\
    bzero((char *) __ptr, sizeof(type));	\
    Hash_SetValue(entryPtr, (Address) __ptr);	\
}

#define ADD_TO_VALUE(entryPtr, amount) \
    (Hash_SetValue(entryPtr, (amount) + (unsigned int) Hash_GetValue(entryPtr)))

#define ADD_BIG(array, amount) {	\
    array[1] += (amount);		\
    if (array[1] > LOW_LIMIT) {		\
	array[0]++;			\
	array[1] -= LOW_LIMIT;		\
    }					\
}

#define IDSTR(id, str)	\
    sprintf(str, "%x.%x.%x", id.host, id.domain, id.inode)

#endif /* _SOSPTRACE */
