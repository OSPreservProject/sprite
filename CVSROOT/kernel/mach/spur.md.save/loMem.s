|* loMem.s --
|*
|*	The first thing that is loaded into the kernel.  Handles traps,
|*	faults, errors and interrupts.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*
|* rcs = $Header$ SPRITE (Berkeley)
|*

#include "machConst.h"
#include "machAsmDefs.h"

/*
 * REGISTER CONVENTIONS
 *
 * The routines in this file use the following conventions for using
 * registers:
 *
 *	1) r1-r8:	Globals used by the Lisp system and are left untouched.
 *	2) r9:		Global that we can use as a temporary.
 *	3) r10:		Where the PC to return to when a trap occurs is stored
 *	4) r11-r15:	Used as special temporaries to save things when
 *			taking traps to switch to kernel mode for user
 *			errors and for allocating pages for window overflow
 *			and underflow.  These cannot be touched by the 
 *			interrupt routines.  See the window overflow handler
 *			for details.
 *	5) r16:		Where the 2nd PC is stored when a trap occurs.
 *	6) r17-r25:	Temporaries.  r22-r25 can be changed by the macros
 *			in machAsmDefs.h.
 *	7) r26-r31:	Output registers.  Generally unused in here except
 *			by the instruction parsing routines and to pass args
 *			to C routines.
 *
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
 *	    swp - 128 >= min_swp_offset and
 *	    swp + page_size <= max_swp_offset
 *
 * The 128 bytes of extra space is required on the bottom in order
 * to handle the case when we are trying to allocate more memory after a
 * window underflow and we need space to save a window in case of a
 * window overflow fault.  The page_size worth of data at the top is
 * there in case we need to save windows when we are executing in the kernel
 * on behalf of a user process.  Having a whole page guarantees us that we
 * can make 32 calls before we run out saved window space.
 *
 * If a user's SWP is found to be bogus then the kernel switches over to
 * the kernel's saved window stack and the user process is killed.  If
 * the SWP is bogus when an interrupt occurs, then the user process is not
 * killed until after the interrupt is handled.  All memory between 
 * min_swp_offset and max_swp_offset is wired down.
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
 * are user mode.  The only way to get to kernel mode is to do a return from
 * trap and then immediately do a compare trap to get back in.  Unfortunately
 * the cmp-tra
	 * Need to allocate more memory.  This has to be done in a really
	 * gross way.  I want to switch to kernel mode in order to allocate
	 * memory but I can't switch to kernel mode because I am in user
	 * mode.  Thus the only way to get to kernel mode is to do a return
	 * from trap and then make the instruction that I return to do a
	 * cmp_trap to get back into the kernel.   Of course the cmp_trap
	 * will trash r10 and r16 in the current window so I need to save
	 * these registers at other locations in the current window so they
	 * don't get trashed.   Now the act of returning from the trap 
	 * means that interrupts are enabled which means that an interrupt
	 * can come in.  Therefore I need to save r10 and r16 in a place
	 * where they won't get trashed if an interrupt comes in.  The
	 * place to put them is in r14 and r15 which are registers that
	 * aren't used for anything in here except for parsing 
	 * instructions which won't happen.
	 */
 */

/*
 * Temporary local registers.  Note that the macros in machAsmDefs.h are allowed
 * to use registers rt6 through rt9.
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
 * Temporary global registers.  Note that the only one that we can use is r9
 * because r1 through r8 are required by the Lisp system.  If these need to
 * be used then they have to be saved first.
 */
#define	GLOB_TMP1	r9

