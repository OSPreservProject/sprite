|* machTrap.s -
|*
|*     Contains the trap handlers.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*

#include "machConst.h"
#include "machAsmDefs.h"

.data
.asciz "$Header$ SPRITE (Berkeley)"
.even
.text

|*
|* ----------------------------------------------------------------------------
|*
|* Trap handling --
|*
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
	.globl	MachReset
MachReset:
	CallTrapHandler(MACH_RESET)

	.globl	MachBusError
MachBusError:
	CallTrapHandler(MACH_BUS_ERROR)

	.globl	MachAddrError
MachAddrError:
	CallTrapHandler(MACH_ADDRESS_ERROR)

	.globl	MachIllegalInst
MachIllegalInst:
	CallTrapHandler(MACH_ILLEGAL_INST)

	.globl	MachZeroDiv
MachZeroDiv:
	CallTrapHandler(MACH_ZERO_DIV)

	.globl	MachChkInst
MachChkInst:
	CallTrapHandler(MACH_CHK_INST)

	.globl	MachTrapv
MachTrapv:
	CallTrapHandler(MACH_TRAPV)

	.globl	MachPrivVio
MachPrivVio:
	CallTrapHandler(MACH_PRIV_VIOLATION)

	.globl	MachTraceTrap
MachTraceTrap:
	CallTrapHandler(MACH_TRACE_TRAP)

	.globl	MachEmu1010
MachEmu1010:
	CallTrapHandler(MACH_EMU1010)

	.globl	MachEmu1111
MachEmu1111:
	CallTrapHandler(MACH_EMU1111)

	.globl	MachFmtError	
MachFmtError:
	CallTrapHandler(MACH_STACK_FMT_ERROR)

	.globl	MachUninitVect
MachUninitVect:
	CallTrapHandler(MACH_UNINIT_VECTOR)

	.globl	MachSigRetTrap
MachSigRetTrap:
	CallTrapHandler(MACH_SIG_RET_TRAP)

	.globl	MachBadTrap
MachBadTrap:
	CallTrapHandler(MACH_BAD_TRAP)

	.globl	MachBrkptTrap
	.globl _MachBrkptTrap
_MachBrkptTrap:
MachBrkptTrap:
	CallTrapHandler(MACH_BRKPT_TRAP)

.globl MachFpUnorderedCond
MachFpUnorderedCond:
    CallTrapHandler(MACH_FP_UNORDERED_COND)

.globl MachFpInexactResult
MachFpInexactResult:
    CallTrapHandler(MACH_FP_INEXACT_RESULT)

.globl MachFpZeroDiv
MachFpZeroDiv:
    CallTrapHandler(MACH_FP_ZERO_DIV)

.globl MachFpUnderflow
MachFpUnderflow:
    CallTrapHandler(MACH_FP_UNDERFLOW)

.globl MachFpOperandError
MachFpOperandError:
    CallTrapHandler(MACH_FP_OPERAND_ERROR)

.globl MachFpOverflow
MachFpOverflow:
    CallTrapHandler(MACH_FP_OVERFLOW)

.globl MachFpNaN
MachFpNaN:
    CallTrapHandler(MACH_FP_NAN)


|*
|* ----------------------------------------------------------------------
|*
|* MachSyscallTrap --
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

	.globl _machMaxSysCall, _machKcallTableOffset, _machArgOffsets
	.globl _machArgDispatch, _machCurStatePtr
	.globl _sys_NumCalls, _proc_RunningProcesses
	.globl MachSyscallTrap
MachSyscallTrap:

	|*
	|* Always save the user stack pointer because it can be needed
	|* while processing the system call.
	|*
	movl	_machCurStatePtr, a0
	movc	usp, a1
	movl	a1, a0@(MACH_USER_SP_OFFSET)

	|*
	|* If this is a fork kernel call, save the registers in the PCB.
	|* This is a hack, and should eventually go away by adding another
	|* parameter to fork, which gives the address of an area of
	|* memory containing the process' saved state.
	|*

	tstl	d0
	jne	1$
	moveml	#0xffff, a0@(MACH_TRAP_REGS_OFFSET)
	movl	sp, a0@(MACH_EXC_STACK_PTR_OFFSET)
	SaveUserFpuState();

	|*
	|* Save registers used here:  two address registers and sp.
	|*

