/*
 * machAsmDefs.h --
 *
 *	Assembly language macros for the SPUR architecture.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHASMDEFS
#define _MACHASMDEFS

/*
 * LD_ADDR --
 *
 *	Load the address of the instruction specified by 'label' into
 *	register 'rd'.
 */
#define LD_ADDR(rd, label) \
	rd_special	rd, Pc ;\
0:	add_nt		rd, rd, $(((label-0b)+4)&0x3fff)

/*
 * LD_CONSTANT --
 *
 *	Load the 32 bit constant 'constant' into register 'rd'
 *
 * Side effects:
 *	Changes the insert register.
 */
#define LD_CONSTANT(rd, constant) \
	add_nt		rd, r0, $((constant)&0xff) ; \
	wr_insert	$1 ; \
	insert		rd, rd, $(((constant)>>8)&0xff) ; \
	wr_insert	$2 ; \
	insert		rd, rd, $(((constant)>>16)&0xff) ; \
	wr_insert	$3 ; \
	insert		rd, rd, $(((constant)>>24)&0xff)

/*
 * CLR_FE_STATUS --
 *
 *	Selectively clear the FEStatus register. Clear each bit in 'bits' 
 *	that is 1.
 */
#define CLR_FE_STATUS(bits) \
	add_nt          VOL_TEMP1, r0, $((bits)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_FE_STATUS_0|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>8)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_FE_STATUS_1|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>16)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_FE_STATUS_2|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>24)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_FE_STATUS_3|MACH_CO_WR_REG

/*
 * CLR_INTR_STATUS --
 *
 *	Selectively clear the IStatus register. Clear each bit in 'bits'
 *	that is 1.
 */
#define CLR_INTR_STATUS(bits) \
	add_nt          VOL_TEMP1, r0, $((bits)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_INTR_STATUS_0|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>8)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_INTR_STATUS_1|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>16)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_INTR_STATUS_2|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>24)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_INTR_STATUS_3|MACH_CO_WR_REG

/*
 * SET_INTR_MASK --
 *
 *	Set the IMask register. 
 */
#define SET_INTR_MASK(bits) \
	add_nt          VOL_TEMP1, r0, $((bits)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_INTR_MASK_0|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>8)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_INTR_MASK_1|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>16)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_INTR_MASK_2|MACH_CO_WR_REG ; \
	add_nt          VOL_TEMP1, r0, $(((bits)>>24)&0xff) ; \
	st_external     VOL_TEMP1, r0, $MACH_INTR_MASK_3|MACH_CO_WR_REG

/*
 * SET_KPSW --
 *
 *	Set bits in the kpsw.
 */
#define SET_KPSW(bits) \
	ld_constant(VOL_TEMP1, bits) ; \
	rd_kpsw		VOL_TEMP2 ; \
	or		VOL_TEMP2, VOL_TEMP1, VOL_TEMP2 ; \
	wr_kpsw		VOL_TEMP2, r0

/*
 * CLR_KPSW --
 *
 *	Clear bits in the KPSW.
 */
#define CLR_KPSW(bits) \
	ld_constant(VOL_TEMP1, bits) ; \
	add_nt		VOL_TEMP2, r0, $-1 ; \
	xor		VOL_TEMP1, VOL_TEMP2, VOL_TEMP1 ; \
	rd_kpsw		VOL_TEMP2 ; \
	and		VOL_TEMP2, VOL_TEMP1, VOL_TEMP2 ; \
	wr_kpsw		VOL_TEMP2, r0

/*
 * MOD_KPSW --
 *
 *	Change bits in the KPSW.
 */
#define MOD_KPSW(set,clr) \
	ld_constant(VOL_TEMP1, clr) ; \
	xor		VOL_TEMP1, VOL_TEMP1, VOL_TEMP1 ; \
	rd_kpsw		VOL_TEMP2 ; \
	and		VOL_TEMP2, VOL_TEMP1, VOL_TEMP2 ; \
	ld_constant(VOL_TEMP1, set) ; \
	or		VOL_TEMP2, VOL_TEMP1, VOL_TEMP2 ; \
	wr_kpsw		VOL_TEMP2, r0

/*
 * SET_UPSW --
 *
 *	Set bits in the upsw.
 */
#define SET_UPSW(bits) \
	ld_constant(VOL_TEMP1, bits) ; \
	rd_special	VOL_TEMP2, upsw ; \
	or		VOL_TEMP2, VOL_TEMP1, VOL_TEMP2 ; \
	wr_special	upsw, VOL_TEMP2, r0

/*
 * CLR_UPSW --
 *
 *	Clear bits in the upsw.
 */
