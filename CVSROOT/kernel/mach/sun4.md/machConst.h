/*
 * machConst.h --
 *
 *     Machine dependent constants.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHCONST
#define _MACHCONST

#ifdef KERNEL
#include "vmSun4Const.h"
#else
#include <kernel/vmSun4Const.h>
#endif

/*
 * Here are the different types of exceptions, represented by the contents
 * of the trap type bits in the trap base register..  These are listed in order
 * of highest priority to lowest. All the MACH_TRAP_INSTR's are of the same
 * priority.
 *
 * Trap Name			Trap Type Field
 */
#define	MACH_RESET		0x000
#define	MACH_INSTR_ACCESS	0x010
#define	MACH_ILLEGAL_INSTR	0x020
#define	MACH_PRIV_INSTR		0x030
#define	MACH_FP_DISABLED	0x040
#define	MACH_CP_DISABLED	0x240		/* 36 */
#define	MACH_WINDOW_OVERFLOW	0x050
#define	MACH_WINDOW_UNDERFLOW	0x060
#define	MACH_MEM_ADDR_ALIGN	0x070
#define	MACH_FP_EXCEP		0x080
#define	MACH_CP_EXCEP		0x280		/* 40 */
#define	MACH_DATA_ACCESS	0x090
#define	MACH_TAG_OVERFLOW	0x0a0
#define	MACH_TRAP_INSTR_FIRST	0x800		/* 128 */
#define	MACH_TRAP_INSTR_LAST	0xff0

#define	MACH_LEVEL1_INT		0x110		/* 17 */
#define	MACH_LEVEL2_INT		0x120
#define	MACH_LEVEL3_INT		0x130
#define	MACH_LEVEL4_INT		0x140
#define	MACH_LEVEL5_INT		0x150
#define	MACH_LEVEL6_INT		0x160
#define	MACH_LEVEL7_INT		0x170
#define	MACH_LEVEL8_INT		0x180
#define	MACH_LEVEL9_INT		0x190
#define	MACH_LEVEL10_INT	0x1a0
#define	MACH_LEVEL11_INT	0x1b0
#define	MACH_LEVEL12_INT	0x1c0
#define	MACH_LEVEL13_INT	0x1d0
#define	MACH_LEVEL14_INT	0x1e0
#define	MACH_LEVEL15_INT	0x1f0

/*
 * Mask for extracting the trap type from the psr.
 */
#define	MACH_TRAP_TYPE_MASK	0xFF0
/*
 * Mask for extracting the trap base address from the psr.
 */
#define MACH_TRAP_ADDR_MASK	0xFFFFF000

/*
 *  Masks for 16 interrupt priority levels:
 *   lowest = 0,   highest = 15.
 */
#define	MACH_SR_PRIO_0		0x0000
#define	MACH_SR_PRIO_1		0x0100
#define	MACH_SR_PRIO_2		0x0200
#define	MACH_SR_PRIO_3		0x0300
#define	MACH_SR_PRIO_4		0x0400
#define	MACH_SR_PRIO_5		0x0500
#define	MACH_SR_PRIO_6		0x0600
#define	MACH_SR_PRIO_7		0x0700
#define	MACH_SR_PRIO_8		0x0700
#define	MACH_SR_PRIO_9		0x0700
#define	MACH_SR_PRIO_10		0x0700
#define	MACH_SR_PRIO_11		0x0700
#define	MACH_SR_PRIO_12		0x0700
#define	MACH_SR_PRIO_13		0x0700
#define	MACH_SR_PRIO_14		0x0700
#define	MACH_SR_PRIO_15		0x0700

/*
 *  State priorities in the processor state register (psr):
 *
 *	MACH_SR_HIGHPRIO	Supervisor mode + interrupts disabled, traps on
 *	MACH_SR_LOWPRIO		Supervisor mode + interrupts enabled, traps on
 *	MACH_SR_USERPRIO	User mode, traps on
 *
 *	For the sun4, these are macros in machAsmDefs.h.  I can't just move
 *	values into the psr the way the sun3 and sun2 do, since that would
 *	change the window we're in, etc.
 */

/*
 * Constants to access bits in the psr register.
 */