/*
 * Trap table.  The hardware jumps to virtual 0x1000 when any type of trap
 * occurs.
 */
	.org 0x1000
	jump PowerUp$w		# Reset - Jump to powerup sequencer.
	Nop

	.org 0x1010
	jump Error$w		# Error
	Nop

	.org 0x1020
	jump WinOvFlow$w	# Window overflow
	Nop

	.org 0x1030
	jump WinUnFlow$w	# Window underflow
	Nop

	.org 0x1040
	jump FaultIntr$w	# Fault or interrupt
	Nop

	.org 0x1050
	jump FPUExcept$w	# FPU Exception
	Nop

	.org 0x1060
	jump Illegal$w		# Illegal op, kernel mode access violation
	Nop

	.org 0x1070
	jump Fixnum$w		# Fixnum, fixnum_or_char, generation
	Nop

	.org 0x1080
	jump Overflow$w		# Integer overflow
	Nop

	.org 0x1090
	jump CmpTrap$w		# Compare trap instruction
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
 * of what _machCurStatePtr points to we can't do it by the instruction
 *
 * 	ld_32	rt1, r0, $_machCurStatePtr
 *
 * because the address of _machCurStatePtr will be longer than 13 bits.  
 * However we can do
 *
 *	ld_32	rt1, r0, $_curStatePtr
 *	ld_32	rt1, rt1, $0
 *
 * We could also do this by doing
 *
 *	LD_CONSTANT(rt1,_machCurStatePtr)
 *	ld_32	rt1, rt1, $0
 */
runningProcesses: 	.long _proc_RunningProcesses
curStatePtr: 		.long _machCurStatePtr
statePtrOffset:		.long _machStatePtrOffset
vmFault_GotDataAddrPtr	.long VMFault_GotDataAddr

/*
 * The instruction to execute on return from a signal handler.  Is here
 * because the value is loaded into a register and it is quicker if
 * is done as an immediate.
 */
SigReturnAddr:
	cmp_trap	always, r0, r0, $MACH_SIG_RETURN_TRAP	
	Nop

/*
 * Address of a Nop to execute.  Here for same reason as SigReturnAddr.
 */
ExecNop:
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
 * WinOvFlow --
 *
 *	Window overflow fault handler.  If in user mode then the swp is
 *	validated
 * first.  If it is invalid then the trapping process is killed.  Also if
 * in user mode more memory is wired down if the trapping process doesn't
 * have at least one page worth of window overflow stack wired down.
 */
WinOvFlow:
	rd_kpsw		rt1
	and		rt1, rt1, $MACH_KPSW_CUR_MODE
	cmp_br_delayed	eq, rt1, r0, winOvFlow_SaveWindow
	Nop
	VERIFY_SWP(0)
winOvFlow_SaveWindow:
	rd_special	rt2, cwp
	wr_special	cwp, rt2, $4		# Move forward one window.
	rd_special	GLOB_TMP1, swp
	st_40           r26, GLOB_TMP1, $0
	st_40           r11, GLOB_TMP1, $8
        st_40           r12, GLOB_TMP1, $16
        st_40           r13, GLOB_TMP1, $24
        st_40           r14, GLOB_TMP1, $32
        st_40           r15, GLOB_TMP1, $40
        st_40           r16, GLOB_TMP1, $48
        st_40           r17, GLOB_TMP1, $56
        st_40           r18, GLOB_TMP1, $64
        st_40           r19, GLOB_TMP1, $72
        st_40           r20, GLOB_TMP1, $80
        st_40           r21, GLOB_TMP1, $88
        st_40           r22, GLOB_TMP1, $96
        st_40           r23, GLOB_TMP1, $104
        st_40           r24, GLOB_TMP1, $112
        st_40           r25, GLOB_TMP1, $120
	rd_special	GLOB_TMP1, cwp
	wr_special	cwp, GLOB_TMP1, $-4		# Move back one window.
	rd_special	GLOB_TMP1, swp
	wr_special	swp, GLOB_TMP1, $128		# swp = swp + 128

