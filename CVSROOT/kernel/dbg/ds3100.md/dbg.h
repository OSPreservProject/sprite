/*
 * dbg.h --
 *
 *	Exported types and procedure headers for the debugger module.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBG
#define _DBG

#ifndef _SPRITE
#include "sprite.h"
#endif

/*
 * Variable to indicate if are using the rs232 debugger or the network debugger.
 */
extern	Boolean	dbg_Rs232Debug;

/*
 * Variable to indicate that dbg wants a packet.
 */
extern	Boolean	dbg_UsingNetwork;

/*
 * Variable that indicates that we are under control of the debugger.
 */
extern	Boolean	dbg_BeingDebugged;

/*
 * Variable that indicates that we are in the debugger command loop.
 */
extern	Boolean	dbg_InDebugger;

/*
 * The maximum stack address.
 */
extern	int	dbgMaxStackAddr;

/*
 * Debugger using syslog to dump output of call command or not.
 */
extern	Boolean	dbg_UsingSyslog;

#define	DBG_MAX_REPLY_SIZE	1024
#define	DBG_MAX_REQUEST_SIZE	1024

/*
 * The UDP port number that the kernel and kdbx use to identify a packet as
 * a debugging packet.  (composed from "uc": 0x75 = u, 0x63 = c)
 */

#define DBG_UDP_PORT 	0x7563

/*
 * Variable that is set to true when we are called through the DBG_CALL macro.
 */
extern	Boolean	dbgPanic;

/*
 * Macro to call the debugger from kernel code.
 */
extern	void Dbg_Call();
#define DBG_CALL	dbgPanic = TRUE; Dbg_Call();

/*
 * Number of bytes between acknowledgements when the the kernel is writing
 * to kdbx.
 */
#define DBG_ACK_SIZE	256

extern	void	Dbg_Init();
extern	void	Dbg_InputPacket();
extern	Boolean	Dbg_InRange();

#endif /* _DBG */
