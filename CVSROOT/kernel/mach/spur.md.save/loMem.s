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
 * C structures and routines that are called from here.
 */
	.globl _proc_RunningProcesses
	.globl _machCurStatePtr
	.globl _machStatePtrOffset
	.globl _machSpecialHandlingOffset
	.globl _machMaxSysCall
	.globl _machKcallTableOffset
	.globl _machNumArgs
	.globl _machDebugState
	.globl _MachCallDebugger
	.globl _MachInterrupt
	.globl _MachUserError
	.globl _MachVMFault 
	.globl _MachSigReturn
	.globl _MachGetWinMem
	.globl _MachUserAction

/*
 * Trap table.  The hardware jumps to virtual 0x1000 when any type of trap
 * occurs.
 */
	.org 0x1000
	jump PowerUp		# Reset - Jump to powerup sequencer.
	Nop

	.org 0x1010
	jump ErrorTrap		# Error
	Nop

	.org 0x1020
	jump WinOvFlow		# Window overflow
	Nop

	.org 0x1030
	jump WinUnFlow		# Window underflow
	Nop

	.org 0x1040
	jump FaultIntr		# Fault or interrupt
	Nop

	.org 0x1050
	jump FPUExcept		# FPU Exception
	Nop

	.org 0x1060
	jump Illegal		# Illegal op, kernel mode access violation
	Nop

	.org 0x1070
	jump Fixnum		# Fixnum, fixnum_or_char, generation
	Nop

	.org 0x1080
	jump Overflow		# Integer overflow
	Nop

	.org 0x1090
	jump CmpTrap		# Compare trap instruction
	Nop

	.org 0x1100
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
 * of what _machCurStatePtr points to we can't do it normally by the 
 * instruction
 *
 * 	ld_32	rt1, r0, $_machCurStatePtr
 *
 * because the address of _machCurStatePtr will be longer than 13 bits.  
 * However if we put _machCurStatePtr in low memory then we can get to
 * it directly.
 *
 * The other way to do it is
 *
 *	LD_CONSTANT(rt1, _machCurStatePtr)
 *	ld_32	rt1, rt1, $0
 *
 * which is a several instruction sequence.
 */
runningProcesses: 		.long _proc_RunningProcesses
_machCurStatePtr: 		.long 0
_machStatePtrOffset:		.long 0
_machSpecialHandlingOffset:	.long 0
_machMaxSysCall	:		.long 0
_machKcallTableOffset:		.long 0

numArgsPtr:			.long _machNumArgs
debugStatePtr:			.long _machDebugState
debugSWStackBase:		.long MACH_DEBUG_STACK_BOTTOM
debugSpillStackEnd:		.long (MACH_DEBUG_STACK_BOTTOM + MACH_KERN_STACK_SIZE)

/*
 * The instruction to execute on return from a signal handler.  Is here
 * because the value is loaded into a register and it is quicker if
 * is done as an immediate.
 */
SigReturnAddr:
	cmp_trap	always, r0, r0, $MACH_SIG_RETURN_TRAP	
	Nop

/*
 * Jump table to return the operand from an instruction.  Also here
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

/*
 ****************************************************************************
 *
 * END: SPECIAL LOW MEMORY OBJECTS
 *
 ****************************************************************************
 */

.org	0x2000
Start:
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
 *	KERN_PT_VIRT_BASE	The virtual address of the kernel's page tables.
 *	KERN_PT_PHYS_BASE	The physical address of the kernel's page tables
 *	KERN_PT2_VIRT_BASE	The virtual address of the 2nd level kernel
 *				page tables.
 *	KERN_PT2_PHYS_BASE	The physical address of the 2nd level kernel
 *				page tables.
 */
#define	KERN_PT_FIRST_PAGE	1024
#define	KERN_NUM_PAGES		1024
#define	KERN_PT_VIRT_BASE	(KERN_PT_FIRST_PAGE * VMMACH_PAGE_SIZE)
#define	KERN_PT_PHYS_BASE	(0xff000000 + KERN_PT_VIRT_BASE)
#define	KERN_PT2_VIRT_BASE	(KERN_PT_VIRT_BASE + VMMACH_SEG_PT_SIZE / 4 * VMMACH_KERN_PT_QUAD)
#define	KERN_PT2_PHYS_BASE	(0xff000000 + KERN_PT2_VIRT_BASE)

/*
 * In this code registers have the following meaning:
 *
 *	r1:	The base address of the page tables.
 *	r2:	The number of page table pages.
 *	r3:	The page table entry.
 *	r4:	The page table increment.
 *
 * First initialize the second level page tables so that they map
 * all of the kernel page tables.
 */
	LD_CONSTANT(r1, KERN_PT2_PHYS_BASE)
	add_nt		r2, r0, $VMMACH_NUM_PT_PAGES
	LD_CONSTANT(r3, (KERN_PT_FIRST_PAGE << VMMACH_PAGE_FRAME_SHIFT) | VMMACH_RESIDENT_BIT | VMMACH_CACHEABLE_BIT | VMMACH_KRW_URO_PROT)
	LD_CONSTANT(r4, 1 << VMMACH_PAGE_FRAME_SHIFT)

1:
	cmp_br_delayed	eq, r2, $0, 2f
	Nop
	st_32		r3, r1, $0
	add_nt		r1, r1, $4
	add_nt		r3, r3, r4
	sub		r2, r2, $1
	cmp_br_delayed	always, r0, r0, 1b
	Nop
2:

