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

/* incls-ed on 7-17 */

#include <sys/types.h>

#include "X.h"
#include "gcstruct.h"

/*
 * driver headers
 */
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>


#include "qd.h"
#include "qdgc.h"

#include "tl.h"
#include "tltemplabels.h"

/*
 *  screen to screen move
 */

/*
   the dma buffer has
   JMPT_SETVIPER24
   rop | FULL_SRC_RESOLUTION
   JMPT_INITBITBLT
   srcdx,srcdy
   srcx,srcy,dstx,dsty,w,h
   {clipx_min,clipx_max,clipy_min,clipy_max,}*
   JMPT_BITBLTDONE
srxdx and srcdy are loaded only for the sign bit.
*/

/* must be passed absolute dst{x,y}.  no translation! */
tlbitblt( pGC, dstX, dstY, dstW, dstH, srcX, srcY)
    GCPtr	pGC;
    int		dstX, dstY;
    int		dstW;
    int		dstH;
    int		srcX, srcY;
{
    register RegionPtr	pgcclip = QDGC_COMPOSITE_CLIP(pGC);
    register unsigned short     *p;
    register BoxPtr	prect = REGION_RECTS(pgcclip);
    register int	nrects = REGION_NUM_RECTS(pgcclip);
    int		xoff, yoff;
    int		maxrects = MAXDMAWORDS/4;
    register int	nboxesThisTime;
    register int stride;
#define clip stride

    SETTRANSLATEPOINT(0,0);
    if (nrects == 0)
	return;

    /*
     * The X toolkit passes in outrageously big rectangles to be blitted,
     * using clipping to restrict the blit to a reasonable size.
     * This is slow on the adder, which generates all the clipped-out
     * pixel addresses, so we soft clip the rectangle.
     */
    if ((clip = pgcclip->extents.x1 - dstX) > 0) {
       srcX += clip;
       dstX += clip;
       dstW -= clip;
    }
    if ((clip = dstX+dstW - pgcclip->extents.x2) > 0) {
	dstW -= clip;
    }
    if ((clip = pgcclip->extents.y1 - dstY) > 0) {
       srcY += clip;
       dstY += clip;
       dstH -= clip;
    }
    if ((clip = dstY+dstH - pgcclip->extents.y2) > 0) {
	dstH -= clip;
    }
    yoff = dstY - srcY;
    xoff = dstX - srcX;
    if ((yoff > 0) && (yoff < dstH) && (abs(xoff) < dstW)) {
	/* reverse y scan direction */
	srcY += dstH-1;
	dstY += dstH-1;
	dstH = -dstH;
	prect += nrects-1;
	stride = -sizeof(BoxRec);
    }
    else {
	if ((yoff == 0) && (xoff > 0) && (xoff < dstW)) {
	    /* reverse x scan direction */
	    srcX += dstW-1;
	    dstX += dstW-1;
	    dstW = -dstW;
	}
	stride = sizeof(BoxRec);
    }

    Need_dma(12);	/* base initialization */
    *p++ = JMPT_SET_MASKED_ALU;
    *p++ = pGC->planemask;
    *p++ = umtable[pGC->alu];
    *p++ = JMPT_INITBITBLT;
    *p++ = dstW & 0x3fff;
    *p++ = dstH & 0x3fff;
    *p++ = srcX & 0x3fff;
    *p++ = srcY & 0x3fff;
    *p++ = dstX & 0x3fff;
    *p++ = dstY & 0x3fff;
    *p++ = dstW & 0x3fff;
    *p++ = dstH & 0x3fff;
    Confirm_dma();
    for (   nrects = REGION_NUM_RECTS(pgcclip);
	    nrects>0; ) {
	nboxesThisTime = min( nrects, maxrects);
	nrects -= nboxesThisTime;

	Need_dma( nboxesThisTime<<2);
	while (-- nboxesThisTime >= 0)
	{
	    *p++ = prect->x1;	/* always non-negative */
	    *p++ = prect->x2;	/*   "	*/
	    *p++ = prect->y1;	/*   "	*/
	    *p++ = prect->y2;	/*   "	*/
	    prect = (BoxPtr)((char*)prect + stride);
	}
	Confirm_dma();
    }
}

