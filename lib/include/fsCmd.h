/*
 * fsCmd.h --
 *
 *	Definitions used by fs command, a general hook into the filesystem.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header: /sprite/src/lib/include/RCS/fsCmd.h,v 1.15 92/08/07 14:49:55 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _FSCMD
#define _FSCMD


/*
 * Flags to the Fs_Command system call.
 *	This is invoked via
 *		status = Fs_Command(command, bufSize, buffer);
 *	
 *	FS_PREFIX_LOAD		Load a prefix into the prefix table.  The
 *				serverID is used to find the server of
 *				the prefix. A serverID of FS_NO_SERVER
 *				will cause a broadcast to find the server
 *				when the first file under that prefix is opened.
 *				(This is mainly needed because Unix servers
 *				don't have Remote Links.)
 *	FS_PREFIX_EXPORT	Export a prefix.  The buffer argument to
 *				Fs_Command is a Fs_TwoPaths struct of which
 *				the first path indicates the local directory
 *				to be exported and the second is its global
 *				prefix.  For dot-dot out the top to be caught
 *				the local file should be the root of a domain.
 *				(This restriction could be fixed.)
 *	FS_PREFIX_CLEAR		Clear the server information about a prefix.
 *				A broadcast will occur the next time the
 *				prefix is used to find the server.
 *	FS_PREFIX_DELETE	Delete a prefix from the table.  Not even
 *				the prefix itself remains.
 *	FS_PREFIX_CONTROL	Control which clients a prefix is exported to
 *	FS_RAISE_MIN_CACHE_SIZE	Set the minimum number of pages in the cache.
 *	FS_LOWER_MAX_CACHE_SIZE	Set the maximum number of pages in the cache.
 *	FS_DISABLE_FLUSH	Disbable automatic flushing of the cache.
 *	FS_SET_TRACING		Set file system tracing flag.
 *	FS_SET_CACHE_DEBUG	Set cache debugging flag.
 *	FS_SET_RPC_DEBUG	Set rpc debugging flag.
 *	FS_SET_RPC_TRACING	Set rpc tracing flag.
 *	FS_SET_RPC_NO_TIMEOUTS	Set rpc_NoTimeouts flag.
 *	FS_SET_NAME_CACHING	Set name caching flag.
 *	FS_SET_CLIENT_CACHING	Set client caching flag.
 *	FS_SET_RPC_CLIENT_HIST	Set client call histgram flag
 *	FS_SET_RPC_SERVER_HIST	Set service-time histogram flag
 *	FS_EMPTY_CACHE		Reset the filesystem cache by writing it
 *				out and invalidating all unlock blocks.
 *	FS_RETURN_STATS		Copy out the filesystem statistics
 *	FS_RETURN_LIFE_TIMES	Copy out the file life time statistics
 *	FS_SET_NO_STICKY_SEGS	Set the no sticky segments flag.
 *	FS_SET_CLEANER_PROCS	Set the maximum number of block cleaner
 *				processes.
 *	FS_SET_READ_AHEAD	Set number of blocks of read ahead.
 *	FS_SET_RA_TRACING	Set read ahead tracing flag.
 *	FS_SET_WRITE_THROUGH	Set the flag that forces write-through.
 *	FS_SET_WRITE_BACK_ON_CLOSE	Set the flag that forces write-back-
 *					on-close.
 *	FS_SET_DELAY_TMP_FILES	Set the flag that uses a fully delayed write
 *				policy on tmp files. 
 *	FS_SET_TMP_DIR_NUM	Set the directory which contains temporary
 *				files.
 *	FS_SET_WRITE_BACK_INTERVAL	Set the number of seconds to delay
 *					before writing back the cache.
 *	FS_SET_WRITE_BACK_ASAP		Set the flag to force write-back
 *					as soon as possible.
 *	FS_SET_WB_ON_LAST_DIRTY_BLOCK	Set the flag to force write-back
 *					as last dirty block from client.
 *	FS_SET_BLOCK_SKEW		Set number of blocks to put between
 *					consecutive allocations on disk.
 *	FS_SET_LARGE_FILE_MODE		Set flag that limits the number of
 *					blocks for large files.
 *	FS_SET_MAX_FILE_PORTION		Set value to divide the maximum
 *					number of blocks by to determine the
 *					maximum file size.
 *	FS_SET_DELETE_HISTOGRAMS	Set the flag to keep delete size/age
 *					information.
 *	FS_ZERO_STATS		Set all file system counts to 0.
 *	FS_SET_MIG_DEBUG	Set process-migration file debugging flag.
 *	FS_REREAD_SUMMARY_INFO	Update the in-memory copy of the domain summary
 *				information.
 *	FS_FIRST_LFS_COMMAND    Value range for LFS specific commands.
 *	FS_LAST_LFS_COMMAND
 *	FS_DO_L1_COMMAND	Simulate L1-foo.
 *	FS_GENERIC_COMMAND	A command for temporary kernel hooks.
 */
