/*
 * sysStat.h --
 *
 *	User-level definitions of routines and types for system statistics
 *	returned by the Sys_Stats system call.  Instead of a /dev/kmem
 *	type interface, we have calls to return specific kernel structure.
 *	The kernel call takes the following arguments:
 *	Sys_Stats(command, option, argPtr)
 *	commands are defined below, and option and argPtr are interpreted
 *	differently by each command.  Typically argPtr is a buffer that
 *	gets filled in with a system structure, and option is used to
 *	indicate the size, or to control tracing, or isn't used at all.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/include/user/RCS/sysStats.h,v 1.5 92/06/23 12:19:43 kupfer Exp $ SPRITE (Berkeley)
 *
 */

#ifndef _SYSSTATS
#define _SYSSTATS

/*
 * Commands for the Sys_Stats system call.
 *	SYS_RPC_CLT_STATS - Return the Rpc_CltStats structure that contains
 *		client-side statistics for the RPC system. (see rpc.h)
 *	SYS_RPC_SRV_STATS - Return the Rpc_SrvStats structure that contains
 *		server-side statistics for the RPC system. (see rpc.h)
 *	SYS_SYNC_STATS - Return the Sync_Instrument structure which
 *		contains lock and wakeup counts. (see kernel/sync.h)
 *	SYS_SCHED_STATS - Return the Sched_Instrument structure which
 *		contains idle time and context switch counts (kernel/sched.h)
 *	SYS_VM_STATS - Return the Vm_Stat structure which contains every
 *		concievable VM statistic, fault counts etc. (kernel/vmStat.h)
 *	SYS_RPC_TRACE_STATS - Used to both return the trace of recent RPCs,
 *		and to enable/disable the trace, see options below.
 *	SYS_FS_PREFIX_STATS - Return entries from the prefix table. (see fs.h)
 *	SYS_PROC_TRACE_STATS - Used to both return the process migration
 *		trace, and to enable/disable the trace, see options below.
 *	SYS_SYS_CALL_STATS - Return the array of system call counters.
 *		option indicates how many integers the buffer argPtr contains.
 *	SYS_RPC_SERVER_HIST - Return the service time histogram for the RPC
 *		indicated by option. (see kernel/rpcHistogram.h)  If option
 *		is less than or equal to zero the histogram is cleared.
 *	SYS_RPC_CLIENT_HIST - Return service time as obseved by a client.
 *	SYS_NET_GET_ROUTE - Return the route table entry for a particular host.
 *		The data returned are three integers: flags, spriteID, and
 *		route type.  This is then followed by type specific data,
 *		either an ethernet address or an internet address.
 *	SYS_RPC_SRV_STATE - Return the RpcServerState structure for the
 *		RPC server indexed by option.  (see kernel/rpcServer.h)
 *	SYS_RPC_CLT_STATE - Return the RpcClientState structure for the
 *		RPC client channel indexed by option.  (see kernel/rpcClient.h)
 *	SYS_NET_ETHER_STATS - Return the Net_EtherStats structure which
 *		contains interface statistics.  (see kernel/net.h)
 *	SYS_RPC_ENABLE_SERVICE - Enable/disable the service side of the RPC
 *		system.  A non-zero option value enables, zero disables.
 *	SYS_GET_VERSION_STRING - Return the kernel version string.  option
 *		indicates how big the users buffer is.
 *	SYS_PROC_MIGRATION - Enable/Disable process migration to this host.
 *		See options defined below.
 *	SYS_DISK_STATS - Return the Sys_DiskStats structure defined below.
 *		option corresponds to a kernel controller table index.
 *	SYS_FS_PREFIX_EXPORT - Return the export list of a prefix.  The
 *		option parameter indicates the size of the buffer argPtr.
 *		argPtr should contain the prefix upon entry, and is
 *		overwritten with an integer array of SpriteIDs that
 *		corresponds to the export list for the prefix.
 *	SYS_LOCK_STATS - Return the locking statistics. option indicates the
 *		size of the buffer in units of Sync_LockStat structures.
 *	SYS_RPC_SRV_COUNTS - Return the count of RPC service calls.  The
 *		option argument is unused.  If argPtr == NULL then the
 *		counts are printed on the console, otherwise it should be
 *		the address of an integer array size RPC_LAST_COMMAND+1
 *	SYS_RPC_CALL_COUNTS - Return the count of RPC calls.  The
 *		option argument is unused.  If argPtr == NULL then the
 *		counts are printed on the console, otherwise it should be
 *		the address of an integer array size RPC_LAST_COMMAND+1
 *	SYS_LOCK_RESET_STATS - Reset the locking statistics.
 *	SYS_INST_COUNTS - Return information from instruction counts. This
 *		only works on special spur kernels.
 *	SYS_RESET_INST_COUNTS - Reset instruction counts.
 *	SYS_RECOV_STATS - Return information about the recov module.
 *	SYS_RECOV_PRINT - Change printing level of recov module traces.
 *	SYS_FS_RECOV_INFO - Return info with names about the state of
 *		files for recovery testing.
 *	SYS_RECOV_CLIENT_INFO - Dump state on server about per-client recovery.
 *	SYS_RPC_SERVER_TRACE - Turn tracing of rpc servers on or off.
 *	SYS_RPC_SERVER_INFO - Return rpc server tracing info to user.
 *	SYS_RPC_SERVER_FREE - Free up space used by rpc server tracing.
 *	SYS_RPC_SET_MAX - Set the maximum number of server processes.
 *	SYS_RPC_SET_NUM - Create enough server processes to have this many.
 *	SYS_RPC_NEG_ACKS - Turn on or off negative acks on the server.
 *	SYS_RPC_CHANNEL_NEG_ACKS -  Set client policy on or off for handling
 *		neg acks by ramping down the number of client channels.
 *	SYS_RECOV_ABS_PINGS - Whether to use absolute ping intervals or not.
 *	SYS_RECOV_PRINT - Set the recovery print level.
 *	SYS_RPC_NUM_NACK_BUFS - Set the number of negative acknowledgement
 *		buffers.
 *	SYS_START_STATS - Turn on the kernel's periodic printing of sched
 *		and io stats.  TEMPORARY for recovery measurements.
 *	SYS_END_STATS - Turn off the kernel's periodic printing of sched
 *		and io stats.  TEMPORARY for recovery measurements.
 *	SYS_TRACELOG_STATS - Trace log buffer commands (for SOSP91 paper).
 *      SYS_DEV_CHANGE_SCSI_DEBUG - Change debug level for scsi driver.
 *      SYS_SYS_CALL_STATS_ENABLE - Turn on or off system call profiling.
 *      SYS_PROC_SERVERPROC_TIMES - Display the instrumentation for 
 *      	Proc_ServerProcs. 
 *	SYS_NET_GEN_STATS - Return the Net_GenStats structure which 
 *	        contains generic instrumentation about a network interface.
 *	SYS_RPC_SANITY_CHECK - Toggle sanity checks on rpc packets.
 *	SYS_FS_EXTRA_STATS - Extra fs stats that should go into fsStats for
 *				the next global compile.
 */

