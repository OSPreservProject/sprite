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
#include "sprite.h"
#include "vmSunConst.h"
#include "sysSysCall.h"
#include "sys.h"
#include "status.h"
#else
#include <kernel/vmSunConst.h>
#include <kernel/sysSysCall.h>
#include <kernel/sys.h>
#include "status.h"
#endif


/*
 * Return codes from some trap routines.
 *
 *    MACH_OK		Successfully handled.
 *    MACH_KERN_ERROR	Debugger must be called.
 *    MACH_USER_ERROR	User process error (bad stack, etc).  Kill user process.
 *    MACH_SIG_RETURN	Returning from signal handler.
 *
 */
#define	MACH_OK		0
#define	MACH_KERN_ERROR	1
#define	MACH_USER_ERROR	2
#define	MACH_SIG_RETURN	3

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
#define	MACH_TRAP_INSTR_1	0x810		/* 129 */
#define	MACH_TRAP_INSTR_2	0x820		/* 130 */
#define	MACH_TRAP_INSTR_3	0x830		/* 131 */
#define	MACH_TRAP_INSTR_4	0x840		/* 132 */
#define	MACH_TRAP_INSTR_5	0x850		/* 133 */
#define	MACH_TRAP_INSTR_LAST	0xff0

#define	MACH_LEVEL0_INT		0x100		/* 16 */
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

#define	MACH_TRAP_DEBUGGER	MACH_TRAP_INSTR_1
#define	MACH_TRAP_SYSCALL	MACH_TRAP_INSTR_3
#define	MACH_TRAP_SIG_RETURN	MACH_TRAP_INSTR_4
#define	MACH_TRAP_FLUSH_WINDOWS	MACH_TRAP_INSTR_5

/*
 * Our trap type is the 2 second to the last hex digits of the tbr register.
 * So "trap type 3" shows up in the last 3 digits of the tbr as 0x030, and
 * trap type 15 shows up as 0x1f0.  The trap types for the software trap
 * instructions start at trap type 128 (0x800 in the tbr).  To get the arguments
 * to a software trap instruction, we take the trap type, subtract off 0x800,
 * and then shift it right by 4 bits to chop off the lowest hex digit.
 * Thus the trap instruction "ta 0" has a trap type of 128 == 0x80 (== 0x800 in
 * the tbr), and ta 1 has a trap type of 129 showing up as 0x810 in the tbr.
 */
/* trap instruction number is trap type - trap type 128 = 0x80 (== -0x800) */
#define	MACH_CALL_DBG_TRAP	((MACH_TRAP_DEBUGGER - 0x800) >>4)
#define	MACH_BRKPT_TRAP		((MACH_TRAP_INSTR_2 - 0x800) >>4)
#define	MACH_SYSCALL_TRAP	((MACH_TRAP_SYSCALL - 0x800) >> 4)
#define	MACH_RET_FROM_SIG_TRAP	((MACH_TRAP_SIG_RETURN - 0x800) >> 4)
#define	MACH_FLUSH_WINDOWS_TRAP	((MACH_TRAP_FLUSH_WINDOWS - 0x800) >> 4)

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
#define	MACH_PS_BIT			0x40	/* and with psr - prev. state */
#define	MACH_FIRST_USER_PSR		0x080	/* traps off, interrupts on,
						 * previous mode not supervisor,
						 * current mode supervisor. */
#define	MACH_NO_INTR_USER_PSR		0xF80
/*
 * psr value for interrupts disabled, traps enabled and window 0.
 * Both supervisor and previous supervisor mode bits are set.
 */
#define	MACH_HIGH_PRIO_PSR		0x00000FE0

/*
 * FPU fsr bits.
 */

#define	MACH_FSR_QUEUE_NOT_EMPTY	 0x2000
#define	MACH_FSR_TRAP_TYPE_MASK		0x1c000
#define	MACH_FSR_NO_TRAP		0x00000
#define	MACH_FSR_IEEE_TRAP		0x04000
#define	MACH_FSR_UNFINISH_TRAP		0x08000
#define	MACH_FSR_UNIMPLEMENT_TRAP	0x0c000
#define	MACH_FSR_SEQ_ERRROR_TRAP	0x10000

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
#ifdef sun4c
#define	MACH_ENABLE_COUNTER0_INTR_LEVEL	MACH_ENABLE_LEVEL10_INTR
#define	MACH_ENABLE_COUNTER1_INTR_LEVEL	MACH_ENABLE_LEVEL14_INTR
#else
#define	MACH_ENABLE_TIMER_INTR_LEVEL	MACH_ENABLE_LEVEL10_INTR
#endif