#define CLR_UPSW(bits) \
	ld_constant(VOL_TEMP1, bits) ; \
	add_nt		VOL_TEMP2, r0, $-1 ; \
	xor		VOL_TEMP1, VOL_TEMP2, VOL_TEMP1 ; \
	rd_special	VOL_TEMP2, upsw ; \
	and		VOL_TEMP2, VOL_TEMP1, VOL_TEMP2 ; \
	wr_special	upsw, VOL_TEMP2, r0

/*
 * MOD_UPSW --
 *
 *	Change bits in the upsw
 */
#define MOD_UPSW(set,clr) \
	ld_constant(VOL_TEMP1, clr) ; \
	xor		VOL_TEMP1, VOL_TEMP1, VOL_TEMP1 ; \
	rd_special	VOL_TEMP2, upsw ; \
	and		VOL_TEMP2, VOL_TEMP1, VOL_TEMP2 ; \
	ld_constant(VOL_TEMP1, set) ; \
	or		VOL_TEMP2, VOL_TEMP1, VOL_TEMP2 ; \
	wr_special	upsw, VOL_TEMP2, r0

/*
 * LD_SLOT_ID(rx) -- 
 *
 *	Load slotid into register rx
 *
 *	rx --	register to load slotid into.
 */
#define LD_SLOT_ID(rx)	ld_external rx, r0, $MACH_SLOT_ID_REG|MACH_CO_RD_REG

/*
 * ST_RPTM(rx, rptm) --
 *
 *	Store to RPTM register in CC
 *
 *	rx -- 	register containing the 32 bit base address of the root page
 *	      	table physical map
 *	rptm --	address of byte 0 of specific RPTM register (e.g., RPTM0)
 *
 * rptm<19:0> <= rx<31:12>
 */
#define ST_RPTM(rx,rptm) \
	extract		VOL_TEMP1, rx, $1 ;\
	st_external	VOL_TEMP1, r0, $rptm|MACH_CO_WR_REG|0x20  ;\
	extract		VOL_TEMP2, rx, $2 ;\
	st_external	VOL_TEMP2, r0, $rptm|MACH_CO_WR_REG|0x40  ;\
	extract		VOL_TEMP2, rx, $3 ;\
	st_external	VOL_TEMP2, r0, $rptm|MACH_CO_WR_REG|0x60 

/*
 *
 * ST_GSN(rx,gsn) --
 *
 *	Store to active segment register in CC
 *
 *	rx --	register containing the global segment number
 *	gsn --	specific GSN register (e.g., GSN0)
 *
 * gsn<7:0> <= rx<7:0>
 */
#define ST_GSN(rx,gsn) st_external	rx, r0, $gsn|MACH_CO_WR_REG

/*
 * ST_PT_BASE(rx) --
 *
 *	Store to high-order bits of PTEVA in the CC
 *
 *	rx -- 	Virtual pagenumber which is base of the page table.
 *
 * pteva<37:28> <= rx<25:16>
 */
#define ST_PT_BASE(rx) \
	sra		VOL_TEMP1, rx, $1 ;\
	sra		VOL_TEMP1, VOL_TEMP1, $1 ;\
	extract		VOL_TEMP2, VOL_TEMP1, $2 ;\
	st_external	VOL_TEMP2, r0, $MACH_PTEVA_4|MACH_CO_WR_REG ;\
	sra		VOL_TEMP1, VOL_TEMP1, $1 ;\
	sra		VOL_TEMP1, VOL_TEMP1, $1 ;\
	extract		VOL_TEMP2, VOL_TEMP1, $1 ;\
	st_external	VOL_TEMP2, r0, $MACH_PTEVA_3|MACH_CO_WR_REG

/*
 * ST_RPT_BASE(rx) -- 
 *
 *	Store to high-order bits of RPTEVA in the CC
 *
 *	rx -- 	Virtual pagenumber which is base of the page table.
 *
 * rpteva<37:28> <= rx<25:16>
 * rpteva<27:18> <= rx<25:16>
 */
#define ST_RPT_BASE(rx) \
	sra		VOL_TEMP1, rx, $1 ;\
	sra		VOL_TEMP1, VOL_TEMP1, $1 ;\
	extract		VOL_TEMP2, VOL_TEMP1, $2 ;\
	st_external	VOL_TEMP2, r0, $MACH_RPTEVA_4|MACH_CO_WR_REG ;\
	wr_insert	$1 ;\
	insert		VOL_TEMP1, rx, VOL_TEMP2 ;\
	sll		VOL_TEMP1, VOL_TEMP1, $2 ;\
	extract		VOL_TEMP2, VOL_TEMP1, $2 ;\
	st_external	VOL_TEMP2, r0, $MACH_RPTEVA_2|MACH_CO_WR_REG  ;\
	sll		VOL_TEMP1, VOL_TEMP1, $2 ;\
	extract		VOL_TEMP2, VOL_TEMP1, $2 ;\
	st_external	VOL_TEMP2, r0, $MACH_RPTEVA_3|MACH_CO_WR_REG 

