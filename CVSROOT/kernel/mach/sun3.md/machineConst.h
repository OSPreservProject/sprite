/*
 * machineConst.h --
 *
 *     Constants for the Sun hardware.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 * $Header$ SPRITE (Berkeley)
 *
 */

#ifndef _MACHINECONST
#define _MACHINECONST

#include "vmSunConst.h"
#include "sunSR.h"

/*
 * MACH_STACK_PTR	The register which serves as the stack pointer.
 * MACH_FRAME_PTR	The register that serves as the frame pointer.
 */

#define	MACH_STACK_PTR	15
#define	MACH_FRAME_PTR	14

/*
 * The number of general purpose registers (d0-d7 and a0-a7)
 */

#define	MACH_NUM_GENERAL_REGS	16

/*
 * MACH_KERNEL_START	The address where the kernel image is loaded at.
 * MACH_CODE_START	The address where the kernel code is loaded at.
 * MACH_STACK_BOTTOM	The address of the bottom of the kernel stack for the
 *			main process that is initially run.
 * MACH_NUM_STACK_PAGES	The number of kernel stack pages.
 * MACH_DUMMY_SP_OFFSET	The offset from the bottom of a newly created kernel
 *			where the new stack pointer is to be initially.
 * MACH_DUMMY_FP_OFFSET	The offset from the bottom of a newly created kernel
 *			where the new frame pointer is to be initially.
 * MACH_EXEC_STACK_OFFSET	Offset of where to start the kernel stack after
 *				an exec.
 * MAGIC		A magic number which is pushed onto the stack before
 *			a context switch.  Used to verify that the stack 
 *			doesn't get trashed.
 */

#ifdef SUN3
#define	MACH_KERNEL_START	0xf000000
#define	MACH_CODE_START		0xf004000
#define	MACH_STACK_BOTTOM	0xf000000
#else
#define	MACH_KERNEL_START	0x800000
#define	MACH_CODE_START		0x804000
#define	MACH_STACK_BOTTOM	0x802000
#endif
#define	MACH_NUM_STACK_PAGES	((MACH_CODE_START - MACH_STACK_BOTTOM ) / VM_PAGE_SIZE)

#define	MACH_DUMMY_SP_OFFSET	(MACH_NUM_STACK_PAGES * VM_PAGE_SIZE - 42)
#define	MACH_DUMMY_FP_OFFSET	(MACH_NUM_STACK_PAGES * VM_PAGE_SIZE - 24)
#define	MACH_EXEC_STACK_OFFSET	(MACH_NUM_STACK_PAGES * VM_PAGE_SIZE - 8)

#define	MAGIC			0xFeedBabe

/*
 * Constants for the user stack.
 * 
 * MACH_LAST_USER_STACK_PAGE	The highest page in the user stack segment.
 * MACH_MAX_USER_STACK_ADDR	The highest value that the user stack pointer
 *				can have.  Note that the stack pointer must be 
 *				decremented before anything can be stored on 
 *				the stack.
 */

#define	MACH_LAST_USER_STACK_PAGE	((MACH_MAX_USER_STACK_ADDR - 1) / VM_PAGE_SIZE)
#define	MACH_MAX_USER_STACK_ADDR	VM_MAP_SEG_ADDR

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
/*
 * Constants to access bits in the interrupt register.
 */

#define	SUN_ENABLE_ALL_INTERRUPTS	0x01
#define	SUN_ENABLE_LEVEL7_INTR		0x80
#define	SUN_ENABLE_LEVEL5_INTR		0x20

#endif SUN3

#endif _MACHINECONST
