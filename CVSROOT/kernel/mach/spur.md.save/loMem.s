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
 * When user processes trap into the kernel the kernel uses the user saved
 * window stack rather than its own.  This is done for simplicity: we only
 * have to worry about one stack.  However, since the SWP can be written
 * in user mode the kernel has to be careful before using the user's
 * SWP.  Thus before the kernel does anything with a user process the first
 * thing that it does is verify that the SWP is valid.  It can do this
 * by looking at the bounds on the SWP that are present in each processes
 * machine state info.  An SWP is valid if it satisfies the following
 * constraints:
 *
 *	    swp >= min_swp_offset - MACH_SAVED_WINDOW_SIZE and
 *	    swp + MACH_OVERFLOW_EXTRA <= max_swp_offset
 *
 * The MACH_OVERFLOW_EXTRA worth of data at the top is there in case we need 
 * to save windows when we are executing in the kernel on behalf of a user
 * process.  It also ensures that we have room to save the current window
 * if we take a window overflow trap from user mode.  The
 * MACH_SAVED_WINDOW_SIZE at the bottom is in case an interrupt comes between
 * the time that we do an underflow and we try to get more window memory.
 *
 * If a user's SWP is found to be bogus then the kernel switches over to
 * the kernel's saved window stack and the user process is killed.  If
 * the SWP is bogus when an interrupt occurs, then the user process is not
 * killed until after the interrupt is handled.  All memory between 
 * min_swp_offset and max_swp_offset is wired down.
 *
 * More memory is allocated for a user process's saved window stack after
 * it takes a underflow or overflow fault right before it returns to user mode.
 * Since while a process is in the kernel user windows can be saved on the
 * saved window stack without allocating more memory, memory is allocated
 * not when there is only MACH_OVERFLOW_EXTRA available on the stack but
 * when there is less than MACH_OVERFLOW_EXTRA + MACH_OVERFLOW_SLOP where
 * MACH_OVERFLOW_SLOP is equal to the maximum amount of user windows (8) that
 * can be saved on a user's saved window stack while it is executing in
 * the kernel.
 *
 * SPILL STACK CONVENTIONS
 *
 * When user processes trap into the kernel the kernel will switch over
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
 * Trap table.  The hardware jumps to virtual 0x1000 when any type of trap
 * occurs.
 */
	.org 0x1000
	jump PowerUp		# Reset - Jump to powerup sequencer.
	Nop

	.org 0x1010
	jump Error		# Error
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
_proc_RunningProcesses: 	.long 0
_machCurStatePtr: 		.long 0
_machStatePtrOffset:		.long 0
_machSpecialHandlingOffset:	.long 0
debugStatePtr			.long _machDebugState

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
	Error					# last two cases are errors
	Error

/*
 ****************************************************************************
 *
 * END: SPECIAL LOW MEMORY OBJECTS
 *
 ****************************************************************************
 */

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
	VERIFY_SWP(winOvFlow_SaveWindow, 0)	# Verify that the SWP is OK.
	USER_ERROR(MACH_USER_BAD_SWP)
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
	 * See if we have to allocate more memory.
	 */
	cmp_br_delayed	eq, SAFE_TEMP1, r0, winOvFlow_Return	# No need to
	Nop							#  check from
								#  kernel mode
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr
	Nop
	ld_32		VOL_TEMP1, VOL_TEMP1, $MACH_MAX_SWP_OFFSET
	rd_special	VOL_TEMP2, swp
	add_nt		VOL_TEMP2, VOL_TEMP2, $(MACH_OVERFLOW_EXTRA + MACH_OVERFLOW_SLOP)
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
	VERIFY_SWP(winUnFlow_RestoreWindow, MACH_SAVED_WINDOW_SIZE)
	USER_ERROR(MACH_USER_BAD_SWP)
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
	 * See if need more memory.
	 */
	cmp_br_delayed	eq, SAFE_TEMP1, $0, winUnFlow_Return	# No need to
	Nop							#   check from
								#   kernel mode
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr
	rd_special	VOL_TEMP2, swp
	ld_32		VOL_TEMP1, VOL_TEMP1, $MACH_MIN_SWP_OFFSET
	Nop
	cmp_br_delayed	ge, VOL_TEMP2, VOL_TEMP1, winUnFlow_Return
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
	USER_ERROR(MACH_USER_FPU_EXCEPT)
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
	USER_ERROR(MACH_USER_ILLEGAL)
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
	USER_ERROR(MACH_USER_FIXNUM)
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
	USER_ERROR(MACH_USER_OVERFLOW)
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
Error:	
	SWITCH_TO_KERNEL_SPILL_STACK()
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
	SWITCH_TO_KERNEL_SPILL_STACK()
	/*
	 * Read the fault/error status register.
	 */
	READ_STATUS_REG(MACH_FE_STATUS_0, VOL_TEMP1)

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
	cmp_br_delayed	ne, SAFE_TEMP1, 0, VMFault
	Nop
	/*
	 * Can't handle any of these types of faults.
	 */
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
	 * Read the interrupt status register.
	 */
	READ_STATUS_REG(MACH_INTR_STATUS_0,OUTPUT_REG1)

	/*
	 * Save the insert register in a safe temporary.
	 */
	rd_insert	SAFE_TEMP1

	/*
	 * Disable interrupts but enable all other traps.  Save the kpsw
	 * in a safe temporary.
	 */
	read_kpsw	SAFE_TEMP2
	and		VOL_TEMP2, SAFE_TEMP2, $((~MACH_KPSW_INTR_TRAP_ENA)&0x3fff)
	or		VOL_TEMP2, VOL_TEMP2, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP2
	/*
	 * See if took the interrupt from user mode.
	 */
	and		VOL_TEMP2, SAFE_TEMP2, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP2, $0, interrupt_KernMode
	Nop

	/*
	 * We took the interrupt from user mode.
	 */
	VERIFY_SWP(interrupt_GoodSWP, 0)

	/*
	 * We have a bogus user swp.  Switch over to the kernel's saved
	 * window stack and take the interrupt.  After taking the interrupt
	 * kill the user process.
	 */
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $MACH_KERN_STACK_START_OFFSET
	wr_special	swp, VOL_TEMP2, $0
	wr_special	cwp, r0, $1
	Nop
	call		_MachInterrupt
	Nop
	read_kpsw	VOL_TEMP1
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_INTR_TRAP_ENA
	write_kpsw	VOL_TEMP1
	add_nt		OUTPUT_REG1, r0, $MACH_USER_BAD_SWP
	call		_MachUserError
	Nop
	/* DOESN'T RETURN */

