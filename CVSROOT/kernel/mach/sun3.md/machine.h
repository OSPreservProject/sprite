/*
 * machine.h --
 *
 *     Types, constants, and variables for the Sun hardware.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 *
 */

#ifndef _MACHINE
#define _MACHINE

#include "machineConst.h"
#include "devAddrs.h"

#ifdef SUN3
#define	DELAY(n)	{ register int N = (n)<<1; N--; while (N > 0) {N--;} }
#else
#define	DELAY(n)	{ register int N = (n)>>1; N--; while (N > 0) {N--;} }
#endif

#ifdef SUN3
/*
 * The interrupt register on a sun3.
 */
#define	SunInterruptReg	((unsigned char *) DEV_INTERRUPT_REG_ADDR)
#endif

/*
 * Machine dependent routines.
 */
extern	void	Mach_InitStack();
extern	void	Mach_ContextSwitch();
extern	void	Mach_GetEtherAddress();
extern	int	Mach_TestAndSet();
extern	int	Mach_GetMachineType();

/*
 * spriteStart is defined in bootSys.s with an underscore.
 */
extern	int		spriteStart;
extern	int		endBss;
extern	int		endText;

#endif _MACHINE