#define	MACH_ENABLE_INTR		0xFFFFF0FF	/* and with psr */
#define	MACH_DISABLE_INTR		(~MACH_ENABLE_INTR)	/* or w/ psr */
#define MACH_ENABLE_FPP			0x1000
#define	MACH_CWP_BITS			0x1f	/* cwp bits in psr */
#define	MACH_ENABLE_TRAP_BIT		0x20	/* or with %psr */
#define	MACH_DISABLE_TRAP_BIT		0xFFFFFFDF	/* and with %psr */
#define	MACH_SUPER_BIT			0x80


/*
 * Bits to enable interrupts in interrupt register.
 */
#define	MACH_ENABLE_ALL_INTERRUPTS	0x1
#define	MACH_ENABLE_LEVEL1_INTR		0x2
#define	MACH_ENABLE_LEVEL4_INTR		0x4
#define	MACH_ENABLE_LEVEL6_INTR		0x8
#define	MACH_ENABLE_LEVEL8_INTR		0x10
#define	MACH_ENABLE_LEVEL10_INTR	0x20
#define	MACH_ENABLE_LEVEL14_INTR	0x80
/*
 * Bit to enable in interrupt register for timer.
 */
#define	MACH_ENABLE_TIMER_INTR_LEVEL	MACH_ENABLE_LEVEL10_INTR

#ifdef NOTDEF
/*
 * Different stack formats on a 68000
 */
#define	MACH_SHORT		0x0
#define	MACH_THROWAWAY		0x1
#define	MACH_INST_EXCEPT	0x2
#define	MACH_MC68010_BUS_FAULT	0x8
#define	MACH_COPROC_MID_INSTR	0x9
#define	MACH_SHORT_BUS_FAULT	0xa
#define	MACH_LONG_BUS_FAULT	0xb

/*
 * The sizes of the different stack formats.
 */
#define	MACH_SHORT_SIZE			8
#define MACH_THROWAWAY_SIZE		8
#define	MACH_INST_EXCEPT_SIZE		12
#define MACH_MC68010_BUS_FAULT_SIZE	58
#define MACH_COPROC_MID_INSTR_SIZE	20
#define MACH_SHORT_BUS_FAULT_SIZE	32
#define	MACH_LONG_BUS_FAULT_SIZE	92

#endif /* NOTDEF */

/*
 * MACH_KERN_START	The address where the kernel image is loaded at.
 * MACH_CODE_START	The address where the kernel code is loaded at.
 * MACH_STACK_START The address of the base of the stack. (1st word is
 *							unusable.)
 * MACH_STACK_BOTTOM	The address of the bottom of the kernel stack for the
 *			main process that is initially run.
 * MACH_KERN_END	The address where the last kernel virtual address is
 *			at.
 * MACH_KERN_STACK_SIZE Number of bytes in a kernel stack.
 * MACH_BARE_STACK_OFFSET	Offset of where a bare kernel stack starts.
 *				It doesn't start at the very top because
 *				the debugger requires a couple of integers
 *				of padding on the top.
 * MAGIC		A magic number which is pushed onto the stack before
 *			a context switch.  Used to verify that the stack 
 *			doesn't get trashed.
 */
#define	MACH_KERN_START		0xe000000
#define	MACH_STACK_START	0xe004000
#define	MACH_CODE_START		0xe004020
#define	MACH_STACK_BOTTOM	0xe000000
#define MACH_KERN_END		VMMACH_DEV_START_ADDR
#define	MACH_KERN_STACK_SIZE	(MACH_STACK_START - MACH_STACK_BOTTOM)
#define	MACH_BARE_STACK_OFFSET	(MACH_KERN_STACK_SIZE - 8)
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

/*
 * Constants for getting to offsets in state structure.  To make sure these
 * constants are correct, there is code in machCode.c that will cause
 * the kernel to die upon booting if the offsets aren't what's here.
 * All sizes are in bytes.
 */
#define	MACH_TRAP_REGS_OFFSET		8
#define	MACH_GLOBALS_OFFSET		8
#define	MACH_OUTS_OFFSET		(MACH_GLOBALS_OFFSET + 8 * 4)
#define	MACH_LOCALS_OFFSET		(MACH_OUTS_OFFSET + 8 * 4)
#define	MACH_INS_OFFSET			(MACH_LOCALS_OFFSET + 8 * 4)
#define	MACH_PSR_OFFSET			(MACH_INS_OFFSET + 8 * 4)
#define	MACH_Y_OFFSET			(MACH_PSR_OFFSET + 4)
#define	MACH_TBR_OFFSET			(MACH_PSR_OFFSET + 8)
#define	MACH_WIM_OFFSET			(MACH_PSR_OFFSET + 12)

