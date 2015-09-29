/*
 * lfsrecov.h --
 *
 *	Declarations of global data structures and routines of the
 *	lfsrecov program.
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
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.7 91/02/09 13:24:52 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _LFSRECOV
#define _LFSRECOV

/* constants */

/* data structures */

/*
 * Recovery is implemented as a two pass operation on the
 * log tail. enum Pass defines which pass is active.
 */
enum Pass { PASS1, PASS2};

/*
 * For a operation from the directory log, the operands 
 * can be in one of three states:
 *	UNKNOWN - The operand is between the START and END record. The
 *		  change may or may not have made it out.
 * 	FORWARD - The operand is after the END record in the log. Change
 *		  did make it out. 
 *	BACKWARD - The operand is before the END record in the log. Change
 *		   didn't make it out.
 */
enum LogStatus { UNKNOWN, FORWARD, BACKWARD};

/*
 * Stats for program.
 */

typedef struct Stats {
    int	   crashTime;	/* Time last checkpoint. */
    int	   timestamp;	/* Timestamp of last checkpoint. */
    struct timeval	startTimeOfDay; /* Start time of program. */
    struct timeval	startPass1TimeOfDay; /* Start time of pass1. */
    struct timeval	startPass2TimeOfDay; /* Start time of pass2. */
    struct timeval	startWriteTimeOfDay; /* Start of writeback. */
    struct timeval	endTimeOfDay; /* End of recovery. */
    int numLogSegments;   /* Number of segments in log. */
    int	segSummaryFetch;  /* Number of segment summary blocks fetched per
			   * pass. */
    int	segUsageBlocks; /* Number of segment usage blocks in log. */
    int	descMapBlocks;  /* Number of desc map blocks in log. */
    int descBlocks;	/* Number of desc blocks in log. */
    int numDesc;	/* Number of desc in log. */
    int numFileBlocks;	/* Number of file blocks. */
    int numDirLogBlocks; /* Number of directory log blocks. */
    int numDirLogEntries; /* Number of directory log entries. */
    int	numDirLogDelete;  /* Number delete log records. */
    int numDirLogDeleteOpen; /* Number delete while open log records. */
    int numDirLogUnlink; /* Number unlink log records. */
    int	numDirLogCreate; /* Number create log records. */
    int numDirLogWithoutEnd; /* Number of directory log entries without end. */
    int	dirLogBothForward; /* Both dir and inode forward. */
    int dirLogDirBlockBackward; /* Dir block backward. */
    int dirLogBothBackwardCreate; /* Both backward on a create. */
    int dirLogBothBackward; /* Both backward on non-create. */
    int descDelete;	    /* Desc deleted. */
    int descMove;	    /* Desc moved. */
    int filesToLostFound;   /* Files added to lost+found. */
    int dirEntryAdded;	    /* Directory entry added. */
    int dirEntryRemoved;    /* Removed entry removed. */
    int loadBlockReads;	    /* Disk block reads during load. */
    int pass1BlockReads;    /* Disk block reads during pass1. */
    int pass2BlockReads;    /* Disk block reads during pass2. */
} Stats;

extern Stats stats;

/*
 * Start and end points of the tail of the recovery log.
 */
extern LogPoint	logStart;	/* Start of recovery log. */
extern LogPoint	logEnd;		/* End of recovery log. */

/*
 * Arguments.
 */
extern int	blockSize;	/* File system block size. */
extern Boolean	verboseFlag;	/* Trace progress of program. */
extern Boolean	showLog ;	/* Show contents of log being processed. */
extern char	*deviceName;		/* Device to use. */
extern Boolean recreateDirEntries; 
extern Lfs	*lfsPtr;

/* procedures */

#endif /* _LFSRECOV */

