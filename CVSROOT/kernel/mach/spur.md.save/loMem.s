/* loMem.s --
 *
 *	The first thing that is loaded into the kernel.  Handles traps,
 *	faults, errors and interrupts.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * rcs = $Header$ SPRITE (Berkeley)
 */

#include "reg.h"
#include "machConst.h"
#include "machAsmDefs.h"

/*
 * SAVED WINDOW STACK CONVENTIONS
 *
 * When user processes trap into the kernel the kernel saves all of the
 * user registers and then switches to its own window stack.  The only
 * exception is in the underflow and overflow routines which have to
 * use the user's stack since it is still in user mode.  The act of saving
 * the user's registers requires that the user's saved window stack always
 * has enough space left on the saved window stack to save the remaining
 * windows in the users register stack.  If when the kernel is trying to save
 * state or handle overflow or underflow faults the user's SWP is bogus
 * then the kernel switches over to the kernel's saved window stack and the
 * user process is killed.  If the SWP is bogus when an interrupt occurs, then
 * the user process is not killed until after the interrupt is handled.  All
 * memory between min_swp_offset and max_swp_offset is wired down.
 *
 * The swp is considered valid if it obeys the constraint
 *
 *	min_wired_addr <= swp <= max_wired_addr - 8 * saved_window_size
 *
 * In order to keep things simple more space is allocated only by the window 
 * underflow and overflow routines.  However, when a user process traps into
 * the kernel windows can be saved on its window stack when the switch is
 * made from the user to the kernel stack.  In order to ensure that the
 * constraints are always upheld there needs to be slop at the high end.  
 * Thus, more space is allocated by the window overflow handler if after an
 * overflow
 *
 * 	swp >= max_wired_addr - 16 * saved_window_size
 *
 * This ensures that an entire bank of windows can be saved because of
 * miscellaneous kernel calls between calls to the window overflow handler
 * yet the valid condition will still be held.
 *
 * There also needs to be slop at the low end in order to handle returns
 * from signal handlers.  The act of returning from a signal handler will
 * cause a window underflow off of the user stack.  Since it doesn't allocate
 * more space until an underflow occurs in user mode (we can't do it in kernel
 * mode because it could cause deadlock) one window of slop is required.
 * Thus more space is allocated by the window underflow handler if after
 * an underflow
 *
 *	swp == min_wired_addr + 1.
 *
 * Of course the other option would be to check the bounds of the window
 * stack whenever we put stuff on or take stuff off but by putting slop
 * at the low and high ends we only have to check in the overflow and
 * underflow handlers.
 *
 * SPILL STACK CONVENTIONS
 *
 * When user processes trap into the kernel the kernel switches over
 * to its own spill stack before doing any C calls.  A separate stack is
 * used instead of the user stack because it is real easy to manage 
 * separate spill stacks and we have no idea if the user spill stack is
 * good or not.
 *
 * SWITCHING TO KERNEL MODE
 *
 * Certain traps (eg. window overflow traps) occur in user mode.  If for
 * some reason kernel mode needs to be entered (an error or need to allocate
 * more window memory) then a gross sequence has to be undertaken.  We
 * want to switch to kernel mode but we can't modify the kpsw because we
 * are in user mode.  The only way to get to kernel mode is to do a return from
 * trap and then immediately do a compare trap to get back in.  After these
 * two instructions we are back in the window that we were in when we wanted
 * to switch to kernel mode.  Unfortunately the compare trap will trash
 * CUR_PC_REG and NEXT_PC_REG which tell us where to continue when we
 * truly return to the user process.  Thus CUR_PC_REG and NEXT_PC_REG 
 * have to be saved in the window before the return_trap-cmp_trap sequence.  
 * Now the act of returning from the trap means that interrupts are enabled.   
 * Therefore I need to save CUR_PC_REG and NEXT_PC_REG in a place where 
 * they won't get trashed if an interrupt comes in.  The place to put them 
 * is in NON_INTR_TEMP1 through NON_INTR_TEMP3 because these are left 
 * untouched by the interrupt handling code.
 *
 * SAVING THE INSERT REGISTER
 *
 * All of these trap handlers try to be transparent to the process that 
 * caused the trapped.  This means that all global variables are saved
 * and restored when they are modified.  One of these globals is the insert
 * register.  Since the insert register may be used by C routines that are
 * called and is used by ParseInstruction, it is saved and restored to/from
 * local registers before and after calls to these routines.  It can't
 * be saved to global memory because traps can be nested.
 */

/*
 * Temporary local registers. 
 */
	.set rt1, 17
	.set rt2, 18
	.set rt3, 19
	.set rt4, 20
	.set rt5, 21
	.set rt6, 22
	.set rt7, 23
	.set rt8, 24
	.set rt9, 25

/*
 * Defining starting KPSW for processors.
 */
#ifdef WITH_IBUFFER
#ifdef WITH_PREFETCH	/* WITH_IBUFFER and WITH_PREFETCH */
#define	DEFAULT_KPSW   (MACH_KPSW_PREFETCH_ENA | MACH_KPSW_IBUFFER_ENA | \
			MACH_KPSW_VIRT_DFETCH_ENA|MACH_KPSW_VIRT_IFETCH_ENA |\
			MACH_KPSW_FAULT_TRAP_ENA | MACH_KPSW_ERROR_TRAP_ENA |\
			MACH_KPSW_ALL_TRAPS_ENA)
#else	/* WITH_IBUFFER but no WITH_PREFETCH */
#define	DEFAULT_KPSW	(MACH_KPSW_IBUFFER_ENA | MACH_KPSW_VIRT_DFETCH_ENA | \				MACH_KPSW_VIRT_IFETCH_ENA | MACH_KPSW_FAULT_TRAP_ENA |\
			MACH_KPSW_ERROR_TRAP_ENA | MACH_KPSW_ALL_TRAPS_ENA)
#endif
#else	/* no WITH_IBUFFER */
#define	DEFAULT_KPSW	(MACH_KPSW_VIRT_DFETCH_ENA|MACH_KPSW_VIRT_IFETCH_ENA|\
			MACH_KPSW_FAULT_TRAP_ENA | MACH_KPSW_ERROR_TRAP_ENA |\
			MACH_KPSW_ALL_TRAPS_ENA)
#endif

/*
 * C structures and routines that are called from here.
 */
	.globl _proc_RunningProcesses
	.globl _machCurStatePtrs
	.globl _machStatePtrOffset
	.globl _machSpecialHandlingOffset
	.globl _machMaxSysCall
	.globl _machKcallTableOffset
	.globl _machNumArgs
	.globl _machDebugState
	.globl _MachInterrupt
	.globl _MachUserError
	.globl _MachVMPCFault 
	.globl _MachVMDataFault 
	.globl _MachSigReturn
	.globl _MachGetWinMem
	.globl _MachUserAction
	.globl _machNonmaskableIntrMask
	.globl _machIntrMask 
	.globl _machDbgInterruptMask
	.globl _machTrapTableOffset
	.globl _machInterruptAddr
	.globl _machDebugStatePtrs
	.globl _machMapSlotIdToPnum
	.globl _machMasterProcessor
	.globl _machFirstProcState
/*
 * The KPSW value to set and 
/*
 * Trap table.  The hardware jumps to virtual 0x1000 when any type of trap
 * occurs.
 */
	.org 0x1000
	jump PowerUp		/* Reset - Jump to powerup sequencer. */
	Nop

	.org 0x1010
	jump ErrorTrap		/* Error */
	Nop

	.org 0x1020
	jump WinOvFlow		/* Window overflow */
	invalidate_ib

	.org 0x1030
	jump WinUnFlow		/* Window underflow */
	invalidate_ib

	.org 0x1040
	jump FaultIntr		/* Fault or interrupt */
	invalidate_ib

	.org 0x1050
	jump FPUExcept		/* FPU Exception */
	Nop

	.org 0x1060
	jump Illegal		/* Illegal op, kernel mode access violation */
	Nop

	.org 0x1070
	jump Fixnum		/* Fixnum, fixnum_or_char, generation */
	Nop

	.org 0x1080
	jump Overflow		/* Integer overflow */
	Nop

	.org 0x1090
	jump CmpTrap		/* Compare trap instruction */
	invalidate_ib


	.org 0x1100
	/*
	 * This entry needs to be at the same place in 
	 * both the physical and virtual space.  If it's moved
	 * here, it needs to move in loMem.s as well.
	 */
        .globl _debugger_active_address
_debugger_active_address:

/*
 ****************************************************************************
 *
 * BEGIN: SPECIAL LOW MEMORY OBJECTS
 *
 * Objects that need to be accessed with immediate fields have to be stored
 * between 0x1000 and 0x2000 because there are only 13 useable bits
 * (the 14th bit is a sign bit).
 *
 ****************************************************************************
 */
/*
 * Store addresses of things that need to be loaded into registers through
 * the use of immediate constants.  For example if we want to get a hold
 * of what _machCurStatePtrs points to we can't do it normally by the 
 * instruction
 *
 * 	ld_32	rt1, r0, $_machCurStatePtrs
 *
 * because the address of _machCurStatePtr will be longer than 13 bits.  
 * However if we put _machCurStatePtr in low memory then we can get to
 * it directly.
 *
 * Other options are LD_PC_RELATIVE or LD_CONSTANT.
 */
/*
 * The following data structures are index by processor number.  Currently
 * only room for 8 processors is defined.
 *
 * _machCurStatePtrs - Pointers to the Mach_State structure of the process
 * running on each processor.
 */
_machCurStatePtrs: 		.long 0; .long 0; .long 0; .long 0
				.long 0; .long 0; .long 0; .long 0
/*
 * Pointers to the state save upon entry to the debugger for each processor.
 */

_machDebugStatePtrs:		.long _machDebugState; .long 0; .long 0; .long 0
				.long 0; .long 0; .long 0; .long 0
/*
 * Pointers to the debugger's save window stack. Since there's only one 
 * debugger we need only one stack.
 */
debugSWStackBasePtr:	.long MACH_DEBUG_STACK_BOTTOM
/*
 * Pointers to the end of the debugger's spill stack.
 */
debugSpillStackEndPtr:	.long (MACH_DEBUG_STACK_BOTTOM + MACH_KERN_STACK_SIZE)
/*
 * _machMapSlotToPnum is index by slot ID and performs the mapping of 
 * slot ids into processor numbers.
 */
_machMapSlotIdToPnum: 		.long 0; .long 0; .long 0; .long 0;
				.long 0; .long 0; .long 0; .long 0;
				.long 0; .long 0; .long 0; .long 0;
				.long 0; .long 0; .long 0; .long 0;
runningProcesses: 		.long _proc_RunningProcesses
firstProcStatePtr:		.long _machFirstProcState
numArgsPtr:			.long _machNumArgs
_machStatePtrOffset:		.long 0
_machSpecialHandlingOffset:	.long 0
_machMaxSysCall	:		.long 0
_machKcallTableOffset:		.long 0
_machTrapTableOffset:		.long 0
_machDbgInterruptMask:		.long 0
_machIntrMask:			.long 0
_machNonmaskableIntrMask:	.long 0
_machInterruptAddr:		.long 0
_machMasterProcessor:		.long 0
feStatusReg:			.long 0

/*
 * The instruction to execute on return from a signal handler.  Is here
 * because the value is loaded into a register and it is quicker if
 * is done as an immediate.
 */
SigReturnAddr:
	cmp_trap	always, r0, r0, $MACH_SIG_RETURN_TRAP	
	Nop

/*
 * Jump table to return the operands from an instruction.  Also here
 * because is jumped to through an immediate constant.
 */
OpRecov:
	add_nt		r31,  r0, $0
	add_nt		r31,  r1, $0
	add_nt		r31,  r2, $0
	add_nt		r31,  r3, $0
	add_nt		r31,  r4, $0
	add_nt		r31,  r5, $0
	add_nt		r31,  r6, $0
	add_nt		r31,  r7, $0
	add_nt		r31,  r8, $0
	add_nt		r31,  r9, $0
	add_nt		r31, r10, $0
	add_nt		r31, r11, $0
	add_nt		r31, r12, $0
	add_nt		r31, r13, $0
	add_nt		r31, r14, $0
	add_nt		r31, r15, $0
	add_nt		r31, r16, $0
	add_nt		r31, r17, $0
	add_nt		r31, r18, $0
	add_nt		r31, r19, $0
	add_nt		r31, r20, $0
	add_nt		r31, r21, $0
	add_nt		r31, r22, $0
	add_nt		r31, r23, $0
	add_nt		r31, r24, $0
	add_nt		r31, r25, $0
	add_nt		r31, r26, $0
	add_nt		r31, r27, $0
	add_nt		r31, r28, $0
	add_nt		r31, r29, $0
	add_nt		r31, r30, $0

/*
 * Jump table to set a register.  r9 contains the value to set.
 */
SetReg:
	add_nt		r0, r9, $0
	add_nt		r1, r9, $0
	add_nt		r2, r9, $0
	add_nt		r3, r9, $0
	add_nt		r4, r9, $0
	add_nt		r5, r9, $0
	add_nt		r6, r9, $0
	add_nt		r7, r9, $0
	add_nt		r8, r9, $0
	add_nt		r9, r9, $0
	add_nt		r10, r9, $0
	add_nt		r11, r9, $0
	add_nt		r12, r9, $0
	add_nt		r13, r9, $0
	add_nt		r14, r9, $0
	add_nt		r15, r9, $0
	add_nt		r16, r9, $0
	add_nt		r17, r9, $0
	add_nt		r18, r9, $0
	add_nt		r19, r9, $0
	add_nt		r20, r9, $0
	add_nt		r21, r9, $0
	add_nt		r22, r9, $0
	add_nt		r23, r9, $0
	add_nt		r24, r9, $0
	add_nt		r25, r9, $0
	add_nt		r26, r9, $0
	add_nt		r27, r9, $0
	add_nt		r28, r9, $0
	add_nt		r29, r9, $0
	add_nt		r30, r9, $0
	add_nt		r31, r9, $0

/*
 ****************************************************************************
 *
 * END: SPECIAL LOW MEMORY OBJECTS
 *
 ****************************************************************************
 */

.org	0x2000
	.globl start
start:
/*
 * The initial boot code.  This is where we start executing in physical mode
 * after we are down loaded.  Our job is to:
 *
 *	1) Initialize VM.
 *	2) Jump to virtual mode
 *	3) Initialize the swp, cwp and spill sp.
 *	4) Jump to the main routine.
 *
 * The following code assumes that the initial kernel does not take more
 * than 4 Mbytes of memory since it starts the kernel page tables at
 * 4 Mbytes.
 *
 *	KERN_PT_FIRST_PAGE	The physical page where the page tables start.
 *	KERN_NUM_PAGES		The number of physical pages in the kernel.
 *	KERN_PT_BASE		The address of the kernel's page tables.
 *	KERN_PT2_BASE		The address of the 2nd level kernel
 *				page tables.
 */

#define	KERN_PT_FIRST_PAGE	1024

#define	KERN_NUM_PAGES		1024
#define	KERN_PT_BASE	(MACH_MEM_SLOT_MASK | (KERN_PT_FIRST_PAGE * VMMACH_PAGE_SIZE))
#define	KERN_PT2_BASE	(KERN_PT_BASE + ((VMMACH_SEG_PT_SIZE / 4) * VMMACH_KERN_PT_QUAD))

/*
 * Initialize the kernel page tables.  In the code that follows registers have 
 * the following meaning:
 *
 *	r1:	The base address of the second-level page tables.
 *	r2:	The number of page table pages.
 *	r3:	The page table entry.
 *	r4:	The page table increment.
 *
 * First initialize the second level page tables so that they map
 * all of the kernel page tables.
 */

#ifdef PATCH_IBUFFER
	/*
	 * Clear out r10 in all windows. 
	 */
	rd_special	r1, cwp
	wr_special	cwp,r0,$0	/* Window 0,1 */
	Nop
	add_nt		r10,r0,$0
	add_nt		r26,r0,$0
	wr_special	cwp,r0,$8	/* Window 2,3 */
	Nop
	add_nt		r10,r0,$0
	add_nt		r26,r0,$0
	wr_special	cwp,r0,$16	/* Window 4,5 */
	Nop
	add_nt		r10,r0,$0
	add_nt		r26,r0,$0
	wr_special	cwp,r0,$24	/* Window 6,7 */
	Nop
	add_nt		r10,r0,$0
	add_nt		r26,r0,$0
	wr_special	cwp, r1, $0
	Nop
