/*
 * recov.h --
 *
 *	External definitions needed by users of the Recovery system.
 *	This module maintains up/down state about other hosts, provides
 *	a call-back mechanism for other modules, and some state bits
 *	that can also be set by other modules.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _RECOV
#define _RECOV

#include "trace.h"

/*
 * External view of the state kept about each host.
 */
typedef struct Recov_State {
    int spriteID;		/* Host ID of the peer */
    unsigned int bootID;	/* Boot timestamp from RPC header */
    int state;			/* Recovery state, defined below */
    int clientState;		/* Client bits */
    Time time;			/* Time of last message */
} Recov_State;
/*
 * recov_PrintLevel defines how noisey we are about other hosts.
 *	Values for the print level should be defined in increasing order.
 */
#define RECOV_PRINT_NONE	0
#define RECOV_PRINT_REBOOT	1
#define RECOV_PRINT_IF_UP	2
#define RECOV_PRINT_CRASH	2
#define RECOV_PRINT_ALL		10

extern int recov_PrintLevel;

/*
 * Host state used by the recov module and accessible via the
 * Recov_{G,S}etHostState calls:
 *	RECOV_STATE_UNKNOWN	Initial state.
 *	RECOV_HOST_ALIVE	Set when we receive a message from the host
 *	RECOV_HOST_DYING	Set when an RPC times out.
 *	RECOV_HOST_DEAD		Set when crash callbacks have been started.
 *	RECOV_HOST_BOOTING	Set in the limbo period when a host is booting
 *				and talking to the world, but isn't ready
 *				for full recovery actions yet.
 *
 *	RECOV_CRASH_CALLBACKS	Set during the crash call-backs, this is used
 *				to block RPC server processes until the
 *				crash recovery actions have completed.
 *	RECOV_WANT_RECOVERY	Set if another module wants a callback at reboot
 *	RECOV_PINGING_HOST	Set for hosts we ping to see when it reboots.
 *	RECOV_REBOOT_CALLBACKS	Set while reboot callbacks are pending.
 *	RECOV_FAILURE		Set if a communication failure occurs during
 *				the reboot callbacks.  This triggers another
 *				invokation of the reboot callbacks so that
 *				a client can't miss out on recovery.
 *
 *	RECOV_CRASH		artificial state to trace RecovCrashCallBacks
 *	RECOV_REBOOT		artificial state to trace RecovRebootCallBacks
 */
#define RECOV_STATE_UNKNOWN	0x0
#define RECOV_HOST_ALIVE	0x1
#define RECOV_HOST_DYING	0x2
#define RECOV_HOST_DEAD		0x4

#define RECOV_HOST_BOOTING	0x10
#define RECOV_CRASH		0x20
#define RECOV_REBOOT		0x40
#define RECOV_FAILURE		0x80

#define RECOV_CRASH_CALLBACKS	0x0100
#define RECOV_WANT_RECOVERY	0x0200
#define RECOV_PINGING_HOST	0x0400
#define RECOV_REBOOT_CALLBACKS	0x0800

/*
 * If dying_state is not defined then crash callbacks are made
 * immidiately after a timeout.  Otherwise the host lingers in
 * the RECOV_HOST_DYING state for recov_CrashDelay seconds.
 */
#undef dying_state

/*
 * Host state flags for use by Recov clients.  These flags are set
 * by users of the Recov module to define/get host states beyond
 * the simple up/down state maintained by the Recov system.
 *	CLT_RECOV_IN_PROGESS	The client has crashed and needs to go
 *				through full recovery.  This is set when
 *				we detect a crash, and reset after the
 *				client tells us it's done re-opening files.
 *	SRV_RECOV_IN_PROGRESS	This is set on a client while it is reopening
 *				files in order to ensure only one set of
 *				reopens is in process.
 *	SRV_RECOV_FAILED	Recovery with the server failed.  This is
 *				used to catch a race between the end of
 *				a failed recovery and re-establishing contact
 *				with the server.  If the server "comes back"
 *				before a failed recovery clears the
 *				SRV_RECOV_IN_PROGRESS bit, then a reboot
 *				callback could get lost.  This bit is used
 *				to detect this.
 */
