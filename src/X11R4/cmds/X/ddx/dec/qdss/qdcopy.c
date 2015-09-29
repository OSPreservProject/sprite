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

#include "X.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "mi.h"

#include "qd.h"
#include "qdgc.h"

/* due to validation...							*
 *	if this is called, pDstDrawable is SURELY a DRAWABLE_WINDOW.	*
 *	-unless, of course, it's a fake window, called from qdCopyArea
 */
RegionPtr
qdCopyAreaWin(pSrcDrawable, pDstDrawable,
	      pGC, srcx, srcy, width, height, dstx, dsty)
    register DrawablePtr pSrcDrawable;
    register DrawablePtr pDstDrawable;
    GCPtr	pGC;   /* composite clip region here is that of pDstDrawable */
    int		srcx, srcy;
    int		width, height;
    int		dstx, dsty;
{
    if ((width == 0) || (height == 0)) return NULL;

    if (pSrcDrawable->type == DRAWABLE_WINDOW) {
	WindowPtr	psrcwin;
	int		abssrcx, abssrcy;	/* screen coordinates */
	int 	absdstx, absdsty;	/* screen coordinates */

	RegionRec	pcompclip[1];
	psrcwin = (WindowPtr)pSrcDrawable;
	abssrcx = QDWIN_X(psrcwin) + srcx;
	abssrcy = QDWIN_Y(psrcwin) + srcy;
	absdstx = pGC->lastWinOrg.x + dstx;
	absdsty = pGC->lastWinOrg.y + dsty;

	/* set up pcompclip to be argument Box */
	pcompclip->extents.x1 = abssrcx;
	pcompclip->extents.x2 = abssrcx + width;
	pcompclip->extents.y1 = abssrcy;
	pcompclip->extents.y2 = abssrcy + height;
#ifdef X11R4
	pcompclip->data = NULL;
#else
	pcompclip->size = 1;
	pcompclip->numRects = 1;
	pcompclip->rects = (BoxPtr) Xalloc(sizeof(BoxRec));
	pcompclip->rects[0] = pcompclip->extents;
#endif

	if ( pGC->subWindowMode == IncludeInferiors) /* used by qdCopyWindow */
	    miIntersect( pcompclip, pcompclip, QDWIN_WINSIZE(psrcwin));
	else
	    miIntersect( pcompclip, pcompclip, QDWIN_CLIPLIST(psrcwin));
	miTranslateRegion(pcompclip, absdstx-abssrcx, absdsty-abssrcy);

	miIntersect( pcompclip, pcompclip, QDGC_COMPOSITE_CLIP(pGC));
	tlbltregion(pGC, pcompclip, absdstx-abssrcx, absdsty-abssrcy);
#ifdef X11R4
	if (pcompclip->data && pcompclip->data->size) Xfree(pcompclip->data);
#else
	Xfree(pcompclip->rects);
#endif
	/*
	 * miHandleExposures wants window-relative coordinates
	 */
	return miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
				 srcx, srcy, width, height, dstx, dsty, 0);
    }
    else if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	return (RegionPtr) tlspaca(pSrcDrawable, pDstDrawable,
				pGC, srcx, srcy, width, height, dstx, dsty);
        
    } else /* pSrcDrawable->type must be UNDRAWABLE_WINDOW */
	return miCopyArea( pSrcDrawable, pDstDrawable,
				pGC, srcx, srcy, width, height, dstx, dsty);
}

