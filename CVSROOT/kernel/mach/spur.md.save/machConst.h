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
 * The hardware page size.
 */
#define	MACH_PAGE_SIZE		4096

/*
 * The number of registers.
 */
#define	MACH_NUM_GLOBAL_REGS		10
#define	MACH_NUM_REGS_PER_WINDOW	16
#define	MACH_NUM_WINDOWS		8
#define	MACH_NUM_REGS_TO_SAVE		15
#define MACH_TOTAL_REGS			(MACH_NUM_GLOBAL_REGS + \
					 MACH_NUM_REGS_PER_WINDOW * \
					 MACH_NUM_WINDOWS)

/*
 * Different user errors.
 */
#define	MACH_USER_BAD_SWP		1
#define	MACH_USER_BUS_ERROR		2
#define	MACH_USER_ILLEGAL_INST		3

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
 * Interrupt Status/Mask Register Assignments (see page 30 of SPUR-MSA).
 */
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
#define	MACH_KPSW_PREFETCH_ENA		0x0004
#define	MACH_KPSW_IBUFFER_ENA		0x0008
#define	MACH_KPSW_VIRT_DFETCH_ENA	0x0010
#define	MACH_KPSW_VIRT_IFETCH_ENA	0x0020
#define	MACH_KPSW_CUR_MODE		0x0040
#define	MACH_KPSW_PREV_MODE		0x0080
#define	MACH_KPSW_INTR_TRAP_ENA		0x0100
#define	MACH_KPSW_FAULT_TRAP_ENA	0x0200
#define	MACH_KPSW_ERROR_TRAP_ENA	0x0400
#define	MACH_KPSW_ALL_TRAPS_ENA		0x0800

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

#endif _MACHCONST
