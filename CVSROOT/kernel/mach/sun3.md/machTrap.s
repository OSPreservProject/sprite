|* excVector.s -
|*
|*     Contains the trap handlers.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*

#include "excAsm.h"
#include "asmDefs.h"

.data
.asciz "$Header$ SPRITE (Berkeley)"
.even
.text

|*
|* ----------------------------------------------------------------------------
|*
|* Trap handling --
|*	Handle exceptions.  In all cases except kernel calls, call
|*	the C trap handler.  See the kernel call code below.
|*
|* Results:
|*	None.
|*
|* Side effects:
|*	None.
|*
|* ----------------------------------------------------------------------------
|*

	.globl	Exc_Reset
Exc_Reset:
	CallTrapHandler(EXC_RESET)

	.globl	Exc_BusError
Exc_BusError:
	CallTrapHandler(EXC_BUS_ERROR)

	.globl	Exc_AddrError
Exc_AddrError:
	CallTrapHandler(EXC_ADDRESS_ERROR)

	.globl	Exc_IllegalInst
Exc_IllegalInst:
    	CallTrapHandler(EXC_ILLEGAL_INST)

	.globl	Exc_ZeroDiv
Exc_ZeroDiv:
    	CallTrapHandler(EXC_ZERO_DIV)

	.globl	Exc_ChkInst
Exc_ChkInst:
    	CallTrapHandler(EXC_CHK_INST)

	.globl	Exc_Trapv
Exc_Trapv:
    	CallTrapHandler(EXC_TRAPV)

	.globl	Exc_PrivVio
Exc_PrivVio:
    	CallTrapHandler(EXC_PRIV_VIOLATION)

	.globl	Exc_TraceTrap
Exc_TraceTrap:
    	CallTrapHandler(EXC_TRACE_TRAP)

	.globl	Exc_Emu1010
Exc_Emu1010:
    	CallTrapHandler(EXC_EMU1010)

	.globl	Exc_Emu1111
Exc_Emu1111:
    	CallTrapHandler(EXC_EMU1111)

	.globl	Exc_FmtError	
Exc_FmtError:
    	CallTrapHandler(EXC_STACK_FMT_ERROR)

	.globl	Exc_UninitVect
Exc_UninitVect:
    	CallTrapHandler(EXC_UNINIT_VECTOR)

	.globl	Exc_SigRetTrap
Exc_SigRetTrap:
	CallTrapHandler(EXC_SIG_RET_TRAP)

	.globl	Exc_BadTrap
Exc_BadTrap:
    	CallTrapHandler(EXC_BAD_TRAP)

	.globl	Exc_BrkptTrap
	.globl _Exc_BrkptTrap
_Exc_BrkptTrap:
Exc_BrkptTrap:
	CallTrapHandler(EXC_BRKPT_TRAP)

|*
|* ----------------------------------------------------------------------
|*
|* Exc_SyscallTrap --
|*
|*	This is the code entered on system call traps.  The code below
|*	is tuned to get into and out of kernel calls as fast as possible.
|*
|* Results:
|*	Returns a status to the caller in d0.
|*
|* Side effects:
|*	Depends on the kernel call.
|*
|* ----------------------------------------------------------------------
|*

	.globl _excMaxSysCall, _excKcallTableOffset, _excArgOffsets
	.globl _excArgDispatch, _excStateOffset
	.globl _sys_NumCalls, _proc_RunningProcesses
	.globl Exc_SyscallTrap
Exc_SyscallTrap:

	|*
	|* If this is a fork kernel call, save the registers in the PCB.
	|* This is a hack, and should eventually go away by adding another
	|* parameter to fork, which gives the address of an area of
	|* memory containing the process' saved state.
	|*

	tstl	d0
	jne	1$
	movl	_proc_RunningProcesses, a0
	movl	a0@, d1
	addl	_excStateOffset, d1
	movl	d1, a0
	moveml	#0x7fff, a0@
	movc	usp, a1
	movl	a1, a0@(60)
	movl	sp@(2), a0@(64)

	|*
	|* Save registers used here:  two address registers and sp.
	|*

1$:	movl	a2, sp@-
	movl	a3, sp@-
	movl	sp, a3

	|*
	|* Check number of kernel call for validity.
	|*

	cmpl	_excMaxSysCall, d0
	jls	2$
	movl	#20002, d0
	jra	return

	|*
	|* Increment a count of the number of times this kernel call
	|* has been invoked.
	|*

2$:
	asll	#2, d0			| Used to index into tables.
	movl	#_sys_NumCalls, a0
	addql	#1, a0@(0, d0:w)

	|*
	|* Copy the arguments from user space and push them onto the
	|* stack.  Note:  this code interacts heavily with the C code
	|* in Exc_SyscallInit().  If you change one, be sure to change
	|* the other.
	|*

	movc	usp, d1
	movl	#_excArgOffsets, a0
	addl	a0@(0, d0:w), d1
	movl	d1, a0
	movl	#_excArgDispatch, a1
	movl	a1@(0, d0:w), a1
	jmp	a1@

	.globl _ExcFetchArgs
