|* machAsm.s --
|*
|*     Contains misc. assembler routines for the SUN.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*
|* rcs = $Header$ SPRITE (Berkeley)
|*

#include "machConst.h"
#include "machAsmDefs.h"

|*----------------------------------------------------------------------------
|*
|* MachRunUserProc -
|*
|*	void	MachRunUserProc()
|*
|* Results:
|*     	Restore registers and return to user space.  Our caller has set
|*	up our exception stack for us.
|*
|* Side effects:
|*	Registers restored.
|*
|*----------------------------------------------------------------------------

    .text
    .globl	_MachRunUserProc
_MachRunUserProc:
    movl	_machCurStatePtr, a0			| a0 = ptr to user 
							|      state struct
    movl	a0@(MACH_USER_SP_OFFSET), a1		| Restore user stack
    movc	a1, usp					|      pointer
    moveml	a0@(MACH_TRAP_REGS_OFFSET), #0xffff	| Restore all regs
    rte							| Return to user space

|*---------------------------------------------------------------------
|*
|* Mach_GetEtherAddress -
|*
|*	Copies the ethernet address from the id prom into the
|*	argument structure.  The prom is mapped funny, so the
|*	loop increments the pointer into the prom as specified in
|*	the sun-2 architecture manual, section 4.8
|*
|*	Mach_GetEtherAddress(etherAddressPtr)
|*	    EtherAddress *etherAddressPtr;	destination of copy
|*
|* Results:
|*     The argument struct has the prom's ethernet address.
|*
|* Side effects:
|*	None.
|*
|*---------------------------------------------------------------------

    .text
    .globl	_Mach_GetEtherAddress
_Mach_GetEtherAddress:
    movl	sp@(4),a0		| Get pointer to target ethernet address
    movl	#6,d0			| loop counter
    movl	#VMMACH_ETHER_ADDR,a1	| The Prom address of the ethernet addr
etherloop:
    movsb	a1@,d1			| Copy one byte of the ethernet address
    movb	d1,a0@			|   from prom to the target address.
    addql	#1,a0			| bump target pointer
    addl	#VMMACH_IDPROM_INC,a1	| bump prom address, as per sec 4.8
    subl	#1,d0			| decrement loop counter
    bne		etherloop		| loop 6 times

    rts					| Return

|*---------------------------------------------------------------------
|*
|* Mach_ContextSwitch -
|*
|*	Mach_ContextSwitch(fromProcPtr, toProcPtr)
|*
|*	Switch the thread of execution to a new processes.  This routine
|*	is passed a pointer to the process to switch from and a pointer to
|*	the process to switch to.  It goes through the following steps:
|*
|*	1) Change to the new context.
|*	2) Push the status register and the
|*	   user stack pointer onto the stack.
|*	3) Push the source and destination function codes onto the stack.
|*	4) Push a magic number onto the stack to see if it gets trashed.
|*	5) Save all of the registers d0-d7, a0-a7 for the process being
|*	   switched from.
|*	6) Restore general registers and the status register of the process 
|*	   being switched to.
|*	7) Verify the magic number.
|*	8) Return in the new process.
|*	
|*	The kernel stack is changed implicitly when the registers are restored.
|*
|* Results:
|*     None.
|*
|* Side effects:
|*	The kernel stack, all general purpose registers and the status register
|*	are all changed.
|*
|*---------------------------------------------------------------------

    .globl _Mach_ContextSwitch
_Mach_ContextSwitch:
|*
|* Setup up the hardware context register for the destination process.
|* VmMach_SetupContext(toProcPtr) returns the context register value.
|*
    movl	sp@(8), sp@-
    jsr		_VmMach_SetupContext
    addql	#4, sp
#ifdef sun3
    movsb	d0, VMMACH_CONTEXT_OFF
#else 
    movsb	d0,VMMACH_USER_CONTEXT_OFF:w 
    movsb	d0,VMMACH_KERN_CONTEXT_OFF:w
#endif
    movl	sp, a1			| Save the stack pointer value in a1
    movw	sr, sp@-		| Save the current value of the status
					|     register on the stack.
    movw	#MACH_SR_HIGHPRIO, sr	| Lock out interrupts.
    movl	usp, a0  		| Push the user stack pointer onto 
    movl	a0, sp@-		|     the stack.
    movl	#MAGIC, sp@-		| Put the magic number on the stack.

    movl	a1@(4), d0		| d0 = fromProcPtr
    addl	_machStatePtrOffset, d0 
    movl	d0, a0			| a0 = pointer to mach struct
    movl	a0@, a0
					| Save registers for process being
					|     switched from
    moveml	#0xffff, a0@(MACH_SWITCH_REGS_OFFSET)

    movl	a1@(8), d0		| d0 = toProcPtr
    addl	_machStatePtrOffset, d0 
    movl	d0, a0			| a0 = pointer to mach struct
    movl	a0@, a0
					| Restore registers for process being
					|     switched to
    moveml	a0@(MACH_SWITCH_REGS_OFFSET), #0xffff

    movl	#MAGIC, d0		| Check against the magic number
    cmpl	sp@, d0			|
    beq		1$			|
    trap	#15			|

