/*
 * machConst.h --
 *
 *	Machine dependent constants.  Most of the constants are defined in
 *	the "SPUR Memory System Architecture" (SPUR-MSA) tech report
 *	and the "SPUR Instruction Set Architecture" (SPUR-ISA) report.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHCONST
#define _MACHCONST

/*
 * The compare trap types.
 */
#define	MACH_SYS_CALL_TRAP		0
#define	MACH_SIG_RETURN_TRAP		1
#define	MACH_GET_WIN_MEM_TRAP		2
#define	MACH_BREAKPOINT_TRAP		3
#define	MACH_USER_FPU_EXCEPT_TRAP	4
#define	MACH_USER_ILLEGAL_TRAP		5
#define	MACH_USER_FIXNUM_TRAP		6
#define	MACH_USER_OVERFLOW_TRAP		7
#define	MACH_USER_BAD_SWP_TRAP		8
#define	MACH_MAX_TRAP_TYPE		8

/*
 * The return codes from the C trap handler routine:
 *
 *	MACH_NORM_RETURN	No special action is required before continuing
 *				execution.
 *	MACH_FAILED_COPY	A cross-address space copy failed.
 *	MACH_FAILED_ARG_FETCH	A system call argument fetch failed.
 */
#define	MACH_NORM_RETURN		0
#define	MACH_FAILED_COPY		1
#define	MACH_FAILED_ARG_FETCH		2
#define	MACH_MAX_RETURN_CODE		2

/*
 * The different type of trap handler errors.  Note that the error types 
 * start at one more than the maximum return code.  Thus when a C trap
 * handler returns a value that is greater than the maximum return code
 * the assembly language trap handler knows that it is an error.
 *
 *	MACH_USER_FPU_EXCEPT	An fpu exception in user mode.
 *	MACH_KERN_FPU_EXCEPT	An fpu exception in kernel mode.
 *	MACH_USER_ILLEGAL	An illegal exception in user mode.
 *	MACH_KERN_ILLEGAL	An illegal exception in kernel mode.
 *	MACH_USER_FIXNUM	A fixnum exception in user mode.
 *	MACH_KERN_FIXNUM	A fixnum exception in kernel mode.
 *	MACH_USER_OVERFLOW	An overflow exception in user mode.
 *	MACH_KERN_OVERFLOW	An overflow exception in kernel mode.
 *	MACH_ERROR		A SPUR error exception.
 *	MACH_BAD_FAULT		One of the fault types that we can't
 *				handle.
 *	MACH_BAD_SRC_REG	An invalid src register in a dissassembled
 *				instruction.
 *	MACH_USER_BAD_SWP	A user process had a bogus swp when it trapped
 *				into the kernel.
 *	MACH_BAD_TRAP_TYPE	An unknown compare trap type.
 *	MACH_KERN_ACCESS_VIOL	An access violation fault occured in the
 *				kernel.
 *	MACH_BAD_SYS_CALL	A bad system call type was passed in.
 *	MACH_BREAKPOINT		A process called the debugger.
 */
#define	MACH_USER_FPU_EXCEPT		(MACH_MAX_RETURN_CODE + 1)
#define	MACH_KERN_FPU_EXCEPT		(MACH_MAX_RETURN_CODE + 2)
#define	MACH_USER_ILLEGAL		(MACH_MAX_RETURN_CODE + 3)
#define	MACH_KERN_ILLEGAL		(MACH_MAX_RETURN_CODE + 4)
#define	MACH_USER_FIXNUM		(MACH_MAX_RETURN_CODE + 5)
#define	MACH_KERN_FIXNUM		(MACH_MAX_RETURN_CODE + 6)
#define	MACH_USER_OVERFLOW		(MACH_MAX_RETURN_CODE + 7)
#define	MACH_KERN_OVERFLOW		(MACH_MAX_RETURN_CODE + 8)
#define	MACH_ERROR			(MACH_MAX_RETURN_CODE + 9)
#define	MACH_BAD_FAULT			(MACH_MAX_RETURN_CODE + 10)
#define	MACH_BAD_SRC_REG		(MACH_MAX_RETURN_CODE + 11)
#define	MACH_USER_BAD_SWP		(MACH_MAX_RETURN_CODE + 12)
#define	MACH_BAD_TRAP_TYPE		(MACH_MAX_RETURN_CODE + 13)
#define	MACH_KERN_ACCESS_VIOL		(MACH_MAX_RETURN_CODE + 14)
#define	MACH_BAD_SYS_CALL		(MACH_MAX_RETURN_CODE + 15)
#define	MACH_BREAKPOINT			(MACH_MAX_RETURN_CODE + 16)

