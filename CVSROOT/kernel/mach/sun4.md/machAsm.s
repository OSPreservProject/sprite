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
 *	Note that this uses out registers o3 and o4.  Since it's a leaf
 *	routine, callable from C, it cannot use %VOL_TEMP1 and %VOL_TEMP2.
 *	It should be okay for it to use the out registers, however.
 *	That just means that if it's called from assembly code, I shouldn't
 *	use those out registers in the calling routine, but I never do that
 *	anyway.
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
	mov %psr, %o3
	set MACH_DISABLE_INTR, %o4
	or %o3, %o4, %o3
	mov %o3, %psr
	nop					/* time for valid state reg */
	retl
	nop

/*
 * ----------------------------------------------------------------------
 *
 * Mach_EnableIntr --
 *
 *      Enable interrupts.
 *	Note that this uses out registers o3 and o4.  Since it's a leaf
 *	routine, callable from C, it cannot use %VOL_TEMP1 and %VOL_TEMP2.
 *	It should be okay for it to use the out registers, however.
 *	That just means that if it's called from assembly code, I shouldn't
 *	use those out registers in the calling routine, but I never do that
 *	anyway.
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
	mov %psr, %o3
	set MACH_ENABLE_INTR, %o4
	and %o3, %o4, %o3
	mov %o3, %psr
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
	swap	[%RETURN_VAL_REG], %RETURN_VAL_REG	/* set addr with addr */
	tst	%RETURN_VAL_REG				/* was it set? */
	be,a	ReturnZero				/* if not, return 0 */
	clr	%RETURN_VAL_REG				/* 0 it in delay slot */
	mov	0x1, %RETURN_VAL_REG			/* yes set, return 1 */
ReturnZero:
	retl
	nop




/*
 * VmMach_MapIntelPage((Address) (NET_IE_SYS_CONF_PTR_ADDR)) is call.
 */
.globl	_VmMach_MapIntelPage
_VmMach_MapIntelPage:
	set	((0x800000 / VMMACH_SEG_SIZE) - 1), %VOL_TEMP1	/* pmeg */
	stha	%VOL_TEMP1, [%o0] VMMACH_SEG_MAP_SPACE		/* segment */
	retl
	nop
