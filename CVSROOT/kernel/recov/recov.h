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

#ifdef KERNEL
#include <trace.h>
#include <proc.h>
#else
#include <kernel/trace.h>
#include <kernel/proc.h>
#endif
#include <recovBox.h>
#include <hostd.h>

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
 *	RECOV_FAST_BOOT		Set if the server went through a fast boot.
 *	RECOV_SERVER_DRIVEN	Set if the server is driving recovery.
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
/* Space here now. */
#define RECOV_HOST_DEAD		0x4

#define RECOV_HOST_BOOTING	0x10
#define RECOV_CRASH		0x20
#define RECOV_REBOOT		0x40
#define RECOV_FAILURE		0x80

#define RECOV_CRASH_CALLBACKS	0x0100
#define RECOV_WANT_RECOVERY	0x0200
#define RECOV_PINGING_HOST	0x0400
#define RECOV_REBOOT_CALLBACKS	0x0800

#define	RECOV_FAST_BOOT		0x1000
#define	RECOV_SERVER_DRIVEN	0x2000

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
 *	CLT_RECOV_IN_PROGRESS	The client needs to go through full
 *				recovery with the server.  This is set when
 *				the server gets a reopen from the client,
 *				and reset after the
 *				client tells us it's done re-opening files.
 *				The flag is used on the server to tell us to
 *				block opens from this client while it is
 *				going through recovery.
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
 *	CLT_OLD_RECOV		This client is running an old kernel and
 *				returned INVALID_RPC to server trying to get
 *				it to do server-driven recovery.
 *	CLT_DOING_SRV_RECOV	Client is currently going through server-driven
 *				recovery.
 *	SRV__DRIVEN_IN_PROGRESS	Set on client if server has contacted client
 *				for server-driven recovery.
 */
#define CLT_RECOV_IN_PROGRESS	0x01
#define SRV_RECOV_IN_PROGRESS	0x02
#define SRV_RECOV_FAILED	0x04
#define CLT_OLD_RECOV		0x08
#define CLT_DOING_SRV_RECOV	0x10
#define	SRV_DRIVEN_IN_PROGRESS	0x20

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
 *	RECOV_CUZ_CRASH_UNDETECTED	Crash wasn't detected until reboot
 *	RECOV_CUZ_SCHED_CALLBACK	Called Proc_CallFunc to schedule
 *					reboot callbacks.
 *	RECOV_CUZ_DONE_CALLBACKS	Called CallBacksDone. 
 *	RECOV_CUZ_FAILURE	RECOV_FAILURE was detected in CallBacksDone.
 *	RECOV_CUZ_WAS_BOOTING	Got alive packet when in booting state.
 *	RECOV_CUZ_NOW_BOOTING	Got booting packet when in dead state.
 *	RECOV_CUZ_WAS_DEAD	Got alive packet when in dead state.
 *	RECOV_CUZ_DOING_CALLBACKS	In RecovRebootCallBacks. 
 *	RECOV_CUZ_START		We're starting up, so maybe unknown state.
 */
#define RECOV_CUZ_WAIT		0x1
#define RECOV_CUZ_WAKEUP	0x2
#define RECOV_CUZ_INIT		0x4
#define RECOV_CUZ_REBOOT	0x8
#define RECOV_CUZ_CRASH		0x10
#define RECOV_CUZ_DONE		0x20
#define RECOV_CUZ_PING_CHK	0x40
#define RECOV_CUZ_PING_ASK	0x80
#define RECOV_CUZ_CRASH_UNDETECTED	0x100
#define RECOV_CUZ_SCHED_CALLBACK	0x200
#define RECOV_CUZ_DONE_CALLBACKS	0x400
#define RECOV_CUZ_FAILURE	0x800
/* 0x1000 taken by stale handle thing, above. */
#define RECOV_CUZ_WAS_BOOTING	0x2000
#define RECOV_CUZ_NOW_BOOTING	0x4000
#define RECOV_CUZ_WAS_DEAD	0x8000
#define RECOV_CUZ_DOING_CALLBACKS	0x10000
#define RECOV_CUZ_START		0x20000

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

/*
 * More parameters for controlling recovery.  The first of these
 * (0 to 99) are for the use of the recovery box and are defined in
 * /usr/include/recovBox.h.
 */