/*
 * JUMP_VIRTUAL(rx) --  
 *	
 *	Switch from fetching instructions from the physical address space
 *	to the virtual address space.
 *
 *	rx -- 	register containing the virtual address to begin execution.
 *
 * Side Effects:
 *	1) IB is disabled. VOL_TEMP1 contains the previous value of the
 *	   K_IBuffer
 *	   bit.
 *	2) The INVALIDATE_IB instruction MUST be executed before re-enabling
 *	   the IBuffer.
 */
#define JUMP_VIRTUAL(rx) \
	rd_kpsw		VOL_TEMP1 ;\
	and		VOL_TEMP2, VOL_TEMP1, $~MACH_KPSW_IBUFFER_ENA ;\
	and 		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_IBUFFER_ENA ;\
	or		VOL_TEMP2, VOL_TEMP2, $MACH_KPSW_VIRT_IFETCH_ENA ;\
	jump_reg	rx, r0 ;\
	wr_kpsw		VOL_TEMP2, r0


/*
 * REENABLE_IB -- 
 *
 *	Re-enable the IB after executing a 'jump_virtual' macro
 *
 *	Assumptions:
 *		VOL_TEMP1 contains the previous value of the K_IBuffer bit.
 */
#define REENABLE_IB() \
	invalidate_ib ;\
	rd_kpsw		VOL_TEMP2 ;\
	or		VOL_TEMP2, VOL_TEMP2, VOL_TEMP1 ;\
	wr_kpsw		VOL_TEMP2, r0

/*
 * READ_STATUS_REGS(baseVal, resReg)
 *
 *	The the value out of the interrupt status, interrupt mask and
 *	fe status registers.
 */
#define READ_CACHE_REG(baseVal, resReg) \
	rd_insert	VOL_TEMP1; \
	ld_external	resReg, r0, $baseVal|MACH_CO_RD_REG; \
	ld_external	VOL_TEMP2, r0, $baseVal|0x20|MACH_CO_RD_REG; \
	wr_insert	$1; \
	insert		resReg, resReg, VOL_TEMP2; \
	ld_external	VOL_TEMP2, r0, $baseVal|0x40|MACH_CO_RD_REG; \
	wr_insert	$2; \
	insert		resReg, resReg, VOL_TEMP2; \
	ld_external	VOL_TEMP2, r0, $baseVal|0x60|MACH_CO_RD_REG; \
	wr_insert	$3; \
	insert		resReg, resReg, VOL_TEMP2; \
	wr_insert	VOL_TEMP1

/*
 * VERIFY_SWP(label, underFlowBytes) -- 
 *
 *	Verify that the saved window pointer is valid.  If so branch to the
 *	given label.  The swp is valid if
 *
 *	    swp >= min_swp_offset - MACH_SAVED_WINDOW_SIZE and
 *	    swp + MACH_OVERFLOW_EXTRA <= max_swp_offset
 *
 *	These bounds ensure that a single window underflow will work and many 
 *	overflows will work.
 *
 *	underFlowBytes -- Used by the window overflow to indicate that the
 *			  lower bound should be min_swp_offset instead of
 *			  min_swp_offset - MACH_SAVED_WINDOW_SIZE.
 */
#define	VERIFY_SWP(label) \
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr; \
	rd_special	VOL_TEMP2, swp; \
	ld_32		VOL_TEMP3, VOL_TEMP1, $MACH_MIN_SWP_OFFSET; \
	Nop; \
	sub_nt		VOL_TEMP3, VOL_TEMP3, $(MACH_SAVED_WINDOW_SIZE - underFlowBytes); \
	cmp_br_delayed	lt, VOL_TEMP2, VOL_TEMP3, 1f; \
	Nop; \
	ld_32		VOL_TEMP3, VOL_TEMP1, $MACH_MAX_SWP_OFFSET; \
	add_nt		VOL_TEMP2, VOL_TEMP2, $MACH_OVERFLOW_EXTRA; \
	cmp_br_delayed	le, VOL_TEMP2, VOL_TEMP3, goodLabel; \
	Nop; \
1:

/*
 * SWITCH_TO_KERNEL_SPILL_STACK()
 *
 *	Switch to the kernel's spill stack while saving the old value
 *	in the process state struct.
 */
#define SWITCH_TO_KERNEL_SPILL_STACK() \
	rd_kpsw		VOL_TEMP1; \
	and		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_PREV_MODE; \
	cmp_br_delayed	eq, VOL_TEMP1, $0, 1f; \
	Nop; \
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr; \
	Nop; \
	st_32		SPILL_SP, VOL_TEMP1, $MACH_TRAP_USP_OFFSET; \
	ld_32		SPILL_SP, VOL_TEMP1, $MACH_KERN_STACK_END; \
	Nop; \
