/*
 * migd.h --
 *
 *	Declarations of constants and structures used by the migration
 *	daemon.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/daemons/migd/RCS/migd.h,v 1.2 92/04/29 22:29:43 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _MIGD
#define _MIGD

#include <sprite.h>
#include <fs.h>
#include <mig.h>
#include <list.h>
#include <pdev.h>


/*
 * Types of pdev connections:
 *
 *	MIGD_DAEMON	- a daemon is talking to us.
 *	MIGD_USER	- a user process is talking to us.
 *	MIGD_LOCAL	- a local process is talking to us (to get the load
 *			  average or perform an eviction).
 *	MIGD_NEW	- the connection is new and we don't know who is at the
 *			  other end.
 *	MIGD_CLOSED	- the connection has been forcibly closed and there's
 *			  nothing more we need to do with it.
 */
typedef enum {
    MIGD_DAEMON,
    MIGD_USER,
    MIGD_LOCAL,
    MIGD_NEW,
    MIGD_CLOSED,
} Migd_ServiceType;

/*
 * Define a structure to keep track of open connections.
 * This is reachable from the ClientData part of a Pdev_Stream.
 */
typedef struct {
    List_Links nextStream;	/* Next stream we have open. */
    Pdev_Stream *streamPtr;	/* Back pointer to pdev info. */
    Migd_ServiceType type;	/* Type of connection. */
    int user;			/* User invoked by. */
    int host;			/* Host invoked by. */
    int processID;		/* Process ID of invoker. */
    int openID;		        /* Unique identifier. */
    int	defaultSelBits;		/* What to set default to after an
				   operation. */
    int format;			/* Byte order of this host (set on first
				   ioctl, used by read/write). */
    int	numRequested;		/* Total number of hosts requested. */
    int numObtained;		/* Total number of hosts obtained. */
    int numEvicted;		/* Total number of hosts given and then
				   reclaimed due to eviction. */
    int numInUse;		/* Number of hosts currently assigned to this
				   process, which is the length of the
				   currentRequests list. */
    int	maxRequests;		/* Maximum number of hosts requested.
				   This may be less than numRequested if
				   hosts are assigned, then returned (or
				   reclaimed), then assigned again. */
    int	maxObtained;		/* Maximum number of hosts obtained.
				   This may be less than numAssigned if
				   hosts are assigned, then returned (or
				   reclaimed), then assigned again. */
    int denied;			/* Non-zero if ever denied a host request. */
    int	numStolen;		/* Number of times we stole hosts back. */
    int	stoleTime;		/* Time at which we stole hosts back. */
    List_Links currentRequests;	/* List of outstanding requests. */
    List_Links messages;	/* Messages to pass to process, defined in
				   global.c. */
    struct Migd_WaitList *waitPtr; /* Waiting list, if any. */
} Migd_OpenStreamInfo;

/* 
 * Define a structure for keeping track of clients that want more
 * hosts than we can give them.
 */
typedef struct Migd_WaitList {
    List_Links links;		/* Link to next client. */
    Migd_OpenStreamInfo *cltPtr;/* Client. */
} Migd_WaitList;

/*
 * Define a structure for managing host information.  This is
 * a superset of the Mig_Info that clients can see.
 */
typedef struct Migd_Info {
    List_Links	links;		/* Links within list of idle hosts. */
    Mig_Info	info;		/* Standard info. */
    int		archType;	/* Numeric identifier for architecture type. */
    int		value;		/* Integer "worth" of the machine. Machines
				 * with a higher worth are preferred. */
    int 	flags;		/* Additional info, defined below. */
    List_Links  clientList;	/* Clients using this host
				   (Migd_RequestInfo structures). */
    Migd_OpenStreamInfo *cltPtr; /* Pointer back to open stream info
					   for daemon. */
    char 	*name;		/* Name of host, for debugging purposes. */
    int		lastHostAssigned; /* Host running process to which this host
				     was last assigned. Used for
				     statistical purposes. */
} Migd_Info;

/*
 * Flags for Migd_Info:
 *	MIGD_CHECK_COUNT	- Check when the count of foreign
 *				  processes decreases to determine when
 *				  host is idle again.
 *	MIGD_WAS_EMPTY		- Host had no foreign processes the
 *				  last time we checkpointed.
 */