#define	RECOV_BULK_REOPEN	100	/* Do bulk reopen rpcs. */
#define	RECOV_SINGLE_REOPEN	101	/* Do normal reopen rpcs. */
#define	RECOV_IGNORE_CLEAN	102	/* Don't reopen files that have only
					 * clean blocks in the cache. */
#define	RECOV_REOPEN_CLEAN	103	/* Do reopen files that have only
					 * clean blocks in the cache. */
#define	RECOV_SKIP_CLEAN	104	/* Skip the reopen of files that have
					 * only clean blocks in the cache.
					 * But don't invalidate them as is
					 * done for IGNORE_CLEAN. */
#define RECOV_DO_SERVER_DRIVEN	105	/* Turn on server-driven recovery
					 * acceptance for clients.
					 */
#define RECOV_NO_SERVER_DRIVEN	106	/* Turn off server-driven recovery
					 * acceptance for clients.
					 */

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
extern	Boolean	recov_RestartDebug;

/*
 * TRUE if we're using transparent server recovery.
 */
extern	Boolean		recov_Transparent;
/*
 * TRUE if we want a client to ignore the fact that a server can do transparent
 * recovery.  If the client ignores this, this means it sends reopen requests
 * even though it doesn't have to.  (In this case, the server will check
 * the client's info against what it has stored in its recovery box.)
 */
extern	Boolean		recov_ClientIgnoreTransparent;

/*
 * TRUE if client is ignoring server-driven recovery.  It will repsond
 * instead with "invalid rpc."
 */
extern	Boolean		recov_ClientIgnoreServerDriven;

/*
 * TRUE if we're avoiding downloading text and initialized data.  (Should
 * be true usually.)
 */
extern	Boolean		recov_DoInitDataCopy;

/*
 * Do batching of reopen rpc's.
 */
extern	Boolean		recov_BulkHandles;

/*
 * Don't bother to reopen files that have only clean blocks in the cache.
 */
extern	Boolean		recov_IgnoreCleanFiles;

/*
 * Skip reopening this file.  It may have clean blocks in the cache, but
 * it has no current streams.  The server will be contacted by reguar means
 * the next time somebody opens the file.
 */
extern	Boolean		recov_SkipCleanFiles;
/*
 * Server is driving recovery, rather than allowing clients to initiate it.
 */
extern	Boolean	recov_ServerDriven;

/*
 * We're blocking rpc's that aren't related to recovery.
 */
extern	Boolean	recov_BlockingRpcs;

/*
 * A printf for during recovery.
 */
#define qprintf if (recov_BlockingRpcs) printf

