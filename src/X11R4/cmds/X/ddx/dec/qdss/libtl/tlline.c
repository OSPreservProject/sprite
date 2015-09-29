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
 *   NOTE: This file is now only included by other files. (Bothner, Aug 89)
 *   Created by djb, May 1986 (adapted from Ray's QDline.c)
 *   edited by drewry, 13 june 1986: added clipping
 *   edited by kelleher, 27 june 1986: added pixeltypes, interp code.
 */

#include <sys/types.h>

#include "X.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "qdgc.h"

#define MININT	0x8000
#define MAXINT	0x7fff
#define NOINTERSECTION( pb1, pb2)  \
	(    (pb1)->x1 >= (pb2)->x2  \
	  || (pb1)->x2 <= (pb2)->x1  \
	  || (pb1)->y1 >= (pb2)->y2  \
	  || (pb1)->y2 <= (pb2)->y1)

/*
 * driver headers
 */
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>

#include "qd.h"

#include "tl.h"
#include "tltemplabels.h"

#define NLINES (req_buf_size/4)
#define DASH_Y (ScreenHeight+64)

/*
 * does the thin lines case only
 */
void
POLYLINES( pWin, pGC, mode, npt, pptInit)
    WindowPtr		pWin;
    GCPtr		pGC;
    int			mode;           /* Origin or Previous */
    int 		npt;            /* number of points */
    register DDXPointPtr pptInit;
{
    register DDXPointPtr abspts;  	/* actually, window-relative */
    register int	ip;		/* index into polygon list */
    register int	ir;		/* index into rectangle list */
    BoxRec		ptbounds;	/* used for trivial reject */
    register BoxPtr	pclip;		/* used for trivial reject */
    BoxPtr		pclipInit;
    int			nclip;		/* used for trivial reject */
    RegionPtr		pdrawreg = QDGC_COMPOSITE_CLIP(pGC);
    register unsigned short *p;
#ifdef DASHED
    int dashPlane; /* mask for plane containing dash pattern */
#endif

    ptbounds.x1	= MAXINT;
    ptbounds.y1	= MAXINT;
    ptbounds.x2	= MININT;
    ptbounds.y2	= MININT;

    if ( mode == CoordModeOrigin)
	abspts = pptInit;
    else	/* CoordModePrevious */
    {
	if ( npt == 0)		/* make sure abspts[0] is valid */
	    return;
	abspts = (DDXPointPtr) ALLOCATE_LOCAL( npt * sizeof( DDXPointRec));
	abspts[ 0].x = pptInit[ 0].x;
	abspts[ 0].y = pptInit[ 0].y;
	for ( ip=1; ip<npt; ip++)
	{
	    abspts[ ip].x = abspts[ ip-1].x + pptInit[ ip].x;
	    abspts[ ip].y = abspts[ ip-1].y + pptInit[ ip].y;
	}
    }

    /*
     * Prune list of clip rectangles by trivial rejection of entire polyline.
     * Note that we have to add one to the right and bottom bounds, because
     * lines are non-zero width.
     */
    for ( ip=0; ip<npt; ip++)
    {
	ptbounds.x1 = min( ptbounds.x1, abspts[ ip].x);
	ptbounds.y1 = min( ptbounds.y1, abspts[ ip].y);
	ptbounds.x2 = max( ptbounds.x2, abspts[ ip].x+1);
	ptbounds.y2 = max( ptbounds.y2, abspts[ ip].y+1);
    }
    /*
     * translate ptbounds to absolute screen coordinates
     */
    ptbounds.x1 += pGC->lastWinOrg.x;
    ptbounds.y1 += pGC->lastWinOrg.y;
    ptbounds.x2 += pGC->lastWinOrg.x;
    ptbounds.y2 += pGC->lastWinOrg.y;

    nclip = 0;
    ir = REGION_NUM_RECTS(pdrawreg);
    pclipInit = pclip =
	(BoxPtr) ALLOCATE_LOCAL( ir * sizeof(BoxRec));
    for (pclip = REGION_RECTS(pdrawreg) ; --ir >= 0; pclip++)
	if ( ! NOINTERSECTION( &ptbounds, pclip))
	    pclipInit[ nclip++] = *pclip;

    /* draw the polyline */

#ifdef DASHED
    dashPlane = InstallDashes(pGC);
#ifdef DEBUG
    if (!dashPlane) abort();
#endif
#endif
    INSTALL_FILLSTYLE(pGC, &pWin->drawable);

    SETTRANSLATEPOINT(pGC->lastWinOrg.x, pGC->lastWinOrg.y);

    for (pclip = pclipInit ; nclip-- > 0; pclip++) {
	register DDXPointRec *pPts = abspts;
	int nlines = npt-1;

	/*
	 *  Break the polyline into reasonable size packets so we
	 *  have enough space in the dma buffer.
	 */
#ifdef DASHED
	Need_dma (16+NCOLORSHORTS);
	if (pGC->lineStyle == LineOnOffDash)
	    *p++ = JMPT_INITFGLINE;
	else
	    *p++ = JMPT_INITFGBGLINE;
	*p++ = dashPlane;
#else
	Need_dma (10+NCOLORSHORTS);	/* per-clip initialization */
#endif
	*p++ = JMPT_SET_MASKED_ALU;
	*p++ = pGC->planemask;
	*p++ = umtable[pGC->alu];
	*p++ = JMPT_SETCLIP;
	*p++ = (pclip->x1) & 0x3fff;
	*p++ = (pclip->x2) & 0x3fff;
	*p++ = (pclip->y1) & 0x3fff;
	*p++ = (pclip->y2) & 0x3fff;
	*p++ = MAC_SETCOLOR;
	SETCOLOR(p, pGC->fgPixel);
#ifdef DASHED
	*p++ = JMPT_INITPATTERNPOLYLINE;
	*p++ = GC_DASH_LENGTH(pGC); /* src_1_dx */
	*p++ = 1; /* src_1_dy; */
	*p++ = 0; /* src_x */
	*p++ = DASH_Y; /* src_y */
#else
	*p++ = JMPT_INITPOLYLINE;
#endif
	Confirm_dma();
	while ( nlines > 0) {
	    register int nlinesThisTime = min(nlines, NLINES);
	    nlines -= NLINES;
	    Need_dma(nlinesThisTime * 4);
	    while (--nlinesThisTime >= 0) {
		*p++ = pPts->x & 0x3fff;
		*p++ = pPts->y & 0x3fff;
		*p++ = ((pPts+1)->x - pPts->x) & 0x3fff;
		*p++ = ((pPts+1)->y - pPts->y) & 0x3fff;
		pPts++;
	    }
	    Confirm_dma();
	}
	/* X11 wants the last point drawn, if not coincident with the first */
	if (pGC->capStyle != CapNotLast
	    && (abspts[0].x != pPts->x || abspts[0].y != pPts->y )) {
	    Need_dma(4);
	    *p++ = pPts->x & 0x3fff;
	    *p++ = pPts->y & 0x3fff;
	    *p++ = 0;
	    *p++ = 1;
	    Confirm_dma();
	}
    }
    Need_dma(4);
    *p++ = JMPT_RESET_FAST_DY_SLOW_DX;
#ifdef DASHED
    *p++ = JMPT_RESETRASTERMODE;
#endif
    *p++ = JMPT_SETMASK;
    *p++ = 0xFFFF;
    Confirm_dma();
    DEALLOCATE_LOCAL(pclipInit);
    if (mode == CoordModePrevious) DEALLOCATE_LOCAL(abspts);
}

void
POLYSEGMENT(pWin, pGC, nseg, pSegs)
    WindowPtr	pWin;
    GCPtr 	pGC;
    int		nseg;
    xSegment	*pSegs;
{
    register unsigned short *p;
    int i;
    BoxRec	ptbounds;	/* used for trivial reject */
    register xSegment	*curSeg;
    RegionPtr	pdrawreg = QDGC_COMPOSITE_CLIP(pGC);
    int		nclip = REGION_NUM_RECTS(pdrawreg);
    register BoxPtr pclip;

#ifdef DASHED
    int dashPlane = InstallDashes(pGC);
#ifdef DEBUG
    if (!dashPlane) abort();
#endif
#endif
    INSTALL_FILLSTYLE(pGC, &pWin->drawable);

    SETTRANSLATEPOINT(pGC->lastWinOrg.x, pGC->lastWinOrg.y);
    ptbounds.x1	= MAXINT;
    ptbounds.y1	= MAXINT;
    ptbounds.x2	= MININT;
    ptbounds.y2	= MININT;

    for (i=nseg, curSeg = pSegs; --i >= 0; curSeg++) {
	ptbounds.x1 = min( ptbounds.x1, curSeg->x1);
	ptbounds.y1 = min( ptbounds.y1, curSeg->y1);
	ptbounds.x2 = max( ptbounds.x2, curSeg->x1+1);
	ptbounds.y2 = max( ptbounds.y2, curSeg->y1+1);
	ptbounds.x1 = min( ptbounds.x1, curSeg->x2);
	ptbounds.y1 = min( ptbounds.y1, curSeg->y2);
	ptbounds.x2 = max( ptbounds.x2, curSeg->x2+1);
	ptbounds.y2 = max( ptbounds.y2, curSeg->y2+1);
    }
    /*
     * translate ptbounds to absolute screen coordinates
     */
    ptbounds.x1 += pGC->lastWinOrg.x;
    ptbounds.y1 += pGC->lastWinOrg.y;
    ptbounds.x2 += pGC->lastWinOrg.x;
    ptbounds.y2 += pGC->lastWinOrg.y;

    for (pclip = REGION_RECTS(pdrawreg); --nclip >= 0; pclip++) {
	int nlines = nseg;
	register int nlinesThisTime;
	curSeg = pSegs;

	if ( NOINTERSECTION( &ptbounds, pclip))
	    continue;

	/*
	 *  Break the polyline into reasonable size packets so we
	 *  have enough space in the dma buffer.
	 */
#ifdef DASHED
	Need_dma (16+NCOLORSHORTS);
	if (pGC->lineStyle == LineOnOffDash)
	    *p++ = JMPT_INITFGLINE;
	else
	    *p++ = JMPT_INITFGBGLINE;
	*p++ = dashPlane;
#else
	Need_dma (10+NCOLORSHORTS);	/* per-clip initialization */
#endif
	*p++ = JMPT_SET_MASKED_ALU;
	*p++ = pGC->planemask;
	*p++ = umtable[pGC->alu];
	*p++ = JMPT_SETCLIP;
	*p++ = pclip->x1;
	*p++ = pclip->x2;
	*p++ = pclip->y1;
	*p++ = pclip->y2;
	*p++ = MAC_SETCOLOR;
	SETCOLOR(p, pGC->fgPixel);
#ifdef DASHED
	*p++ = JMPT_INITPATTERNPOLYLINE;
	*p++ = GC_DASH_LENGTH(pGC); /* src_1_dx */
	*p++ = 1; /* src_1_dy; */
	*p++ = 0; /* src_x */
	*p++ = DASH_Y; /* src_y */
#else
	*p++ = JMPT_INITPOLYLINE;
#endif
	Confirm_dma();
	if (pGC->capStyle == CapNotLast)
	    while (nlines > 0) {
		nlinesThisTime = min(nlines, NLINES);
		nlines -= nlinesThisTime;
		Need_dma(nlinesThisTime * 4);
		for (; --nlinesThisTime >= 0; curSeg++) {
		    *p++ = curSeg->x1 & 0x3fff;
		    *p++ = curSeg->y1 & 0x3fff;
		    *p++ = (curSeg->x2 - curSeg->x1) & 0x3fff;
		    *p++ = (curSeg->y2 - curSeg->y1) & 0x3fff;
		}
		Confirm_dma();
	    }
	else
	    while (nlines > 0) {
		nlinesThisTime = min(nlines, NLINES>>1);
		Need_dma(nlinesThisTime * 8);
		nlines -= nlinesThisTime;
		for (; --nlinesThisTime >= 0; curSeg++) {
		    *p++ = curSeg->x1 & 0x3fff;
		    *p++ = curSeg->y1 & 0x3fff;
		    *p++ = (curSeg->x2 - curSeg->x1) & 0x3fff;
		    *p++ = (curSeg->y2 - curSeg->y1) & 0x3fff;
		    *p++ = curSeg->x2 & 0x3fff;
		    *p++ = curSeg->y2 & 0x3fff;
		    *p++ = 0;
		    *p++ = 1;
		}
		Confirm_dma();
	    }
    }

    Need_dma(4);
    *p++ = JMPT_RESET_FAST_DY_SLOW_DX;
#ifdef DASHED
    *p++ = JMPT_RESETRASTERMODE;
#endif
    *p++ = JMPT_SETMASK;
    *p++ = 0xFFFF;
    Confirm_dma();
}
