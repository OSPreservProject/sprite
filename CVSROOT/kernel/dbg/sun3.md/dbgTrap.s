|* dbgTrap.s -
|*
|*     Contains the routine which will initialize things and call the main
|*     debugger routine.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*

	.data
	.asciz "$Header$ SPRITE (Berkeley)"
	.even
	.text

#include "machConst.h"
#include "dbgAsm.h"
#include "machAsmDefs.h"

| ***********************************************************************
|
| _Dbg_Trap --
|
|     Call the main debugger routine.  There is a division of labor between
|     the original trap handler that called us, this routine and the main 
|     debugger routine.  The trap handler has put the stack in the following
|     state:
|
|		------------
|		|          |    Trap type (4 bytes)
|		------------
|		|          |	Bus error register (4 bytes)
|		------------
|		|          |	Status register (2 bytes)
|		------------
|		|          | 	Program counter where trap occured (4 bytes)
|		------------
|		|	   |	Vector offset register (2 bytes)
|		------------
|		|          |	Bus error stuff if a bus error.
|		------------
|
|     The bottom part of the stack (status register, PC, VOR, Bus error
|     stuff) is the exception stack which was created by the 68000 when
|     the trap occured. This routine is responsible for changing the 
|     function codes to user data, changing the context to
|     the kernel context, locking out interrupts, and saving registers.  It
|     then calls the |     main debugger routine.  The main debugger routine
|     is responsible for dealing with the stack.  When this routine is 
|     returned to it is responsible for restoring the context, function codes, 
|     and registers.  The PC and status register are restored automatically 
|     when this routine does its return from exception.
|
|     This debugger code is non reentrant.  Therefore before this routine 
|     will do the things listed above it checks to make sure that the 
|     debugger is not already active.  If it is then this routine will
|     restore registers, set a flag to reenter the debugger as soon as it
|     returns, and return.  If it isn't active it marks itself as active and
|     does the above.
|
| Return value:
|     None.
|
| Side effects:
|     The flag dbgInDebugger is modified to indicate that the debugger is 
|     active, dbgTermReason is modified to indicate why we are in the debugger,
|     and dbgMonPC is cleared if it is set.
|

	.text
	.globl	_Dbg_Trap
_Dbg_Trap:
	movw	#MACH_SR_HIGHPRIO,sr 		| Lock out interrupts

| Check to see if we are already in the debugger.  If we are then we can't
| enter it again so complain, mark an interrupt as pending, 
| and return.

	tstl	_dbgInDebugger			| This flag should not be set.
	beqs	2$				| If it isn't then go ahead

	jsr	_DbgComplain			| If it is set then complain.

| If _dbgMonPC is non-zero then it contains the real PC

	tstl	_dbgMonPC			| Check if monitor PC is set.
	beqs	1$				| If zero don't do anything
						| Move the PC onto the stack
	movl	_dbgMonPC, sp@(MACH_PC_OFFSET + MACH_TRAP_INFO_SIZE)
	clrl	_dbgMonPC			| Clear out the PC for next time
1$:
	movl	#1, _dbgIntPending		| We leave an interrupt pending
						| marker.
	addl	#MACH_TRAP_INFO_SIZE, sp	| Blow off trap type and bus
						|    error register
	rte					| Return to where called from.

2$:
	movl	#1, _dbgInDebugger		| Set the flag to indicate
						| that we are in the debugger.

| If _dbgMonPC is non-zero then it contains the real PC

	tstl	_dbgMonPC
	beqs	3$				| If zero don't do anything

						| Move the PC onto the stack
	movl	_dbgMonPC, sp@(MACH_PC_OFFSET + MACH_TRAP_INFO_SIZE) 
	clrl	_dbgMonPC			| Clear out the PC for next time

						| Also in this case this was
						| an interrupt, not a trap
	movl	#DBG_INTERRUPT_SIG, _dbgTermReason	
	bras	4$
3$:

| Otherwise, the reason is some sort of trap.  We will determine the reason in
| the main debugger routine.

	movl	#DBG_NO_REASON_SIG, _dbgTermReason

4$:
	moveml	#0xffff, sp@-		| Save all of the gprs

| Save the function code registers.

	movc	sfc, d0
	movl	d0, _dbgSfcReg
	movc	dfc, d0
	movl	d0, _dbgDfcReg

| Save user and kernel context registers

        jsr 	_VmMachGetUserContext
	movl	d0, _dbgUserContext
        jsr 	_VmMachGetKernelContext
	movl	d0, _dbgKernelContext

| Call the debugger routine

callDbg:

	subl	#DBG_STACK_HOLE, sp 	| Put a hole in the stack so that 
					| kdbx can play with its concept of 
					| the stack.

	jsr	_Dbg_Main		| Call the debugger

	movl	_dbgSavedSP, sp		| Set the sp pointer to the right
					| value so it can get at the saved regs

| See if we have an interrupt pending.  If so then check for exception type
| of bus error or address error.  If one of these then ignore the interrupt.
| Otherwise give our reason as interrupted and go back in.

	tstl	_dbgIntPending
	beqs	5$
	clrl	_dbgIntPending
	cmpl	#MACH_BUS_ERROR, sp@
	beqs	5$
	cmpl	#MACH_ADDRESS_ERROR, sp@
	beqs	5$
	movl	#DBG_INTERRUPT_SIG, _dbgTermReason
	bras	callDbg

5$:
	moveml	sp@+, #0xffff		| Restore all registers.
	addl	#MACH_TRAP_INFO_SIZE, sp | Blow off bus error reg and
					 |   and trap type.

| Return from the exception

	clrl	_dbgInDebugger		| Clear the flag which indicates
					| that we are in the debugger.

	rte
