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
 * Temporary registers.  Note that the macros in machAsmDefs.h are allowed
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
 * Store addresses of things that need to be loaded into registers.
 */
runningProcesses: 	.long _proc_RunningProcesses
curStatePtr: 		.long _machCurStatePtr
statePtrOffset:		.long _machStatePtrOffset
vmFault_GotDataAddrPtr	.long VMFault_GotDataAddr

/*
 * Jump table to return the operand from an instruction.
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
 * Window overflow fault handler.  If in user mode then the swp is validated
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
	rd_special	r9, swp
	st_40           r26, r9, $0
	st_40           r11, r9, $8
        st_40           r12, r9, $16
        st_40           r13, r9, $24
        st_40           r14, r9, $32
        st_40           r15, r9, $40
        st_40           r16, r9, $48
        st_40           r17, r9, $56
        st_40           r18, r9, $64
        st_40           r19, r9, $72
        st_40           r20, r9, $80
        st_40           r21, r9, $88
        st_40           r22, r9, $96
        st_40           r23, r9, $104
        st_40           r24, r9, $112
        st_40           r25, r9, $120
	rd_special	r9, cwp
	wr_special	cwp, r9, $-4		# Move back one window.
	rd_special	r9, swp
	wr_special	swp, r9, $128		# swp = swp + 128

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
	ld_32		rt2, rt1, $MACH_MAX_SWP_OFFSET
	add_nt		rt3, r9, $MACH_PAGE_SIZE
	cmp_br_delayed	le, rt2, rt3, winOvFlow_Return
	Nop
	jump WinGetPage$w
	add_nt		r9, r9, $128

winOvFlow_Return:
	add_nt		r9, r16, r0
	return_trap	r10, $0
	jump_reg	r9, $0

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
        rd_special	r9, swp
        rd_special      rt1, cwp
	wr_special      cwp, rt1,  $-8          # move back two windows
        wr_special      swp, r9, $-128
        ld_40           r10, r9,   $0
        ld_40           r11, r9,   $8
        ld_40           r12, r9,  $16
        ld_40           r13, r9,  $24
        ld_40           r14, r9,  $32
        ld_40           r15, r9,  $40
        ld_40           r16, r9,  $48
        ld_40           r17, r9,  $56
        ld_40           r18, r9,  $64
        ld_40           r19, r9,  $72
        ld_40           r20, r9,  $80
        ld_40           r21, r9,  $88
        ld_40           r22, r9,  $96
        ld_40           r23, r9, $104
        ld_40           r24, r9, $112
        ld_40           r25, r9, $120
        rd_special      r9, cwp
        wr_special      cwp,  r9, $8          # move back ahead two windows
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
	rd_special	r9, swp
	ld_32		rt2, rt1, $MACH_MIN_SWP_OFFSET
	cmp_br_delayed	gt, r9, rt2, winUnFlow_Return
	Nop
	jump WinGetPage$w
	add_nt		r9, r9, $-128

winUnFlow_Return:
	add_nt		r9, r16, r0
	return_trap	r10, $0
	jump_reg	r9, $0

/*
 * FPU Exception handler.  Currently we kill the user process if in user
 * mode or enter the kernel debugger if in kernel mode.
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
 * Illegal instruction handler.  Currently we kill the user process if in user
 * mode or enter the kernel debugger if in kernel mode.
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
 * Fixnum Exception handler.  Currently we kill the user process if in user
 * mode or enter the kernel debugger if in kernel mode.
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
 * Overflow fault handler.  Currently we kill the user process if in user
 * mode or enter the kernel debugger if in kernel mode.
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

/*
 * Call the C routine to get a new saved window stack page wired down.
 * Enable traps and switch to kernel mode before calling the allocator
 * because we will take a window overflow.  r9 is assumed to contain the
 * new swp.
 */