1$:
    addql	#4, sp			| Pop the magic number.
    movl	sp@+, a0		| Restore the user stack pointer.
    movl	a0, usp
    movw	sp@+, sr		| Restore the status register.

|*
|* Get a pointer to the current machine state struct.
|*
    .globl	_proc_RunningProcesses, _machCurStatePtr, _machStatePtrOffset
    movl	_proc_RunningProcesses, a0
    movl	a0@, d0
    addl	_machStatePtrOffset, d0
    movl	d0, a0
    movl	a0@, a0
    movl	a0, _machCurStatePtr
|*
|* Set the end of the kernel stack marker for kdbx.
|*
    .globl	_dbgMaxStackAddr
    movl	a0@(MACH_KERN_STACK_START_OFFSET), d0
    addl	#MACH_KERN_STACK_SIZE, d0
    movl	d0, _dbgMaxStackAddr

    rts

|*---------------------------------------------------------------------
|*
|* Mach_TestAndSet --
|*
|*	int Mach_TestAndSet(intPtr)
|*	    int *intPtr;
|*
|*     	Test and set an operand.
|*
|* Results:
|*     	Returns 0 if *intPtr was zero and 1 if *intPtr was non-zero.  Also
|*	in all cases *intPtr is set to a non-zero value.
|*
|* Side effects:
|*     None.
|*
|*---------------------------------------------------------------------

    .globl _Mach_TestAndSet
_Mach_TestAndSet:
    clrl	d0		| Set the return register to 0.

    movl	sp@(4), a0	| Move the address of the operand to a0.
    tas		a0@		| Test and set the operand.

    beq		1$		| If it wasn't set then just return 0.

    moveq	#1, d0		| Otherwise return 1.

1$: rts

|*---------------------------------------------------------------------
|*
|* Mach_GetMachineType -
|*
|*	Returns the type of machine that is stored in the id prom.
|*
|*	int	Mach_GetMachineType()
|*
|* Results:
|*     The type of machine (1 or 2).
|*
|* Side effects:
|*	None.
|*
|*---------------------------------------------------------------------

    .text
    .globl	_Mach_GetMachineType
_Mach_GetMachineType:
    clrl	d0			| Clear the return register
    movl	#VMMACH_MACH_TYPE_ADDR, a0 | Get the address of the machine type
					|     in a register.
    movsb	a0@,d0			| Store the machine type in the return
					|     register.
    rts


|*
|* ----------------------------------------------------------------------
|*
|* MachMonNmiNop --
|*
|*       The code which is executed when we redirect non-maskable
|*       interrupts on the Sun-2.  When the AMD chip timer #1 interrupts,
|*       it causes a level 7 interrupt which is nonmaskable. The timer
|*       output has to be cleared to clear the interrupt condition. The
|*       timer is setup to repeatedly interrupt every 25ms.
|*
|*	NOTE: This is code taken from locore.s.
|*
|* Results:
|*	None.
|*
|* Side effects:
|*	The AMD chip's timer #1 output is cleared.
|*
|* ----------------------------------------------------------------------
|*

#define	AMD9513_CSR	0xee0002

    .globl _MachMonNmiNop
_MachMonNmiNop:
        moveml  #0xC0C0,sp@-            | Save d0,d1,a0,a1

#ifdef sun2
        movw    #0xFFE1, AMD9513_CSR	| Clear the output of timer #1 
					| on the AMD timer chip to clear
					| the current interrupt.
| 
| Move the upper half of the status register to the leds.
|
        movb    sp@(16),d0		| Move the upper half of the sr to d0
        eorb    #0xFF,d0		| Exclusive or the bits.
        movsb   d0,0xB                  | Move it to the leds.

#endif /* sun2 */
        moveml  sp@+,#0x0303            | restore regs
        rte


