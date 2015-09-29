/*-
 * spriteGC.c --
 *	Functions to support the meddling with GC's we do to preserve
 *	the software cursor...
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
	"$Header: /mic/X11R3/src/cmds/Xsp/ddx/sprite/RCS/spriteGC.c,v 1.9 89/11/18 20:57:24 tve Exp $ SPRITE (Berkeley)";
#endif lint

#include    "spriteddx.h"

#include    "Xmd.h"
#include    "extnsionst.h"
#include    "regionstr.h"
#include    "windowstr.h"
#include    "fontstruct.h"
#include    "dixfontstr.h"
#include    "../mi/mifpoly.h"		/* for SppPointPtr */
#ifdef foobarsun4
#include    "alloca.h"
#include    "stdio.h"
#endif

/*
 * Overlap BoxPtr and Box elements
 */
#define BOX_OVERLAP(pCbox,X1,Y1,X2,Y2) \
 	(((pCbox)->x1 <= (X2)) && ((X1) <= (pCbox)->x2) && \
	 ((pCbox)->y1 <= (Y2)) && ((Y1) <= (pCbox)->y2))

/*
 * Overlap BoxPtr, origins, and rectangle
 */
#define ORG_OVERLAP(pCbox,xorg,yorg,x,y,w,h) \
    BOX_OVERLAP((pCbox),(x)+(xorg),(y)+(yorg),(x)+(xorg)+(w),(y)+(yorg)+(h))

/*
 * Overlap BoxPtr, origins and RectPtr
 */
#define ORGRECT_OVERLAP(pCbox,xorg,yorg,pRect) \
    ORG_OVERLAP((pCbox),(xorg),(yorg),(pRect)->x,(pRect)->y,(pRect)->width, \
		(pRect)->height)
/*
 * Overlap BoxPtr and horizontal span
 */
#define SPN_OVERLAP(pCbox,y,x,w) BOX_OVERLAP((pCbox),(x),(y),(x)+(w),(y))

/*
 * See if should check for cursor overlap. Drawable must be a window,
 * be realized and on the same screen as the cursor. We don't try and do
 * any region checking because it's slow and it leaves mouse droppings when
 * doing a newiconify in uwm and you're moving the grid over a child of
 * the RootWindow...
 */
#define Check(pDraw) \
 	((((DrawablePtr)(pDraw))->type == DRAWABLE_WINDOW) && \
	 ((WindowPtr)(pDraw))->viewable && \
	 spriteCursorLoc ((((DrawablePtr)(pDraw))->pScreen), &cursorBox))

/*-
 *-----------------------------------------------------------------------
 * spriteSaveCursorBox --
 *	Given an array of points, figure out the bounding box for the
 *	series and remove the cursor if it overlaps that box.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteSaveCursorBox (xorg, yorg, mode, lw, pPts, nPts, pCursorBox)
    register int  	    xorg;   	    /* X-Origin for points */
    register int  	    yorg;   	    /* Y-Origin for points */
    int	    	  	    mode;   	    /* CoordModeOrigin or
					     * CoordModePrevious */
    int			    lw;		    /* line width */
    register DDXPointPtr    pPts;   	    /* Array of points */
    int	    	  	    nPts;   	    /* Number of points */
    register BoxPtr 	    pCursorBox;	    /* Bounding box for cursor */
{
    register int  	    minx,   	    /* Lowest X coordinate of all pts*/
			    miny,   	    /* Lowest Y coordinate */
			    maxx,   	    /* Highest X coordinate */
			    maxy;   	    /* Highest Y coordinate */

    minx = maxx = pPts->x + xorg;
    miny = maxy = pPts->y + yorg;

    pPts++;
    nPts--;

    if (mode == CoordModeOrigin) {
	while (nPts--) {
	    minx = min(minx, pPts->x + xorg);
	    maxx = max(maxx, pPts->x + xorg);
	    miny = min(miny, pPts->y + yorg);
	    maxy = max(maxy, pPts->y + yorg);
	    pPts++;
	}
    } else {
	xorg = minx;
	yorg = miny;
	while (nPts--) {
	    minx = min(minx, pPts->x + xorg);
	    maxx = max(maxx, pPts->x + xorg);
	    miny = min(miny, pPts->y + yorg);
	    maxy = max(maxy, pPts->y + yorg);
	    xorg += pPts->x;
	    yorg += pPts->y;
	    pPts++;
	}
    }
    if (BOX_OVERLAP(pCursorBox,minx-lw,miny-lw,maxx+lw,maxy+lw)) {
	spriteRemoveCursor();
    }
}
		       