/*
 * rt1 == MACH_KPSW_CUR_MODE if we are in user mode and 0 otherwise.  If
 * we are in kernel mode then we can just return.  However, if we are in
 * user mode then we have to make sure that we have at least one page
 * of overflow stack available.
 */

	cmp_br_delayed	eq, rt1, $0, winOvFlow_Return
	Nop
	ld_32		rt1, r0, $curStatePtr
	Nop
	ld_32		rt1, rt1, $0
	Nop
	ld_32		rt2, rt1, $MACH_MAX_SWP_OFFSET
	add_nt		rt3, GLOB_TMP1, $MACH_PAGE_SIZE
	cmp_br_delayed	le, rt2, rt3, winOvFlow_Return
	Nop
	/*
	 * Need to allocate more memory.  This has to be done in a really
	 * gross way.  I want to switch to kernel mode in order to allocate
	 * memory but I can't switch to kernel mode because I am in user
	 * mode.  Thus the only way to get to kernel mode is to do a return
	 * from trap and then make the instruction that I return to do a
	 * cmp_trap to get back into the kernel.   Of course the cmp_trap
	 * will trash r10 and r16 in the current window so I need to save
	 * these registers at other locations in the current window so they
	 * don't get trashed.   Now the act of returning from the trap 
	 * means that interrupts are enabled which means that an interrupt
	 * can come in.  Therefore I need to save r10 and r16 in a place
	 * where they won't get trashed if an interrupt comes in.  The
	 * place to put them is in r14 and r15 which are registers that
	 * aren't used for anything in here except for parsing 
	 * instructions which won't happen.
	 */
	add_nt		r14, r10, $0	# Save the first and 2nd PCs
	add_nt		r15, r16, $0
	add_nt		r10, r0, $ExecNop
	return_trap	r10, $0
	cmp_trap	always, r0, r0, $MACH_GET_WIN_MEM_TRAP
	Nop

winOvFlow_Return:
	add_nt		GLOB_TMP1, r16, r0
	return_trap	r10, $0
	jump_reg	GLOB_TMP1, $0

/*
 * Window underflow fault handler.  If in user mode then the swp is validated
 * first.  If it is invalid then the trapping process is killed.  Also if
 * in user mode more memory is wired down if the trapping process doesn't
 * have the memory behind the new swp wired down.
 */
WinUnFlow:
	rd_kpsw		rt1
	and		rt1, rt1, $MACH_KPSW_CUR_MODE
	cmp_br_delayed	eq, rt1, r0, winUnFlow_RestoreWindow
	Nop
	VERIFY_SWP(128)
winUnFlow_RestoreWindow:
        rd_special	GLOB_TMP1, swp
        rd_special      rt1, cwp
	wr_special      cwp, rt1,  $-8          # move back two windows
        wr_special      swp, GLOB_TMP1, $-128
        ld_40           r10, GLOB_TMP1,   $0
        ld_40           r11, GLOB_TMP1,   $8
        ld_40           r12, GLOB_TMP1,  $16
        ld_40           r13, GLOB_TMP1,  $24
        ld_40           r14, GLOB_TMP1,  $32
        ld_40           r15, GLOB_TMP1,  $40
        ld_40           r16, GLOB_TMP1,  $48
        ld_40           r17, GLOB_TMP1,  $56
        ld_40           r18, GLOB_TMP1,  $64
        ld_40           r19, GLOB_TMP1,  $72
        ld_40           r20, GLOB_TMP1,  $80
        ld_40           r21, GLOB_TMP1,  $88
        ld_40           r22, GLOB_TMP1,  $96
        ld_40           r23, GLOB_TMP1, $104
        ld_40           r24, GLOB_TMP1, $112
        ld_40           r25, GLOB_TMP1, $120
        rd_special      GLOB_TMP1, cwp
        wr_special      cwp,  GLOB_TMP1, $8	# move back ahead two windows
        Nop

/*
 * rt1 == MACH_KPSW_CUR_MODE if we are in user mode and 0 otherwise.  If
 * we are in kernel mode then we can just return.  However, if we are in
 * user mode then we have to see if we should free up a window overflow
 * stack page.
 */
	cmp_br_delayed	eq, rt1, $0, winUnFlow_Return
	Nop
	ld_32		rt1, r0, $curStatePtr
	Nop
	ld_32		rt1, rt1, $0
	rd_special	GLOB_TMP1, swp
	ld_32		rt2, rt1, $MACH_MIN_SWP_OFFSET
	cmp_br_delayed	gt, GLOB_TMP1, rt2, winUnFlow_Return
	Nop
	/*
	 * Need to get more memory for the window overflow.  See the
	 * window overflow code for a comment on how this works.
	 */
	add_nt		r14, r10, $0	# Save the first and 2nd PCs
	add_nt		r15, r16, $0
	add_nt		r10, r0, $ExecNop
	return_trap	r10, $0
	cmp_trap	always, r0, r0, $MACH_GET_WIN_MEM_TRAP