interrupt_GoodSWP:
interrupt_KernMode:
	call 		_MachInterrupt
	Nop
	/*
	 * Clear the interrupt status register.
	 */
	CLR_INTR_STATUS(0xffffffff)

	/*
	 * Restore user stack pointer, insert register and kpsw.
	 */
	RESTORE_USER_SPILL_SP()
	wr_insert	SAFE_TEMP1
	wr_kpsw		SAFE_TEMP2
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
	read_kpsw	SAFE_TEMP2			# Save KPSW
	add_nt		OUTPUT_REG5, SAFE_TEMP2, $0	# 5th arg to 
							#   MachVMFault is the
							#   kpsw
	/*
	 * Enable all traps.
	 */
	or		VOL_TEMP2, VOL_TEMP2, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP2
	/*
	 * Check kernel or user mode.
	 */
	and		VOL_TEMP2, SAFE_TEMP2, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP2, $0, vmFault_GetDataAddr
	Nop
	VERIFY_SWP(vmFault_GetDataAddr, 0)
	add_nt		OUTPUT_REG1, r0, $MACH_USER_BAD_SWP
	call		_MachUserError
	Nop
	/* DOESN'T RETURN */

vmFault_GetDataAddr:
	FETCH_CUR_INSTRUCTION(VOL_TEMP1)
	extract		VOL_TEMP1, VOL_TEMP1, $3  	# Opcode <31:25> -> 
							#	 <07:01>
	srl		VOL_TEMP1, VOL_TEMP1, $1	# Opcode <07:01> ->
							#	 <06:00>
	and		VOL_TEMP1, VOL_TEMP1, $0xf0	# Get upper 4 bits.
	/*
	 * All instructions besides loads, stores and test-and-set instructions
	 * have opcodes greater than 0x20.
	 */
	cmp_br_delayed	gt, VOL_TEMP1, $0x20, vmFault_NoData
	Nop
	/*
	 * Get the data address by calling ParseInstruction.  We pass the
	 * address to return to in VOL_TEMP1 and the PC of the faulting 
	 * instruction is already in CUR_PC_REG.
	 */
	rd_special	VOL_TEMP1, pc
	add_nt		VOL_TEMP1, VOL_TEMP1, $16
	jump		ParseInstruction
	Nop
	/*
	 * We now have the data address in VOL_TEMP2
	 */
	add_nt		OUTPUT_REG3, r0, $1	# 3rd arg is TRUE to indicate
						#   that there is a data addr
	add_nt		OUTPUT_REG4, VOL_TEMP2, $0	# 4th arg is the data
							#    addr.
	cmp_br_delayed	always, vmFault_CallHandler
	Nop
vmFault_NoData:
	add_nt		OUTPUT_REG3, r0, $0		# 3rd arg is FALSE
							#   (no data addr)
vmFault_CallHandler:
	add_nt		OUTPUT_REG1, SAFE_TEMP1, $0	# 1st arg is fault type.
	add_nt		OUTPUT_REG2, CUR_PC_REG, $0	# 2nd arg is the 
							#   faulting PC.
	rd_insert	VOL_TEMP1
	call		_MachVMFault
	Nop
	wr_insert	VOL_TEMP1