#endif

	LD_CONSTANT(r1, KERN_PT2_BASE)
	add_nt		r2, r0, $VMMACH_NUM_PT_PAGES
	LD_CONSTANT(r3, MACH_MEM_SLOT_MASK | (KERN_PT_FIRST_PAGE << VMMACH_PAGE_FRAME_SHIFT) | VMMACH_RESIDENT_BIT | VMMACH_CACHEABLE_BIT | VMMACH_KRW_URO_PROT | VMMACH_REFERENCED_BIT | VMMACH_MODIFIED_BIT)
	LD_CONSTANT(r4, 1 << VMMACH_PAGE_FRAME_SHIFT)

1:
	st_32		r3, r1, $0
	add_nt		r1, r1, $4
	add_nt		r3, r3, r4
	sub		r2, r2, $1
	cmp_br_delayed	gt, r2, $0, 1b
	Nop


/*
 * Next initialize the kernel page table to point to 4 Mbytes of mapped
 * code.
 */
	LD_CONSTANT(r1, KERN_PT_BASE)
	add_nt		r2, r0, $KERN_NUM_PAGES
	LD_CONSTANT(r3, MACH_MEM_SLOT_MASK | (MACH_FIRST_PHYS_PAGE << VMMACH_PAGE_FRAME_SHIFT) | VMMACH_RESIDENT_BIT | VMMACH_CACHEABLE_BIT | VMMACH_KRW_URO_PROT | VMMACH_REFERENCED_BIT | VMMACH_MODIFIED_BIT)

1:
	st_32		r3, r1, $0
	add_nt		r1, r1, $4
	add_nt		r3, r3, r4
	sub		r2, r2, $1
	cmp_br_delayed	gt, r2, $0, 1b
	Nop

/*
 * Initialize the PTEVA.
 */
	LD_CONSTANT(r1, VMMACH_KERN_PT_BASE >> VMMACH_PAGE_SHIFT)
	ST_PT_BASE(r1)
/*
 * Initialize the RPTEVA.
 */
	ST_RPT_BASE(r1)
/*
 * Initialize the 0th segment register.
 */
	ST_GSN(r0, MACH_GSN_0)
/*
 * Initialize the RPTM register.
 */
	LD_CONSTANT(r1, KERN_PT2_BASE)
	ST_RPTM(r1, MACH_RPTM_0)
/*
 * Clear out the cache.
 */
	LD_CONSTANT(r1, 0x03000000)
	LD_CONSTANT(r2, (0x03000000 | VMMACH_CACHE_SIZE))
1:
	st_32		r0, r1, $0
	add_nt		r1, r1, $VMMACH_CACHE_BLOCK_SIZE
	cmp_br_delayed	lt, r1, r2, 1b
	Nop

/*
 * Clear snoop tags.
 */
	LD_CONSTANT(r1, 0x04000000)
	LD_CONSTANT(r2, (0x04000000 | VMMACH_CACHE_SIZE))
1:
	st_32		r0, r1, $0
	add_nt		r1, r1, $VMMACH_CACHE_BLOCK_SIZE
	cmp_br_delayed	lt, r1, r2, 1b
	Nop

/*
 * Initialize the cwp, swp and SPILL_SP to their proper values.
 */
	wr_special	cwp, r0, $4
	Nop
	LD_CONSTANT(r1, MACH_STACK_BOTTOM)
	wr_special	swp, r1, $0
	LD_CONSTANT(SPILL_SP, MACH_CODE_START)

/*
 * Clear out the upsw.
 */
	wr_special	upsw, r0, $0

/*
 * Clear out the interrupt mask register so that no interrupts are enabled.
 */
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, r0)

/*
 * Clear the fe status register.
 */
	add_nt		r1, r0, $-1
	WRITE_STATUS_REGS(MACH_FE_STATUS_0, r1)

/*
 * Now jump to virtual mode through the following sequence:
 *
 *	1) Disable instruction buffer just in case it is on.
 *	2) Invalidate the instruction buffer.
 *	3) Make a good kpsw.
 *	4) Call the main function while setting the kpsw to put is in
 *	   virtual mode in the nop slot of the call.
 */
	wr_kpsw		r0, $0
	invalidate_ib
	add_nt		r1, r0, $(DEFAULT_KPSW)
	LD_PC_RELATIVE(r2, mainAddr)
	jump_reg	r2, $0
	wr_kpsw		r1, $0
	jump		ErrorTrap
	Nop
mainAddr:	.long	_main
/*
 *---------------------------------------------------------------------------
 *
 * WinOvFlow --
 *
 *	Window overflow fault handler.  If in user mode then the swp is
 *	validated first.  If it is invalid then the trapping process is 
 *	killed.  Also if in user mode more memory will be wired down if not
 * 	enough is wired already.
 */
WinOvFlow:
	rd_kpsw		SAFE_TEMP1			
	and		SAFE_TEMP1, SAFE_TEMP1, $MACH_KPSW_CUR_MODE
	cmp_br_delayed	eq, SAFE_TEMP1, r0, winOvFlow_SaveWindow
	Nop
	VERIFY_SWP(winOvFlow_SaveWindow)	/* Verify that the SWP is OK.*/
	/*
	 * The SWP is bogus.  Act as if we were able to do the overflow
	 * so when we trap back in to kill the user process we won't take
	 * another overflow.
	 */
	rd_special	r1, swp
	wr_special	swp, r1, $MACH_SAVED_WINDOW_SIZE
	USER_ERROR(MACH_USER_BAD_SWP_TRAP)
	/* DOESN'T RETURN */
winOvFlow_SaveWindow:
	/*
	 * Actually save the window.
	 */
	add_nt		SAFE_TEMP2, r1, $0	/* Save r1 */
	rd_special	VOL_TEMP1, cwp
	wr_special	cwp, VOL_TEMP1, $4	/* Move forward one window. */
	Nop
	rd_special	r1, swp
	and		r1, r1, $~7		/* Eight byte align swp */
	wr_special	swp, r1, $MACH_SAVED_WINDOW_SIZE
	add_nt		r1, r1, $MACH_SAVED_WINDOW_SIZE

	st_40		r10, r1, $0
#ifdef PATCH_IBUFFER
	add_nt		r10,r0,$0
#endif
	st_40		r11, r1, $8
	st_40		r12, r1, $16
	st_40		r13, r1, $24
	st_40		r14, r1, $32
	st_40		r15, r1, $40
	st_40		r16, r1, $48
	st_40		r17, r1, $56
	st_40		r18, r1, $64
	st_40		r19, r1, $72
	st_40		r20, r1, $80
	st_40		r21, r1, $88
	st_40		r22, r1, $96
	st_40		r23, r1, $104
	st_40		r24, r1, $112
	st_40		r25, r1, $120
	rd_special	r1, cwp
	wr_special	cwp, r1, $-4		/* Move back one window. */
	Nop

	add_nt		r1, SAFE_TEMP2, $0	/* Restore r1 */

	/* 
	 * See if we have to allocate more memory.  We need to allocate more
	 * if
	 *
	 *	swp > max_swp - 2 * MACH_SAVED_REG_SET_SIZE
	 */
	cmp_br_delayed	eq, SAFE_TEMP1, r0, winOvFlow_Return	/* No need to */
	Nop							/* check from */
								/* kernel mode*/
	LD_MACH_CUR_STATE_PTR(VOL_TEMP1)
	rd_special	VOL_TEMP2, swp
	ld_32		VOL_TEMP1, VOL_TEMP1, $MACH_MAX_SWP_OFFSET
	sub		VOL_TEMP1, VOL_TEMP1, $(2 * MACH_SAVED_REG_SET_SIZE)
	cmp_br_delayed	ule, VOL_TEMP2, VOL_TEMP1, winOvFlow_Return
	Nop
	/*
	 * Allocate more memory.
	 */
	add_nt		NON_INTR_TEMP1, CUR_PC_REG, $0
#ifdef PATCH_IBUFFER
	/*
	 * Patch for IBUFFER.
	 *	Clear CUR_PC_REG so we can detect when interrupt trap
	 *	handling messes up.
	 */
	add_nt		CUR_PC_REG, r0, $0
#endif
	add_nt		NON_INTR_TEMP2, NEXT_PC_REG, $0
	rd_special	VOL_TEMP1, pc
	return_trap	VOL_TEMP1, $12
	Nop
	cmp_trap	always, r0, r0, $MACH_GET_WIN_MEM_TRAP
	Nop

winOvFlow_Return:
#ifdef PATCH_IBUFFER
	/*
	 * Patch for IBUFFER.
	 *	1) Clear CUR_PC_REG so we can detect when interrupt trap
	 *	   handling messes up.
	 *	2) invalidate Ibuffer so we don't get hit when we return.
	 */
	add_nt		SAFE_TEMP1, CUR_PC_REG, $0
	add_nt		CUR_PC_REG, r0, $0
	invalidate_ib
	jump_reg	SAFE_TEMP1, $0
#else
	jump_reg	CUR_PC_REG, $0
#endif
	return_trap	NEXT_PC_REG, $0

/*
 *----------------------------------------------------------------------------
 *
 * WinUnFlow --
 *
 *	Window underflow fault handler.  If in user mode then the swp is 
 *	validated first.  If it is invalid then the trapping process is 
 *	killed.  Also if in user mode more memory is wired down if 
 *	the trapping process doesn't have the memory behind the new swp wired
 *	down.
 *
 *----------------------------------------------------------------------------
 */
WinUnFlow:
	rd_kpsw		SAFE_TEMP1
	and		SAFE_TEMP1, SAFE_TEMP1, $MACH_KPSW_CUR_MODE
	cmp_br_delayed	eq, SAFE_TEMP1, r0, winUnFlow_RestoreWindow
	Nop
	VERIFY_SWP(winUnFlow_RestoreWindow)
	/*
	 * We have a bogus SWP.  Act as if we took an underflow so when we
	 * return to user mode to try to kill the process we won't try to
	 * take an underflow.  As it turns out it shouldn't matter because
	 * the return trap in the USER_ERROR macro will be done with all
	 * traps disabled which means that the underflow will be ignored.
	 * However, its better to be safe than sorry.
	 */
	rd_special	VOL_TEMP1, swp
	wr_special	swp, VOL_TEMP1, $-MACH_SAVED_WINDOW_SIZE
	USER_ERROR(MACH_USER_BAD_SWP_TRAP)
	/* DOESN'T RETURN */
winUnFlow_RestoreWindow:
	add_nt		SAFE_TEMP2, r1, $0	/* Save r1 */
	rd_special	r1, swp
	and		r1, r1, $~7		/* Eight byte align swp */
	rd_special	VOL_TEMP1, cwp
	wr_special	cwp, VOL_TEMP1,  $-8	/* move back two windows */
	Nop


	ld_40		r10, r1,   $0
	ld_40		r11, r1,   $8
	ld_40		r12, r1,  $16
	ld_40		r13, r1,  $24
	ld_40		r14, r1,  $32
	ld_40		r15, r1,  $40
	ld_40		r16, r1,  $48
	ld_40		r17, r1,  $56
	ld_40		r18, r1,  $64
	ld_40		r19, r1,  $72
	ld_40		r20, r1,  $80
	ld_40		r21, r1,  $88
	ld_40		r22, r1,  $96
	ld_40		r23, r1, $104
	ld_40		r24, r1, $112
	ld_40		r25, r1, $120
	wr_special	swp, r1, $-MACH_SAVED_WINDOW_SIZE
	rd_special	r1, cwp
	wr_special	cwp,  r1, $8	/* move back ahead two windows */
	Nop
	add_nt		r1, SAFE_TEMP2, $0	/* Restore r1 */
	/*
	 * See if need more memory.  We need more if 
	 *
	 * 	swp <= min_swp + MACH_SAVED_WINDOW_SIZE
	 */
	cmp_br_delayed	eq, SAFE_TEMP1, $0, winUnFlow_Return	/* No need to */
	Nop							/* check from */
								/* kernel mode*/
	LD_MACH_CUR_STATE_PTR(VOL_TEMP1)
	rd_special	VOL_TEMP2, swp
	ld_32		VOL_TEMP1, VOL_TEMP1, $MACH_MIN_SWP_OFFSET
	Nop
	add_nt		VOL_TEMP1, VOL_TEMP1, $MACH_SAVED_WINDOW_SIZE
	cmp_br_delayed	ugt, VOL_TEMP2, VOL_TEMP1, winUnFlow_Return
	Nop
	/*
	 * Need to get more memory for window underflow.
	 */
	add_nt		NON_INTR_TEMP1, CUR_PC_REG, $0
#ifdef PATCH_IBUFFER
	/*
	 * Patch for IBUFFER.
	 *	Clear CUR_PC_REG so we can detect when interrupt trap
	 *	handling messes up.
	 */
	add_nt		CUR_PC_REG, r0, $0
#endif
	add_nt		NON_INTR_TEMP2, NEXT_PC_REG, $0
	rd_special	VOL_TEMP1, pc	/* Return from traps and then */
	return_trap	VOL_TEMP1, $12	/*   take the compare trap to */
	Nop				/*   get back in in kernel mode. */
	cmp_trap	always, r0, r0, $MACH_GET_WIN_MEM_TRAP
	Nop

winUnFlow_Return:
#ifdef PATCH_IBUFFER
	/*
	 * Patch for IBUFFER.
	 *	1) Clear CUR_PC_REG so we can detect when interrupt trap
	 *	   handling messes up.
	 *	2) invalidate Ibuffer so we don't get hit when we return.
	 */
	add_nt		SAFE_TEMP1, CUR_PC_REG, $0
	add_nt		CUR_PC_REG, r0, $0
	invalidate_ib
	jump_reg	SAFE_TEMP1, $0
#else
	jump_reg	CUR_PC_REG, $0
#endif
	return_trap	NEXT_PC_REG, $0
	Nop

/*
 *---------------------------------------------------------------------------
 *
 * FPUExcept --
 *
 *	FPU Exception handler.  Currently we take either a user or kernel error
 *	whichever is appropriate.
 *
 *---------------------------------------------------------------------------
 */
FPUExcept:
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, fpuExcept_KernError
	Nop
	USER_ERROR(MACH_USER_FPU_EXCEPT_TRAP)
	/* DOESN'T RETURN */
fpuExcept_KernError:
	CALL_DEBUGGER(r0, MACH_KERN_FPU_EXCEPT)

/*
 *---------------------------------------------------------------------------
 *
 * Illegal --
 *
 *	Illegal instruction handler.  Currently we take either a user or 
 * 	kernel error whichever is appropriate.
 *
 *---------------------------------------------------------------------------
 */
Illegal:
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, illegal_KernError
	Nop
	USER_ERROR(MACH_USER_ILLEGAL_TRAP)
	/* DOESN'T RETURN */
illegal_KernError:
	CALL_DEBUGGER(r0, MACH_KERN_ILLEGAL)

/*
 *---------------------------------------------------------------------------
 *
 * Fixnum --
 *
 *	Fixnum Exception handler.  Currently we take either a user or kernel
 *	error whichever is appropriate.
 *
 *---------------------------------------------------------------------------
 */
Fixnum:
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, fixnum_KernError
	Nop
	USER_ERROR(MACH_USER_FIXNUM_TRAP)
	/* DOESN'T RETURN */
fixnum_KernError:
	CALL_DEBUGGER(r0, MACH_KERN_FIXNUM)

/*
 *---------------------------------------------------------------------------
 *
 * Overflow --
 *
 *	Overflow fault handler.  Currently we take either a user or kernel
 *	error whichever is appropriate.
 *
 *---------------------------------------------------------------------------
 */
Overflow:
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, overflow_KernError
	Nop
	USER_ERROR(MACH_USER_OVERFLOW_TRAP)
	/* DOESN'T RETURN */
overflow_KernError:
	CALL_DEBUGGER(r0, MACH_KERN_OVERFLOW)