/*-
 *-----------------------------------------------------------------------
 * spriteFillSpans --
 *	Remove the cursor if any of the spans overlaps the area covered
 *	by the cursor. This assumes the points have been translated
 *	already, though perhaps it shouldn't...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteFillSpans(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec	  cursorBox;

    if (Check(pDrawable)) {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;
	
	for (pts = pptInit, widths = pwidthInit, nPts = nInit;
	     nPts--;
	     pts++, widths++) {
		 if (SPN_OVERLAP(&cursorBox,pts->y,pts->x,*widths)) {
		     spriteRemoveCursor();
		     break;
		 }
	}
    }

    (* pShadowGC->FillSpans)(pDrawable, pShadowGC, nInit, pptInit,
			     pwidthInit, fSorted);
}

/*-
 *-----------------------------------------------------------------------
 * spriteSetSpans --
 *	Remove the cursor if any of the horizontal segments overlaps
 *	the area covered by the cursor. This also assumes the spans
 *	have been translated from the window's coordinates to the
 *	screen's.
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteSetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    int			*psrc;
    register DDXPointPtr ppt;
    int			*pwidth;
    int			nspans;
    int			fSorted;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec	  cursorBox;

    if (Check(pDrawable)) {
	register DDXPointPtr    pts;
	register int    	*widths;
	register int    	nPts;
	
	for (pts = ppt, widths = pwidth, nPts = nspans;
	     nPts--;
	     pts++, widths++) {
		 if (SPN_OVERLAP(&cursorBox,pts->y,pts->x,*widths)) {
		     spriteRemoveCursor();
		     break;
		 }
	}
    }

    (* pShadowGC->SetSpans) (pDrawable, pShadowGC, psrc, ppt, pwidth,
			     nspans, fSorted);
}

/*-
 *-----------------------------------------------------------------------
 * spriteGetSpans --
 *	Remove the cursor if any of the desired spans overlaps it.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
unsigned int *
spriteGetSpans(pDrawable, wMax, ppt, pwidth, nspans)
    DrawablePtr		pDrawable;	/* drawable from which to get bits */
    int			wMax;		/* largest value of all *pwidths */
    register DDXPointPtr ppt;		/* points to start copying from */
    int			*pwidth;	/* list of number of bits to copy */
    int			nspans;		/* number of scanlines to copy */
{
    BoxRec	  cursorBox;

    if (Check(pDrawable)) {
	register DDXPointPtr    pts;
	register int    	    *widths;
	register int    	    nPts;
	register int    	    xorg,
				    yorg;
	
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
	
	for (pts = ppt, widths = pwidth, nPts = nspans;
	     nPts--;
	     pts++, widths++) {
		 if (SPN_OVERLAP(&cursorBox,pts->y+yorg,
				 pts->x+xorg,*widths)) {
				     spriteRemoveCursor();
				     break;
		 }
	}
    }

    /*
     * XXX: Because we have no way to get at the GC used to call us,
     * we must rely on the GetSpans vector never changing and stick it
     * in the fbFd structure. Gross.
     */
    return (* spriteFbs[pDrawable->pScreen->myNum].GetSpans)(pDrawable, wMax,
							  ppt, pwidth, nspans);
}