|*
|* ----------------------------------------------------------------------
|*
|* Mach_MonTrap --
|*
|*	Trap to the monitor.  This involves dummying up a trap stack for the
|*	monitor, allowing non-maskable interrupts and then jumping to the
|*	monitor trap routine.  When it returns, non-maskable interrupts are
|*	enabled and we return.
|*
|* Results:
|*	None.
|*
|* Side effects:
|*	None.
|*
|* ----------------------------------------------------------------------
|*

	.globl	_Mach_MonTrap
_Mach_MonTrap:
	jsr	_Mach_MonStartNmi	| Restart non-maskable interrupts.
	movl	sp@(4), a0		| Address to trap to.
	clrw	sp@-			| Put on a dummy vector offset register.
	movl	#1$, sp@-		| Put the return address onto the stack.
	movw	sr, sp@-		| Push the current status register.
	jra	a0@			| Trap
1$:	jsr	_Mach_MonStopNmi	| Stop non-maskable interrupts.
	rts

|*
|* ----------------------------------------------------------------------
|*
|* MachSetVBR --
|*
|*	Set the value of the vector base register to that value that is 
|*	passed in on the stack.
|*
|*	MachSetVBR(vectorBaseAddr)
|*	    Address	vectorBaseAddr;
|*	
|* Results:
|*	None.
|*
|* Side effects:
|*	Vector base register is changed.
|*
|* ----------------------------------------------------------------------
|*

	.globl	_MachSetVBR
_MachSetVBR:
	movl	sp@(4),d0		| Get vector base address.
	movc	d0, vbr			| Load vector base register.
	rts	

|*
|* ----------------------------------------------------------------------
|*
|* MachGetVBR --
|*
|*	Get the value of the vector base register.
|*
|*	int	MachGetVBR()
|*	
|* Results:
|*	The value of the vector base register.
|*
|* Side effects:
|*	None.
|*
|* ----------------------------------------------------------------------
|*

	.globl	_MachGetVBR
_MachGetVBR:
	movc	vbr, d0			| Get vector base register.
	rts

|*
|* ----------------------------------------------------------------------
|*
|* Mach_GetStackPointer --
|*
|*	Return the value of the user's stack pointer.
|*
|* Results:
|*	Returns the user stack pointer value.
|*
|* Side effects:
|*	None.
|*
|* ----------------------------------------------------------------------
|*
	.globl _Mach_GetStackPointer
_Mach_GetStackPointer:
	movc usp, d0
	rts
/*
 * Routines between MachProbeStart() and MachProbeEnd() will return
 * FAILURE if a bus error occurs in them.
 */
	.even
	.globl _MachProbeStart
_MachProbeStart:

/*
 *----------------------------------------------------------------------
 *
 * Mach_Probe --
 *
 *	Copy a block of memory from one virtual address to another handling
 *	bus errors that may occur. This	routine is intended to be used to 
 *	probe for memory mapped devices.
 *
 * NOTE: This trap handlers force this routine to return SYS_NO_ACCESS if an
 *	 bus error occurs.
 *
 * Calling sequences:
 *
 * ReturnStatus
 * Mach_Probe(size, srcAddress, destAddress)
 *    int		size;	 Size in bytes of the read to do. Must
 *				  1, 2, 4, or 8  
 *  Address	srcAddress;	 Address to read from. 
 *  Address	destAddress;	 Address to store the read value. 
 *	
 *
 * Results:
 *	SUCCESS if the copy worked and  SYS_NO_ACCESS otherwise
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */
	.globl _Mach_Probe
_Mach_Probe:
	/*
	 * Move arguments into registers d0 gets size, a1 gets src address
	 * and a0 gets destAddress.
	 */
	movel sp@(12),a0
	movel sp@(8), a1
	movel sp@(4),d0
	/*
	 * Index based on the size argument to the correct moveb, movew, 
	 * movel or (simulated moved) instruction. Values other than 1,2,4, or
	 * 8 
	 */
	subql #1,d0
	moveq #7,d1
	cmpl d1,d0
	jhi bad
	asll #1,d0
1$:
	movew pc@(2$-1$-2:b,d0:l),d1
	clrl d0
	jmp pc@(2,d1:w)
2$:
	.word oneByte-2$
	.word twoByte-2$
	.word bad-2$
	.word fourByte-2$
	.word bad-2$
	.word bad-2$
	.word bad-2$
	.word eightByte-2$
oneByte:
	moveb a1@,a0@
	rts
twoByte:
	movew a1@,a0@
	rts
fourByte:
	movel a1@,a0@
	rts
eightByte:
	movel a1@,a0@
	movel a1@(4),a0@(4)
	rts
bad:
	moveq #1,d0
	rts
	.globl _MachProbeEnd
	.even
_MachProbeEnd:
