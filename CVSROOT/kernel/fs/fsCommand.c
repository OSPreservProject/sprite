/* 
 * fsCommand.c --
 *
 *	The guts of the Fs_Command system call.  This is used to
 *	set/get various filesystem parameters.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "fs.h"
#include "fsInt.h"
#include "fsOpTable.h"
#include "fsPrefix.h"
#include "fsTrace.h"
#include "fsMigrate.h"
#include "fsNameHash.h"
#include "fsBlockCache.h"
#include "fsPdev.h"
#include "fsDebug.h"
#include "fsStat.h"
#include "mem.h"
#include "byte.h"
#include "timer.h"
#include "user/fsCmd.h"
#include "rpc.h"
#include "sched.h"


#define SWAP_TO_BUFFER(int1, buffer) \
    if ((int *)buffer != (int *)NIL && (int *)buffer != (int *)0) {	\
	register int tmp;						\
	tmp = int1 ; int1 = *(int *)buffer ; *(int *)buffer = tmp;	\
    }

/*
 *----------------------------------------------------------------------
 *
 * Fs_Command --
 *
 *	Hook into the fs module.  System parameters can be adjusted,
 *	the prefix table modified, and filesystem stats can be returned.
 *
 * Results:
 *	0 or an error code from any of the operations.
 *
 * Side effects:
 *	See description by each command.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
Fs_Command(command, bufSize, buffer)
    int command;
    int bufSize;
    Address buffer;
{
    ReturnStatus 	status = SUCCESS;
    extern	int	fsMaxBlockCleaners;
    extern	int	fsReadAheadBlocks;
    extern	Boolean	fsClientCaching;

    switch(command) {
	case FS_PREFIX_LOAD: {
	    /*
	     * Load the prefix (contained in buffer) into the prefix table.
	     * This will cause a broadcast to find the
	     * server the first time the prefix is matched.
	     */
	    if (buffer[0] != '/') {
		status = FS_INVALID_ARG;
	    } else {
		Fs_PrefixLoad(buffer, FS_IMPORTED_PREFIX);
		status = SUCCESS;
	    }
	    break;
	}
	case FS_PREFIX_EXPORT: {
	    /*
	     * Export a local directory under a prefix.
	     */
	    Fs_TwoPaths *argPtr = (Fs_TwoPaths *)buffer;
	    char *localPath, *prefix;
	    Fs_Stream *streamPtr;

	    localPath = (char *)Mem_Alloc(argPtr->pathLen1);
	    prefix = (char *)Mem_Alloc(argPtr->pathLen2);
	    status = Vm_CopyIn(argPtr->pathLen1, argPtr->path1, localPath);
	    if (status == SUCCESS) {
		status = Vm_CopyIn(argPtr->pathLen2, argPtr->path2, prefix);
		if (status == SUCCESS) {
		    status = Fs_Open(localPath, FS_READ, FS_DIRECTORY, 0,
						&streamPtr);
		    if (status == SUCCESS) {
			if (streamPtr->ioHandlePtr->fileID.type !=
				FS_LCL_FILE_STREAM) {
			    Sys_Panic(SYS_WARNING,
		    "Tried to export non-local file \"%s\" as prefix \"%s\"\n",
				localPath, prefix);
			    (void)Fs_Close(streamPtr);
			    status = FS_NO_ACCESS;
			} else {
			    (void)FsPrefixInstall(prefix,streamPtr->ioHandlePtr,
						    FS_LOCAL_DOMAIN,
		    FS_EXPORTED_PREFIX|FS_IMPORTED_PREFIX|FS_OVERRIDE_PREFIX);
			}
		    }
		}
	    }
	    Mem_Free(prefix);
	    Mem_Free(localPath);
	    break;
	}
	case FS_PREFIX_CLEAR: {
	    /*
	     * Clear the handle information about a prefix.
	     */
	    status = Fs_PrefixClear(buffer, FALSE);
	    break;
	}
	case FS_PREFIX_DELETE: {
	    /*
	     * Remote a prefix table entry all-together.
	     */
	    status = Fs_PrefixClear(buffer, TRUE);
	    break;
	}
	case FS_PREFIX_CONTROL: {
	    /*
	     * Modify the export list associated with a prefix.
	     */
	    register Fs_PrefixControl *controlPtr;
	    controlPtr = (Fs_PrefixControl *)buffer;
	    if (bufSize < sizeof(Fs_PrefixControl)) {
		status = GEN_INVALID_ARG;
	    } else {
		Fs_PrefixExport(controlPtr->prefix, controlPtr->clientID,
				controlPtr->delete);
		status = SUCCESS;
	    }
	    break;
	}
	case FS_RAISE_MIN_CACHE_SIZE: {
	    /*
	     * Make the minimum size of the file system block cache larger.
	     */
	    if (buffer != (Address)NIL && buffer != (Address)0) {
		FsSetMinSize(*(int *) buffer);
	    }
	    break;
	}
	case FS_LOWER_MAX_CACHE_SIZE: {
	    /*
	     * Make the minimum size of the file system block cache larger.
	     */
	    if (buffer != (Address)NIL && buffer != (Address)0) {
		FsSetMaxSize(*(int *) buffer);
	    }
	    break;
	}
	case FS_DISABLE_FLUSH: {
	    /*
	     * Turn on or off automatic flushing of the cache.
	     */
	    SWAP_TO_BUFFER(fsShouldSyncDisks, buffer);
	    break;
	}
	/*
	 * The following cases are used to set flags and to
	 * return their old values.
	 */
	case FS_SET_TRACING: {
	    /*
	     * Set the file system tracing flag.
	     */
	    SWAP_TO_BUFFER(fsTracing, buffer);
	    break;
	}
	case FS_SET_CACHE_DEBUG: {
	    /*
	     * Set the cache debug flag.
	     */
	    SWAP_TO_BUFFER(fsCacheDebug, buffer);
	    break;
	}
	case FS_SET_MIG_DEBUG: {
	    /*
	     * Set the migration debug flag.
	     */
	    SWAP_TO_BUFFER(fsMigDebug, buffer);
	    break;
	}
	case FS_SET_PDEV_DEBUG: {
	    /*
	     * Set the pseudo-device debug flag.
	     */
	    SWAP_TO_BUFFER(fsPdevDebug, buffer);
	    break;
	}
	case FS_SET_RPC_DEBUG: {
	    /*
	     * Set the rpc debug flag.
	     */
	    SWAP_TO_BUFFER(fsRpcDebug, buffer);
	    break;
	}
	case FS_SET_RPC_TRACING: {
	    /*
	     * Set the rpc tracing flag.
	     */
	    SWAP_TO_BUFFER(rpc_Tracing, buffer);
	    break;
	}
	case FS_SET_RPC_NO_TIMEOUTS: {
	    /*
	     * Set the rpc "no timeouts" flag, useful when debugging.
	     */
	    SWAP_TO_BUFFER(rpc_NoTimeouts, buffer);
	    break;
	}
	case FS_SET_NAME_CACHING: {
	    /*
	     * Set the rpc tracing flag.
	     */
	    SWAP_TO_BUFFER(fsNameCaching, buffer);
	    break;
	}
	case FS_SET_CLIENT_CACHING: {
	    /*
	     * Set the rpc tracing flag.
	     */
	    SWAP_TO_BUFFER(fsClientCaching, buffer);
	    break;
	}
	case FS_SET_RPC_CLIENT_HIST: {
	    extern int rpcCallTiming;
	    SWAP_TO_BUFFER(rpcCallTiming, buffer);
	    break;
	}
	case FS_SET_RPC_SERVER_HIST: {
	    extern int rpcServiceTiming;
	    SWAP_TO_BUFFER(rpcServiceTiming, buffer);
	    break;
	}
	case FS_SET_NO_STICKY_SEGS: {
	    extern Boolean vm_NoStickySegments;
	    SWAP_TO_BUFFER(vm_NoStickySegments, buffer);
	    break;
	}
	case FS_TEST_CS: {
	    register	int	i;
	    Timer_Ticks	startTicks, endTicks, diffTicks;
	    Time	time;
	    int		us;

	    Timer_GetCurrentTicks(&startTicks);
	    for (i = *(int *) buffer; i > 0; i--) {
		Sched_ContextSwitch(PROC_READY);
	    }
	    Timer_GetCurrentTicks(&endTicks);
	    Timer_SubtractTicks(endTicks, startTicks, &diffTicks);
	    Timer_TicksToTime(diffTicks, &time);
	    us = (time.seconds * 1000000) + time.microseconds;
	    Sys_Printf("microseconds = %d per CS = %d\n", us,
		       us / *(int *)buffer);
	    break;
	}
	case FS_EMPTY_CACHE: {
	    int *numLockedBlocksPtr = (int *)buffer;

	    Fs_CacheEmpty(numLockedBlocksPtr);
	    break;
	}
	case FS_SET_WRITE_THROUGH: {
	    /*
	     * Set the file system write-through flag.
	     */
	    SWAP_TO_BUFFER(fsWriteThrough, buffer);
	    break;
	}
	case FS_SET_WRITE_BACK_ON_CLOSE: {
	    /*
	     * Set the file system write-back-on-close flag.
	     */
	    SWAP_TO_BUFFER(fsWriteBackOnClose, buffer);
	    break;
	}
	case FS_SET_DELAY_TMP_FILES: {
	    /*
	     * Set the flag that delays writes on temporary files.
	     */
	    SWAP_TO_BUFFER(fsDelayTmpFiles, buffer);
	    break;
	}
	case FS_SET_TMP_DIR_NUM: {
	    /*
	     * Set the directory that contains /tmp.
	     */
	    SWAP_TO_BUFFER(fsTmpDirNum, buffer);
	    break;
	}
	case FS_SET_WRITE_BACK_ASAP: {
	    /*
	     * Set the file system write-back as soon as possible flag.
	     */
	    SWAP_TO_BUFFER(fsWriteBackASAP, buffer);
	    break;
	}
	case FS_SET_WB_ON_LAST_DIRTY_BLOCK: {
	    /*
	     * Set the file system write-back as soon as possible flag.
	     */
	    SWAP_TO_BUFFER(fsWBOnLastDirtyBlock, buffer);
	    break;
	}
	case FS_ZERO_STATS: {
	    /*
	     * Zero out the counters in the fsStats struct.  Unfortunately,
	     * some values in the structure can't be zeroed out, so this
	     * must be changed to zero out only some portions.
	     */
	    Byte_Zero(sizeof(FsStats), (Address) &fsStats);
	    status = SUCCESS;
	    break;
	}
	case FS_RETURN_STATS: {
	    if (bufSize >= sizeof(FsStats)) {
		Byte_Copy(sizeof(FsStats), (Address) &fsStats, buffer);
		status = SUCCESS;
	    } else {
		status = FS_INVALID_ARG;
	    }
	    break;
	}
	case FS_GET_FRAG_INFO: {
	    int	*arrPtr = (int *)buffer;

	    Fs_CheckFragmentation(arrPtr, arrPtr + 1, arrPtr + 2);
	    break;
	}
	case FS_SET_CLEANER_PROCS:
	    SWAP_TO_BUFFER(fsMaxBlockCleaners, buffer);
	    break;
	case FS_SET_READ_AHEAD:
	    SWAP_TO_BUFFER(fsReadAheadBlocks, buffer);
	    break;
	case FS_SET_RA_TRACING:
	    SWAP_TO_BUFFER(fsRATracing, buffer);
	    break;
	default:
	    status = FS_INVALID_ARG;
    }
    return(status);
}

