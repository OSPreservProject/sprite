/*
 * machMon.h --
 *
 *     Structures, constants and defines for access to the SPUR monitor.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHMON
#define _MACHMON

/*
 * Functions and defines to access the monitor.
 */

extern  void	Mach_MonPrintf();
extern	void 	Mach_MonPutChar ();
extern	int  	Mach_MonMayPut();
extern	void	Mach_MonAbort();
extern	void	Mach_MonReboot();


#endif _MACHMON
