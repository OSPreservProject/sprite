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
#include "vmSunConst.h"
#else
#include <kernel/vmSunConst.h>
#endif

/*
 * Here are the different types of exceptions.
 */
#define	MACH_RESET		0
#define	MACH_BUS_ERROR		2
#define	MACH_ADDRESS_ERROR	3
#define	MACH_ILLEGAL_INST	4
#define	MACH_ZERO_DIV		5
#define	MACH_CHK_INST		6
#define	MACH_TRAPV		7
#define	MACH_PRIV_VIOLATION	8
#define	MACH_TRACE_TRAP		9
#define	MACH_EMU1010		10
#define	MACH_EMU1111		11
#define	MACH_STACK_FMT_ERROR	14
#define	MACH_UNINIT_VECTOR	15
#define	MACH_SPURIOUS_INT	24
#define	MACH_LEVEL1_INT		25
#define	MACH_LEVEL2_INT		26
#define	MACH_LEVEL3_INT		27
#define	MACH_LEVEL4_INT		28
#define	MACH_LEVEL5_INT		29
#define	MACH_LEVEL6_INT		30
#define	MACH_LEVEL7_INT		31
#define	MACH_SYSCALL_TRAP	32
#define	MACH_SIG_RET_TRAP	33
#define	MACH_BAD_TRAP		34
#define	MACH_BRKPT_TRAP		35
#define	MACH_UNKNOWN_EXC	36

#ifdef sun3
#define MACH_FP_UNORDERED_COND  48
#define MACH_FP_INEXACT_RESULT  49
#define MACH_FP_ZERO_DIV        50
#define MACH_FP_UNDERFLOW       51
#define MACH_FP_OPERAND_ERROR   52
#define MACH_FP_OVERFLOW        53
#define MACH_FP_NAN             54
#endif

/*
 * The offsets for the various things on the exception stack
 */
#define	MACH_PC_OFFSET	2
#define	MACH_VOR_OFFSET	6

/*
 * Offsets of the fields in the Mach_State structure.
 */
#define	MACH_USER_SP_OFFSET		0
#define MACH_TRAP_REGS_OFFSET		(MACH_USER_SP_OFFSET + 4)
#define	MACH_EXC_STACK_PTR_OFFSET	(MACH_TRAP_REGS_OFFSET + 64)
#define	MACH_LAST_SYS_CALL_OFFSET	(MACH_EXC_STACK_PTR_OFFSET + 4)
#ifdef sun3
#define MACH_TRAP_FP_REGS_OFFSET        (MACH_LAST_SYS_CALL_OFFSET + 4)
#define MACH_TRAP_FP_CTRL_REGS_OFFSET   (MACH_TRAP_FP_REGS_OFFSET + 96)
#define MACH_TRAP_FP_STATE_OFFSET       (MACH_TRAP_FP_CTRL_REGS_OFFSET + 12)
#define	MACH_SWITCH_REGS_OFFSET		(MACH_TRAP_FP_STATE_OFFSET + 180)
#else
#define	MACH_SWITCH_REGS_OFFSET		(MACH_LAST_SYS_CALL_OFFSET + 4)
#endif
#define	MACH_KERN_STACK_START_OFFSET	(MACH_SWITCH_REGS_OFFSET + 64)
#define	MACH_SIG_EXC_STACK_SIZE_OFFSET	(MACH_KERN_STACK_START_OFFSET + 4)
#define	MACH_SIG_EXC_STACK_OFFSET	(MACH_SIG_EXC_STACK_SIZE_OFFSET + 4)
#define MACH_SWITCH_FP_REGS_OFFSET      (MACH_SIG_EXC_STACK_OFFSET + 92)
#define MACH_SWITCH_FP_CTRL_REGS_OFFSET (MACH_SWITCH_FP_REGS_OFFSET + 96)
#define MACH_SWITCH_FP_STATE_OFFSET     (MACH_SWITCH_FP_CTRL_REGS_OFFSET + 12)