/*
 * Next initialize the kernel page table to point to 4 Mbytes of mapped
 * code.
 */
	LD_CONSTANT(r1, KERN_NUM_PAGES)
	add_nt		r2, r0, $KERN_PT_FIRST_PAGE
	LD_CONSTANT(r3, VMMACH_RESIDENT_BIT | VMMACH_CACHEABLE_BIT | VMMACH_KRW_URO_PROT)

1:
	cmp_br_delayed	eq, r2, $0, 2f
	Nop
	st_32		r3, r1, $0
	add_nt		r1, r1, $4
	add_nt		r3, r3, r4
	sub		r2, r2, $1
	cmp_br_delayed	always, r0, r0, 1b
	Nop
2:

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
	LD_CONSTANT(r1, KERN_PT2_PHYS_BASE)
	ST_RPTM(r1, MACH_RPTM_0)
/*
 * Clear out the cache.
 */
	LD_CONSTANT(r1, 0x3000000)
	LD_CONSTANT(r2, VMMACH_CACHE_SIZE)
1:
	st_32		r0, r1, $0
	add_nt		r1, r1, $VMMACH_CACHE_BLOCK_SIZE
	cmp_br_delayed	lt, r1, r2, 1b
	Nop

/*
 * Initialize the cwp, swp and SPILL_SP to their proper values.
 */
	wr_special	cwp, r0, $1
	LD_CONSTANT(r1, MACH_STACK_BOTTOM)
	wr_special	swp, r1, $0
	LD_CONSTANT(SPILL_SP, MACH_CODE_START)

/*
 * Now jump to virtual mode through the following sequence:
 *
 *	1) Disable instruction buffer just in case it is on.
 *	2) Invalidate the instruction buffer.
 *	3) Make a good kpsw.
 *	4) Jump to the main function.
 */
	wr_kpsw		r0, $0
	invalidate_ib
	add_nt		r1, r0, $(MACH_KPSW_PREFETCH_ENA | MACH_KPSW_IBUFFER_ENA | MACH_KPSW_VIRT_DFETCH_ENA | MACH_KPSW_VIRT_IFETCH_ENA | MACH_KPSW_FAULT_TRAP_ENA | MACH_KPSW_ERROR_TRAP_ENA | MACH_KPSW_ALL_TRAPS_ENA)
	call		_main
	wr_kpsw		r1, $0
	jump		ErrorTrap
	Nop

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
	VERIFY_SWP(winOvFlow_SaveWindow)	# Verify that the SWP is OK.
	USER_ERROR(MACH_USER_BAD_SWP_TRAP)
	/* DOESN'T RETURN */
winOvFlow_SaveWindow:
	/*
	 * Actually save the window.
	 */
	add_nt		SAFE_TEMP2, r1, $0	# Save r1
	rd_special	VOL_TEMP1, cwp
	wr_special	cwp, VOL_TEMP1, $4	# Move forward one window.
	Nop
	rd_special	r1, swp
	wr_special	swp, r1, $MACH_SAVED_WINDOW_SIZE
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
	rd_special	r1, cwp
	wr_special	cwp, r1, $-4		# Move back one window.
	Nop

	add_nt		r1, SAFE_TEMP2, $0	# Restore r1
	/* 
	 * See if we have to allocate more memory.  We need to allocate more
	 * if
	 *
	 *	swp > max_swp - 2 * MACH_SAVED_REG_SET_SIZE
	 */
	cmp_br_delayed	eq, SAFE_TEMP1, r0, winOvFlow_Return	# No need to
	Nop							#  check from
								#  kernel mode
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr
	Nop
	ld_32		VOL_TEMP1, VOL_TEMP1, $MACH_MAX_SWP_OFFSET
	rd_special	VOL_TEMP2, swp
	sub		VOL_TEMP1, VOL_TEMP1, $(2 * MACH_SAVED_REG_SET_SIZE)
	cmp_br_delayed	le, VOL_TEMP2, VOL_TEMP1, winOvFlow_Return
	Nop
	/*
	 * Allocate more memory.
	 */
	add_nt		NON_INTR_TEMP1, CUR_PC_REG, $0
	add_nt		NON_INTR_TEMP2, NEXT_PC_REG, $0
	rd_special	VOL_TEMP1, pc
	return_trap	VOL_TEMP1, $12
	Nop
	cmp_trap	always, r0, r0, $MACH_GET_WIN_MEM_TRAP
	Nop

winOvFlow_Return:
	jump_reg	CUR_PC_REG, $0
	return_trap	NEXT_PC_REG, $0
	Nop

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
	USER_ERROR(MACH_USER_BAD_SWP_TRAP)
	/* DOESN'T RETURN */
winUnFlow_RestoreWindow:
	add_nt		SAFE_TEMP2, r1, $0	# Save r1
	rd_special	r1, swp
	rd_special	VOL_TEMP1, cwp
	wr_special	cwp, VOL_TEMP1,  $-8	# move back two windows
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
	wr_special	cwp,  r1, $8	# move back ahead two windows
	Nop
	add_nt		r1, SAFE_TEMP2, $0	# Restore r1
	/*
	 * See if need more memory.  We need more if 
	 *
	 * 	swp <= min_swp + MACH_SAVED_WINDOW_SIZE
	 */
	cmp_br_delayed	eq, SAFE_TEMP1, $0, winUnFlow_Return	# No need to
	Nop							#   check from
								#   kernel mode
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr
	rd_special	VOL_TEMP2, swp
	ld_32		VOL_TEMP1, VOL_TEMP1, $MACH_MIN_SWP_OFFSET
	Nop
	add_nt		VOL_TEMP1, VOL_TEMP1, $MACH_SAVED_WINDOW_SIZE
	cmp_br_delayed	gt, VOL_TEMP2, VOL_TEMP1, winUnFlow_Return
	Nop
	/*
	 * Need to get more memory for window underflow.
	 */
	add_nt		NON_INTR_TEMP1, CUR_PC_REG, $0
	add_nt		NON_INTR_TEMP2, NEXT_PC_REG, $0
	rd_special	VOL_TEMP1, pc	# Return from traps and then
	return_trap	VOL_TEMP1, $12	#   take the compare trap to
	Nop				#   get back in in kernel mode.
	cmp_trap	always, r0, r0, $MACH_GET_WIN_MEM_TRAP
	Nop

