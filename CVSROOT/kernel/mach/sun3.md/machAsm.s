|* sunSubr.s --
|*
|*     Contains misc. assembler routines for the SUN.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*
|* rcs = $Header$ SPRITE (Berkeley)
|*

#include "sunSR.h"
#include "vmSunConst.h"
#include "machineConst.h"
#include "asmDefs.h"

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
    movc	sfc,d1			| Save source function code
    movl	#VM_MMU_SPACE,d0	| Get function code in a reg
    movc	d0,sfc			| Set source function code

    movl	sp@(4),a0		| Get pointer to target ethernet address
    movl	d2,sp@-			| save d2

    movl	#6,d0			| loop counter
    movl	#VM_ETHER_ADDR,a1	| The Prom address of the ethernet addr
etherloop:
    movsb	a1@,d2			| Copy one byte of the ethernet address
    movb	d2,a0@			|   from prom to the target address.
    addql	#1,a0			| bump target pointer
    addl	#VM_IDPROM_INC,a1	| bump prom address, as per sec 4.8
    subl	#1,d0			| decrement loop counter
    bne		etherloop		| loop 6 times

    movc	d1,sfc			| Restore source function code
    movl	sp@+,d2			| Restore d2

    rts					| Return

|*---------------------------------------------------------------------
|*
|* Mach_ContextSwitch -
|*
|*	Mach_ContextSwitch(switchToRegs, switchFromRegs)
|*
|*	Switch the thread of execution to a new processes.  This routine
|*	is passed a pointer to the saved registers of the process that is
|*      begin switched to and a pointer to the saved registers of the process
|*      that is being switched from.  It goes through the following steps:
|*
|*	1) Push the status register and the
|*	   user stack pointer onto the stack.
|*	2) Push the source and destination function codes onto the stack.
|*	2) Push a magic number onto the stack to see if it gets trashed.
|*	3) Save all of the registers d0-d7, a0-a7 for the process being
|*	   switched from.
|*	4) Restore general registers and the status register of the process 
|*	   being switched to.
|*	5) Verify the magic number.
|*	6) Return in the new process.
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

ENTRY(Mach_ContextSwitch, 0, 0)
    
    movw	sr, sp@-		| Save the current value of the status
					|     register on the stack.
    movw	#SUN_SR_HIGHPRIO, sr	| Lock out interrupts.

    movl	usp, a0  		| Push the user stack pointer onto 
    movl	a0, sp@-		|     the stack.

    movc	sfc, d0			| Push source and dest function codes 
    movl	d0, sp@-		|     onto the stack.
    movc	dfc, d0
    movl	d0, sp@-

    movl	#MAGIC, sp@-		| Put the magic number on the stack.

    movl	a6@(12), a0		| Save all of the registers for the 
    moveml	#0xffff, a0@		|    process being switched from.

    movl	a6@(8), a0		| Restore all of the registers for the
    moveml	a0@, #0xffff		|    process being switched to.

    movl	#MAGIC, d0		| Check against the magic number
    cmpl	sp@, d0			|
    beq		1$			|
    trap	#15			|

1$:
    addql	#4, sp			| Pop the magic number.

    movl	sp@+, d0		| Restore the dest and source
    movc	d0, dfc			|     function codes.
    movl	sp@+, d0
    movc	d0, sfc

    movl	sp@+, a0		| Restore the user stack pointer.
    movl	a0, usp

    movw	sp@+, sr		| Restore the status register.

RETURN

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
    movc	sfc,d1			| Save source function code
    movl	#VM_MMU_SPACE,d0	| Get function code in a reg
    movc	d0,sfc			| Set source function code

    clrl	d0			| Clear the return register

    movl	#VM_MACH_TYPE_ADDR, a0	| Get the address of the machine type
					|     in a register.
    movsb	a0@,d0			| Store the machine type in the return
					|     register.

    movc	d1,sfc			| Restore source function code

    rts


|*
|* ----------------------------------------------------------------------
|*
|* MonNmiNop --
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

    .globl _MonNmiNop
_MonNmiNop:
        moveml  #0xC0C0,sp@-            | Save d0,d1,a0,a1

#ifdef SUN2
        movw    #0xFFE1, AMD9513_CSR	| Clear the output of timer #1 
					| on the AMD timer chip to clear
					| the current interrupt.
| 
| Move the upper half of the status register to the leds.
|

        movc    dfc,a0			| Change to MMU space
        moveq   #VM_MMU_SPACE,d1	|
        movc    d1,dfc			|
        movb    sp@(16),d0		| Move the upper half of the sr to d0
        eorb    #0xFF,d0		| Exclusive or the bits.
        movsb   d0,0xB                  | Move it to the leds.
        movc    a0,dfc			| Change back to normal space.

#endif SUN2
        moveml  sp@+,#0x0303            | restore regs
        rte


|*
|* ----------------------------------------------------------------------
|*
|* Mon_Trap --
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

	.globl	_Mon_Trap
_Mon_Trap:
	jsr	_Mon_StartNmi		| Restart non-maskable interrupts.
	movl	sp@(4), a0		| Address to trap to.
	clrw	sp@-			| Put on a dummy vector offset register.
	movl	#1$, sp@-		| Put the return address onto the stack.
	movw	sr, sp@-		| Push the current status register.
	jra	a0@			| Trap
1$:	jsr	_Mon_StopNmi		| Stop non-maskable interrupts.
	rts

|*
|* ----------------------------------------------------------------------
|*
|* ExcSetVBR --
|*
|*	Set the value of the vector base register to that value that is 
|*	passed in on the stack.
|*
|*	ExcSetVBR(vectorBaseAddr)
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

	.globl	_ExcSetVBR
_ExcSetVBR:
	movl	sp@(4),d0		| Get vector base address.
	movc	d0, vbr			| Load vector base register.
	rts	

|*
|* ----------------------------------------------------------------------
|*
|* ExcGetVBR --
|*
|*	Get the value of the vector base register.
|*
|*	int	ExcGetVBR()
|*	
|* Results:
|*	The value of the vector base register.
|*
|* Side effects:
|*	None.
|*
|* ----------------------------------------------------------------------
|*

	.globl	_ExcGetVBR
_ExcGetVBR:
	movc	vbr, d0			| Get vector base register.
	rts	
