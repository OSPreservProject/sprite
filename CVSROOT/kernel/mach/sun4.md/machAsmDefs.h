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
 * Save global registers.
 * Store-doubles are faster and we do this from even register boundaries.
 * For now, we only save the globals here, since the locals and ins will
 * be saved on normal save-window operations.  Note that this means the
 * stack pointer and MACH_GLOBALS_OFFSET must be double-word aligned.
 */
#define	MACH_SAVE_GLOBAL_STATE()				\
	add	%sp, MACH_GLOBALS_OFFSET, %VOL_TEMP1;		\
	std	%g0, [%VOL_TEMP1];				\
	std	%g2, [%VOL_TEMP1 + 8];				\
	std	%g4, [%VOL_TEMP1 + 16];				\
	std	%g6, [%VOL_TEMP1 + 24]

/*
 * Restore the global registers.  We do load doubles here for speed
 * for even-register boundaries.  For now, we only restore the globals
 * from here, since the locals and ins will be restored as part of the
 * normal restore window operations.  Note that this means the stack pointer
 * and MACH_GLOBALS_OFFSET must be double-word aligned.
 */
#define	MACH_RESTORE_GLOBAL_STATE()				\
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

#define	MACH_SAVE_WINDOW_TO_BUFFER(reg1)		\
	std	%r16, [reg1];				\
	std	%r18, [reg1 + 8];			\
	std	%r20, [reg1 + 16];			\
	std	%r22, [reg1 + 24];			\
	std	%r24, [reg1 + 32];			\
	std	%r26, [reg1 + 40];			\
	std	%r28, [reg1 + 48];			\
	std	%r30, [reg1 + 56]


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
#define	MACH_ENABLE_TRAPS(useReg)			\
	mov	%psr, useReg;				\
	or	useReg, MACH_ENABLE_TRAP_BIT, useReg;	\
	mov	useReg, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Should I use xor here and MACH_ENABLE_TRAP_BIT?
 */
#define	MACH_DISABLE_TRAPS(useReg1, useReg2)		\
	mov	%psr, useReg1;				\
	set	MACH_DISABLE_TRAP_BIT, useReg2;		\
	and	useReg1, useReg2, useReg1;		\
	mov	useReg1, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Equivalents to C macros for enabling and disabling interrupts.
 * They aren't quite equivalent, since they don't do panic's on negative
 * disable counts.  These macros should really only be used for debugging,
 * in any case.
 */
#define	DISABLE_INTR_ASM(reg1, reg2, NoDisableLabel)	\
	set	_mach_AtInterruptLevel, reg1;	\
	ld	[reg1], reg1;			\
	tst	reg1;				\
	bne	NoDisableLabel;			\
	nop;					\
	mov	%psr, reg1;			\
	set	MACH_DISABLE_INTR, reg2;	\
	or	reg1, reg2, reg1;		\
	mov	reg1, %psr;			\
	MACH_WAIT_FOR_STATE_REGISTER();		\
	set	_mach_NumDisableIntrsPtr, reg1;	\
	ld	[reg1], reg2;			\
	add	reg2, 0x1, reg2;		\
	st	reg2, [reg1];			\
NoDisableLabel:

#define	ENABLE_INTR_ASM(reg1, reg2, NoEnableLabel)	\
	set	_mach_AtInterruptLevel, reg1;	\
	ld	[reg1], reg1;			\
	tst	reg1;				\
	bne	NoEnableLabel;			\
	nop;					\
	set	_mach_NumDisableIntrsPtr, reg1;	\
	ld	[reg1], reg2;			\
	sub	reg2, 0x1, reg2;		\
	st	reg2, [reg1];			\
	tst	reg2;				\
	bne	NoEnableLabel;			\
	nop;					\
	mov	%psr, reg1;			\
	set	MACH_ENABLE_INTR, reg2;		\
	and	reg1, reg2, reg1;		\
	mov	reg1, %psr;			\
	MACH_WAIT_FOR_STATE_REGISTER();		\
NoEnableLabel:

/*
 * Enable interrupts and keep traps enabled.
 * Uses given register.
 */
#define	QUICK_ENABLE_INTR(reg)				\
	mov	%psr, reg;				\
	andn	reg, MACH_DISABLE_INTR, reg;		\
	mov	reg, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Disable interrupts and keep traps enabled.
 * Uses given register.
 */
#define	QUICK_DISABLE_INTR(reg)				\
	mov	%psr, reg;				\
	or	reg, MACH_DISABLE_INTR, reg;		\
	mov	reg, %psr;				\
	MACH_WAIT_FOR_STATE_REGISTER()

/*
 * Set interrupts to a particular value.  This is useful for a routine that
 * just wants to save the value, change it, and then reset it without worrying
 * about whether this turns interrupts on or off.
 */