#define SYS_RPC_CLT_STATS	1
#define SYS_RPC_SRV_STATS	2
#define SYS_SYNC_STATS		3
#define SYS_SCHED_STATS		4
#define SYS_VM_STATS		5
#define SYS_RPC_TRACE_STATS	6
#define SYS_FS_PREFIX_STATS	7
#define SYS_PROC_TRACE_STATS	8
#define SYS_SYS_CALL_STATS	9
#define SYS_RPC_SERVER_HIST	10
#define SYS_RPC_CLIENT_HIST	11
#define SYS_NET_GET_ROUTE	12
#define SYS_RPC_SRV_STATE	13
#define SYS_RPC_CLT_STATE	14
#define	SYS_NET_ETHER_STATS	15
#define SYS_RPC_ENABLE_SERVICE	16
#define SYS_GET_VERSION_STRING	17
#define SYS_PROC_MIGRATION	18
#define	SYS_DISK_STATS		19
#define SYS_FS_PREFIX_EXPORT	20
#define SYS_LOCK_STATS		21
#define SYS_RPC_SRV_COUNTS	22
#define SYS_RPC_CALL_COUNTS	23
#define SYS_LOCK_RESET_STATS	24
#define SYS_INST_COUNTS		25
#define SYS_RESET_INST_COUNTS	26
#define SYS_RECOV_STATS		27
#define SYS_FS_RECOV_INFO	28
#define	SYS_RECOV_CLIENT_INFO	29
#define	SYS_RPC_SERVER_TRACE 	30
#define	SYS_RPC_SERVER_INFO	31
#define	SYS_RPC_SERVER_FREE	32
#define	SYS_RPC_SET_MAX		33
#define	SYS_RPC_SET_NUM		34
#define	SYS_RPC_NEG_ACKS	35
#define	SYS_RPC_CHANNEL_NEG_ACKS	36
#define SYS_RECOV_ABS_PINGS	37
#define SYS_RECOV_PRINT		38
#define	SYS_RPC_NUM_NACK_BUFS	39
#define	SYS_TRACELOG_STATS	40
#define SYS_START_STATS		100
#define SYS_END_STATS		101
#define SYS_DEV_CHANGE_SCSI_DEBUG 102
#define SYS_SYS_CALL_STATS_ENABLE 103
#define SYS_PROC_SERVERPROC_TIMES 104
#define SYS_NET_GEN_STATS	105
#define SYS_RPC_SANITY_CHECK	107
#define SYS_FS_EXTRA_STATS	108