_ExcFetchArgs:
	movl	a0@-, sp@-		| 16 argument words.
	movl	a0@-, sp@-
	movl	a0@-, sp@-
	movl	a0@-, sp@-
	movl	a0@-, sp@-		| 12 argument words.
	movl	a0@-, sp@-
	movl	a0@-, sp@-
	movl	a0@-, sp@-
	movl	a0@-, sp@-		| 8 argument words.
	movl	a0@-, sp@-
	movl	a0@-, sp@-
	movl	a0@-, sp@-
	movl	a0@-, sp@-		| 4 argument words.
	movl	a0@-, sp@-
	movl	a0@-, sp@-
	movl	a0@-, sp@-

	.globl _ExcFetchArgsEnd
_ExcFetchArgsEnd:			| Marks last place where PC could be
					| when a page fault occurs while
					| fetching arguments.  Needed to
					| distinguish a page fault during
					| arg fetch (which is OK) from other
					| page faults in the kernel, which are
					| fatal errors.

	|*
	|* Find the location in the current process's control block
	|* of the trapFlags and kcallTable fields.  Then lookup the
	|* address of the kernel-call handling routine and invoke it.
	|*

1$:
	movl	_proc_RunningProcesses, a0
	movl	a0@, d1			| d1 now has PCB address.
	addl	_excKcallTableOffset, d1
	movl	d1, a2			| a2 now has address of kcallTable
					| field in PCB.
	movl	a2@, a0			| a0 points to 0th entry in table.
	movl	a0@(0, d0:w), a1
	jsr	a1@			| Dispatches to the top-level kernel
					| call procedure.

	|*
	|* Disable interrupts and see if any special processing must
	|* be done on the process.
	|*

	movl	a3, sp			| Pop kcall args off stack.
	movw	#0x2700, sr		| Disable interrupts.
	tstl	a2@(4)
	jeq	return

	|*
	|* Something's up with the process (context switch, maybe, or
	|* single-step mode?).  Restore the stack to what it was at
	|* the beginning of the kernel call, then go through a slow
	|* trap-processing procedure to take special action.
	|*

	clrl	a2@(4)
	movw	#0x2000, sr
	movl	sp@+, a3
	movl	sp@+, a2
	CallTrapHandler(EXC_SYSCALL_TRAP)

return:
	movl	sp@+, a3
	movl	sp@+, a2
	rte

/*
 *-------------------------------------------------------------------------
 *
 * ExcReturnFromTrap --
 *
 *	Routine to return from a trap handler.  Called by CallTrapHandler
 *	macro after have returned from Exc_Trap.  The proper action is
 *	taken depending on the error code and then an rte to user space
 *	is performed.
 *
 *-------------------------------------------------------------------------
 */
	.globl ExcReturnFromTrap
ExcReturnFromTrap:

|*
|* Take proper action depending on the return code.
|*
        cmpl 	#EXC_OK, d0
        beq 	normReturn
	cmpl	#EXC_KERN_ERROR, d0
	beq	kernError
	cmpl	#EXC_USER_ERROR, d0
	beq	userError
	cmpl	#EXC_SIG_RETURN, d0
	beq	sigReturn

callHandler:
|*
|* Are calling a signal handler. d0 and the saved SP both
|* point to where the exception stack used to return to user
|* space is to be put.  We set up a short exception stack, restore
|* registers, make SP point to the short stack and then do an rte.
|*
	movl	sp, a0	 		|* Put the return PC into d1
	addl	#EXC_TRAP_STACK_SIZE, a0|*
	movl	a0@(EXC_PC_OFFSET), d1	|*
	movl	d0, a0 			|* Set up the exception stack.
	clrw	a0@			|*    SR <= 0
	movl	d1, a0@(EXC_PC_OFFSET)	|*    PC <= d1
	clrw	a0@(EXC_VOR_OFFSET) 	|*    VOR <= 0
	addl	#6, sp  		|* Remove type and bus error reg
	movl	sp@+, d0  		|* Restore user stack pointer.
	movc	d0, usp			|*
	moveml	sp@+, #0x7fff 		|* Restore all regs but SP.
	movl	sp@, sp 		|* Saved SP points to new stack.
	rte

sigReturn:
|*
|* Are returning from a signal handler.  A1 points to where the
|* restored exception stack is to be stored at, A0 points to where
|* to copy the stack from and D0 contains the size.  We first restore registers
|* so that we can get to A0, A1 and D0.  Then we copy the new exception
|* stack into the right spot on the stack, call the routine that processes
|* return from signals and then do a normal return with the restored stack.
|*
	RestoreTrapRegs()
	movl	a1, sp 		|* Set SP to new value.
	movl	a0, sp@-	|* Save address of exception stack.
	movl	a1, sp@- 	|* Call byte copy with arguments 
	movl	a0, sp@- 	|*     Byte_Copy(d0, a0, a1)
	movl	d0, sp@-	|*
	jsr	_Byte_Copy	|*
	addl	#12, sp
	jsr	_ExcSigReturn 	|* Call ExcSigReturn(a0, trapStack)
	addql	#4, sp
	jra	ExcReturnFromTrap	|* Do a normal return from trap.

userError:
|*
|* Got an error on a copy in from user space.  Blow away the
|* exception stack and return SYS_ARG_NOACCESS to the function doing the copy.
|* The size of the exception stack has been put into the saved D0.
|*
	RestoreTrapRegs()
	addl	d0, sp
	movl	#0x20000, d0
	rts

kernError:
|*
|* Got a fatal kernel error.
|*
	jsr	_Sys_SyncDisks
	jra 	_Dbg_Trap

normReturn:
|*
|* Normal return from trap (no errors).
|*
	RestoreTrapRegs()
        rte