winUnFlow_Return:
	jump_reg	CUR_PC_REG, $0
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
PowerUp: 			# Jump to power up sequencer

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
FaultIntr:
	/*
	 * On this type of trap we are supposed to return to the current 
	 * PC.
	 */
	rd_kpsw		KPSW_REG
	LD_CONSTANT(SAFE_TEMP1, MACH_KPSW_USE_CUR_PC)
	or		KPSW_REG, KPSW_REG, SAFE_TEMP1
	/*
	 * Read the fault/error status register.
	 */
	READ_STATUS_REGS(MACH_FE_STATUS_0, VOL_TEMP1)
	/*
	 * If no bits are set then it must be an interrupt.
	 */
	cmp_br_delayed	eq, VOL_TEMP1, r0, Interrupt
	Nop
	/*
	 * If any of the bits FEStatus<19:16> are set then is one of the
	 * four VM faults.  Store the fault type in a safe temporary and
	 * call the VMFault handler.
	 */
	extract		SAFE_TEMP1, VOL_TEMP1, $2
	and		SAFE_TEMP1, SAFE_TEMP1, $0xf
	cmp_br_delayed	ne, SAFE_TEMP1, r0, VMFault
	Nop
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
	 * as a arg to the interrupt routine.
	 */
	READ_STATUS_REGS(MACH_INTR_STATUS_0, OUTPUT_REG1)
	WRITE_STATUS_REGS(MACH_INTR_STATUS_0, OUTPUT_REG1)
	/*
	 * The second argument is the kpsw.
	 */
	add_nt		OUTPUT_REG2, KPSW_REG, r0

	/*
	 * Save the insert register in a safe temporary.
	 */
	rd_insert	SAFE_TEMP1

	/*
	 * Disable interrupts but enable all other traps.
	 */
	and		VOL_TEMP2, KPSW_REG, $(~MACH_KPSW_INTR_TRAP_ENA)
	or		VOL_TEMP2, VOL_TEMP2, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP2, $0
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

	/*
	 * We have a bogus user swp.  Switch over to the kernel's stacks
	 * and take the interrupt.  After taking the interrupt kill the user
	 * process.
	 */
	SWITCH_TO_KERNEL_STACKS()
	call		_MachInterrupt
	Nop
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	add_nt		OUTPUT_REG1, r0, $MACH_USER_BAD_SWP
	call		_MachUserError
	Nop
	/* DOESN'T RETURN */

interrupt_GoodSWP:
	SAVE_USER_STATE()
	SWITCH_TO_KERNEL_STACKS()
	call		_MachInterrupt
	Nop
	/*
	 * Restore the insert register and then do a normal return
	 * from trap in case the user process needs to take some action.
	 */
	wr_insert	SAFE_TEMP1
	jump		returnTrap_NormReturn
	Nop

interrupt_KernMode:
	call 		_MachInterrupt
	Nop
	/*
	 * Restore insert register and kpsw and return.
	 */
	wr_insert	SAFE_TEMP1
	wr_kpsw		KPSW_REG, $0
	jump_reg	CUR_PC_REG, $0
	return_trap	NEXT_PC_REG, $0
	Nop

/*
 *---------------------------------------------------------------------------
 *
 * VMFault --
 *
 *	Handle virtual memory faults.  The current fault type was stored
 *	in SAFE_TEMP1 before we were called.
 *
 *---------------------------------------------------------------------------
 */
VMFault:
	/*
	 * Check kernel or user mode.
	 */
	and		VOL_TEMP1, KPSW_REG, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, $0, vmFault_GetDataAddr
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

vmFault_GetDataAddr:
	/*
	 * Enable all traps.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0

	/*
	 * See if is a load, store or test-and-set instruction.  These types
	 * of instructions have opcodes less than 0x30.  If we find one
	 * of these then we have to extract the data address.
	 */
	FETCH_CUR_INSTRUCTION(VOL_TEMP1)
	extract		VOL_TEMP1, VOL_TEMP1, $3  	# Opcode <31:25> -> 
							#	 <07:01>
	srl		VOL_TEMP1, VOL_TEMP1, $1	# Opcode <07:01> ->
							#	 <06:00>
	and		VOL_TEMP1, VOL_TEMP1, $0xf0	# Get upper 4 bits.
	add_nt		VOL_TEMP2, r0, $0x30
	cmp_br_delayed	ge, VOL_TEMP1, VOL_TEMP2, vmFault_NoData
	Nop
	/*
	 * Get the data address by calling ParseInstruction.  We pass the
	 * address to return to in VOL_TEMP1 and the PC of the faulting 
	 * instruction is already in CUR_PC_REG.
	 */
	rd_insert	SAFE_TEMP2
	rd_special	VOL_TEMP1, pc
	add_nt		VOL_TEMP1, VOL_TEMP1, $16
	jump		ParseInstruction
	Nop
	wr_insert	SAFE_TEMP2
	/*
	 * We now have the data address in VOL_TEMP2
	 */
	add_nt		OUTPUT_REG3, r0, $1	# 3rd arg is TRUE to indicate
						#   that there is a data addr
	add_nt		OUTPUT_REG4, VOL_TEMP2, $0	# 4th arg is the data
							#    addr.
	cmp_br_delayed	always, r0, r0, vmFault_CallHandler
	Nop
