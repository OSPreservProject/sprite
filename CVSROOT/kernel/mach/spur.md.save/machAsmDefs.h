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
 * The macros in these macros are allowed to use temporary registers 1 through
 * 4.
#define	LREG1	r22
#define	LREG2	r23
#define	LREG3	r24
#define	LREG4	r25

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
 *
 * Side effects:
 *	Changes the temporary register LREG1.
 */
#define CLR_FE_STATUS(bits) \
	add_nt          LREG1, r0, $((bits)&0xff) ; \
	st_external     LREG1, r0, $MACH_FE_STATUS_0|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>8)&0xff) ; \
	st_external     LREG1, r0, $MACH_FE_STATUS_1|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>16)&0xff) ; \
	st_external     LREG1, r0, $MACH_FE_STATUS_2|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>24)&0xff) ; \
	st_external     LREG1, r0, $MACH_FE_STATUS_3|MACH_CO_WR_REG

/*
 * CLR_INTR_STATUS --
 *
 *	Selectively clear the IStatus register. Clear each bit in 'bits'
 *	that is 1.
 *
 * Side effects:
 *	Changes the temporary register LREG1.
 */
#define CLR_INTR_STATUS(bits) \
	add_nt          LREG1, r0, $((bits)&0xff) ; \
	st_external     LREG1, r0, $MACH_INTR_STATUS_0|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>8)&0xff) ; \
	st_external     LREG1, r0, $MACH_INTR_STATUS_1|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>16)&0xff) ; \
	st_external     LREG1, r0, $MACH_INTR_STATUS_2|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>24)&0xff) ; \
	st_external     LREG1, r0, $MACH_INTR_STATUS_3|MACH_CO_WR_REG

/*
 * SET_INTR_MASK --
 *
 *	Set the IMask register. 
 *
 * Side effects:
 *	Changes the temporary register LREG1.
 */
#define SET_INTR_MASK(bits) \
	add_nt          LREG1, r0, $((bits)&0xff) ; \
	st_external     LREG1, r0, $MACH_INTR_MASK_0|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>8)&0xff) ; \
	st_external     LREG1, r0, $MACH_INTR_MASK_1|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>16)&0xff) ; \
	st_external     LREG1, r0, $MACH_INTR_MASK_2|MACH_CO_WR_REG ; \
	add_nt          LREG1, r0, $(((bits)>>24)&0xff) ; \
	st_external     LREG1, r0, $MACH_INTR_MASK_3|MACH_CO_WR_REG

/*
 * SET_KPSW --
 *
 *	Set bits in the kpsw.
 *
 * Side effects:
 *	Changes the temporary registers LREG1 and LREG2 and the insert
 *	register.
 */
#define SET_KPSW(bits) \
	ld_constant(LREG1, bits) ; \
	rd_kpsw		LREG2 ; \
	or		LREG2, LREG1, LREG2 ; \
	wr_kpsw		LREG2, r0

/*
 * CLR_KPSW --
 *
 *	Clear bits in the KPSW.
 *
 * Side effects:
 *	Changes the temporary registers LREG1 and LREG2 and the insert 
 *	register.
 */
#define CLR_KPSW(bits) \
	ld_constant(LREG1, bits) ; \
	add_nt		LREG2, r0, $-1 ; \
	xor		LREG1, LREG2, LREG1 ; \
	rd_kpsw		LREG2 ; \
	and		LREG2, LREG1, LREG2 ; \
	wr_kpsw		LREG2, r0

/*
 * MOD_KPSW --
 *
 *	Change bits in the KPSW.
 *
 * Side effects:
 *	Changes the temporary registers LREG1 and LREG2 and the	insert
 *	register.
 */
#define MOD_KPSW(set,clr) \
	ld_constant(LREG1, clr) ; \
	xor		LREG1, LREG1, LREG1 ; \
	rd_kpsw		LREG2 ; \
	and		LREG2, LREG1, LREG2 ; \
	ld_constant(LREG1, set) ; \
	or		LREG2, LREG1, LREG2 ; \
	wr_kpsw		LREG2, r0

