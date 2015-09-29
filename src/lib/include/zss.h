/*
 * zss.h --
 *
 *	Definitions for the zss library.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.8 92/03/02 15:32:57 bmiller Exp $ SPRITE (Berkeley)
 */

#ifndef _ZSS_LIB
#define _ZSS_LIB


#include <sprite.h>
#include <kernel/zss.h>

/*
 * The following are definitions for the Zss_Cmd system call.
 */

#define ZSS_ATTACH		1	/* Attach a storage system. */
#define ZSS_VERBOSE		2	/* Print verbose messages. */
#define ZSS_MAX_FREE_TABLE_SIZE 3	/* Change the maximum size of the
					 * free table. */
#define ZSS_CHECKPOINT		4	/* Checkpoint the volume. */
#define ZSS_WRITE		5	/* Write a fragment. */
#define ZSS_READ		6	/* Read a fragment. */
#define ZSS_APPEND		7	/* Append to a fragment. */
#define ZSS_SANITY		8	/* Turn on sanity checks. */
#define ZSS_GET_HEADER		9	/* Return the volume header. */
#define ZSS_DETACH		10	/* Detach a volume. */
#define ZSS_DELETE		11	/* Delete a fragment. */
#define ZSS_GET_SUMMARY_TABLE	12	/* Get the summary table. */
#define ZSS_FRAGMENT_SUMMARY	13	/* Get the summary info for a 
					 * fragment. */
#define ZSS_VOLUMES		14	/* Return the ids of attached
					 * volumes. */

/*
 * Argument for ZSS_MAX_FREE_TABLE_SIZE.
 */

typedef struct {
    Zebra_Uid	vid;		/* Volume ID */
    int		maxSize;	/* Maximum size of free table. */
} ZssFreeTableCmd;

/* 
 * Argument for ZSS_CHECKPOINT.
 */

typedef struct {
    Zebra_Uid	vid;		/* Volume ID */
    int		flags;		/* Debugging flags. */
} ZssCheckpointCmd;

/*
 * Argument for ZSS_WRITE. 
 */

typedef struct {
    int			serverID;	/* Server to access. -1 means local. */
    Zebra_Uid		vid;		/* Volume ID to access. */
    Zebra_Sfid		sfid;		/* SFID of fragment. */
    Zebra_Checksum	checksum;	/* Checksum of the data buffer. */
    Boolean		full;		/* TRUE => fragment will not be
					 * appended to. */
    Boolean		dataOnly;	/* Only write the data. */
    Boolean		hdrOnly;	/* Only write the frame header. */
    int			length;		/* Transfer length, in bytes. */
    Address		buffer;		/* Address of the data buffer. */
    int			frame;		/* Frame sfid was in. (out) */
} ZssWriteCmd;


/*
 * Argument for ZSS_READ.
 */

typedef struct {
    int			serverID;	/* Server to access. -1 means local. */
    Zebra_Uid		vid;		/* Volume ID to access. */
    Zebra_Sfid		sfid;		/* SFID of fragment. */
    int			offset;		/* Starting offset. */
    int			length;		/* Transfer length, in bytes. */
    Address		buffer;		/* Address of the data buffer. */
    int			frame;		/* Frame sfid was in. (out) */
}  ZssReadCmd;

/*
 * Argument for ZSS_APPEND. Data buffer follows.
 */

typedef ZssWriteCmd ZssAppendCmd;

/* 
 * Argument for ZSS_SANITY.
 */

typedef struct {
    Zebra_Uid	vid;		/* Volume to use. */
    int		sanity;		/* 1 == on, 0 == off. */
} ZssSanityCmd;

/* 
 * Argument for ZSS_DETACH.
 */

typedef struct {
    Zebra_Uid	vid;		/* Volume to use. */
    Boolean	safe;		/* Do a checkpoint before detaching. */
} ZssDetachCmd;

/*
 * Argument for ZSS_DELETE.
 */

typedef struct {
    int		serverID;	/* Server to access. -1 means local. */
    Zebra_Uid	vid;		/* Volume to use. */
    Zebra_Sfid	sfid;		/* Stripe fragment to delete. */
} ZssDeleteCmd;