winUnFlow_Return:
	add_nt		GLOB_TMP1, r16, r0
	return_trap	r10, $0
	jump_reg	GLOB_TMP1, $0

/*
 * FPU Exception handler.  Currently we take either a user or kernel error
 * whichever is appropriate.
 */
FPUExcept:
	rd_kpsw		rt1
	and		rt1, rt1, $MACH_KPSW_CUR_MODE
	cmp_br_delayed	eq, rt1, $0, fpuExcept_KernError
	Nop
	USER_ERROR(MACH_USER_FPU_EXCEPT)
fpuExcept_KernError:
	CALL_DEBUGGER(MACH_KERN_FPU_EXCEPT)

/*
 * Illegal instruction handler.  Currently we take either a user or kernel error
 * whichever is appropriate.
 */
Illegal:
	rd_kpsw		rt1
	and		rt1, rt1, $MACH_KPSW_CUR_MODE
	cmp_br_delayed	eq, rt1, $0, illegal_KernError
	Nop
	USER_ERROR(MACH_USER_ILLEGAL)
illegal_KernError:
	CALL_DEBUGGER(MACH_KERN_ILLEGAL)

/*
 * Fixnum Exception handler.  Currently we take either a user or kernel error
 * whichever is appropriate.
 */
Fixnum:
	rd_kpsw		rt1
	and		rt1, rt1, $MACH_KPSW_CUR_MODE
	cmp_br_delayed	eq, rt1, $0, fixnum_KernError
	Nop
	USER_ERROR(MACH_USER_FIXNUM)
fixnum_KernError:
	CALL_DEBUGGER(MACH_KERN_FIXNUM)

/*
 * Overflow fault handler.  Currently we take either a user or kernel error
 * whichever is appropriate.
 */
Overflow:
	rd_kpsw		rt1
	and		rt1, rt1, $MACH_KPSW_CUR_MODE
	cmp_br_delayed	eq, rt1, $0, overflow_KernError
	Nop
	USER_ERROR(MACH_USER_OVERFLOW)
overflow_KernError:
	CALL_DEBUGGER(MACH_KERN_OVERFLOW)

PowerUp: 			# Jump to power up sequencer
Error:	
	CallDebugger(MACH_ERROR)

FaultIntr:
	/*
	 * Read the fault/error status register.
	 */
	ld_external	rt1, r0, $MACH_FE_STATUS_0|MACH_RD_REG
	ld_external	rt2, r0, $MACH_FE_STATUS_1|MACH_RD_REG
	wr_insert	$1
	insert		rt1, rt1, rt2
	ld_external	rt2, r0, $MACH_FE_STATUS_2|MACH_RD_REG
	wr_insert	$2
	insert		rt1, rt1, rt2
	ld_external	rt2, r0, $MACH_FE_STATUS_3|MACH_RD_REG
	wr_insert	$3
	insert		rt1, rt1, rt2

	/*
	 * If no bits are set then it must be an interrupt.
	 */
	cmp_br_delayed	eq, rt1, r0, Interrupt
	Nop
	/*
	 * If any of the bits FEStatus<19:16> are set then is one of the
	 * four VM faults.
	 */
	extract		rt2, rt1, $2
	and		rt2, rt2, $0xf
	cmp_br_delayed	ne, rt2, 0, VMFault
	Nop
	/*
	 * We can't handle any of the rest of the faults.
	 */
	rd_kpsw		rt1
	and		rt1, rt1, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, rt1, $0, faultIntr_KernError
	Nop
	VERIFY_SWP(0)
	SWITCH_TO_KERNEL_SPILL_STACK()
