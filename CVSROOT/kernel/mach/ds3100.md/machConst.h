/*
 * machConst.h --
 *
 *	Machine dependent constants.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

#ifndef _MACHCONST
#define _MACHCONST

#ifdef KERNEL
#include "vmPmaxConst.h"
#else
#include <kernel/vmPmaxConst.h>
#endif

/*
 * The bits in the cause register.
 *
 *	MACH_CR_BR_DELAY	Exception happened in branch delay slot.
 *	MACH_CR_COP_ERR		Coprocessor error.
 *	MACH_CR_INT_PENDING	Which hardware interrupt is pending.
 *	MACH_CR_SOFT_INT	Which software interrupt is pending.
 *	MACH_CR_EXC_CODE	The exception type (see exception codes below).
 */
#define MACH_CR_BR_DELAY	0x80000000
#define MACH_CR_COP_ERR		0x30000000
#define MACH_CR_INT_PENDING	0x0000FC00
#define MACH_CR_SOFT_INT	0x00000300
#define MACH_CR_EXC_CODE	0x0000003C
#define MACH_CR_EXC_CODE_SHIFT	2

/*
 * Shift to get to the hardware interrupt bits and the number of hardware
 * interrupts.
 */
#define MACH_CR_HARD_INT_SHIFT		10
#define MACH_NUM_HARD_INTERRUPTS	6

/*
 * The different exception codes.
 *
 *	MACH_EXC_INT		Interrupt pending.
 *	MACH_EXC_TLB_MOD	TLB modified fault.
 *	MACH_EXC_TLB_LD_MISS	TLB miss on load or ifetch.
 *	MACH_EXC_TLB_ST_MISS	TLB miss on a store.
 *	MACH_EXC_ADDR_ERR_LD	An address error on a load or ifetch.
 *	MACH_EXC_ADDR_ERR_ST	An address error on a store.
 *	MACH_EXC_BUS_ERR_IFETCH	A bus error on an ifetch.
 *	MACH_EXC_BUS_ERR_LD_ST	A bus error on a load or store.
 *	MACH_EXC_SYSCALL	A system call.
 *	MACH_EXC_BREAK		A breakpoint.
 *	MACH_EXC_RES_INST	A reserved instruction exception.
 *	MACH_EXC_COP_UNUSABLE	Coprocessor unusable.
 *	MACH_EXC_OVFLOW		Arithmetic overflow.
 */
#define MACH_EXC_INT		0
#define MACH_EXC_TLB_MOD	1
#define MACH_EXC_TLB_LD_MISS	2
#define MACH_EXC_TLB_ST_MISS	3
#define MACH_EXC_ADDR_ERR_LD	4
#define MACH_EXC_ADDR_ERR_ST	5
#define MACH_EXC_BUS_ERR_IFETCH	6
#define MACH_EXC_BUS_ERR_LD_ST	7
#define MACH_EXC_SYSCALL	8
#define MACH_EXC_BREAK		9
#define MACH_EXC_RES_INST	10
#define MACH_EXC_COP_UNUSABLE	11
#define MACH_EXC_OVFLOW		12
#define MACH_EXC_MAX		12

/*
 * The bits in the status register.  All bits are active when set to 1.
 *
 *	MACH_SR_CO_USABILITY	Control the usability of the four coprocessors.
 *	MACH_SR_BOOT_EXC_VEC	Use alternate exception vectors.
 *	MACH_SR_TLB_SHUTDOWN	TLB disabled.
 *	MACH_SR_PARITY_ERR	Parity error.
 *	MACH_SR_CACHE_MISS	Most recent D-cache load resulted in a miss.
 *	MACH_SR_PARITY_ZERO	Zero replaces outgoing parity bits.
 *	MACH_SR_SWAP_CACHES	Swap I-cache and D-cache.
 *	MACH_SR_ISOL_CACHES	Isolate D-cache from main memory.
 *	MACH_SR_INT_MASK	Which of the 8 interrupts are enabled.
 *	MACH_SR_KU_OLD		Old kernel/user mode bit. 1 => user mode.
 *	MACH_SR_INT_ENA_OLD	Old interrupt enable bit.
 *	MACH_SR_KU_PREV		Previous kernel/user mode bit. 1 => user mode.
 *	MACH_SR_INT_ENA_PREV	Previous interrupt enable bit.
 *	MACH_SR_KU_CUR		Current kernel/user mode bit. 1 => user mode.
 *	MACH_SR_INT_ENA_CUR	Current interrupt enable bit.
 */
