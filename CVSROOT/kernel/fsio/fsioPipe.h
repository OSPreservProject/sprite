/*
 * fsPipe.h --
 *
 *	Declarations for anonymous pipe access.
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

#ifndef _FSPIPE
#define _FSPIPE

/*
 * The I/O descriptor for a local anonymous pipe: FS_LCL_PIPE_STREAM.
 */

typedef struct FsPipeIOHandle {
    FsHandleHeader	hdr;		/* Standard handle header. The 'major'
					 * field of the fileID is a generation
					 * number. 'minor' field is unused. */
    List_Links		clientList;	/* Client use info needed to allow
					 * remote access after migration. */
    FsUseCounts		use;		/* Summary reference counts. */
    int			flags;		/* PIPE_READER_GONE, PIPE_WRITER_GONE */
    int			firstByte;	/* Indexes into buffer. */
    int			lastByte;
    int			bufSize;	/* Total number of bytes in buffer */
    Address		buffer;		/* The buffer for the data. */
    List_Links		readWaitList;	/* For the waiting readers of the pipe*/
    List_Links		writeWaitList;	/* For the waiting writers on the pipe*/
} FsPipeIOHandle;			/* 68 BYTES */

#define PIPE_READER_GONE	0x1
#define PIPE_WRITER_GONE	0x2

/*
 * When a client re-opens a pipe it sends the following state to the server.
 */
typedef struct FsPipeReopenParams {
    Fs_FileID	fileID;		/* File ID of pipe to reopen. MUST BE FIRST */
    FsUseCounts use;		/* Recovery use counts */
} FsPipeReopenParams;

/*
 * Stream operations.
 */
extern ReturnStatus FsPipeRead();
extern ReturnStatus FsPipeWrite();
extern ReturnStatus FsPipeIOControl();
extern ReturnStatus FsPipeSelect();
extern ReturnStatus FsPipeGetIOAttr();
extern ReturnStatus FsPipeSetIOAttr();
extern ReturnStatus FsPipeRelease();
extern ReturnStatus FsPipeMigEnd();
extern ReturnStatus FsPipeMigrate();
extern ReturnStatus FsPipeReopen();
extern void	    FsPipeScavenge();
extern void	    FsPipeClientKill();
extern ReturnStatus FsPipeClose();

extern FsHandleHeader *FsRmtPipeVerify();
extern ReturnStatus FsRmtPipeMigrate();
extern ReturnStatus FsRmtPipeReopen();
extern ReturnStatus FsRmtPipeClose();

#endif _FSPIPE
