/*
 * fsConsist.h --
 *
 *	Declarations for cache consistency routines.
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

#ifndef _FSCONSIST
#define _FSCONSIST

#include "fsClient.h"

/*
 * Flags to determine what type of consistency operation is required.
 *
 *    	FS_WRITE_BACK_BLOCKS	Write back all dirty blocks.
 *    	FS_INVALIDATE_BLOCKS	Invalidate all block in the cache for this
 *				file.  This means that the file is no longer
 *				cacheable.
 *    	FS_DELETE_FILE		Delete the file from the local cache and
 *				the file handle table.
 *    	FS_CANT_READ_CACHE_PIPE	The named pipe is no longer read cacheable
 *				on the client.  This would happen if two
 *				separate clients tried to read the named pipe
 *				at the same time.
 *	FS_WRITE_BACK_ATTRS	Write back the cached attributes.
 *	FS_DEBUG_CONSIST	Forces machine into debugger
 */

#define	FS_WRITE_BACK_BLOCKS		0x01
#define	FS_INVALIDATE_BLOCKS		0x02
#define	FS_DELETE_FILE			0x04
#define	FS_CANT_READ_CACHE_PIPE		0x08
#define	FS_WRITE_BACK_ATTRS		0x10
#define FS_DEBUG_CONSIST		0x100

/*
 * Cache conistency statistics.
 */
typedef struct FsCacheConsistStats {
    int numConsistChecks;	/* The number of times consistency was checked*/
    int numClients;		/* The number of clients considered */
    int notCaching;		/* # of other clients that weren't caching */
    int readCaching;		/* # of other clients that were read caching */
    int writeCaching;		/* # of lastWriters that re-opened  */
    int writeBack;		/* # of lastWriters forced to write-back */
    int readInvalidate;		/* # of readers forced to stop caching */
    int writeInvalidate;	/* # of writers forced to stop caching */
} FsCacheConsistStats;

extern FsCacheConsistStats fsConsistStats;

/*
 * Cache consistency routines.
 */
extern void		FsConsistInit();
extern ReturnStatus	FsFileConsistency();
extern void		FsReopenClient();
extern ReturnStatus	FsReopenConsistency();
extern ReturnStatus	FsMigrateConsistency();
extern void		FsGetClientAttrs();
extern Boolean		FsConsistClose();
extern void		FsDeleteLastWriter();
extern void		FsClientRemoveCallback();
extern void		FsConsistKill();
extern void		FsGetAllDirtyBlocks();
extern void		FsFetchDirtyBlocks();

extern ReturnStatus	Fs_RpcConsist();
extern ReturnStatus	Fs_RpcConsistReply();

#endif _FSCONSIST