vmFault_ReturnFromTrap:
	/* 
	 * Clear fault bit and restore the kpsw.
	 */
	st_external	SAFE_TEMP1, r0, $MACH_FE_STATUS_2|MACH_WR_REG	
	wr_kpsw		SAFE_TEMP2
	jump		ReturnTrap
	Nop

/*
 *--------------------------------------------------------------------------
 *
 * CmpTrap --
 *
 *	Handle a cmp_trap trap.  This involves determining the trap type
 *	and vectoring to the right spot to handle trap.
 *
 *--------------------------------------------------------------------------
 */
CmpTrap:
	SWITCH_TO_KERNEL_SPILL_STACK()
	rd_kpsw		VOL_TEMP1
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1

	FETCH_CUR_INSTRUCTION(r17)
	and             VOL_TEMP1, r17, $0x1ff        # Get trap number

	/*
	 * Verify the SWP for all but user error traps.  On user error traps
	 * we can't user VERIFY_SWP because the reason that we are trapping
	 * may be because the SWP is bogus.
	 */
	cmp_br_delayed	eq, VOL_TEMP1, $MACH_USER_ERROR_TRAP, 1f
	Nop
	VERIFY_SWP(1f, 0)
	add_nt		OUTPUT_REG1, r0, $MACH_BAD_USER_SWP
	call		_MachUserError
	Nop
	/* DOESN'T RETURN */
1:
	cmp_br_delayed	gt, VOL_TEMP1, $MACH_MAX_TRAP_TYPE, cmpTrap_BadTrapType
	Nop

	sll		VOL_TEMP1, VOL_TEMP1, $3	# Multiple by 8 to
							#   get offset
	rd_special	VOL_TEMP2, pc
	add_nt		VOL_TEMP2, VOL_TEMP1, $16
	jump_reg	VOL_TEMP2, $0
	Nop
	jump		SysCallTrap	
	Nop
	jump		UserErrorTrap
	Nop
	jump		SigReturnTrap
	Nop
	jump		GetWinMemTrap
	Nop
	jump		cmpTrap_CallDebugger
	Nop
cmpTrap_CallDebugger:
	CALL_DEBUGGER(r0, MACH_DEBUGGER_CALL)

cmpTrap_BadTrapType:
	/*
	 * A trap type greater than the maximum value was specified.  If
	 * in kernel mode call the debugger.  Otherwise call
	 * the user error routine.
	 */
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP2, VOL_TEMP1, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP2, $0, cmpTrap_KernError
	Nop
	add_nt		OUTPUT_REG1, r0, $MACH_BAD_TRAP_TYPE
	rd_insert	VOL_TEMP2
	call		_MachUserError
	Nop
	wr_insert	VOL_TEMP2
	and		VOL_TEMP1,VOL_TEMP1,$((~MACH_KPSW_ALL_TRAPS_ENA)&0x3fff)
	wr_kpsw		VOL_TEMP1
	jump		ReturnTrap
	Nop

cmpTrap_KernError:
	CALL_DEBUGGER(r0, MACH_BAD_TRAP_TYPE)

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
 *----------------------------------------------------------------------------
 *
 * UserErrorTrap -
 *
 *	Handle a user error trap.  Before we were called the old CUR_PC_REG 
 *	and NEXT_PC_REG were saved in NON_INTR_TEMP2 and NON_INTR_TEMP3 
 *	respectively and the error type was stored in NON_INTR_TEMP1.
 *
 *----------------------------------------------------------------------------
 */
UserErrorTrap:
	add_nt		OUTPUT_REG1, NON_INTR_TEMP1, $0
	add_nt		CUR_PC_REG, NON_INTR_TEMP2, $0
	add_nt		NEXT_PC_REG, NON_INTR_TEMP3, $0
	cmp_br_delayed	eq, OUTPUT_REG1, $MACH_USER_BAD_SWP, 1f
	Nop
	VERIFY_SWP(2f, 0)
1:
	/*
	 * Switch over to the kernel's saved window stack since our SWP is
	 * bogus.
	 */
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr
	Nop
	ld_32		VOL_TEMP2, VOL_TEMP1, $MACH_KERN_STACK_START
	wr_special	swp, VOL_TEMP2, $0
	wr_special	cwp, r0, $1
	Nop
	add_nt		OUTPUT_REG1, r0, $MACH_USER_BAD_SWP
2:
	rd_insert	VOL_TEMP1
	call		_MachUserError
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * Disable all traps and do a normal return trap.
	 */
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1,VOL_TEMP1,$((~MACH_KPSW_ALL_TRAPS_ENA)&0x3fff)
	wr_kpsw		VOL_TEMP1
	jump		ReturnTrap
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * SigReturnTrap -
 *
 *	Return from signal trap.  The previous window contains
 *	the saved info when we called the signal handler.
 *
 *----------------------------------------------------------------------------
 */