#ifdef notdef
/*
 *----------------------------------------------------------------------
 *
 * Fs_Cat --
 *
 *	Cat a file to the screen.  The named file is opened, then
 *	a series of reads are done and the returned data is printed
 *	on the screen.  (Used when testing simple kernels.)
 *
 * Results:
 *	0 or an error code from any of the file operations.
 *
 * Side effects:
 *	Does an open, reads and write, and a close.
 *
 *----------------------------------------------------------------------
 */
int
Fs_Cat(fileName)
    char *fileName;
{
    int error;
    Fs_Stream *streamPtr;
    int offset;
    Address buffer;

    streamPtr = (Fs_Stream *)NIL;
    error = Fs_Open(fileName, FS_READ, FS_FILE, 0, &streamPtr);
    if (error) {
	return(error);
    }

#define CAT_BUFSIZE	80

    buffer = Mem_Alloc(CAT_BUFSIZE);
    offset = 0;
    while (1) {
	int savedLen, len;

	Byte_Zero(CAT_BUFSIZE, buffer);

	savedLen = len = CAT_BUFSIZE;
	error = Fs_Read(streamPtr, buffer, offset, &len);
	if (error || len < savedLen) {
	    break;
	} else {
	    offset += len;
	}
	Sys_Printf("%s", buffer);
    }
    (void)Fs_Close(streamPtr);
    Mem_Free(buffer);
    return(error);
}
#endif /* notdef */

