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

#include "Xmd.h"
#include "servermd.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "scrnintstr.h"

#include "qd.h"
#include "qdgc.h"
#include "qdprocs.h"

static void bitmapToScr(), bitmapToPixmap(), imageToPixmap();

#if NPLANES==24
#define DOPIXEL(psrc, dst, pGC, delta) qddopixel(psrc, dst, pGC, delta)
#else
#define DOPIXEL(psrc, dst, pGC, delta) qddopixel(psrc, dst, pGC)
#endif

void
qdGetImage( pDraw, x, y, w, h, format, planemask, pImage)
    DrawablePtr		pDraw;
    int			x, y, w, h;
    unsigned int	format;
    unsigned long	planemask;
    unsigned char	*pImage;
{
    switch( format)
    {
      case XYPixmap:
#ifdef X11R4
        if (pDraw->type == DRAWABLE_PIXMAP)
	    tlCancelPixmap(pDraw);
#endif
	miGetImage( pDraw, x, y, w, h, format, planemask, pImage);
	break;
      case ZPixmap:
	if ( pDraw->type == DRAWABLE_WINDOW)
	    tlgetimage( pDraw, x, y, w, h, planemask, pImage);
	else
#ifdef X11R4
	    tlCancelPixmap(pDraw), 
#endif
	    miGetImage( pDraw, x, y, w, h, format, planemask, pImage);
	break;
    }
#ifndef X11R4
    if ((pDraw->type == DRAWABLE_WINDOW) &&
	(((WindowPtr)pDraw)->backingStore != NotUseful))
    {
	miBSGetImage( pDraw, (PixmapPtr) 0, x, y, w, h, format, planemask, pImage);
    }
#endif
}

