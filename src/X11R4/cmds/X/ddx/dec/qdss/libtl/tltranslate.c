/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 *  Reset the hardware translation point (index registers)
 *
 *  Written by Brian Kelleher; July 1986
 */

#include	<sys/types.h>
#include	"Ultrix2.0inc.h"
#include	<vaxuba/qduser.h>
#include	"tltemplabels.h"
#include	"tl.h"

tlsettranslate( x, y)
    int x, y;
{
    register unsigned short *p;

    Need_dma(7);

    *p++ = JMPT_SETTRANSLATE;
    *p++ = x & 0x3fff;
    *p++ = y & 0x3fff;
    *p++ = x & 0x3fff;
    *p++ = y & 0x3fff;
    *p++ = x & 0x3fff;
    *p++ = y & 0x3fff;

    Confirm_dma();
}

/*
 * qd level routines can call this, and not know about the
 * SETTRANSLATEPOINT macro.
 */
tlSetTranslateState( x, y)
    int x, y;
{
    SETTRANSLATEPOINT( x, y);
}
