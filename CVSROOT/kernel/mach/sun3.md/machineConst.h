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
 * MACH_KERN_START	The address where the kernel image is loaded at.
 * MACH_CODE_START	The address where the kernel code is loaded at.
 * MACH_STACK_BOTTOM	The address of the bottom of the kernel stack for the
 *			main process that is initially run.
 * MACH_KERN_END	The address where the last kernel virtual address is
 *			at.
 * MACH_KERN_STACK_SIZE Number of bytes in a kernel stack.
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
#define	MACH_KERN_START		0xf000000
#define	MACH_CODE_START		0xf004000
#define	MACH_STACK_BOTTOM	0xf000000
#else
#define	MACH_KERN_START		0x800000
#define	MACH_CODE_START		0x804000
#define	MACH_STACK_BOTTOM	0x802000
#endif
#define MACH_KERN_END		VMMACH_DEV_START_ADDR
#define	MACH_KERN_STACK_SIZE	(MACH_CODE_START - MACH_STACK_BOTTOM)
#define	MACH_EXEC_STACK_OFFSET	(MACH_KERN_STACK_SIZE - 8)

#define	MAGIC			0xFeedBabe

/*
 * Constants for the user's address space.
 * 
 * MACH_FIRST_USER_ADDR		The lowest possible address in the user's VAS.
 * MACH_LAST_USER_ADDR		The highest possible address in the user's VAS.
 * MACH_LAST_USER_STACK_PAGE	The highest page in the user stack segment.
 * MACH_MAX_USER_STACK_ADDR	The highest value that the user stack pointer
 *				can have.  Note that the stack pointer must be 
 *				decremented before anything can be stored on 
 *				the stack.
 */

#define	MACH_FIRST_USER_ADDR		VMMACH_PAGE_SIZE
#define	MACH_LAST_USER_ADDR		(MACH_MAX_USER_STACK_ADDR - 1)
#define	MACH_LAST_USER_STACK_PAGE	((MACH_MAX_USER_STACK_ADDR - 1) / VMMACH_PAGE_SIZE)
#define	MACH_MAX_USER_STACK_ADDR	VMMACH_MAP_SEG_ADDR

#ifdef SUN3
/*
 * Constants to access bits in the interrupt register.
 */

#define	SUN_ENABLE_ALL_INTERRUPTS	0x01
#define	SUN_ENABLE_LEVEL7_INTR		0x80
#define	SUN_ENABLE_LEVEL5_INTR		0x20

#endif SUN3

#endif _MACHINECONST