RegionPtr
qdCopyArea( pSrcDrawable, pDstDrawable,
				    pGC, srcx, srcy, width, height, dstx, dsty)
    register DrawablePtr pSrcDrawable;
    register DrawablePtr pDstDrawable;
    GCPtr	pGC;   /* composite clip region here is that of pDstDrawable */
    int		srcx, srcy;
    int		width, height;
    int		dstx, dsty;
{
/* We start with some magic to guard against both src and dst being
 * pixmaps that fight for offscreen space.
 */
    if (pDstDrawable->type == DRAWABLE_PIXMAP) {
	tlConfirmPixmap((QDPixPtr)pDstDrawable);
	if (pSrcDrawable->type == DRAWABLE_PIXMAP)
	    tlConfirmPixmap((QDPixPtr)pSrcDrawable);
	tlSinglePixmap((QDPixPtr)pDstDrawable);
    }
    if (pDstDrawable->type == DRAWABLE_PIXMAP
	/* make sure dst is still off-screen */
	&& ((QDPixPtr)pDstDrawable)->planes
	/* if src is Bitmap which is not offscreen, use mi */
	&& (pSrcDrawable->depth > 1 || ((QDPixPtr)pSrcDrawable)->planes)) {
	RegionPtr region;

	SETUP_PIXMAP_AS_WINDOW(pDstDrawable, pGC);
	CHECK_MOVED(pGC, pDstDrawable);
	/*
	 * If src is Bitmap in plane different from dst, must use
	 * tlPlaneCopy. Otherwise, make fake window. (If src and
	 * dst are Bitmaps in the same plane, prefer qdCopyAreaWin, because
	 * it ultimately call tlbitblt, which correctly deals with
	 * overlapping src and dst.)
	 */
	if (pSrcDrawable->depth == 1
	    && !(((QDPixPtr)pSrcDrawable)->planes
	       & ((QDPixPtr)pDstDrawable)->planes)) {
	    pGC->fgPixel = Allplanes;
	    pGC->bgPixel = 0;
	    tlPlaneBlt(pGC,
		       dstx + QDPIX_X((QDPixPtr)pDstDrawable),
		       dsty + QDPIX_Y((QDPixPtr)pDstDrawable),
		       width, height,
		       srcx + QDPIX_X((QDPixPtr)pSrcDrawable),
		       srcy + QDPIX_Y((QDPixPtr)pSrcDrawable),
		       ((QDPixPtr)pSrcDrawable)->planes);
	    region = NULL;
	}
        else
	    region = qdCopyAreaWin(pSrcDrawable, pDstDrawable,
				   pGC, srcx, srcy, width, height, dstx, dsty);
	CLEANUP_PIXMAP_AS_WINDOW(pGC);
	return region;	
    } else
	return miCopyArea(pSrcDrawable, pDstDrawable,
			  pGC, srcx, srcy, width, height, dstx, dsty);
}

/* Validation ensures that pDstDrawable is DRAWABLE_WINDOW. */

