/*
 * machAsmDefs.h --
 *
 *     Machine-dependent macros.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _MACHASMDEFS
#define _MACHASMDEFS

#ifdef KERNEL
#include "vmSun4Const.h"
#include "machConst.h"
#else
#include <kernel/vmSun4Const.h>
#include <kernel/machConst.h>
#endif

/*
 * Wait the 3 instructions necessary to allow a newly-written state register
 * to settle.
 */
#define	MACH_WAIT_FOR_STATE_REGISTER()			\
	nop;						\
	nop;						\
	nop
/*
 * Bump the invalid window forward one.  This is done by changing the
 * invalid window mask.  We shift the invalid window bit left by 1,
 * but modulo the number of implemented windows.
 */
#define	MACH_ADVANCE_WIM(REG1, REG2)			\
	mov	%wim, REG1;				\
	srl	REG1, 0x1, REG2;			\
	sll	REG1, (MACH_NUM_WINDOWS - 1), REG1;	\
	or	REG1, REG2, REG1;			\
	set	MACH_VALID_WIM_BITS, REG2;		\
	and	REG1, REG2, REG1;			\
	mov	REG1, %wim;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Move the invalid window backwards one.  This is done by changing the
 * invalid window mask.  We shift the invalid window bit right by 1,
 * but modulo the number of implemented windows.
 */
#define	MACH_RETREAT_WIM(REG1, REG2, happyWindowLabel)	\
	mov	%wim, REG1;				\
	sll	REG1, 0x1, REG1;			\
	set	MACH_VALID_WIM_BITS, REG2;		\
	andcc	REG2, REG1, REG1;			\
	bne	happyWindowLabel;			\
	nop;						\
	mov	0x1, REG1;				\
happyWindowLabel:					\
	mov	REG1, %wim;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Test whether we're in an invalid window.  If we are in an invalid window,
 * then the condition codes should indicate a not zero ("bne" instruction
 * will branch).
 */
#define	MACH_INVALID_WINDOW_TEST()				\
	mov	%psr, %VOL_TEMP1;				\
	and	%VOL_TEMP1, MACH_CWP_BITS, %VOL_TEMP1;		\
	set	0x1, %VOL_TEMP2;				\
	sll	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP1;		\
	mov	%wim, %VOL_TEMP2;				\
	andcc	%VOL_TEMP1, %VOL_TEMP2, %g0

/*
 * Test whether we're about to encounter a window underflow condition.
 * We put current cwp into temp1.  We shift a one by that many bits.
 * Then we shift it again, to "advance" the window by one.  We and it
 * with the valid window bits to get it modulo the number of windows - 1.
 * Then we compare it with current wim to see if they're the same.  If
 * so, then we would get an underflow if we did a restore operation.
 * If we are in an underflow situation, then the condition codes should
 * indicate a not zero ("bne" instruction will branch).
 */
#define	MACH_UNDERFLOW_TEST()					\
	mov	%psr, %VOL_TEMP1;				\
	and	%VOL_TEMP1, MACH_CWP_BITS, %VOL_TEMP1;		\
	set	0x1, %VOL_TEMP2;				\
	sll	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP1;		\
	sll	%VOL_TEMP1, 0x1, %VOL_TEMP1;			\
	set	MACH_VALID_WIM_BITS, %VOL_TEMP2;		\
	andcc	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1;		\
	bne	testWimOkay;					\
	nop;							\
	mov	0x1, %VOL_TEMP1;				\
testWimOkay:							\
	mov	%wim, %VOL_TEMP2;				\
	andcc	%VOL_TEMP1, %VOL_TEMP2, %g0
    

/*
 * Save trap state registers.  This saves %l3 (= %VOL_TEMP1) and
 * %l4 (= %VOLl_TEMP2) since store-doubles are faster and we do
 * this from even register boundaries.  Note however that these
 * registers are not restored, since this would overwrite the registers
 * as they are being used to restore things...
 */