WinGetPage:
	ld_32		rt1, r0, $curStatePtr		# Switch to the
	add_nt		rt2, r4, $0			#   kernel's stack
	ld_32		r4, rt1, $MACH_KERN_STACK_END	#   after saving the
	Nop						#   user sp.
	MOD_KPSW(MACH_KPSW_ALL_TRAPS_ENA, MACH_KPSW_CUR_MODE)
	add_nt		r27, r9, $0
	call		_MachGetWindowPage$w
	Nop
	SET_KPSW(MACH_KPSW_CUR_MODE)
	add_nt		r4, rt2, $0			# Restore the user sp
	add_nt		r9, r16, r0
	return_trap	r10, $0
	jump_reg	r9, $0

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
	 *	min_swp_offset <= swp <= max_swp_offset + page_size.
	 */
	ld_32		rt1, r0, $curStatePtr
	rd_special	rt2, swp
	ld_32		rt3, rt1, $MACH_MIN_SWP_OFFSET
	Nop
	cmp_br_delayed	gt, rt3, rt2, interrupt_BadSWP
	Nop
	ld_32		rt3, rt1, $MACH_MAX_SWP_OFFSET
	add_nt		rt2, rt2, $MACH_PAGE_SIZE
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
	or		rt1, rt1, $MACH_INTR_TRAP_ENA
	write_kpsw	rt1
	add_nt		r27, r0, $MACH_USER_BAD_SWP
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
	 * Restore saved KPSW and return from trap.
	 */
	wr_kpsw		rt5
	add_nt		r9, r16, $0
	return_trap	r10, $0
	jump_reg	r9, $0

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
	 * Disable faults but enable all other traps.
	 */
	read_kpsw	rt5
	and		rt2, rt5, $((~MACH_FAULT_TRAP_ENA)&0x3fff)
	or		rt2, rt2, $MACH_ALL_TRAPS_ENA
	wr_kpsw		rt2
	and		rt2, rt5, $MACH_KPSW_PREV_MODE
	cmp_br_delayed	eq, rt2, $0, vmFault_GetDataAddr
	Nop
	VERIFY_SWP(0)
vmFault_GetDataAddr:
	ld_32		rt2, r10, $0		# Fetch instruction.
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
	/*
	 * User process so switch to the kernel spill stack.
	 */
	add_nt		rt2, r4, $0
	SWITCH_TO_KERNEL_SPILL_STACK()
	call		MachVMFault$w
	Nop
	jump		vmFault_ReturnFromTrap$w
	add_nt		r4, rt2, $0
vmFault_KernMode:
	/*
	 * Kernel process so just call the routine.
	 */
	call		MachVMFault$w
	Nop
vmFault_ReturnFromTrap:
	/*
	 * Return from the trap handler which involves clearing the fault
	 * bits in the FE status register, restoring the kpsw and 
	 * then returning.
	 */
	st_external	rt1, r0, $MACH_FE_STATUS_2|MACH_WR_REG	
	wr_kpsw		rt5
	add_nt		r9, r16, $0
	return_trap	r10, $0
	jump_reg	r9, $0

CmpTrap:

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

#ifdef BARB
	/*
	 * First, make sure we have the right address.  In barb, the address
	 * may claim to be 0x20000 or so, but we really want the right segment.
	 * if (barb_addr >= 0x20000 && barb_addr < 0x40000000)
	 * barb_addr = barb_addr - 0x20000 + 0x40000000
	 */
	ld_constant(PARSE_TEMP1, 0x20000)
	ld_constant(PARSE_TEMP2, 0x40000000)
	cmp_br_delayed	lt, r10, PARSE_TEMP1, addr_okay_bz
	nop
	cmp_br_delayed  ge, r10, PARSE_TEMP2, addr_okay_bz
	nop
	sub		PARSE_TEMP1, r10, PARSE_TEMP1
	add		PARSE_TEMP1, PARSE_TEMP1, PARSE_TEMP2
	jump		addr_okay$w
	nop
addr_okay_bz:
	add_nt		PARSE_TEMP1, r10, $0
addr_okay:
	ld_32		TRAP_INST, PARSE_TEMP1, $0 # recover trapping instr
#else		
	ld_32		TRAP_INST, r10,   $0	# recover trapping instr
#endif
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