#ifdef notdef
/*
 *----------------------------------------------------------------------
 *
 * Fs_Copy --
 *
 *	Copy a file. (Used when testing simple kernels.)
 *
 * Results:
 *	0 or an error code from any of the file operations.
 *
 * Side effects:
 *	Creates a copy of the first file in the second.
 *
 *----------------------------------------------------------------------
 */
int
Fs_Copy(srcFileName, dstFileName)
    char *srcFileName;
    char *dstFileName;
{
    int error;
    Fs_Stream *srcStreamPtr;
    Fs_Stream *dstStreamPtr;
    int offset;
    Address buffer;

    srcStreamPtr = (Fs_Stream *)NIL;
    error = Fs_Open(srcFileName, FS_READ, FS_FILE, 0, &srcStreamPtr);
    if (error) {
	Sys_SafePrintf("Fs_Copy: can't open source file (%s)\n", srcFileName);
	return(error);
    }
    dstStreamPtr = (Fs_Stream *)NIL;
    error = Fs_Open(dstFileName, FS_CREATE|FS_WRITE, FS_FILE, 0666, &dstStreamPtr);
    if (error) {
	Sys_SafePrintf("Fs_Copy: can't open destination file (%s)\n",
				 dstFileName);
	(void)Fs_Close(srcStreamPtr);
	return(error);
    }

#define CP_BUFSIZE	2048

    buffer = Mem_Alloc(CP_BUFSIZE);
    offset = 0;
    while (1) {
	int len;

	len = CP_BUFSIZE;
	error = Fs_Read(srcStreamPtr, buffer, offset, &len);
	if (error) {
	    Sys_SafePrintf("Fs_Copy: read failed\n");
	    break;
	} else if (len == 0) {
	    break ;
	}
	error = Fs_Write(dstStreamPtr, buffer, offset, &len);
	if (error) {
	    Sys_SafePrintf("Fs_Copy: write failed\n");
	    break;
	}
	offset += len;
    }
    Sys_SafePrintf("Fs_Copy: copied %d bytes\n", offset);

    (void)Fs_Close(srcStreamPtr);
    (void)Fs_Close(dstStreamPtr);
    Mem_Free(buffer);
    return(error);
}
#endif /* notdef */