/*
 *---------------------------------------------------------------------------
 *
 * PowerUp --
 *
 *	Jump to power up sequencer.
 *
 *---------------------------------------------------------------------------
 */
PowerUp: 			/* Jump to power up sequencer */

/*
 *---------------------------------------------------------------------------
 *
 * Error --
 *
 *	Error handler.  Just call the debugger.
 *
 *---------------------------------------------------------------------------
 */
ErrorTrap:	
	SWITCH_TO_KERNEL_STACKS()
	CALL_DEBUGGER(r0, MACH_ERROR)

/*
 *---------------------------------------------------------------------------
 *
 * FaultIntr --
 *
 *	Handle a fault or an interrupt.
 *
 *---------------------------------------------------------------------------
 */
faultIntr_Const1:
	.long	MACH_KPSW_USE_CUR_PC
faultIntr_Const3:
	.long	MACH_FAULT_TRY_AGAIN
faultIntr_Const4:
	.long	MACH_FAULT_ILL_CACHE_OP
FaultIntr:
faultIntr_NormFault:
	/*
	 * On this type of trap we are supposed to return to the current 
	 * PC.
	 */
	rd_kpsw		KPSW_REG
	/*
 	 * Disable ibuffer in kernel. 
	 */
	and	VOL_TEMP1,KPSW_REG,$~(MACH_KPSW_IBUFFER_ENA|MACH_KPSW_PREFETCH_ENA);
	wr_kpsw	VOL_TEMP1,$0
	LD_PC_RELATIVE(SAFE_TEMP1, faultIntr_Const1)
	or		KPSW_REG, KPSW_REG, SAFE_TEMP1
	/*
	 * Read and clear the fault/error status register.
	 */
	READ_STATUS_REGS(MACH_FE_STATUS_0, SAFE_TEMP1)
	st_32		SAFE_TEMP1, r0, $feStatusReg
	WRITE_STATUS_REGS(MACH_FE_STATUS_0, SAFE_TEMP1)
	/*
	 * If no bits are set then it must be an interrupt.
	 */
	cmp_br_delayed	ne, SAFE_TEMP1, r0, noInterrupt
	Nop
	jump 	Interrupt
	Nop
noInterrupt:
	/*
	 * If any of the bits FEStatus<19:16> are set then is one of the
	 * four VM faults.  Store the fault type in a safe temporary and
	 * call the VMFault handler.
	 */
	extract		VOL_TEMP1, SAFE_TEMP1, $2
	and		VOL_TEMP1, VOL_TEMP1, $0xf
	cmp_br_delayed	eq, VOL_TEMP1, r0, noFault
	Nop
	jump		VMFault
	Nop
noFault:
	/*
	 * Handle the bus retry error.
	 */
	LD_PC_RELATIVE(SAFE_TEMP2, faultIntr_Const3)
	and		VOL_TEMP1, SAFE_TEMP1, SAFE_TEMP2
	cmp_br_delayed	eq, VOL_TEMP1, r0, 1f
	nop
#ifdef PATCH_IBUFFER
	/*
	 * Patch for IBUFFER.
	 *	Clear CUR_PC_REG so we can detect when interrupt trap
	 *	handling messes up.
	 */
	add_nt		SAFE_TEMP1,CUR_PC_REG,$0
	add_nt		CUR_PC_REG,r0,$0
	jump_reg	SAFE_TEMP1, $0
#else
	jump_reg	CUR_PC_REG, $0
#endif
	return_trap	NEXT_PC_REG, $0

1:
	/*
	 * If a user does an illegal cache op on a non-cachable page, we
	 * get a MACH_FAULT_ILL_CACHE_OP even if the user has no access to 
	 * the page. 
	 */

	LD_PC_RELATIVE(SAFE_TEMP2, faultIntr_Const4)
	and		VOL_TEMP1, SAFE_TEMP1, SAFE_TEMP2
	cmp_br_delayed	eq, VOL_TEMP1, r0, 2f
	Nop

	/*
	 * If the fault came from user mode map it into a protection fault.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, 2f
	Nop
	add_nt		VOL_TEMP1,r0,$(MACH_FAULT_PROTECTION >> 16)
	jump		VMFault
	Nop

2:
	/*
	 * Can't handle any of these types of faults.
	 */
	SWITCH_TO_KERNEL_STACKS()
	CALL_DEBUGGER(r0, MACH_BAD_FAULT)

/*
 *---------------------------------------------------------------------------
 *
 * Interrupt --
 *
 *	Handle an interrupt.
 *
 *---------------------------------------------------------------------------
 */
Interrupt:
	/*
	 * Read the interrupt status register and clear it.  The ISR is passed
	 * as an arg to the interrupt routine. Store the interrupt mask in
         * SAFE_TEMP3.
	 */
	READ_STATUS_REGS(MACH_INTR_STATUS_0, OUTPUT_REG1)
	READ_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP3)
	and		OUTPUT_REG1, OUTPUT_REG1, SAFE_TEMP3
	WRITE_STATUS_REGS(MACH_INTR_STATUS_0, OUTPUT_REG1)


#ifdef notdef
	/*
	 * Check to see if the interrupt happened on a test_and_set 
	 * instruction.  If so panic because this isn't supposed to
	 * happen.
	 */
	ld_32		VOL_TEMP1, CUR_PC_REG, r0
	nop
	extract		VOL_TEMP1, VOL_TEMP1, $3
	srl		VOL_TEMP1, VOL_TEMP1, $1
	and		VOL_TEMP1, VOL_TEMP1, $0xf0
	add_nt		VOL_TEMP2, r0, $0x30
	cmp_br_delayed	ge, VOL_TEMP1, VOL_TEMP2, interrupt_OK
	nop
	st_32		CUR_PC_REG, r0, $interruptPC
	or		KPSW_REG, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		KPSW_REG, $0
	cmp_trap	always, r0, r0, $MACH_CALL_DEBUGGER_TRAP

interrupt_OK:

#endif

	/* 
	 * Save the address of the interrupt for profiling. 
	 */
	st_32		CUR_PC_REG, r0, $_machInterruptAddr 

	/*
	 * The second argument is the kpsw.
	 */
	add_nt		OUTPUT_REG2, KPSW_REG, r0
	/*
	 * Save the insert register in a safe temporary and then
	 * disable all but non-maskable interrupts.  Don't enable all traps
	 * until after we have verified that the user's swp isn't bogus.
	 */
	rd_insert	SAFE_TEMP1
	ld_32		SAFE_TEMP2, r0, $_machNonmaskableIntrMask
	nop
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP2)
	/*
	 * See if took the interrupt from user mode.
	 */
	and		VOL_TEMP2, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP2, $0, interrupt_KernMode
	Nop
	/*
	 * We took the interrupt from user mode.
	 */
	VERIFY_SWP(interrupt_GoodSWP)
	jump	interrupt_BadSWP
	Nop

interrupt_GoodSWP:
	SAVE_USER_STATE()
	SWITCH_TO_KERNEL_STACKS()
	wr_kpsw		KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	call		_MachInterrupt
	Nop
#ifdef PATCH_IBUFFER
	add_nt		OUTPUT_REG1,KPSW_REG,$0
	call		_MachPatchUserModeIbuffer
	Nop
#endif
	/*
	 * Restore the insert register and the kpsw, enable interrupts and 
	 * then do a normal return from trap in case the user process needs to
	 * take some action. Insure that WRITE_STATUS_REGS is called with
	 * interrupts disabled.
	 */
	wr_insert	SAFE_TEMP1
	and		VOL_TEMP1, KPSW_REG, $~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP1, $0
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP3)
	wr_kpsw		KPSW_REG, $0
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
	jump		ReturnTrap
	Nop

interrupt_KernMode:
	/*
	 * Save all globals on the spill stack in case they get
	 * trashed.
	 */
	sub		SPILL_SP, SPILL_SP, $64
	st_40		r1, SPILL_SP, $0
	st_40		r2, SPILL_SP, $8
	st_40		r3, SPILL_SP, $16
	st_40		r5, SPILL_SP, $24
	st_40		r6, SPILL_SP, $32
	st_40		r7, SPILL_SP, $40
	st_40		r8, SPILL_SP, $48
	st_40		r9, SPILL_SP, $56
	/*
	 * Enable all traps and take the interrupt.
	 */
#ifdef PATCH_IBUFFER
	add_nt		VOL_TEMP1, r26, $0
#endif
	wr_kpsw		KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	call 		_MachInterrupt
	Nop
#ifdef PATCH_IBUFFER
	add_nt		OUTPUT_REG1,KPSW_REG,$0
	add_nt		OUTPUT_REG2,CUR_PC_REG,$0
	add_nt		OUTPUT_REG3,NEXT_PC_REG,$0
	add_nt		OUTPUT_REG4,VOL_TEMP1,$0
	call		_MachPatchIbuffer
	Nop
	add_nt		CUR_PC_REG,r27,$0
#endif
	/*
	 * Restore the globals and the spill sp.
	 */
	ld_40		r1, SPILL_SP, $0
	ld_40		r2, SPILL_SP, $8
	ld_40		r3, SPILL_SP, $16
	ld_40		r5, SPILL_SP, $24
	ld_40		r6, SPILL_SP, $32
	ld_40		r7, SPILL_SP, $40
	ld_40		r8, SPILL_SP, $48
	ld_40		r9, SPILL_SP, $56
	nop
	add_nt		SPILL_SP, SPILL_SP, $64

	/*
	 * Restore insert register and kpsw, enable interrupts and return.
	 */
	wr_insert	SAFE_TEMP1
	and		VOL_TEMP1, KPSW_REG, $~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP1, $0
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP3)
	or		KPSW_REG, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		KPSW_REG, $0
#ifdef PATCH_IBUFFER
	/*
	 * Patch for IBUFFER.
	 *	Clear CUR_PC_REG so we can detect when interrupt trap
	 *	handling messes up.
	 */
	add_nt		SAFE_TEMP1,CUR_PC_REG,$0
	add_nt		CUR_PC_REG,r0,$0
	jump_reg	SAFE_TEMP1, $0
#else
	jump_reg	CUR_PC_REG, $0
#endif
	return		NEXT_PC_REG, $0

interrupt_BadSWP:
	/*
	 * We have a bogus user swp.  Switch over to the kernel's stacks
	 * and take the interrupt.  After taking the interrupt kill the user
	 * process.
	 */
	SWITCH_TO_KERNEL_STACKS()
	wr_kpsw		KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	call		_MachInterrupt
	Nop
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP3)
#ifdef PATCH_IBUFFER
	/*
	 * Patch for IBUFFER.
	 *	Clear CUR_PC_REG so we can detect when interrupt trap
	 *	handling messes up.
	 */
	add_nt		CUR_PC_REG,r0,$0
#endif
        add_nt          OUTPUT_REG1, r0, $MACH_USER_BAD_SWP
	call		_MachUserError
	Nop
	/* DOESN'T RETURN */

/*
 *---------------------------------------------------------------------------
 *
 * VMFault --
 *
 *	Handle virtual memory faults.  The current fault type was stored
 *	in VOL_TEMP1 before we were called.
 *
 *---------------------------------------------------------------------------
 */
VMFault:
	add_nt		SAFE_TEMP1, VOL_TEMP1, $0
	/*
	 * Check kernel or user mode.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, vmFault_KernMode
	Nop
	/*
	 * Make sure that the saved window stack is OK.
	 */
	VERIFY_SWP(vmFault_GoodSWP)
	USER_SWP_ERROR()
	/* DOESN'T RETURN */

vmFault_GoodSWP:
	SAVE_USER_STATE()
	SWITCH_TO_KERNEL_STACKS()
	jump		vmFault_PC
	nop

vmFault_KernMode:
	/*
	 * Save all globals on the spill stack in case they get
	 * trashed.
	 */
	add_nt		SPILL_SP, SPILL_SP, $-64  /* Make sure of alignment */
	st_40		r1, SPILL_SP, $0
	st_40		r2, SPILL_SP, $8
	st_40		r3, SPILL_SP, $16
	st_40		r5, SPILL_SP, $24
	st_40		r6, SPILL_SP, $32
	st_40		r7, SPILL_SP, $40
	st_40		r8, SPILL_SP, $48
	st_40		r9, SPILL_SP, $56

vmFault_PC:
	/*
	 * Enable all traps.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	cmp_br_delayed	eq, SAFE_TEMP1, $MACH_VM_FAULT_DIRTY_BIT, vmFault_GetDataAddr
	nop
	/*
	 * Handle the fault on the PC first by calling
	 * MachVMPCFault(faultType, PC, kpsw).
	 */
	add_nt		OUTPUT_REG1, SAFE_TEMP1, $0
	add_nt		OUTPUT_REG2, CUR_PC_REG, $0
	add_nt		OUTPUT_REG3, KPSW_REG, $0
	rd_insert	VOL_TEMP1
	call		_MachVMPCFault
	nop
	wr_insert	VOL_TEMP1
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_NORM_RETURN, vmFault_GetDataAddr
	nop
	/*
	 * We got some sort of error.  If it was a user error then make sure
	 * that we do a normal return from trap so that the signal will
	 * be taken when it returns.  Otherwise leave the error code
	 * alone so that the kernel debugger will be called.
	 */
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_KERN_ACCESS_VIOL, 1f
	nop
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
1:	jump		vmFault_Return
	nop

vmFault_GetDataAddr:
	/*
	 * See if is a load, store or test-and-set instruction.  These types
	 * of instructions have opcodes less than 0x30.  If we find one
	 * of these then we have to extract the data address.
	 */
	FETCH_CUR_INSTRUCTION(SAFE_TEMP2)
	Nop
	extract		VOL_TEMP1, SAFE_TEMP2, $3  	/* Opcode <31:25> ->  */
							#	 <07:01>
	srl		VOL_TEMP1, VOL_TEMP1, $1	/* Opcode <07:01> -> */
							#	 <06:00>
	and		VOL_TEMP1, VOL_TEMP1, $0xf0	/* Get upper 4 bits. */
	add_nt		VOL_TEMP2, r0, $0x30
	cmp_br_delayed	lt, VOL_TEMP1, VOL_TEMP2, vmFault_IsData
	Nop
	jump		vmFault_Return
	nop
vmFault_IsData:
	/*
	 * Get the data address by calling ParseInstruction.  We pass the
	 * address to return to in VOL_TEMP1 and the instruction where we
	 * faulted at in VOL_TEMP2.  Disable interrupts because this routine
	 * screws around with windows.
	 */
	add_nt		VOL_TEMP2, SAFE_TEMP2, $0
	rd_insert	SAFE_TEMP2
	rd_kpsw		SAFE_TEMP3
	and		VOL_TEMP3, SAFE_TEMP3, $~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP3, $0
	/*
 	 * Restore the globals if we are coming from user mode.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, r0, vmFault_KernParse
	nop
	LD_MACH_CUR_STATE_PTR(VOL_TEMP1)
	nop
	ld_40		r1, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 8)
	ld_40		r2, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 16)
	ld_40		r3, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 24)
	ld_40		r4, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 32)
	ld_40		r5, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 40)
	ld_40		r6, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 48)
	ld_40		r7, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 56)
	ld_40		r8, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 64)
	ld_40		r9, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + 72)
	nop

	rd_special	VOL_TEMP1, pc
	add_nt		VOL_TEMP1, VOL_TEMP1, $16
	jump		ParseInstruction
	Nop

	LD_MACH_CUR_STATE_PTR(VOL_TEMP1)
	add_nt		VOL_TEMP2,OUTPUT_REG3,OUTPUT_REG4

	ld_32		SPILL_SP, VOL_TEMP1, $MACH_KERN_STACK_END_OFFSET

	jump		vmFault_CallDataFault
	nop

vmFault_KernParse:
	/*
	 * Restore globals since they could have gotten trashed while we
	 * called the C routines.
	 */
	ld_40		r1, SPILL_SP, $0
	ld_40		r2, SPILL_SP, $8
	ld_40		r3, SPILL_SP, $16
	ld_40		r5, SPILL_SP, $24
	ld_40		r6, SPILL_SP, $32
	ld_40		r7, SPILL_SP, $40
	ld_40		r8, SPILL_SP, $48
	ld_40		r9, SPILL_SP, $56

	rd_special	VOL_TEMP1, pc
	add_nt		VOL_TEMP1, VOL_TEMP1, $16
	jump		ParseInstruction
	Nop
	add_nt		VOL_TEMP2,OUTPUT_REG3,OUTPUT_REG4