#define CLT_RECOV_IN_PROGRESS	0x1
#define SRV_RECOV_IN_PROGRESS	0x2
#define SRV_RECOV_FAILED	0x4

/*
 * Whether or not to use absolute intervals for pinging servers.  The default
 * is to use them in order to avoid synchronizing clients due to server
 * reboots.
 */
extern	Boolean	recov_AbsoluteIntervals;

/*
 * Trace types for use with Recov_HostTrace.  Compatible with recov.h bits.
 *	RECOV_TRACE_FS_STALE	A stale handle was returned from a file server
 */
#define RECOV_TRACE_FS_STALE	0x1000

/*
 * A trace is kept for debugging/understanding the host state transisions.
 */
typedef struct RecovTraceRecord {
    int		spriteID;		/* Host ID whose state changed */
    int		state;			/* Their new state */
} RecovTraceRecord;

/*
 * Tracing events, these describe the trace record.
 *
 *	RECOV_CUZ_WAIT		Wait in Recov_WaitForHost
 *	RECOV_CUZ_WAKEUP	Wakeup in Recov_WaitForHost
 *	RECOV_CUZ_INIT		First time we were interested in the host
 *	RECOV_CUZ_REBOOT	We detected a reboot
 *	RECOV_CUZ_CRASH		We detected a crash
 *	RECOV_CUZ_DONE		Recovery actions completed
 *	RECOV_CUZ_PING_CHK	We are pinging the host to check it out
 *	RECOV_CUZ_PING_ASK	We are pinging the host because we were asked
 */
#define RECOV_CUZ_WAIT		0x1
#define RECOV_CUZ_WAKEUP	0x2
#define RECOV_CUZ_INIT		0x4
#define RECOV_CUZ_REBOOT	0x8
#define RECOV_CUZ_CRASH		0x10
#define RECOV_CUZ_DONE		0x20
#define RECOV_CUZ_PING_CHK	0x40
#define RECOV_CUZ_PING_ASK	0x80

#ifndef CLEAN

#define RECOV_TRACE(zspriteID, zstate, event) \
    if (recovTracing) {\
	RecovTraceRecord rec;\
	rec.spriteID = zspriteID;\
	rec.state = zstate;\
	Trace_Insert(recovTraceHdrPtr, event, (ClientData)&rec);\
    }
#else

#define RECOV_TRACE(zspriteID, zstate, event)

#endif /* not CLEAN */

extern Trace_Header *recovTraceHdrPtr;
extern Boolean recovTracing;
/*
 * Statistics about the recovery module.
 */
typedef struct Recov_Stats {
    int packets;	/* Number of packets examined */
    int	pings;		/* Number of pings made to check on other hosts */
    int pingsSuppressed;/* Number of pings that were suppressed due to
			 * recent message traffic */
    int timeouts;	/* The number of timeout's detected */
    int crashes;	/* The number of times crash call-backs were called */
    int nonCrashes;	/* The number of times crash call-backs were avoided */
    int reboots;	/* The number of times reboot call-backs were called */
    int numHostsPinged;	/* The number of hosts being pinged */
} Recov_Stats;

extern Recov_Stats recov_Stats;

extern void             Recov_Init();
extern void		Recov_CrashRegister();
extern void		Recov_RebootRegister();
extern void		Recov_HostAlive();
extern void		Recov_HostDead();
extern int		Recov_GetHostState();
extern ReturnStatus	Recov_IsHostDown();
extern void		Recov_HostTrace();
extern int		Recov_SetClientState();
extern int		Recov_GetClientState();
extern void		Recov_ClearClientState();
extern void		Recov_AddHandleCountToClientState();
extern ReturnStatus	Recov_GetStats();

extern void		Recov_HostTrace();
extern void		Recov_PrintTrace();

extern void		Recov_Proc();
extern void		RecovAddHostToPing();
extern int		RecovCheckHost();

#endif /* _RECOV */