#ifdef NOTDEF
#define	MACH_PC_OFFSET			(MACH_PSR_OFFSET + 16)
#define	MACH_NEXT_PC_OFFSET		(MACH_PSR_OFFSET + 20)
#endif NOTDEF

/*
 * Maximum number of processors configurable.
 */

#define	MACH_MAX_NUM_PROCESSORS		1

/*
 * More window-related constants
 */
#define	MACH_NUM_WINDOWS		7	/* # of implemented windows */
#define	MACH_VALID_WIM_BITS	0x0000007f	/* is wim value in range? */


/*
 * The size of a single saved window (locals and ins) and all of the saved
 * windows (in bytes).
 */
#define	MACH_SAVED_WINDOW_SIZE	64	/* in bytes */
#define	MACH_SAVED_REG_SET_SIZE		\
				(MACH_NUM_WINDOWS * MACH_SAVED_WINDOW_SIZE)
/*
 * The number of registers.
 */
#define	MACH_NUM_GLOBAL_REGS		8
#define	MACH_NUM_REGS_PER_WINDOW	16
#define	MACH_NUM_ACTIVE_REGS		32
#define	MACH_TOTAL_REGS			(MACH_NUM_GLOBAL_REGS + \
					 MACH_NUM_REGS_PER_WINDOW * \
					 MACH_NUM_WINDOWS)

/*
 * Definitions of registers.
 *
 *
 * The different types of registers.
 *
 *	r0 to r7	globals		g0 to g7
 *	r8 to r15	outs		o0 to o7
 *	r16 to r23	locals		l0 to l7
 *	r24 to r31	ins		i0 to i7
 *
 *		Special Registers:
 *	current sp		= o6 = r14
 *	current fp		= i6 = r30 = caller's sp
 *	"0 & discard reg"	= g0 = r0
 *	ret addr from C call	= o7 = r15 in old window
 *	ret value from C call	= o0 = r8 in old window
 *	ret addr to C call	= i7 = r31 in new window
 *	ret val to C call	= i0 = r24 in new window
 *	ret pc from trap	= l1 = r17 in new window
 *	ret npc from trap	= l2 = r18 in new window
 *	psr saved into		= l0 = r16 as first part of trap
 *
 * System routines may end up using a window pointed to by wim if a trap
 * was taken before we got a window overflow.  In this case, they can use
 * the local registers, but not thein or out registers, since they may be
 * in use.  They can use global registers known to be free.
 *
 *	RETURN_ADDR_REG	(r15)	Where the return address from a function call
 *				is stored at call-time.
 *	RETURN_ADDR_REG_CHILD (r31)
 *				Where the called window finds return addr.
 *	CUR_PSR_REG	(r16)	Register where psr is stored at trap time.
 *	CUR_PC_REG	(r17)	Register where the first PC is stored on a trap.
 *	NEXT_PC_REG	(r18)	Register where the 2nd PC is stored on a trap.
 *	VOL_TEMP[1-2]	(r19-r20)
 *				Volatile temporary register.  Means that 
 *				macros in machAsmDefs.h can modify these 
 *				registers.
 *	SAFE_TEMP	(r21)
 *				Register that cannot be modified by macros or
 *				subroutines within the same window.
 *	NON_INTR_TEMP[1-2] (r22-r23)
 *				Registers that cannot be modified by interrupt
 *				handlers.
 *	RETURN_VAL_REG	(r8)	Where a value is returned from a C routine.
 *	RETURN_VAL_REG_CHILD (r24)
 *				Where to return a value to our caller.
 *	
 */
#define	RETURN_ADDR_REG		r15		/* o7 */
#define	RETURN_ADDR_REG_CHILD	r31		/* i7 */
#define	CUR_PSR_REG		r16		/* l0 */
#define	CUR_PC_REG		r17		/* l1 */
#define	NEXT_PC_REG		r18		/* l2 */
#define	VOL_TEMP1		r19		/* l3 */
#define	VOL_TEMP2		r20		/* l4 */
#define	SAFE_TEMP		r21		/* l5 */
#define	NON_INTR_TEMP1		r22		/* l6 */
#define	NON_INTR_TEMP2		r23		/* l7 */
#define	RETURN_VAL_REG		r8		/* o0 */
#define	RETURN_VAL_REG_CHILD	r24		/* i0 */
#define	TBR_REG			r6		/* g6 */

#endif /* _MACHCONST */