/*
 * Amount of data that is pushed onto the stack after a trap occurs.
 */
#define	MACH_TRAP_INFO_SIZE	24

/*
 * Return codes from Exc_Trap.
 *
 *   MACH_OK		The trap was handled successfully.
 *   MACH_KERN_ERROR	The trap could not be handled so the debugger must be
 *			called.
 *   MACH_USER_ERROR	A cross address space copy to/from user space failed
 *			because of a bad address.
 *   MACH_SIG_RETURN	Are returning from a signal handler.
 */
#define	MACH_OK		0
#define	MACH_KERN_ERROR	1
#define	MACH_USER_ERROR	2
#define	MACH_SIG_RETURN	3

/*
 *  Definition of bits in the 68010 status register (SR)
 *	
 *	MACH_SR_TRACEMODE	Trace mode mask
 *	MACH_SR_SUPSTATE	Supervisor state mask
 *	MACH_SR_INTMASK		Interrupt level mask
 *	MACH_SR_CC		Condition codes mask
 */

#define	MACH_SR_TRACEMODE	0x8000
#define	MACH_SR_SUPSTATE	0x2000
#define	MACH_SR_INTMASK		0x0700
#define	MACH_SR_CC		0x001F

/*
 *  Masks for eight interrupt priority levels:
 *   lowest = 0,   highest = 7.
 */
#define	MACH_SR_PRIO_0		0x0000
#define	MACH_SR_PRIO_1		0x0100
#define	MACH_SR_PRIO_2		0x0200
#define	MACH_SR_PRIO_3		0x0300
#define	MACH_SR_PRIO_4		0x0400
#define	MACH_SR_PRIO_5		0x0500
#define	MACH_SR_PRIO_6		0x0600
#define	MACH_SR_PRIO_7		0x0700

/*
 *  State priorities in the status register:
 *
 *	MACH_SR_HIGHPRIO	Supervisor mode + interrupts disabled
 *	MACH_SR_LOWPRIO		Supervisor mode + interrupts enabled
 *	MACH_SR_USERPRIO	User mode
 */
#define	MACH_SR_HIGHPRIO	0x2700
#define	MACH_SR_LOWPRIO		0x2000
#define	MACH_SR_USERPRIO	0x0000

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

/*
 * MACH_KERN_START	The address where the kernel image is loaded at.
 * MACH_CODE_START	The address where the kernel code is loaded at.
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
#ifdef sun3
#define	MACH_KERN_START		0xe000000
#define	MACH_CODE_START		0xe004000
#define	MACH_STACK_BOTTOM	0xe000000
#else
#define	MACH_KERN_START		0x800000
#define	MACH_CODE_START		0x804000
#define	MACH_STACK_BOTTOM	0x802000
#endif
#define MACH_KERN_END		VMMACH_DEV_START_ADDR
#define	MACH_KERN_STACK_SIZE	(MACH_CODE_START - MACH_STACK_BOTTOM)
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
 * Maximum number of processors configuable.
 */

#define	MACH_MAX_NUM_PROCESSORS		1

#ifdef sun3
/*
 * Constants to access bits in the interrupt register.
 */

#define	MACH_ENABLE_ALL_INTERRUPTS	0x01
#define	MACH_ENABLE_LEVEL7_INTR		0x80
#define	MACH_ENABLE_LEVEL5_INTR		0x20

/*
 * Constants to access bits in the system enable register.
 */
#define MACH_ENABLE_FPP			0x40

/*
 * Then number of floating point registers in the mc68881/2
 */
#define MACH_NUM_FPRS       8

/*
 * The amount of memory needed to store the internal state of the
 * floating point coprocessor.  This is 180 for the mc68881, but
 * 212 for the mc68882.
 */
#define MACH_FP_STATE_SIZE  180

#endif /* sun3 */

/*
 * The number of general purpose registers (d0-d7 and a0-a7)
 */
#define	MACH_NUM_GPRS	16

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

#endif /* _MACHCONST */