faultIntr_KernError:
	CALL_DEBUGGER(MACH_BAD_FAULT)

Interrupt:
	/*
	 * Save the insert register in temp reg 4.
	 */
	rd_insert	rt4
	/*
	 * Get the interrupt status register as the first parameter to
	 * the interrupt handling routine.
	 */
	ld_external	r27, r0, $MACH_INTR_STATUS_0|MACH_RD_REG
	ld_external	rt2, r0, $MACH_INTR_STATUS_1|MACH_RD_REG
	wr_insert	$1
	insert		r27, r27, rt2
	ld_external	rt2, r0, $MACH_INTR_STATUS_2|MACH_RD_REG
	wr_insert	$2
	insert		r27, r27, rt2
	ld_external	rt2, r0, $MACH_INTR_STATUS_3|MACH_RD_REG
	wr_insert	$3
	insert		r27, r27, rt2

	/*
	 * Disable interrupts but enable all other traps.
	 */
	read_kpsw	rt5
	and		rt2, rt5, $((~MACH_INTR_TRAP_ENA)&0x3fff)
	or		rt2, rt2, $MACH_ALL_TRAPS_ENA
	wr_kpsw		rt2
	and		rt2, rt5, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, rt2, $0, interrupt_KernMode
	Nop
	/*
	 * We took the interrupt from user mode.  Verify that the swp
	 * is OK which means:
	 *
	 *	swp >= min_swp_offset and
	 *	swp + page_size - 128 <= max_swp_offset
	 *
	 * This is 128 bytes different then the standard check (VERIFY_SWP)
	 * because we may interrupt a process after it has taken a window
	 * overflow or underflow fault but before it has had a chance to
	 * allocate more memory for the window stack.
	 */
	ld_32		rt1, r0, $curStatePtr
	rd_special	rt2, swp
	ld_32		rt3, rt1, $MACH_MIN_SWP_OFFSET
	Nop
	cmp_br_delayed	gt, rt3, rt2, interrupt_BadSWP
	Nop
	ld_32		rt3, rt1, $MACH_MAX_SWP_OFFSET
	add_nt		rt2, rt2, $(MACH_PAGE_SIZE - 128)
	cmp_br_delayed	ge, rt3, rt2, interrupt_GoodSWP
	Nop
interrupt_BadSWP:
	/*
	 * We have a bogus user swp.  Switch over to the kernel's saved
	 * window and spill stacks and take the interrupt.  After taking
	 * the interrupt kill the user process.
	 */
	SWITCH_TO_KERNEL_STACKS()
	call		_MachInterrupt$w
	Nop
	read_kpsw	rt1
	or		rt1, rt1, $MACH_KPSW_INTR_TRAP_ENA
	write_kpsw	rt1
	add_nt		r11, r0, $MACH_USER_BAD_SWP
	call		_MachUserError$w
	Nop

interrupt_GoodSWP:
	/*
	 * We have a good user swp.  Switch to the kernel's spill stack, call
	 * the interrupt handler and then go back to the user's stack.
	 */
	ld_32		r4, rt1, $MACH_KERN_STACK_END
	add_nt		rt1, r4, $0
	call		_MachInterrupt$w
	Nop
	cmp_br_delayed	always, interrupt_Return
	add_nt		r4, rt1, $0
interrupt_KernMode:
	/*
	 * We took the interrupt in kernel mode so all that we have to
	 * do is call the interrupt handler.
	 */
	call 		_MachInterrupt$w
	Nop
interrupt_Return:
	/*
	 * Restore saved KPSW and insert register and return from trap.
	 */
	wr_insert	rt4
	wr_kpsw		rt5
	add_nt		GLOB_TMP1, r16, $0
	return_trap	r10, $0
	jump_reg	GLOB_TMP1, $0

/*
 * VMFault --
 *
 *	Handle virtual memory faults.
 */