vmFault_NoData:
	add_nt		OUTPUT_REG3, r0, $0		# 3rd arg is FALSE
							#   (no data addr)
vmFault_CallHandler:
	add_nt		OUTPUT_REG1, SAFE_TEMP1, $0	# 1st arg is fault type.
	add_nt		OUTPUT_REG2, CUR_PC_REG, $0	# 2nd arg is the 
							#   faulting PC.
	add_nt		OUTPUT_REG5, KPSW_REG, $0	# 5th arg to 
							#   MachVMFault is the
							#   kpsw
	rd_insert	VOL_TEMP1
	call		_MachVMFault
	Nop
	wr_insert	VOL_TEMP1

	/* 
	 * Clear fault bits.
	 */
	st_external	SAFE_TEMP1, r0, $MACH_FE_STATUS_2|MACH_CO_WR_REG	
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
CmpTrap:
	/*
	 * On this type of trap we are supposed to return to next PC instead
	 * or cur PC.
	 */
	rd_kpsw		KPSW_REG
	LD_CONSTANT(SAFE_TEMP1, MACH_KPSW_USE_NEXT_PC)
	or		KPSW_REG, KPSW_REG, SAFE_TEMP1

	/*
	 * Get the trap number.
	 */
	FETCH_CUR_INSTRUCTION(r17)
	and		SAFE_TEMP1, r17, $0x1ff 
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
	sll		VOL_TEMP1, SAFE_TEMP1, $3	# Multiple by 8 to
							#   get offset
	rd_special	VOL_TEMP2, pc
	add_nt		VOL_TEMP2, VOL_TEMP1, $16
	jump_reg	VOL_TEMP2, $0
	Nop
	jump		SysCallTrap	
	Nop
	jump		SigReturnTrap
	Nop
	jump		GetWinMemTrap
	Nop
	jump		BreakpointTrap
	Nop
	jump		cmpTrap_FPUTrap
	Nop
	jump		cmpTrap_IllegalTrap
	Nop
	jump		cmpTrap_FixnumTrap
	Nop
	jump		cmpTrap_OverflowTrap
	Nop
	jump		cmpTrap_BadSWPTrap
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

cmpTrap_BadSWPTrap:
	add_nt		OUTPUT_REG1, r0, $MACH_USER_BAD_SWP
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
	rd_insert	VOL_TEMP2
	call		_MachUserError
	Nop
	wr_insert	VOL_TEMP2
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
	ld_32		SAFE_TEMP1, r0, $_machCurStatePtr
	Nop
	ld_32		SAFE_TEMP2, SAFE_TEMP1, $(MACH_TRAP_REGS_OFFSET + 8 * 16)
	Nop
	/*
	 * Check number of kernel call for validity.
	 */
	ld_32		VOL_TEMP1, r0, $_machMaxSysCall
	Nop
	cmp_br_delayed	le, SAFE_TEMP2, VOL_TEMP1, sysCallTrap_FetchArgs
	Nop
	/*
	 * Bad kernel call.  Take a user error and then do a normal return
	 * from trap.
	 */
	add_nt		OUTPUT_REG1, r0, $MACH_BAD_SYS_CALL
	call		_MachUserError
	Nop
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
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
	 * the user stack pointer - numArgs * 4 to point to the base
	 * of the arguments on the spill stack.
	 */
	ld_32		VOL_TEMP1, SAFE_TEMP1, $(MACH_TRAP_REGS_OFFSET + 8 * MACH_SPILL_SP)
	Nop
	sll		VOL_TEMP3, SAFE_TEMP3, $2
	add_nt		VOL_TEMP1, VOL_TEMP1, VOL_TEMP2
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
	ld_32		VOL_TEMP2, VOL_TEMP1, $-28	# 7 args
	Nop
	st_32		VOL_TEMP2, SPILL_SP, $-28
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-24	# 6 args
	Nop
	st_32		VOL_TEMP2, SPILL_SP, $-24
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-20	# 5 args
	Nop
	st_32		VOL_TEMP2, SPILL_SP, $-20
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-16	# 4 args
	Nop
	st_32		VOL_TEMP2, SPILL_SP, $-16
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-12	# 3 args
	Nop
	st_32		VOL_TEMP2, SPILL_SP, $-12
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-8	# 2 args
	Nop
	st_32		VOL_TEMP2, SPILL_SP, $-8
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $-4	# 1 args
	Nop
	st_32		VOL_TEMP2, SPILL_SP, $-4
	Nop
	.globl _MachFetchArgEnd
_MachFetchArgEnd:
	/*
	 * Set the kernel's spill stack.
	 */
	add_nt		SPILL_SP, VOL_TEMP3, $0

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
	 * instruction.  This forces to advance the window by a call
	 * and then do a jump_reg.  Unfortunately this means that we can't
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
	add_nt		RETURN_ADDR_REG, RETURN_ADDR_REG, $16
	jump_reg	r9, $0
	Nop