/*
 * Bits to access bus error register.
 */
#ifdef sun4c
#define	MACH_SIZE_ERROR		0x02
#define	MACH_SB_ERROR		0x10
#define	MACH_TIMEOUT_ERROR	0x20
#define	MACH_PROT_ERROR		0x40
#define	MACH_INVALID_ERROR	0x80
#else
#define	MACH_SIZE_ERROR		0x02
#define	MACH_VME_ERROR		0x10
#define	MACH_TIMEOUT_ERROR	0x20
#define	MACH_PROT_ERROR		0x40
#define	MACH_INVALID_ERROR	0x80
#endif

/*
 * MACH_KERN_START	The address where the kernel image is loaded at.
 * MACH_CODE_START	The address where the kernel code is loaded at.
 * MACH_STACK_START The address of the base of the stack.  1st word is unusable.
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
#define	MACH_KERN_START		0xf6000000
#define	MACH_STACK_START	(MACH_KERN_START + 0x6000)
#define	MACH_CODE_START		(MACH_STACK_START + 0x20)
#define	MACH_STACK_BOTTOM	MACH_KERN_START
#define MACH_KERN_END		VMMACH_NET_MAP_START
#define	MACH_KERN_STACK_SIZE	(MACH_STACK_START - MACH_STACK_BOTTOM)
#define	MACH_BARE_STACK_OFFSET	(MACH_KERN_STACK_SIZE - 8)

/*
 * Constants for the user's address space.
 *
 * MACH_FIRST_USER_ADDR		The lowest possible address in the user's VAS.
 * MACH_LAST_USER_ADDR		The highest possible address in the user's VAS.
 * MACH_LAST_USER_STACK_PAGE	The highest page in the user stack segment.
 * MACH_MAX_USER_STACK_ADDR	The highest value that the user stack pointer
 *				can have.  Note that the stack pointer must be
 *				decremented before anything can be stored on
 *				the stack.  Also note that on the sun4 we must
 *				strip off the high couple of bits, since 0's
 *				and 1's in them point to the same entry in the
 *				segment table.
 *				ACTUALLY: it turns out we can't just strip
 *				those bits, since doing so may put us in the
 *				invalid hole, since the kernel doesn't start
 *				right at the bottom of the top part after the
 *				hole.  I'll have to deal with the user stack
 *				not being contiguous in some way, so it can
 *				start beneath the kernel and continue across
 *				the hole, but for now, I just shrink everything
 *				so the user process stack must start at the
 *				top address beneath the hole.  Yuckola.
 *
 */
#define	MACH_FIRST_USER_ADDR		VMMACH_PAGE_SIZE
#define	MACH_LAST_USER_ADDR		(MACH_MAX_USER_STACK_ADDR - 1)
#define	MACH_LAST_USER_STACK_PAGE	((MACH_MAX_USER_STACK_ADDR - 1) / VMMACH_PAGE_SIZE)
#ifdef NOTDEF
#define	MACH_MAX_USER_STACK_ADDR	(VMMACH_MAP_SEG_ADDR & VMMACH_ADDR_MASK)
#else
#define	MACH_MAX_USER_STACK_ADDR	(0x20000000 - VMMACH_USER_SHARED_PAGES*VMMACH_PAGE_SIZE)
#endif /* NOTDEF */

/*
 * The control space offset of the VME interrupt vector.
 */
#define	MACH_VME_INTR_VECTOR		0xE0000000

/*
 * Instruction executed from stack to cause a return trap to the kernel from
 * a signal handler.  This is "ta MACH_RET_FROM_SIG_TRAP" instruction.
 */
#define	MACH_SIG_TRAP_INSTR	0x91d02004
/*
 * Constants for getting to offsets in structures:  To make sure these
 * constants are correct, there is code in machCode.c that will cause
 * the kernel to die upon booting if the offsets aren't what's here.
 * All sizes are in bytes.
 */
/*
 * Byte offsets from beginning of a Mach_RegState structure to the fields
 * for the various types of registers.
 */
#define	MACH_LOCALS_OFFSET	0
#define	MACH_INS_OFFSET		(MACH_LOCALS_OFFSET + MACH_NUM_LOCALS * 4)
#define	MACH_GLOBALS_OFFSET	(MACH_INS_OFFSET + (MACH_NUM_INS * 2) * 4 + \
				 MACH_NUM_EXTRA_ARGS * 4)