1$:	movl	a2, sp@-
	movl	a3, sp@-
	movl	sp, a3

	|*
	|* Check number of kernel call for validity.
	|*

	cmpl	_machMaxSysCall, d0
	jls	2$
	movl	#20002, d0
	jra	return

2$:
	|*
	|* Store this kernel call in the last kernel call variable.
	|*
	movl	d0, a0@(MACH_LAST_SYS_CALL_OFFSET)

	|*
	|* Increment a count of the number of times this kernel call
	|* has been invoked.
	|*
	asll	#2, d0			| Used to index into tables.
	movl	#_sys_NumCalls, a0
	addql	#1, a0@(0, d0:w)

	|*
	|* Copy the arguments from user space and push them onto the
	|* stack.  Note:  this code interacts heavily with the C code
	|* in Mach_SyscallInit().  If you change one, be sure to change
	|* the other.
	|*

	movc	usp, d1
	movl	#_machArgOffsets, a0
	addl	a0@(0, d0:w), d1
	movl	d1, a0
	movl	#_machArgDispatch, a1
	movl	a1@(0, d0:w), a1
	jmp	a1@

	.globl _MachFetchArgs
_MachFetchArgs:
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

	.globl _MachFetchArgsEnd
_MachFetchArgsEnd:			| Marks last place where PC could be
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
	addl	_machKcallTableOffset, d1
	movl	d1, a2			| a2 now has address of kcallTable
					| field in PCB.
	movl	a2@, a0			| a0 points to 0th entry in table.
	movl	a0@(0, d0:w), a1
	jsr	a1@			| Dispatches to the top-level kernel
					| call procedure.

	|*
	|* Disable interrupts and see if any special processing must
	|* be done on the process.  Note:  this is checking the
	|* specialHandling field of the process control block, and depends
	|* on the fact that specialHandling follows immediately after the
	|* kcallTable field, whose address was loaded into a2 above.
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
	CallTrapHandler(MACH_SYSCALL_TRAP)

return:
	movl	sp@+, a3
	movl	sp@+, a2
	rte

/*
 *-------------------------------------------------------------------------
 *
 * MachReturnFromUserTrap --
 *
 *	Routine to return from a trap handler.  Called by CallTrapHandler
 *	macro after have returned from MachTrap.  The proper action is
 *	taken depending on the error code and then an rte to user space
 *	is performed.
 *
 *-------------------------------------------------------------------------
 */
	.globl MachReturnFromUserTrap
MachReturnFromUserTrap:

|*
|* Take proper action depending on the return code.
|*
        cmpl 	#MACH_OK, d0
        beq 	normReturn
	cmpl	#MACH_KERN_ERROR, d0
	beq	kernError
	cmpl	#MACH_SIG_RETURN, d0
	beq	sigReturn
|*
|* Bogus return code so trap to debugger.
|*
	jra 	_Dbg_Trap

sigReturn:
|*
|* Are returning from a signal handler.  First get pointer to
|* mach state structure.
|*
	movl	_machCurStatePtr, a0
|*
|* The saved stack pointer points to where the exception stack is to
|* be restored at.
|*
	movl	a0@(MACH_TRAP_REGS_OFFSET + 60), sp
	movl	sp, a0@(MACH_EXC_STACK_PTR_OFFSET)	
|*
|* Call bcopy((Address)excStack, (Address)sp, sizeof(excStack));
|*
	movl	sp, d0
	movl	a0@(MACH_SIG_EXC_STACK_SIZE_OFFSET), sp@-
	movl	d0, sp@-
