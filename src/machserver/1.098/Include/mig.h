/*
 * mig.h --
 *
 *	Declarations of structures, constants, and procedures to manage
 *	the global database of host load averages and uptimes.
 *
 * Copyright 1987, 1988, 1989, 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/mig.h,v 1.15 90/05/16 11:51:27 douglis Exp $ SPRITE (Berkeley)
 */

#ifndef _MIG
#define _MIG

#include <fs.h>

/*
 * We keep track of 1, 5, and 15-minute load averages in the database.
 * We sample the load every MIG_LOAD_INTERVAL seconds and update it
 * in the global daemon's database at least every MIG_GLOBAL_UPDATE_INTERVAL
 * seconds.  A host is considered "down" if it hasn't updated within
 * MIG_TIMEOUT seconds.
 */
#define MIG_NUM_LOAD_VALUES 3
#define MIG_LOAD_INTERVAL 5
#define MIG_GLOBAL_UPDATE_INTERVAL (12 * MIG_LOAD_INTERVAL)
#define MIG_TIMEOUT (3 * MIG_GLOBAL_UPDATE_INTERVAL)

/*
 * Constants affecting how hard and how often processes will try to
 * contact the global daemon.  A process should sleep 1 second, try
 * again, and then double the time it sleeps until it has tried
 * MIG_DAEMON_RETRY_COUNT times.  2**5 is just about 30 seconds.
 */
 
#define MIG_DAEMON_RETRY_COUNT 5

/* 
 * Host states:
 *
 *	MIG_HOST_ACTIVE		- Host has active user, or a high load from
 *				  local processes.
 *	MIG_HOST_IDLE		- Host is completely idle, with no foreign
 *				  processes.
 *	MIG_HOST_PART_USED	- Host is being used by some processes,
 *				  but still has capacity for foreign procs.
 *	MIG_HOST_FULL		- Host is completely used by the highest
 *				  priority processes but would otherwise
 *				  accept foreign processes.
 *	MIG_HOST_REFUSES	- Host refuses all migrations.
 *	MIG_HOST_DOWN		- Host is not running or is unreachable.
 *
 * These states are associated with hosts; processes have priorities
 * based on the ones defined in proc.h (PROC_HIGH/NORMAL/LOW_PRIORITY),
 * but 0-based.  We also define a constant for indexing among these priorities.
 */
#define	MIG_HOST_ACTIVE		0
#define	MIG_HOST_IDLE		1
#define	MIG_HOST_PART_USED	2
#define	MIG_HOST_FULL		3
#define	MIG_HOST_REFUSES	4
#define	MIG_HOST_DOWN		5
#define MIG_NUM_STATES	 	(MIG_HOST_DOWN + 1)

#define MIG_LOW_PRIORITY	 0
#define MIG_NORMAL_PRIORITY	 1
#define MIG_HIGH_PRIORITY	 2
#define MIG_NUM_PRIORITIES	 (MIG_HIGH_PRIORITY + 1)

/*
 * For each machine, keep track of the timestamp for its information, various
 * load averages, and info about idle time and foreign processes.
 * The following structures define the way this information is used.
 * A Mig_LoadVector is the load information that varies from time to time.
 * This info is updated by the loadavg daemons and may be used by widgets or
 * other programs that periodically sample the load average.  Other information
 * is more static -- it won't change after a host boots, or it changes
 * independent of the loadavg daemon (for example, when a host is used
 * for migration).
 */

/*
 ********************************************************************
 *			       IMPORTANT NOTE 			    *
 ********************************************************************
 *								    *
 * Changes to the structures in this file must also be reflected in *
 * the Fmt format strings used by the migration server.		    *
 *								    *
 ********************************************************************
 */
 
typedef struct {
    int 	timestamp;			/* when info last updated */
    int 	noInput;			/* time since last input */
    int		allowMigration;			/* host allowing migration? */
    int		foreignProcs;			/* total number of foreign
						   processes recorded
						   by kernel */
    int		utils[MIG_NUM_LOAD_VALUES];	/* avg utilizations (in %) */
    int		pad;				/* pad structures */
    double	lengths[MIG_NUM_LOAD_VALUES];	/* avg ready-queue lengths */
} Mig_LoadVector;