SigReturnTrap:
	/*
	 * Enable traps.
	 */
	rd_kpsw		VOL_TEMP1
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1
	/*
	 * Get back to the previous window.
	 */
	rd_special	VOL_TEMP1, pc
	return		VOL_TEMP1, $12
	Nop
	/*
	 * We are now in the previous window.  The user stack pointer was saved
	 * in NON_INTR_TEMP1 and the old hold mask in NON_INTR_TEMP2.  Also
	 * the first and second PCs were saved in CUR_PC_REG and NEXT_PC_REG.
	 * Restore the stack pointer and then call the signal return handler
	 * with the old hold mask as an argument.
	 */
	add_nt		OUTPUT_REG1, NON_INTR_TEMP2, $0
	rd_insert	VOL_TEMP1
	call		_MachSigReturn()
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * Store the new user stack pointer value into the mach struct.
	 */
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr
	Nop
	st_32		NON_INTR_TEMP1, VOL_TEMP1, $MACH_TRAP_USP_OFFSET
	/*
	 * Restore the kpsw.
	 */
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1, VOL_TEMP1, $((~MACH_KPSW_ALL_TRAPS_ENA)&0x3fff)
	wr_kpsw		VOL_TEMP1
	jump		ReturnTrap
	Nop


/*
 *----------------------------------------------------------------------------
 *
 * GetWinMemTrap --
 *
 *	Get more memory for the window stack.
 *
 *----------------------------------------------------------------------------
 */
GetWinMemTrap:
	VERIFY_SWP(1f, 0)
	add_nt		OUTPUT_REG1, r0, $MACH_USER_BAD_SWP
	call		_MachUserError
	Nop
	/* DOESN'T RETURN */
1:
	/*
	 * Enable all traps.
	 */
	rd_kpsw		SAFE_TEMP2
	or		VOL_TEMP1, SAFE_TEMP2, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1
	/*
	 * Call _MachGetWinMem(swp)
	 */
	rd_special	OUTPUT_REG1, swp
	rd_insert	VOL_TEMP1
	call		_MachGetWinMem
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * Restore the kpsw plush the current and next PCs and then do a
	 * normal return from trap.
	 */
	wr_kpsw		SAFE_TEMP2
	add_nt		CUR_PC_REG, NON_INTR_TEMP1, $0
	add_nt		NEXT_PC_REG, NON_INTR_TEMP2, $0
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
 *	RETURN_VAL_REG.  If it is not one of MACH_NORM_RETURN or 
 *	MACH_FAILED_COPY then it is a kernel error value.
 *
 *----------------------------------------------------------------------------
 */
ReturnTrap:
	/*
	 * Check the return code from the fault handler.
	 */
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_NORM_RETURN, returnTrap_NormReturn
	Nop
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_FAILED_COPY, returnTrap_FailedCopy
	Nop
	CALL_DEBUGGER(RETURN_VAL_REG, 0)

returnTrap_NormReturn:
	/*
	 * If we are not returning to user mode then just return.
	 */
	rd_kpsw		SAFE_TEMP2
	and		VOL_TEMP1, SAFE_TEMP2, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, VOL_TEMP1, 0, returnTrap_Return
	Nop
	/*
	 * See if we have to take any special action for this process.
	 */
	ld_32		VOL_TEMP1, r0, $proc_RunningProcesses
	ld_32		VOL_TEMP2, r0, $machSpecialHandlingOffset
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	Nop
	add_nt		VOL_TEMP1, VOL_TEMP1, VOL_TEMP2
	ld_32		VOL_TEMP1, VOL_TEMP1, $0
	Nop
	cmp_br_delayed	ne, VOL_TEMP1, $0, returnTrap_SpecialAction
	Nop

	/*
	 * Restore the spill sp and put us back into user mode.
	 */
	ld_32		VOL_TEMP1, r0, $_machCurStatePtr
	Nop
	ld_32		SPILL_SP, VOL_TEMP1, $MACH_TRAP_USP_OFFSET
	Nop
	or		SAFE_TEMP2, SAFE_TEMP2, $MACH_KPSW_CUR_MODE
	wr_kpsw		SAFE_TEMP2

returnTrap_Return:
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
	rd_kpsw		VOL_TEMP1
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1
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
	return		RETURN_ADDR_REG, $0
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
	rd_kpsw		SAFE_TEMP1
	or		VOL_TEMP1, SAFE_TEMP1, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1
	rd_insert	VOL_TEMP1
	call		_MachUserAction
	Nop
	wr_insert	VOL_TEMP1
	/*
	 * The user action routine will return TRUE (1) if a signal is
	 * pending and FALSE (0) otherwise.
	 *
	 *	MACH_CALL_SIG_HANDLER	Set up to call a signal handler
	 *				when the process continues.
	 *	MACH_SIG_PENDING	A signal needs to be taken.
	 *	MACH_DO_NOTHING		No action is pending.
	 */
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_CALL_SIG_HANDLER, returnTrap_CallSigHandler
	Nop
	cmp_br_delayed	eq, RETURN_VAL_REG, $MACH_SIG_PENDING, returnTrap_SigPending
	Nop
	wr_kpsw		SAFE_TEMP1
	jump		returnTrap_NormReturn
	Nop