#define MACH_SR_COP_USABILITY	0xf0000000
#define MACH_SR_COP_0_BIT	0x10000000
#define MACH_SR_COP_1_BIT	0x20000000
#define MACH_SR_BOOT_EXC_VEC	0x00400000
#define MACH_SR_TLB_SHUTDOWN	0x00200000
#define MACH_SR_PARITY_ERR	0x00100000
#define MACH_SR_CACHE_MISS	0x00080000
#define MACH_SR_PARITY_ZERO	0x00040000
#define MACH_SR_SWAP_CACHES	0x00020000
#define MACH_SR_ISOL_CACHES	0x00010000
#define MACH_SR_INT_MASK	0x0000ff00
#define MACH_SR_KU_OLD		0x00000020
#define MACH_SR_INT_ENA_OLD	0x00000010
#define MACH_SR_KU_PREV		0x00000008
#define MACH_SR_INT_ENA_PREV	0x00000004
#define MACH_SR_KU_CUR		0x00000002
#define MACH_SR_INT_ENA_CUR	0x00000001

/*
 * The interrupt masks.  If a bit in the mask is 1 then the interrupt is
 * enabled.
 */
#define MACH_INT_MASK_5		0x8000
#define MACH_INT_MASK_4		0x4000
#define MACH_INT_MASK_3		0x2000
#define MACH_INT_MASK_2		0x1000
#define MACH_INT_MASK_1		0x0800
#define MACH_INT_MASK_0		0x0400
#define MACH_KERN_INT_MASK	0xfc00
#define MACH_SOFT_INT_MASK_1	0x0200
#define MACH_SOFT_INT_MASK_0	0x0100
#define MACH_ALL_INT_ENABLED	0xff00

/*
 * The system control status register.
 */
#define MACH_CSR_MONO		0x0800
#define MACH_CSR_MEM_ERR	0x0400
#define	MACH_CSR_VINT		0x0200

/*
 * The bits in the context register.
 */
#define MACH_CNTXT_PTE_BASE	0xFFE00000
#define MACH_CNTXT_BAD_VPN	0x001FFFFC

/*
 * The fields in the processor revision identifier register.
 */
#define MACH_PRID_IMP		0x0000FF00
#define MACH_PRID_REV		0x000000FF

/*
 * Location of exception vectors.
 */
#define MACH_RESET_EXC_VEC	0xBFC00000
#define MACH_UTLB_MISS_EXC_VEC	0x80000000
#define MACH_GEN_EXC_VEC	0x80000080

/*
 * Offsets of the fields in the Mach_State structure.
 */
#define	MACH_USER_PC_OFFSET		0
#define MACH_TRAP_REGS_OFFSET		(MACH_USER_PC_OFFSET + 4)
#define MACH_FP_REGS_OFFSET		(MACH_TRAP_REGS_OFFSET + (4 * MACH_NUM_GPRS))
#define	MACH_FP_SR_OFFSET		(MACH_FP_REGS_OFFSET + (4 * MACH_NUM_FPRS))
#define MACH_TRAP_MULT_LO_OFFSET	(MACH_FP_SR_OFFSET + 4)
#define MACH_TRAP_MULT_HI_OFFSET	(MACH_TRAP_MULT_LO_OFFSET + 4)
#define MACH_TRAP_UNIX_RET_VAL_OFFSET	(MACH_TRAP_MULT_HI_OFFSET + 4)
#define	MACH_SWITCH_REGS_OFFSET		(MACH_TRAP_UNIX_RET_VAL_OFFSET + 4 + 4)
#define	MACH_KERN_STACK_START_OFFSET	(MACH_SWITCH_REGS_OFFSET + (4 * MACH_NUM_GPRS) + (4 * MACH_NUM_FPRS) + 4 + 8)
#define MACH_KERN_STACK_END_OFFSET	(MACH_KERN_STACK_START_OFFSET + 4)
#define	MACH_SSTEP_INST_OFFSET		(MACH_KERN_STACK_END_OFFSET + 4)
#define MACH_TLB_HIGH_ENTRY_OFFSET	(MACH_SSTEP_INST_OFFSET + 4)
/*
 * Constants for setting up the TLB entries.  This code depends
 * implictly upon MACH_KERN_STACK_PAGES.
 */
#define MACH_TLB_LOW_ENTRY_1_OFFSET	(MACH_TLB_HIGH_ENTRY_OFFSET + 4)
#define MACH_TLB_LOW_ENTRY_2_OFFSET	(MACH_TLB_LOW_ENTRY_1_OFFSET + 4)
#define MACH_TLB_LOW_ENTRY_3_OFFSET	(MACH_TLB_LOW_ENTRY_2_OFFSET + 4)
#define MACH_STATE_SIZE			(MACH_TLB_LOW_ENTRY_3_OFFSET + 4)

/*
 * Offsets into the debug state struct.
 */
