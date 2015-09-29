|* start.s
|*
|*	Initial instructions executed by a boot program loaded by the PROM.
|*	This is derived from 
|*	"@(#)srt0.s	4.15 83/10/12	Copyr 1983 Sun Micro";
|*	The first thing done here is to relocate the boot program if
|*	the boot PROM hasn't loaded the program into the expected place.
|*	After that the stack is set up, BSS area is zero filled, and
|*	then the code jumps to main().  The entry point to transfer to
|*	another program is also defined here.
|*
#ifdef notdef
.data
.asciz "$Header: /sprite/src/boot/scsiDiskBoot/RCS/start.s,v 1.5 89/01/06 08:10:09 brent Exp $ SPRITE (Berkeley)"
.even
#endif not lint
.text

#include "machConst.h"

	.globl	_etext
	.globl	_edata
	.globl	_end
	.globl	_main
	.globl	_Boot_Exit
	.globl	_Boot_Transfer

	.globl	start
start:
#ifdef notdef
	movw	#MACH_SR_HIGHPRIO,sr	| lock out interrupts, just in case
#endif
leax:	lea	pc@(start-(leax+2)),a0	| True current location of "start"
	lea	start:l,a1		| Desired      location of "start"
	cmpl	a0,a1
	jeq	begin			| If the same, just go do it.
	movl	#_edata,d0		| Desired end of program
	subl	a1,d0			| Calculate length, round up.
	lsrl	#2,d0
movc:	movl	a0@+,a1@+		| Move it where it belongs.
	dbra	d0,movc
	jmp	begin:l			| Force non-PCrel jump

begin:
	movl	sp,start-4:l		| Save old stack pointer value
	lea	start-4:l,sp		| Set up new stack below load point
	movl	#_edata,a0		| Zero fill BSS area (and _endData)
clr:
	clrl	a0@+
	cmpl	#_end,a0
	ble	clr
|
| Argc and argv are set to zeros here and the boot main program knows
| to get arguments from the monitor vector.
|
	clrl	sp@-			| argv = 0 for now.
	clrl	sp@-			| argc = 0 for now.
	jsr	_main
	addqw	#8,sp
	jsr	_Exit			| after main() returns, call Exit().
| Just fall thru into _Boot_Exit if exit() ever returns.

_Boot_Exit:
	movl	start-4:l,sp		| Restore caller's stack pointer
	rts				| Return to caller (PROM probably)

| Boot_Transfer --
| Transfer control to a new program, no arguments are set up.

_Boot_Transfer:
	movl	sp@(4),a0		| Address to call
	movl	a0,sp			| Set the stack below the load point
	jra	a0@			| Jump to callee using new stack

