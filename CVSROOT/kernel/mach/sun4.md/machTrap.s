/*
 * trap.s --
 *
 *	Traps for sun4.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

.seg	"data"
.asciz	"$Header$ SPRITE (Berkeley)"
.align	8
.seg	"text"

#include "machConst.h"
#include "machAsmDefs.h"

.align	8
.seg	"text"

/*
 * ----------------------------------------------------------------------
 *
 * MachWindowOverflow --
 *
 *	Window overflow trap handler.  It assumes it's called with the %psr
 *	read into %l0.
 *	The window we've trapped into is currently invalid.  We want to
 *	make it valid.  We do this by moving one window further, saving that
 *	window to the stack, marking it as the new invalid window, and then
 *	moving back to the window that we trapped into.  It is then valid
 *	and usable.  Note that we move first to the window to save and then
 *	mark it invalid.  In the other order we would get another overflow
 *	trap, but with traps turned off...
 *	Note that %sp should point to highest (on stack, lowest in memory)
 *	usable word in the current stack frame.  %fp is the %sp of the
 *	caller's stack frame, so the first (highest in memory) usable word
 *	of a current stack frame is (%fp - 4).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stack grows.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachWindowOverflow
MachWindowOverflow:
	/*
	 * We enter inside of an invalid window, so we can't use in registers,
	 * out registers or global registers to begin with.
	 * We temporarily clear out some globals since we won't be able to
	 * use locals in window we move to before calling macros.
	 */
	mov	%g3, %l3
	mov	%g4, %l4
	save				/* move to the window to save */
	MACH_ADVANCE_WIM(%g3, %g4)	/* reset %wim to one current window */
	/*
	 * save this window to stack - save locals and ins to top 16 words
	 * on the stack. (Since our stack grows down, the top word is %sp
	 * and the bottom will be (%sp + offset).
	 */
	MACH_SAVE_WINDOW_TO_STACK()
	restore				/* move back to trap window */
	mov	%l3, %g3		/* restore global registers */
	mov	%l4, %g4
#ifdef NOTDEF
	/* clear used locals and outs? */
	MACH_CLEAR_WINDOW()
#endif /* NOTDEF */
	/* jump to return from trap routine */
	/* Phooey - use up a clean register */
	set	_MachReturnFromTrap, %l3
	jmp	%l3
	nop


/*
 * ----------------------------------------------------------------------
 *
 * MachWindowUnderflow --
 *
 *	Window underflow trap handler.  It assumes it's called with the %psr
 *	read into %l0.
 *
 *	For an underflow, we tried to retreat to an invalid window and couldn't,
 *	so we trapped.  Trapping advances the current window, so we are 2
 *	windows away from the invalid window.  We want to restore that invalid
 *	window.  First we mark the window behind the invalid window as the
 *	new invalid window, and then we move to the invalid window.  Then we
 *	restore data from the stack into the invalid window.  Then we return
 *	to our trap window, 2 windows away again.  Note that we first mark
 *	the new invalid window and then move to the old invalid window.  If
 *	we did this in the other order, we'd get another window underflow trap.
 *	(This is the opposite order from window overflow.)
 *
 *	For the window to restore, the %sp should be good since it's the %fp
 *	of the window it called.  I hope I can count on this...  (This won't
 *	be true with system and user windows mixed, but this is just for system
 *	code right now.)
 *
 *	The %sp should point to highest (on stack, lowest in memory)
 *	usable word in the current stack frame.  %fp is the %sp of the
 *	caller's stack frame, so the first (highest in memory) usable word
 *	of a current stack frame is (%fp - 4).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The stack shrinks.
 *
 * ----------------------------------------------------------------------
 */
.globl	MachWindowUnderflow
MachWindowUnderflow:
	/*
	 * It should be ok to use locals here - it's a dead window.
	 * Note that this means one cannot do a restore and then a save
	 * and expect that the old locals in the dead window are still the
	 * same!
	 */
	MACH_RETREAT_WIM(%l3, %l4)	/* mark new invalid window */
	restore				/* move to window to restore */
	restore
	/* restore data from stack to window */
	MACH_RESTORE_WINDOW_FROM_STACK()
	/* Move back to trap window with 2 saves.  Clear registers too??? */
	save
#ifdef NOTDEF
	MACH_CLEAR_WINDOW()
#endif /* NOTDEF */
	save
#ifdef NOTDEF
	MACH_CLEAR_WINDOW()
#endif /* NOTDEF */
	/* jump to return from trap routine */
	/* Phooey - use a clean register */
	set	_MachReturnFromTrap, %l3
	jmp	%l3
	nop



/*
 * ----------------------------------------------------------------------
 *
 * MachTrap --
 *
 *	Jump to system trap table.
 *
 *	Use of registers:  %l3 is trap type and then the place to jump to.
 *	%l4 is the address of my trap table to reset the %tbr with.
 *	Note that this code cannot use %l1 or %l2 since that's where the pc
 *	and npc are written on a trap.
 *
 *	The old system %tbr has been stored in %g6.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	_MachTrap
_MachTrap:
	rd	%tbr, %l3
	and	%l3, MACH_TRAP_TYPE_MASK, %l3		/* get trap type */

	cmp	%l3, MACH_WINDOW_OVERFLOW		/* my overflow? */
	be	MachWindowOverflow
	rd	%psr, %l0
	cmp	%l3, MACH_WINDOW_UNDERFLOW		/* my underflow? */
	be	MachWindowUnderflow
	rd	%psr, %l0

							/* no - their stuff */
	add	%l3, %g6, %l3			/* add t.t. to real tbr */
	jmp	%l3			/* jmp (non-pc-rel) to real tbr */
	nop




/*
 * ----------------------------------------------------------------------
 *
 * MachReturnFromTrap --
 *
 *	Restore old %psr from %l0 and re-enable traps.  Then jump to where
 *	we were when we got a trap.
 *
 * Results:
 *	None.
 * * Side effects:
 *	None.
 *
 * ----------------------------------------------------------------------
 */
.globl	_MachReturnFromTrap
_MachReturnFromTrap:
	/* restore psr and re-enable traps. */
#ifdef NOTDEF
	or	%l0, MACH_ENABLE_TRAP_BIT, %l0
#endif /* NOTDEF */
	mov	%l0, %psr
	jmp	%l1
	rett	%l2
	nop
