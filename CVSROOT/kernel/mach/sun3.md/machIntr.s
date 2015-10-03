|* machIntr.s -
|*
|*     Contains the interrupt handlers.
|*
|* Copyright 1989 Regents of the University of California
|* Permission to use, copy, modify, and distribute this
|* software and its documentation for any purpose and without
|* fee is hereby granted, provided that the above copyright
|* notice appear in all copies.  The University of California
|* makes no representations about the suitability of this
|* software for any purpose.  It is provided "as is" without
|* express or implied warranty.
|*


#include "machConst.h"
#include "machAsmDefs.h"
.data
.asciz "$Header$ SPRITE (Berkeley)"
	.even
	.globl	_machSpuriousCnt
_machSpuriousCnt:	.long	0
	.globl	_machLevel1Cnt
_machLevel1Cnt:	.long	0
	.globl	_machLevel2Cnt
_machLevel2Cnt:	.long	0
	.globl	_machLevel3Cnt
_machLevel3Cnt:	.long	0
	.globl	_machLevel4Cnt
_machLevel4Cnt:	.long	0
	.globl	_machLevel5Cnt
_machLevel5Cnt:	.long	0
	.globl	_machLevel6Cnt
_machLevel6Cnt:	.long	0

.even
.text

|*
|* ----------------------------------------------------------------------------
|*
|* Interrupt handling --
|*
|*     Handle exceptions.  Enter the debugger for unsuported interrupts.
|*
|* Results:
|*     None.
|*
|* Side effects:
|*     None.
|*
|* ----------------------------------------------------------------------------
|*
	.globl	_proc_RunningProcesses

	.globl	MachSpurious
MachSpurious:
	addl	#1,_machSpuriousCnt
	CallTrapHandler(MACH_SPURIOUS_INT)

	.globl	MachLevel1Int
MachLevel1Int:
	addl	#1,_machLevel1Cnt
	CallTrapHandler(MACH_LEVEL1_INT)

	.globl	MachLevel2Int
MachLevel2Int:
	addl	#1,_machLevel2Cnt
	CallTrapHandler(MACH_LEVEL2_INT)

	.globl	MachLevel3Int
MachLevel3Int:
	addl	#1,_machLevel3Cnt
	CallTrapHandler(MACH_LEVEL3_INT)

	.globl	MachLevel4Int
MachLevel4Int:
	addl	#1,_machLevel4Cnt
	CallTrapHandler(MACH_LEVEL4_INT)

	.globl	MachLevel5Int
MachLevel5Int:
	addl	#1,_machLevel5Cnt
/*	movl    sp@(2), _Prof_InterruptPC */
	CallTrapHandler(MACH_LEVEL5_INT)

	.globl	MachLevel6Int
MachLevel6Int:
	addl	#1,_machLevel6Cnt
	CallTrapHandler(MACH_LEVEL6_INT) 

/*
 * ----------------------------------------------------------------------------
 *
 * Call a Vectored Interrupt Handler --
 *
 *      Call an interrupt handler.  The temporary registers d0, d1, a0 and a1
 *	are saved first, then interrupts are disabled and an "At Interrupt 
 *	Level" flag is set so the handler can determine that it is running a
 *	interrupt level.  The registers are restored at the end.
 *	This code assumes that if the interrupt occured in user mode, and if
 *	the specialHandling flag is set on the way back to user mode, then
 *	a context switch is desired. Note that schedFlags is not checked.
 *
 *  Algorithm:
 *	Save temporary registers
 *	Determine if interrupt occured while in kernel mode or user mode
 *	Compute the routine it call and its clientdata using the vector
 *		from the VOR register.
 *	Call routine
 *	if interrupt occured while in user mode.
 *	    if specialHandling is set for the current process 
 *    	        Set the old status register trace mode bit on.
 *	        Clear the specialHandling flag.
 *	    endif
 *	endif
 *	restore registers
 *		
 *
 * ----------------------------------------------------------------------------
 */

#define	INTR_SR_OFFSET	16
#define	INTR_VOR_OFFSET	22
.even
.text
	.globl _MachVectoredInterrupt

_MachVectoredInterrupt:
/*	movl    sp@(2), _Prof_InterruptPC */
	moveml	#0xC0C0, sp@-
	movw	#MACH_SR_HIGHPRIO, sr 
	movw	sp@(INTR_SR_OFFSET), d0
	andl	#MACH_SR_SUPSTATE, d0
	movl	d0, _mach_KernelMode
	movl	#1, _mach_AtInterruptLevel 
        movw    sp@(INTR_VOR_OFFSET), d0 
	andl 	#1020, d0 
        lea  	_machInterruptRoutines, a0 
        lea 	_machInterruptArgs, a1 
        movel 	a1@(d0:l),sp@-
	movel 	a0@(d0:l),a0
	jbsr 	a0@ 
	addql 	#4, sp 
	clrl	_mach_AtInterruptLevel 
	tstl	_mach_KernelMode
	bne	1f
	movl	_proc_RunningProcesses, a0
	movl	a0@, a1
	movl	_machSpecialHandlingOffset, d1
	tstl	a1@(0,d1:l)
	beq	1f
	clrl	a1@(0,d1:l)
	movw	sp@(INTR_SR_OFFSET), d0
	orw	#MACH_SR_TRACEMODE, d0
	movw	d0, sp@(INTR_SR_OFFSET)
1:	moveml	sp@+, #0x0303
	rte 