/*
 * returnTrap_CallSigHandler --
 *
 *	Need to start the process off calling a signal handler.
 */
returnTrap_CallSigHandler:
	/* 
	 * Save the current stack pointer in the current window.  The PCs
	 * are already saved in CUR_PC_REG and NEXT_PC_REG.  Note that the
	 * current state pointer is put in OUTPUT_REG5 so that we can use
	 * it after we shift the window.
	 */
	ld_32		OUTPUT_REG5, r0, $_machCurStatePtr
	Nop
	ld_32		NON_INTR_TEMP1, OUTPUT_REG5, $MACH_TRAP_USP_OFFSET
	ld_32		NON_INTR_TEMP2, OUTPUT_REG5, $MACH_OLD_HOLD_MASK_OFFSET
	Nop
	/*
	 * Load in the PC, stack pointer and the arguments to the signal
	 * handler.
	 */
	ld_32		OUTPUT_REG1, OUTPUT_REG5, $MACH_SIG_NUM_OFFSET
	ld_32		OUTPUT_REG2, OUTPUT_REG5, $MACH_SIG_CODE_OFFSET
	ld_32		OUTPUT_REG3, OUTPUT_REG5, $MACH_OLD_HOLD_MASK_OFFSET
	Nop
	/*
	 * Enable traps so that we can safely advance the window.
	 */
	rd_kpsw		VOL_TEMP1
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP1
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
	ld_32		SPILL_SP, INPUT_REG5, $MACH_TRAP_USP_OFFSET
	Nop
	ld_32		VOL_TEMP1, INPUT_REG5, $MACH_NEW_CUR_PC_OFFSET
	rd_kpsw		VOL_TEMP2
	or		VOL_TEMP2, VOL_TEMP2, $MACH_KPSW_CUR_MODE
	jump_reg	VOL_TEMP1, $0
	wr_kpsw		VOL_TEMP2

/*
 * returnTrap_SigPending --
 *
 *	We need to process a user signal.
 */
returnTrap_SigPending:
	/*
	 * Before we call the user error handler save enough state so that
	 * the user can debug his process.  Call the routine to save state.
	 * Note that we don't want to shift the window so we simulate a call
	 * by passing the return address in SAFE_REG2, where to save things
	 * in SAFE_REG1 and then doing a jump.
	 */
	ld_32		SAFE_REG1, r0, $_machCurStatePtr
	Nop
	add_nt		SAFE_REG1, SAFE_REG1, $MACH_TRAP_REG_STATE_OFFSET
	rd_special	SAFE_REG2, pc
	add_nt		SAFE_REG2, SAFE_REG2, $16
	jump		SaveState, $0
	Nop

	/*
	 * Enable all traps.
	 */
	read_kpsw	VOL_TEMP1
	or		VOL_TEMP2, VOL_TEMP2, $MACH_KPSW_ALL_TRAPS_ENA
	wr_kpsw		VOL_TEMP2

	rd_insert	VOL_TEMP2
	call		_MachHandleSig
	Nop
	wr_insert	VOL_TEMP2

	/*
	 * Restore kpsw.
	 */
	wr_kpsw		VOL_TEMP1

	/*
	 * Call the routine to restore state.  The return address is passed
	 * in SAFE_REG2 and where to restore the state from is in SAFE_REG1.
	 */
	ld_32		SAFE_REG1, r0, $_machCurStatePtr
	Nop
	add_nt		SAFE_REG1, SAFE_REG1, $MACH_TRAP_REG_STATE_OFFSET
	rd_special	SAFE_REG2, pc
	add_nt		SAFE_REG2, SAFE_REG2, $16
	jump		RestoreState, $0
	Nop

	/*
	 * Now we have processed our signal.  Go through the normal return
	 * from trap sequence.
	 */
	jump		ReturnTrap
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * SaveState -
 *
 *	Save the state of the process in the given state struct.  Also push
 *	all of the windows, except for the current one, onto the saved
 *	window stack.  SAFE_TEMP1 contains where to save the state to and 
 *	SAFE_TEMP2 contains the return address.  
 *
 *	NOTE: We are called with all traps disabled.  This is important
 *	      because since we are going back to previous windows we can't
 *	      afford to take traps or interrupts because otherwise we would
 *	      trash some windows.
 *
 *----------------------------------------------------------------------------
 */
