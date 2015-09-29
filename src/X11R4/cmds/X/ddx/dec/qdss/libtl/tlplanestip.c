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
 * Expands a bitmap in the frame buffer to a full depth pixmap elsewhere
 * in the frame buffer.
 * The full depth pixels are either set to
 * foreground/destination (tlPlaneStipple)
 * or foreground/background (tlPlaneCopy).
 *
 * Calling sequence is the same as that of tlbitblt, except for appending
 * of a planemask argument.
 * tlOddSize() depends on this congruence,
 * as it uses a function pointer to point to either of them.
 */
#include "X.h"
#include "gcstruct.h"
#include "windowstr.h"

#include <sys/types.h>
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>

#include "qd.h"
#include "qdgc.h"

#include "tltemplabels.h"
#include "tl.h"

void
PROCNAME( pGC, dstx, dsty, width, height, srcx, srcy, srcplane)
    GCPtr               pGC;
    int                 srcx, srcy;
    int                 width, height;
    int                 dstx, dsty;
    unsigned long       srcplane;
{
    register int	nclip;
    register BoxPtr	pc;
    register u_short	*p;
    register int	xoff, yoff;
    RegionRec		argRegion[1];

#if 0 /* TILED */
?
#else
    SETTRANSLATEPOINT(0,0);	/* this ok?  abs dst{x,y} assumed. */
#endif
    argRegion->extents.x1 = dstx;
    argRegion->extents.x2 = dstx+width;
    argRegion->extents.y1 = dsty;
    argRegion->extents.y2 = dsty+height;
#ifdef X11R4
    argRegion->data = NULL;
#else
    argRegion->size = 1;
    argRegion->numRects = 1;
    argRegion->rects = (BoxPtr) Xalloc(sizeof(BoxRec));
    argRegion->rects[0] = argRegion->extents;
#endif
    miIntersect(argRegion, argRegion, QDGC_COMPOSITE_CLIP(pGC));

    nclip = REGION_NUM_RECTS(argRegion);
    if (nclip == 0) goto done;

#ifdef TILED
    INSTALL_FILLSTYLE(pGC, 0);
#endif
#ifdef TWOCOLOR
    if (pGC->alu == GXcopy) {
	Need_dma(12);
	*p++ = JMPT_SET_MASKED_ALU;
	*p++ = pGC->planemask;
	*p++ = LF_SOURCE|FULL_SRC_RESOLUTION;
	*p++ = JMPT_SETFOREBACKCOLOR;
	*p++ = pGC->fgPixel;
	*p++ = pGC->bgPixel;
    }
    else {
	SetPlaneAlu(pGC);
	Need_dma(6);
    }
    *p++ = JMPT_SETCOLOR;
    *p++ = 0xFF;
    *p++ = JMPT_INIT2COLORBITMAP;
#else /* !TWOCOLOR */
    Need_dma(9);
    *p++ = JMPT_SET_MASKED_ALU;
    *p++ = pGC->planemask;
    *p++ = umtable[pGC->alu];
    *p++ = JMPT_SETCOLOR;
    *p++ = pGC->fgPixel;
    *p++ = JMPT_INIT1COLORBITMAP;
#endif /* !TWOCOLOR */
    *p++ = srcplane;
    *p++ = JMPT_RESETCLIP;
#ifdef TILED
    *p++ = JMPT_TILEBITMAP;
#else
    *p++ = JMPT_COLORBITMAP;
#endif
    Confirm_dma();
    xoff = dstx - srcx;
    yoff = dsty - srcy;
    for ( pc = REGION_RECTS(argRegion); --nclip >= 0; pc++) {
	Need_dma(6);
	*p++ = (pc->x1 - xoff) & 0x3fff;
	*p++ = (pc->y1 - yoff) & 0x3fff;
	*p++ = pc->x1 & 0x3fff;
	*p++ = pc->y1 & 0x3fff;
	*p++ = (pc->x2 - pc->x1) & 0x3fff;
	*p++ = (pc->y2 - pc->y1) & 0x3fff;
	Confirm_dma();
    }
#ifdef TWOCOLOR
    if (pGC->alu == GXcopy) {
	Need_dma(1);
	*p++ = JMPT_RESET_FORE_BACK;
	Confirm_dma();
    }
#else
    Need_dma(2);
    *p++ = JMPT_SETMASK;
    *p++ = 0xFFFF;
    Confirm_dma();
#endif
  done:
#ifdef X11R4
    if (argRegion->data && argRegion->data->size) Xfree(argRegion->data);
#else
    Xfree(argRegion->rects);
#endif
}
