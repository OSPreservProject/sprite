/*
 * Fill rectangles.  Used by both PolyFillRect and PaintWindow.
 * no depth dependencies.
 */

/*
Copyright 1989 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the software
without specific, written prior permission.  M.I.T. makes no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.
*/

/* $XConsortium: cfbfillrct.c,v 5.9 89/11/19 15:42:14 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "windowstr.h"

#include "cfb.h"
#include "cfbmskbits.h"

#if PPW == 4
extern void cfb8FillBoxOpaqueStippled32();
extern void cfb8FillBoxTransparentStippled32();
#endif

void
cfbFillBoxSolid (pDrawable, nBox, pBox, pixel, isCopy)
    DrawablePtr	    pDrawable;
    int		    nBox;
    BoxPtr	    pBox;
    unsigned long   pixel;
    Bool	    isCopy;
{
    unsigned long   *pdstBase, *pdstRect;
    int		    widthDst;
    register int    h;
    register unsigned long   fill;
    register unsigned long   *pdst;
    register unsigned long   leftMask, rightMask;
    int		    nmiddle;
    register int    m;
    int		    w;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (unsigned long *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthDst = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	pdstBase = (unsigned long *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    fill = PFILL(pixel);
    for (; nBox; nBox--, pBox++)
    {
    	pdstRect = pdstBase + pBox->y1 * widthDst;
    	h = pBox->y2 - pBox->y1;
	w = pBox->x2 - pBox->x1;
#if PPW == 4
	if (w == 1)
	{
	    register char    *pdstb = ((char *) pdstRect) + pBox->x1;
	    int	    incr = widthDst << 2;

	    if (isCopy)
	    {
		while (h--)
		{
		    *pdstb = fill;
		    pdstb += incr;
		}
	    }
	    else
	    {
		while (h--)
		{
		    *pdstb ^= fill;
		    pdstb += incr;
		}
	    }
	}
	else
	{
#endif
	pdstRect += (pBox->x1 >> PWSH);
	if ((pBox->x1 & PIM) + w <= PPW)
	{
	    maskpartialbits(pBox->x1, w, leftMask);
	    pdst = pdstRect;
	    if (isCopy)
	    {
		while (h--) {
		    *pdst = (*pdst & ~leftMask) | (fill & leftMask);
		    pdst += widthDst;
		}
	    }
	    else
	    {
		while (h--) {
		    *pdst = (*pdst & ~leftMask) | ((fill ^ *pdst) & leftMask);
		    pdst += widthDst;
		}
	    }
	}
	else
	{
	    maskbits (pBox->x1, w, leftMask, rightMask, nmiddle);
	    if (isCopy)
	    {
		if (leftMask)
		{
		    if (rightMask)
		    {
			while (h--) {
			    pdst = pdstRect;
			    *pdst = (*pdst & ~leftMask) | (fill & leftMask);
			    pdst++;
			    m = nmiddle;
			    while (m--)
				*pdst++ = fill;
			    *pdst = (*pdst & ~rightMask) | (fill & rightMask);
			    pdstRect += widthDst;
			}
		    }
		    else
		    {
			while (h--) {
			    pdst = pdstRect;
			    *pdst = (*pdst & ~leftMask) | (fill & leftMask);
			    pdst++;
			    m = nmiddle;
			    while (m--)
				*pdst++ = fill;
			    pdstRect += widthDst;
			}
		    }
		}
		else
		{
		    if (rightMask)
		    {
			while (h--) {
			    pdst = pdstRect;
			    m = nmiddle;
			    while (m--)
				*pdst++ = fill;
			    *pdst = (*pdst & ~rightMask) | (fill & rightMask);
			    pdstRect += widthDst;
			}
		    }
		    else
		    {
			while (h--)
			{
			    pdst = pdstRect;
			    m = nmiddle;
			    while (m--)
				*pdst++ = fill;
			    pdstRect += widthDst;
			}
		    }
		}
	    }
	    else
	    {
		while (h--) {
		    pdst = pdstRect;
		    if (leftMask)
		    {
			*pdst = (*pdst & ~leftMask) | ((fill ^ *pdst) & leftMask);
			pdst++;
		    }
		    m = nmiddle;
		    while (m--)
			*pdst++ ^= fill;
		    if (rightMask)
			*pdst = (*pdst & ~rightMask) | ((fill ^ *pdst) & rightMask);
		    pdstRect += widthDst;
		}
	    }
	}
#if PPW == 4
	}
#endif
    }
}

void
cfbFillBoxTile32 (pDrawable, nBox, pBox, tile)
    DrawablePtr	    pDrawable;
    int		    nBox;	/* number of boxes to fill */
    BoxPtr 	    pBox;	/* pointer to list of boxes to fill */
    PixmapPtr	    tile;	/* rotated, expanded tile */
{
    register int srcpix;	
    int *psrc;		/* pointer to bits in tile, if needed */
    int tileHeight;	/* height of the tile */

    int nlwDst;		/* width in longwords of the dest pixmap */
    int w;		/* width of current box */
    register int h;	/* height of current box */
    register unsigned long startmask;
    register unsigned long endmask; /* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    int nlwExtra;	/* to get from right of box to left of next span */
    register int nlw;	/* loop version of nlwMiddle */
    register unsigned long *p;	/* pointer to bits we're writing */
    int y;		/* current scan line */
    int srcy;		/* current tile position */

    unsigned long *pbits;/* pointer to start of pixmap */

    tileHeight = tile->drawable.height;
    psrc = (int *)tile->devPrivate.ptr;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pbits = (unsigned long *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	nlwDst = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	pbits = (unsigned long *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    while (nBox--)
    {
	w = pBox->x2 - pBox->x1;
	h = pBox->y2 - pBox->y1;
	y = pBox->y1;
	p = pbits + (pBox->y1 * nlwDst) + (pBox->x1 >> PWSH);
	srcy = y % tileHeight;

	if ( ((pBox->x1 & PIM) + w) < PPW)
	{
	    maskpartialbits(pBox->x1, w, startmask);
	    nlwExtra = nlwDst;
	    while (h--)
	    {
		srcpix = psrc[srcy];
		++srcy;
		if (srcy == tileHeight)
		    srcy = 0;
		*p = (*p & ~startmask) | (srcpix & startmask);
		p += nlwExtra;
	    }
	}
	else
	{
	    maskbits(pBox->x1, w, startmask, endmask, nlwMiddle);
	    nlwExtra = nlwDst - nlwMiddle;

	    if (startmask && endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    srcpix = psrc[srcy];
		    ++srcy;
		    if (srcy == tileHeight)
		        srcy = 0;
		    nlw = nlwMiddle;
		    *p = (*p & ~startmask) | (srcpix & startmask);
		    p++;
		    while (nlw--)
			*p++ = srcpix;
		    *p = (*p & ~endmask) | (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else if (startmask && !endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    srcpix = psrc[srcy];
		    ++srcy;
		    if (srcy == tileHeight)
		        srcy = 0;
		    nlw = nlwMiddle;
		    *p = (*p & ~startmask) | (srcpix & startmask);
		    p++;
		    while (nlw--)
			*p++ = srcpix;
		    p += nlwExtra;
		}
	    }
	    else if (!startmask && endmask)
	    {
		while (h--)
		{
		    srcpix = psrc[srcy];
		    ++srcy;
		    if (srcy == tileHeight)
		        srcy = 0;
		    nlw = nlwMiddle;
		    while (nlw--)
			*p++ = srcpix;
		    *p = (*p & ~endmask) | (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else /* no ragged bits at either end */
	    {
		while (h--)
		{
		    srcpix = psrc[srcy];
		    ++srcy;
		    if (srcy == tileHeight)
		        srcy = 0;
		    nlw = nlwMiddle;
		    while (nlw--)
			*p++ = srcpix;
		    p += nlwExtra;
		}
	    }
	}
        pBox++;
    }
}

void
cfbPolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    int xorg, yorg;
    register int n;		/* spare counter */
    xRectangle *prect;		/* temporary */
    RegionPtr prgnClip;
    register BoxPtr pbox;	/* used to clip with */
    register BoxPtr pboxClipped;
    BoxPtr pboxClippedBase;
    BoxPtr pextent;
    cfbPrivGC	*priv;
    int numRects;
    Bool isCopy;
    Pixel pixel;
    PixmapPtr	pRoTile;
    PixmapPtr	pTile;
    int		xrot, yrot;
    int		rgnResult;

    priv = (cfbPrivGC *) pGC->devPrivates[cfbGCPrivateIndex].ptr;
    prgnClip = priv->pCompositeClip;

    switch (pGC->fillStyle)
    {
    case FillSolid:
	isCopy = pGC->alu == GXcopy;
	pixel = pGC->fgPixel;
	if (pGC->alu == GXinvert)
	    pixel = pGC->planemask;
	break;
    case FillTiled:
	pRoTile = ((cfbPrivGCPtr) pGC->devPrivates[cfbGCPrivateIndex].ptr)->
							pRotatedPixmap;
	if (!pRoTile)
	{
	    pTile = pGC->tile.pixmap;
	    xrot = pDrawable->x + pGC->patOrg.x;
	    yrot = pDrawable->y + pGC->patOrg.y;
	}
	break;
#if (PPW == 4)
    case FillStippled:
    case FillOpaqueStippled:
	pTile = ((cfbPrivGCPtr) pGC->devPrivates[cfbGCPrivateIndex].ptr)->
							pRotatedPixmap;
	break;
#endif
    }
    prect = prectInit;
    xorg = pDrawable->x;
    yorg = pDrawable->y;
    if (xorg || yorg)
    {
	prect = prectInit;
	n = nrectFill;
	while(n--)
	{
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }

    prect = prectInit;

    pextent = (*pGC->pScreen->RegionExtents)(prgnClip);

    while (nrectFill--)
    {
	BoxRec box;
	int	x2, y2;

	/*
	 * clip the box to the extent of the region --
	 * avoids overflowing shorts and minimizes other
	 * computations
	 */

	box.x1 = prect->x;
	if (box.x1 < pextent->x1)
		box.x1 = pextent->x1;

	box.y1 = prect->y;
	if (box.y1 < pextent->y1)
		box.y1 = pextent->y1;

	x2 = (int) prect->x + (int) prect->width;
	if (x2 > pextent->x2)
		x2 = pextent->x2;
	box.x2 = x2;

	y2 = (int) prect->y + (int) prect->height;
	if (y2 > pextent->y2)
		y2 = pextent->y2;
	box.y2 = y2;

	prect++;

	if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    continue;

	if (REGION_NUM_RECTS (prgnClip) == 1 ||
	    (rgnResult = (*pGC->pScreen->RectIn)(prgnClip, &box)) == rgnIN)
	{
	    switch (pGC->fillStyle)
	    {
	    case FillSolid:
		cfbFillBoxSolid (pDrawable, 1, &box, pixel, isCopy);
		break;
	    case FillTiled:
		if (pRoTile)
		    cfbFillBoxTile32 (pDrawable, 1, &box, pRoTile);
		else
		    cfbFillBoxTileOdd (pDrawable, 1, &box, pTile, xrot, yrot);
		break;
#if (PPW == 4)
	    case FillStippled:
		cfb8FillBoxTransparentStippled32 (pDrawable, 1, &box, pTile,
						 pGC->fgPixel);
		break;
	    case FillOpaqueStippled:
		cfb8FillBoxOpaqueStippled32 (pDrawable, 1, &box, pTile,
						 pGC->fgPixel, pGC->bgPixel);
		break;
#endif
	    }
	} 
	else if (rgnResult == rgnPART)
	{
	    numRects = REGION_NUM_RECTS(prgnClip);
	    pboxClippedBase = (BoxPtr)ALLOCATE_LOCAL(numRects * sizeof(BoxRec));

	    if (!pboxClippedBase)
		return;

	    pboxClipped = pboxClippedBase;
	    pbox = REGION_RECTS(prgnClip);
	    n = numRects;

	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
		pboxClipped->x1 = max(box.x1, pbox->x1);
		pboxClipped->y1 = max(box.y1, pbox->y1);
		pboxClipped->x2 = min(box.x2, pbox->x2);
		pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

		/* see if clipping left anything */
		if(pboxClipped->x1 < pboxClipped->x2 && 
		   pboxClipped->y1 < pboxClipped->y2)
		{
		    pboxClipped++;
		}
	    }
	    switch (pGC->fillStyle)
	    {
	    case FillSolid:
		cfbFillBoxSolid (pDrawable,
				 pboxClipped-pboxClippedBase,
				 pboxClippedBase, pixel, isCopy);
		break;
	    case FillTiled:
		if (pRoTile)
		    cfbFillBoxTile32 (pDrawable,
				      pboxClipped-pboxClippedBase,
				      pboxClippedBase, pRoTile);
		else
		    cfbFillBoxTileOdd (pDrawable,
				       pboxClipped-pboxClippedBase,
				      pboxClippedBase, pTile, xrot, yrot);
		break;
#if (PPW == 4)
	    case FillStippled:
		cfb8FillBoxTransparentStippled32 (pDrawable,
		    pboxClipped-pboxClippedBase, pboxClippedBase,
		    pTile, pGC->fgPixel);
		break;
	    case FillOpaqueStippled:
		cfb8FillBoxOpaqueStippled32 (pDrawable,
		    pboxClipped-pboxClippedBase, pboxClippedBase,
		    pTile, pGC->fgPixel, pGC->bgPixel);
		break;
#endif
	    }
	    DEALLOCATE_LOCAL(pboxClippedBase);
	}
    }
}