vmFault_CallDataFault:
	wr_insert	SAFE_TEMP2
	wr_kpsw		SAFE_TEMP3, $0

	/*
	 * We now have the data address in VOL_TEMP2.  Call
	 * MachVMDataFault(faultType, PC, dataAddr, kpsw)
	 */
	add_nt		OUTPUT_REG1, SAFE_TEMP1, $0
	add_nt		OUTPUT_REG2, CUR_PC_REG, $0
	add_nt		OUTPUT_REG3, VOL_TEMP2, $0
	add_nt		OUTPUT_REG4, KPSW_REG, $0
	rd_insert	VOL_TEMP1
	call		_MachVMDataFault
	Nop
	wr_insert	VOL_TEMP1

vmFault_Return:
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	ne, VOL_TEMP1, r0, 1f
	nop
	/*
	 * Restore the globals and the spill sp.
	 */
	ld_40		r1, SPILL_SP, $0
	ld_40		r2, SPILL_SP, $8
	ld_40		r3, SPILL_SP, $16
	ld_40		r5, SPILL_SP, $24
	ld_40		r6, SPILL_SP, $32
	ld_40		r7, SPILL_SP, $40
	ld_40		r8, SPILL_SP, $48
	ld_40		r9, SPILL_SP, $56
	nop
	add_nt		SPILL_SP, SPILL_SP, $64
1:
	jump		ReturnTrap
	Nop

/*
 *--------------------------------------------------------------------------
 *
 * CmpTrap --
 *
 *	Handle a cmp_trap trap.  This involves determining the trap type
 *	and vectoring to the right spot to handle the trap.
 *
 *--------------------------------------------------------------------------
 */
cmpTrap_Const1:
	.long	~MACH_KPSW_USE_CUR_PC
CmpTrap:
	/*
	 * On this type of trap we are supposed to return to next PC instead
	 * of cur PC.
	 */
	rd_kpsw		KPSW_REG
	/*
 	 * Disable ibuffer in kernel. 
	 */
	and	VOL_TEMP1,KPSW_REG,$~(MACH_KPSW_IBUFFER_ENA|MACH_KPSW_PREFETCH_ENA);
	wr_kpsw	VOL_TEMP1,$0
	LD_PC_RELATIVE(SAFE_TEMP1, cmpTrap_Const1)
	and		KPSW_REG, KPSW_REG, SAFE_TEMP1

	/*
 	 * Clear the FE_STATUS registers incase there is a fault pending
	 * causes by the fetch of the instuction after the cmp_trap.
	 */
	add_nt		SAFE_TEMP1,r0,$-1	
	WRITE_STATUS_REGS(MACH_FE_STATUS_0, SAFE_TEMP1)
	/*
	 * Get the trap number.
	 */
	FETCH_CUR_INSTRUCTION(SAFE_TEMP1)
	Nop
	and		SAFE_TEMP1, SAFE_TEMP1, $0x1ff 
	/*
	 * If is one of the special user traps then handle these 
	 * specially and as quickly as possible.
	 */
	cmp_br_delayed	lt, SAFE_TEMP1, $MACH_USER_RET_TRAP_TRAP, 1f
	nop
	jump		SpecialUserTraps
	nop
1:
	/* 
	 * See if are coming from kernel mode or not.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, cmpTrap_CheckType
	Nop
	/*
	 * Verify the SWP for user processes.
	 */
	VERIFY_SWP(cmpTrap_SaveState)
	USER_SWP_ERROR()
	/* DOESN'T RETURN */

cmpTrap_SaveState:
	cmp_br_delayed	eq, SAFE_TEMP1, $MACH_SIG_RETURN_TRAP, cmpTrap_CallFunc
	Nop
	SAVE_USER_STATE()
	SWITCH_TO_KERNEL_STACKS()

cmpTrap_CheckType:
	cmp_br_delayed	gt, SAFE_TEMP1, $MACH_MAX_TRAP_TYPE, cmpTrap_BadTrapType
	Nop

cmpTrap_CallFunc:
	sll		VOL_TEMP1, SAFE_TEMP1, $3	/* Multiple by 8 to */
							/*   get offset */
	rd_special	VOL_TEMP2, pc
	add_nt		VOL_TEMP2, VOL_TEMP2, $16
	jump_reg	VOL_TEMP2, VOL_TEMP1
	Nop
	jump		BreakpointTrap
	Nop
	jump		SingleStepTrap
	Nop
	jump		CallDebuggerTrap
	Nop
	jump		cmpTrap_RefreshTrap
	Nop
	jump		SysCallTrap	
	Nop
	jump		SigReturnTrap
	Nop
	jump		GetWinMemTrap
	Nop
	jump		cmpTrap_SaveStateTrap
	nop
	jump		cmpTrap_RestoreStateTrap
	nop
	jump		cmpTrap_TestAndSetTrap
	nop
	jump		cmpTrap_UserCSTrap
	nop
	jump		cmpTrap_BadSWPTrap
	nop
	jump		cmpTrap_CmpTrapTrap
	nop
	jump		cmpTrap_FPUTrap
	Nop
	jump		cmpTrap_IllegalTrap
	Nop
	jump		cmpTrap_FixnumTrap
	Nop
	jump		cmpTrap_OverflowTrap
	Nop

cmpTrap_BadSWPTrap:
	add_nt		OUTPUT_REG1, r0, $MACH_USER_BAD_SWP
	jump		UserErrorTrap
	Nop

	/*
	 * A user context switch trap.  We were called as
	 *
	 *	UserCS(swpBaseAddr, swpMaxAddr, newSWP)
	 *	    Address	swpBaseAddr;	* Base of saved window stack.
	 *	    Address	swpMaxAddr;	* Max addr of saved window
	 *					* stack.
	 *	    Address	newSWP;		* The new swp.
	 */

cmpTrap_CmpTrapTrap:
	add_nt		OUTPUT_REG1, r0, $MACH_BAD_TRAP_TYPE
	jump		UserErrorTrap
	Nop

cmpTrap_FPUTrap:
	add_nt		OUTPUT_REG1, r0, $MACH_USER_FPU_EXCEPT
	jump		UserErrorTrap
	Nop

cmpTrap_IllegalTrap:
	add_nt		OUTPUT_REG1, r0, $MACH_USER_ILLEGAL
	jump		UserErrorTrap
	Nop

cmpTrap_FixnumTrap:
	add_nt		OUTPUT_REG1, r0, $MACH_USER_FIXNUM
	jump		UserErrorTrap
	Nop

cmpTrap_OverflowTrap:
	add_nt		OUTPUT_REG1, r0, $MACH_USER_OVERFLOW
	jump		UserErrorTrap
	Nop
cmpTrap_UserCSTrap:
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	add_nt		OUTPUT_REG1, INPUT_REG1, $0
	add_nt		OUTPUT_REG2, INPUT_REG2, $0
	add_nt		OUTPUT_REG3, INPUT_REG3, $0
	call		_MachUserContextSwitch
	nop
	jump		ReturnTrap
	nop

	/*
	 * Save the user's state in the special page.
	 */
cmpTrap_SaveStateTrap:
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	call		_MachSaveUserState
	nop
	jump		ReturnTrap
	nop

	/*
	 * Restore the user's state from the special page.
	 */
cmpTrap_RestoreStateTrap:
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	call		_MachRestoreUserState
	nop
	jump		ReturnTrap
	nop

	/* 
	 * User level test and set.  We were passed the address to set.
	 */
cmpTrap_TestAndSetTrap:
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	add_nt		OUTPUT_REG1, INPUT_REG1, $0
	call		_MachUserTestAndSet
	nop
	jump		ReturnTrap
	nop

cmpTrap_RefreshTrap:
	rd_special	VOL_TEMP1, upsw
	nop
	nop
	wr_special	upsw, VOL_TEMP1, $0
	rd_special	VOL_TEMP1, swp
	nop
	nop
	wr_special	swp, VOL_TEMP1, $0
	rd_insert	VOL_TEMP1 
	nop
	nop
	wr_insert	VOL_TEMP1

	wr_kpsw		KPSW_REG, $0
	return_trap	NEXT_PC_REG, $0
	Nop

cmpTrap_BadUserTrap:
	/*
	 * Error handling user traps.
	 */
	add_nt		OUTPUT_REG1, r0, $MACH_BAD_TRAP_TYPE
	jump		UserErrorTrap
	Nop

cmpTrap_BadTrapType:
	/*
	 * A trap type greater than the maximum value was specified.  If
	 * in kernel mode call the debugger.  Otherwise call the user error
	 * routine.
	 */
	and		VOL_TEMP2, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP2, $0, cmpTrap_KernError
	Nop
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	add_nt		OUTPUT_REG1, r0, $MACH_BAD_TRAP_TYPE
	call		_MachUserError
	Nop
	jump		ReturnTrap
	Nop

cmpTrap_KernError:
	CALL_DEBUGGER(r0, MACH_BAD_TRAP_TYPE)

/*
 *----------------------------------------------------------------------------
 *
 * BreakpointTrap --
 *
 *	Handle a breakpoint trap.
 *
 *----------------------------------------------------------------------------
 */
BreakpointTrap:
	/*
	 * We just took a breakpoint trap.  If this is a user trap then 
	 * call the user error routine.  Otherwise call the kernel debugger.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, breakpoint_KernBP
	Nop
	/*
	 * Enable all traps and call the user error routine.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	add_nt		OUTPUT_REG1, r0, $MACH_BREAKPOINT
	rd_insert	VOL_TEMP1
	call		_MachUserError
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * Do a return from trap.
	 */
	jump		ReturnTrap
	Nop

breakpoint_KernBP:
	CALL_DEBUGGER(r0, MACH_BREAKPOINT);

/*
 *----------------------------------------------------------------------------
 *
 * SingleStepTrap --
 *
 *	Handle a single step trap.
 *
 *----------------------------------------------------------------------------
 */
SingleStepTrap:
	/*
	 * We just took a single step trap.  If this is a user trap then 
	 * call the user error routine.  Otherwise call the kernel debugger.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, singleStep_KernSS
	Nop
	/*
	 * Enable all traps and call the user error routine.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	add_nt		OUTPUT_REG1, r0, $MACH_SINGLE_STEP
	rd_insert	VOL_TEMP1
	call		_MachUserError
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * Do a return from trap.
	 */
	jump		ReturnTrap
	Nop

singleStep_KernSS:
	CALL_DEBUGGER(r0, MACH_SINGLE_STEP);

/*
 *----------------------------------------------------------------------------
 *
 * CallDebuggerTrap --
 *
 *	Handle a call debugger trap.
 *
 *----------------------------------------------------------------------------
 */
CallDebuggerTrap:
	/*
	 * We just took a breakpoint trap.  If this is a user trap then 
	 * call the user error routine.  Otherwise call the kernel debugger.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, callDebugger_KernBP
	Nop
	/*
	 * Enable all traps and call the user error routine.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	add_nt		OUTPUT_REG1, r0, $MACH_CALL_DEBUGGER
	rd_insert	VOL_TEMP1
	call		_MachUserError
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * Do a return from trap.
	 */
	jump		ReturnTrap
	Nop

callDebugger_KernBP:
	add_nt		CUR_PC_REG, NEXT_PC_REG, $0
	add_nt		NEXT_PC_REG, r0, $0
	CALL_DEBUGGER(r0, MACH_CALL_DEBUGGER);

/*
 *----------------------------------------------------------------------------
 *
 * SysCallTrap -
 *
 *	Handle a system call trap.
 *
 *----------------------------------------------------------------------------
 */
SysCallTrap:
	/* 
	 * Enable all traps.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	/*
	 * Get the type of system call.  It was stored in the callers 
	 * r16 before they trapped.
	 */
	LD_MACH_CUR_STATE_PTR(SAFE_TEMP1)
	Nop
	ld_32		SAFE_TEMP2, SAFE_TEMP1, $(MACH_TRAP_REGS_OFFSET + (8 * 16))
	/*
	 * Check number of kernel call for validity.
	 */
	ld_32		VOL_TEMP1, r0, $_machMaxSysCall
	Nop
	cmp_br_delayed	ule, SAFE_TEMP2, VOL_TEMP1, sysCallTrap_FetchArgs
	Nop
	/*
	 * Bad kernel call.  Take a user error and then do a normal return
	 * from trap.
	 */
	add_nt		OUTPUT_REG1, r0, $MACH_BAD_SYS_CALL
	call		_MachUserError
	Nop
	jump		ReturnTrap
	Nop

sysCallTrap_FetchArgs:
	/*
	 * This is a good system call.  Fetch the arguments.  The first 5
	 * come from the input registers and the rest from the spill
	 * stack.  First fetch the first 5 regardless whether we need them
	 * or not since its so cheap.
	 */
	add_nt		OUTPUT_REG1, INPUT_REG1, $0
	add_nt		OUTPUT_REG2, INPUT_REG2, $0
	add_nt		OUTPUT_REG3, INPUT_REG3, $0
	add_nt		OUTPUT_REG4, INPUT_REG4, $0
	add_nt		OUTPUT_REG5, INPUT_REG5, $0
	/*
	 * Now find out how many args there truly are which is
	 *
	 * 	int numArgs = machNumArgs[sys_call_type]
	 *
	 * For this code the registers are used in the following way:
	 *
	 *	SAFE_TEMP1:	machCurStatePtr
	 *	SAFE_TEMP2:	sysCallType
	 *	SAFE_TEMP3:	numArgs
	 *	VOL_TEMP1:	user stack pointer
	 *	VOL_TEMP2:	sysCallType * 4
	 */
	ld_32		VOL_TEMP3, r0, $numArgsPtr
	Nop
	sll		VOL_TEMP2, SAFE_TEMP2, $2
	ld_32		SAFE_TEMP3, VOL_TEMP3, VOL_TEMP2
	Nop
	/*
	 * If num args is less than or equal to 5 then we are done.
	 */
	cmp_br_delayed	le, SAFE_TEMP3, $5, sysCallTrap_CallRoutine
	Nop
	/*
	 * Haven't fetched enough args so we have to copy them from the user's
	 * spill stack to the kernel's spill stack.  Set VOL_TEMP1 to be
	 * the user stack pointer + (numArgs - 5) * 8 to point to the base
	 * of the arguments on the spill stack.
	 */
	ld_32		VOL_TEMP1, SAFE_TEMP1, $(MACH_TRAP_REGS_OFFSET + (8 * MACH_SPILL_SP))
	sub		VOL_TEMP3, SAFE_TEMP3, $5
	sll		VOL_TEMP3, VOL_TEMP3, $3
	add		VOL_TEMP1, VOL_TEMP1, VOL_TEMP3
	/*
	 * Move back the kernel's spill sp to cover the args that we are
	 * about to copy onto it. 
	 */
	add_nt		NON_INTR_TEMP1, SPILL_SP, $0
	sub		SPILL_SP, SPILL_SP, VOL_TEMP3
	/*
	 * Now fetch the args.  This code fetches up to 7 more args by jumping
	 * into the right spot in the sequence.  The spot to jump is 
	 * equal to 16 * (12 - numArgs) where 16 is the number of bytes
	 * required for each move of data.  For the following code
	 * the register conventions are:
	 *
	 *	SAFE_TEMP1:	machCurStatePtr
	 *	SAFE_TEMP2:	sysCallType
	 *	SAFE_TEMP3:	(12 - numArgs) * 16
	 *	VOL_TEMP1:	User spill SP
	 *	VOL_TEMP2:	Temporary.
	 *	VOL_TEMP3:	Spill stack size.
	 *	NON_INTR_TEMP1:	Kernel SP
	 */
	add_nt		VOL_TEMP2, r0, $12
	sub		SAFE_TEMP3, VOL_TEMP2, SAFE_TEMP3
	sll		SAFE_TEMP3, SAFE_TEMP3, $3
	sll		SAFE_TEMP3, SAFE_TEMP3, $1
	rd_special	VOL_TEMP2, pc
	add_nt		VOL_TEMP2, SAFE_TEMP3, VOL_TEMP2
	jump_reg	VOL_TEMP2, $16
	Nop
	.globl	_MachFetchArgStart
