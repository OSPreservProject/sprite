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
 * LD_PC_RELATIVE --
 *
 *	Load the the value stored at label into register rd.  This load is done
 *	PC relative so label should not be more than a 14 signed immediate
 *	away from the current PC.
 */
#define LD_PC_RELATIVE(rd, label) \
	rd_special	rd, pc ;\
0:	ld_32		rd, rd, $((label-0b)+4); \
	nop

/*
 * LD_CONSTANT --
 *
 *	Load the 32 bit constant 'constant' into register 'rd'
 *
 * Side effects:
 *	Changes the insert register.
 */
#define LD_CONSTANT(rd, constant) \
	rd_insert	VOL_TEMP1; \
	add_nt		rd, r0, $((constant)&0xff) ; \
	wr_insert	$1 ; \
	insert		rd, rd, $(((constant)>>8)&0xff) ; \
	wr_insert	$2 ; \
	insert		rd, rd, $(((constant)>>16)&0xff) ; \
	wr_insert	$3 ; \
	insert		rd, rd, $(((constant)>>24)&0xff); \
	wr_insert	VOL_TEMP1

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
 * READ_STATUS_REGS(baseVal, resReg)
 *
 *	Read the value out of the interrupt status, interrupt mask and
 *	fe status registers.
 */
#define READ_STATUS_REGS(baseVal, resReg) \
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
 * WRITE_STATUS_REGS(baseVal, resReg)
 *
 *	Store the value to the interrupt status, interrupt mask and
 *	fe status registers.
 */
#define WRITE_STATUS_REGS(baseVal, reg) \
	st_external	reg, r0, $baseVal|MACH_CO_WR_REG; \
	extract		VOL_TEMP1, reg, $1; \
	st_external	VOL_TEMP1, r0, $baseVal|0x20|MACH_CO_WR_REG; \
	extract		VOL_TEMP1, reg, $2; \
	st_external	VOL_TEMP1, r0, $baseVal|0x40|MACH_CO_WR_REG; \
	extract		VOL_TEMP1, reg, $3; \
	st_external	VOL_TEMP1, r0, $baseVal|0x60|MACH_CO_WR_REG

/*
 * VERIFY_SWP(label, underFlowBytes) -- 
 *
 *	Verify that the saved window pointer is valid.  If so branch to the
 *	given label.  The swp is valid if
 *
 *	    swp >= min_swp_offset and
 *	    swp <= max_swp_offset - MACH_SAVED_REG_SET_SIZE
 *
 *	These bounds ensure that a single window underflow will work and many 
 *	overflows will work.
 */
#define	VERIFY_SWP(label) \
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr; \
	rd_special	VOL_TEMP2, swp; \
	ld_32		VOL_TEMP3, VOL_TEMP1, $MACH_MIN_SWP_OFFSET; \
	Nop; \
	cmp_br_delayed	lt, VOL_TEMP2, VOL_TEMP3, 1f; \
	Nop; \
	ld_32		VOL_TEMP3, VOL_TEMP1, $MACH_MAX_SWP_OFFSET; \
	sub		VOL_TEMP3, VOL_TEMP3, $MACH_SAVED_REG_SET_SIZE; \
	cmp_br_delayed	le, VOL_TEMP2, VOL_TEMP3, label; \
	Nop; \
1:

/*
 * SWITCH_TO_KERNEL_STACKS()
 *
 *	Switch to the kernel's spill and overflow stacks.  The act of switching
 *	to the kernel's overflow stack involves leaving the cwp alone and
 *	setting the swp equal to the cwp - 1 + stack_base.  This requires
 *	the following sequence of instructions:
 *
 *		1) VOL_TEMP3 <= cwp
 *		2) VOL_TEMP3 = (VOL_TEMP3 - 1) & 0x1c to decrement the cwp
 *			and mask out bits in case it wraps.
 *		3) VOL_TEMP3 <<= 5 to shift cwp<4:2> to swp<9:7>
 *		4) VOL_TEMP3 += StackBase
 *		5) swp <= VOL_TEMP3
 */
#define	SWITCH_TO_KERNEL_STACKS() \
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr; \
	Nop; \
	ld_32		SPILL_SP, VOL_TEMP1, $MACH_KERN_STACK_END_OFFSET; \
	ld_32		VOL_TEMP2, VOL_TEMP1, $MACH_KERN_STACK_START_OFFSET; \
	rd_special	VOL_TEMP3, cwp; \
	sub		VOL_TEMP3, VOL_TEMP3, $4; \
	and		VOL_TEMP3, VOL_TEMP3, $0x1c; \
	sll		VOL_TEMP3, VOL_TEMP3, $3; \
	sll		VOL_TEMP3, VOL_TEMP3, $2; \
	add_nt		VOL_TEMP3, VOL_TEMP3, VOL_TEMP2; \
	wr_special	swp, VOL_TEMP3, $0

