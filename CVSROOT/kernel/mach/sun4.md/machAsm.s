/* machAsm.s --
 *
 *     Contains misc. assembler routines for the sun4.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 * rcs = $Header$ SPRITE (Berkeley)
 */

#include "machConst.h"
.seg	"text"

/*
 * ----------------------------------------------------------------------
 *
 * __MachGetPc --
 *
 *	Jump back to caller without over-writing pc that was put into
 *	%RETURN_VAL_REG as a side effect of the jmp to this routine.
 *
 * Results:
 *	Old pc is returned in %RETURN_VAL_REG.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	__MachGetPc
__MachGetPc:
	jmpl	%RETURN_VAL_REG, %g0
	nop

/*
 * ----------------------------------------------------------------------
 *
 * Mach_DisableIntr --
 *
 *	Disable interrupts.  This leaves nonmaskable interrupts enabled.
 *	Note that this uses out registers.  Since it's a leaf
 *	routine, callable from C, it cannot use %VOL_TEMP1 and %VOL_TEMP2.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Maskable interrupts are disabled.
 *
 * ----------------------------------------------------------------------
 */
.globl	_Mach_DisableIntr
_Mach_DisableIntr:
	mov %psr, %OUT_TEMP1
	set MACH_DISABLE_INTR, %OUT_TEMP2
	or %OUT_TEMP1, %OUT_TEMP2, %OUT_TEMP1
	mov %OUT_TEMP1, %psr
	nop					/* time for valid state reg */
	retl
	nop

/*
 * ----------------------------------------------------------------------
 *
 * Mach_EnableIntr --
 *
 *      Enable interrupts.
 *	Note that this uses out registers.  Since it's a leaf
 *	routine, callable from C, it cannot use %VOL_TEMP1 and %VOL_TEMP2.
 *	This enables all interrupts, so if before disabling them
 *	we had only certain priority interrupts enabled, this will lose
 *	that information.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      Interrupts are enabled.
 *
 * ----------------------------------------------------------------------
 */
.globl	_Mach_EnableIntr
_Mach_EnableIntr:
	mov %psr, %OUT_TEMP1
	set MACH_ENABLE_INTR, %OUT_TEMP2
	and %OUT_TEMP1, %OUT_TEMP2, %OUT_TEMP1
	mov %OUT_TEMP1, %psr
	nop					/* time for valid state reg */
	retl
	nop

/*
 * Jmpl writes current pc into RETURN_VAL_REG_CHILD here, which is where values
 * are returned from calls.  The return from __MachGetPc must not overwrite 
 * this.  Since this uses a non-pc-relative jump, we CANNOT use this routine 
 * while executing before we've copied the kernel to where it was linked for. 
 */
.globl	_Mach_GetPC
_Mach_GetPC:
	set	__MachGetPc, %RETURN_VAL_REG
	jmpl	%RETURN_VAL_REG, %RETURN_VAL_REG
	nop
	retl
	nop

/*
 *---------------------------------------------------------------------
 *
 * Mach_TestAndSet --
 *
 *     int Mach_TestAndSet(intPtr)
 *          int *intPtr;
 *
 *     Test and set an operand.
 *
 * Results:
 *     Returns 0 if *intPtr was zero and 1 if *intPtr was non-zero.  Also
 *     in all cases *intPtr is set to a non-zero value.
 *
 * Side effects:
 *     None.
 *
 *---------------------------------------------------------------------
 */
.globl	_Mach_TestAndSet
_Mach_TestAndSet:
	/*
	 * We're a leaf routine, so our return value goes in the save register
	 * that our operand does.  Move the operand before overwriting.
	 * The ISP description of the swap instruction indicates that it is
	 * okay to use the same address and destination registers.  This will
	 * put the address into the addressed location to set it.  That means
	 * we can't use address 0, but we shouldn't be doing that anyway.
	 */
#ifdef	NO_SUN4_CACHING
	set	0x1, %OUT_TEMP1
	ld	[%RETURN_VAL_REG], %OUT_TEMP2
	st	%OUT_TEMP1, [%RETURN_VAL_REG]
	mov	%OUT_TEMP2, %RETURN_VAL_REG
#else
	swap	[%RETURN_VAL_REG], %RETURN_VAL_REG	/* set addr with addr */
#endif /* NO_SUN4_CACHING */
	tst	%RETURN_VAL_REG				/* was it set? */
	be,a	ReturnZero				/* if not, return 0 */
	clr	%RETURN_VAL_REG				/* silly, delay slot */
	mov	0x1, %RETURN_VAL_REG			/* yes set, return 1 */
ReturnZero:
	retl
	nop


.globl	_Mach_GetMachineType
_Mach_GetMachineType:
	set	VMMACH_MACH_TYPE_ADDR, %o0
	lduba	[%o0] VMMACH_CONTROL_SPACE, %o0
	retl
	nop

#ifdef NOTDEF
.globl	_panic
_panic:
	mov	%o7, %VOL_TEMP1
	sethi   %hi(-0x17ef7c),%VOL_TEMP2
	ld      [%VOL_TEMP2+%lo(-0x17ef7c)],%VOL_TEMP2
	call    %VOL_TEMP2
	nop
	mov	%VOL_TEMP1, %o7
loopForever:
	ba	loopForever
	nop
#endif NOTDEF