/*-
 *-----------------------------------------------------------------------
 * spritePutImage --
 *	Remove the cursor if it is in the way of the image to be
 *	put down...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePutImage(pDst, pGC, depth, x, y, w, h, leftPad, format, pBits)
    DrawablePtr	  pDst;
    GCPtr   	  pGC;
    int		  depth;
    int	    	  x;
    int	    	  y;
    int	    	  w;
    int	    	  h;
    int	    	  format;
    char    	  *pBits;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec	  cursorBox;

    if (Check(pDst)) {
	register WindowPtr pWin = (WindowPtr)pDst;

	if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			x,y,w,h)) {
			    spriteRemoveCursor();
	}
    }

    (* pShadowGC->PutImage) (pDst, pShadowGC, depth, x, y, w, h,
			     leftPad, format, pBits);
}

/*-
 *-----------------------------------------------------------------------
 * spriteGetImage --
 *	Remove the cursor if it overlaps the image to be gotten.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteGetImage (pSrc, x, y, w, h, format, planeMask, pBits)
    DrawablePtr	  pSrc;
    int	    	  x;
    int	    	  y;
    int	    	  w;
    int	    	  h;
    unsigned int  format;
    unsigned int  planeMask;
    int	    	  *pBits;
{
    BoxRec	  cursorBox;
    if (Check(pSrc)) {
	register WindowPtr	pWin = (WindowPtr)pSrc;
	
	if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			x,y,w,h)) {
			    spriteRemoveCursor();
	}
    }

    (* spriteFbs[pSrc->pScreen->myNum].GetImage) (pSrc, x, y, w, h, format,
					       planeMask, pBits);
}

/*-
 *-----------------------------------------------------------------------
 * spriteCopyArea --
 *	Remove the cursor if it overlaps either the source or destination
 *	drawables, then call the screen-specific CopyArea routine.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
RegionPtr
spriteCopyArea (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty)
    DrawablePtr	  pSrc;
    DrawablePtr	  pDst;
    GCPtr   	  pGC;
    int	    	  srcx;
    int	    	  srcy;
    int	    	  w;
    int	    	  h;
    int	    	  dstx;
    int	    	  dsty;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec	  cursorBox;
    register WindowPtr	pWin;
    int	    	  out = FALSE;

    if (Check(pSrc)) {
	pWin = (WindowPtr)pSrc;

	if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			srcx, srcy, w, h)) {
			    spriteRemoveCursor();
			    out = TRUE;
	}
    }

    if (!out && Check(pDst)) {
	pWin = (WindowPtr)pDst;
	    
	if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			dstx, dsty, w, h)) {
			    spriteRemoveCursor();
	}
    }

    return (* pShadowGC->CopyArea) (pSrc, pDst, pShadowGC, srcx, srcy,
			     w, h, dstx, dsty);
}

/*-
 *-----------------------------------------------------------------------
 * spriteCopyPlane --
 *	Remove the cursor as necessary and call the screen-specific
 *	CopyPlane function.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
RegionPtr
spriteCopyPlane (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty, plane)
    DrawablePtr	  pSrc;
    DrawablePtr	  pDst;
    register GC   *pGC;
    int     	  srcx,
		  srcy;
    int     	  w,
		  h;
    int     	  dstx,
		  dsty;
    unsigned int  plane;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec	  cursorBox;
    register WindowPtr	pWin;
    int	    	  out = FALSE;

    if (Check(pSrc)){
	pWin = (WindowPtr)pSrc;

	if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			srcx, srcy, w, h)) {
			    spriteRemoveCursor();
			    out = TRUE;
	}
    }

    if (!out && Check(pDst)) {
	pWin = (WindowPtr)pDst;
	
	if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			dstx, dsty, w, h)) {
			    spriteRemoveCursor();
	}
    }


    return (* pShadowGC->CopyPlane) (pSrc, pDst, pShadowGC, srcx, srcy, w, h,
			      dstx, dsty, plane);
}

/*-
 *-----------------------------------------------------------------------
 * spritePolyPoint --
 *	See if any of the points lies within the area covered by the
 *	cursor and remove the cursor if one does. Then put the points
 *	down.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePolyPoint (pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		mode;		/* Origin or Previous */
    int		npt;
    xPoint 	*pptInit;
{
    register GCPtr 	pShadowGC = (GCPtr) pGC->devPriv;
    register xPoint 	*pts;
    register int  	nPts;
    register int  	xorg;
    register int  	yorg;
    BoxRec  	  	cursorBox;

    if (Check(pDrawable)){
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
	
	if (mode == CoordModeOrigin) {
	    for (pts = pptInit, nPts = npt; nPts--; pts++) {
		if (ORG_OVERLAP(&cursorBox,xorg,yorg,pts->x,pts->y,0,0)){
		    spriteRemoveCursor();
		    break;
		}
	    }
	} else {
	    for (pts = pptInit, nPts = npt; nPts--; pts++) {
		if (ORG_OVERLAP(&cursorBox,xorg,yorg,pts->x,pts->y,0,0)){
		    spriteRemoveCursor();
		    break;
		} else {
		    xorg += pts->x;
		    yorg += pts->y;
		}
	    }
	}
    }

    (* pShadowGC->PolyPoint) (pDrawable, pShadowGC, mode, npt, pptInit);
}