SysCall_Return:
	/*
	 * Now we are back from the system call handler.  Store the return
	 * value into the return val reg for the parent, restore the kernels
	 * stack pointer and the kpsw and then do a normal return from
	 * trap.
	 */
	st_40		RETURN_VAL_REG, SAFE_TEMP1, $(MACH_TRAP_REGS_OFFSET + 8 * MACH_RETURN_VAL_REG)
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
SigReturnTrap:
	/*
	 * Enable traps.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	/*
	 * Go back to the previous window.
	 */
	rd_special	VOL_TEMP1, pc
	return		VOL_TEMP1, $12
	Nop
	/*
	 * Disable traps.  Note that the kpsw that we use now is the one
	 * that we saved in the previous window when we called the signal
	 * handler.  It will have all traps disabled.
	 */
	wr_kpsw		KPSW_REG, $0
	/*
	 * Switch over to the kernel stacks and save the user state.
	 */
	SAVE_USER_STATE()
	SWITCH_TO_KERNEL_STACKS()
	/*
	 * Reenable traps.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	/*
	 * The old hold mask was stored in NON_INTR_TEMP1.  Also the first and
	 * second PCs were saved in CUR_PC_REG and NEXT_PC_REG.  Call
	 * the signal return handler with the old hold mask as an argument.
	 */
	add_nt		OUTPUT_REG1, NON_INTR_TEMP1, $0
	rd_insert	VOL_TEMP1
	call		_MachSigReturn
	Nop
	wr_insert	VOL_TEMP1

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
GetWinMemTrap:
	VERIFY_SWP(getWinMemTrap_GoodSWP)
	USER_SWP_ERROR()
	/* DOESN'T RETURN */

getWinMemTrap_GoodSWP:
	/*
	 * Enable all traps.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	/*
	 * Call _MachGetWinMem()
	 */
	rd_insert	VOL_TEMP1
	call		_MachGetWinMem
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * Restore the current and next PCs and then do a
	 * normal return from trap.
	 */
	add_nt		CUR_PC_REG, NON_INTR_TEMP1, $0
	add_nt		NEXT_PC_REG, NON_INTR_TEMP2, $0
	add_nt		RETURN_VAL_REG, r0, $MACH_NORM_RETURN
	jump		ReturnTrap
	Nop

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
	ld_32		VOL_TEMP2, r0, $_machSpecialHandlingOffset
	Nop
	add_nt		VOL_TEMP1, VOL_TEMP1, VOL_TEMP2
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	Nop
	cmp_br_delayed	ne, VOL_TEMP1, $0, returnTrap_SpecialAction
	Nop

	/*
	 * Restore the user's state and put us back into user mode.
	 */
	RESTORE_USER_STATE()
	/*
	 * Put us back into user mode.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_CUR_MODE
	wr_kpsw		VOL_TEMP1, $0

returnTrap_Return:
	/*
	 * If we are supposed to return to the next PC then set the current
	 * PC to the next PC and clear the next PC so that the return
	 * happens correctly.
	 */
	LD_CONSTANT(VOL_TEMP2, MACH_KPSW_USE_CUR_PC)
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
	jump_reg	CUR_PC_REG, $0
	return_trap	NEXT_PC_REG, $0

returnTrap_No2ndPC:
	return_trap	CUR_PC_REG, $0
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
	rd_insert	VOL_TEMP1
	call		_MachUserAction
	Nop
	wr_insert	VOL_TEMP1
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
 *	Need to start the process off calling a signal handler.
 */
returnTrap_CallSigHandler:
	wr_kpsw		KPSW_REG, $0
	RESTORE_USER_STATE()
	/* 
	 * Save the old hold mask in the current window.  The PCs
	 * are already saved in CUR_PC_REG and NEXT_PC_REG.  Note that the
	 * current state pointer is put in OUTPUT_REG5 so that we can use
	 * it after we shift the window.
	 */
	ld_32		OUTPUT_REG5, r0, $_machCurStatePtr
	Nop
	ld_32		NON_INTR_TEMP1, OUTPUT_REG5, $MACH_OLD_HOLD_MASK_OFFSET
	Nop
	/*
	 * Load in the PC and the arguments to the signal handler.
	 */
	ld_32		OUTPUT_REG1, OUTPUT_REG5, $MACH_SIG_NUM_OFFSET
	ld_32		OUTPUT_REG2, OUTPUT_REG5, $MACH_SIG_CODE_OFFSET
	ld_32		OUTPUT_REG3, OUTPUT_REG5, $MACH_OLD_HOLD_MASK_OFFSET
	Nop
	/*
	 * Enable traps so that we can safely advance the window.
	 */
	or		VOL_TEMP1, KPSW_REG, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1, $0
	/*
	 * Advance to the window where the signal handler will execute.
	 */
	call		1f
	Nop