#define MACH_DEBUG_REGS_OFFSET		0
#define MACH_DEBUG_FP_REGS_OFFSET	(MACH_NUM_GPRS * 4)
#define MACH_DEBUG_SIG_OFFSET		(MACH_DEBUG_FP_REGS_OFFSET + MACH_NUM_FPRS * 4)
#define MACH_DEBUG_EXC_PC_OFFSET	(MACH_DEBUG_SIG_OFFSET + 32 * 4)
#define MACH_DEBUG_CAUSE_REG_OFFSET	(MACH_DEBUG_EXC_PC_OFFSET + 4)
#define MACH_DEBUG_MULT_HI_OFFSET	(MACH_DEBUG_CAUSE_REG_OFFSET + 4)
#define MACH_DEBUG_MULT_LO_OFFSET	(MACH_DEBUG_MULT_HI_OFFSET + 4)
#define MACH_DEBUG_FPC_CSR_REG_OFFSET	(MACH_DEBUG_MULT_LO_OFFSET + 4)
#define MACH_DEBUG_FPC_EIR_REG_OFFSET	(MACH_DEBUG_FPC_CSR_REG_OFFSET + 4)
#define MACH_DEBUG_TRAP_CAUSE_OFFSET	(MACH_DEBUG_FPC_EIR_REG_OFFSET + 4)
#define MACH_DEBUG_TRAP_INFO_OFFSET	(MACH_DEBUG_TRAP_CAUSE_OFFSET + 4)
#define	MACH_DEBUG_TLB_INDEX_OFFSET	(MACH_DEBUG_TRAP_INFO_OFFSET + 4)
#define MACH_DEBUG_TLB_RANDOM_OFFSET	(MACH_DEBUG_TLB_INDEX_OFFSET + 4)
#define MACH_DEBUG_TLB_LOW_OFFSET	(MACH_DEBUG_TLB_RANDOM_OFFSET + 4)
#define MACH_DEBUG_TLB_CONTEXT_OFFSET	(MACH_DEBUG_TLB_LOW_OFFSET + 4)
#define MACH_DEBUG_BAD_VADDR_OFFSET	(MACH_DEBUG_TLB_CONTEXT_OFFSET + 4)
#define MACH_DEBUG_TLB_HI_OFFSET	(MACH_DEBUG_BAD_VADDR_OFFSET + 4)
#define MACH_DEBUG_STATUS_REG_OFFSET	(MACH_DEBUG_TLB_HI_OFFSET + 4)

/*
 * Coprocessor 0 registers:
 *
 *	MACH_COP_0_TLB_INDEX	TLB index.
 *	MACH_COP_0_TLB_RANDOM	TLB random.
 *	MACH_COP_0_TLB_LOW	TLB entry low.
 *	MACH_COP_0_TLB_CONTEXT	TLB context.
 *	MACH_COP_0_BAD_VADDR	Bad virtual address.
 *	MACH_COP_0_TLB_HI	TLB entry high.
 *	MACH_COP_0_STATUS_REG	Status register.
 *	MACH_COP_0_CAUSE_REG	Exception cause register.
 *	MACH_COP_0_EXC_PC	Exception PC.
 *	MACH_COP_0_PRID		Processor revision identifier.
 */
#define MACH_COP_0_TLB_INDEX	$0
#define MACH_COP_0_TLB_RANDOM	$1
#define MACH_COP_0_TLB_LOW	$2
#define MACH_COP_0_TLB_CONTEXT	$4
#define MACH_COP_0_BAD_VADDR	$8
#define MACH_COP_0_TLB_HI	$10
#define MACH_COP_0_STATUS_REG	$12
#define MACH_COP_0_CAUSE_REG	$13
#define MACH_COP_0_EXC_PC	$14
#define MACH_COP_0_PRID		$15

/*
 * Return codes from Mach_Trap.
 *
 *   MACH_OK		The trap was handled successfully.
 *   MACH_KERN_ERROR	The trap could not be handled so the debugger must be
 *			called.
 *   MACH_USER_ERROR	A cross address space copy to/from user space failed
 *			because of a bad address.
 */
#define	MACH_OK			0
#define	MACH_KERN_ERROR		1
#define	MACH_USER_ERROR		2

/*
 * Values for the code field in a break instruction.
 */
#define MACH_BREAK_CODE_FIELD	0x03ffffc0
#define	MACH_BREAKPOINT_VAL	0
#define MACH_SIG_RET_VAL	0x00010000
#define MACH_SSTEP_VAL		0x00020000

/*
 * Constants to differentiate between a breakpoint trap and all others.
 */
#define MACH_OTHER_TRAP_TYPE	0
#define MACH_BRKPT_TRAP		1