/*
 * The size of a single saved window and all of the saved windows.
 */
#define	MACH_SAVED_WINDOW_SIZE		128
#define	MACH_SAVED_REG_SET_SIZE		(MACH_NUM_WINDOWS * MACH_SAVED_WINDOW_SIZE)

/*
 * The number of registers.
 */
#define	MACH_NUM_GLOBAL_REGS		10
#define	MACH_NUM_REGS_PER_WINDOW	16
#define	MACH_NUM_ACTIVE_REGS		32
#define	MACH_NUM_WINDOWS		8
#define MACH_TOTAL_REGS			(MACH_NUM_GLOBAL_REGS + \
					 MACH_NUM_REGS_PER_WINDOW * \
					 MACH_NUM_WINDOWS)

/*
 * Error status bits in the FEStatus register (see page 32 of SPUR-MSA)
 */
#define	MACH_ERROR_SYSTEM_RESET		0x001
#define	MACH_ERROR_BUS_RESET		0x002
#define	MACH_ERROR_TRY_AGAIN		0x004
#define	MACH_ERROR_TIMEOUT		0x008
#define	MACH_ERROR_BUS_ERR_INCONSIST	0x010
#define	MACH_ERROR_BUS_ERR_TIMEOUT	0x040
#define	MACH_ERROR_BUS_ERR_CONSIST	0x080
#define	MACH_ERROR_SBC_INCONSIST	0x100
#define	MACH_ERROR_BAD_SBC_REQ		0x200
#define	MACH_ERROR_BAD_SBC_ACK		0x400
#define	MACH_ERROR_IGNORE_FAULT		0x800

/*
 * Fault status bits in the FEStatus register (see page 31 of SPUR-MSA).
 */
#define	MACH_FAULT_PROTECTION		0x010000
#define	MACH_FAULT_PAGE_FAULT		0x020000
#define	MACH_FAULT_REF_BIT		0x040000
#define	MACH_FAULT_DIRTY_BIT		0x080000
#define	MACH_FAULT_BAD_SUB_OP		0x100000
#define	MACH_FAULT_TRY_AGAIN		0x200000
#define	MACH_FAULT_ILL_CACHE_OP		0x400000

/*
 * The VM fault bits.
 */
#define	MACH_VM_FAULT_PROTECTION	0x1
#define	MACH_VM_FAULT_PAGE_FAULT	0x2
#define	MACH_VM_FAULT_REF_BIT		0x4
#define	MACH_VM_FAULT_DIRTY_BIT		0x8

/*
 * Interrupt Status/Mask Register Assignments (see page 30 of SPUR-MSA).
 */
#define	MACH_NUM_INTR_TYPES		20
#define	MACH_EXT_INTR_0			0x00001
#define	MACH_EXT_INTR_1			0x00002
#define	MACH_EXT_INTR_2			0x00004
#define	MACH_EXT_INTR_3			0x00008
#define	MACH_EXT_INTR_4			0x00010
#define	MACH_EXT_INTR_5			0x00020
#define	MACH_EXT_INTR_6			0x00040
#define	MACH_EXT_INTR_7			0x00080
#define	MACH_EXT_INTR_8			0x00100
#define	MACH_EXT_INTR_9			0x00200
#define	MACH_EXT_INTR_10		0x00400
#define	MACH_EXT_INTR_11		0x00800
#define	MACH_EXT_INTR_12		0x01000
#define	MACH_EXT_INTR_13		0x02000
#define	MACH_EXT_INTR_14		0x04000
#define	MACH_EXT_INTR_15		0x08000
#define	MACH_TIMER_T1_INTR		0x10000
#define	MACH_TIMER_T2_INTR		0x20000
#define	MACH_UART_INTR			0x40000
#define	MACH_AUX_INTR			0x80000

/*
 * Bits in the KPSW (see SPUR-ISA page 36).
 */