_MachFetchArgStart:
	ld_32		VOL_TEMP2, VOL_TEMP1, $-56	/* 7 args */
	Nop
	st_32		VOL_TEMP2, NON_INTR_TEMP1, $-56
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-48	/* 6 args */
	Nop
	st_32		VOL_TEMP2, NON_INTR_TEMP1, $-48
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-40	/* 5 args */
	Nop
	st_32		VOL_TEMP2, NON_INTR_TEMP1, $-40
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-32	/* 4 args */
	Nop
	st_32		VOL_TEMP2, NON_INTR_TEMP1, $-32
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-24	/* 3 args */
	Nop
	st_32		VOL_TEMP2, NON_INTR_TEMP1, $-24
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-16	/* 2 args */
	Nop
	st_32		VOL_TEMP2, NON_INTR_TEMP1, $-16
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-8	/* 1 arg */
	Nop
	st_32		VOL_TEMP2, NON_INTR_TEMP1, $-8
	Nop
	.globl _MachFetchArgEnd
_MachFetchArgEnd:

sysCallTrap_CallRoutine:
	/*
	 * Now call the routine to handle the system call.  We do this
	 * by jumping through 
	 *
	 *	(proc_RunningProcesses[0]->kcallTable[sysCallType])()
	 */

	/* 
	 * VOL_TEMP1 <= proc_RunningProccesses[0]
	 */
	ld_32		VOL_TEMP1, r0, $runningProcesses
	Nop						
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	Nop
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	Nop
	/*
	 * VOL_TEMP2 <= offset of kcall table pointer in PCB
	 */
	ld_32		VOL_TEMP2, r0, $_machKcallTableOffset
	Nop						
	/*
	 * VOL_TEMP2 <= pointer to Oth entry in kcall table
	 */
	add_nt		VOL_TEMP1, VOL_TEMP1, VOL_TEMP2
	ld_32		VOL_TEMP2, VOL_TEMP1, $0
	Nop
	/*
	 * Call the routine.  Note that SPUR doesn't have a call_reg 
	 * instruction.  This forces us to advance the window by a call
	 * and then do a jump_reg.  Unfortunately this means that we can't use
	 * any temporaries in the current window so we have to use a global.
	 * The global that we use is r9.  However, since we restore user
	 * state before we return to user mode r9 will get restored.
	 */
	sll		VOL_TEMP3, SAFE_TEMP2, $2
	ld_32		r9, VOL_TEMP2, VOL_TEMP3
	Nop
	call		1f
	Nop
1:
	rd_special	RETURN_ADDR_REG, pc
	add_nt		RETURN_ADDR_REG, RETURN_ADDR_REG, $8
	jump_reg	r9, $0
	Nop

SysCall_Return:
	/*
	 * Now we are back from the system call handler.  Store the return
	 * value into the return val reg for the parent, restore the kernels
	 * stack pointer and the kpsw and then do a normal return from
	 * trap.
	 */
	st_32		RETURN_VAL_REG, SAFE_TEMP1, $(MACH_TRAP_REGS_OFFSET + (8 * MACH_RETURN_VAL_REG))
	ld_32		SPILL_SP, SAFE_TEMP1, $MACH_KERN_STACK_END_OFFSET
	Nop
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
	jump		ReturnTrap
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * UserErrorTrap -
 *
 *	Handle a user error trap.  Before we were called the old CUR_PC_REG 
 *	and NEXT_PC_REG were saved in NON_INTR_TEMP2 and NON_INTR_TEMP3 
 *	respectively and the error type was stored in OUTPUT_REG1.
 *
 *----------------------------------------------------------------------------
 */
UserErrorTrap:
	add_nt		CUR_PC_REG, NON_INTR_TEMP1, $0
	add_nt		NEXT_PC_REG, NON_INTR_TEMP2, $0
	LD_MACH_CUR_STATE_PTR(VOL_TEMP1)
	Nop
	st_32		CUR_PC_REG, VOL_TEMP1, $MACH_TRAP_CUR_PC_OFFSET
	st_32		NEXT_PC_REG, VOL_TEMP1, $MACH_TRAP_NEXT_PC_OFFSET
	/*
	 * Enable all traps.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	/*
	 * Call the user error handler.
	 */
	rd_insert	VOL_TEMP1
	call		_MachUserError
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * Do a normal return from trap.
	 */
	jump		ReturnTrap
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * SigReturnTrap -
 *
 *	Return from signal trap.  The previous window contains
 *	the saved info when we called the signal handler.  Note that we
 *	have not switched to the kernel's stacks.  This is because we
 *	need to go back one window before we save user state.
 *
 *----------------------------------------------------------------------------
 */
sigReturnTrap_Const1:
	.long MACH_KPSW_USE_CUR_PC
SigReturnTrap:
	/*
	 * Switch over to the kernel stacks after saving the user's spill sp
	 * and the kpsw.  We don't need to save any other state because we
	 * will restore it from off of the user's spill stack.  We save the
	 * kpsw because we can't trust the kpsw on the user's stack.
	 */
	LD_MACH_CUR_STATE_PTR(VOL_TEMP1)
	Nop
	st_32		SPILL_SP, VOL_TEMP1, $(MACH_TRAP_REGS_OFFSET + (8 * 4))
	st_32		KPSW_REG, VOL_TEMP1, $MACH_TRAP_KPSW_OFFSET
	SWITCH_TO_KERNEL_STACKS()
	/*
	 * Reenable traps and call the routine to handle returns from 
	 * signals.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	call		_MachSigReturn
	Nop

	jump		ReturnTrap
	Nop


/*
 *----------------------------------------------------------------------------
 *
 * GetWinMemTrap --
 *
 *	Get more memory for the window stack.  The current and next PCs were
 *	stored in NON_INTR_TEMP1 and NON_INTR_TEMP2 respectively before
 *	we were trapped to.
 *
 *----------------------------------------------------------------------------
 */
getWinMemTrap_Const1:
	.long	MACH_KPSW_USE_CUR_PC
GetWinMemTrap:
	/*
	 * Enable all traps.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	/*
	 * Call _MachGetWinMem()
	 */
	call		_MachGetWinMem
	Nop
	/*
	 * Store the current and next PCs into the saved state for this
	 * process because the ReturnTrap will restore the user state.
	 * Also store the kpsw that indicates that we should
	 * return to the current PC instead of the next one.
	 */

	LD_MACH_CUR_STATE_PTR(SAFE_TEMP1)
	Nop
	st_32		NON_INTR_TEMP1, SAFE_TEMP1, $MACH_TRAP_CUR_PC_OFFSET
	st_32		NON_INTR_TEMP2, SAFE_TEMP1, $MACH_TRAP_NEXT_PC_OFFSET
	LD_PC_RELATIVE(SAFE_TEMP2, getWinMemTrap_Const1)
	or		KPSW_REG, KPSW_REG, SAFE_TEMP2
	st_32		KPSW_REG, SAFE_TEMP1, $MACH_TRAP_KPSW_OFFSET
	/*
	 * Do a normal return from trap.
	 */
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
	jump		ReturnTrap
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * SpecialUserTraps --
 *
 *	Handle special user traps.  The compare trap type was passed in
 *	register SAFE_TEMP1.  There are three types of traps:
 *	
 *	1) Return from user trap hander trap
 *	2) One of FPU, illegal, fixnum and overflow
 *	3) Compare traps.
 *
 *----------------------------------------------------------------------------
 */

SpecialUserTraps:
	/*
	 * The trap to return from a user trap hander is handled differently
	 * then the other special user traps.
	 */
	cmp_br_delayed	eq, SAFE_TEMP1, $MACH_USER_RET_TRAP_TRAP, UserRetTrapTrap
	nop
	/*
	 * On the four traps that happened in user mode we stored the
	 * current pc and next pc in the temporaries and then we trapped
	 * back into the kernel.
	 */
	cmp_br_delayed	gt, SAFE_TEMP1, $MACH_USER_OVERFLOW_TRAP, 1f
	nop
	add_nt		CUR_PC_REG, NON_INTR_TEMP1, $0
	add_nt		NEXT_PC_REG, NON_INTR_TEMP2, $0
1:
/*
 * Recover the operands of the trapping instruction 
 */

	/*
	 * Enable traps and fetch the current instruction.  We have to
	 * enable all traps because fetching the instruction could cause
	 * a fault.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	ld_32		VOL_TEMP2, CUR_PC_REG, $0
	/*
	 * Leave all traps enabled but disable interrupts because are going
	 * to be mucking register windows in ParseInstruction
	 */
	and		VOL_TEMP1, VOL_TEMP1, $~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP1, $0
	
	rd_special	VOL_TEMP1, pc
	add_nt		VOL_TEMP1, VOL_TEMP1, $16
	jump		ParseInstruction
	nop
	/*
	 * We now have the operands.  Switch back to user mode.
	 */
	or		KPSW_REG, KPSW_REG, $(MACH_KPSW_CUR_MODE|MACH_KPSW_ALL_TRAPS_ENA)
	wr_kpsw		KPSW_REG, $0
	/*
	 * We are now in user mode so we can determine what trap handler
	 * to call for this trap.  We already have the trap type in
	 * SAFE_TEMP1.  Now we want to determine the traps offset into the
	 * trap table.  The first four entries of the table are for
	 * fpu, illegal, fixnum and overflow exceptions.  The following
	 * entry is the first compare trap type.  The following code
	 * sets SAFE_TEMP1 to be equal to the index into the trap table for
	 * the given trap type.
	 */
	add_nt		SAFE_TEMP2, SAFE_TEMP1, $0
	cmp_br_delayed	gt, SAFE_TEMP1, $MACH_USER_OVERFLOW_TRAP, 1f
	nop
	/*
	 * This is one of the 4 traps in addition to the compare traps.
	 */
	sub		SAFE_TEMP1, SAFE_TEMP1, $MACH_USER_FPU_EXCEPT_TRAP
	jump		2f
	nop
1:
	/* 
	 * We have a compare trap.
	 */
	add_nt		VOL_TEMP1, r0, $MACH_LAST_USER_CMP_TRAP
	cmp_br_delayed	gt, SAFE_TEMP1, VOL_TEMP1, specialUserTraps_Error
	nop
	add_nt		VOL_TEMP1, r0, $MACH_FIRST_USER_CMP_TRAP
	cmp_br_delayed	lt, SAFE_TEMP1, VOL_TEMP1, specialUserTraps_Error
	nop
	sub		SAFE_TEMP1, SAFE_TEMP1, $(MACH_FIRST_USER_CMP_TRAP - MACH_NUM_OTHER_USER_TRAPS)

2:
	/*
	 * Attempt to call the trap handler.  We do this by getting 
	 * a pointer to the trap table and then looking at the handler there.
	 * If the handler is non-zero then it is a valid handler.  Otherwise
	 * we have a bad user trap type.
	 */
	LD_MACH_CUR_STATE_PTR(VOL_TEMP1)
	ld_32		VOL_TEMP2, r0, $_machTrapTableOffset
	ld_32		VOL_TEMP1, VOL_TEMP1, $MACH_SPEC_PAGE_ADDR_OFFSET
	nop
	add		VOL_TEMP1, VOL_TEMP1, VOL_TEMP2
	/*
	 * VOL_TEMP1 now contains the address of the base of the trap table.
	 * See if there is anything to call.  Each entry is 8 bytes long
	 * so we shift SAFE_TEMP1 (the trap table index) by 3 to get the
	 * trap table offset.
	 */
	sll		SAFE_TEMP1, SAFE_TEMP1, $3
	ld_32		SAFE_TEMP1, VOL_TEMP1, SAFE_TEMP1
	Nop
	cmp_br_delayed	eq, SAFE_TEMP1, $0, specialUserTraps_Error
	nop
	/*
	 * We now know that we have a good handler to call.  Set up
	 * the args and call it.  We call the routine as:
	 *
	 *	Handler(curPC, nextPC, opcode, destReg, operands)
	 *	
	 * where operands is a pointer to the stack and the two 
	 * source operands.  
	 */
	add_nt		r25,SPILL_SP,$0              /* Follow C protocol */
	add_nt		OUTPUT_REG5,SAFE_TEMP1,$0    /* Put the handle
							pointer where we 
	                                                can get at it.  */
	call		1f                          /* Bump the windows */
	add_nt		SPILL_SP, SPILL_SP, $-16      /* Make room on stack */

						/* Our operands are now
						   in the input registers */
1:	st_40		INPUT_REG3, SPILL_SP, $0    /* First operand */
	st_40		INPUT_REG4, SPILL_SP, $8    /* Second operand */
	add_nt		OUTPUT_REG3, INPUT_REG1,$0  /* opcode */
	add_nt		OUTPUT_REG4, INPUT_REG2,$0  /* destReg */
	add_nt		OUTPUT_REG1, CUR_PC_REG, $0
	add_nt		OUTPUT_REG2, NEXT_PC_REG, $0
	add_nt		OUTPUT_REG5, SPILL_SP, $0
	jump_reg	INPUT_REG5, $0
	nop

specialUserTraps_Error:
	/*
	 * We got an error trying to call the trap handler.  Trap back
	 * into the kernel after shifting our window back to the original
	 * window that we were executing in so we reenter kernel in a normal 
	 * trap state.  Since we are shifting our window back and we don't 
	 * want to lose the current and next PC's we put those in the
	 * non-intr-temp registers so they don't get trashed in case
	 * an interrupt sneaks in.
	 */
	add_nt		SPILL_SP, SPILL_SP, $-16
	st_32		SAFE_TEMP2, SPILL_SP, $0   /* Save the trap type */
	add_nt		NON_INTR_TEMP1, CUR_PC_REG, $0
	add_nt		NON_INTR_TEMP2, NEXT_PC_REG, $0
	rd_special	VOL_TEMP1, pc
	return		VOL_TEMP1, $12
	nop
	/*
	 * We use r1 as the trap type.  Since we are in the window that 
	 * caused the error we can't use a local.
	 */
	st_40		r1, SPILL_SP, $8
	ld_32		r1, SPILL_SP, $0  /* Get the trap type back */
	/* 
	 * Set our first output register to the type of error and trap.
	 */
	nop
	cmp_br_delayed	ne, r1, $MACH_USER_FPU_EXCEPT_TRAP, 1f
	nop
	ld_40		r1, SPILL_SP, $8
	add_nt		SPILL_SP, SPILL_SP, $16
	cmp_trap	always, r0, r0, $MACH_FPU_ERROR_TRAP
1:
	cmp_br_delayed	ne, r1, $MACH_USER_ILLEGAL_TRAP, 1f
	nop
	ld_40		r1, SPILL_SP, $8
	add_nt		SPILL_SP, SPILL_SP, $16
	cmp_trap	always, r0, r0, $MACH_ILLEGAL_ERROR_TRAP
1:
	cmp_br_delayed	ne, r1, $MACH_USER_FIXNUM_TRAP, 1f
	nop
	ld_40		r1, SPILL_SP, $8
	add_nt		SPILL_SP, SPILL_SP, $16
	cmp_trap	always, r0, r0, $MACH_FIXNUM_ERROR_TRAP
1:
	cmp_br_delayed	ne, r1, $MACH_USER_OVERFLOW_TRAP, 1f
	nop
	ld_40		r1, SPILL_SP, $8
	add_nt		SPILL_SP, SPILL_SP, $16
	cmp_trap	always, r0, r0, $MACH_OVERFLOW_ERROR_TRAP
1:
	ld_40		r1, SPILL_SP, $8
	add_nt		SPILL_SP, SPILL_SP, $16
	cmp_trap	always, r0, r0, $MACH_CMP_TRAP_ERROR_TRAP

/*
 *----------------------------------------------------------------------------
 *
 * UserRetTrapTrap --
 *
 *	Return a value from a user trap handler.  We were passed the value
 *	to return in INPUT_REG1, the first pc in INPUT_REG2, the next pc in 
 *	INPUT_REG3 and the register to set in INPUT_REG4.  The window
 *	that we want to put the value in is three windows back.
 *
 *----------------------------------------------------------------------------
 */