/*
 * Options for the Sys_Stats SYS_RPC_TRACE_STATS command.  If the option
 * is a positive value then that number of trace records are returned
 * into the buffer referenced by argPtr.
 */
#define SYS_RPC_TRACING_PRINT	-1
#define SYS_RPC_TRACING_OFF	-2
#define SYS_RPC_TRACING_ON	-3

/*
 * Options for the Sys_Stats SYS_PROC_TRACE_STATS command.  Use these
 * values for the option argument to the Test_Stats call when using
 * the PROC_TRACE_STATS command.  Any argument greater than the
 * largest positive defined constant is the number of trace records to
 * copy into the output buffer (i.e., it is not permissible to copy
 * only 1-3 records).
 */
#define SYS_PROC_TRACING_PRINT	1
#define SYS_PROC_TRACING_OFF	2
#define SYS_PROC_TRACING_ON	3

/*
 * Options for the Sys_Stats SYS_PROC_MIGRATION command.
 * ALLOW, REFUSE, and GET_STATUS are obsoleted by GET_STATE and SET_STATE.
 *
 *   SYS_PROC_MIG_ALLOW		- allow all migrations to this machine.
 *   SYS_PROC_MIG_REFUSE	- refuse all migrations to this machine.
 *   SYS_PROC_MIG_GET_STATUS	- get whether all migrations are allowed
 *				  or refused.
 *   SYS_PROC_MIG_SET_DEBUG	- set the migration debug level.
 *   SYS_PROC_MIG_GET_VERSION	- get the migration version.
 *   SYS_PROC_MIG_GET_STATE	- get the general migration state.
 *   SYS_PROC_MIG_SET_STATE	- set it.
 *   SYS_PROC_MIG_SET_VERSION	- set the migration version.
 *   SYS_PROC_MIG_GET_STATS	- get statistics.
 *   SYS_PROC_MIG_RESET_STATS	- reset statistics.
 */
#define SYS_PROC_MIG_ALLOW		0
#define SYS_PROC_MIG_REFUSE		1
#define SYS_PROC_MIG_GET_STATUS		2
#define SYS_PROC_MIG_SET_DEBUG		3
#define SYS_PROC_MIG_GET_VERSION	4
#define SYS_PROC_MIG_GET_STATE		5
#define SYS_PROC_MIG_SET_STATE		6
#define SYS_PROC_MIG_SET_VERSION	7
#define SYS_PROC_MIG_GET_STATS		8
#define SYS_PROC_MIG_RESET_STATS	9

/*
 * Options for SYS_TRACELOG_STATS.
 */