1:
	/*
	 * Set the signal handler executing in user mode.
	 */
	add_nt		RETURN_ADDR_REG, r0, $SigReturnAddr
	ld_32		VOL_TEMP1, INPUT_REG5, $MACH_NEW_CUR_PC_OFFSET
	rd_kpsw		VOL_TEMP2
	or		VOL_TEMP2, VOL_TEMP2, $MACH_KPSW_CUR_MODE|MACH_KPSW_ALL_TRAPS_ENA
	jump_reg	VOL_TEMP1, $0
	wr_kpsw		VOL_TEMP2, $0

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
 *	NOTE: We are called with all traps disabled.  This is important
 *	      because since we are going back to previous windows we can't
 *	      afford to take interrupts because otherwise we would
 *	      trash some windows.
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
	st_32		CUR_PC_REG, VOL_TEMP1, $MACH_REG_STATE_CUR_PC_OFFSET
	st_32		NEXT_PC_REG, VOL_TEMP1, $MACH_REG_STATE_NEXT_PC_OFFSET
	rd_insert	VOL_TEMP3
	st_32		VOL_TEMP3, VOL_TEMP1, $MACH_REG_STATE_INSERT_OFFSET

	/*
	 * Save all of the globals.
	 */
	st_40		r0, VOL_TEMP1, $0
	st_40		r1, VOL_TEMP1, $8
	st_40		r2, VOL_TEMP1, $16
	st_40		r3, VOL_TEMP1, $24
	st_40		r4, VOL_TEMP1, $32
	st_40		r5, VOL_TEMP1, $40
	st_40		r6, VOL_TEMP1, $48
	st_40		r7, VOL_TEMP1, $56
	st_40		r8, VOL_TEMP1, $64
	st_40		r9, VOL_TEMP1, $72

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
	st_40		r10, r1, $80
	st_40		r11, r1, $88
	st_40		r12, r1, $96
	st_40		r13, r1, $104
	st_40		r14, r1, $112
	st_40		r15, r1, $120
	st_40		r16, r1, $128
	st_40		r17, r1, $136
	st_40		r18, r1, $144
	st_40		r19, r1, $152
	st_40		r20, r1, $160
	st_40		r21, r1, $168
	st_40		r22, r1, $176
	st_40		r23, r1, $184
	st_40		r24, r1, $192
	st_40		r25, r1, $200
	st_40		r26, r1, $208
	st_40		r27, r1, $216
	st_40		r28, r1, $224
	st_40		r29, r1, $232
	st_40		r30, r1, $240
	st_40		r31, r1, $248

	/*
	 * Now push all of the windows before the current one onto the saved
	 * window stack.  Note that swp points to the last window
	 * saved so that we have to save from swp + 1 up through cwp - 1.
	 * r1 contains the swp, r2 the cwp and r3  the current value of the 
	 * swp shifted over to align with the cwp.
	 */
	rd_special	r1, swp
	and		r1, r1, $0x380
	rd_special	r2, cwp
	and		r2, r2, $0x1c
	/*
	 * Set r3 to the swp aligned with the cwp so that we can use it for
	 * comparisons.
	 */
	srl		r3, r1, $1	# Read the swp and then shift it so
	srl		r3, r3, $1	#    it aligns with the cwp.  That is
	srl		r3, r3, $1	#    swp<9:7> -> swp<4:2>
	srl		r3, r3, $1
	srl 		r3, r3, $1

saveState_SaveRegs:
	add_nt		r3, r3, $4
	and		r3, r3, $0x1c
	cmp_br_delayed	eq, r3, r2, saveState_Done
	Nop
	wr_special	cwp, r3, $0	# Set the cwp to the window to save.
	Nop
					# Increment the swp by one window.
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
	wr_special	cwp, r2, $0x4	# Move back to the current window.
	wr_special	swp, r1, $0	# Update the swp.
	/*
	 * Save the cwp and swp.
	 */
	st_32		r1, VOL_TEMP1, $MACH_TRAP_SWP_OFFSET
	st_32		r2, VOL_TEMP1, $MACH_TRAP_CWP_OFFSET
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
	ld_40		r10, r1, $80
	ld_40		r11, r1, $88
	ld_40		r12, r1, $96
	ld_40		r13, r1, $104
	ld_40		r14, r1, $112
	ld_40		r15, r1, $120
	ld_40		r16, r1, $128
	ld_40		r17, r1, $136
	ld_40		r18, r1, $144
	ld_40		r19, r1, $152
	ld_40		r20, r1, $160
	ld_40		r21, r1, $168
	ld_40		r22, r1, $176
	ld_40		r23, r1, $184
	ld_40		r24, r1, $192
	ld_40		r25, r1, $200
	ld_40		r26, r1, $208
	ld_40		r27, r1, $216
	ld_40		r28, r1, $224
	ld_40		r29, r1, $232
	ld_40		r30, r1, $240
	ld_40		r31, r1, $248
	/*
	 * Go forward to the window that we are to execute in.
	 */
	wr_special	cwp, r2, $4
	Nop
	add_nt		VOL_TEMP1, r1, $0
	add_nt		VOL_TEMP2, r9, $0
	/*
	 * Restore the current PC, next PC, insert register, kpsw and the upsw.
	 */
	ld_32		KPSW_REG, VOL_TEMP1, $MACH_REG_STATE_KPSW_OFFSET
	ld_32		r1, VOL_TEMP1, $MACH_REG_STATE_UPSW_OFFSET
	ld_32		r2, VOL_TEMP1, $MACH_REG_STATE_INSERT_OFFSET
	ld_32		CUR_PC_REG, VOL_TEMP1, $MACH_REG_STATE_CUR_PC_OFFSET
	ld_32		NEXT_PC_REG, VOL_TEMP1, $MACH_REG_STATE_NEXT_PC_OFFSET
	wr_special	upsw, r1, $0
	wr_insert	r2
	/*
	 * Restore the globals.
	 */
	ld_40		r1, VOL_TEMP1, $8
	ld_40		r2, VOL_TEMP1, $16
	ld_40		r3, VOL_TEMP1, $24
	ld_40		r4, VOL_TEMP1, $32
	ld_40		r5, VOL_TEMP1, $40
	ld_40		r6, VOL_TEMP1, $48
	ld_40		r7, VOL_TEMP1, $56
	ld_40		r8, VOL_TEMP1, $64
	ld_40		r9, VOL_TEMP1, $72
	/*
	 * Return to our caller.
	 */
	jump_reg	VOL_TEMP2, $0
	Nop

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
 * MachContextSwitch(fromProcPtr, toProcPtr) --
 *
 *	Switch the thread of execution to a new process.  fromProcPtr
 *	contains a pointer to the process to switch from and toProcPtr
 *	a pointer to the process to switch to.
 *
 *----------------------------------------------------------------------------
 */
	.globl	_MachContextSwitch