VMFault:
	/*
	 * Handle a VM fault.  Note that the type of fault was stored in
	 * rt2 right before we were called so we save it right away in rt1
	 * because rt1 won't get corrupted by the parsing of the instruction.
	 */
	add_nt		rt1, rt2, $0
	/*
	 * Enable all other traps.
	 */
	read_kpsw	rt5
	add_nt		r31, rt5, $0		# 5th arg to MachVMFault is the
						#     kpsw
	or		rt2, rt2, $MACH_ALL_TRAPS_ENA
	wr_kpsw		rt2
	and		rt2, rt5, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, rt2, $0, vmFault_GetDataAddr
	Nop
	VERIFY_SWP(0)
vmFault_GetDataAddr:
	FETCH_CUR_INSTRUCTION(rt2)
	extract		rt2, rt2, $3		# Opcode <31:25> -> <07:01>
	srl		rt2, rt2, $1		# Opcode <07:01> -> <06:00>
	and		rt2, rt2, $0xf0		# Get upper 4 bits.
	/*
	 * All instructions besides loads, stores and test-and-set instructions
	 * have opcodes greater than 0x20.
	 * 
	cmp_br_delayed	gt, rt2, $0x20, vmFault_NoData
	Nop
	/*
	 * Get the data address.
	 */
	ld_32		r11, r0, $vmFault_GotDataAddrPtr
	Nop
	jump		ParseInstruction$w
	Nop
vmFault_GotDataAddr:
	/*
	 * We now have the data address in r14.
	 */
	add_nt		r29, r0, $1		# 3rd arg is TRUE to indicate
						#   that there is a data addr
	add_nt		r30, r14, $0		# 4th arg is the data addr.
	cmp_br_delayed	always, vmFault_CallHandler
	Nop
vmFault_NoData:
	add_nt		r29, r0, $0		# 3rd arg is FALSE
						#   (no data addr)
	cmp_br_delayed	always, vmFault_CallHandler
	Nop
vmFault_CallHandler:
	add_nt		r27, rt1, $0		# 1st arg is fault type.
	add_nt		r28, r10, $0		# 2nd arg is the faulting PC.
	and		rt2, rt5, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, rt2, $0, 1f
	Nop
	SAVE_STATE_AND_SWITCH_STACK()
	call		_MachVMFault$w
	Nop
	RESTORE_USER_STATE()
	jump		vmFault_ReturnFromTrap$w
	Nop
vmFault_KernMode:
	/*
	 * Kernel process so just call the routine.
	 */
	call		_MachVMFault$w
	Nop
vmFault_ReturnFromTrap:
	/* 
	 * Clear fault bit and restore the kpsw.
	 */
	st_external	rt1, r0, $MACH_FE_STATUS_2|MACH_WR_REG	
	wr_kpsw		rt5
	jump		ReturnTrap$w

/*
 * Compare trap handler -
 *
 *	Handle a cmp_trap trap.  This involves determining the trap type
 *	and vectoring to the right spot to handle trap.
 */
CmpTrap:
	FETCH_CUR_INSTRUCTION(r17)
        and             rt1, r17, $0x1ff        # Get trap number
	cmp_br_delayed	gt, rt1, $MACH_MAX_TRAP_TYPE, cmpTrap_BadTrapType
	Nop
	sll		rt1, rt1, $3		# Multiple by 8 to get offset
	rd_special	rt2, pc
	add_nt		rt2, rt1, $16
	jump_reg	rt2, $0
	Nop
	jump		SysCallTrap$w	
        Nop
	jump		UserErrorTrap$w
	Nop
	jump		SigReturnTrapa$w
	Nop
cmpTrap_BadTrapType:
	/*
	 * A trap type greater than the maximum value was specified.  If
	 * in kernel mode call the debugger.  Otherwise call
	 * the user error routine.
	 */
	rd_kpsw		rt1
	and		rt2, rt1, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, rt2, $0, cmpTrap_KernError
	Nop
	VERIFY_SWP(0)
	SAVE_STATE_AND_SWITCH_STACK()
	or		rt2, rt1, $MACH_KPSW_ALL_TRAP_ENA
	write_kpsw	rt2
	add_nt		r11, r0, $MACH_BAD_TRAP_TYPE
	call		_MachUserError$w
	Nop
	RESTORE_STATE()
	jump		ReturnTrap$w
	Nop