/*
 * SET_UPSW --
 *
 *	Set bits in the upsw.
 *
 * Side effects:
 *	Changes the temporary registers LREG1 and LREG2 and the insert 
 *	register.
 */
#define SET_UPSW(bits) \
	ld_constant(LREG1, bits) ; \
	rd_special	LREG2, upsw ; \
	or		LREG2, LREG1, LREG2 ; \
	wr_special	upsw, LREG2, r0

/*
 * CLR_UPSW --
 *
 *	Clear bits in the upsw.
 *
 * Side effects:
 *	Changes the temporary registers LREG1 and LREG2 and the insert 
 *	register.
 */
#define CLR_UPSW(bits) \
	ld_constant(LREG1, bits) ; \
	add_nt		LREG2, r0, $-1 ; \
	xor		LREG1, LREG2, LREG1 ; \
	rd_special	LREG2, upsw ; \
	and		LREG2, LREG1, LREG2 ; \
	wr_special	upsw, LREG2, r0

/*
 * MOD_UPSW --
 *
 *	Change bits in the upsw
 *
 * Side effects:
 *	Changes the temporary registers LREG1 and LREG2 and the insert
 *	register.
 */
#define MOD_UPSW(set,clr) \
	ld_constant(LREG1, clr) ; \
	xor		LREG1, LREG1, LREG1 ; \
	rd_special	LREG2, upsw ; \
	and		LREG2, LREG1, LREG2 ; \
	ld_constant(LREG1, set) ; \
	or		LREG2, LREG1, LREG2 ; \
	wr_special	upsw, LREG2, r0

/*
 * SWITCH_TO_KERNEL_MODE() -- 
 *
 *	Switch to kernel mode and save the current mode in the prev mode
 *	bit of the kpsw.
 */
#define	SWITCH_TO_KERNEL_MODE() \
	rd_kpsw		LREG1; \
	and		LREG2, LREG1, $MACH_KPSW_CUR_MODE; \
	cmp_br_delayed	eq, LREG2, r0, 1f; \
	Nop; \
	or		LREG1, LREG1, $MACH_KPSW_PREV_MODE; \
	cmp_br_delayed	always, 2f; \
	Nop; \
1:	and		LREG1, LREG1, $(~MACH_KPSW_PREV_MODE); \
2:	and 		LREG1, LREG1, $(~MACH_KPSW_CUR_MODE)

/*
 * VERIFY_SWP(savedBytes) -- 
 *
 *	Verify that the saved window pointer is valid.  If not kill
 *	the current process.  The swp is valid if it is greater than or
 *	equal to the minimum value stored in the current process' state
 *	structure and there is at least one page of bytes between the
 *	maximum swp value and the current value.
 *
 *	savedBytes - Number of bytes that there should be available on the
 *		     saved window stack.
 */
#define	VERIFY_SWP(savedBytes) \
	ld_32		LREG1, r0, $curStatePtr; \
	rd_special	LREG2, swp; \
	ld_32		LREG3, LREG1, $MACH_MIN_SWP_OFFSET; \
	Nop; \
	add_nt		LREG3, LREG3, savedBytes; \
	cmp_br_delayed	gt, LREG3, LREG2, 1f; \
	Nop; \
	ld_32		LREG3, LREG1, $MACH_MAX_SWP_OFFSET; \
	add_nt		LREG2, LREG2, $MACH_PAGE_SIZE; \
	cmp_br_delayed	ge, LREG3, LREG2, 2f; \
	Nop; \
1:	USER_ERROR(MACH_USER_BAD_SWP); \
2:

/*
 * SWITCH_TO_KERNEL_SPILL_STACK() -
 *
 *	Switch to both the kernel's normal stack and the saved window stack.
 */
#define SWITCH_TO_KERNEL_SPILL_STACK() \
	ld_32		LREG1, r0, $curStatePtr; \
	Nop; \
	ld_32		r4, LREG1, $MACH_KERN_STACK_END; \
	Nop

/*
 * SWITCH_TO_KERNEL_STACKS() -
 *
 *	Switch to both the kernel's normal stack and the saved window stack.
 */
