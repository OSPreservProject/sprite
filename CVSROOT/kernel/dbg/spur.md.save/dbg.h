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
 * Macro to call the debugger from kernel code.
 */
#define DBG_CALL asm("cmp_trap always, r0, r0, $MACH_BREAKPOINT_TRAP");

extern	void	Dbg_Init();
extern	void	Dbg_InputPacket();

#endif _DBG
