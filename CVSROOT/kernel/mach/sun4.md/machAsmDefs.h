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
#include "vmSunConst.h"
#include "machConst.h"
#else
#include <kernel/vmSunConst.h>
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
 * Set the window invalid mask to point to the current window.
 */
#define	MACH_SET_WIM_TO_CWP()					\
	mov	%psr, %VOL_TEMP1;				\
	and	%VOL_TEMP1, MACH_CWP_BITS, %VOL_TEMP1;		\
	set	0x1, %VOL_TEMP2;				\
	sll	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP1;		\
	mov	%VOL_TEMP1, %wim;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Move the invalid window backwards one.  This is done by changing the
 * invalid window mask.  We shift the invalid window bit right by 1,
 * but modulo the number of implemented windows.
 */
#define	MACH_RETREAT_WIM(REG1, REG2, happyWindow)	\
	mov	%wim, REG1;				\
	sll	REG1, 0x1, REG1;			\
	set	MACH_VALID_WIM_BITS, REG2;		\
	andcc	REG2, REG1, REG1;			\
	bne	happyWindow;				\
	nop;						\
	mov	0x1, REG1;				\
happyWindow:						\
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
#define	MACH_UNDERFLOW_TEST(moduloOkay)				\
	mov	%psr, %VOL_TEMP1;				\
	and	%VOL_TEMP1, MACH_CWP_BITS, %VOL_TEMP1;		\
	set	0x1, %VOL_TEMP2;				\
	sll	%VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP1;		\
	sll	%VOL_TEMP1, 0x1, %VOL_TEMP1;			\
	set	MACH_VALID_WIM_BITS, %VOL_TEMP2;		\
	andcc	%VOL_TEMP1, %VOL_TEMP2, %VOL_TEMP1;		\
	bne	moduloOkay;					\
	nop;							\
	mov	0x1, %VOL_TEMP1;				\
moduloOkay:							\
	mov	%wim, %VOL_TEMP2;				\
	andcc	%VOL_TEMP1, %VOL_TEMP2, %g0

/*
 * The sequence we need to go through to restore the psr without restoring
 * the old current window number.  We want to remain in our current window.
 * 1) Get old psr.  2) Clear only its cwp bits.  3) Get current psr.
 * 4) Grab only its cwp bits.  5) Stick the two together and put it in
 * the psr reg.  6) Wait for the register to be valid.
 */
#define	MACH_RESTORE_PSR()					\
	mov	%CUR_PSR_REG, %VOL_TEMP2;			\
	set     (~MACH_CWP_BITS), %VOL_TEMP1;			\
	and     %VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2;		\
	mov     %psr, %VOL_TEMP1;				\
	and     %VOL_TEMP1, MACH_CWP_BITS, %VOL_TEMP1;		\
	or      %VOL_TEMP2, %VOL_TEMP1, %VOL_TEMP2;		\
	mov     %VOL_TEMP2, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()
    

/*
 * Save trap state registers.
 * Store-doubles are faster and we do this from even register boundaries.
 * For now, we only save the globals here, since the locals and ins will
 * be saved on normal save-window operations.  Note that this means the
 * stack pointer and MACH_GLOBALS_OFFSET must be double-word aligned.
 */
#define	MACH_SAVE_TRAP_STATE()					\
	add	%sp, MACH_GLOBALS_OFFSET, %VOL_TEMP1;		\
	std	%g0, [%VOL_TEMP1];				\
	std	%g2, [%VOL_TEMP1 + 8];				\
	std	%g4, [%VOL_TEMP1 + 16];				\
	std	%g6, [%VOL_TEMP1 + 24]

/*
 * Restore the trap state registers.  We do load doubles here for speed
 * for even-register boundaries.  For now, we only restore the globals
 * from here, since the locals and ins will be restored as part of the
 * normal restore window operations.  Note that this means the stack pointer
 * and MACH_GLOBALS_OFFSET must be double-word aligned.
 */
#define	MACH_RESTORE_TRAP_STATE()				\
	add	%sp, MACH_GLOBALS_OFFSET, %VOL_TEMP1;		\
	ldd	[%VOL_TEMP1], %g0;				\
	ldd	[%VOL_TEMP1 + 8], %g2;				\
	ldd	[%VOL_TEMP1 + 16], %g4;				\
	ldd	[%VOL_TEMP1 + 24], %g6


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