#define	SET_INTRS_TO(regValue, useReg1, useReg2)		\
	mov	%psr, useReg1;					\
	set	MACH_ENABLE_INTR, useReg2;			\
	and	useReg1, useReg2, useReg1;			\
	set	MACH_DISABLE_INTR, useReg2;			\
	and	regValue, useReg2, useReg2;			\
	or	useReg1, useReg2, useReg1;			\
	mov	useReg1, %psr;					\
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


/*
 * For sticking debug info into a buffer.  After each value, stamp a special
 * mark, which gets overwritten by the next value, so we always know where
 * the end of the list is.
 */
#define	MACH_DEBUG_BUF(reg1, reg2, DebugLabel, stuff)	\
	set	_debugCounter, reg1; 		\
	ld	[reg1], reg1;			\
	sll	reg1, 2, reg1;			\
	set	_debugSpace, reg2;		\
	add	reg2, reg1, reg2;		\
	st	stuff, [reg2];			\
	set	_debugCounter, reg1;		\
	ld	[reg1], reg1;			\
	add	reg1, 1, reg1;			\
	set	500, reg2;			\
	subcc	reg2, reg1, %g0;		\
	bgu	DebugLabel;			\
	nop;					\
	clr	reg1;				\
DebugLabel:					\
	set	_debugCounter, reg2;		\
	st	reg1, [reg2];			\
	sll	reg1, 2, reg1;			\
	set	_debugSpace, reg2;		\
	add	reg2, reg1, reg2;		\
	set	0x11100111, reg1;		\
	st	reg1, [reg2]

#define	MACH_DEBUG_ONCE(reg1, reg2, NoMoreLabel, DebugLabel, stuff, a_num)\
	set	_debugCounter, reg1;		\
	ld	[reg1], reg1;			\
	cmp	reg1, a_num;			\
	bg	NoMoreLabel;			\
	nop;					\
	MACH_DEBUG_BUF(reg1, reg2, DebugLabel, stuff);	\
NoMoreLabel:					\
	nop

/*
 * This is the equivalent to a call to VmmachGetPage followed by a test on
 * the returned pte to see if it is resident and user readable and writable.
 * This is used where a call to the vm code won't work since output registers
 * aren't available.  This sets the condition codes so that 0 is returned
 * if the address is resident and not protected and not zero is returned if
 * a fault would occur.
 */
#define	MACH_CHECK_FOR_FAULT(checkReg, reg1)	\
	set	VMMACH_PAGE_MAP_MASK, reg1;		\
	and	checkReg, reg1, reg1;			\
	lda	[reg1] VMMACH_PAGE_MAP_SPACE, reg1;	\
	srl	reg1, VMMACH_PAGE_PROT_SHIFT, reg1;	\
	cmp	reg1, VMMACH_PTE_OKAY_VALUE

/*
 * This is similar to the above macro, except that it is intended for checking
 * stack pointers to see if a restore of a window would page fault.  We check
 * the value of the stack pointer, and the offset of the window size.
 */
#define MACH_CHECK_STACK_FAULT(checkReg, reg1, ansReg, reg2, TestAgainLabel, LastOKLabel)	\
	clr	ansReg;					\
	set	VMMACH_PAGE_MAP_MASK, reg1;		\
	and	checkReg, reg1, reg1;			\
	lda	[reg1] VMMACH_PAGE_MAP_SPACE, reg1;	\
	srl	reg1, VMMACH_PAGE_PROT_SHIFT, reg1;	\
	cmp	reg1, VMMACH_PTE_OKAY_VALUE;		\
	be	TestAgainLabel;				\
	nop;						\
	set	0x2, ansReg;				\
TestAgainLabel:						\
	set	VMMACH_PAGE_MAP_MASK, reg1;		\
	add	checkReg, (MACH_SAVED_WINDOW_SIZE - 4), reg2;	\
	and	reg2, reg1, reg1;			\
	lda	[reg1] VMMACH_PAGE_MAP_SPACE, reg1;	\
	srl	reg1, VMMACH_PAGE_PROT_SHIFT, reg1;	\
	cmp	reg1, VMMACH_PTE_OKAY_VALUE;		\
	be	LastOKLabel;				\
	nop;						\
	or	0x4, ansReg, ansReg;			\
LastOKLabel:						\
	tst	ansReg
	
	


/*
 * Get a ptr to the pcb structure of the current process.
 * Return this in reg1
 */
#define	MACH_GET_CUR_PROC_PTR(reg1)			\
	set	_proc_RunningProcesses, reg1;		\
	ld	[reg1], reg1;				\
	ld	[reg1], reg1

/*
 * Get a ptr to the Mach_State struct of the current process.  Put it in reg1.
 */
#define	MACH_GET_CUR_STATE_PTR(reg1, reg2)		\
	MACH_GET_CUR_PROC_PTR(reg1);			\
        set     _machStatePtrOffset, reg2;		\
        ld      [reg2], reg2;				\
        add     reg1, reg2, reg1;			\
        ld      [reg1], reg1

#endif /* _MACHASMDEFS */
