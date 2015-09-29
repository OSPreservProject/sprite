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
#include	"qdgc.h"
#include	"gcstruct.h"

extern	int	Nplanes;
extern	int	Nchannels;
/*
 *  tlputimage
 *
 *  put a rectangle of Z-mode data into the framebuffer.
 */
int
tlputimage( pwin, pgc, x, y, w, h, pcolor)
    WindowPtr		pwin;
    GCPtr		pgc;
    register int	x, y, w, h;
    register unsigned char *pcolor;
{
    register struct adder * adder = Adder;

    RegionPtr pSaveGCclip = QDGC_COMPOSITE_CLIP(pgc);
    register BoxPtr pclip = pSaveGCclip->rects;	/* step through GC clip */
    register int nclip	  = pSaveGCclip->numRects;	/* number of clips */

    INVALID_SHADOW;	/* XXX */
    SETTRANSLATEPOINT( pwin->absCorner.x, pwin->absCorner.y);

    tlptbsetup(x, y, w, h);
    write_ID( adder, LU_FUNCTION_R3, umtable[pgc->alu]);
    adder->rasterop_mode = (DST_WRITE_ENABLE | DST_INDEX_ENABLE | NORMAL);

    for ( ; nclip > 0; nclip--, pclip++)
    {
	adder->x_clip_min = pclip->x1;
	adder->y_clip_min = pclip->y1;
	adder->x_clip_max = pclip->x2;
	adder->y_clip_max = pclip->y2;
#if	NPLANES==24
	write_ID( adder, RED_UPDATE, RED(pgc->planemask));
	write_ID( adder, GREEN_UPDATE, 0x0);
	write_ID( adder, BLUE_UPDATE, 0x0);
	  ptbzblock( x, y, w, h, pcolor+0, ZRED );
	write_ID( adder, RED_UPDATE, 0x0);
	write_ID( adder, GREEN_UPDATE, GREEN(pgc->planemask));
	write_ID( adder, BLUE_UPDATE, 0x0);
	  ptbzblock( x, y, w, h, pcolor+1, ZGREEN);
	write_ID( adder, RED_UPDATE, 0x0);
	write_ID( adder, GREEN_UPDATE, 0x0);
	write_ID( adder, BLUE_UPDATE, BLUE(pgc->planemask));
	  ptbzblock( x, y, w, h, pcolor+2, ZBLUE);
#else	/* NPLANES==8 */
	write_ID( adder, CS_UPDATE_MASK, GREEN(pgc->planemask));
	  ptbzblock( x, y, w, h, pcolor, ZGREEN);
#endif
    }
    poll_status( adder, ADDRESS_COMPLETE );
    dmafxns.enable();
}

/*
 *  put RGB data into the framebuffer.
 */
static
ptbzblock( x, y, w, h, pcolor, zblock)
    register int	x, y, w, h;
    register unsigned char *	pcolor;
    register int	zblock;
{
    register int		width;
    register struct adder * adder = Adder;

    poll_status( adder, ADDRESS_COMPLETE); 
    adder->command = (PTBZ | zblock);
    while (h--)
    {
	for (width = w; width > 0; width--)
	{
	    poll_status( adder, TX_READY);
	    adder->id_data = *pcolor;
	    pcolor += NPLANES/8;
	}
    }
}