UserRetTrapTrap:
	/*
	 * Enable all traps and disable interrupts so that we can pop a few
	 * windows.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	and		VOL_TEMP1, VOL_TEMP1, $~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP1, $0
	/*
	 * Pop the first window.  This will get us back into the window that
	 * the trap handler was executing in.
	 */
	rd_special	VOL_TEMP1, pc
	return		VOL_TEMP1, $12
	nop
	/*
	 * Move our 4 values into our input registers and go back yet another
	 * window.
	 */
	add_nt		INPUT_REG1, OUTPUT_REG1, $0
	add_nt		INPUT_REG2, OUTPUT_REG2, $0
	add_nt		INPUT_REG3, OUTPUT_REG3, $0
	add_nt		INPUT_REG4, OUTPUT_REG4, $0
	rd_special	VOL_TEMP1, pc
	return		VOL_TEMP1, $12
	nop
	/*
	 * Now we are in the window after the one where the trap occured.  
	 * Store to the register in the previous window.  r9 is the value
	 * to set and r1 is the register to set.
	 */
	add_nt		VOL_TEMP1, r9, $0
	add_nt		VOL_TEMP2, r1, $0
	add_nt		r9, OUTPUT_REG1, $0
	add_nt		r1, OUTPUT_REG4, $0
	rd_special	VOL_TEMP3, pc
	return		VOL_TEMP3, $12
	nop
	sll		r1, r1, $2
	jump_reg	r1, $SetReg	
	jump		1f
1:
	/*
	 * Go back to the current next window.
	 */
	call		1f
	nop
1:
	add_nt		r9, VOL_TEMP1, $0
	add_nt		r1, VOL_TEMP2, $0
	/*
	 * We now can finally do a normal return.   If the next PC
	 * (OUTPUT_REG3) is zero then return to just the current PC.
	 */
	rd_kpsw		VOL_TEMP1
	or		VOL_TEMP1, VOL_TEMP1, $(MACH_KPSW_INTR_TRAP_ENA|MACH_KPSW_CUR_MODE)
	wr_kpsw		VOL_TEMP1, $0
	cmp_br_delayed	eq, OUTPUT_REG3, $0, 1f
	nop
	jump_reg	OUTPUT_REG2, $0
	return		OUTPUT_REG3, $0
1:
	return		OUTPUT_REG2, $0
	nop

/*
 *----------------------------------------------------------------------------
 *
 * ReturnTrap -
 *
 *	Return from a trap handler.  We are called with all traps disabled
 *	and if we are a user process then we are running on the kernel's spill
 *	statck.   Assume that the type of return to do has been stored in
 *	RETURN_VAL_REG.  If it is not one of MACH_NORM_RETURN,
 *	MACH_FAILED_COPY or MACH_FAILED_ARG_FETCH then it is a kernel error
 *	value.
 *
 *----------------------------------------------------------------------------
 */
returnTrap_Const1:
	.long	MACH_KPSW_USE_CUR_PC

ReturnTrap:
	/*
	 * Restore the kpsw to that which we trapped with.
	 */
	wr_kpsw		KPSW_REG, $0

	/*
	 * Check the return code from the fault handler.
	 */
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_NORM_RETURN, returnTrap_NormReturn
	Nop
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_FAILED_COPY, returnTrap_FailedCopy
	Nop
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_FAILED_ARG_FETCH, returnTrap_FailedArgFetch
	Nop
	CALL_DEBUGGER(RETURN_VAL_REG, 0)

returnTrap_NormReturn:
	/*
	 * If we are not returning to user mode then just return.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, r0, returnTrap_Return
	Nop
	/*
	 * See if we have to take any special action for this process.
	 * This is determined by looking at 
	 * proc_RunningProcesses[0]->specialHandling.
	 */
	ld_32		VOL_TEMP1, r0, $runningProcesses
	Nop
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	nop
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	ld_32		VOL_TEMP2, r0, $_machSpecialHandlingOffset
	Nop
	add_nt		VOL_TEMP1, VOL_TEMP1, VOL_TEMP2
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	Nop
	cmp_br_delayed	ne, VOL_TEMP1, $0, returnTrap_SpecialAction
	Nop
	/*
	 * See if we have to allocate more memory on the saved window
	 * stack.
	 */
	LD_MACH_CUR_STATE_PTR(VOL_TEMP1)
	Nop
        ld_32           VOL_TEMP2, VOL_TEMP1, $MACH_MAX_SWP_OFFSET
	ld_32		VOL_TEMP3, VOL_TEMP1, $MACH_TRAP_SWP_OFFSET
        sub             VOL_TEMP2, VOL_TEMP2, $(2 * MACH_SAVED_REG_SET_SIZE)
        cmp_br_delayed  ule, VOL_TEMP3, VOL_TEMP2, returnTrap_UserReturn
        Nop
        /*
         * Allocate more memory.
         */
	or              VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw         VOL_TEMP1, $0
	call		_MachGetWinMem
	nop
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
	jump		ReturnTrap
	nop

returnTrap_UserReturn:
	/*
	 * Restore the user's state and put us back into user mode.
	 */
	RESTORE_USER_STATE()
	/*
	 * Put us back into user mode.
	 */
	or		KPSW_REG, KPSW_REG, $MACH_KPSW_CUR_MODE

returnTrap_Return:
	or		KPSW_REG, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		KPSW_REG, $0
	/*
	 * If we are supposed to return to the next PC then set the current
	 * PC to the next PC and clear the next PC so that the return
	 * happens correctly.
	 */
	LD_PC_RELATIVE(VOL_TEMP2, returnTrap_Const1)
	and		VOL_TEMP1, KPSW_REG, VOL_TEMP2
	cmp_br_delayed	eq, VOL_TEMP1, VOL_TEMP2, 1f
	Nop
	add_nt		CUR_PC_REG, NEXT_PC_REG, $0
	add_nt		NEXT_PC_REG, r0, $0
1:
	/*
	 * If the 2nd PC in NEXT_PC_REG is zero then 
	 * we don't do the jump to the 2nd PC because there is none.
	 */
	cmp_br_delayed	eq, NEXT_PC_REG, $0, returnTrap_No2ndPC
	Nop
#ifdef PATCH_IBUFFER
	add_nt		SAFE_TEMP1, CUR_PC_REG, $0
	add_nt		CUR_PC_REG, r0, $0
	invalidate_ib
	jump_reg	SAFE_TEMP1, $0
#else
	jump_reg	CUR_PC_REG, $0
#endif
	return		NEXT_PC_REG, $0

returnTrap_No2ndPC:
#ifdef PATCH_IBUFFER
	add_nt		SAFE_TEMP1, CUR_PC_REG, $0
	add_nt		CUR_PC_REG, r0, $0
	return		SAFE_TEMP1, $0
#else
	return		CUR_PC_REG, $0
#endif
	Nop

/*
 * returnTrap_FailedCopy --
 *
 *	A copy to/from user space failed.
 */
returnTrap_FailedCopy:
	/*
	 * Enable all traps, go back to the previous window and return an
	 * error to the caller.
	 */
#ifdef PATCH_IBUFFER
	add_nt		CUR_PC_REG, r0, $0
#endif
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	rd_special	VOL_TEMP1, pc
	return		VOL_TEMP1, $12
	Nop
	/*
	 * The error to return is 0x20000 for SYS_ARG_NOACCESS.  Since we only
	 * have 14 bits of immediate we have to insert a 2 into the 3rd byte.
	 */
	add_nt		RETURN_VAL_REG_CHILD, r0, $0
	rd_insert	VOL_TEMP1
	wr_insert	$2
	insert		RETURN_VAL_REG_CHILD, r0, $2
	wr_insert	VOL_TEMP1
	/*
	 * The spill stack before the call was stored in r25.  Restore the
	 * spill SP and then do a normal return.
	 */
	add_nt		SPILL_SP, r25, $0
	return		RETURN_ADDR_REG, $8
	Nop

/*
 * returnTrap_FailedArgFetch --
 *
 *	The fetching of the user's args during a system call failed.
 */
returnTrap_FailedArgFetch:
	/*
	 * Enable all traps and go back to the previous window.
	 */
#ifdef PATCH_IBUFFER
	add_nt		CUR_PC_REG, r0, $0
#endif
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	rd_special	VOL_TEMP1, pc
	return		VOL_TEMP1, $12
	Nop
	/*
	 * We are now back in the window that was doing the arg fetch.  Set
	 * the return value to SYS_ARG_NOACCESS and then jump to the code
	 * that does the system call return.  The error to return is 0x20000
	 * for SYS_ARG_NOACCESS.  Since we only have 14 bits of immediate we
	 * have to insert a 2 into the 3rd byte.
	 */
	add_nt		RETURN_VAL_REG, r0, $0
	rd_insert	VOL_TEMP1
	wr_insert	$2
	insert		RETURN_VAL_REG, r0, $2
	wr_insert	VOL_TEMP1
	jump		SysCall_Return
	Nop

/*
 * returnTrap_SpecialAction --
 *
 *	Need to take some special action for this process before returning
 *	to user mode.
 */
returnTrap_SpecialAction:
	/*
	 * Reenable all traps and then call the routine 
	 * that tells us what action to take.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	call		_MachUserAction
	Nop
	/*
	 * The user action routine will return TRUE (1) if we should call
	 * a signal handler or FALSE (0) otherwise.
	 */
	cmp_br_delayed	eq, RETURN_VAL_REG, $1, returnTrap_CallSigHandler
	Nop
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
	jump		ReturnTrap
	Nop

/*
 * returnTrap_CallSigHandler --
 *
 *	Need to start the process off calling a signal handler.  When we
 *	are called all traps have already been enabled.
 */
returnTrap_CallSigHandler:
	/*
	 * See if we have to allocate more memory on the saved window
	 * stack.
	 */
	LD_MACH_CUR_STATE_PTR(SAFE_TEMP1)
	Nop
        ld_32           VOL_TEMP1, SAFE_TEMP1, $MACH_MAX_SWP_OFFSET
	ld_32		VOL_TEMP2, SAFE_TEMP1, $MACH_TRAP_SWP_OFFSET
        sub             VOL_TEMP1, VOL_TEMP1, $(2 * MACH_SAVED_REG_SET_SIZE)
        cmp_br_delayed  ule, VOL_TEMP2, VOL_TEMP1, 1f
        Nop
        /*
         * Allocate more memory.
         */
	call		_MachGetWinMem
	nop
1:
	/*
	 * Disable all traps and restore the user's state.
	 */
	wr_kpsw		KPSW_REG, $0
	RESTORE_USER_STATE()
	/*
	 * Pass the correct args to the signal handler:
	 *
	 *	Handler(sigNum, sigCode, contextPtr)
	 */
	ld_32		INPUT_REG1, SAFE_TEMP1, $MACH_SIG_NUM_OFFSET
	ld_32		INPUT_REG2, SAFE_TEMP1, $MACH_SIG_CODE_OFFSET
	add_nt		INPUT_REG3, SPILL_SP, $0
	/*
	 * Set the signal handler executing in user mode.
	 * Assume that the user's signal handler will do a "return r10,r0,8".
	 */
	ld_32		VOL_TEMP1, SAFE_TEMP1, $MACH_NEW_CUR_PC_OFFSET
	add_nt		RETURN_ADDR_REG, r0, $(SigReturnAddr-8)
	or		KPSW_REG, KPSW_REG, $(MACH_KPSW_CUR_MODE|MACH_KPSW_ALL_TRAPS_ENA)
	jump_reg	VOL_TEMP1, $0
	wr_kpsw		KPSW_REG, $0

/*
 *----------------------------------------------------------------------------
 *
 * SaveState -
 *
 *	Save the state of the process in the given state struct.  Also push
 *	all of the windows, except for the current one, onto the saved
 *	window stack.  VOL_TEMP1 contains where to save the state to and 
 *	VOL_TEMP2 contains the return address.  
 *
 *----------------------------------------------------------------------------
 */
SaveState:
	/*
	 * Save kpsw, upsw, insert register and the current and next PCs 
	 * of the fault.
	 */
	st_32		KPSW_REG, VOL_TEMP1, $MACH_REG_STATE_KPSW_OFFSET
	rd_special	VOL_TEMP3, upsw
	st_32		VOL_TEMP3, VOL_TEMP1, $MACH_REG_STATE_UPSW_OFFSET
	and		CUR_PC_REG, CUR_PC_REG, $~3
	st_32		CUR_PC_REG, VOL_TEMP1, $MACH_REG_STATE_CUR_PC_OFFSET
	and		NEXT_PC_REG, NEXT_PC_REG, $~3
	st_32		NEXT_PC_REG, VOL_TEMP1, $MACH_REG_STATE_NEXT_PC_OFFSET
	rd_insert	VOL_TEMP3
	st_32		VOL_TEMP3, VOL_TEMP1, $MACH_REG_STATE_INSERT_OFFSET
#ifdef	PATCH_IBUFFER	
	st_32		r26, VOL_TEMP1, $0