1:

/*
 * RESTORE_USER_SPILL_STACK()
 *
 *	Switch back to the user's stack.
 */
#define	RESTORE_USER_SPILL_STACK() \
	rd_kpsw		VOL_TEMP1; \
	and		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_PREV_MODE; \
	cmp_br_delayed	eq, VOL_TEMP1, $0, 1f; \
	Nop; \
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr; \
	Nop; \
	ld_32		SPILL_SP, VOL_TEMP1, $MACH_TRAP_USP_OFFSET; \
	Nop; \
1:

/*
 * USER_ERROR(errorType) --
 *
 *	Handle a user error.  This is only called from a trap handler
 *	that happened in user mode.  The error is taken by trapping back
 *	into the kernel.  Before trapping back in save the error type and
 *	first and second PCs in registers that won't be trashed by the interrupt
 *	handler.
 *
 *	errorType - Type of user error.
 */
#define USER_ERROR(userError) \
	add_nt		NON_INTR_TEMP1, r0, $userError; \
	add_nt		NON_INTR_TEMP2, CUR_PC_REG, $0; \
	add_nt		NON_INTR_TEMP3, NEXT_PC_REG, $0; \
	rd_special	VOL_TEMP1, pc; \
	return_trap	VOL_TEMP1, $12; \
	Nop; \
	cmp_trap	always, r0, r0, $MACH_USER_ERROR_TRAP; \
	Nop

/*
 * CALL_DEBUGGER(regErrorVal, constErrorVal)
 *
 *	Call the kernel debugger.  It is assumed that when we are called
 *	the spill stack is the kernel's and the saved window stack is OK.
 *	This macro does the following:
 *	
 *	    1) Saves the state into _machDebugState
 *	    2) Call routine _MachCallDebugger(errorType)
 *	    3) Restore state from _machDebugState
 *	    4) If came from user mode put back into user mode and restore spill
 *	       stack.
 *	    5) Turn on all traps.
 *	    6) Return from trap.
 *
 *	regErrorVal -	Register that holds an error value.
 *	constErrorVal -	Constant error value.
 */
#define	CALL_DEBUGGER(regErrorVal, constErrorVal) \
	add_nt		SAFE_TEMP1, r0, $debugStatePtr; \
	rd_special	SAFE_TEMP2, pc; \
	add_nt		SAFE_TEMP2, $16; \
	jump		SaveState; \
	Nop; \
	\
	add_nt		OUTPUT_REG1, regErrorVal, $constErrorVal; \
	call		_MachCallDebugger; \
	Nop; \
	\
	add_nt		SAFE_TEMP1, r0, $debugStatePtr; \
	rd_special	SAFE_TEMP2, pc; \
	add_nt		SAFE_TEMP2, $16; \
	jump		RestoreState; \
	Nop; \
	\
	rd_kpsw		SAFE_TEMP1; \
	and		VOL_TEMP1, SAFE_TEMP1, $MACH_KPSW_PREV_MODE; \
	cmp_br_delayed	eq, VOL_TEMP1, 0, 1f; \
	Nop; \
	or		SAFE_TEMP1, SAFE_TEMP1, $MACH_KPSW_CUR_MODE; \
	RESTORE_USER_SPILL_STACK(); \
	\
1:	or		SAFE_TEMP1, SAFE_TEMP1, $MACH_KPSW_ALL_TRAPS_ENA; \
	wr_kpsw		SAFE_TEMP1; \
	\
	jump_reg	CUR_PC_REG, $0; \
	return_trap	NEXT_PC_REG, $0; \
	Nop

/*
 * FETCH_CUR_INSTRUCTION(destReg) --
 *
 *	Load the contents of the instruction at CUR_PC_REG into destReg.
 *
 *	destReg -- Register to load instruction into.
 */
#ifdef BARB 
#define FETCH_CUR_INSTRUCTION(destReg) \
        LD_CONSTANT(VOL_TEMP1, 0x20000); \
        LD_CONSTANT(VOL_TEMP2, 0x40000000); \
        cmp_br_delayed  lt, CUR_PC_REG, VOL_TEMP1, 1f; \
        nop; \
        cmp_br_delayed  ge, CUR_PC_REG, VOL_TEMP2, 1f; \
        nop; \
        sub             VOL_TEMP1, CUR_PC_REG, VOL_TEMP1; \
        add             VOL_TEMP1, VOL_TEMP1, VOL_TEMP2; \
        cmp_br_delayed	always, 2f; \
        nop; \
1: \
        add_nt          VOL_TEMP1, CUR_PC_REG, $0; \
2: \
        ld_32           destReg, VOL_TEMP1, $0
#else
#define FETCH_CUR_INSTRUCTION(destReg) \
        ld_32           destReg, CUR_PC_REG, $0
#endif

#endif _MACHASMDEFS