typedef struct {
    int		hostID;			/* host for which info valid */
    int 	bootTime;		/* when host last rebooted  */
    int		migVersion;		/* migration version level of
					   kernel */
    int		maxProcs;		/* maximum number of foreign
					   processes */
    int		foreign[MIG_NUM_PRIORITIES];
    					/* number of foreign
					   jobs currently assigned to this
					   host, for each priority */
    int		state;			/* state of the host w.r.t.
					   migration: see above. */
    int		pad[4];			/* for future expansion */
    Mig_LoadVector loadVec;		/* updated periodically by loadavg
					   daemon */
} Mig_Info;


/*
 * Define structures and constants for interfacing with the daemons using
 * pdevs.
 */

/*
 * IOControls for communication between Mig lib and global migration daemon
 *	IOC_MIG_GETINFO		- return Mig_Info for one or more hosts.
 *	IOC_MIG_GETIDLE		- get idle host(s).
 *	IOC_MIG_DONE		- return idle host(s) to free pool.
 *	IOC_MIG_KILL		- remove load value(s) from database.
 *	IOC_MIG_DAEMON		- flags process as a loadavg daemon
 * 				  and initializes load information.
 *	IOC_MIG_CHANGE		- daemon is changing host status.
 *	IOC_MIG_GET_PARAMS 	- get system parameters.
 *	IOC_MIG_SET_PARAMS 	- set system parameters (must be root).
 *	IOC_MIG_GET_STATS 	- get statistics.
 *	IOC_MIG_GET_UPDATE 	- get update on host availability.
 *	IOC_MIG_EVICT		- force all processes to be evicted.
 *	IOC_MIG_RESET_STATS 	- reset statistics.
 */

#define IOC_MIG_GETINFO		(IOC_GENERIC_LIMIT+1)
#define IOC_MIG_GETIDLE         (IOC_GENERIC_LIMIT+2)
#define IOC_MIG_DONE            (IOC_GENERIC_LIMIT+3)
#define IOC_MIG_KILL            (IOC_GENERIC_LIMIT+4)
#define IOC_MIG_DAEMON		(IOC_GENERIC_LIMIT+5)
#define IOC_MIG_CHANGE          (IOC_GENERIC_LIMIT+6)
#define IOC_MIG_GET_PARAMS      (IOC_GENERIC_LIMIT+7)
#define IOC_MIG_SET_PARAMS      (IOC_GENERIC_LIMIT+8)
#define IOC_MIG_GET_STATS	(IOC_GENERIC_LIMIT+9)
#define IOC_MIG_GET_UPDATE	(IOC_GENERIC_LIMIT+10)
#define IOC_MIG_EVICT		(IOC_GENERIC_LIMIT+11)
#define IOC_MIG_RESET_STATS	(IOC_GENERIC_LIMIT+12)
#define IOC_MIG_LASTCMD		IOC_MIG_RESET_STATS

/*
 * IOC_MIG_GETINFO -
 * For requesting information about host statuses.  Returns the number
 * of hosts for which information is returned, followed by an array of
 * Mig_Info structures.  The data returned are not aligned as though
 * they were in a structure; therefore, the caller may need to bcopy
 * from a character buffer into a structure to make use of the data.
 * The normal interface to get this info is via the library routine
 * Mig_GetAllInfo or Mig_GetInfo, which get the Mig_Info structures
 * for many hosts or a single host, respectively [due to historical
 * reasons].
 */

typedef struct {
    int firstHost;		/* ID of first host requested. */
    int numRecs;		/* Number of entries requested. */
} Mig_InfoRequest;


/*
 * IOC_MIG_GETIDLE -
 *
 * For requesting idle hosts.  Returns the number of hosts assigned followed
 * by the numeric identifiers of the hosts.
 */

typedef struct {
    int numHosts;		/* Number of hosts requested. */
    int flags;			/* Flags, defined below. */
    int priority;		/* Priority of processes, defined above. */
    int	virtHost;		/* Virtual host of process making request. */
} Mig_IdleRequest;

/*
 * Flags for foreign processes:
 *
 * 	MIG_PROC_RELOCATE	- Wish to relocate if evicted.
 * 	MIG_PROC_AGENT		- Request is on behalf of another process.
 *				  Do not reclaim host because process that
 *				  makes the request closes its connection.
 */
 
