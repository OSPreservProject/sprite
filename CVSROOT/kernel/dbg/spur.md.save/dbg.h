/*
 * dbg.h --
 *
 *     Exported types and procedure headers for the debugger module.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DBG
#define _DBG

/*
 * Debugger using syslog to dump output of call command or not.
 */
extern	Boolean	dbg_UsingSyslog;

/*
 * Debugger is using the rs232 to debug.
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
 * Macro to call the debugger from kernel code.  Note that trap type
 * 0 is MACH_CALL_DEBUGGER_TRAP and is defined in machConst.h.
 */
#define DBG_CALL asm("cmp_trap always, r0, r0, $2");

extern	void	Dbg_Init();
extern	void	Dbg_InputPacket();

#endif _DBG