void
qdPixPutImage( pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr		pDraw;
    GCPtr		pGC;
    int			depth, x, y, w, h, leftPad;
    unsigned int	format;
    unsigned char	*pImage;
{
    CHECK_MOVED(pGC, pDraw);
    if (depth == 1 && format != XYBitmap) {
	if (QD_PIX_DATA((PixmapPtr)pDraw) == NULL && leftPad == 0) {
	    SETUP_PIXMAP_AS_WINDOW(pDraw, pGC);
	    /* ignore {fg,bg}Pixel if gotten here via doBitmap */
	    pGC->fgPixel = Allplanes;
	    pGC->bgPixel = 0;
	    bitmapToScr(pGC, x, y, w, h, leftPad, pImage);
	    CLEANUP_PIXMAP_AS_WINDOW(pGC);
	    return;
	}
	else
	    goto doBitmap;
    }
    switch( format)
    {
      case XYBitmap:
	if (QD_PIX_DATA((PixmapPtr)pDraw) == NULL
	 && leftPad == 0 && pGC->alu == GXcopy) {
	    SETUP_PIXMAP_AS_WINDOW(pDraw, pGC);
	    bitmapToScr(pGC, x, y, w, h, leftPad, pImage);
	    CLEANUP_PIXMAP_AS_WINDOW(pGC);
	    break;
	}
      doBitmap:
	if ( pDraw->depth > 1) {
	    bitmapToPixmap( (PixmapPtr)pDraw, pGC, x, y, w, h,
							leftPad, pImage);
	    break;
	}
	/* otherwise, fall through */
      case XYPixmap:
	miPutImage( pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage);
	break;
      case ZPixmap:
	if (QD_PIX_DATA((PixmapPtr)pDraw) == NULL)
	    tlputimage( pDraw, pGC, x, y, w, h, pImage);
	else
	    imageToPixmap( pDraw, pGC, x, y, w, h, pImage );
	break;
    }
}

void
qdWinPutImage( pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
    WindowPtr		pDraw;
    GCPtr		pGC;
    int			depth, x, y, w, h, leftPad;
    unsigned int	format;
    unsigned char	*pImage;
{
    switch( format)
    {
      case XYBitmap:
	if (leftPad == 0 && pGC->alu == GXcopy)
	{
	    bitmapToScr(pGC, x, y, w, h, leftPad, pImage);
	    break;
	}
      case XYPixmap:
	miPutImage( pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage);
	break;
      case ZPixmap:
	tlputimage( pDraw, pGC, x, y, w, h, pImage );
	break;
    }
}

/*
 * use the bitmap to select back or fore color from GC
 */
static void
bitmapToScr(pGC, x, y, w, h, leftPad, pImage)
    GCPtr               pGC;
    int                 x, y, w, h, leftPad;
    unsigned char       *pImage;
{
    PixmapRec	tempBMap[1];
    int         ic;     /* clip rect index */
    int	numRects = REGION_NUM_RECTS(QDGC_COMPOSITE_CLIP(pGC));
    register BoxPtr rects = REGION_RECTS(QDGC_COMPOSITE_CLIP(pGC));
    BoxPtr newRects;


    x += pGC->lastWinOrg.x;
    y += pGC->lastWinOrg.y;

    /*
     * create a temporary bitmap and transplant pImage into it
     */
    QDPIX_WIDTH(tempBMap) = w+leftPad;
    QDPIX_HEIGHT(tempBMap) = h;
    tempBMap->devKind = PixmapBytePad(QDPIX_WIDTH(tempBMap), 1);
    QD_PIX_DATA(tempBMap) = pImage;

    for ( ic=0; ic < numRects; ic++, rects++)
	tlBitmapBichrome( pGC, tempBMap, pGC->fgPixel, pGC->bgPixel,
	    x, y, rects);
}

/*
 * use the bitmap to select back or fore color from GC
 */
static void
bitmapToPixmap( pPix, pGC, x, y, w, h, leftPad, pImage)
    PixmapPtr   pPix;
    GCPtr       pGC;
    int         x, y, w, h, leftPad;
    unsigned char *pImage;
{
    int		ic;	/* clip rect index */
    BoxPtr	pc = REGION_RECTS(QDGC_COMPOSITE_CLIP(pGC));

    int		br;	/* bitmap row */
    register int bc;	/* bitmap column */
    register unsigned *	pbr;	/* pointer to bitmap row */
    int		bwlongs = PixmapWidthInPadUnits( w, 1); /* bitmap width
							    in longs */

    DDXPointRec	mp;	/* pixmap point */

    /*
     * for each clipping rectangle
     */
    for (ic = REGION_NUM_RECTS(QDGC_COMPOSITE_CLIP(pGC)); --ic >= 0; pc++)
    {
	int	brlast = min( h, pc->y2-y);  /* last bitmap row to paint */
	  
	/*
	 * for each row in the intersection of bitmap source and dest clip
	 */
	for ( br = max( 0, pc->y1-y), mp.y = y + br,
		pbr = (unsigned *) &pImage[ br*(bwlongs<<2) ];
	      br < brlast;
	      br++, mp.y++, pbr+=bwlongs
	    )
	{
	    int	bclast = leftPad + min( w, pc->x2-x);

	    /*
	     * for each column in the intersection
	     */
	    for ( bc = leftPad + max( 0, pc->x1-x), mp.x = x + (bc-leftPad);
		  bc < bclast;
		  bc++, mp.x++)
	    {
		/*
                 * examine each bit.
                 */
		/* XXX - now uses qddopixel below
		unsigned char	pixel;
                pixel = ( pbr[ bc>>5] & (1 << (bc&0x1f)))
			    ? pGC->fgPixel
			    : pGC->bgPixel;
		*/
		DOPIXEL((pbr[ bc>>5] & (1 << (bc&0x1f)))?
		    (unsigned char *) &pGC->fgPixel:
		    (unsigned char *) &pGC->bgPixel,
		    QD_PIX_DATA(pPix) + mp.x + mp.y * QDPIX_WIDTH(pPix),
		    pGC, QDPIX_WIDTH(pPix) * QDPIX_HEIGHT(pPix));
	    }
	}
    }
}

/*
 * transfer an image into a pixmap
 */
static void
imageToPixmap( pPix, pGC, x, y, w, h, pImage)
    PixmapPtr           pPix;
    GCPtr               pGC;
    int                 x, y, w, h;
    unsigned char       *pImage;
{
    extern unsigned int Allplanes;
    register unsigned char	*pcolor;	/* current z-val */
    register int		minx, miny, maxy, width;
    RegionPtr	pSaveGCclip = QDGC_COMPOSITE_CLIP(pGC);
    register BoxPtr	pclip = REGION_RECTS(pSaveGCclip);
    register int	nclip = REGION_NUM_RECTS(pSaveGCclip);
    DDXPointRec		mp;	/* pixmap point */
    int fast = pGC->alu == GXcopy && (pGC->planemask & Allplanes) == Allplanes;

    /*
     * for each clipping rectangle
     */
    for ( ; nclip > 0; nclip--, pclip++)
    {
	minx = max(x,	pclip->x1);
	width= min(x+w,	pclip->x2) - minx;
	miny = max(y,	pclip->y1);
	maxy = min(y+h,	pclip->y2);

        if (width <= 0) return;

	for (mp.y = miny; mp.y < maxy; mp.y++)
	{
	    int i;
	    unsigned char *pdst =
		QD_PIX_DATA(pPix) + minx + mp.y * QDPIX_WIDTH(pPix);
	    pcolor =
		pImage + (NPLANES/8)*((mp.y-y) * QDPIX_WIDTH(pPix) + (minx-x));

#if NPLANES<24
            if (fast)
		bcopy(pcolor, pdst, width);
	    else
#endif
	    for (i = width; --i >= 0; pdst++)
	    {
		DOPIXEL(pcolor, pdst, pGC,
			QDPIX_WIDTH(pPix) * QDPIX_HEIGHT(pPix));
		pcolor += (NPLANES/8);
	    }
	}
    }
}

