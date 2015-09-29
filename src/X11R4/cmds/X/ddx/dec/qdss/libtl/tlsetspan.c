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
 *  setspan.c
 *
 *  written by Kelleher, july 1986
 *  (adapted from code written by drewry)
 *
 *  Set a span (type determined by srcpixtype) in the framebuffer
 *  to the values given.
 *
 *  NOTES
 *  this will work only if each channel has <8K pixels
 */

#include <sys/types.h>
#include <stdio.h>	/* debug */

#include "X.h"
#include "windowstr.h"
#include "regionstr.h"
#include "pixmap.h"
#include "gcstruct.h"
#include "dixstruct.h"

#include "Xproto.h"
#include "Xprotostr.h"
#include "mi.h"
#include "Xmd.h"

/*
 * driver headers
 */
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdioctl.h>
#include <vaxuba/qdreg.h>

#include	"tl.h"
#include "qd.h"
#include "qdgc.h"
#include "tltemplabels.h"

extern	int	Nchannels;

#define	pixelsize	(1)

tlsetspans(pDraw, pGC, pcolorInit, pPoint, pWidth, n, fSorted)
    DrawablePtr pDraw;
    GCPtr	pGC;
    unsigned char *pcolorInit;
    DDXPointPtr pPoint;
    int *pWidth;
    int n;
    int fSorted;
{
    int		x = pPoint->x;
    int		y = pPoint->y;
    int		width = *pWidth;
    register unsigned short *p;
    register int npix;
#if NPLANES==24
    register int zblock;
#endif
    int		i;
    RegionPtr	pSaveGCclip = QDGC_COMPOSITE_CLIP(pGC);
    int		nclip = REGION_NUM_RECTS(pSaveGCclip);
    register BoxPtr	pclip = REGION_RECTS(pSaveGCclip);
    register unsigned char	*pcolor;

    extern struct DMAreq *DMArequest;
    extern u_short *DMAbuf;

    if ( width == 0)
	return;
    if ( width > MAXDGAWORDS)
    {
	ErrorF( "tlsetspans: width > MAXDGAWORDS\n");
	width = MAXDGAWORDS;
    }

    SETTRANSLATEPOINT(pGC->lastWinOrg.x, pGC->lastWinOrg.y);

    /*
     *  Just set up some state and fall through to code below
     *  for all RGB pixel data.  The state we are interested in is
     *  the number of zblocks, and the zblock identifiers, as well
     *  as the size of pixel data (for bopping through the array).
     *  Z buffer data is handled seperately.
     */

    while (nclip-- > 0) {
	Need_dma(8);
	*p++ = JMPT_SET_MASKED_ALU;
	*p++ = pGC->planemask;
	*p++ = umtable[pGC->alu];
	*p++ = JMPT_SETCLIP;
	/* don't need to mask with 0x3fff, since clipped to screen size */
	*p++ = pclip->x1;
	*p++ = pclip->x2;
	*p++ = pclip->y1;
	*p++ = pclip->y2;
	Confirm_dma();

#if NPLANES==24
	for (i=0, pcolor = pcolorInit; i<Nchannels; i++) {
	    switch (i)
	    {
		case 0:
	    	    zblock = ZRED;
		    break;
		case 1:
	    	    zblock = ZGREEN;
		    break;
		case 2:
	    	    zblock = ZBLUE;
		    break;
	    }
	    /* 	24--all red, all green, all blue; NOT r/g/b,r/g/b,r/g/b,... */
	    Need_dma(11 + width);	/* planes+initptob */
	    *p++ = JMPT_SETRGBPLANEMASK;
	    switch(zblock) {
	      case ZGREEN:
		*p++ = 0x00;
		*p++ = GREEN(pGC->planemask);
		*p++ = 0x00;
		break;
	     case ZRED:
		*p++ = RED(pGC->planemask);
		*p++ = 0x00;
		*p++ = 0x00;
		break;
	     case ZBLUE:
		*p++ = 0x00;
		*p++ = 0x00;
		*p++ = BLUE(pGC->planemask);
		break;
	    }
#else
        {
	    pcolor = pcolorInit;
	    /* 	8--all grey (actually green) */
	    Need_dma(7 + width);	/* planes+initptob */
#endif
	    *p++ = JMPT_INITPTOB;
	    *p++ = x & 0x3fff;
	    *p++ = y & 0x3fff;
	    *p++ = width &0x3fff;
	    *p++ = 1;

#if NPLANES==24
	    *p++ = PTBZ | zblock;
#else
	    *p++ = PTBZ | 0;
#endif
	    /* DGA magic bit pattern for PTB */
	    *p++ = 0x6000 |(0x1fff & -width);
	    npix = width;
	    while (npix--)
		*p++ = *pcolor++;
	    Confirm_dma ();
	}
	pclip++;
    }
}