SaveState:
	/*
	 * Save kpsw, upsw, insert register and the current and next PCs 
	 * of the fault.
	 */
	rd_kpsw		VOL_TEMP1
	st_32		VOL_TEMP1, SAFE_TEMP1, $MACH_REG_STATE_KPSW_OFFSET
	rd_special	VOL_TEMP1, upsw
	st_32		VOL_TEMP1, SAFE_TEMP1, $MACH_REG_STATE_UPSW_OFFSET
	st_32		CUR_PC_REG, SAFE_TEMP1, $MACH_REG_STATE_CUR_PC_OFFSET
	st_32		NEXT_PC_REG, SAFE_TEMP1, $MACH_REG_STATE_NEXT_PC_OFFSET
	rd_insert	VOL_TEMP1
	st_32		VOL_TEMP1, SAFE_TEMP1, $MACH_REG_STATE_INSERT_OFFSET
	/*
	 * Save all of the globals.
	 */
	st_40		r1, SAFE_TEMP1, $8
	st_40		r2, SAFE_TEMP1, $16
	st_40		r3, SAFE_TEMP1, $24
	st_40		r4, SAFE_TEMP1, $32
	st_40		r5, SAFE_TEMP1, $40
	st_40		r6, SAFE_TEMP1, $48
	st_40		r7, SAFE_TEMP1, $56
	st_40		r8, SAFE_TEMP1, $64
	st_40		r9, SAFE_TEMP1, $72
	/*
	 * Set r3 to the swp aligned with the cwp so that we can use it for
	 * comparisons.
	 */
	rd_special	r3, swp		# Read the swp and then shift it so
	srl		r3, r3, $1	#    it aligns with the cwp.  That is
	srl		r3, r3, $1	#    swp<9:7> -> swp<4:2>
	srl		r3, r3, $1
	srl 		r3, r3, $1
	srl		r3, r3, $1
	/*
	 * Move where to save to into a global.
	 */
	add_nt		r1, SAFE_TEMP1, $0
	/*
	 * See if the current value of the cwp is just one past the swp.
	 * If so then the swp points to what we want to save in the state
	 * struct.  Otherwise we have to go back one window.
	 */
	rd_special	r2, cwp
	sub_nt		r2, r2, $4
	and		r2, r2, 0x1c0
	cmp_br_delayed	ne, r2, r3, saveState_1
	Nop
saveState_SWP:
	/*
	 * The SWP points to the window that we want to save.  We restore
	 * the window that the SWP points to, decrement the SWP and then
	 * do the normal saving of windows.
	 */
	wr_special	cwp, r2, $0	# Go back one window
	rd_special	r5, swp
	ld_40		r10, r5, $0
	ld_40		r11, r5, $8
	ld_40		r12, r5, $16
	ld_40		r13, r5, $24
	ld_40		r14, r5, $32
	ld_40		r15, r5, $40
	ld_40		r16, r5, $48
	ld_40		r17, r5, $56
	ld_40		r18, r5, $64
	ld_40		r19, r5, $72
	ld_40		r20, r5, $80
	ld_40		r21, r5, $88
	ld_40		r22, r5, $96
	ld_40		r23, r5, $104
	ld_40		r24, r5, $112
	ld_40		r25, r5, $120
	/*
	 * Go back to the current window and make the swp point to the previous
	 * window since we just restored one.
	 */
	wr_special	cwp, r2, $4
	wr_special	swp, r5, $-MACH_SAVED_WINDOW_SIZE
	sub_nt		r3, r3, $4
	and		r3, r3, 0x1c0

saveState_1:
	/*
	 * Have to go back to previous active window to get register values.
	 */
	wr_special	cwp, r2, $0	# Go back one window.
	Nop
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
	 * Now push all of the windows before the current one
	 * onto the saved window stack.  Note that swp points to last window
	 * saved so that we have to save from swp + 1 up through cwp - 1.
	 * r3 contains the current value of the swp shifted over to align with
	 * the cwp and r2 contains the cwp - 1.
	 */
	rd_special	r1, swp
saveState_SaveRegs:
	add_nt		r3, r3, $4
	and		r3, r3, $0x1c0
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
	rd_special	VOL_TEMP1, swp
	st_32		VOL_TEMP1, SAFE_TEMP1, $MACH_TRAP_SWP_OFFSET
	rd_special	VOL_TEMP1, cwp
	st_32		VOL_TEMP1, SAFE_TEMP1, $MACH_TRAP_CWP_OFFSET
	/*
	 * Return to our caller.
	 */
	jump_reg	SAFE_TEMP2, $0
	Nop


/*
 *----------------------------------------------------------------------------
 *
 * RestoreState -
 *
 *	Restore the state of the process from the given state struct.
 *	window stack.  SAFE_TEMP1 contains where to restore the state from
 *	and SAFE_TEMP2 contains the return address.
 *
 *----------------------------------------------------------------------------
 */