/*-
 *-----------------------------------------------------------------------
 * spritePolylines --
 *	Find the bounding box of the lines and remove the cursor if
 *	the box overlaps the area covered by the cursor. Then call
 *	the screen's Polylines function to draw the lines themselves.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePolylines (pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr	  pDrawable;
    GCPtr   	  pGC;
    int	    	  mode;
    int	    	  npt;
    DDXPointPtr	  pptInit;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  cursorBox;

    if (Check(pDrawable)){
	spriteSaveCursorBox(((WindowPtr)pDrawable)->absCorner.x,
			    ((WindowPtr)pDrawable)->absCorner.y,
			    mode,
			    pShadowGC->lineWidth,
			    pptInit,
			    npt,
			    &cursorBox);
    }
    (*pShadowGC->Polylines) (pDrawable, pShadowGC, mode, npt, pptInit);
}

/*-
 *-----------------------------------------------------------------------
 * spritePolySegment --
 *	Treat each segment as a box and remove the cursor if any box
 *	overlaps the cursor's area. Then draw the segments. Note that
 *	the endpoints of the segments are in no way guaranteed to be
 *	in the right order, so we find the bounding box of the segment
 *	in two comparisons and use that to figure things out.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePolySegment(pDraw, pGC, nseg, pSegs)
    DrawablePtr pDraw;
    GCPtr 	pGC;
    int		nseg;
    xSegment	*pSegs;
{
    register GCPtr 	pShadowGC = (GCPtr) pGC->devPriv;
    register xSegment	*pSeg;
    register int  	nSeg;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;
    Bool    	  	nuke = FALSE;

    if (Check(pDraw)){
	xorg = ((WindowPtr)pDraw)->absCorner.x;
	yorg = ((WindowPtr)pDraw)->absCorner.y;
	
	for (nSeg = nseg, pSeg = pSegs; nSeg--; pSeg++) {
	    if (pSeg->x1 < pSeg->x2) {
		if (pSeg->y1 < pSeg->y2) {
		    nuke = BOX_OVERLAP(&cursorBox,
				       pSeg->x1+xorg,pSeg->y1+yorg,
				       pSeg->x2+xorg,pSeg->y2+yorg);
		} else {
		    nuke = BOX_OVERLAP(&cursorBox,
				       pSeg->x1+xorg,pSeg->y2+yorg,
				       pSeg->x2+xorg,pSeg->y1+yorg);
		}
	    } else if (pSeg->y1 < pSeg->y2) {
		nuke = BOX_OVERLAP(&cursorBox,
				   pSeg->x2+xorg,pSeg->y1+yorg,
				   pSeg->x1+xorg,pSeg->y2+yorg);
	    } else {
		nuke = BOX_OVERLAP(&cursorBox,
				   pSeg->x2+xorg,pSeg->y2+yorg,
				   pSeg->x1+xorg,pSeg->y1+yorg);
	    }
	    if (nuke) {
		spriteRemoveCursor();
		break;
	    }
	}
    }

    (* pShadowGC->PolySegment) (pDraw, pShadowGC, nseg, pSegs);
}

/*-
 *-----------------------------------------------------------------------
 * spritePolyRectangle --
 *	Remove the cursor if it overlaps any of the rectangles to be
 *	drawn, then draw them.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePolyRectangle(pDraw, pGC, nrects, pRects)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		nrects;
    xRectangle	*pRects;
{
    register GCPtr 	pShadowGC = (GCPtr) pGC->devPriv;
    register xRectangle	*pRect;
    register int  	nRect;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;

    if (Check(pDraw)) {
	xorg = ((WindowPtr)pDraw)->absCorner.x;
	yorg = ((WindowPtr)pDraw)->absCorner.y;
	
	for (nRect = nrects, pRect = pRects; nRect--; pRect++) {
	    if (ORGRECT_OVERLAP(&cursorBox,xorg,yorg,pRect)){
		spriteRemoveCursor();
		break;
	    }
	}
    }

    (* pShadowGC->PolyRectangle) (pDraw, pShadowGC, nrects, pRects);
}

/*-
 *-----------------------------------------------------------------------
 * spritePolyArc --
 *	Using the bounding rectangle of each arc, remove the cursor
 *	if it overlaps any arc, then draw all the arcs.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePolyArc(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    register GCPtr 	pShadowGC = (GCPtr) pGC->devPriv;
    register xArc	*pArc;
    register int  	nArc;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;

    if (Check(pDraw)) {
	xorg = ((WindowPtr)pDraw)->absCorner.x;
	yorg = ((WindowPtr)pDraw)->absCorner.y;
	
	for (nArc = narcs, pArc = parcs; nArc--; pArc++) {
	    if (ORGRECT_OVERLAP(&cursorBox,xorg,yorg,pArc)){
		spriteRemoveCursor();
		break;
	    }
	}
    }

    (* pShadowGC->PolyArc) (pDraw, pShadowGC, narcs, parcs);
}

/*-
 *-----------------------------------------------------------------------
 * spriteFillPolygon --
 *	Find the bounding box of the polygon to fill and remove the
 *	cursor if it overlaps this box...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteFillPolygon(pDraw, pGC, shape, mode, count, pPts)
    DrawablePtr		pDraw;
    register GCPtr	pGC;
    int			shape, mode;
    register int	count;
    DDXPointPtr		pPts;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  	cursorBox;

    if (Check(pDraw)) {
	spriteSaveCursorBox(((WindowPtr)pDraw)->absCorner.x,
			    ((WindowPtr)pDraw)->absCorner.y,
			    mode,
			    0,
			    pPts,
			    count,
			    &cursorBox);
    }

    (* pShadowGC->FillPolygon) (pDraw, pShadowGC, shape, mode, count, pPts);
}

/*-
 *-----------------------------------------------------------------------
 * spritePolyFillRect --
 *	Remove the cursor if it overlaps any of the filled rectangles
 *	to be drawn by the output routines.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    register xRectangle	*pRect;
    register int  	nRect;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;

    if (Check(pDrawable)) {
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
	
	for (nRect = nrectFill, pRect = prectInit; nRect--; pRect++) {
	    if (ORGRECT_OVERLAP(&cursorBox,xorg,yorg,pRect)){
		spriteRemoveCursor();
		break;
	    }
	}
    }

    (* pShadowGC->PolyFillRect) (pDrawable, pShadowGC, nrectFill, prectInit);
}

/*-
 *-----------------------------------------------------------------------
 * spritePolyFillArc --
 *	See if the cursor overlaps any of the bounding boxes for the
 *	filled arc and remove it if it does.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePolyFillArc(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    register xArc	*pArc;
    register int  	nArc;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;

    if (Check(pDraw)) {
	xorg = ((WindowPtr)pDraw)->absCorner.x;
	yorg = ((WindowPtr)pDraw)->absCorner.y;
	
	for (nArc = narcs, pArc = parcs; nArc--; pArc++) {
	    if (ORGRECT_OVERLAP(&cursorBox,xorg,yorg,pArc)){
		spriteRemoveCursor();
		break;
	    }
	}
    }

    (* pShadowGC->PolyFillArc) (pDraw, pShadowGC, narcs, parcs);
}

/*-
 *-----------------------------------------------------------------------
 * spriteText --
 *	Find the extent of a text operation and remove the cursor if they
 *	overlap. pDraw is assumed to be a window.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
spriteText(pDraw, pGC, x, y, count, chars, fontEncoding, drawFunc)
    DrawablePtr   pDraw;
    GCPtr	  pGC;
    int		  x,
		  y;
    int		  count;
    char 	  *chars;
    FontEncoding  fontEncoding;
    void    	  (*drawFunc)();
{
    CharInfoPtr   *charinfo;
    unsigned int  n,
		  w;
    register int  xorg,
		  yorg;
    ExtentInfoRec extents;
    BoxRec  	  cursorBox;

    charinfo = (CharInfoPtr *)ALLOCATE_LOCAL (count * sizeof(CharInfoPtr));
    if (charinfo == (CharInfoPtr *)NULL) {
	return x;
    }

    GetGlyphs(pGC->font, count, chars, fontEncoding, &n, charinfo);

    if (n != 0) {
	/*
	 * Because we need to know the width of the operation in order to
	 * return the new offset, we always have to do this. Let's hope
	 * QueryGlyphExtents has been optimized...
	 */
	QueryGlyphExtents(pGC->font, charinfo, n, &extents);
	w = extents.overallWidth;
	if (spriteCursorLoc (pDraw->pScreen, &cursorBox)) {
	    xorg = ((WindowPtr)pDraw)->absCorner.x;
	    yorg = ((WindowPtr)pDraw)->absCorner.y;
    
	    if (BOX_OVERLAP(&cursorBox,
			    x + xorg + extents.overallLeft,
			    y + yorg - extents.overallAscent,
			    x + xorg + extents.overallRight,
			    y + yorg + extents.overallDescent)) {
				spriteRemoveCursor();
	    }
	}
        (* drawFunc)(pDraw, pGC, x, y, n, charinfo, pGC->font->pGlyphs);
    } else {
	w = 0;
    }
    
    DEALLOCATE_LOCAL(charinfo);
    return x+w;
}

