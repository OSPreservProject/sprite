|*
|* devVidSun3.s --
|*
|*	Routines to manipulate video memory on a Sun-3.
|*
|* Copyright 1987 Regents of the University of California
|* All rights reserved.
|*

#ifdef SUN3

    .data
    .asciz "$Header$ SPRITE (Berkeley)"
    .even
    .text

#include "vmSun3Const.h"

|*----------------------------------------------------------------------
|*
|*  Dev_VidEnable --
|*
|*	Enables or disables the video display on a Sun-3.
|*	The Sun-3 video memory is enabled and disabled by manipulating the 3rd
|*	bit (mask=0x8) in the system enable register. The S.E.R. is 
|*	in MMU space.
|*
|*	See Sun-3 Architecture Manual (v2.0, 15 July 1986), p. 15.
|*
|*  Calling format:
|*	Boolean onOff;
|*	status = Dev_VidEnable(onOff);
|*
|*  Results:
|*	SUCCESS	- always returned (because it's a system call).
|*
|*  Side Effects:
|*	The deisply is enabled or disabled.
|*
|*----------------------------------------------------------------------

#define VIDEO_ENABLE_BIT	0x8

    .text
    .globl 	_Dev_VidEnable:
_Dev_VidEnable:
    movl	d2,sp@-			| save d2
    movc	sfc,d1			| Save source function code
    movl	#VM_MMU_SPACE,d0	| Put code for MMU space into d0
    movc	d0,sfc			| Set source function code 
    movsb	VM_SYSTEM_ENABLE_REG,d2	| d2 = copy of system enable reg

    tstl	sp@(8)			| is arg TRUE or FALSE?
    jeq		off
    orb 	#VIDEO_ENABLE_BIT,d2	| On: Set enable video bit
    jmp		done
off:
    andb	#~VIDEO_ENABLE_BIT,d2	| Turn off enable video bit
done:
    movc	d1,sfc			| Restore prev. source function code

    movc	dfc,d1			| Save dest. function code
    movl	#VM_MMU_SPACE,d0	| Put code for MMU space into d0
    movc	d0,dfc			| Set dest. function code 

    movsb	d2, VM_SYSTEM_ENABLE_REG 

    movc	d1,dfc			| Restore prev. dest. function code

    movl	sp@+,d2			| Restore d2
    movl	#0, d0			| Return SUCCESS
    rts

#endif SUN3