extern void 	Recov_Init _ARGS_((void));
extern void 	Recov_CrashRegister _ARGS_((void (*crashCallBackProc)(), ClientData crashData));
extern void 	Recov_RebootRegister _ARGS_((int spriteID, void (*rebootCallBackProc)(), ClientData rebootData));
extern void 	Recov_RebootUnRegister _ARGS_((int spriteID, void (*rebootCallBackProc)(), ClientData rebootData));
extern void 	Recov_HostAlive _ARGS_((int spriteID, unsigned int bootID, Boolean asyncRecovery, Boolean rpcNotActive, unsigned int recovType));
extern void 	Recov_HostDead _ARGS_((int spriteID));
extern int 	Recov_GetHostState _ARGS_((int spriteID));
extern int 	Recov_GetHostOldState _ARGS_((int spriteID));
extern void 	Recov_SetHostOldState _ARGS_((int spriteID, int state));
extern Boolean 	Recov_GetHostInfo _ARGS_((int spriteID, Recov_State *recovStatePtr));
extern ReturnStatus 	Recov_IsHostDown _ARGS_((int spriteID));
extern void 	Recov_HostTrace _ARGS_((int spriteID, int event));
extern int 	Recov_GetClientState _ARGS_((int spriteID));
extern int 	Recov_SetClientState _ARGS_((int spriteID, int stateBits));
extern void 	Recov_ClearClientState _ARGS_((int spriteID, int stateBits));
extern void 	Recov_AddHandleCountToClientState _ARGS_((int type, int clientID, ReturnStatus status));
extern void 	RecovRebootCallBacks _ARGS_((ClientData data, Proc_CallInfo *callInfoPtr));
extern ReturnStatus 	Recov_DumpClientRecovInfo _ARGS_((int length, Address resultPtr, int *lengthNeededPtr));
extern ReturnStatus 	Recov_GetStats _ARGS_((int size, Address userAddr));
extern ReturnStatus 	Recov_DumpState _ARGS_((int size, Address userAddr));
extern void 		Recov_ChangePrintLevel _ARGS_((int newLevel));
extern void 		RecovPingInit _ARGS_((void));
extern void 		RecovPrintPingList _ARGS_((void));
extern void 		Recov_PrintTrace _ARGS_((ClientData numRecs));
extern void 		Recov_PrintState _ARGS_((void));
extern int 		Recov_PrintTraceRecord _ARGS_((ClientData clientData, int event, Boolean printHeaderFlag));
extern void 		Recov_Proc _ARGS_((void));
extern void 		RecovAddHostToPing _ARGS_((int spriteID));
extern int 		RecovCheckHost _ARGS_((int spriteID));
extern int 		RecovGetLastHostState _ARGS_((int spriteID));
extern Boolean		Recov_InitRecovBox _ARGS_((void));
extern ReturnStatus	Recov_InitType _ARGS_((int objectSize, int maxNumObjects, int applicationTypeID, int *objectTypePtr, unsigned short (*Checksum)()));
extern ReturnStatus	Recov_InsertObject _ARGS_((int typeID, ClientData objectPtr, int applicationObjectNum, Recov_ObjectID *objectIDPtr));
extern ReturnStatus	Recov_InsertObjects _ARGS_((int typeID, int numObjs, char *obuffer, int *objNumBuffer, Recov_ObjectID *objIDBuffer));
extern ReturnStatus	Recov_DeleteObject _ARGS_((Recov_ObjectID objectID));
extern ReturnStatus	Recov_UpdateObject _ARGS_((ClientData objectPtr, Recov_ObjectID objectID));
extern ReturnStatus	Recov_ReturnObject _ARGS_((ClientData objectPtr, Recov_ObjectID objectID, Boolean checksum));
extern ReturnStatus	Recov_ReturnObjects _ARGS_((int typeID, int *olengthPtr, char *obuffer, int *ilengthPtr, char *ibuffer, int *alengthPtr, char *abuffer));
extern ReturnStatus	Recov_ReturnContents _ARGS_((int *lengthPtr, char *buffer));
extern int		Recov_MaxNumObjects _ARGS_((int objectSize, Boolean restart));
extern	void		Recov_PrintSpace _ARGS_((int objectSize));
extern	void		Recov_ToggleChecksum _ARGS_((int typeID));
extern	int		Recov_NumObjects _ARGS_((int typeID));
extern	int		Recov_GetObjectSize _ARGS_((int typeID));
extern ReturnStatus	Recov_MapType _ARGS_((int applicationTypeID, int *typeIDPtr));
extern ReturnStatus	Recov_MapObjectNum _ARGS_((int typeID, int applicationObjectNum, int *objectNumPtr));
extern unsigned short	Recov_Checksum _ARGS_((int len, Address bufPtr));
extern	ReturnStatus	Recov_Cmd _ARGS_((int option, Address argPtr));
extern	void		Recov_InitServerDriven _ARGS_((void));
extern	void		Recov_StartServerDrivenRecovery _ARGS_((int serverID));
extern	void 		Recov_WaitForServerDriven _ARGS_((int serverID));
extern	void		Recov_MarkOldClient _ARGS_((int clientID));
extern	void		Recov_ServerStartingRecovery _ARGS_((void));
extern	void		Recov_ServerFinishedRecovery _ARGS_((void));
extern	Boolean		Recov_HoldForRecovery _ARGS_((int clientID, int command));
extern	void		Recov_StopServerDriven _ARGS_((void));
extern	int		Recov_GetCurrentHostStates _ARGS_((Dev_ClientInfo *infoBuffer, int bufEntries));
extern	void		Recov_MarkDoingServerRecovery _ARGS_((int clientID));
extern	void		Recov_UnmarkDoingServerRecovery _ARGS_((int clientID));
#endif /* _RECOV */