#endif
	/*
	 * Clear the upsw so that kernel wont get lisp traps.
	 */
	wr_special	upsw, r0, $0

	/*
	 * Save all of the globals.
	 */

 	st_40		r0, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+0
 	st_40		r1, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+8
 	st_40		r2, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+16
 	st_40		r3, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+24
 	st_40		r4, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+32
 	st_40		r5, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+40
 	st_40		r6, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+48
 	st_40		r7, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+56
 	st_40		r8, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+64
 	st_40		r9, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+72

	/*
	 * Move where to save to into a global.
	 */
	add_nt		r1, VOL_TEMP1, $0

	/*
	 * Now go back to the previous window.  Enable all traps and
	 * disable interrupts so that we can take a window underflow
	 * fault if necessary.
	 */

	rd_kpsw		r5
	or		VOL_TEMP3, r5, $MACH_KPSW_ALL_TRAPS_ENA
	and		VOL_TEMP3, VOL_TEMP3, $(~MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw		VOL_TEMP3, $0
	rd_special	VOL_TEMP3, pc
	return		VOL_TEMP3, $12
	Nop

	/*
	 * Now we are in the previous window.  Save all of its registers
	 * into the state structure.
	 */
	st_40		r10, r1, $MACH_REG_STATE_REGS_OFFSET+80
	st_40		r11, r1, $MACH_REG_STATE_REGS_OFFSET+88
 	st_40		r12, r1, $MACH_REG_STATE_REGS_OFFSET+96
 	st_40		r13, r1, $MACH_REG_STATE_REGS_OFFSET+104
	st_40		r14, r1, $MACH_REG_STATE_REGS_OFFSET+112
 	st_40		r15, r1, $MACH_REG_STATE_REGS_OFFSET+120
 	st_40		r16, r1, $MACH_REG_STATE_REGS_OFFSET+128
 	st_40		r17, r1, $MACH_REG_STATE_REGS_OFFSET+136
 	st_40		r18, r1, $MACH_REG_STATE_REGS_OFFSET+144
 	st_40		r19, r1, $MACH_REG_STATE_REGS_OFFSET+152
 	st_40		r20, r1, $MACH_REG_STATE_REGS_OFFSET+160
 	st_40		r21, r1, $MACH_REG_STATE_REGS_OFFSET+168
 	st_40		r22, r1, $MACH_REG_STATE_REGS_OFFSET+176
 	st_40		r23, r1, $MACH_REG_STATE_REGS_OFFSET+184
 	st_40		r24, r1, $MACH_REG_STATE_REGS_OFFSET+192
 	st_40		r25, r1, $MACH_REG_STATE_REGS_OFFSET+200
 	st_40		r26, r1, $MACH_REG_STATE_REGS_OFFSET+208
	st_40		r27, r1, $MACH_REG_STATE_REGS_OFFSET+216
 	st_40		r28, r1, $MACH_REG_STATE_REGS_OFFSET+224
 	st_40		r29, r1, $MACH_REG_STATE_REGS_OFFSET+232
 	st_40		r30, r1, $MACH_REG_STATE_REGS_OFFSET+240
 	st_40		r31, r1, $MACH_REG_STATE_REGS_OFFSET+248

	/*
	 * Now push all of the windows before the current one onto the saved
	 * window stack.  Note that swp points to the last window
	 * saved so that we have to save from swp + 1 up through cwp - 1.
	 * r1 contains the swp, r2 the cwp and r3  the current value of the 
	 * swp shifted over to align with the cwp.
	 */
	rd_special	r1, swp
	and		r1, r1, $~7
	rd_special	r2, cwp
	and		r2, r2, $0x1c
	/*
	 * Set r3 to the swp aligned with the cwp so that we can use it for
	 * comparisons.
	 */
	and		r3, r1, $0x380
	srl		r3, r3, $1	/* Read the swp and then shift it so */
	srl		r3, r3, $1	/*    it aligns with the cwp.  That is */
	srl		r3, r3, $1	/*    swp<9:7> -> swp<4:2> */
	srl		r3, r3, $1
	srl 		r3, r3, $1

saveState_SaveRegs:
	add_nt		r3, r3, $4
	and		r3, r3, $0x1c
	cmp_br_delayed	eq, r3, r2, saveState_Done
	Nop
	wr_special	cwp, r3, $0	/* Set the cwp to the window to save. */
	Nop
					/* Increment the swp by one window. */
	add_nt		r1, r1, $MACH_SAVED_WINDOW_SIZE
	st_40		r10, r1, $0
	st_40		r11, r1, $8
	st_40		r12, r1, $16
	st_40		r13, r1, $24
	st_40		r14, r1, $32
	st_40		r15, r1, $40
	st_40		r16, r1, $48
	st_40		r17, r1, $56
	st_40		r18, r1, $64
	st_40		r19, r1, $72
	st_40		r20, r1, $80
	st_40		r21, r1, $88
	st_40		r22, r1, $96
	st_40		r23, r1, $104
	st_40		r24, r1, $112
	st_40		r25, r1, $120
	jump		saveState_SaveRegs
	Nop

saveState_Done:
	wr_special	cwp, r2, $0x4	/* Move back to the current window. */
	wr_special	swp, r1, $0	/* Update the swp. */
	/*
	 * Save the cwp and swp.
	 */
	st_32		r1, VOL_TEMP1, $MACH_REG_STATE_SWP_OFFSET
	st_32		r2, VOL_TEMP1, $MACH_REG_STATE_CWP_OFFSET
	/*
	 * Restore the kpsw and return to our caller.
	 */
	wr_kpsw		r5, $0
	jump_reg	VOL_TEMP2, $0
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * RestoreState -
 *
 *	Restore the state of the process from the given state struct.
 *	window stack.  VOL_TEMP1 contains where to restore the state from
 *	and VOL_TEMP2 contains the return address.
 *
 *----------------------------------------------------------------------------
 */
RestoreState:
	/*
	 * Disable interrupts because we will be screwing around with the
	 * swp and cwp here.  The kpsw will be restored below when we
	 * restore the state.
	 */
	rd_kpsw		r8
	and		VOL_TEMP3, r8, $~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP3, $0

#ifdef PATCH_IBUFFER
	/*
	 * Clear r10 in all windows to insure that all fault and interrupt
	 * locations are gone.
	 */
	rd_special	r1, cwp
	wr_special	cwp,r0,$0	/* Window 0,1 */
	Nop
	add_nt		r10,r0,$0
	add_nt		r26,r0,$0
	wr_special	cwp,r0,$8	/* Window 2,3 */
	Nop
	add_nt		r10,r0,$0
	add_nt		r26,r0,$0
	wr_special	cwp,r0,$16	/* Window 4,5 */
	Nop
	add_nt		r10,r0,$0
	add_nt		r26,r0,$0
	wr_special	cwp,r0,$24	/* Window 6,7 */
	Nop
	add_nt		r10,r0,$0
	add_nt		r26,r0,$0
	wr_special	cwp, r1, $0
	Nop
#endif
	/*
	 * Restore the cwp and swp.  This is a little tricky because
	 * the act of restoring them will switch windows so we can't rely
	 * on anything in our current window.
	 */
	add_nt		r1, VOL_TEMP1, $0
	add_nt		r9, VOL_TEMP2, $0
	ld_32		r2, r1, $MACH_REG_STATE_CWP_OFFSET
	ld_32		r3, r1, $MACH_REG_STATE_SWP_OFFSET
	wr_special	cwp, r2, $0
	wr_special	swp, r3, $0

	/*
	 * Now we are back in the window that we want to restore since the
	 * saved cwp points to the window that we saved in the reg state 
	 * struct.
	 */
  	ld_40		r10, r1, $MACH_REG_STATE_REGS_OFFSET+80
  	ld_40		r11, r1, $MACH_REG_STATE_REGS_OFFSET+88
  	ld_40		r12, r1, $MACH_REG_STATE_REGS_OFFSET+96
  	ld_40		r13, r1, $MACH_REG_STATE_REGS_OFFSET+104
  	ld_40		r14, r1, $MACH_REG_STATE_REGS_OFFSET+112
  	ld_40		r15, r1, $MACH_REG_STATE_REGS_OFFSET+120
  	ld_40		r16, r1, $MACH_REG_STATE_REGS_OFFSET+128
  	ld_40		r17, r1, $MACH_REG_STATE_REGS_OFFSET+136
  	ld_40		r18, r1, $MACH_REG_STATE_REGS_OFFSET+144
  	ld_40		r19, r1, $MACH_REG_STATE_REGS_OFFSET+152
  	ld_40		r20, r1, $MACH_REG_STATE_REGS_OFFSET+160
  	ld_40		r21, r1, $MACH_REG_STATE_REGS_OFFSET+168
  	ld_40		r22, r1, $MACH_REG_STATE_REGS_OFFSET+176
  	ld_40		r23, r1, $MACH_REG_STATE_REGS_OFFSET+184
  	ld_40		r24, r1, $MACH_REG_STATE_REGS_OFFSET+192
  	ld_40		r25, r1, $MACH_REG_STATE_REGS_OFFSET+200
  	ld_40		r26, r1, $MACH_REG_STATE_REGS_OFFSET+208
  	ld_40		r27, r1, $MACH_REG_STATE_REGS_OFFSET+216
  	ld_40		r28, r1, $MACH_REG_STATE_REGS_OFFSET+224
  	ld_40		r29, r1, $MACH_REG_STATE_REGS_OFFSET+232
  	ld_40		r30, r1, $MACH_REG_STATE_REGS_OFFSET+240
  	ld_40		r31, r1, $MACH_REG_STATE_REGS_OFFSET+248
	/*
	 * Go forward to the window that we are to execute in.
	 */
	wr_special	cwp, r2, $4
	Nop
	add_nt		VOL_TEMP1, r1, $0
	add_nt		VOL_TEMP2, r9, $0
	/*
	 * Restore the current PC, next PC, insert register, kpsw. 
	 */
	ld_32		KPSW_REG, VOL_TEMP1, $MACH_REG_STATE_KPSW_OFFSET
	/*
	 * Insert the current processor number into the KPSW_REG.
	 */
#ifdef notdef
	GET_PNUM_FROM_BOARD(r1)
	wr_insert	$3
	insert		KPSW_REG, KPSW_REG, r1
#endif
	ld_32		r1, VOL_TEMP1, $MACH_REG_STATE_UPSW_OFFSET
	ld_32		r2, VOL_TEMP1, $MACH_REG_STATE_INSERT_OFFSET
	ld_32		CUR_PC_REG, VOL_TEMP1, $MACH_REG_STATE_CUR_PC_OFFSET
	ld_32		NEXT_PC_REG, VOL_TEMP1, $MACH_REG_STATE_NEXT_PC_OFFSET
	wr_insert	r2
	wr_special	upsw,r1,$0
	/*
	 * Restore the kpsw to that which we came in with.
	 */
	wr_kpsw		r8, $0
	/*
	 * Restore the globals.
	 */
	ld_40		r1, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+8
	ld_40		r2, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+16
	ld_40		r3, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+24
	ld_40		r4, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+32
	ld_40		r5, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+40
	ld_40		r6, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+48
	ld_40		r7, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+56
	ld_40		r8, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+64
	ld_40		r9, VOL_TEMP1, $MACH_REG_STATE_REGS_OFFSET+72

	/*
	 * Return to our caller.
	 */
	jump_reg	VOL_TEMP2, $0
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * MachRefreshCCWells()
 *
 *	Cause a fault in order to refresh the CC wells.
 *
 *----------------------------------------------------------------------------
 */

	.globl _MachRefreshCCWells
_MachRefreshCCWells:
	cmp_trap	always,r0,r0,$MACH_REFRESH_TRAP
	return		RETURN_ADDR_REG, $8
	nop

/*
 *----------------------------------------------------------------------------
 *
 * MachRunUserProc()
 *
 *	Start the user process executing.
 *
 *----------------------------------------------------------------------------
 */
	.globl _MachRunUserProc
_MachRunUserProc:
	/*
	 * Disable all traps and set the previous mode to be from user space.
	 */
	rd_kpsw		KPSW_REG
	or		KPSW_REG, KPSW_REG, $MACH_KPSW_PREV_MODE
	and		KPSW_REG, KPSW_REG, $(~MACH_KPSW_ALL_TRAPS_ENA)
	/*
	 * Do a normal return from trap.
	 */
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
	jump		ReturnTrap
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * Mach_ContextSwitch(fromProcPtr, toProcPtr) --
 *
 *	Switch the thread of execution to a new process.  fromProcPtr
 *	contains a pointer to the process to switch from and toProcPtr
 *	a pointer to the process to switch to.
 *
 *----------------------------------------------------------------------------
 */
	.globl	_Mach_ContextSwitch
_Mach_ContextSwitch:
	/*
	 * Setup the virtual memory context for the process.
	 */
	add_nt		OUTPUT_REG1, INPUT_REG2, $0
	call		_VmMach_SetupContext
	Nop
	invalidate_ib
	/*
	 * Grab a pointer to the state structure to save to.  Note that
	 * the restore and save state routines do not touch any of the
	 * NON_INTR_TEMP registers.
	 */
	ld_32		NON_INTR_TEMP1, r0, $_machStatePtrOffset
	Nop
	add_nt		VOL_TEMP1, INPUT_REG1, NON_INTR_TEMP1
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	Nop
	/*
	 * Now compute where to save the registers to and call the routine
	 * to save state.  Note that SaveState saves our parent window back.
	 * Thus our locals and output regs will not get modified by saving
	 * and restoring state.
	 */
	add_nt		VOL_TEMP1, VOL_TEMP1, $MACH_SWITCH_REG_STATE_OFFSET
	rd_special	VOL_TEMP2, pc
	add_nt		VOL_TEMP2, VOL_TEMP2, $16
	jump		SaveState
	Nop
	/*
	 * Grab a pointer to the state structure to restore state from.
	 */
	add_nt		VOL_TEMP1, INPUT_REG2, NON_INTR_TEMP1
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	Nop
	/*
	 * This is now our current state structure address.
	 */
	ST_MACH_CUR_STATE_PTR(VOL_TEMP1, VOL_TEMP2)
	/*
	 * Now compute where to restore the registers from and call the routine
	 * to restore state.
	 */
	add_nt		VOL_TEMP1, VOL_TEMP1, $MACH_SWITCH_REG_STATE_OFFSET
	rd_special	VOL_TEMP2, pc
	add_nt		VOL_TEMP2, VOL_TEMP2, $16
	jump		RestoreState
	Nop
	/*
	 * We are now in the new process so return.
	 */
	return		RETURN_ADDR_REG, $8
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * Mach_DisableNonmaskableIntr --
 *
 *	Mach_DisableNonmaskableIntr()
 *
 *	Disable non-maskable interrupts.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Interrupts are disabled.
 *
 *----------------------------------------------------------------------------
 */
	.globl	_Mach_DisableNonmaskableIntr
_Mach_DisableNonmaskableIntr:
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1, VOL_TEMP1, $(~MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw		VOL_TEMP1, $0
	return		RETURN_ADDR_REG, $8
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * Mach_EnableNonmaskableIntr --
 *
 *	void Mach_EnableNonmaskableIntr()
 *
 *	Enable interrupts.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Interrupts are enabled.
 *
 *----------------------------------------------------------------------------
 */
	.globl	_Mach_EnableNonmaskableIntr
_Mach_EnableNonmaskableIntr:
	rd_kpsw		SAFE_TEMP2
	and		VOL_TEMP1, SAFE_TEMP2, $~MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP1, $0
	READ_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP1)
	ld_32		VOL_TEMP1, r0, $_machNonmaskableIntrMask
	nop
	or		SAFE_TEMP1, SAFE_TEMP1, VOL_TEMP1
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP1)
	or		VOL_TEMP1, SAFE_TEMP2, $MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP1, $0
	return		RETURN_ADDR_REG, $8
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * Mach_DisableIntr --
 *
 *	Mach_DisableIntr()
 *
 *	Disable maskable interrupts.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Interrupts are disabled.
 *
 *----------------------------------------------------------------------------
 */
	.globl	_Mach_DisableIntr
_Mach_DisableIntr:
	rd_kpsw		SAFE_TEMP2
	and		VOL_TEMP1, SAFE_TEMP2, $(~MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw		VOL_TEMP1, $0
	ld_32		SAFE_TEMP1, r0, $_machNonmaskableIntrMask
	nop
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP1)
	wr_kpsw		SAFE_TEMP2, $0
	return		RETURN_ADDR_REG, $8
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * Mach_EnableIntr --
 *
 *	void Mach_EnableIntr()
 *
 *	Enable all interrupts.
 *
 * Results:
 *     	None.
 *
 * Side effects:
 *	Interrupts are enabled.
 *
 *----------------------------------------------------------------------------
 */
	.globl	_Mach_EnableIntr
_Mach_EnableIntr:
	rd_kpsw		SAFE_TEMP2
	and		VOL_TEMP1, SAFE_TEMP2, $(~MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw		VOL_TEMP1, $0
	ld_32		SAFE_TEMP1, r0, $_machIntrMask
	nop
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, SAFE_TEMP1)
	wr_kpsw		SAFE_TEMP2, $0
	return		RETURN_ADDR_REG, $8
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * Mach_TestAndSet --
 *
 *	int Mach_TestAndSet(intPtr)
 *	    int *intPtr;
 *
 *	Test-and-set an integer.
 *
 * Results:
 *     	Returns 0 if *intPtr was zero and 1 if *intPtr was non-zero.  Also
 *	in all cases *intPtr is set to a non-zero value.
 *
 * Side effects:
 *	*intPtr set to a non-zero value if not there already.
 *
 *----------------------------------------------------------------------------
 */
	.globl	_Mach_TestAndSet
_Mach_TestAndSet:
	rd_kpsw		SAFE_TEMP1
	and		VOL_TEMP1, SAFE_TEMP1, $(~MACH_KPSW_INTR_TRAP_ENA)
	jump		1f
	wr_kpsw		VOL_TEMP1, $0
	.align		5	/* 32 byte alignment puts us at the head of
				 * a block. This should fix a bug with 
				 * test_and_set and prefeching. 
				 */
1:
	test_and_set	VOL_TEMP1, INPUT_REG1, $0
	nop
	nop
	nop
	nop
	nop
	wr_kpsw		SAFE_TEMP1, $0

	add_nt		RETURN_VAL_REG_CHILD, VOL_TEMP1, $0
	return		RETURN_ADDR_REG, $8
	Nop


/*
 *----------------------------------------------------------------------------
 *
 * Mach_SetProcessorNumber --
 *
 *	void Mach_SetProcessorNumber(pnum)
 *	    int pnum;
 *
 *	Set the processor number of the current processor to pnum.
 *
 * Results:
 *
 * Side effects: kpsw is modified.
 *	
 *
 *----------------------------------------------------------------------------
 */
	.globl _Mach_SetProcessorNumber
_Mach_SetProcessorNumber:
	/*
	 * Update number stored in high byte or kpsw.
	 */
	rd_kpsw		SAFE_TEMP1
	wr_insert	$3
	insert		SAFE_TEMP1,SAFE_TEMP1,INPUT_REG1
	wr_kpsw		SAFE_TEMP1, $0
	return		RETURN_ADDR_REG, $8
	Nop