/*
 * SAVE_USER_STATE --
 *
 *	Save the state of the user process onto the saved window stack
 *	and into the machine state struct.
 */
#define	SAVE_USER_STATE() \
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr; \
	Nop; \
	add_nt		VOL_TEMP1, VOL_TEMP1, $MACH_TRAP_REG_STATE_OFFSET; \
	rd_special	VOL_TEMP2, pc; \
	add_nt		VOL_TEMP2, VOL_TEMP2, $16; \
	Nop; \
	jump		SaveState; \
	Nop

/*
 * RESTORE_USER_STATE --
 *
 *	Restore the state of the user process from the machine state struct.
 */
#define	RESTORE_USER_STATE() \
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr; \
	Nop; \
	add_nt		VOL_TEMP1, VOL_TEMP1, $MACH_TRAP_REG_STATE_OFFSET; \
	rd_special	VOL_TEMP2, pc; \
	add_nt		VOL_TEMP2, VOL_TEMP2, $16; \
	Nop; \
	jump		RestoreState; \
	Nop

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
	add_nt		NON_INTR_TEMP1, CUR_PC_REG, $0; \
	add_nt		NON_INTR_TEMP2, NEXT_PC_REG, $0; \
	rd_special	VOL_TEMP1, pc; \
	return_trap	VOL_TEMP1, $12; \
	Nop; \
	cmp_trap	always, r0, r0, $userError; \
	Nop

/*
 * USER_SWP_ERROR() ==
 *
 *	Handle a user swp error.  This is called from a trap handler
 *	that happened in kernel mode.
 */
#define	USER_SWP_ERROR() \
	rd_kpsw		VOL_TEMP1; \
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_ALL_TRAPS_ENA; \
	wr_kpsw		VOL_TEMP1, $0; \
	SWITCH_TO_KERNEL_STACKS(); \
	add_nt		OUTPUT_REG1, r0, $MACH_USER_BAD_SWP; \
	call		_MachUserError; \
	Nop


/*
 * SWITCH_TO_DEBUGGER_STACKS()
 *
 *	Switch to the debugger's spill and overflow stacks.  The act of
 *	switching to the debugger's overflow stack involves leaving the cwp
 *	alone and setting the swp equal to the cwp - 1 + stack_base.
 *	This requires the following sequence of instructions:
 *
 *		1) VOL_TEMP2 <= cwp
 *		2) VOL_TEMP2 = (VOL_TEMP2 - 1) & 0x1c to decrement the cwp
 *			and mask out bits in case it wraps.
 *		3) VOL_TEMP2 <<= 5 to shift cwp<4:2> to swp<9:7>
 *		4) VOL_TEMP2 += StackBase
 *		5) swp <= VOL_TEMP2
 */
#define	SWITCH_TO_DEBUGGER_STACKS() \
	ld_32		SPILL_SP, r0, $debugSpillStackEnd; \
	ld_32		VOL_TEMP1, r0, $debugSWStackBase; \
	rd_special	VOL_TEMP2, cwp; \
	sub		VOL_TEMP2, VOL_TEMP2, $4; \
	and		VOL_TEMP2, VOL_TEMP2, $0x1c; \
	sll		VOL_TEMP2, VOL_TEMP2, $3; \
	sll		VOL_TEMP2, VOL_TEMP2, $2; \
	add_nt		VOL_TEMP2, VOL_TEMP2, VOL_TEMP1; \
	wr_special	swp, VOL_TEMP2, $0

/*
 * CALL_DEBUGGER(regErrorVal, constErrorVal)
 *
 *	Call the kernel debugger.  It is assumed that when we are called
 *	the spill stack is the kernel's and the saved window stack is OK.
 *	This macro does the following:
 *	
 *	    1) Saves the state into _machDebugState
 *	    2) Switch to the debuggers own spill and saved window stacks.
 *	    3) Turn off interrupts, turn on all traps, and turn off and 
 *	       invalidate the ibuffer.
 *	    4) Flush the cache.
 *	    5) Call routine _MachCallDebugger(errorType, &machDebugState)
 *	    6) Restore state from _machDebugState (note that this will 
 *	       reenable the ibuffer unless the debugger has modified the
 *	       kpsw to not have it enabled).
 *	    7) Do a normal return from trap.
 *
 *	regErrorVal -	Register that holds an error value.
 *	constErrorVal -	Constant error value.
 */