RestoreState:
	/*
	 * Restore the current PC, next PC, insert register, kpsw and the upsw.
	 */
	ld_32		VOL_TEMP1, SAFE_TEMP1, $MACH_REG_STATE_KPSW_OFFSET
	ld_32		VOL_TEMP2, SAFE_TEMP1, $MACH_REG_STATE_UPSW_OFFSET
	ld_32		VOL_TEMP3, SAFE_TEMP1, $MACH_REG_STATE_INSERT_OFFSET
	ld_32		CUR_PC_REG, SAFE_TEMP1, $MACH_REG_STATE_CUR_PC_OFFSET
	ld_32		NEXT_PC_REG, SAFE_TEMP1, $MACH_REG_STATE_NEXT_PC_OFFSET
	wr_kpsw		VOL_TEMP1
	wr_special	upsw, VOL_TEMP2, $0
	wr_insert	VOL_TEMP3
	/*
	 * Restore the previous window from the state struct.
	 */
	rd_special	r2, cwp
	wr_special	cwp, r2, $-4	# Go back one window.
	Nop
	ld_40		r10, SAFE_TEMP1, $80
	ld_40		r11, SAFE_TEMP1, $88
        ld_40		r12, SAFE_TEMP1, $96
        ld_40		r13, SAFE_TEMP1, $104
        ld_40		r14, SAFE_TEMP1, $112
        ld_40		r15, SAFE_TEMP1, $120
        ld_40		r16, SAFE_TEMP1, $128
        ld_40		r17, SAFE_TEMP1, $136
        ld_40		r18, SAFE_TEMP1, $144
        ld_40		r19, SAFE_TEMP1, $152
        ld_40		r20, SAFE_TEMP1, $160
        ld_40		r21, SAFE_TEMP1, $168
        ld_40		r22, SAFE_TEMP1, $176
        ld_40		r23, SAFE_TEMP1, $184
        ld_40		r24, SAFE_TEMP1, $192
        ld_40		r25, SAFE_TEMP1, $200
        ld_40		r26, SAFE_TEMP1, $208
        ld_40		r27, SAFE_TEMP1, $216
        ld_40		r28, SAFE_TEMP1, $224
        ld_40		r29, SAFE_TEMP1, $232
        ld_40		r30, SAFE_TEMP1, $240
        ld_40		r31, SAFE_TEMP1, $248
	/*
	 * Switch back to the current window.
	 */
	wr_special	cwp, r2, $0
	Nop
	/*
	 * Restore the globals.
	 */
	ld_40		r1, SAFE_TEMP1, $8
	ld_40		r2, SAFE_TEMP1, $16
	ld_40		r3, SAFE_TEMP1, $24
	ld_40		r4, SAFE_TEMP1, $32
	ld_40		r5, SAFE_TEMP1, $40
	ld_40		r6, SAFE_TEMP1, $48
	ld_40		r7, SAFE_TEMP1, $56
	ld_40		r8, SAFE_TEMP1, $64
	ld_40		r9, SAFE_TEMP1, $72
	/*
	 * Return to our caller.
	 */
	jump_reg	SAFE_TEMP2, $0
	Nop

/*
 *----------------------------------------------------------------------------
 *
 * MachRunUserProc(regStatePtr)
 *
 *	Start the user process executing.  This involves restoring the
 * 	state of the world from regStatePtr and then doing a normal
 *	return from trap.
 *
 *----------------------------------------------------------------------------
 */
	.globl _MachRunUserProc
_MachRunUserProc:
	/*
	 * Disable all traps.
	 */
	rd_kpsw		VOL_TEMP1
	and		VOL_TEMP1, VOL_TEMP1, $((~MACH_KPSW_ALL_TRAPS_ENA)&0x3fff)
	wr_kpsw		VOL_TEMP1
	/*
	 * Change the cwp so that it points to the next window after the
	 * one that we are about to restore to.
	 */
	add_nt		r1, INPUT_REG1, $0
	ld_32		r2, r1, $MACH_TRAP_CWP_OFFSET
	Nop
	wr_special	cwp, r2, $4
	Nop
	/*
	 * Now we are in the window after the one that we are supposed to
	 * restore to.  Now restore the windows.
	 */
	add_nt		SAFE_TEMP1, r1, $0
	rd_special	SAFE_REG2, pc
	add_nt		SAFE_REG2, SAFE_REG2, $16
	jump		RestoreState, $0
	Nop
	/*
	 * Set the previous mode bit in the kpsw to indicate that we came
	 * from user mode and then do a normal return from trap.
	 */
	rd_kpsw		VOL_TEMP1
	or		VOL_TEMP1, VOL_TEMP1, $MACH_KPSW_PREV_MODE
	wr_kpsw		VOL_TEMP1
	jump		ReturnTrap

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
	add_nt		OUTPUT_REG1, INPUT_REG1, $0
	call		_VmMach_SetupContext
	Nop
	/*
	 * Grab a pointer to the state structure to save to.  Note that
	 * the restore and save state routines do not touch any of the
	 * NON_INTR_TEMP registers.
	 */
	ld_32		NON_INTR_TEMP1, r0, $statePtrOffset
	Nop
	add_nt		SAFE_TEMP1, INPUT_REG1, NON_INTR_TEMP1
	ld_32		SAFE_TEMP1, SAFE_TEMP1, $0
	Nop
	/*
	 * Now compute where to save the registers to and call the routine
	 * to save state.  Note that SaveState saves our parent window back.
	 * Thus our locals and output regs will not get modified by saving
	 * and restoring state.
	 */
	add_nt		SAFE_REG1, SAFE_REG1, $MACH_SWITCH_REG_STATE_OFFSET
	rd_special	SAFE_REG2, pc
	add_nt		SAFE_REG2, SAFE_REG2, $16
	jump		SaveState, $0
	Nop
	/*
	 * Grab a pointer to the state structure to restore state from.
	 */
	add_nt		SAFE_TEMP1, INPUT_REG2, NON_INTR_TEMP1
	ld_32		SAFE_TEMP1, SAFE_TEMP1, $0
	Nop
	/*
	 * This is now our current state structure address.
	 */
	st_32		SAFE_TEMP1, r0, $_machCurStatePtr
	/*
	 * Now compute where to restore the registers from and call the routine
	 * to restore state.
	 */
	add_nt		SAFE_REG1, SAFE_REG1, $MACH_SWITCH_REG_STATE_OFFSET
	rd_special	SAFE_REG2, pc
	add_nt		SAFE_REG2, SAFE_REG2, $16
	jump		RestoreState, $0
	Nop
	/*
	 * We are now in the new process so return.
	 */
	return		r10, $0
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
 *	RET_ADDR:	r17 (VOL_TEMP1)  -- Address to return to when done.
 *	DATA_VAL_REG:	r18 (VOL_TEMP2) -- Data address from instruction.
 *	TRAP_INST:	r18 (VOL_TEMP2) -- Trapping instruction.
 *	OPCODE:		r19 (VOL_TEMP3) -- Opcode.
 *	CALLER_TEMP1	r20 (SAFE_TEMP1) -- Temporary reserved for caller.
 *	CALLER_TEMP2	r21 (SAFE_TEMP2) -- Temporary reserved for caller.
 *	CALLER_TEMP3	r22 (SAFE_TEMP3) -- Temporary reserved for caller.
 *	PARSE_TEMP1:	r23 -- One temporary to use.
 *	PARSE_TEMP2:	r24 -- 2nd temporary to use.
 *	SAVED_R10:	r25 -- Place to save r10.
 *	SAVED_R14:	r27 -- Place to save r14.
 *	SAVED_R15:	r28 -- Place to save r15.
 */