/*-
 *-----------------------------------------------------------------------
 * spritePolyText8 --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
spritePolyText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int 	count;
    char	*chars;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;

    if ((pDraw->type == DRAWABLE_WINDOW) && ((WindowPtr)pDraw)->viewable) {
	return spriteText (pDraw, pShadowGC, x, y, count, chars, Linear8Bit,
			pShadowGC->PolyGlyphBlt);
    } else {
	return (* pShadowGC->PolyText8)(pDraw, pShadowGC, x, y, count, chars);
    }
}

/*-
 *-----------------------------------------------------------------------
 * spritePolyText16 --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
spritePolyText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;

    if ((pDraw->type == DRAWABLE_WINDOW) && ((WindowPtr)pDraw)->viewable) {
	return spriteText (pDraw, pShadowGC, x, y, count, chars,
			(pShadowGC->font->pFI->lastRow == 0 ?
			 Linear16Bit : TwoD16Bit),
			pShadowGC->PolyGlyphBlt);
    } else {
	return (* pShadowGC->PolyText16) (pDraw, pShadowGC, x, y,
					  count, chars);
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteImageText8 --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteImageText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char	*chars;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;

    if ((pDraw->type == DRAWABLE_WINDOW) && ((WindowPtr)pDraw)->viewable) {
	(void) spriteText (pDraw, pShadowGC, x, y, count, chars,
			Linear8Bit, pShadowGC->ImageGlyphBlt);
    } else {
	(* pShadowGC->ImageText8) (pDraw, pShadowGC, x, y, count, chars);
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteImageText16 --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteImageText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;

    if ((pDraw->type == DRAWABLE_WINDOW) && ((WindowPtr)pDraw)->viewable) {
	(void) spriteText (pDraw, pShadowGC, x, y, count, chars,
			   (pShadowGC->font->pFI->lastRow == 0 ?
			    Linear16Bit : TwoD16Bit),
			   pShadowGC->ImageGlyphBlt);
    } else {
	(* pShadowGC->ImageText16) (pDraw, pShadowGC, x, y, count, chars);
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteImageGlyphBlt --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    pointer 	pglyphBase;	/* start of array of glyphs */
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  	cursorBox;
    ExtentInfoRec 	extents;
    register int  	xorg,
			yorg;

    if (Check(pDrawable)) {
	QueryGlyphExtents (pGC->font, ppci, nglyph, &extents);
	xorg = ((WindowPtr)pDrawable)->absCorner.x + x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y + y;
	if (BOX_OVERLAP(&cursorBox,xorg+extents.overallLeft,
			yorg+extents.overallAscent,
			xorg+extents.overallRight,
			yorg+extents.overallDescent)) {
			    spriteRemoveCursor();
	}
    }

    (* pShadowGC->ImageGlyphBlt) (pDrawable, pShadowGC, x, y, nglyph,
				  ppci, pglyphBase);
}

