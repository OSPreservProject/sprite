/*
 * fsioPipe.h --
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

#include "fsio.h"
/*
 * The I/O descriptor for a local anonymous pipe: FSIO_LCL_PIPE_STREAM.
 */

typedef struct Fsio_PipeIOHandle {
    Fs_HandleHeader	hdr;		/* Standard handle header. The 'major'
					 * field of the fileID is a generation
					 * number. 'minor' field is unused. */
    List_Links		clientList;	/* Client use info needed to allow
					 * remote access after migration. */
    Fsutil_UseCounts		use;		/* Summary reference counts. */
    int			flags;		/* FSIO_PIPE_READER_GONE, FSIO_PIPE_WRITER_GONE */
    int			firstByte;	/* Indexes into buffer. */
    int			lastByte;
    int			bufSize;	/* Total number of bytes in buffer */
    Address		buffer;		/* The buffer for the data. */
    List_Links		readWaitList;	/* For the waiting readers of the pipe*/
    List_Links		writeWaitList;	/* For the waiting writers on the pipe*/
} Fsio_PipeIOHandle;			/* 68 BYTES */

#define FSIO_PIPE_READER_GONE	0x1
#define FSIO_PIPE_WRITER_GONE	0x2

/*
 * When a client re-opens a pipe it sends the following state to the server.
 */
typedef struct Fsio_PipeReopenParams {
    Fs_FileID	fileID;		/* File ID of pipe to reopen. MUST BE FIRST */
    Fsutil_UseCounts use;		/* Recovery use counts */
} Fsio_PipeReopenParams;

/*
 * Stream operations.
 */
extern ReturnStatus Fsio_PipeRead();
extern ReturnStatus Fsio_PipeWrite();
extern ReturnStatus Fsio_PipeIOControl();
extern ReturnStatus Fsio_PipeSelect();
extern ReturnStatus Fsio_PipeGetIOAttr();
extern ReturnStatus Fsio_PipeSetIOAttr();
extern ReturnStatus Fsio_PipeMigClose();
extern ReturnStatus Fsio_PipeMigOpen();
extern ReturnStatus Fsio_PipeMigrate();
extern ReturnStatus Fsio_PipeReopen();
extern Boolean	    Fsio_PipeScavenge();
extern void	    Fsio_PipeClientKill();
extern ReturnStatus Fsio_PipeClose();

#endif _FSPIPE