#define	MACH_FPU_FSR_OFFSET	(MACH_GLOBALS_OFFSET + (MACH_NUM_GLOBALS * 4))
#define	MACH_FPU_QUEUE_COUNT	(MACH_FPU_FSR_OFFSET + 4)
#define	MACH_FPU_REGS_OFFSET	(MACH_FPU_QUEUE_COUNT + 4)
#define	MACH_FPU_QUEUE_OFFSET	(MACH_FPU_REGS_OFFSET + (MACH_NUM_FPS * 4))

						/* skip over calleeInputs too */
/*
 * Byte offset from beginning of a Mach_State structure to various register
 * fields, the savedSps field, the savedRegs field, the savedMask field, 
 * the kernStackStart, and the FPU status field.
 */
#define	MACH_TRAP_REGS_OFFSET	0
#define	MACH_SWITCH_REGS_OFFSET	(MACH_TRAP_REGS_OFFSET + 4)
#define	MACH_SAVED_REGS_OFFSET	(MACH_SWITCH_REGS_OFFSET + 4)
#define	MACH_SAVED_MASK_OFFSET	(MACH_SAVED_REGS_OFFSET + (MACH_NUM_WINDOWS *\
				MACH_NUM_WINDOW_REGS * 4))
#define	MACH_SAVED_SPS_OFFSET	(MACH_SAVED_MASK_OFFSET + 4)
#define	MACH_KSP_OFFSET		(MACH_SAVED_SPS_OFFSET + (MACH_NUM_WINDOWS * 4))
#define	MACH_FPU_STATUS_OFFSET  (MACH_KSP_OFFSET + 4)

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
 * The size of the floating point state size in Mach_RegState.
 */
#define	MACH_FP_STATE_SIZE (8 + (MACH_NUM_FPS*4) + (MACH_FPU_MAX_QUEUE_DEPTH*8))
/*
 * The size of a single saved window (locals and ins) in bytes.
 */
#define	MACH_SAVED_WINDOW_SIZE	(MACH_NUM_LOCALS * 4 + MACH_NUM_INS * 4)

/*
 * The size of the state frame that's saved on interrupts, etc.  This must
 * be the size of Mach_RegState, and this is checked in machCode.c.  We
 * shouldn't succeed in booting if these constants are out of whack.  Keep
 * it this way!  Size is in bytes.
 */
#define	MACH_SAVED_STATE_FRAME	(MACH_SAVED_WINDOW_SIZE + MACH_INPUT_STORAGE +\
				MACH_NUM_EXTRA_ARGS * 4 + MACH_NUM_GLOBALS * 4\
				+ MACH_FP_STATE_SIZE)

/*
 * The compiler stores parameters to C routines in its caller's stack frame,
 * so this is at %fp + some_amount.  "Some_amount has to be below (higher addr)
 * than the saved window area, so this means all routines that call C routines
 * with arguments must have a stack frame that is at least
 * MACH_SAVED_WINDOW_SIZE + MACH_INPUT_STORAGE = 96 bytes.  If the
 * frame is also being used for trap state, then it's
 * MACH_SAVED_STATE_FRAME + MACH_INPUT_STORAGE.  This MACH_INPUT_STORAGE is
 * the space for calleeInputs in the Mach_Reg_State structure.
 */
#define	MACH_INPUT_STORAGE	(MACH_NUM_INS * 4)

/*
 * Ugh, there are only 6 input register storage slots, and one "hidden param"
 * slot for an agregate return value.  This is the space before the area
 * where parameters past the 6th begin.
 */
#define	MACH_ACTUAL_HIDDEN_AND_INPUT_STORAGE	(7 * 4)
/*
 * Number of input registers really used as parameters.
 */
#define	MACH_NUM_REAL_IN_REGS	6

/*
 * This doesn't include the space for extra params, since we never use
 * it where extra params are needed.
 */
#define	MACH_FULL_STACK_FRAME	(MACH_SAVED_WINDOW_SIZE + MACH_INPUT_STORAGE)


/*
 * Constant for offset of first argument in a saved window area.
 */
#define	MACH_ARG0_OFFSET	(MACH_NUM_LOCALS * 4)

/*
 * Constant for offset of fp in saved window area.  Fp is %i6.
 */
#define	MACH_FP_OFFSET		(MACH_NUM_LOCALS * 4 + 6 * 4)
#define	MACH_FP_REG		6

/*
 * Constant for offset of return pc in saved window area.  RetPC is %i7.
 */
#define	MACH_RETPC_OFFSET	(MACH_NUM_LOCALS * 4 + 7 * 4)

/*
 * Constant for offset of return from trap pc reg in saved window area.
 * RetFromTrap pc is %l1.
 */
