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
/*
 * Flags for the Rpc_RebootNotify.
 */
#define RECOV_WHEN_HOST_DOWN		0x1
#define RECOV_WHEN_HOST_REBOOTS		0x2

/*
 * Host state flags for use by RPC clients.  These flags are set
 * by users of the RPC module to define/get host states beyond
 * the simple up/down state maintained by the RPC system.
 *	RECOV_IN_PROGESS	The client has crashed and needs to go
 *				through full recovery.  This is set when
 *				we detect a crash, and reset after the
 *				client tells us it's done re-opening files.
 */
#define RECOV_IN_PROGRESS	0x1

/*
 * Trace types for use with Rpc_HostTrace.  These are defined to be compatible
 *		with the values defined in rpcRecovery.c
 *	RECOV_TRACE_FS_STALE	A stale handle was returned from a file server
 */
#define RECOV_TRACE_FS_STALE	0x1000

void		Recov_HostNotify();
void		Recov_HostAlive();
void		Recov_HostDead();
int		Recov_WaitForHost();
ReturnStatus	Recov_HostIsDown();
void		Recov_HostTrace();
void		Recov_SetClientState();
int		Recov_GetClientState();

void		Recov_PrintTrace();

#endif _RECOV

