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
 * Host state flags for use by Recov clients.  These flags are set
 * by users of the Recov module to define/get host states beyond
 * the simple up/down state maintained by the Recov system.
 *	RECOV_IN_PROGESS	The client has crashed and needs to go
 *				through full recovery.  This is set when
 *				we detect a crash, and reset after the
 *				client tells us it's done re-opening files.
 */
#define RECOV_IN_PROGRESS	0x1

/*
 * Trace types for use with Recov_HostTrace.  Compatible with recov.h bits.
 *	RECOV_TRACE_FS_STALE	A stale handle was returned from a file server
 */
#define RECOV_TRACE_FS_STALE	0x1000

void		Recov_CrashRegister();
void		Recov_RebootRegister();
void		Recov_HostAlive();
void		Recov_HostDead();
ReturnStatus	Recov_IsHostDown();
void		Recov_HostTrace();
int		Recov_SetClientState();
int		Recov_GetClientState();
void		Recov_ClearClientState();

void		Recov_HostTrace();
void		Recov_PrintTrace();

#endif _RECOV