#define SYS_TRACELOG_ON		1
#define SYS_TRACELOG_OFF	2
#define SYS_TRACELOG_DUMP	3
#define SYS_TRACELOG_RESET	4
/* 
 * Structure to return SYS_TRACELOG_STATS.
 */
typedef struct Sys_TracelogRecord {
    int		recordLen;	/* Size of this record in bytes. */
    int		time[2];	/* (Timer_Ticks) Timestamp. */
    ClientData	data;		/* Arbitrarily long data. */
} Sys_TracelogRecord;

#define SYS_TRACELOG_KERNELLEN 32
#define SYS_TRACELOG_TYPELEN 8
/*
 * This is the header we write to the user level file.
 * Note: things are in somewhat of a state of flux.  The current status is:
 * File is stored as:
 *   magic #
 *   Sys_TracelogHeader
 *   A bunch of records
 * The fields: numBytes, numRecs, and lostRecords are not used.
 * The traceDir is filled in by the user-level dump program.
 * Lost records are indicated by a special record type in the file.
 * The reason for this format is that it is inconvenient to have the length
 * in the header, since any routine post-processing data and writing a
 * new file would have to go back and modify the header after it knew
 * how many records it had.  This way, you write out a fixed header and
 * then whatever records you want.
 */
typedef struct Sys_TracelogHeader {
    int		numBytes;	/* Total size of the records in bytes. */
				/* Flags are stored in the high 2 bytes. */
    int		numRecs;	/* Number of records. */
    int		machineID;	/* ID of this machine. */
    char	kernel[SYS_TRACELOG_KERNELLEN];	/* Kernel we're running. */
    char	machineType[SYS_TRACELOG_TYPELEN]; /* Machine type. */
    int		bootTime[2];	/* Time of boot (to convert of trace time. */
    int		lostRecords;	/* Records lost from overflow. */
    int		traceDir[4];	/* FileID of the trace directory. */
} Sys_TracelogHeader;

/*
 * This is the structure returned by the kernel.
 */
typedef struct Sys_TracelogHeaderKern {
    int		numBytes;	/* Total size of the records in bytes. */
				/* Flags are stored in the high 2 bytes. */
    int		numRecs;	/* Number of records. */
    int		machineID;	/* ID of this machine. */
    char	kernel[SYS_TRACELOG_KERNELLEN];	/* Kernel we're running. */
    char	machineType[SYS_TRACELOG_TYPELEN]; /* Machine type. */
    int		bootTime[2];	/* Time of boot (to convert of trace time. */
    int		lostRecords;	/* Records lost from overflow. */
} Sys_TracelogHeaderKern;

#define LOST_TYPE 128

#define TRACELOG_FLAGMASK 0xf0000000
#define TRACELOG_TYPEMASK 0x0fff0000
#define TRACELOG_BYTEMASK 0x0000ffff

#define TRACELOG_MAGIC 0x44554d50
#define TRACELOG_MAGIC2 0x44554d51

/*
 * Structure to return for disk stats.
 */
#define	SYS_DISK_NAME_LENGTH	100
typedef struct Sys_DiskStats {
    char	name[SYS_DISK_NAME_LENGTH];	/* Type of disk. */
    int		controllerID;			/* Which controller it is. */
    int		numSamples;			/* Number of times idle time
						 * was sampled. */
    int		idleCount;			/* Number of times disk was
						 * idle when sampled. */
    int		diskReads;			/* The number of sector reads 
						 * from this disk. */
    int		diskWrites;			/* The number of sector writes
						 * from this disk. */
} Sys_DiskStats;

#ifdef SOSP91

#include <spriteTime.h>

typedef	struct	Sys_SchedOverallTimes {
    Time	kernelTime;
    Time	userTime;
    Time	userTimeMigrated;
} Sys_SchedOverallTimes;

/*
 * Statistics for name lookup on client.
 */
typedef struct Sys_SospNameStats {
    Time	totalNameTime;
    Time	nameTime;
    Time	prefixTime;
    Time	miscTime;
    int		numPrefixLookups;
    int		numComponents;
    int		numPrefixComponents;
} Sys_SospNameStats;
#endif SOSP91

extern ReturnStatus		Sys_Stats();

#endif /* _SYSSTATS */