#define	MACH_KPSW_PREFETCH_ENA		0x00004
#define	MACH_KPSW_IBUFFER_ENA		0x00008
#define	MACH_KPSW_VIRT_DFETCH_ENA	0x00010
#define	MACH_KPSW_VIRT_IFETCH_ENA	0x00020
#define	MACH_KPSW_CUR_MODE		0x00040
#define	MACH_KPSW_PREV_MODE		0x00080
#define	MACH_KPSW_INTR_TRAP_ENA		0x00100
#define	MACH_KPSW_FAULT_TRAP_ENA	0x00200
#define	MACH_KPSW_ERROR_TRAP_ENA	0x00400
#define	MACH_KPSW_ALL_TRAPS_ENA		0x00800

/*
 * Sprite defined bits in the kpsw.
 */
#define	MACH_KPSW_USE_CUR_PC		0x10000
#define	MACH_KPSW_USE_NEXT_PC		0x20000

/*
 * Bits in UPSW (see SPUR-ISA page 36).
 */
#define	MACH_UPSW_OVERFLOW_ENA		0x04
#define	MACH_UPSW_GENERATION_ENA	0x08
#define	MACH_UPSW_TAG_EXCEPT_ENA	0x10
#define	MACH_UPSW_FPU_EXECPT_ENA	0x20
#define	MACH_UPSW_FPU_ENABLE_ENA	0x40
#define	MACH_UPSW_FPU_PARALLEL_ENA	0x80

/*
 * Cache controller registers (see page 22 of SPUR-MSA).
 */
#define	MACH_GSN_0		0x0080
#define	MACH_GSN_1		0x0180
#define	MACH_GSN_2		0x0280
#define	MACH_GSN_3		0x0380
#define MACH_RPTM_0		0x0000
#define MACH_RPTM_1		0x0100
#define MACH_RPTM_2		0x0200
#define MACH_RPTM_3		0x0300
#define	MACH_RPTM_02		0x0060
#define	MACH_RPTM_12		0x0160
#define	MACH_RPTM_22		0x0260
#define	MACH_RPTM_32		0x0360
#define	MACH_RPTM_01		0x0040
#define	MACH_RPTM_11		0x0140
#define	MACH_RPTM_21		0x0240
#define	MACH_RPTM_31		0x0340
#define	MACH_RPTM_00		0x0020
#define	MACH_RPTM_10		0x0120
#define	MACH_RPTM_20		0x0220
#define	MACH_RPTM_30		0x0320
#define	MACH_GVA_0		0x0400
#define	MACH_GVA_1		0x0420
#define	MACH_GVA_2		0x0440
#define	MACH_GVA_3		0x0460
#define	MACH_GVA_4		0x0480
#define	MACH_PTEVA_0		0x0500
#define	MACH_PTEVA_1		0x0520
#define	MACH_PTEVA_2		0x0540
#define	MACH_PTEVA_3		0x0560
#define	MACH_PTEVA_4		0x0580
#define	MACH_RPTEVA_0		0x0600
#define	MACH_RPTEVA_1		0x0620
#define	MACH_RPTEVA_2		0x0640
#define	MACH_RPTEVA_3		0x0660
#define	MACH_RPTEVA_4		0x0680
#define	MACH_G_0		0x0700
#define	MACH_G_1		0x0720
#define	MACH_G_2		0x0740
#define	MACH_G_3		0x0760
#define	MACH_G4			0x0780
#define	MACH_T_00		0x0800
#define	MACH_T_01		0x0820
#define	MACH_T_02		0x0840
#define	MACH_T_03		0x0860
#define	MACH_T_04		0x0900
#define	MACH_T_05		0x0920
#define	MACH_T_06		0x0940
#define	MACH_T_07		0x0960
#define	MACH_T_10		0x0A00
#define	MACH_T_11		0x0A20
#define	MACH_T_12		0x0A40
#define	MACH_T_13		0x0A60
#define	MACH_T_20		0x0B00
#define	MACH_T_21		0x0B20
#define	MACH_T_22		0x0B40
#define	MACH_T_23		0x0B60
#define	MACH_INTR_STATUS_0	0x0C00
#define	MACH_INTR_STATUS_1	0x0C20
#define	MACH_INTR_STATUS_2	0x0C40
#define	MACH_INTR_STATUS_3	0x0C60
#define	MACH_INTR_MASK_0	0x0D00
#define	MACH_INTR_MASK_1	0x0D20
#define	MACH_INTR_MASK_2	0x0D40
#define	MACH_INTR_MASK_3	0x0D60
#define	MACH_FE_STATUS_0	0x0E00
#define	MACH_FE_STATUS_1	0x0E20
#define	MACH_FE_STATUS_2	0x0E40
#define	MACH_FE_STATUS_3	0x0E60
#define	MACH_MODE_REG		0x0F20
#define	MACH_SLOT_ID_REG	0x0F00