#define MIG_PROC_RELOCATE	0x0001
#define MIG_PROC_AGENT		0x0002


/*
 * IOC_MIG_DONE -
 *
 * For returning idle hosts to the pool, or removing hosts from the database,
 * An array of hostIDs is passed.  In each case, nothing is returned.
 * MIG_ALL_HOSTS indicates that the operation should be performed for
 * all hosts (either hosts in use for migration, or every host in the database,
 * respectively).  Hosts are reclaimed implicitly if the process closes the
 * pdev talking to the server (including if it exits).
 */

#define MIG_ALL_HOSTS 0

/*
 * IOC_MIG_GET_UPDATE -
 *
 * For getting updates to host availability.  The caller is responsible
 * for making as many ioctls as needed until the stream is not selectable,
x * since only a single update is transferred with each ioctl. (Normally
 * only one is necessary.)
 *
 * There is no input for this ioctl. The output is an integer specifying
 * a host that is no longer available, or 0, indicating that a new
 * host is available and IOC_MIG_GET_IDLE should be used to get a new
 * host.
 */

/*
 * IOC_MIG_GET_STATS -
 * IOC_MIG_RESET_STATS -
 *
 * GET_STATS returns a structure, defined below.  RESET_STATS takes and
 * returns no arguments.
 *
 * Define the structure used to maintain statistics for migration.  Note
 * that changes to these structures must be reflected in the byte-swapping
 * constants used by the migration daemon.
 *
 * Define the maximum number of architecture types we'll
 * keep statistics about, and statistics that are kept track of on a
 * per-machine-type basis.
 *
 * MIG_MAX_ARCH_TYPES 	- maximum number of architectures managed by migd.
 *			- This can be increased, but only by complicating
 *			  the interface to obtain statistics so it can
 *			  transfer the buffer in pieces.
 * MIG_MAX_ARCH_LEN 	- maximum length of a string used in stats.
 * MIG_MAX_HOSTS_DIST 	- maximum number of hosts in a request that we'll keep
 *			  track of (i.e. last element of array is this many
 *			  or more).
 * MIG_INTERVAL_PERIOD  - number of seconds between incrementing counters
 * MIG_STATS_VERSION	- version of statistics structure, to catch
 *			  inconsistencies.
 */

#define MIG_MAX_ARCH_TYPES 8
#define MIG_MAX_ARCH_LEN 12
#define MIG_MAX_HOSTS_DIST 20
#define MIG_INTERVAL_PERIOD 300
#define MIG_STATS_VERSION 5


/*
 * Statistics that are kept as a sum and as a sum of squares (for standard
 * deviation).  The ones that are likely to overflow are kept as two
 * integers and a macro is used to add to them.
 */

#define MIG_COUNTER_HIGH 1
#define MIG_COUNTER_LOW 0

typedef struct {
    unsigned int requested;		/* Number of hosts requested. */
    unsigned int obtained;		/* Number of hosts obtained. */
    unsigned int evicted;		/* Number of hosts taken back from
					   client due to eviction. */
    unsigned int reclaimed;		/* Number of hosts taken back from
					   client due to other causes
					   (e.g., fairness). */
    unsigned int timeUsed;		/* Total time before returning hosts,
					   in seconds. */
    unsigned int timeToEviction;	/* Total time before evictions occur. */
    unsigned int hostIdleObtained[2];	/* Idle time of hosts obtained when
					   assigned, in minutes. */
    unsigned int hostIdleEvicted[2];	/* Idle time of hosts at time they are
					   assigned, just for those hosts that
					   later evict processes. */
    unsigned int idleTimeWhenActive[2];	/* The amount of time hosts were idle
					   when they became non-idle. */
    unsigned int hostCounts[MIG_NUM_STATES]; /* Number of hosts in each
						state. */
    int pad[2];				/* Pad to double-word boundary. */
} Mig_StatTotals;