#define MIGD_CHECK_COUNT 1	
#define MIGD_WAS_EMPTY   2	

/*
 * Subscripts into the queueThreshold array.
 */
#define MIGD_MIN_THRESHOLD 0
#define MIGD_MAX_THRESHOLD 1

/*
 * Arbitrary value larger than the load average on any node (we hope!)
 */
#define MIGD_MAX_LOAD 1000.0

/*
 * If we have to deal with severe clock skew, define USE_GLOBAL_CLOCK.
 */
#define USE_GLOBAL_CLOCK


#define mnew(type)	(type *)Malloc(sizeof(type))

/*
 * We always want to print our processID because we want to catch multiple
 * daemons using the same log file.
 */
#define PRINT_PID fprintf(stderr, "%x: ", migd_Pid)

/*
 * We probably always want to log certain messages to syslog, but we
 * also want them to go to a log file -- at least while syslog isn't
 * logged permanently!
 */
#define SYSLOG0(level, message) \
    syslog(level, message); \
    if (migd_LogToFiles) { \
	PRINT_PID; \
	fprintf(stderr, message); \
    }
#define SYSLOG1(level, message, arg1) \
    syslog(level, message, arg1); \
    if (migd_LogToFiles) { \
	PRINT_PID; \
	fprintf(stderr, message, arg1); \
    }
#define SYSLOG2(level, message, arg1, arg2) \
    syslog(level, message, arg1, arg2); \
    if (migd_LogToFiles) { \
	PRINT_PID; \
	fprintf(stderr, message, arg1, arg2); \
    }
#define SYSLOG3(level, message, arg1, arg2, arg3) \
    syslog(level, message, arg1, arg2, arg3); \
    if (migd_LogToFiles) { \
	PRINT_PID; \
	fprintf(stderr, message, arg1, arg2, arg3); \
    }
#define DATE() \
    if (migd_LogToFiles) { \
        int t; \
	t = time(0); \
	fprintf(stderr, "%x %s", migd_Pid, ctime(&t)); \
    }

/*
 * Macro to add to a counter, watching for overflow.  We use unsigned
 * integers and wrap around if the high-order bit gets set.  This assumes
 * that the amount to be added each time is
 * relatively small (so we can't miss the overflow bit).
 */

#define OVERFLOW_BIT (1 << (sizeof(unsigned int) * 8 - 1))
#define ADD_WITH_OVERFLOW(counter, thisCount) \
    counter[MIG_COUNTER_LOW] += thisCount; \
    if (counter[MIG_COUNTER_LOW] & OVERFLOW_BIT) { \
        counter[MIG_COUNTER_HIGH] += 1; \
        counter[MIG_COUNTER_LOW] &= ~OVERFLOW_BIT; \
    }



extern char 	*Malloc();		/* Utility. */

extern int	migd_Debug;
extern int	migd_Quit;
extern int	migd_Verbose;
extern int	migd_DontFork;
extern int	migd_Version;
extern int	migd_DoStats;
extern int	migd_NeverEvict;
extern int	migd_NeverRunGlobal;
extern int	migd_LogToFiles;
extern int	migd_AlwaysAccept;
extern int	migd_GlobalMaster;
extern int	migd_Pid;
extern char	*migd_GlobalPdevName;
extern char	*migd_GlobalErrorName;	/* Place to write errors for global
					   daemon, or NULL. */
extern char	*migd_LocalPdevName;
extern char	*migd_ProgName;
extern int	migd_HostID;
extern char	*migd_HostName;
extern int	migd_AlwaysAccept;
extern int	migd_NeverAccept;
extern int	migd_MigVersion;
extern int	migd_WriteInterval;
extern int	migd_LoadInterval;
extern double	migd_Weights[];
extern Mig_SystemParms migd_Parms;
extern Fs_TimeoutHandler migd_TimeoutToken;

extern void	Migd_GatherLoad();
extern void	Migd_GetLocalLoad();
extern void	Migd_End();
extern int	Migd_CreateGlobal();
extern int	Migd_ContactGlobal();
extern void 	Migd_HandleRequests();
extern void 	Migd_Evict();
extern int	Migd_GetParms();
extern int	Migd_SetParms();
extern int	Migd_EvictIoctl();

#endif _MIGD



