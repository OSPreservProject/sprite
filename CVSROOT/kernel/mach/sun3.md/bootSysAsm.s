|* bootSys.s -
|*
|*     Contains code that is the first executed at boot time.
|*
|* Copyright (C) 1985 Regents of the University of California
|* All rights reserved.
|*

.data
.asciz "$Header$ SPRITE (Berkeley)"
.even
.text

#include "machConst.h"

|
| "Start" is used for the -e option to the loader.  "SpriteStart" is
| used for the prof module, which prepends an underscore to the name of
| global variables and therefore can't find "_start".
|

	.text
	.globl	start
	.globl	_spriteStart
start:
_spriteStart:
	movw	#MACH_SR_HIGHPRIO,sr		| lock out interrupts
|
| The function codes are only used to get to and from MMU space so set them
| permanently here.
|
	movl	#VMMACH_MMU_SPACE, d0
	movc	d0, dfc
	movc	d0, sfc
#ifdef sun2
|
| The kernel has been loaded into the wrong location.  We copy it to the right
| location by copying up 4 Meg worth of pmegs starting at address 10M.
|
	clrl	d0
	movl	#(MACH_KERN_START + VMMACH_SEG_MAP_OFF), a0
loopStart:
	movsb	d0, a0@
	addql	#1, d0
	addl	#VMMACH_SEG_SIZE, a0
	cmpl	#(0x400000 / VMMACH_SEG_SIZE), d0
	bne	loopStart

#endif

#ifdef sun3
|
| The kernel has been loaded into the wrong location.  We copy it to the right
| location by copying up 8 Meg worth of pmegs.  This is done in all contexts.
|

	clrl	d1
contextLoop:
	movsb	d1, VMMACH_CONTEXT_OFF
	clrl	d0
	movl	#(MACH_KERN_START + VMMACH_SEG_MAP_OFF), a0
loopStart:
	movsb	d0, a0@
	addql	#1, d0
	addl	#VMMACH_SEG_SIZE, a0
	cmpl	#(0x800000 / VMMACH_SEG_SIZE), d0
	bne	loopStart

	addql	#1, d1
	cmpl	#VMMACH_NUM_CONTEXTS, d1
	bne 	contextLoop

	clrl	d1
	movsb	d1, VMMACH_CONTEXT_OFF

|
| Enable the floating point processor.
|

        .data
	.globl fpu_present
fpu_present:
	.word 1
fpu_null_state:
	.long   0

	.text
        movsb   VMMACH_SYSTEM_ENABLE_REG, d0    | Read the enable register.
	orb     #MACH_ENABLE_FPP, d0            | Set the fpu enable bit.
	movsb   d0, VMMACH_SYSTEM_ENABLE_REG    | Write the enable register.
	movl    (MACH_EMU1111*4), d1            | Save current inrpt 11 handler.
	movl    #fcatch, (MACH_EMU1111*4)       | Set up to catch a trap.
	movl    sp, a0                          | Save sp, just in case.
	frestore fpu_null_state                 | Set the fpu to the null state.
	bra     1f
fcatch:                                         | Oops, there isn't any fpu.
	movl    a0, sp                          | Restore the stack,
	andb    #~MACH_ENABLE_FPP, d0           | Clear the fpu enable bit.
	movsb   d0, VMMACH_SYSTEM_ENABLE_REG    | Write the enable register.
	movw    #0, fpu_present                 | Clear the fpu_present flag.
1:
	movl    d1, (MACH_EMU1111*4)            | Restore the inrpt 11 handler.
#endif

|
| Force non-PC relative jump to real start of kernel.
|

	jmp	begin:l

|
| Trap to the debugger.  Used when need to trap via the monitor.
|

	.globl _Dbg_Mon
_Dbg_Mon:	
	trap #15

	.globl _edata, _end
begin:
|
| Zero out the bss segment.
|
	movl	#_edata, a0
	movl	#_end, a1
1:
	clrl	a0@
	addql	#4, a0
	cmpl	a0, a1
	bne	1b

	movl	#start,sp		| Set the stack pointer
 	movl	#start,_dbgMaxStackAddr	| Store the top of the stack for the
					|     debugger.
	jsr	_main

| 
| We returned so call Proc_Exit
|

	clrl	sp@-
	jsr	_Proc_Exit