/*
 * MACH_KERN_START	The address where the kernel image is loaded at.
 * MACH_CODE_START	The address where the kernel code is loaded at.
 * MACH_STACK_BOTTOM	The address of the bottom of the kernel stack for the
 *			main process that is initially run.
 * MACH_KERN_END	The address where the last kernel virtual address is
 *			at.
 * MACH_KERN_STACK_PAGES Number of pages in a kernel stack.  This is used
 *			to allocate TLB entries.  There are some
 *			hard-coded dependencies on this as well, so
 *			grep for MACH_KERN_STACK_PAGES in *.[cs].  The
 *			last page is not mapped, so there should be
 *			MACH_KERN_STACK_PAGES-1 tlb low entries, etc.
 * MACH_KERN_STACK_SIZE Number of bytes in a kernel stack.
 * MACH_BARE_STACK_OFFSET	Offset of where a bare kernel stack starts.
 *				It doesn't start at the very top because
 *				the debugger requires a couple of integers
 *				of padding on the top.
 * MAGIC		A magic number which is pushed onto the stack before
 *			a context switch.  Used to verify that the stack
 *			doesn't get trashed.
 */
#define	MACH_KERN_START		0x80000000
#define	MACH_CODE_START		0x80030000
#define	MACH_KERN_STACK_PAGES	4
#define	MACH_KERN_STACK_SIZE	(MACH_KERN_STACK_PAGES * VMMACH_PAGE_SIZE)
#define	MACH_STACK_BOTTOM	(MACH_CODE_START - MACH_KERN_STACK_SIZE)
#define MACH_KERN_END		(VMMACH_VIRT_CACHED_START + 16 * 1024 * 1024)
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
#define	MACH_LAST_USER_STACK_PAGE	(VMMACH_USER_MAPPING_BASE_PAGE - VMMACH_USER_SHARED_PAGES- 1)
#define	MACH_MAX_USER_STACK_ADDR	(VMMACH_USER_MAPPING_BASE_ADDR - VMMACH_USER_SHARED_PAGES*VMMACH_PAGE_SIZE)

/*
 * Maximum number of processors configurable.
 */
#define	MACH_MAX_NUM_PROCESSORS		1

/*
 * The number of general purpose and floating point registers.
 */
#define	MACH_NUM_GPRS	32
#define	MACH_NUM_FPRS	32

/*
 * The indices of all of the registers in the standard 32 register array.
 */
#define ZERO	0
#define AST	1
#define V0	2
#define V1	3
#define A0	4
#define A1	5
#define A2	6
#define A3	7
#define T0	8
#define T1	9
#define T2	10
#define T3	11
#define T4	12
#define T5	13
#define T6	14
#define T7	15
#define S0	16
#define S1	17
#define S2	18
#define S3	19
#define S4	20
#define S5	21
#define S6	22
#define S7	23
#define T8	24
#define T9	25
#define K0	26
#define K1	27
#define GP	28
#define SP	29
#define S8	30
#define RA	31

/*
 * Magic number for system calls to differentiate between Sprite and UNIX
 * system calls.
 */
#define MACH_SYSCALL_MAGIC	0xbab1fade

/*
 * Mininum and maximum cache sizes.
 */
#define MACH_MIN_CACHE_SIZE	(16 * 1024)
#define MACH_MAX_CACHE_SIZE	(64 * 1024)

/*
 * The floating point status register.
 */
#define	MACH_FPC_CSR	$31

/*
 * Indices in the TLB where we store the stack TLB entries.
 * This is implicitly dependent on MACH_KERN_STACK_PAGES and must
 * have MACH_KERN_STACK_PAGES-1 defines, used in machAsm.s.
 */
#define MACH_STACK_TLB_INDEX_1	0x100
#define MACH_STACK_TLB_INDEX_2	0x200
#define MACH_STACK_TLB_INDEX_3	0x300

/*
 * The standard amount of space that we have to leave on a stack frame when
 * we create it.
 */
#define MACH_STAND_FRAME_SIZE	24

/*
 * The floating point coprocessor status register bits.
 */
#define MACH_FPC_ROUNDING_BITS		0x00000003
#define MACH_FPC_STICKY_BITS		0x0000007c
#define MACH_FPC_TRAP_ENABLE_BITS	0x00000f80
#define MACH_FPC_EXCEPTION_BITS		0x0003f000
#define MACH_FPC_COND_BIT		0x00800000

/*
 * UNIX signal numbers to translate into Sprite signal numbers and codes.
 * This is so I can steal the code for softfp.
 */
#define MACH_SIGFPE		1
#define MACH_SIGILL		2

/*
 * Constants to determine if have a floating point instruction.
 */
#define MACH_OPCODE_SHIFT	26
#define MACH_OPCODE_C1		0x11

/*
 * Special UNIX system calls.
 */
#define MACH_UNIX_SIG_RETURN		103
#define MACH_UNIX_LONG_JUMP_RETURN	139

#endif /* _MACHCONST */