/*-
 *-----------------------------------------------------------------------
 * spritePolyGlyphBlt --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    char 	*pglyphBase;	/* start of array of glyphs */
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  	cursorBox;
    ExtentInfoRec 	extents;
    register int  	xorg,
			yorg;

    if (Check(pDrawable)) {
	QueryGlyphExtents (pGC->font, ppci, nglyph, &extents);
	xorg = ((WindowPtr)pDrawable)->absCorner.x + x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y + y;
	if (BOX_OVERLAP(&cursorBox,xorg+extents.overallLeft,
			yorg+extents.overallAscent,
			xorg+extents.overallRight,
			yorg+extents.overallDescent)){
			    spriteRemoveCursor();
	}
    }

    (* pShadowGC->PolyGlyphBlt) (pDrawable, pShadowGC, x, y,
				nglyph, ppci, pglyphBase);
}

/*-
 *-----------------------------------------------------------------------
 * spritePushPixels --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spritePushPixels(pGC, pBitMap, pDst, w, h, x, y)
    GCPtr	pGC;
    PixmapPtr	pBitMap;
    DrawablePtr pDst;
    int		w, h, x, y;
{

    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  	cursorBox;
    register int  	xorg,
			yorg;

    if (Check(pDst)) {
	xorg = ((WindowPtr)pDst)->absCorner.x + x;
	yorg = ((WindowPtr)pDst)->absCorner.y + y;
	
	if (BOX_OVERLAP(&cursorBox,xorg,yorg,xorg+w,yorg+h)){
	    spriteRemoveCursor();
	}
    }

    (* pShadowGC->PushPixels) (pShadowGC, pBitMap, pDst, w, h, x, y);
}

/*-
 *-----------------------------------------------------------------------
 * spriteLineHelper --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
spriteLineHelper (pDraw, pGC, caps, npt, pPts, xOrg, yOrg)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		caps;
    int		npt;
    SppPointPtr pPts;
    int		xOrg, yOrg;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    (* pShadowGC->LineHelper) (pDraw, pGC, caps, npt, pPts, xOrg, yOrg);
}

/*-
 *-----------------------------------------------------------------------
 * spriteChangeClip --
 *	Front end for changing the clip in the GC. Just passes the command
 *	on through the shadow GC.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	???
 *
 *-----------------------------------------------------------------------
 */
void
spriteChangeClip (pGC, type, pValue, numRects)
    GCPtr   	  pGC;
    int	    	  type;
    pointer 	  pValue;
    int	    	  numRects;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    (* pShadowGC->ChangeClip) (pShadowGC, type, pValue, numRects);
    /*
     *	Now copy the clip info back from the shadow to the real
     *	GC so that if we use it as the source for a CopyGC,
     *	the clip info will get copied along with everything
     *	else.
     */
    pGC->clientClip = pShadowGC->clientClip;
    pGC->clientClipType = pShadowGC->clientClipType;
}

/*-
 *-----------------------------------------------------------------------
 * spriteDestroyClip --
 *	Ditto for destroying the clipping region of the GC.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	???
 *
 *-----------------------------------------------------------------------
 */
void
spriteDestroyClip (pGC)
    GCPtr   pGC;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    (* pShadowGC->DestroyClip) (pShadowGC);
    /* presumably this is NULL,  or junk,  or something.... */
    pGC->clientClip = pShadowGC->clientClip;
    pGC->clientClipType = pShadowGC->clientClipType;

}

