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

#include "excAsm.h"
#include "machineConst.h"

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
	movw	#SUN_SR_HIGHPRIO,sr		| lock out interrupts
|
| The function codes are only used to get to and from MMU space so set them
| permanently here.
|
	movl	#VMMACH_MMU_SPACE, d0
	movc	d0, dfc
	movc	d0, sfc
#ifdef SUN2
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

#ifdef SUN3
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
#endif

|
| Force non-PC relative jump to real start of kernel.
|

	jmp	begin:l

| Trap to the debugger.  The two nops make space so that the pc can be stored
| there.  The trap never returns to here, instead returning to the PC given in
| Dbg_MonPC.

	.globl _Dbg_Mon, _dbgMonPC, _dbgTraceLevel
_Dbg_Mon:	
	trap #15
_dbgMonPC:
	nop
	nop
_dbgTraceLevel:
	nop
	nop

begin:
 	movl	#start,sp		| Set the stack pointer
 	movl	#start,_dbgMaxStackAddr	| Store the top of the stack for the
					|     debugger.
	jsr	_main

| 
| We returned so call Proc_Exit
|

	clrl	sp@-
	jsr	_Proc_Exit
