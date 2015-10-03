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

#include "devAddrs.h"

/*
 * The number of general purpose registers (d0-d7 and a0-a7)
 */
#define	MACH_NUM_GENERAL_REGS	16

/*
 * The indices of all of the registers in the standard 16 register array of
 * saved register.
 */
#define	D0	0
#define	D1	1
#define	D2	2
#define	D3	3
#define	D4	4
#define	D5	5
#define	D6	6
#define	D7	7
#define	A0	8
#define	A1	9
#define	A2	10
#define	A3	11
#define	A4	12
#define	A5	13
#define	A6	14
#define	FP	14
#define	A7	15
#define	SP	15

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
extern	void	Mach_Init();
extern	int	Mach_GetMachineType();
extern	Address	Mach_GetStackPointer();

/*
 * spriteStart is defined in bootSys.s with an underscore.
 */
extern	int		spriteStart;
extern	int		endBss;
extern	int		endText;

/*
 * Machine dependent variables.
 */
extern	int	mach_SP;
extern	int	mach_FP;
extern	Address	mach_KernStart;
extern	Address	mach_CodeStart;
extern	Address	mach_StackBottom;
extern	int	mach_KernStackSize;
extern	Address	mach_KernEnd;
extern	int	mach_DummySPOffset;
extern	int	mach_DummyFPOffset;
extern	int	mach_ExecStackOffset;
extern	Address	mach_FirstUserAddr;
extern	Address	mach_LastUserAddr;
extern	Address	mach_MaxUserStackAddr;
extern	int	mach_LastUserStackPage;

#endif _MACHINE