typedef struct {
    char arch[MIG_MAX_ARCH_LEN];	/* String representation of machine
					   type. */
    unsigned int numClients;		/* Number of processes requesting
					   hosts. */
    unsigned int gotAll;		/* Number of processes getting as
					   many hosts as requested. */
    unsigned int requestDist[MIG_MAX_HOSTS_DIST + 1];
    					/* Distribution of maximum number of
					   hosts requested. */
    unsigned int obtainedDist[MIG_MAX_HOSTS_DIST + 1];
    					/* Distribution of maximum number of
					   hosts obtained. */
    unsigned int nonIdleTransitions;	/* Number of times hosts went from
					   idle to non-idle. */
    Mig_StatTotals counters;		/* Counters of different types of
					   operations, cumulative. */
    Mig_StatTotals squared;		/* Sum of Squares of above counters
					   (for calculating std. dev.). */
} Mig_ArchStats;

typedef struct {
    unsigned int version;		/* Version number of the daemon. */
    unsigned int checkpointInterval;	/* Interval for checkpointing (and
					   incrementing counters). */
    unsigned int firstRun;		/* Time when statistics first started
					   gathering. */
    unsigned int restarts;		/* Number of times the daemon
					   restarted. */
    unsigned int intervals;		/* Number of intervals over which
					   statistics have been gathered. */
    unsigned int maxArchs;		/* Maximum number of architecture
					   types we know about. */
    unsigned int getLoadRequests;	/* Number of times clients asked for
					   load info. */
    unsigned int totalRequests;		/* Total number of times hosts were
					   requested. */
    unsigned int totalObtained;		/* Total number of times hosts were
					   assigned. */
    unsigned int numRepeatRequests;	/* Number of times the requesting host
					   was the same as the previous
					   request. */
    unsigned int numRepeatAssignments;	/* Number of times the same
					   <physical,virtual> host pair was
					   assigned twice in a row. */
    unsigned int numFirstAssignments;	/* Number of assignments that were
					   to hosts that hadn't been assigned
					   to anyone since going idle.*/
    Mig_ArchStats archStats[MIG_MAX_ARCH_TYPES]; /* Per-architecture stats. */
} Mig_Stats;    


/*
 * IOC_MIG_KILL	  	- sends int.
 * IOC_MIG_DAEMON 	- sends Mig_Info structure.
 * IOC_MIG_CHANGE  	- sends int.
 * IOC_MIG_EVICT  	- receives int.
 */

/*
 * IOC_MIG_GET_PARAMS -
 * IOC_MIG_SET_PARAMS -
 *
 * Define the parameters used by the migration daemons.  This can be
 * updated or obtained via an ioctl.  If the global daemon is updated
 * then it notifies the other daemons to retrieve the new parameters.
 * One can also connect to the local daemon and modify the info for a
 * particular host.  The migration version and criteria are not
 * broadcast from the global server.  
 *
 * The criteria for allowing migration, unless overridden, are idle
 * time (noInput) and ready queue lengths.  If we are not allowing
 * foreign processes, but our time since last input is greater than
 * noInput and our average queue lengths are ALL less than the
 * corresponding values in min, start accepting foreign processes.  If
 * we are allowing foreign processes and either the idle time drops or
 * ANY of the average queue lengths exceeds its corresponding value in
 * max, stop accepting them.
 */

typedef struct {
    int		criteria;			/* Whom to allow, and when
						   (c.f. proc.h). */
    int		version;			/* Migration version. */
    int		noInput;			/* Idle time needed. */
    int		pad;				/* Fill out to doubleword. */
    double	minThresh[MIG_NUM_LOAD_VALUES];	/* Minimum load before
						   becoming idle */
    double	maxThresh[MIG_NUM_LOAD_VALUES];	/* Maximum load before
						   refusing migrations once
						   idle. */
} Mig_SystemParms;


/* 
 * Declare the global variables that refer to the pdevs used.  
 */
extern int mig_GlobalPdev;     
extern int mig_LocalPdev;


extern Mig_Info *    Mig_GetInfo();
extern int	     Mig_GetAllInfo();
extern int	     Mig_GetIdleNode();
extern int	     Mig_OpenInfo();
extern int	     Mig_UpdateInfo();
extern int	     Mig_Done();
extern int	     Mig_ConfirmIdle();
extern int	     Mig_DeleteHost();
extern int	     Mig_Evict();
extern char *	     Mig_GetPdevName();
extern int	     Mig_OpenPdev();

#endif /* _MIG */