#define	SRC1_REG		r14
#define	SRC1_VAL		r14
#define	PREV_SRC1_REG		r30
#define	SRC2_REG		r15
#define	SRC2_VAL		r15
#define	PREV_SRC2_REG		r31
#define	RET_ADDR		VOL_TEMP1
#define	TRAP_INST		VOL_TEMP2
#define	OPCODE			VOL_TEMP3
#define	PARSE_TEMP1		rt7
#define	PARSE_TEMP2		rt8
#define	SAVED_R10		rt9
#define	SAVED_R14		r27
#define	SAVED_R15		r28

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
	CallDebugger(MACH_BAD_SRC_REG)

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
	 * Currently back in trap handler register window.
	 */
	and		PARSE_TEMP1, OPCODE, $0x20	 # Have store if bits 
	cmp_br_delayed  ne, PARSE_TEMP1, $0, parse_store #     <7:4> = 2
	Nop

	/*
	 * Parsing load or test&set.  Check for register or immediate value
	 */
	extract		PARSE_TEMP1, TRAP_INST, $1	# <15:8> to <7:0>
	and		PARSE_TEMP1, PARSE_TEMP1, $0x40	# Check for immediate 
							#     2nd oprd
	cmp_br_delayed  ne, PARSE_TEMP1, $0, parse5a
	Nop
	cmp_br_delayed	always, parse5b
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
	add_nt		SRC2_VAL, SRC2_VAL, $0x2000	# Sign extend SRC2_VAL
	cmp_br_delayed	always, parse_end
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
	CallDebugger(MACH_BAD_SRC_REG)

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
	add_nt		PARSE_TEMP1, PARSE_TEMP1,$0x2000  # Sign extend 
							  #    PARSE_TEMP2.
parse_pos:
	and		PARSE_TEMP2, TRAP_INST, $0x01ff	     # Put together into
	or		SRC2_VAL, PARSE_TEMP1,  PARSE_TEMP2  #    14-bit value.

/*
 *  In the trap window.  SRC1_VAL contains first part of address.
 *  SRC2_VAL contains second part.  Now add them and mask.
 */
parse_end:
	add_nt		SRC1_VAL, SRC1_VAL, SRC2_VAL	
	and		PARSE_TEMP1, OPCODE, $0x02	# Check for 40-bit
	sll		PARSE_TEMP1, PARSE_TEMP1, $1	# If 0x02, then 40-bit
	add_nt		PARSE_TEMP1, PARSE_TEMP1, $0x3ff8   # Gen up mask to 
	and		DATA_VAL_REG, SRC1_VAL, PARSE_TEMP1 #  to make 8-bit
							    #  aligned if 40-bit
	add_nt		r10, SAVED_R10, $0		# Restore pre-parse r10
	add_nt		r14, SAVED_R14, $0		# Restore pre-parse r14
	add_nt		r15, SAVED_R15, $0		# Restore pre-parse r15
	jump_reg	RET_ADDR, r0			# Go back to caller
	Nop