#define SWITCH_TO_KERNEL_STACKS() \
	ld_32		LREG1, r0, $curStatePtr; \
	Nop; \
	ld_32		r4, LREG1, $MACH_KERN_STACK_END; \
	Nop; \
	ld_32		LREG2, LREG1, $MACH_KERN_STACK_START; \
	add_nt		LREG2, r0, $0x380; \
	wr_special	cwp, r0; \
	wr_special	swp, LREG2

/*
 * USER_ERROR(errorType) --
 *
 *	Handle a fatal user error.  This involves switching to kernel
 *	mode, switching to the kernel's normal stack and saved window
 *	stack and calling the user error routine.
 *
 *	errorType - Type of user error.
 */
#define USER_ERROR(userError) \
	SWITCH_TO_KERNEL_MODE(); \
	SWITCH_TO_KERNEL_STACKS(); \
	SET_KPSW(MACH_KPSW_ALL_TRAPS_ENA); \
	add_nt		r27, r0, $userError; \
	call		_MachUserError$w; \
	Nop

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
	extract		LREG1, rx, $1 ;\
	st_external	LREG1, r0, $rptm|MACH_CO_WR_REG|0x20  ;\
	extract		LREG2, rx, $2 ;\
	st_external	LREG2, r0, $rptm|MACH_CO_WR_REG|0x40  ;\
	extract		LREG2, rx, $3 ;\
	st_external	LREG2, r0, $rptm|MACH_CO_WR_REG|0x60 

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
	sra		LREG1, rx, $1 ;\
	sra		LREG1, LREG1, $1 ;\
	extract		LREG2, LREG1, $2 ;\
	st_external	LREG2, r0, $MACH_PTEVA_4|MACH_CO_WR_REG ;\
	sra		LREG1, LREG1, $1 ;\
	sra		LREG1, LREG1, $1 ;\
	extract		LREG2, LREG1, $1 ;\
	st_external	LREG2, r0, $MACH_PTEVA_3|MACH_CO_WR_REG

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
	sra		LREG1, rx, $1 ;\
	sra		LREG1, LREG1, $1 ;\
	extract		LREG2, LREG1, $2 ;\
	st_external	LREG2, r0, $MACH_RPTEVA_4|MACH_CO_WR_REG ;\
	wr_insert	$1 ;\
	insert		LREG1, rx, LREG2 ;\
	sll		LREG1, LREG1, $2 ;\
	extract		LREG2, LREG1, $2 ;\
	st_external	LREG2, r0, $MACH_RPTEVA_2|MACH_CO_WR_REG  ;\
	sll		LREG1, LREG1, $2 ;\
	extract		LREG2, LREG1, $2 ;\
	st_external	LREG2, r0, $MACH_RPTEVA_3|MACH_CO_WR_REG 

/*
 * JUMP_VIRTUAL(rx) --  
 *	
 *	Switch from fetching instructions from the physical address space
 *	to the virtual address space.
 *
 *	rx -- 	register containing the virtual address to begin execution.
 *
 * Side Effects:
 *	1) IB is disabled. LREG1 contains the previous value of the K_IBuffer
 *	   bit.
 *	2) The INVALIDATE_IB instruction MUST be executed before re-enabling
 *	   the IBuffer.
 */
#define JUMP_VIRTUAL(rx) \
	rd_kpsw		LREG1 ;\
	and		LREG2, LREG1, $~MACH_KPSW_IBUFFER_ENA ;\
	and 		LREG1, LREG1, $MACH_KPSW_IBUFFER_ENA ;\
	or		LREG2, LREG2, $MACH_KPSW_VIRT_IFETCH_ENA ;\
	jump_reg	rx, r0 ;\
	wr_kpsw		LREG2, r0


/*
 * REENABLE_IB -- 
 *
 *	Re-enable the IB after executing a 'jump_virtual' macro
 *
 *	Assumptions:
 *		LREG1 contains the previous value of the K_IBuffer bit.
 */
#define REENABLE_IB() \
	invalidate_ib ;\
	rd_kpsw		LREG2 ;\
	or		LREG2, LREG2, LREG1 ;\
	wr_kpsw		LREG2, r0

#endif _MACHASMDEFS
