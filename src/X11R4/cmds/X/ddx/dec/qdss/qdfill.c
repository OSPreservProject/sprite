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

#include "misc.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "qd.h"
#include "qdgc.h"
#include "qdprocs.h"
#include "pixmapstr.h"

/* Mixing of levels! Needed for ScreenHeight */
#include "libtl/tl.h"

/*
 * make this FillSpans instead?		XX
 */
void
qdPolyFillBoxesOddSize(pWin, pGC, nrect, pdestboxes)
    WindowPtr	pWin;
    GCPtr	pGC;
    int		nrect;           /* number of rectangles to fill */
    BoxPtr	pdestboxes;
{
    int		nr, ok;
    register BoxPtr pbox;
    RegionPtr	pSaveGCclip = QDGC_COMPOSITE_CLIP(pGC);
    RegionPtr	pcompregion;		/* built from pboxes argument */
    int rx, ry;  /* origin in window coordinates */
    int width, height;
    int ix, iy;	/* steps through the box; in window coordinate */
    PixmapPtr	pTile =
#ifdef X11R4
	pGC->fillStyle == FillTiled ? pGC->tile.pixmap : pGC->stipple;
#else
	pGC->fillStyle == FillTiled ? pGC->tile : pGC->stipple;
#endif
    int xStart;

    pcompregion = qdRegionInit( pdestboxes, nrect);
    miTranslateRegion( pcompregion, pGC->lastWinOrg.x, pGC->lastWinOrg.y);
    miIntersect( pcompregion, pcompregion, QDGC_COMPOSITE_CLIP(pGC));
    QDGC_COMPOSITE_CLIP(pGC) = pcompregion;

    rx = pcompregion->extents.x1 - pGC->lastWinOrg.x;
    ry = pcompregion->extents.y1 - pGC->lastWinOrg.y;
    width = pcompregion->extents.x2 - pcompregion->extents.x1;
    height = pcompregion->extents.y2 - pcompregion->extents.y1;
    xStart = rx - UMOD( rx-pGC->patOrg.x, QDPIX_WIDTH(pTile));
    iy = ry - UMOD( ry-pGC->patOrg.y, QDPIX_HEIGHT(pTile));
    /*
     * for each row of tiles, bump iy
     * subtract enough off of initial iy to align tile
     *
     * for each column of tiles, bump ix
     * subtract enough off of initial ix to align tile
     */

    switch ( pGC->fillStyle) {
      case FillStippled: {
	  long gcval;
	  gcval = (long)FillSolid;
	  /*  temporarily set fillstyle to FillSolid */
	  DoChangeGC(pGC, GCFillStyle, &gcval, 0);
	  ValidateGC(pWin, pGC);
	  for ( ; iy < ry + height; iy += QDPIX_HEIGHT(pTile))
	      for (ix = xStart; ix < rx + width; ix += QDPIX_WIDTH(pTile))
		  qdPushPixels(pGC, pTile, pWin,
			       QDPIX_WIDTH(pTile), QDPIX_HEIGHT(pTile),
			       ix, iy);
	  gcval = (long)FillStippled;
	  DoChangeGC(pGC, GCFillStyle, &gcval, 0);
	  ValidateGC(pWin, pGC);
	  break;
      }
      case FillOpaqueStippled:
	for ( ; iy < ry + height; iy += QDPIX_HEIGHT(pTile))
	    for (ix = xStart; ix < rx + width; ix += QDPIX_WIDTH(pTile))
#ifdef X11R4
		(*pGC->ops->CopyPlane)(pTile, pWin, pGC, 0, 0,
#else
		(*pGC->CopyPlane)(pTile, pWin, pGC, 0, 0,
#endif
				  QDPIX_WIDTH(pTile), QDPIX_HEIGHT(pTile),
				  ix, iy, 1);
	break;
      case FillTiled: {
#if 1
#define TILE_X 0
#define TILE_Y ScreenHeight
	  if ( tlConfirmPixmap(pTile)
	      && (pWin->drawable.type == DRAWABLE_WINDOW
		  || QDPIX_Y((QDPixPtr)pWin) != NOTOFFSCREEN)) {
	      int tileX, tileY;
	      int tileW = QDPIX_WIDTH(pTile);
	      int tileH = QDPIX_HEIGHT(pTile);
	      if (tileW < 32 && tileH < 32) {
		  tileW = (32 / tileW) * tileW;
		  tileH = (32 / tileH) * tileH;
		  tileX = TILE_X;
		  tileY = TILE_Y;
		  /* Makes gross assumptions about what tlrotile does! */
		  tlrotile(pTile, pGC->patOrg, NULL);
	      } else {
		  tileX = QDPIX_X((QDPixPtr)pTile);
		  tileY = QDPIX_Y((QDPixPtr)pTile);
	      }
	      for ( ; iy < ry + height; iy += tileH)
		  for (ix = xStart; ix < rx + width; ix += tileW)
		      tlbitblt(pGC,
			       pGC->lastWinOrg.x + ix, pGC->lastWinOrg.y + iy,
			       tileW, tileH, tileX, tileY);
	      }
	  else
#endif
	      for ( ; iy < ry + height; iy += QDPIX_HEIGHT(pTile))
		  for (ix = xStart; ix < rx + width; ix += QDPIX_WIDTH(pTile))
#ifdef X11R4
		      (*pGC->ops->CopyArea)(pTile, pWin, pGC, 0, 0,
#else
		      (*pGC->CopyArea)(pTile, pWin, pGC, 0, 0,
#endif
				       QDPIX_WIDTH(pTile), QDPIX_HEIGHT(pTile),
				       ix, iy);
	  break;
      }
      case FillSolid:
	FatalError( "Should have called tldrawshapes code!\n");
	break;
    }
    miRegionDestroy( pcompregion);
    QDGC_COMPOSITE_CLIP(pGC) = pSaveGCclip;
}

void
qdPolyFillRectOddSize( pDrawable, pGC, nrect, prect)
    DrawablePtr	pDrawable;
    GCPtr	pGC;
    int		nrect;           /* number of rectangles to fill */
    xRectangle * prect;          /* Pointer to first rectangle to fill */
{
    BoxPtr	pdestboxes;
    register BoxPtr	pdb;
    int		nr;

    if ( nrect == 0) return;

    pdb = pdestboxes = (BoxPtr) ALLOCATE_LOCAL( nrect*sizeof(BoxRec));
    /*
    * turn the rectangles into Boxes
    */
    for (nr = nrect; --nr >= 0; pdb++, prect++)
    {
	pdb->x1 = prect->x;
	pdb->x2 = prect->x + (int)prect->width;
	pdb->y1 = prect->y;
	pdb->y2 = prect->y + (int)prect->height;
    }

    qdPolyFillBoxesOddSize((WindowPtr)pDrawable, pGC, nrect, pdestboxes);
    DEALLOCATE_LOCAL(pdestboxes);
}

void
qdFillPolygon( pDrawable, pGC, shape, mode, npt, pptInit)
    DrawablePtr         pDrawable;
    register GCPtr      pGC;
    int                 shape, mode;
    register int        npt;
    DDXPointPtr         pptInit;
{
    DDXPointPtr         abspts;
    DDXPointPtr         closepts;
    Bool		allocated = FALSE;

    if ( pDrawable->type != DRAWABLE_WINDOW || shape != Convex ||
		pGC->fillStyle != FillSolid)
    {
	miFillPolygon( pDrawable, pGC, shape, mode, npt, pptInit);
	return;
    }

    if ( npt == 0)		/* make sure abspts[0] is valid */
	 return;
    if ( mode == CoordModeOrigin)
	abspts = pptInit;
    else	/* CoordModePrevious */
    {
	register int	ip;

	/* It is 'npt=1' in case we need to close the polygon below */
	abspts = (DDXPointPtr) ALLOCATE_LOCAL( (npt+1) * sizeof( DDXPointRec));
        allocated = TRUE;
	abspts[ 0].x = pptInit[ 0].x;
	abspts[ 0].y = pptInit[ 0].y;
	for ( ip=1; ip<npt; ip++)
	{
	    abspts[ ip].x = abspts[ ip-1].x + pptInit[ ip].x;
	    abspts[ ip].y = abspts[ ip-1].y + pptInit[ ip].y;
	}

    }
    /* close the polygon if necessary */
    if (abspts[npt-1].x != abspts[0].x
	    || abspts[npt-1].y != abspts[0].y)	/* not closed */
    {
	register int	ip;

	if (allocated)
	    closepts = abspts;
	else {
	    allocated = TRUE;
	    closepts =
		(DDXPointPtr) ALLOCATE_LOCAL( (npt+1) * sizeof(DDXPointRec));
	    for ( ip = npt; --ip >= 0; ) {
		closepts[ ip].x = abspts[ ip].x;
		closepts[ ip].y = abspts[ ip].y;
	    }
	}
	closepts[npt].x = abspts[0].x;
	closepts[npt].y = abspts[0].y;
	npt++;	/* add duplicate point to end */
    }
    else
	closepts = abspts;
    tlconpoly( (WindowPtr)pDrawable, pGC, npt, closepts);
    if (allocated) DEALLOCATE_LOCAL( closepts );
}

void
qdPixFillRect(pPix, pGC, nshapes, pshape)
     QDPixPtr pPix;
     GCPtr pGC;
     int nshapes;
     DDXPointPtr pshape;
     
{
    extern int PixmapUseOffscreen;
/* if PixmapUseOffscreen allow debugging of tlSolidSpans */
    if (QD_PIX_DATA(&pPix->pixmap) == NULL) {
	extern void tlSolidRects(), tlTiledRects(),
	    tlStipRects(), tlOpStipRects();
	static void (*FillFuncs[4])() = {
	    tlSolidRects, tlTiledRects, tlStipRects, tlOpStipRects};
	/* make dummy window and use that as the drawable */
	SETUP_PIXMAP_AS_WINDOW(&pPix->pixmap.drawable, pGC);
	CHECK_MOVED(pGC, &pPix->pixmap.drawable);
	(*FillFuncs[pGC->fillStyle])(pPix, pGC, nshapes, pshape);
	CLEANUP_PIXMAP_AS_WINDOW(pGC);
    }
    else
	miPolyFillRect(pPix, pGC, nshapes, pshape);
}