_MachContextSwitch:
	/*
	 * Setup the virtual memory context for the process.
	 */
	add_nt		OUTPUT_REG1, INPUT_REG2, $0
	call		_VmMach_SetupContext
	Nop
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
	st_32		VOL_TEMP1, r0, $_machCurStatePtr
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
	return		RETURN_ADDR_REG, $0
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * Mach_DisableIntr --
 *
 *	Mach_DisableIntr()
 *
 *	Disable interrupts.
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
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1, VOL_TEMP1, $(~MACH_KPSW_INTR_TRAP_ENA)
	wr_kpsw		VOL_TEMP1, $0
	return		RETURN_ADDR_REG, $8
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * Mach_EnableIntr --
 *
 *	void Mach_EnableIntr()
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
	.globl	_Mach_EnableIntr
_Mach_EnableIntr:
	rd_kpsw		VOL_TEMP1
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_INTR_TRAP_ENA
	wr_kpsw		VOL_TEMP1, $0
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
	return	RETURN_ADDR_REG, $8
	Nop

/*
 * ParseInstruction --
 *
 *	Relevant instructions: LD_40*, LD_32*, ST_40, ST_32, TEST_AND_SET.
 *	We want to get the address of the operand that accesses memory.
 *	The address of the instruction is passed in r10 and the address to 
 *	jump to when done is passed in VOL_TEMP1.  Note that since the act of
 *	doing calls to switch windows trashes r10 we have to save r10.  Also
 *	since we use r14 and r15 we have to save and restore them at the end.
 *	We return the data address from the instruction in VOL_TEMP2.
 *
 *	SRC1_REG:	r14 -- src1 register
 *	SRC1_VAL:	r14 -- Value of src1 register.
 *	PREV_SRC1_REG:	r30 -- src1 register in previous window.
 *	SRC2_REG:	r15 -- src2 register or immediate.
 *	SRC2_VAL:	r15 -- Value of src2 register or immediate.
 *	PREV_SRC2_REG:	r31 -- src2 register in previous window.
 *	RET_ADDR:	r18 (VOL_TEMP1)  -- Address to return to when done.
 *	DATA_VAL_REG:	r19 (VOL_TEMP2) -- Data address from instruction.
 *	TRAP_INST:	r19 (VOL_TEMP2) -- Trapping instruction.
 *	OPCODE:		r20 (VOL_TEMP3) -- Opcode.
 *	CALLER_TEMP1	r21 (SAFE_TEMP1) -- Temporary reserved for caller.
 *	CALLER_TEMP2	r22 (SAFE_TEMP2) -- Temporary reserved for caller.
 *	CALLER_TEMP3	r23 (SAFE_TEMP3) -- Temporary reserved for caller.
 *	PARSE_TEMP1:	r24 -- One temporary to use.
 *	PARSE_TEMP2:	r25 -- 2nd temporary to use.
 *	SAVED_R10:	r27 -- Place to save r10.
 *	SAVED_R14:	r28 -- Place to save r14.
 *	SAVED_R15:	r29 -- Place to save r15.
 */

#define	SRC1_REG		r14
#define	SRC1_VAL		r14
#define	PREV_SRC1_REG		r30
#define	SRC2_REG		r15
#define	SRC2_VAL		r15
#define	PREV_SRC2_REG		r31
#define	RET_ADDR		r18
#define	DATA_VAL_REG		r19
#define	TRAP_INST		r19
#define	OPCODE			r20
#define	PARSE_TEMP1		r24
#define	PARSE_TEMP2		r25
#define	SAVED_R10		r27
#define	SAVED_R14		r28
#define	SAVED_R15		r29

ParseInstruction:
	FETCH_CUR_INSTRUCTION(TRAP_INST)
	add_nt		SAVED_R10, r10, $0	# Save "return" address
	add_nt		SAVED_R14, r14, $0	# Save r14 and r15 because
	add_nt		SAVED_R15, r15, $0	#  these will be used to 
						#  recover operands.
	extract		OPCODE, TRAP_INST, $3	# Opcode <31:25> -> <07:01>
	srl		OPCODE, OPCODE, $1	# Opcode <07:01> -> <06:00>
	sll		SRC1_REG, TRAP_INST, $1	# s1 <19:15> to <20:16>
	extract		SRC1_REG, SRC1_REG, $2	# s1 <20:16> to <04:00>
	and		SRC1_REG, SRC1_REG, $0x1f
	srl		SRC2_REG, TRAP_INST, $1	# s2 <13:09> to <12:08>
	extract		SRC2_REG, SRC2_REG, $1	# s2 <12:08> to <04:00>
	and		SRC2_REG, SRC2_REG, $0x1f

	/*
	 * Go back one window.
	 */
	rd_special	PARSE_TEMP1, pc
	return		PARSE_TEMP1, $12
	Nop

	/*
	 * Now we're in the previous window (where the trap occurred)
	 * If s1 < 30 then go through the jump table to recover the value.
	 * Otherwise do it by hand since we trashed r14 and r15 which are r30
	 * and r31 respectively in the previous window.
	 */
	cmp_br_delayed	 lt, PREV_SRC1_REG, $30, parse4
	Nop
	call		parse1up		# Back to trap handler window.
	Nop
parse1up:
	cmp_br_delayed	eq, SRC1_REG,  $30, parse1a
	Nop
	cmp_br_delayed	eq, SRC1_REG,  $31, parse1b
	Nop
	CALL_DEBUGGER(r0, MACH_BAD_SRC_REG)

parse1a:
	add_nt		SRC1_VAL, SAVED_R14, $0
	jump		parse5
	Nop