/*
 * Argument for ZSS_GET_SUMMARY_TABLE.
 */

typedef struct {
    Zebra_Uid	vid;		/* Volume to use. */
    int		first;		/* Index of first frame to return. */
    int		count;		/* Number of frames to return. */
    int		bufSize;	/* Size of the buffer. */
    Address	buffer;		/* Buffer for summary table. */
} ZssSummaryCmd;

/*
 * Argument for ZSS_FRAGMENT_SUMMARY
 */

typedef struct {
    Zebra_Uid		vid;		/* Volume to use. */
    Zebra_Sfid		sfid;		/* Fragment to get summary for. */
    ZssFrameSummary	summary;	/* Summary info for fragment. */
} ZssFragSummaryCmd;

/*
 * Argument for ZSS_VOLUMES.
 */
typedef struct {
    int		count;				/* # of ids returned. (out) */
    Zebra_Uid	vids[ZSS_MAX_VOLUMES];		/* Volume IDs (out) . */
} ZssVolumesCmd;

/*
 * Library routines.
 */


extern ReturnStatus	Zss_ReadVolumeHeader _ARGS_((int fd, 
				ZssVolumeHeader *headerPtr));
extern ReturnStatus	Zss_WriteVolumeHeader _ARGS_((int fd, 
				ZssVolumeHeader *headerPtr));
extern ReturnStatus	Zss_PrintVolumeHeader _ARGS_((FILE *stream, 
				ZssVolumeHeader *headerPtr));
extern ReturnStatus	Zss_ReadCheckpoint _ARGS_((int fd, 
				ZssVolumeHeader *headerPtr,
				int index, ZssCheckpoint *checkpointPtr));
extern ReturnStatus	Zss_WriteCheckpoint _ARGS_((int fd, 
				ZssVolumeHeader *headerPtr,
				int index, ZssCheckpoint *checkpointPtr));
extern ReturnStatus	Zss_PrintCheckpoint _ARGS_((FILE *stream, 
				ZssCheckpoint *checkpointPtr));
extern ReturnStatus	Zss_ReadSummaryTable _ARGS_((int fd, 
				ZssVolumeHeader *headerPtr, 
				ZssFrameSummary *tablePtr));
extern ReturnStatus	Zss_WriteSummaryTable _ARGS_((int fd, 
				ZssVolumeHeader *headerPtr, 
				ZssFrameSummary *tablePtr));
extern ReturnStatus	Zss_PrintSummaryTable _ARGS_((FILE *stream, 
				ZssFrameSummary *tablePtr));
extern ReturnStatus	Zss_ReadFrameHeader _ARGS_((int fd, 
				ZssVolumeHeader *diskHeaderPtr, int index,
				ZssFrameHeader *frameHeaderPtr));
extern ReturnStatus	Zss_WriteFrameHeader _ARGS_((int fd, 
				ZssVolumeHeader *diskHeaderPtr, int index,
				ZssFrameHeader *frameHeaderPtr));
extern ReturnStatus	Zss_PrintFrameHeader _ARGS_((FILE *stream, 
				ZssFrameHeader *frameHeaderPtr));
extern ReturnStatus	Zss_PrintFrameSubHeader _ARGS_((FILE *stream, 
				ZssFrameSubHeader *frameSubHeaderPtr));
extern ReturnStatus	Zss_ReadFreeTable _ARGS_((int fd, 
				ZssVolumeHeader *headerPtr, int *tablePtr));
extern ReturnStatus	Zss_WriteFreeTable _ARGS_((int fd, 
				ZssVolumeHeader *headerPtr, int *tablePtr));
extern ReturnStatus	Zss_Checksum _ARGS_((int length, Address buffer,
				Zebra_Checksum *checksumPtr));
extern ReturnStatus	Zss_ChecksumCopy _ARGS_((int length, Address inbuffer,
				Address outbuffer, 
				Zebra_Checksum *checksumPtr));
#endif /* _ZSS_LIB */