#define	CALL_DEBUGGER(regErrorVal, constErrorVal) \
	ld_32		VOL_TEMP1, r0, $debugStatePtr; \
	rd_special	VOL_TEMP2, pc; \
	add_nt		VOL_TEMP2, VOL_TEMP2, $16; \
	jump		SaveState; \
	Nop; \
	\
	SWITCH_TO_DEBUGGER_STACKS(); \
	\
	rd_kpsw		VOL_TEMP1; \
	and		VOL_TEMP1, VOL_TEMP1, $~MACH_KPSW_INTR_TRAP_ENA; \
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_ALL_TRAPS_ENA; \
	and		VOL_TEMP1, VOL_TEMP1, $~MACH_KPSW_IBUFFER_ENA; \
	wr_kpsw		VOL_TEMP1, $0; \
	invalidate_ib; \
	\
	add_nt		r1, r0, $0; \
	LD_CONSTANT(r2, VMMACH_CACHE_SIZE); \
1:	st_external	r0, r1, $MACH_CO_FLUSH; \
	add_nt		r1, r1, $VMMACH_CACHE_BLOCK_SIZE; \
	cmp_br_delayed	lt, r1, r2, 1b; \
	Nop; \
	\
	add_nt		OUTPUT_REG1, regErrorVal, $constErrorVal; \
	ld_32		OUTPUT_REG2, r0, $debugStatePtr; \
	Nop; \
	call		_kdb; \
	Nop; \
	\
	ld_32		VOL_TEMP1, r0, $debugStatePtr; \
	rd_special	VOL_TEMP2, pc; \
	add_nt		VOL_TEMP2, VOL_TEMP2, $16; \
	jump		RestoreState; \
	Nop; \
	\
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN; \
	jump		ReturnTrap; \
	Nop

/*
 * Save the cache controller state.
 */
#define SAVE_CC_STATE() \
	add_nt		r1, r0, $0; \
	LD_CONSTANT(r2, 0xff03b000); \
	nop; \
1:	ld_external	r5, r1, $MACH_CO_RD_REG; \
	nop; \
	ld_external	r6, r1, $(MACH_CO_RD_REG + 0x20); \
	wr_insert	$1; \
	insert		r5, r5, r6; \
	ld_external	r6, r1, $(MACH_CO_RD_REG + 0x40); \
	wr_insert	$2; \
	insert		r5, r5, r6; \
	ld_external	r6, r1, $(MACH_CO_RD_REG + 0x60); \
	wr_insert	$3; \
	insert		r5, r5, r6; \
	st_32		r5, r2, $0; \
	add_nt		r1, r1, $0x80; \
	add_nt		r2, r2, $4; \
	add_nt		r3, r0, $0x1000; \
	cmp_br_delayed	lt, r1, r3, 1b; \
	nop

/*
 * FETCH_CUR_INSTRUCTION(destReg) --
 *
 *	Load the contents of the instruction at CUR_PC_REG into destReg.
 *
 *	destReg -- Register to load instruction into.
 */
#ifdef BARB 
#define FETCH_CUR_INSTRUCTION(destReg) \
        LD_CONSTANT(VOL_TEMP2, 0x20000); \
        LD_CONSTANT(VOL_TEMP3, 0x40000000); \
        cmp_br_delayed  lt, CUR_PC_REG, VOL_TEMP2, 1f; \
        nop; \
        cmp_br_delayed  ge, CUR_PC_REG, VOL_TEMP3, 1f; \
        nop; \
        sub             VOL_TEMP2, CUR_PC_REG, VOL_TEMP2; \
        add             VOL_TEMP2, VOL_TEMP2, VOL_TEMP3; \
        cmp_br_delayed	always, r0, r0, 2f; \
        nop; \
1: \
        add_nt          VOL_TEMP2, CUR_PC_REG, $0; \
2: \
        ld_32           destReg, VOL_TEMP2, $0
#else
#define FETCH_CUR_INSTRUCTION(destReg) \
        ld_32           destReg, CUR_PC_REG, $0
#endif

/*
 * EXT_MASK(mask)
 *
 *	Take the given unsigned mask and extend the sign bit.
 */
#define	EXT_MASK(mask) (-((~(mask))+1))

#endif _MACHASMDEFS