/*
 * Copy from screen to screen.
 * This interface is (usually) more efficient than tlbitblt.
 * The destination is passed as a *pre-clipped* region.
 * The source is the destination translated by -(xoff,yoff).
 */

tlbltregion( pGC, dstRegion, xoff, yoff)
    GCPtr	pGC; /* only used for planemask and alu */
    register RegionPtr	dstRegion;
    register xoff, yoff; /* delta(dst,src) */
{
    register unsigned short     *p;
    register BoxPtr	prect = REGION_RECTS(dstRegion);
    register int	nrects = REGION_NUM_RECTS(dstRegion);
    int		maxrects = MAXDMAWORDS/6;
    register int	nboxesThisTime;

    SETTRANSLATEPOINT(0,0);
    if (nrects == 0)
	return;

    Need_dma(6);	/* base initialization */
    *p++ = JMPT_SET_MASKED_ALU;
    *p++ = pGC->planemask;
    *p++ = umtable[pGC->alu];
    *p++ = JMPT_INITBITBLTNOCLIP;

/*
 * The fooX0 macros are used when scanning left-to-right.
 * The fooX1 macros are used when scanning right-to-left.
 * The fooY0 macros are used when scanning top-to-bottom.
 * The fooX1 macros are used when scanning bottom-to-top.
 */

#define SRCX0 prect->x1-xoff
#define SRCX1 prect->x2-xoff-1
#define SRCY0 prect->y1-yoff
#define SRCY1 prect->y2-yoff-1
#define DSTX0 prect->x1
#define DSTX1 prect->x2-1
#define DSTY0 prect->y1
#define DSTY1 prect->y2-1
#define SIZX0 prect->x2 - prect->x1
#define SIZY0 prect->y2 - prect->y1
#define SIZX1 (prect->x1 - prect->x2) & 0x3fff
#define SIZY1 (prect->y1 - prect->y2) & 0x3fff

#ifdef __STDC__
#define DO_LOOPBODY(Xdir,Ydir) \
    *p++ = SRC##Xdir; *p++ = SRC##Ydir;\
    *p++ = DST##Xdir; *p++ = DST##Ydir;\
    *p++ = SIZ##Xdir; *p++ = SIZ##Ydir;
#else
#define DO_LOOPBODY(Xdir,Ydir) \
    *p++ = SRC/**/Xdir; *p++ = SRC/**/Ydir;\
    *p++ = DST/**/Xdir; *p++ = DST/**/Ydir;\
    *p++ = SIZ/**/Xdir; *p++ = SIZ/**/Ydir;
#endif
#define DO_BOXES(Xdir,Ydir,INCR) \
    Confirm_dma();\
    for (nrects = REGION_NUM_RECTS(dstRegion); nrects>0; ) {\
	nboxesThisTime = min( nrects, maxrects);\
	nrects -= nboxesThisTime;\
	Need_dma( nboxesThisTime*6);\
	while (-- nboxesThisTime >= 0) { DO_LOOPBODY(Xdir,Ydir); prect INCR;}\
	Confirm_dma(); }

    if (yoff > 0) {
	/* reverse y scan direction */
	*p++ = 0; /* src dx sign */
	*p++ = 0x3fff; /* src dy sign */
	prect += nrects-1;
	DO_BOXES(X0,Y1,--)
    }
    else if (xoff > 0) {
	/* reverse x scan direction */
	*p++ = 0x3fff; /* src dx sign */
	*p++ = 0; /* src dy sign */
	DO_BOXES(X1,Y0,++)
    } else {
	*p++ = 0; /* src dx sign */
	*p++ = 0; /* src dy sign */
	DO_BOXES(X0,Y0,++)
    }
}