#define	MACH_SAVE_TRAP_STATE()					\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_GLOBALS_OFFSET, %VOL_TEMP1;	\
	std	%g0, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%g2, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%g4, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%g6, [%VOL_TEMP1];				\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_OUTS_OFFSET, %VOL_TEMP1;	\
	std	%o0, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%o2, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%o4, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%o6, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_LOCALS_OFFSET, %VOL_TEMP1;	\
	std	%l0, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%l2, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%l4, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%l6, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_INS_OFFSET, %VOL_TEMP1;	\
	std	%i0, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%i2, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%i4, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	std	%i6, [%VOL_TEMP1];				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_PSR_OFFSET, %VOL_TEMP1;	\
	mov	%psr, %VOL_TEMP2;				\
	st	%VOL_TEMP2, [%VOL_TEMP1];			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_Y_OFFSET, %VOL_TEMP1;		\
	mov	%y, %VOL_TEMP2;					\
	st	%VOL_TEMP2, [%VOL_TEMP1];			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_TBR_OFFSET, %VOL_TEMP1;	\
	mov	%tbr, %VOL_TEMP2;				\
	st	%VOL_TEMP2, [%VOL_TEMP1];			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_WIM_OFFSET, %VOL_TEMP1;	\
	mov	%wim, %VOL_TEMP2;				\
	st	%VOL_TEMP2, [%VOL_TEMP1]

/*
 * Restore the trap state registers.  Note that this does not restore
 * %l3 or %l4, since they are the %VOL_TEMP1 and %VOL_TEMP2 registers
 * that are in use for the macro.  NOTE that this doesn't restore the
 * psr to exactly what it was.  It restores it to what it was, except for
 * the current window pointer.  That part of the psr it leaves as it is
 * right now.  This is because when returning from context switches, we
 * may not return to the window we left from...  Also, I don't restore
 * the window invalid mask.  That would only make it point to something wrong.
 */
#define	MACH_RESTORE_TRAP_STATE()				\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_GLOBALS_OFFSET, %VOL_TEMP1;	\
	ldd	[%VOL_TEMP1], %g0;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %g2;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %g4;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %g6;				\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_OUTS_OFFSET, %VOL_TEMP1;	\
	ldd	[%VOL_TEMP1], %o0;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %o2;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %o4;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %o6;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_LOCALS_OFFSET, %VOL_TEMP1;	\
	ldd	[%VOL_TEMP1], %l0;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ld	[%VOL_TEMP1], %l2;				\
	add	%VOL_TEMP1, 12, %VOL_TEMP1;			\
	ld	[%VOL_TEMP1], %l5;				\
	add	%VOL_TEMP1, 4, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %l6;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_INS_OFFSET, %VOL_TEMP1;	\
	ldd	[%VOL_TEMP1], %i0;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %i2;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %i4;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	ldd	[%VOL_TEMP1], %i6;				\
	add	%VOL_TEMP1, 8, %VOL_TEMP1;			\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_PSR_OFFSET, %VOL_TEMP1;	\
	ld	[%VOL_TEMP1], %VOL_TEMP2;			\
	set	(~MACH_CWP_BITS), %VOL_TEMP1;			\
	and	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2;		\
	mov	%psr, %VOL_TEMP1;				\
	and	%VOL_TEMP1, MACH_CWP_BITS, %VOL_TEMP1;		\
	or	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2;		\
	mov	%VOL_TEMP2, %psr;				\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_Y_OFFSET, %VOL_TEMP1;		\
	ld	[%VOL_TEMP1], %VOL_TEMP2;			\
	mov	%VOL_TEMP2, %y;					\
	set	_stateHolder, %VOL_TEMP1;			\
	add	%VOL_TEMP1, MACH_TBR_OFFSET, %VOL_TEMP1;	\
	ld	[%VOL_TEMP1], %VOL_TEMP2;			\
	mov	%VOL_TEMP2, %tbr;				\
	MACH_WAIT_FOR_STATE_REGISTER()


/*
 * Save r16 to r23 (locals) and r24 to r31 (ins) to 16 words at
 * the top of this window's stack.
 */
