/* 
 * fsCommand.c --
 *
 *	The guts of the Fs_Command system call.  This is used to
 *	set/get various filesystem parameters.
 *
 *
 * Copyright 1985 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <fs.h>
#include <fsutil.h>
#include <fsNameOps.h>
#include <fsprefix.h>
#include <fsutilTrace.h>
#include <fslcl.h>
#include <fscache.h>
#include <fspdev.h>
#include <fsStat.h>
#include <fsdm.h>
#include <timer.h>
#include <user/fsCmd.h>
#include <rpc.h>
#include <sched.h>
#include <fsrmt.h>
#include <vm.h>
#include <stdlib.h>
#include <stdio.h>
#include <lfs.h>

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
    extern	int	fscache_MaxBlockCleaners;
    extern	int	fscache_NumReadAheadBlocks;
    extern	Boolean	fsconsist_ClientCachingEnabled;

    switch(command) {
	case FS_PREFIX_LOAD: {
	    /*
	     * Load the prefix and serverID into the prefix table.
	     * serverID is usually FS_NO_SERVER, although a known serverID
	     * can be loaded into the table.
	     */
	    Fs_PrefixLoadInfo *argPtr = (Fs_PrefixLoadInfo *) buffer;
	    if (argPtr->prefix[0] != '/' ||(argPtr->serverID < 0 || 
		argPtr->serverID >= NET_NUM_SPRITE_HOSTS)) {
		status = FS_INVALID_ARG;
	    } else {
		int prefixFlags = FSPREFIX_IMPORTED;

		if (argPtr->serverID != RPC_BROADCAST_SERVER_ID) {
		    prefixFlags |= FSPREFIX_REMOTE;
		}
		Fsprefix_Load(argPtr->prefix, argPtr->serverID, prefixFlags);
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

	    localPath = (char *)malloc(argPtr->pathLen1);
	    prefix = (char *)malloc(argPtr->pathLen2);
	    status = Vm_CopyIn(argPtr->pathLen1, argPtr->path1, localPath);
	    if (status == SUCCESS) {
		status = Vm_CopyIn(argPtr->pathLen2, argPtr->path2, prefix);
		if (status == SUCCESS) {
		    status = Fs_Open(localPath, FS_READ|FS_FOLLOW,
						FS_DIRECTORY, 0, &streamPtr);
		    if (status == SUCCESS) {
			if (streamPtr->ioHandlePtr->fileID.type !=
				FSIO_LCL_FILE_STREAM) {
			    printf(
		    "Tried to export non-local file \"%s\" as prefix \"%s\"\n",
				localPath, prefix);
			    (void)Fs_Close(streamPtr);
			    status = FS_NO_ACCESS;
			} else {
			    (void)Fsprefix_Install(prefix,streamPtr->ioHandlePtr,
						    FS_LOCAL_DOMAIN,
		    FSPREFIX_EXPORTED|FSPREFIX_IMPORTED|FSPREFIX_OVERRIDE);
			}
		    }
		}
	    }
	    free(prefix);
	    free(localPath);
	    break;
	}
	case FS_PREFIX_CLEAR: {
	    /*
	     * Clear the handle information about a prefix.
	     */
	    status = Fsprefix_Clear(buffer, FALSE);
	    break;
	}
	case FS_PREFIX_DELETE: {
	    /*
	     * Remote a prefix table entry all-together.
	     */
	    status = Fsprefix_Clear(buffer, TRUE);
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
		Fsprefix_Export(controlPtr->prefix, controlPtr->clientID,
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
		Fscache_SetMinSize(*(int *) buffer);
	    }
	    break;
	}
	case FS_LOWER_MAX_CACHE_SIZE: {
	    /*
	     * Make the minimum size of the file system block cache larger.
	     */
	    if (buffer != (Address)NIL && buffer != (Address)0) {
		Fscache_SetMaxSize(*(int *) buffer);
	    }
	    break;
	}
	case FS_DISABLE_FLUSH: {
	    /*
	     * Turn on or off automatic flushing of the cache.
	     */
	    SWAP_TO_BUFFER(fsutil_ShouldSyncDisks, buffer);
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
	    SWAP_TO_BUFFER(fsutil_Tracing, buffer);
	    break;
	}
	case FS_SET_CACHE_DEBUG: {
	    /*
	     * Set the cache debug flag.
	     */
	    extern int fsconsist_Debug;
	    SWAP_TO_BUFFER(fsconsist_Debug, buffer);
	    break;
	}
	case FS_SET_MIG_DEBUG: {
	    /*
	     * Set the migration debug flag.
	     */
	    extern int fsio_MigDebug;
	    SWAP_TO_BUFFER(fsio_MigDebug, buffer);
	    break;
	}
	case FS_SET_PDEV_DEBUG: {
	    /*
	     * Set the pseudo-device debug flag.
	     */
	    extern Boolean  fspdev_Debug;
	    SWAP_TO_BUFFER(fspdev_Debug, buffer);
	    break;
	}
	case FS_SET_RPC_DEBUG: {
	    /*
	     * Set the rpc debug flag.
	     */
	    SWAP_TO_BUFFER(fsrmt_RpcDebug, buffer);
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
	    extern int fslclNameCaching;
	    SWAP_TO_BUFFER(fslclNameCaching, buffer);
	    break;
	}
	case FS_SET_CLIENT_CACHING: {
	    /*
	     * Set the rpc tracing flag.
	     */
	    SWAP_TO_BUFFER(fsconsist_ClientCachingEnabled, buffer);
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
	    printf("microseconds = %d per CS = %d\n", us,
		       us / *(int *)buffer);
	    break;
	}
	case FS_EMPTY_CACHE: {
	    int *numLockedBlocksPtr = (int *)buffer;

	    Fscache_Empty(numLockedBlocksPtr);
	    break;
	}
	case FS_ZERO_STATS: {
	    /*
	     * Zero out the counters in the fs_Stats struct.  Unfortunately,
	     * some values in the structure can't be zeroed out, so this
	     * must be changed to zero out only some portions.
	     */
	    bzero((Address) &fs_Stats, sizeof(Fs_Stats));
	    status = SUCCESS;
	    break;
	}
	case FS_RETURN_STATS: {
	    if (bufSize > 0) {
		if (bufSize > sizeof(Fs_Stats)) {
		    bufSize = sizeof(Fs_Stats);
		}
		bcopy((Address) &fs_Stats, buffer, bufSize);
		status = SUCCESS;
	    } else {
		status = FS_INVALID_ARG;
	    }
	    break;
	}
	case FS_RETURN_LIFE_TIMES: {
	    if (bufSize >= sizeof(Fs_TypeStats)) {
		bcopy((Address)&fs_TypeStats, buffer, sizeof(Fs_TypeStats));
		status = SUCCESS;
	    } else {
		status = FS_INVALID_ARG;
	    }
	    break;
	}
	case FS_GET_FRAG_INFO: {
	    int	*arrPtr = (int *)buffer;

	    Fscache_CheckFragmentation(arrPtr, arrPtr + 1, arrPtr + 2);
	    break;
	}
	case FS_SET_CLEANER_PROCS:
	    SWAP_TO_BUFFER(fscache_MaxBlockCleaners, buffer);
	    break;
	case FS_SET_READ_AHEAD:
	    SWAP_TO_BUFFER(fscache_NumReadAheadBlocks, buffer);
	    break;
	case FS_SET_RA_TRACING:
	    SWAP_TO_BUFFER(fscache_RATracing, buffer);
	    break;
	case FS_REREAD_SUMMARY_INFO:
	    status = Fsdm_RereadSummaryInfo(buffer);
	    break;
	case FS_SET_BLOCK_SKEW: {
	    /*
	     * Set the block allocation gap.
	     */
	    extern int ofs_AllocGap;
	    SWAP_TO_BUFFER(ofs_AllocGap, buffer);
	    break;
	}

	default:
	    if ((command >= FS_FIRST_LFS_COMMAND) &&
	        (command <= FS_LAST_LFS_COMMAND)) {
		status = Lfs_Command(command, bufSize, buffer);
	    } else {
		status = FS_INVALID_ARG;
	    }
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
    error = Fs_Open(fileName, FS_READ|FS_FOLLOW, FS_FILE, 0, &streamPtr);
    if (error) {
	return(error);
    }