cmpTrap_KernError:
	CALL_DEBUGGER(MACH_BAD_TRAP_TYPE)

/*
 * SysCallTrap -
 *
 *	Handle a system call trap.
 */
SysCallTrap:

/*
 * UserErrorTrap -
 *
 *	Handle a user error trap.  Before we were called the old r10 (firstPC
 *	at fault time) and r16 (secondPC) were saved in r14 and r15 
 *	respectively and the error type was stored in r13.
 */
UserErrorTrap:
	VERIFY_SWP(0)
	SAVE_STATE_AND_SWITCH_STACK()
	add_nt		r10, r14, $0
	add_nt		r16, r15, $0
	add_nt		r11, r13, $0
	jump		_MachUserError$w
	Nop
	RESTORE_STATE()
	jump		ReturnTrap$w
	Nop

/*
 * SigReturnTrap -
 *
 *	Return from signal trap.  The previous window contains
 *	the saved info when we called the signal handler.
 */
SigReturnTrap:
	VERIFY_SWP(0)
	/*
	 * Get back to the previous window.
	 */
	rd_special	rt1, pc
	return		rt1, $12
	Nop
	/*
	 * We are now in the previous window.  The user stack pointer was saved
	 * in rt1 and the old hold mask in rt2.  Also the first and second
	 * PCs were saved in r10 and r16.  Restore the stack pointer and
	 * then call the signal return handler with the old hold mask as
	 * an argument.  The signal return handler will check for other signals
	 * pending and return one of the normal ReturnTrap codes.
	 */
	add_nt		r4, rt1, $0
	add_nt		r27, rt2, $0
	SAVE_STATE_AND_SWITCH_STACK()
	jump		_MachSigReturn()
	Nop
	RESTORE_STATE()
	jump		ReturnTrap$w
	Nop

/*
 * ReturnTrap -
 *
 *	Return from a trap handler.  Assume that the type of return to
 *	do has been stored in r11.
 */
ReturnTrap:
	/*
	 * Check the return code from the fault handler.
	 */
	cmp_br_delayed	eq, r11, $MACH_NORM_RETURN, returnTrap_NormReturn
	Nop
	cmp_br_delayed	eq, r11, $MACH_FAILED_COPY, returnTrap_FailedCopy
	Nop
	cmp_br_delayed	eq, r11, $MACH_CALL_SIG_HANDLER, returnTrap_CallSigHandler
	Nop
	CALL_DEBUGGER_REG(r11)
returnTrap_FailedCopy:
	/*
	 * A copy to/from user space failed.  Go back to the previous window
	 * and then return an error to the caller.
	 */
	rd_special	rt1, pc
	return		rt1, $12
	Nop
	/*
	 * The error to return is 0x20000 for SYS_ARG_NOACCESS.  Since we only
	 * have 14 bits of immediate we have to insert a 2 into the 3rd byte.
	 */
	add_nt		r27, r0, $0
	wr_insert	$2
	insert		r27, r0, $2
	or		rt5, rt5, $MACH_ALL_TRAPS_ENA
	and		rt5, rt5 $
	wr_kpsw		rt5
	return		r10, $0
	Nop
returnTrap_CallSigHandler:
	/*
	 * Need to call a users signal handler.  First save the current
	 * stack pointer in the current window.  The PCs are already saved
	 * in r10 and r16.
	 */
	ld_32		r31, r0, $curStatePtr
	Nop
	ld_32		r31, r31, $0
	Nop
	ld_32		rt2, r31, $MACH_OLD_HOLD_MASK_OFFSET
	add_nt		rt1, r4, $0
	/*
	 * Go to the next window which is where the signal handler will
	 * execute in.
	 */
	call		1f
	Nop