|*	movl	sp, sp@-
	pea	a0@(MACH_SIG_EXC_STACK_OFFSET)
	jsr	_bcopy
	addl	#12, sp
|*
|* Call the normal return from trap return MachUserReturn(procPtr, &excStack)
|* after enabling interrupts because they were disabled when we were called.
|*
	movw	#0x2000, sr
	movl	_proc_RunningProcesses, a0
	movl	a0@, sp@-
	jsr	_MachUserReturn
|*
|* Do a normal return.
|*
	jra	normReturn

kernError:
|*
|* Got a fatal kernel error.  First sync disks, then restore the registers so
|* that the debugger doesn't have to rely on being able to get registers from
|* the proc table and then move the stack pointer back so that the trap code
|* and bus error register are visible.
|*
	jsr	_Sys_SyncDisks
	RestoreUserFpuState()
	RestoreUserRegs()
	subl	#MACH_TRAP_INFO_SIZE, sp
	jra 	_Dbg_Trap

normReturn:
|*
|* Normal return from trap (no errors).
|*
	RestoreUserFpuState()
	RestoreUserRegs()
        rte


/*
 * ----------------------------------------------------------------------------
 *
 * RestoreKernRegs --
 *
 *      Restore the 4 saved temporary registers from the stack after moving
 *	the stack pointer past the trap code and bus error register.
 *
 * ----------------------------------------------------------------------------
 */
#define RestoreKernRegs() \
	addql	#8, sp; \
	moveml	sp@+, #0x0303


/*
 *-------------------------------------------------------------------------
 *
 * MachReturnFromKernTrap --
 *
 *	Routine to return from a trap handler.  Called by CallTrapHandler
 *	macro after have returned from MachTrap.  The proper action is
 *	taken depending on the error code and then an rte to kern space
 *	is performed.
 *
 *-------------------------------------------------------------------------
 */
	.globl MachReturnFromKernTrap
MachReturnFromKernTrap:

|*
|* Take proper action depending on the return code.
|*
        cmpl 	#MACH_OK, d0
        beq 	kernNormReturn
	cmpl	#MACH_KERN_ERROR, d0
	beq	kernKernError
	cmpl	#MACH_USER_ERROR, d0
	beq	kernUserError

|*
|* Bogus return code so trap to debugger.
|*
	jra 	_Dbg_Trap

kernUserError:
|*
|* Got an error on a copy in from user space.  Blow away the
|* exception stack and return SYS_ARG_NOACCESS to the function doing the copy.
|* We have to compute the exception size of the exception stack from the
|* vector offset register.
|*
	RestoreKernRegs()
	clrl	d0
	movw	sp@(6), d0		| D0 = VOR
	lsrl	#8, d0			| DO >> 12 to get to stack format
	lsrl	#4, d0
	cmpl	#MACH_MC68010_BUS_FAULT, d0
	bne	1$
	addl	#MACH_MC68010_BUS_FAULT_SIZE, sp
	bra	4$
1$:	cmpl	#MACH_SHORT_BUS_FAULT, d0
	bne	2$
	addl	#MACH_SHORT_BUS_FAULT_SIZE, sp
	bra	4$
2$:	cmpl	#MACH_LONG_BUS_FAULT, d0
	bne	3$
	addl	#MACH_LONG_BUS_FAULT_SIZE, sp
	bra	4$
3$:	trap	#15

4$:	movl	#0x20000, d0
	rts

kernKernError:
|*
|* Got a fatal kernel error.  First sync disks, then restore the registers so
|* that the debugger doesn't have to rely on being able to get registers from
|* the proc table and then move the stack pointer back so that the trap code
|* and bus error register are visible.
|*
	jsr	_Sys_SyncDisks
	RestoreKernRegs()
	subl	#MACH_TRAP_INFO_SIZE, sp
	jra 	_Dbg_Trap

kernNormReturn:
|*
|* Normal return from trap (no errors).
|*
	RestoreKernRegs()
        rte