/*-
 *-----------------------------------------------------------------------
 * spriteCopyClip --
 *	Ditto for copying the clipping region of the GC. Note that it
 *	is possible for us to be called to copy the clip from a shadow
 *	GC to a top-level (one of our) GC (the backing-store scheme in
 *	MI will do such a thing), so we must be careful.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	???
 *
 *-----------------------------------------------------------------------
 */
void
spriteCopyClip (pgcDst, pgcSrc)
    GCPtr   pgcDst, pgcSrc;
{
    register GCPtr pShadowSrcGC = (GCPtr) pgcSrc->devPriv;
    register GCPtr pShadowDstGC = (GCPtr) pgcDst->devPriv;

    if ((void (*)())pgcSrc->FillSpans != (void (*)())spriteFillSpans) {
	/*
	 * The source is a shadow GC in its own right, so don't try to
	 * reference through the devPriv field...
	 */
	pShadowSrcGC = pgcSrc;
    }
    
    (* pShadowDstGC->CopyClip) (pShadowDstGC, pShadowSrcGC);
    pgcDst->clientClip = pShadowDstGC->clientClip;
    pgcDst->clientClipType = pShadowDstGC->clientClipType;
}

/*-
 *-----------------------------------------------------------------------
 * spriteDestroyGC --
 *	Function called when a GC is being freed. Simply unlinks and frees
 *	the GCInterest structure.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The GCInterest structure is removed from the chain but its own
 *	links are untouched (so FreeGC has something to follow...)
 *
 *-----------------------------------------------------------------------
 */
void
spriteDestroyGC (pGC, pGCI)
    GCPtr	   pGC;	/* GC pGCI is attached to */
    GCInterestPtr  pGCI;	/* GCInterest being destroyed */
{
    if (pGC->devPriv)
	FreeGC ((GCPtr)pGC->devPriv);
    Xfree (pGCI);
}

/*-
 *-----------------------------------------------------------------------
 * spriteValidateGC --
 *	Called when a GC is about to be used for drawing. Copies all
 *	changes from the GC to its shadow and validates the shadow.
 *
 * Results:
 *	TRUE, for no readily apparent reason.
 *
 * Side Effects:
 *	Vectors in the shadow GC will likely be changed.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
spriteValidateGC (pGC, pGCI, changes, pDrawable)
    GCPtr	  pGC;
    GCInterestPtr pGCI;
    Mask	  changes;
    DrawablePtr	  pDrawable;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    if ( pGC->depth != pDrawable->depth )
	FatalError( "spriteValidateGC: depth mismatch.\n" );
/*
 *  spriteValidateGC copies the GC to the shadow.  Alas,
 *  spriteChangeClip has stored the new clip in the shadow,
 *  where it will be overwritten,  unless we pretend
 *  that the clip hasn't changed.
 */
    changes &= ~GCClipMask;

    CopyGC (pGC, pShadowGC, changes);
    pShadowGC->serialNumber = pGC->serialNumber;
    ValidateGC (pDrawable, pShadowGC);
}
	
/*-
 *-----------------------------------------------------------------------
 * spriteCopyGC --
 *	Called when a GC with its shadow is the destination of a copy.
 *	Calls CopyGC to transfer the changes to the shadow GC as well.
 *	Should not be used for the CopyGCSource since we like to copy from
 *	the real GC to the shadow using CopyGC...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Any changes in the real GC are copied to the shadow.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
spriteCopyGC (pGCDst, pGCI, changes, pGCSrc)
    GCPtr   	  pGCDst;
    GCInterestPtr pGCI;
    int    	  changes;
    GCPtr   	  pGCSrc;
{
    CopyGC (pGCDst, (GCPtr) pGCDst->devPriv, pGCDst->stateChanges|changes);
}


/*
 * Array of functions to replace the functions in the GC.
 * Caveat: Depends on the ordering of functions in the GC structure.
 */
static void (* spriteGCFuncs[]) () = {
    spriteFillSpans,
    spriteSetSpans,

    spritePutImage,
    spriteCopyArea,
    spriteCopyPlane,
    spritePolyPoint,
    spritePolylines,
    spritePolySegment,
    spritePolyRectangle,
    spritePolyArc,
    spriteFillPolygon,
    spritePolyFillRect,
    spritePolyFillArc,
    (void(*)())spritePolyText8,
    (void(*)())spritePolyText16,
    spriteImageText8,
    spriteImageText16,
    spriteImageGlyphBlt,
    spritePolyGlyphBlt,
    spritePushPixels,
    spriteLineHelper,
    spriteChangeClip,
    spriteDestroyClip,
    spriteCopyClip,
};