/*
 * ParseInstruction --
 *
 *      Decode the trapping instruction and return it's opcode and operations.
 *	The address of the instruction is passed in r10 and the address to 
 *	jump to when done is passed in VOL_TEMP1, and the trapping instruction
 *      is passed in VOL_TEMP2.  The return values are:
 *      OUTPUT1 -- opcode
 *      OUTPUT2 -- destination register (for loads and operations)
 *		   condition code (for compares)
 *                 value to store (for stores)
 *      OUTPUT3 -- operand 1
 *      OUTPUT4 -- operand 2
 *
 *	SRC_REG:	r15 -- src register
 *	PREV_SRC_REG:	r31 -- src register in previous window.
 *	TRAP_INST:	r19 (VOL_TEMP2) -- Input: Trapping instruction.
 *	CALLER_TEMP1	r21 (SAFE_TEMP1) -- Temporary reserved for caller.
 *	CALLER_TEMP2	r22 (SAFE_TEMP2) -- Temporary reserved for caller.
 *	CALLER_TEMP3	r23 (SAFE_TEMP3) -- Temporary reserved for caller.
 *	PARSE_TEMP1:	r24 -- One temporary to use.
 *	SAVED_R15:	r25 -- Place to save r15.
	SAVED_CUR_PC    r31 -- Place to save CUR_PC
 */

#define	SRC_REG	         	r15
#define	PREV_SRC_REG		r31   /* These must be r15 and r31 to work with the tables */

#define	RET_ADDR		r18
#define	TRAP_INST		r19
#define	PARSE_TEMP1		r24
#define	SAVED_R15		r25
#define	SAVED_CUR_PC		r31


ParseInstruction:
	add_nt		SAVED_R15, r15, $0	/* Save r15 for recovering register values */
	add_nt		SAVED_CUR_PC,CUR_PC_REG,$0  /* Save CUR_PC across call */
	/*                 	
       	 * fetch the opcode
	 */
	extract		OUTPUT_REG1, TRAP_INST, $3	/* Opcode <31:25> -> <07:01> */
	srl		OUTPUT_REG1, OUTPUT_REG1, $1	/* Opcode <07:01> -> <06:00> */
	
	/*
	 * fetch the destination register or cc
	 */
	sll		OUTPUT_REG2, TRAP_INST, $3	/* Rdes <24:20> to <27:23> */
	sll		OUTPUT_REG2, OUTPUT_REG2, $1	/* <27:23> to <28:24> */
	extract		OUTPUT_REG2, OUTPUT_REG2, $3    /* <28:24> to <04:00> */
	and		OUTPUT_REG2, OUTPUT_REG2, $0x1f /* Pitch the extra bits  */
	/*
	* fetch the first operand 
	*/
	sll		SRC_REG, TRAP_INST, $1	/* s1 <19:15> to <20:16> */
	extract		SRC_REG, SRC_REG, $2	/* s1 <20:16> to <04:00> */
	and		SRC_REG, SRC_REG, $0x1f /* Pitch the extra bits */

	cmp_br_delayed	ult, SRC_REG,  $31, @getf  
	nop
	jump		@endf
	add_nt		SRC_REG, SAVED_R15, $0   /* get r31 directly */


/* Here, we get the value of the register (0-30) from the previous window.  
 * It's real tricky.  Basically, we go back to the previous window, execute
 * a single instruction out of a table (moving the proper register into r31),
 * then returning to this window.  It relies on a couple of architecture 
 * features.
 * First, SRC_REG (r15) in this window will be PREV_SRC_REG (r31) in the 
 * previous window.  Second, the jump_reg below will take us to the move table
 * while the jump in the delay slot will bring us back.  The instruction in the
 * move table is executed in the delay slot of the jump.  Got it?
 */
@get:
	rd_special	PARSE_TEMP1, pc         /* Back up one window */
	return		PARSE_TEMP1, $12
	Nop
	sll		PREV_SRC_REG, PREV_SRC_REG, $2	/* s1 = s1 * 4 */
	jump_reg	PREV_SRC_REG, $OpRecov /*Retrieve value of first op. */
	jump	        @nextf  /* The instruction in table will be
				 * executed in the delay slot of this jump. */
@next:
	call 	       	@endf			/* Get back to trap window.  */
	Nop

@end:
	add_nt		OUTPUT_REG3,SRC_REG,$0


	/*  Now, we need to do a little decoding on the opcode to get the rest of 
         *  the operands.  We take different action depending on whether the opcode
         *  is a store, compare, or other operation.
         */

	/*
	 * It's a store if the op code is > 0x20 and < 0x30
	 */
#define ST_BASE       0x20
	add_nt		PARSE_TEMP1,OUTPUT_REG1,$-ST_BASE
	cmp_br_delayed  lt, PARSE_TEMP1, $0x20-ST_BASE, @not_storef
	nop
	cmp_br_delayed  lt, PARSE_TEMP1, $0x30-ST_BASE, parse_store
	nop

	/*
	 * See if it's a compare
	 */
@not_store:
#define CMP_BASE        0x50
#define CMP_BR_DELAYED  0x50
#define CMP_TRAP        0x51

	add_nt		PARSE_TEMP1,OUTPUT_REG1,$-CMP_BASE
	cmp_br_delayed  eq,PARSE_TEMP1,$CMP_BR_DELAYED-CMP_BASE,parse_cmp
	cmp_br_delayed  eq,PARSE_TEMP1,$CMP_TRAP-CMP_BASE,parse_cmp
	nop


	/*
	 * Assume load or op.  Check for register or immediate value.
	 */
parse_op:
	extract		PARSE_TEMP1, TRAP_INST, $1
	and		PARSE_TEMP1, PARSE_TEMP1, $0x40
	cmp_br_delayed  ne, PARSE_TEMP1, $0, @immedf
	Nop
	jump		@regf
	Nop

@immed:
	/*
	 * 2nd operand is an immediate.
	 */
	and		OUTPUT_REG4, TRAP_INST, $0x1fff	/* Extract immediate val. */
	add_nt		PARSE_TEMP1, TRAP_INST, $0	/* Check for a  */
	srl		PARSE_TEMP1, PARSE_TEMP1, $1	/*   negative number */
	and		PARSE_TEMP1, PARSE_TEMP1, $0x1000 
	cmp_br_delayed	eq, PARSE_TEMP1, $0, parse_end
	Nop
	jump		parse_end
	add_nt		OUTPUT_REG4, OUTPUT_REG4, $~0x1fff	/* Sign extend */

	/*
	 * Second operand is a register.  Get it like we did the one above.
	 */
@reg:	srl		SRC_REG,TRAP_INST,$1	/* Extract register number  */
	extract		SRC_REG,SRC_REG,$1
	and		SRC_REG,SRC_REG,$0x1f
	
	cmp_br_delayed	ult, SRC_REG,  $31, @getf
	nop
	jump		@endf
	add_nt		SRC_REG, SAVED_R15, $0   /* get r31 directly */

@get:	rd_special	PARSE_TEMP1, pc         /* Back up one window */
	return		PARSE_TEMP1, $12
	Nop
	sll		PREV_SRC_REG, PREV_SRC_REG, $2	/* s1 = s1 * 4 */
	jump_reg	PREV_SRC_REG, $OpRecov  /* Retrieve value of 2nd op. */
	jump	        @nextf  /* The instruction in table will be
			      /* executed in the delay slot of this jump. */
@next:
	call 	       	@endf			/* Get back to trap window.  */
	Nop

@end:
	jump		parse_end
	add_nt		OUTPUT_REG4,SRC_REG,$0

/*
 *   Compares have either a short (5 bit immediate) or a register as the second op
 *   and a word offset for a destination address.  
 */
parse_cmp:
	extract		SRC_REG, TRAP_INST, $1		/* Extract register or immediate val. */
	srl		SRC_REG, SRC_REG,$1
	and		OUTPUT_REG4, SRC_REG,$0x1f

	extract		PARSE_TEMP1, TRAP_INST, $1    /* Check for value type */
	and		PARSE_TEMP1, PARSE_TEMP1, $0x40 /* (immediate or register) */
	cmp_br_delayed  eq, PARSE_TEMP1, $0, @regf
	Nop

@immed:
	/*
	 * 2nd operand is an immediate.
	 */
	cmp_br_delayed	gt, PARSE_TEMP1, $0x0f, @offsetf  /* If negative, sign extend */
	Nop
	jump		@offsetf
	or		OUTPUT_REG4, OUTPUT_REG4, $~0x1f	

@reg:
	/*
	 * Second operand is a register.  Get it like we did the one above.
	 */
	cmp_br_delayed	lt, SRC_REG,  $31, @getf  
	nop
	jump		@endf
	add_nt		SRC_REG, SAVED_R15, $0   /* get r31 directly */
@get:

	rd_special	PARSE_TEMP1, pc         /* Back up one window */
	return		PARSE_TEMP1, $12
	Nop
	sll		PREV_SRC_REG, PREV_SRC_REG, $2	/* s1 = s1 * 4 */
	jump_reg	PREV_SRC_REG, $OpRecov  /* Retrieve value of 2nd op. */
	jump	        @nextf   /* The instruction in table will be
         		          * executed in the delay slot of this jump. */
@next:
	call 	       	@endf			/* Get back to trap window.  */
	Nop

@end:
	add_nt		OUTPUT_REG4,SRC_REG,$0

	/*
	 *		Get the destination address as the last operand.
	 */
@offset:
#ifdef 0          /* Not needed for now.  -- daj */
	and		PARSE_TEMP1,TRAP_INST,$0x100	/* Sign extend if negative */
	cmp_br_delayed	eq,PARSE_TEMP1,r0,@posf
	and		OUTPUT_REG5,TRAP_INST,$0x1ff
	or		OUTPUT_REG5,OUTPUT_REG5,$~0x1ff
	sll		OUTPUT_REG5,OUTPUT_REG5,$2	/* Change to byte displacement */
	jump		parse_end
	add_nt		OUTPUT_REG5,OUTPUT_REG5,r10	/* Add instruction address  */
#else
	jump		parse_end
	nop
#endif

/*
 * Stores have a two part immediate value as the second operand.
 */

parse_store:
	sll		PARSE_TEMP1, TRAP_INST, $3	/* Dest<24:20> move to <27:23> */
	sll		PARSE_TEMP1, PARSE_TEMP1, $2	/* Move to <29:25> */
	extract		PARSE_TEMP1, PARSE_TEMP1, $3	/* Move to <5:1>   */
	rd_insert	OUTPUT_REG4
	wr_insert	$1
	insert		PARSE_TEMP1, r0, PARSE_TEMP1	/* Move to <13:09> */
	wr_insert	OUTPUT_REG4
	srl		OUTPUT_REG4, PARSE_TEMP1, $1	/* Sign-bit <13> to <12> */
	and		OUTPUT_REG4, OUTPUT_REG4, $0x1000 /* Check for negative */
	cmp_br_delayed	eq, OUTPUT_REG4, $0, @posf    /*    number */
	and 		PARSE_TEMP1, PARSE_TEMP1,$0x1e00  /* Mask out valid bits. */
	add_nt		PARSE_TEMP1, PARSE_TEMP1,$~0x1fff  /* Sign extend  */

@pos:
	and		OUTPUT_REG4, TRAP_INST, $0x01ff	     /* Put together into */
	or		OUTPUT_REG4, PARSE_TEMP1,  OUTPUT_REG4  /*    14-bit value. */

	/*
	 * Get value being stored
	 */

	add_nt		SRC_REG,OUTPUT_REG2,r0   /* Register number */
	cmp_br_delayed	ult, SRC_REG,  $31, @getf
	nop
	jump		@endf
	add_nt		SRC_REG, SAVED_R15, $0   /* get r31 directly */

@get:	rd_special	PARSE_TEMP1, pc         /* Back up one window */
	return		PARSE_TEMP1, $12
	Nop
	sll		PREV_SRC_REG, PREV_SRC_REG, $2	/* s1 = s1 * 4 */
	jump_reg	PREV_SRC_REG, $OpRecov  /* Retrieve value of 2nd op. */
	jump	        @nextf  /* The instruction in table will be
			      /* executed in the delay slot of this jump. */
@next:
	call 	       	@endf			/* Get back to trap window.  */
	Nop

@end:
	add_nt		OUTPUT_REG2,SRC_REG,$0


	/*
	 * All done.  Restore the registers and go back to the caller.
 	 */		
parse_end:
	add_nt		CUR_PC_REG,SAVED_CUR_PC,$0
	jump_reg	RET_ADDR, r0			/* Go back to caller */
	add_nt		r15, SAVED_R15, $0		/* Restore pre-parse r15 */
.org	MACH_STACK_BOTTOM
mainStack:
/*
 * Leave room for the stacks.
 */
.org MACH_CODE_START
codeStart:
/*
 * Non Master processor jump to this location when spun up by the master
 * processor.  Current it is assumed that the processor is executing in 
 * physical mode.
 */
slaveStart:

/*
 * Initialize the PTEVA.
 */
	LD_CONSTANT(r1, VMMACH_KERN_PT_BASE >> VMMACH_PAGE_SHIFT)
	ST_PT_BASE(r1)
/*
 * Initialize the RPTEVA.
 */
	ST_RPT_BASE(r1)
/*
 * Initialize the 0th segment register.
 */
	ST_GSN(r0, MACH_GSN_0)
/*
 * Initialize the RPTM register.
 */
	LD_CONSTANT(r1, KERN_PT2_BASE)
	ST_RPTM(r1, MACH_RPTM_0)
/*
 * Clear out the cache.
 */
	LD_CONSTANT(r1, 0x03000000)
	LD_CONSTANT(r2, (0x03000000 | VMMACH_CACHE_SIZE))
1:
	st_32		r0, r1, $0
	add_nt		r1, r1, $VMMACH_CACHE_BLOCK_SIZE
	cmp_br_delayed	lt, r1, r2, 1b
	Nop


/*
 * Clear snoop tags.
 */
	LD_CONSTANT(r1, 0x04000000)
	LD_CONSTANT(r2, (0x04000000 | VMMACH_CACHE_SIZE))
1:
	st_32		r0, r1, $0
	add_nt		r1, r1, $VMMACH_CACHE_BLOCK_SIZE
	cmp_br_delayed	lt, r1, r2, 1b
	Nop

/*
 * Clear out the upsw.
 */
	wr_special	upsw, r0, $0

/*
 * Clear out the interrupt mask register so that no interrupts are enabled.
 */
	WRITE_STATUS_REGS(MACH_INTR_MASK_0, r0)

/*
 * Clear the fe status register.
 */
	add_nt		r1, r0, $-1
	WRITE_STATUS_REGS(MACH_FE_STATUS_0, r1)

/*
 * Now jump to virtual mode through the following sequence:
 *
 *	1) Disable instruction buffer just in case it is on.
 *	2) Invalidate the instruction buffer.
 *	3) Make a good kpsw.
 *	4) Jump to slaveMain while setting the kpsw to put is in
 *	   virtual mode in the nop slot of the call.
 */
	wr_kpsw		r0, $0
	invalidate_ib
	add_nt		r1, r0, $(DEFAULT_KPSW)
	LD_PC_RELATIVE(r2,slaveMain)
	jump_reg	r2, $0
	wr_kpsw		r1, $0


slaveMain:
/*
 * Initialized the kpsw with the processor number.
 */
	GET_PNUM_FROM_BOARD(SAFE_TEMP1)
	rd_kpsw		SAFE_TEMP2
	wr_insert	$3
	insert		SAFE_TEMP2,SAFE_TEMP2,SAFE_TEMP1
	wr_kpsw		SAFE_TEMP2, $0
/*
 * Initialize the cwp, swp and SPILL_SP to their proper values. These values
 * are uptained from the Mach_State of the first process for this processor.
 */
	ld_32		VOL_TEMP2,r0,$firstProcStatePtr
	sll		SAFE_TEMP1,SAFE_TEMP1,$2
	ld_32		r1,VOL_TEMP2,SAFE_TEMP1
	Nop
	add_nt		r1, r1, $MACH_SWITCH_REG_STATE_OFFSET
	ld_32		r2, r1, $MACH_REG_STATE_CWP_OFFSET
	ld_32		r3, r1, $MACH_REG_STATE_SWP_OFFSET
	wr_special	cwp, r0, r2
	wr_special	swp, r3, $0
	ld_32		r4, r1, $MACH_REG_STATE_REGS_OFFSET+32
	jump		_mainSlaveStart
	Nop