parse1b: 
	add_nt		SRC1_VAL, SAVED_R15, $0
	jump 		parse5
	Nop
	
parse4:	sll		PREV_SRC1_REG, PREV_SRC1_REG, $2	# s1 = s1 * 4
	jump_reg	PREV_SRC1_REG, $OpRecov  # Retrieve value of first op.
	jump	        parse41		 	# Value is returned in r31

parse41:
	add_nt		r30, r31, $0		# Move to src1's register.
	call 	       	parse5			# Get back to trap window. 
	Nop

parse5:	
	/*
	 * SRC1_VAL now contains src1.  Additional offset will depend on
	 * type of operation.  Load can take either register or immediate
	 * value.  Store takes immediate value only, and from two places.
	 * We have a store if opcode<7> = 2.
	 * Currently back in trap handler register window.
	 */
	and		PARSE_TEMP1, OPCODE, $0x20
	cmp_br_delayed  ne, PARSE_TEMP1, $0, parse_store
	Nop

	/*
	 * Parsing load or test&set.  Check for register or immediate value.
	 */
	extract		PARSE_TEMP1, TRAP_INST, $1
	and		PARSE_TEMP1, PARSE_TEMP1, $0x40
	cmp_br_delayed  ne, PARSE_TEMP1, $0, parse5a
	Nop
	cmp_br_delayed	always, r0, r0, parse5b
	Nop

parse5a:
	/*
	 * 2nd operand is an immediate.
	 */
	and		SRC2_VAL, TRAP_INST, $0x1fff	# Extract immediate val.
	add_nt		PARSE_TEMP1, TRAP_INST, $0	# Check for a 
	srl		PARSE_TEMP1, PARSE_TEMP1, $1	#   negative number
	and		PARSE_TEMP1, PARSE_TEMP1, $0x1000 
	cmp_br_delayed	eq, PARSE_TEMP1, $0, parse_end
	Nop
	add_nt		SRC2_VAL, SRC2_VAL, $~0x1fff	# Sign extend SRC2_VAL
	cmp_br_delayed	always, r0, r0, parse_end
	Nop

parse5b:
	/*
	 * Go back one window.
	 */
	rd_special	PARSE_TEMP2, pc
	return 		PARSE_TEMP2, $12
	Nop

	/*
	 * Now we're in the previous window (where the trap occurred)
	 * If s2 < 30 then go through the jump table to recover the value.
	 * Otherwise do it by hand since we trashed r14 and r15 which are
	 * r30 and r31 respectively in the previous window.
	 */
	cmp_br_delayed	 lt, PREV_SRC2_REG, $30, parse6
	Nop
	call		pars2up
	Nop
pars2up:
	cmp_br_delayed	 eq, SRC2_REG,  $30, parse2a
	Nop
	cmp_br_delayed	 eq, SRC2_REG,  $31, parse2b
	Nop
	CALL_DEBUGGER(r0, MACH_BAD_SRC_REG)

parse2a:
	add_nt		SRC2_VAL, SAVED_R14, $0
	jump		parse_end
	Nop

parse2b:
	add_nt		SRC2_VAL, SAVED_R15, $0
	jump		parse_end
	Nop

parse6:	sll		PREV_SRC2_REG, PREV_SRC2_REG, $2  # Multiply src2 reg
							  #   by 4 to represent
							  #   offset in jump 
							  #   table.
	jump_reg	PREV_SRC2_REG, $OpRecov  # Recover 2nd operand register
	jump 		parse7			 #   and put value in r31
parse7:	
	call		parse_end
	Nop

/*
 * Handle store here by getting immediate part and putting in SRC2_VAL.  Then
 * join up with the load/store logic and calculate address.  For stores,
 * we are in the trap handler register window all along.
 */

parse_store:
	sll		PARSE_TEMP1, TRAP_INST, $3	# Dest<24:20> move to <27:23>
	sll		PARSE_TEMP1, PARSE_TEMP1, $2	# Move to <29:25>
	extract		PARSE_TEMP1, PARSE_TEMP1, $3	# Move to <5:1>  
	wr_insert	$1
	insert		PARSE_TEMP1, r0, PARSE_TEMP1	# Move to <13:09>
	srl		PARSE_TEMP2, PARSE_TEMP1, $1	# Sign-bit <13> to <12>
	and		PARSE_TEMP2, PARSE_TEMP2, $0x1000 # Check for negative
	cmp_br_delayed	eq, PARSE_TEMP2, $0, parse_pos    #    number
	and 		PARSE_TEMP1, PARSE_TEMP1,$0x1e00  # Mask out valid bits.
	add_nt		PARSE_TEMP1, PARSE_TEMP1,$~0x1fff  # Sign extend 
							  #    PARSE_TEMP2.
parse_pos:
	and		PARSE_TEMP2, TRAP_INST, $0x01ff	     # Put together into
	or		SRC2_VAL, PARSE_TEMP1,  PARSE_TEMP2  #    14-bit value.

/*
 *  In the trap window.  SRC1_VAL contains first part of address.
 *  SRC2_VAL contains second part.  Now add them and mask.
 */
parse_end:
	add_nt		DATA_VAL_REG, SRC1_VAL, SRC2_VAL	
	add_nt		r10, SAVED_R10, $0		# Restore pre-parse r10
	add_nt		r14, SAVED_R14, $0		# Restore pre-parse r14
	add_nt		r15, SAVED_R15, $0		# Restore pre-parse r15
	jump_reg	RET_ADDR, r0			# Go back to caller
	Nop

/*
 * Leave room for the stacks.
 */
.org MACH_CODE_START