#define CAT_BUFSIZE	80

    buffer = malloc(CAT_BUFSIZE);
    offset = 0;
    while (1) {
	int savedLen, len;

	bzero(buffer, CAT_BUFSIZE);

	savedLen = len = CAT_BUFSIZE;
	error = Fs_Read(streamPtr, buffer, offset, &len);
	if (error || len < savedLen) {
	    break;
	} else {
	    offset += len;
	}
	printf("%s", buffer);
    }
    (void)Fs_Close(streamPtr);
    free(buffer);
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
    error = Fs_Open(srcFileName, FS_READ|FS_FOLLOW, FS_FILE, 0, &srcStreamPtr);
    if (error) {
	Sys_SafePrintf("Fs_Copy: can't open source file (%s)\n", srcFileName);
	return(error);
    }
    dstStreamPtr = (Fs_Stream *)NIL;
    error = Fs_Open(dstFileName, FS_CREATE|FS_WRITE|FS_FOLLOW, FS_FILE, 0666, &dstStreamPtr);
    if (error) {
	Sys_SafePrintf("Fs_Copy: can't open destination file (%s)\n",
				 dstFileName);
	(void)Fs_Close(srcStreamPtr);
	return(error);
    }

#define CP_BUFSIZE	2048

    buffer = malloc(CP_BUFSIZE);
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
    free(buffer);
    return(error);
}
#endif /* notdef */