#define	MACH_TRAP_PC_OFFSET	(4)

/*
 * Constant for offset of saved psr in trap regs in saved window area.
 * CurPsr is %l0.
 */
#define	MACH_PSR_OFFSET		(0)

/*
 * Number of parameters beyond the sixth that are allowed on trap entry (for
 * system calls.
 */
#define	MACH_NUM_EXTRA_ARGS	(SYS_MAX_ARGS - 6)

/*
 * The number of registers.
 * MACH_NUM_GLOBALS - Number of %g registers in sparc.
 * MACH_NUM_INS     - Number of %i registers in sparc.
 * MACH_NUM_LOCALS  - Number of %l registers in sparc.
 * MACH_NUM_WINDOW_REGS - Number of registers in each window.
 * MACH_NUM_FPS     - Number of %f registers in sparc. This registers may be
 *		      used as MACH_NUM_FPS floats or MACH_NUM_FPS/2 doubles or
 *		      MACH_NUM_FPS/4 extendeds.
 * MACH_FPU_MAX_QUEUE_DEPTH - The maximum number of entires in the FPU queue
 *			      of unfinished instructions.  This number is 
 *			      implementation dependent with the current 
 *			      value set to match SunOS 4.0.3.
 * 
 */
#define	MACH_NUM_GLOBALS		8
#define	MACH_NUM_INS			8
#define	MACH_NUM_LOCALS			8
#define	MACH_NUM_WINDOW_REGS		(MACH_NUM_LOCALS + MACH_NUM_INS)
#define	MACH_NUM_FPS			32
#define	MACH_FPU_MAX_QUEUE_DEPTH	16
/*
 * The amount to shift left by to multiply a number by the number of registers
 * per window.  How would I get this from the constant above?
 */
#define	MACH_NUM_REG_SHIFT		4

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
 *	f0 to f31	floating points
 *
 *		Special Registers:
 *	current sp		= o6 = r14
 *	current fp		= i6 = r30 = caller's sp
 *	"0 & discard reg"	= g0 = r0
 *	ret addr from C call	= o7 = r15 in old window
 *	ret value from C call	= o0 = r8 in old window
 *	ret addr to C call	= i7 = r31 in new window
 *	ret val to C call	= i0 = r24 in new window
 *	psr saved into		= l0 = r16 as first part of trap
 *	ret pc from trap	= l1 = r17 in new window
 *	ret npc from trap	= l2 = r18 in new window
 *	tbr from trap		= l3 = r19 as first part of trap
 *	y from trap		= l4 = r20 as first part of trap
 *
 * System routines may end up using a window pointed to by wim if a trap
 * was taken before we got a window overflow.  In this case, they can use
 * the local registers, but not the in or out registers, since they may be
 * in use.  They can use global registers known to be free.
 *
 * NOTE: the order of the what's stored into the local routines is important
 * since that's what gets saved on a stack frame for interrupts, etc.  If this
 * changes, the corresponding changes need to be made in machAsmDefs.h and
 * the mach.h definition of Mach_RegState.
 *
 *	RETURN_ADDR_REG	(r15)	Where the return address from a function call
 *				is stored at call-time.
 *	RETURN_ADDR_REG_CHILD (r31)
 *				Where the called window finds return addr.
 *	CUR_PSR_REG	(r16)	Register where psr is stored at trap time.
 *	CUR_PC_REG	(r17)	Register where the first PC is stored on a trap.
 *	NEXT_PC_REG	(r18)	Register where the 2nd PC is stored on a trap.
 *	CUR_TBR_REG	(r19)	Register where the tbr is stored on a trap.
 *	CUR_Y_REG	(r20)	Register where the y reg is stored on a trap.
 *	SAFE_TEMP	(r21) 	Register that cannot be modified by macros or
 *				subroutines within the same window.
 *	VOL_TEMP[1-2]	(r22-r23)
 *				Volatile temporary registers.  Means that
 *				macros in machAsmDefs.h can modify these
 *				registers.
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
#define	CUR_TBR_REG		r19		/* l3 */
#define	CUR_Y_REG		r20		/* l4 */
#define	SAFE_TEMP		r21		/* l5 */
#define	VOL_TEMP1		r22		/* l6 */
#define	VOL_TEMP2		r23		/* l7 */
#define	RETURN_VAL_REG		r8		/* o0 */
#define	RETURN_VAL_REG_CHILD	r24		/* i0 */
#define	TBR_REG			r6		/* g6 */
#define	OUT_TEMP1		r12		/* o4 */
#define	OUT_TEMP2		r13		/* o5 */


#endif /* _MACHCONST */