/*
 * Cache sup-op Codes (page 21 of SPUR-MSA).
 */
#define	MACH_CO_RESET		0x04
#define	MACH_CO_RD_REG		0x08
#define	MACH_CO_WR_REG		0x0C
#define	MACH_CO_FLUSH		0x10

/*
 * Offset into the register state struct.
 */
#define	MACH_REG_STATE_REGS_OFFSET	0
#define	MACH_REG_STATE_KPSW_OFFSET	(MACH_REG_STATE_REGS_OFFSET + MACH_NUM_ACTIVE_REGS * 8)
#define	MACH_REG_STATE_UPSW_OFFSET	(MACH_REG_STATE_KPSW_OFFSET + 4)
#define	MACH_REG_STATE_CUR_PC_OFFSET	(MACH_REG_STATE_UPSW_OFFSET + 4)
#define	MACH_REG_STATE_NEXT_PC_OFFSET	(MACH_REG_STATE_CUR_PC_OFFSET + 4)
#define	MACH_REG_STATE_INSERT_OFFSET	(MACH_REG_STATE_NEXT_PC_OFFSET + 4)
#define	MACH_REG_STATE_SWP_OFFSET	(MACH_REG_STATE_INSERT_OFFSET + 4)
#define	MACH_REG_STATE_CWP_OFFSET	(MACH_REG_STATE_SWP_OFFSET + 4)

/*
 * Offsets into the process state structure "Mach_State".
 *
 * First: trapRegsState
 */

#define	MACH_TRAP_REG_STATE_OFFSET	0
#define	MACH_TRAP_REGS_OFFSET		(MACH_TRAP_REG_STATE_OFFSET)
#define	MACH_TRAP_KPSW_OFFSET		(MACH_TRAP_REGS_OFFSET + MACH_NUM_ACTIVE_REGS * 8)
#define	MACH_TRAP_UPSW_OFFSET		(MACH_TRAP_KPSW_OFFSET + 4)
#define	MACH_TRAP_CUR_PC_OFFSET		(MACH_TRAP_UPSW_OFFSET + 4)
#define	MACH_TRAP_NEXT_PC_OFFSET	(MACH_TRAP_CUR_PC_OFFSET + 4)
#define	MACH_TRAP_INSERT_OFFSET		(MACH_TRAP_NEXT_PC_OFFSET + 4)
#define	MACH_TRAP_SWP_OFFSET		(MACH_TRAP_INSERT_OFFSET + 4)
#define	MACH_TRAP_CWP_OFFSET		(MACH_TRAP_SWP_OFFSET + 4)
/*
 * Other misc. fields of the user state structure.
 */
#define	MACH_MIN_SWP_OFFSET		(MACH_TRAP_CWP_OFFSET + 4)
#define	MACH_MAX_SWP_OFFSET		(MACH_MIN_SWP_OFFSET + 4)
#define	MACH_NEW_CUR_PC_OFFSET		(MACH_MAX_SWP_OFFSET + 4)
#define	MACH_SIG_NUM_OFFSET		(MACH_NEW_CUR_PC_OFFSET + 4)
#define	MACH_SIG_CODE_OFFSET		(MACH_SIG_NUM_OFFSET + 4)
#define	MACH_OLD_HOLD_MASK_OFFSET	(MACH_SIG_CODE_OFFSET + 4)
/*
 * switchRegsState.
 */
