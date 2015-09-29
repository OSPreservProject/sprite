|*
|* emuAsm.s --
|*
|*	Assembly code for the sprited emulator.
|*
|* Copyright 1991 Regents of the University of California
|* Permission to use, copy, modify, and distribute this
|* software and its documentation for any purpose and without
|* fee is hereby granted, provided that the above copyright
|* notice appear in all copies.  The University of California
|* makes no representations about the suitability of this
|* software for any purpose.  It is provided "as is" without
|* express or implied warranty.
|*

    .data
    .asciz "$Header: /r3/kupfer/spriteserver/src/lib/c/emulator/sun3.md/RCS/emuAsm.s,v 1.1 91/10/04 12:12:53 kupfer Exp $ SPRITE (Berkeley)"
    .even
    .text


|*
|* ----------------------------------------------------------------------
|*
|* SpriteEmuMach_SaveState --
|*
|*	Save state for the child process of a fork.
|*
|*	SpriteEmuMach_SaveState(stateArray)
|*	    long *stateArray; /* array for saving state for child */
|*
|*	The state is stored as
|*	  stateArray[0]: return address to caller
|*	  stateArray[1..16]: saved registers (a0..a7 and d0..d7)
|*
|* Results:
|*	Returns a non-zero value (to indicate to the caller that it's
|*	the parent process).
|*
|* Side effects:
|*	Saves all the A and D registers in the given array.  The saved
|*	a0 points to the start of the array.
|*
|* ----------------------------------------------------------------------
|*
	.globl _SpriteEmuMach_SaveState
_SpriteEmuMach_SaveState:
	movl	sp@(4), a0		| a0 = &saveArray[0]
	movl	sp@, a0@		| save return address
	moveml	#0xffff, a0@(4)		| save registers
	movel	a0, d0			| set non-zero return value
	rts


|*
|* ----------------------------------------------------------------------
|*
|* SpriteEmuMach_ChildInit --
|*
|*	Initialization for a child process.  The stack pointer
|*	currently points to the state array saved by
|*	SpriteEmuMach_SaveState. 
|*
|* Results:
|*	Returns 0.
|*
|* Side effects:
|*	Does Mach and emulation library initialization.  Restores the
|*	A and D registers from the state array.
|*
|* ----------------------------------------------------------------------
|*
	.globl _SpriteEmu_Init
	.globl _SpriteEmuMach_ChildInit
_SpriteEmuMach_ChildInit:
|*
|* Restore the real stack pointer and put the return address at the
|* top of the stack.  This lets us call SpriteEmu_Init safely.
|*
	moveml	sp@(4), #0xffff		| restore registers from save area
	movl	a0@, a1			| get SaveState's return address 
	movl	a1, sp@			| top-of-stack gets the return addr.
	jsr	_SpriteEmu_Init
|*
|* The stack is already set up for us to return, so do it.
|*
	movel	#0, d0			| set return value
	rts
