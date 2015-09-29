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

#include "X.h"		/* required by windowstr.h */

#include "windowstr.h"

#include	<sys/types.h>
#include	"Ultrix2.0inc.h"
#include	<vaxuba/qduser.h>
#include	<vaxuba/qdreg.h>
#include	"tl.h"
#include	"qd.h"
#include	"tltemplabels.h"

extern	int	Nplanes;
extern 	int 	Nchannels;

/*
 *  tlgetspan
 *
 *  written by Brian Kelleher; June 1986
 *  severely hacked by matt.
 *
 *  Get a span of Z-mode data from the framebuffer.
 */
int
tlgetspan( pwin, x, y, w, pcolor)
    WindowPtr		pwin;
    register int	x, y, w;
    register unsigned char *pcolor;
{
    register struct adder *	adder = Adder;	/* must be in a register */

    INVALID_SHADOW;	/* XXX */
    /* no src index'ing */

    /*
     *  Wait for the DMA buffer to empty, and then for the fifo
     *  maintained by the gate array to empty.
     */
    dmafxns.flush ( TRUE);

#if	NPLANES==24
    write_ID( adder, RED_UPDATE, 0xffff);
    write_ID( adder, GREEN_UPDATE, 0xffff);
    write_ID( adder, BLUE_UPDATE, 0xffff);
#else	/* NPLANES == 8 */
    write_ID( adder, CS_UPDATE_MASK, 0xffff);
#endif
    write_ID( adder, DST_OCR_A, EXT_NONE|INT_NONE|NO_ID|NO_WAIT); 
    write_ID( adder, LU_FUNCTION_R3, FULL_SRC_RESOLUTION | LF_ONES);
    write_ID( adder, MASK_1, 0xffff);

    adder->source_1_dy = 1;
    adder->slow_dest_dx = 0;
    adder->slow_dest_dy = 1;
    adder->fast_dest_dx = w;
    adder->fast_dest_dy = 0;
    adder->error_1 = 0;
    adder->error_2 = 0;

    adder->rasterop_mode = (NORMAL);	/* no src_index'ing */

#if	NPLANES==24
    btpzblock( x, y, w, 1, 0xff, pcolor+0, ZRED);
    btpzblock( x, y, w, 1, 0xff, pcolor+1, ZGREEN);
    btpzblock( x, y, w, 1, 0xff, pcolor+2, ZBLUE);
#else	/* NPLANES==8 */
    btpzblock( x, y, w, 1, 0xff, pcolor, ZGREEN);
#endif
    dmafxns.enable();
}

/*
 *  tlgetimage
 *
 *  Get a rectangle of Z-mode data from the framebuffer.
 */
int
tlgetimage( pwin, x, y, w, h, planemask, pcolor)
    WindowPtr		pwin;
    register int	x, y, w, h;
    unsigned long	planemask;
    register unsigned char *pcolor;
{
    register struct adder *	adder = Adder;	/* must be in a register */
    register u_short		*p;

    INVALID_SHADOW;	/* XXX */
    SETTRANSLATEPOINT( pwin->absCorner.x, pwin->absCorner.y);

    /*
     *  Wait for the DMA buffer to empty, and then for the fifo
     *  maintained by the gate array to empty.
     */
    Need_dma(1);
    *p++ = TEMPLATE_DONE;
    Confirm_dma ();
    dmafxns.flush ( TRUE);

#if	NPLANES==24
    write_ID( adder, RED_UPDATE, 0xffff);
    write_ID( adder, GREEN_UPDATE, 0xffff);
    write_ID( adder, BLUE_UPDATE, 0xffff);
#else	/* NPLANES == 8 */
    write_ID( adder, CS_UPDATE_MASK, 0xffff);
#endif
    write_ID( adder, DST_OCR_A, EXT_NONE|INT_NONE|NO_ID|NO_WAIT); 
    write_ID( adder, LU_FUNCTION_R3, FULL_SRC_RESOLUTION | LF_ONES);
    write_ID( adder, MASK_1, 0xffff);

    adder->source_1_dy = 1;
    adder->slow_dest_dx = 0;
    adder->slow_dest_dy = h;
    adder->fast_dest_dx = w;
    adder->fast_dest_dy = 0;
    adder->error_1 = 0;
    adder->error_2 = 0;

    adder->rasterop_mode = (SRC_1_INDEX_ENABLE | NORMAL);

#if	NPLANES==24
    btpzblock( x, y, w, h, RED(planemask), pcolor+0, ZRED);
    btpzblock( x, y, w, h, GREEN(planemask), pcolor+1, ZGREEN);
    btpzblock( x, y, w, h, BLUE(planemask), pcolor+2, ZBLUE);
#else	/* NPLANES==8 */
    btpzblock( x, y, w, h, GREEN(planemask), pcolor, ZGREEN);
#endif
    dmafxns.enable();
}

/*
 *  Get RGB data from the framebuffer.
 */
#define	pixelsize	(NPLANES/8)
static
btpzblock( x, y, w, h, planebyte, pcolor, zblock)
    int				x, y;
    register int		w, h;
    register unsigned char	planebyte;
    register unsigned char *	pcolor;
    int				zblock;
{
    register int		width;
    register struct adder *	adder = Adder;	/* must be in a register */

    adder->source_1_x = x;
    adder->source_1_y = y;
    adder->source_1_dx = 1;
    adder->command = (BTPZ | zblock);
    while (h--)
    {
	for (width = w; width > 0; width--)
	{
	    poll_status( adder, RX_READY);
	    *pcolor = (u_char)adder->id_data & planebyte;
	    pcolor = pcolor + pixelsize;
	}
    }
    poll_status( adder, ADDRESS_COMPLETE); 
}