#define	MACH_SWITCH_REG_STATE_OFFSET	(MACH_OLD_HOLD_MASK_OFFSET + 4)
#define	MACH_SWITCH_REGS_OFFSET		(MACH_SWITCH_REG_STATE_OFFSET)
#define	MACH_SWITCH_KPSW_OFFSET		(MACH_SWITCH_REGS_OFFSET + 256)
#define	MACH_SWITCH_UPSW_OFFSET		(MACH_SWITCH_KPSW_OFFSET + 4)
#define	MACH_SWITCH_CUR_PC_OFFSET	(MACH_SWITCH_UPSW_OFFSET + 4)
#define	MACH_SWITCH_NEXT_PC_OFFSET	(MACH_SWITCH_CUR_PC_OFFSET + 4)
#define	MACH_SWITCH_INSERT_OFFSET	(MACH_SWITCH_NEXT_PC_OFFSET + 4)
#define	MACH_SWITCH_SWP_OFFSET		(MACH_SWITCH_INSERT_OFFSET + 4)
#define	MACH_SWITCH_CWP_OFFSET		(MACH_SWITCH_SWP_OFFSET + 4)

/*
 * Kernel stack bounds.
 */
#define	MACH_KERN_STACK_START_OFFSET	(MACH_SWITCH_CWP_OFFSET + 4)
#define	MACH_KERN_STACK_END_OFFSET	(MACH_KERN_STACK_START_OFFSET + 4)

/*
 * The different register indices into the registers in the Mach_RegState
 * struct:
 *
 *	MACH_SPILL_SP		The spill stack pointer.
 *	MACH_RETURN_VAL_REG	The register where a value is returned from
 *				a procedure call.
 *	MACH_INPUT_REG1		First input parameter.
 */
#define	MACH_SPILL_SP		4
#define	MACH_RETURN_VAL_REG	27
#define	MACH_INPUT_REG1		11

/*
 * The base of the user's saved window stack is the base of the stack
 * segment.
 */
#define	MACH_SAVED_WINDOW_STACK_BASE	(3 * VMMACH_SEG_SIZE)

/*
 * Bound on the kernel's address space.
 *
 *	MACH_KERN_START		The lowest valid kernel address.
 *	MACH_KERN_END		The highest valid kernel address.
 *	MACH_CODE_START		The first page where code begins.
 *	MACH_DEBUG_STACK_BOTTOM	The bottom of the debugger's stack.
 *	MACH_STACK_BOTTOM	The bottom of the first process's stack.
 *	MACH_KERN_STACK_SIZE	The number of bytes in a kernel stack.
 */
#define	MACH_KERN_START		0
#define	MACH_KERN_END		(16 * 1024 * 1024)
#define	MACH_DEBUG_STACK_BOTTOM	0x4000
#define	MACH_STACK_BOTTOM	(MACH_DEBUG_STACK_BOTTOM + MACH_KERN_STACK_SIZE)
#define	MACH_CODE_START		(MACH_STACK_BOTTOM + MACH_KERN_STACK_SIZE)
#define	MACH_KERN_STACK_SIZE	0x4000

/*
 * Bounds on the user's address space.
 *
 *	MACH_FIRST_USER_ADDR	Lowest possible user address.
 *	MACH_LAST_USER_ADDR	Highest possible user address.  This 
 *				corresponds to the top of the stack segment.
 *				Note that it is the end of the stack minus
 *				one page so that it fits in 32 bits.
 *	MACH_LAST_USER_STACK_PAGE The last virtual page in the user's stack
 *				  segment.
 *	MACH_MAX_USER_STACK_ADDR  The highest possible value for the user
 *				  stack pointer.  
 *	MACH_SW_STACK_SIZE	The size of the user's saved window stack that
 *				lives at the top of the user stack.
 */
#define	MACH_FIRST_USER_ADDR		VMMACH_SEG_SIZE
#define	MACH_LAST_USER_ADDR		(3 * VMMACH_SEG_SIZE + (VMMACH_SEG_SIZE - VMMACH_PAGE_SIZE))
#define	MACH_LAST_USER_STACK_PAGE	((MACH_LAST_USER_ADDR - 1) / VMMACH_PAGE_SIZE)
#define	MACH_MAX_USER_STACK_ADDR	(MACH_LAST_USER_ADDR - MACH_SW_STACK_SIZE)
#define	MACH_SW_STACK_SIZE		(1024 * 1024 - VMMACH_PAGE_SIZE)

/*
 * The base of the user's saved window stack is the top of the stack segment.
 */
#define	MACH_SAVED_WINDOW_STACK_BASE	MACH_MAX_USER_STACK_ADDR

#endif _MACHCONST
