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
 *	Jump back to caller without writing pc that was put into %o0 as
 *	a side effect of the jmp to this routine.
 *
 * Results:
 *	Old pc is returned in %o0.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	__MachGetPc
__MachGetPc:
	jmpl	%o0, %g0
	nop

/*
 * ----------------------------------------------------------------------
 *
 * Mach_DisableIntr --
 *
 *	Disable interrupts.  This leaves nonmaskable interrupts enabled.
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
	mov %psr, %VOL_TEMP1
	set MACH_DISABLE_INTR, %VOL_TEMP2
	or %VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
	mov %VOL_TEMP1, %psr
	retl
	nop

/*
 * ----------------------------------------------------------------------
 *
 * Mach_EnableIntr --
 *
 *      Enable interrupts.
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
	mov %psr, %VOL_TEMP1
	set MACH_ENABLE_INTR, %VOL_TEMP2
	and %VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1
	mov %VOL_TEMP1, %psr
	retl
	nop