RegionPtr
qdCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, bitPlane)
    DrawablePtr 	pSrcDrawable;
    DrawablePtr		pDstDrawable;
    GCPtr		pGC;
    int 		srcx, srcy;
    int 		width, height;
    int 		dstx, dsty;
    unsigned long	bitPlane;
{
    int xOrg = dstx + pGC->lastWinOrg.x;
    int yOrg = dsty + pGC->lastWinOrg.y;
    unsigned long mask = 
	pSrcDrawable->depth == 1 ? ((QDPixPtr)pSrcDrawable)->planes
	    : bitPlane;
    if (pSrcDrawable->type == DRAWABLE_PIXMAP) {
	int w = QDPIX_WIDTH((PixmapPtr)pSrcDrawable) - srcx;
	int h = QDPIX_HEIGHT((PixmapPtr)pSrcDrawable) - srcy;
	if (w > width) w = width;
	if (h > height) h = height;
	if (QDPIX_Y((QDPixPtr)pSrcDrawable) != NOTOFFSCREEN)
	    tlPlaneBlt(pGC, xOrg, yOrg, w, h,
		       srcx + QDPIX_X((QDPixPtr)pSrcDrawable),
		       srcy + QDPIX_Y((QDPixPtr)pSrcDrawable),
		       mask);
	else {
	    RegionPtr pcl = QDGC_COMPOSITE_CLIP(pGC);
	    register BoxPtr	rects = REGION_RECTS(pcl);
	    int saveMask = ((QDPixPtr)pSrcDrawable)->planes;
	    int ic;
	    ((QDPixPtr)pSrcDrawable)->planes = mask;
	    for (ic = REGION_NUM_RECTS(pcl); --ic >= 0; rects++) {
		BoxRec	cb;
		cb.x1 = max( rects->x1, xOrg);
		cb.y1 = max( rects->y1, yOrg);
		cb.x2 = min( rects->x2, xOrg+width);
		cb.y2 = min( rects->y2, yOrg+height);
		if (cb.x1 < cb.x2 && cb.y1 < cb.y2)
		    tlBitmapBichrome(pGC, pSrcDrawable,
				     pGC->fgPixel, pGC->bgPixel,
				     xOrg - srcx, yOrg - srcy, &cb);
	    }
	    ((QDPixPtr)pSrcDrawable)->planes = saveMask;
	}
    }
    else if (pSrcDrawable->type == DRAWABLE_WINDOW) {
	WindowPtr	psrcwin;
	int		abssrcx, abssrcy;	/* screen coordinates */
	int 	absdstx, absdsty;	/* screen coordinates */
	
	RegionRec	pcompclip[1];
	RegionPtr	pSaveClip = QDGC_COMPOSITE_CLIP(pGC);
	psrcwin = (WindowPtr)pSrcDrawable;
	abssrcx = QDWIN_X(psrcwin) + srcx;
	abssrcy = QDWIN_Y(psrcwin) + srcy;
	absdstx = pGC->lastWinOrg.x + dstx;
	absdsty = pGC->lastWinOrg.y + dsty;

	/* set up pcompclip to be argument Box */
	pcompclip->extents.x1 = abssrcx;
	pcompclip->extents.x2 = abssrcx + width;
	pcompclip->extents.y1 = abssrcy;
	pcompclip->extents.y2 = abssrcy + height;
#ifdef X11R4
	pcompclip->data = NULL;
#else
	pcompclip->size = 1;
	pcompclip->numRects = 1;
	pcompclip->rects = (BoxPtr) Xalloc(sizeof(BoxRec));
	pcompclip->rects[0] = pcompclip->extents;
#endif

	if ( pGC->subWindowMode == IncludeInferiors) /* used by qdCopyWindow */
	    miIntersect( pcompclip, pcompclip, QDWIN_WINSIZE(psrcwin));
	else
	    miIntersect( pcompclip, pcompclip, QDWIN_CLIPLIST(psrcwin));
	miTranslateRegion(pcompclip, absdstx-abssrcx, absdsty-abssrcy);
	miIntersect( pcompclip, pcompclip, QDGC_COMPOSITE_CLIP(pGC));
	QDGC_COMPOSITE_CLIP(pGC) = pcompclip;
	tlPlaneBlt(pGC, xOrg, yOrg, width, height,
		   srcx + QDWIN_X((WindowPtr)pSrcDrawable),
		   srcy + QDWIN_Y((WindowPtr)pSrcDrawable),
		   mask);
	QDGC_COMPOSITE_CLIP(pGC) = pSaveClip;
#ifdef X11R4
	if (pcompclip->data && pcompclip->data->size) Xfree(pcompclip->data);
#else
	Xfree(pcompclip->rects);
#endif
    }
    return miHandleExposures(pSrcDrawable, pDstDrawable, pGC, srcx, srcy,
			     width, height, dstx, dsty, bitPlane);
}

RegionPtr
qdCopyPlanePix(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, bitPlane)
    DrawablePtr 	pSrcDrawable;
    DrawablePtr		pDstDrawable;
    GCPtr		pGC;
    int 		srcx, srcy;
    int 		width, height;
    int 		dstx, dsty;
    unsigned long	bitPlane;
{
    CHECK_MOVED(pGC, pDstDrawable);
    if (QD_PIX_DATA((PixmapPtr)pDstDrawable) == NULL) {
	RegionPtr region;
	/* make dummy window and use that as the drawable */
	SETUP_PIXMAP_AS_WINDOW(pDstDrawable, pGC);
	region = qdCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
	CLEANUP_PIXMAP_AS_WINDOW(pGC);
	return region;
    }
    else
	return miCopyPlane(pSrcDrawable, pDstDrawable,
		       pGC, srcx, srcy, width, height, dstx, dsty, bitPlane);
}