/*-
 *-----------------------------------------------------------------------
 * spriteCreatePrivGC --
 *	Create a GC private to the Sun DDX code. Such a GC is unhampered
 *	by any shadow GC and should only be used by functions which know
 *	what they are doing. This substitutes the old CreateGC function
 *	in the drawable's screen's vector and calls the DIX CreateGC
 *	function. Once done, it restores the CreateGC vector to its
 *	proper state and returns the GCPtr CreateGC gave back.
 *
 * Results:
 *	A pointer to the new GC.
 *
 * Side Effects:
 *	The GC's graphicsExposures field is set FALSE.
 *
 *-----------------------------------------------------------------------
 */
GCPtr
spriteCreatePrivGC(pDrawable, mask, pval, pStatus)
    DrawablePtr	  pDrawable;
    BITS32	  mask;
    long	  *pval;
    BITS32	  *pStatus;
{
    fbFd    	  *fb;
    GCPtr   	  pGC;
    BITS32  	  ge = FALSE;

    fb = &spriteFbs[pDrawable->pScreen->myNum];
    pDrawable->pScreen->CreateGC = fb->CreateGC;

    pGC = GetScratchGC (pDrawable->depth, pDrawable->pScreen);
    ChangeGC (pGC, mask, pval);
    ChangeGC (pGC, GCGraphicsExposures, &ge);

    pDrawable->pScreen->CreateGC = spriteCreateGC;

    return pGC;
}

/*-
 *-----------------------------------------------------------------------
 * spriteCreateGC --
 *	This function is used to get our own validation hooks into each
 *	GC to preserve the cursor. It calls the regular creation routine
 *	for the screen and then, if that was successful, tacks another
 *	GCInterest structure onto the GC *after* the one placed on by
 *	the screen-specific CreateGC...
 *
 * Results:
 *	TRUE if created ok. FALSE otherwise.
 *
 * Side Effects:
 *	A GCInterest structure is stuck on the end of the GC's list.
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteCreateGC (pGC)
    GCPtr	pGC;	/* The GC to play with */
{
    GCInterestPtr	pGCI;
    register GCPtr	pShadowGC;
    int			i;
    
    if ((*spriteFbs[pGC->pScreen->myNum].CreateGC) (pGC)) {
	
	if (pGC->depth != pGC->pScreen->rootDepth) {
	    /*
	     * This GC will never be used for drawing on the screen so no
	     * shadow needed.
	     */
	    return (TRUE);
	}
	pShadowGC = (GCPtr) Xalloc (sizeof (GC));
	if (pShadowGC == (GCPtr)NULL) {
	    return FALSE;
	}
	
	*pShadowGC = *pGC;
	pGC->devPriv = (pointer)pShadowGC;
	bcopy (spriteGCFuncs, &pGC->FillSpans, sizeof (spriteGCFuncs));
	
	pGCI = (GCInterestPtr) Xalloc (sizeof (GCInterestRec));
	if (!pGCI) {
	    return FALSE;
	}

	/*
	 * Any structure being shared between these two GCs must have its
	 * reference count incremented. This includes:
	 *  font, tile, stipple.
	 * Anything which doesn't have a reference count must be duplicated:
	 *  dash
	 * 
	 */
	if (pGC->font) {
	    pGC->font->refcnt++;
	}
	if (pGC->tile) {
	    pGC->tile->refcnt++;
	}
	if (pGC->stipple) {
	    pGC->stipple->refcnt++;
	}

	pShadowGC->dash = (unsigned char *)
	    Xalloc(pGC->numInDashList * sizeof(unsigned char));
	for (i=0; i<pGC->numInDashList; i++) {
	    pShadowGC->dash[i] = pGC->dash[i];
	}

	pGC->pNextGCInterest = pGCI;
	pGC->pLastGCInterest = pGCI;
	pGCI->pNextGCInterest = (GCInterestPtr) &pGC->pNextGCInterest;
	pGCI->pLastGCInterest = (GCInterestPtr) &pGC->pNextGCInterest;
	pGCI->length = sizeof(GCInterestRec);
	pGCI->owner = 0;	    	    /* server owns this */
	pGCI->ValInterestMask = ~0; 	    /* interested in everything */
	pGCI->ValidateGC = spriteValidateGC;
	pGCI->ChangeInterestMask = 0; 	    /* interested in nothing */
	pGCI->ChangeGC = (int (*)()) NULL;
	pGCI->CopyGCSource = (void (*)())NULL;
	pGCI->CopyGCDest = spriteCopyGC;
	pGCI->DestroyGC = spriteDestroyGC;
	

	/*
	 * Because of this weird way of handling the GCInterest lists,
	 * we need to modify the output library's GCInterest structure to
	 * point to the pNextGCInterest field of the shadow GC...
	 */
	pGCI = pShadowGC->pNextGCInterest;
	pGCI->pLastGCInterest = pGCI->pNextGCInterest =
	    (GCInterestPtr) &pShadowGC->pNextGCInterest;

	return TRUE;
    } else {
	return FALSE;
    }
}