#define FS_PREFIX_LOAD			1
#define FS_PREFIX_EXPORT		2
#define FS_PREFIX_CLEAR			3
#define FS_PREFIX_DELETE		4
#define FS_PREFIX_CONTROL		5
#define FS_RAISE_MIN_CACHE_SIZE		6
#define FS_LOWER_MAX_CACHE_SIZE		7
#define FS_DISABLE_FLUSH		8
#define	FS_SET_TRACING			9
#define	FS_SET_CACHE_DEBUG		10
#define	FS_SET_RPC_DEBUG		11
#define	FS_SET_RPC_TRACING		12
#define	FS_SET_NAME_CACHING		13
#define	FS_SET_CLIENT_CACHING		14
#define	FS_TEST_CS			15
#define	FS_SET_RPC_CLIENT_HIST		16
#define	FS_SET_RPC_SERVER_HIST		17
#define	FS_EMPTY_CACHE			18
#define FS_RETURN_STATS			19
#define	FS_GET_FRAG_INFO		21
#define FS_SET_NO_STICKY_SEGS		22
#define	FS_SET_CLEANER_PROCS		23
#define	FS_SET_READ_AHEAD		24
#define	FS_SET_RA_TRACING		25
#define FS_SET_RPC_NO_TIMEOUTS		26
#define	FS_SET_WRITE_THROUGH		27
#define	FS_SET_WRITE_BACK_ON_CLOSE	28
#define	FS_SET_DELAY_TMP_FILES		29
#define	FS_SET_TMP_DIR_NUM		30
#define	FS_SET_WRITE_BACK_INTERVAL	31
#define	FS_SET_WRITE_BACK_ASAP		32
#define FS_SET_WB_ON_LAST_DIRTY_BLOCK	33
#define	FS_SET_BLOCK_SKEW		34
#define	FS_SET_LARGE_FILE_MODE		35
#define	FS_SET_MAX_FILE_PORTION		36
#define	FS_SET_DELETE_HISTOGRAMS	37
#define	FS_ZERO_STATS			38
#define	FS_SET_MIG_DEBUG		40
#define FS_RETURN_LIFE_TIMES		41
#define FS_REREAD_SUMMARY_INFO		42
#define FS_DO_L1_COMMAND		43
#define FS_GENERIC_COMMAND		44
#define FS_FIRST_LFS_COMMAND	       100
#define	FS_CLEAN_LFS_COMMAND	       101
#define	FS_SET_CONTROL_FLAGS_LFS_COMMAND 102
#define FS_GET_CONTROL_FLAGS_LFS_COMMAND 103
#define FS_FREE_FILE_NUMBER_LFS_COMMAND 104
#define	FS_ADJUST_SEG_USAGE_LFS_COMMAND	105
#define	FS_ZERO_ASPLOS_STATS_LFS_COMMAND	106	/* For ASPLOS paper.
							 * Remove when that's
							 * done.  -Mary 2/15/92.
							 */
#define	FS_TOGGLE_ASPLOS_STATS_LFS_COMMAND	107	/* For ASPLOS paper. */
#define	FS_LAST_LFS_COMMAND	       199

/*
 * Structure used with the FS_PREFIX_CONTROL command.  A particular client
 * can be added or deleted from the export list of the prefix.  The delete
 * argument should be FALSE to add, TRUE to delete.  Remember that an
 * empty export list allows all clients access.
 */
typedef struct Fs_PrefixControl {
    int clientID;		/* SpriteID of client to add/delete */
    Boolean delete;		/* TRUE means delete, FALSE means add */
    char prefix[FS_MAX_PATH_NAME_LENGTH];	/* The prefix to control */
} Fs_PrefixControl;

/* Structure used with the FS_PREFIX_LOAD command. The prefix should be
 * loaded with the given serverID. 
 */
typedef struct Fs_PrefixLoadInfo {
    char prefix[FS_MAX_PATH_NAME_LENGTH];	/* Prefix to load. */
    int  serverID;				/* Server of prefix. */
} Fs_PrefixLoadInfo;

#endif /* _FSUSER */