1f:
	/*
	 * Load in the PC, stack pointer and the arguments to the signal
	 * handler.
	 */
	ld_32		rt1, r15, $MACH_NEW_FIRST_PC_OFFSET
	ld_32		r4, r15, $MACH_NEW_USER_SP_OFFSET
	add_nt		r10, r0, $SigReturnAddr
	ld_32		r11, r15, $MACH_SIG_NUM_OFFSET
	ld_32		r12, r15, $MACH_SIG_CODE_OFFSET
	ld_32		r13, r15, $MACH_OLD_HOLD_MASK_OFFSET
	/*
	 * Call the signal handler and switch to user mode.
	 */
	rd_kpsw		GLOB_TMP1
	or		GLOB_TMP1, GLOB_TMP1, $MACH_ALL_TRAPS_ENA|MACH_KPSW_CUR_MODE
	jump_reg	rt2, $0
	wr_kpsw		GLOB_TMP1

returnTrap_NormReturn:
	/*
	 * Do a return from trap.  If the 2nd PC in r16 is zero then we
	 * don't do the jump to the 2nd PC because there is none.
	 */
	cmp_br_delayed	eq, r16, $0, returnTrap_No2ndPC
	add_nt		GLOB_TMP1, r16, $0
	return_trap	r10, $0
	jump_reg	GLOB_TMP1, $0
returnTrap_No2ndPC:
	return_trap	r10, $0
	Nop

/*
 * ParseInstruction --
 *
 *	Relevant instructions: LD_40*, LD_32*, ST_40, ST_32, TEST_AND_SET.
 *	We want to get the address of the operand that accesses memory.
 *	The address of the instruction is passed in r10 and the address to 
 *	jump to when done is passed in r11.
 *
 *	RET_ADDR:	r11 -- Address to return to when done.
 *	SRC1_REG:	r14 -- src1 register
 *	SRC1_VAL:	r14 -- Value of src1 register.
 *	PREV_SRC1_REG:	r30 -- src1 register in previous window.
 *	SRC2_REG:	r15 -- src2 register or immediate.
 *	SRC2_VAL:	r15 -- Value of src2 register or immediate.
 *	PREV_SRC2_REG:	r31 -- src2 register in previous window.
 *	TRAP_INST:	r17 -- Trapping instruction.
 *	OPCODE:		r18 -- Opcode.
 *	PARSE_TEMP1:	r19 -- One temporary to use.
 *	PARSE_TEMP2:	r20 -- 2nd temporary to use.
 *	SAVED_R10:	r21 -- Place to save r10.
 *	SAVED_R14:	r22 -- Place to save r14.
 *	SAVED_R15:	r23 -- Place to save r15.
 */

#define	RET_ADDR		r11
#define	SRC1_REG		r14
#define	SRC1_VAL		r14
#define	PREV_SRC1_REG		r30
#define	SRC2_REG		r15
#define	SRC2_VAL		r15
#define	PREV_SRC2_REG		r31
#define	TRAP_INST		rt2
#define	OPCODE			rt3
#define	PARSE_TEMP1		rt4
#define	PARSE_TEMP2		rt5
#define	SAVED_R10		rt6
#define	SAVED_R14		rt7
#define	SAVED_R15		rt8

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
	call		parse1up$w		# Back to trap handler window.
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
	jump	        parse41$w		 # Value is returned in r31

parse41:
	add_nt		r30, r31, $0		# Move to src1's register.
	call 	       	parse5$w		# Get back to trap window. 
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
	call		pars2up$w
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
	jump 		parse7$w		 #   and put value in r31
parse7:	
	call		parse_end$w
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
	add_nt		PARSE_TEMP1, PARSE_TEMP1, $0x3ff8 # Gen up mask to make
	and		SRC1_VAL, SRC1_VAL, PARSE_TEMP1   #   to make 8-bit
							  #   aligned if 40-bit
	add_nt		r10, SAVED_R10, $0		# Restore pre-parse r10
	jump_reg	r21, r0			# Go back to caller
	Nop