#define	MACH_SAVE_WINDOW_TO_STACK()			\
	std	%r16, [%sp];				\
	std	%r18, [%sp + 8];			\
	std	%r20, [%sp + 16];			\
	std	%r22, [%sp + 24];			\
	std	%r24, [%sp + 32];			\
	std	%r26, [%sp + 40];			\
	std	%r28, [%sp + 48];			\
	std	%r30, [%sp + 56]

#define	MACH_RESTORE_WINDOW_FROM_STACK()		\
	ldd	[%sp], %r16;				\
	ldd	[%sp + 8], %r18;			\
	ldd	[%sp + 16], %r20;			\
	ldd	[%sp + 24], %r22;			\
	ldd	[%sp + 32], %r24;			\
	ldd	[%sp + 40], %r26;			\
	ldd	[%sp + 48], %r28;			\
	ldd	[%sp + 56], %r30


/*
 * Clear out the local and out registers for a new window to move into
 * or a window we're moving out of.
 * Outs: r8 to r15, locals: r16 to r23.
 */
#define	MACH_CLEAR_WINDOW()				\
	clr	%r8;					\
	clr	%r9;					\
	clr	%r10;					\
	clr	%r11;					\
	clr	%r12;					\
	clr	%r13;					\
	clr	%r14;					\
	clr	%r15;					\
	clr	%r16;					\
	clr	%r17;					\
	clr	%r18;					\
	clr	%r19;					\
	clr	%r20;					\
	clr	%r21;					\
	clr	%r22;					\
	clr	%r23

/*
 * Enabling and disabling traps.
 */
#define	MACH_ENABLE_TRAPS()					\
	mov	%psr, %VOL_TEMP1;				\
	or	%VOL_TEMP1, MACH_ENABLE_TRAP_BIT, %VOL_TEMP1;	\
	mov	%VOL_TEMP1, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Should I use xor here and MACH_ENABLE_TRAP_BIT?
 */
#define	MACH_DISABLE_TRAPS()					\
	mov	%psr, %VOL_TEMP1;				\
	set	MACH_DISABLE_TRAP_BIT, %VOL_TEMP2;		\
	and	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1;		\
	mov	%VOL_TEMP1, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Run at high priority: supervisor mode, interrupts disabled, traps enabled.
 * This must be done in 2 steps - 1) leaving traps off, if they were off,
 * set new interrupt level.  2) Enable traps.  This keeps us from getting
 * an interrupt at the old level rather than the new right after enabling
 * traps.  
 */
#define	MACH_SR_HIGHPRIO()					\
	mov	%psr, %VOL_TEMP1;				\
	set	(MACH_DISABLE_INTR | MACH_SUPER_BIT), %VOL_TEMP2;	\
	or	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1;		\
	mov	%VOL_TEMP1, %psr;				\
	or	%VOL_TEMP1, MACH_ENABLE_TRAP_BIT, %VOL_TEMP1;	\
	mov	%VOL_TEMP1, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Run at low supervisor priority: supervisor mode, interrupts enabled, traps
 * enabled.  As described above for MACH_SR_HIGHPRIO, we must do this in
 * 2 steps.
 */
#define	MACH_SR_LOWPRIO()					\
	mov	%psr, %VOL_TEMP1;				\
	set	MACH_SUPER_BIT, %VOL_TEMP2;			\
	or	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1;		\
	set	MACH_ENABLE_INTR, %VOL_TEMP2;			\
	and	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1;		\
	mov	%VOL_TEMP1, %psr;				\
	or	%VOL_TEMP1, MACH_ENABLE_TRAP_BIT, %VOL_TEMP2;	\
	mov	%VOL_TEMP1, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()
/*
 * Run at user priority: user mode, traps on.
 */
#define	MACH_SR_USERPRIO()					\
	mov	%psr, %VOL_TEMP1;				\
	set	MACH_ENABLE_TRAP_BIT, %VOL_TEMP2;		\
	or	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1;		\
	set	(~MACH_SUPER_BIT), %VOL_TEMP2;			\
	and	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1;		\
	mov	%VOL_TEMP1, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()
#endif /* _MACHASMDEFS */
