/* devVid.c -
 *
 *     This file contains video routines.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
#include	"machConst.h"
#include	"vmSunConst.h"

.seg	"data"
.asciz "$Header$ SPRITE (Berkeley)"
.align	8
.seg	"text"

#define	VIDEO_ENABLE_BIT	0x8
#define	SUCCESS			0

/*
 * ----------------------------------------------------------------------------
 *
 * Dev_VidEnable --
 *
 *	Enables or disables the video display on a Sun-4.
 *	The Sun-4 video memory is enabled and disabled by manipulating the
 *	video-enable bit in the system enable register.
 *
 *	status = Dev_VidEnable(onOff)
 *		Boolean	onOff;		(true == on)
 *
 * Results:
 *     SUCCESS - always returned (because it's from a system call).
 *
 * Side effects:
 *     The display is enabled or disabled.
 *
 * ----------------------------------------------------------------------------
 */
.globl	_Dev_VidEnable
_Dev_VidEnable:
#ifndef sun4c
	set	VMMACH_SYSTEM_ENABLE_REG, %OUT_TEMP1
	lduba	[%OUT_TEMP1] VMMACH_CONTROL_SPACE, %OUT_TEMP2
	tst	%o0
	be	VidOff
	nop
	or	%OUT_TEMP2, VIDEO_ENABLE_BIT, %OUT_TEMP2
	b	LeaveVidEnable
	nop
VidOff:
	set	~VIDEO_ENABLE_BIT, %o0
	and	%OUT_TEMP2, %o0, %OUT_TEMP2
LeaveVidEnable:
	stba	%OUT_TEMP2, [%OUT_TEMP1] VMMACH_CONTROL_SPACE
	set	SUCCESS, %o0
	retl
	nop
#else
	/*
	 * Address of enable bit in video status register.
	 */
	set	0xffd1c001, %OUT_TEMP1
	ldub	[%OUT_TEMP1], %OUT_TEMP2
	tst	%o0
	be	VidOff
	nop
	or	%OUT_TEMP2, 0x40, %OUT_TEMP2
	b	LeaveVidEnable
	nop
VidOff:
	set	~0x40, %o0
	and	%OUT_TEMP2, %o0, %OUT_TEMP2
LeaveVidEnable:
	stb	%OUT_TEMP2, [%OUT_TEMP1]
	set	SUCCESS, %o0
	retl
	nop
#endif
