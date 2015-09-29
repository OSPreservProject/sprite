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

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
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
#include "gcstruct.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"

#include "../../mfb/maskbits.h"

#include "qd.h"
#include "qdgc.h"

/* PushPixels to absolete screen coordinates */

static void
qdPushPixelsToScreen(pGC, pBitMap, dx, dy, xOrg, yOrg)
    GCPtr		pGC;
    QDPixPtr		pBitMap;
    int			dx, dy, xOrg, yOrg;
{
    xOrg += pGC->lastWinOrg.x;
    yOrg += pGC->lastWinOrg.y;
    if (QDPIX_Y(pBitMap) != NOTOFFSCREEN) {
	tlPlaneTile(pGC, xOrg, yOrg, dx, dy,
		       QDPIX_X(pBitMap),
		       QDPIX_Y(pBitMap),
		       pBitMap->planes);
    } else {
	/* fillStyle must be FillSolid */
	RegionPtr pcl = QDGC_COMPOSITE_CLIP(pGC);
	register BoxPtr	rects = REGION_RECTS(pcl);
	int ic;
	for (ic = REGION_NUM_RECTS(pcl); --ic >= 0; rects++)
	    {
		BoxRec	cb;

		cb.x1 = max( rects->x1, xOrg);
		cb.y1 = max( rects->y1, yOrg);
		cb.x2 = min( rects->x2, xOrg+dx);
		cb.y2 = min( rects->y2, yOrg+dy);

		if (cb.x1 < cb.x2 && cb.y1 < cb.y2)
		    tlBitmapStipple(pGC, pBitMap, pGC->fgPixel, pGC->bgPixel,
				    xOrg, yOrg, &cb);
	    }
    }
}

/*
 * xxPushPixels
 * pBitMap is a stencil (dx by dy of it is used, it may
 * be bigger) which is placed on the drawable at xOrg, yOrg.  Where a 1 bit
 * is set in the bitmap, the foreground pattern is put onto the drawable using
 * the GC's logical function. The drawable is not changed where the bitmap
 * has a zero bit or outside the area covered by the stencil.
*/
void
qdPushPixels( pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr		pGC;
    PixmapPtr		pBitMap;
    DrawablePtr 	pDrawable;
    int			dx, dy, xOrg, yOrg;
{

    /*
     * If pDrawable is not the screen
     */
    if ( pDrawable->type == UNDRAWABLE_WINDOW)
	return;
    if (pGC->fillStyle != FillSolid && !tlConfirmPixmap(pBitMap)) {
#ifdef X11R4
	qdUnnaturalPushPixels( pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg);
#else
	miPushPixels( pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg);
#endif
	return;
    }

    if ( pDrawable->type == DRAWABLE_PIXMAP)
    {
	CHECK_MOVED(pGC, pDrawable);
	if (QD_PIX_DATA((PixmapPtr)pDrawable) == NULL) {
	    SETUP_PIXMAP_AS_WINDOW(pDrawable, pGC);
	    qdPushPixelsToScreen(pGC, pBitMap, dx, dy, xOrg, yOrg);
	    CLEANUP_PIXMAP_AS_WINDOW(pGC);
	    }
#if NPLANES<24
	else if (pDrawable->depth > 1 && pGC->fillStyle == FillSolid) {
	    extern unsigned int Allplanes;
	    int ix, iy;
	    RegionPtr pGCclip = QDGC_COMPOSITE_CLIP(pGC);
	    BoxPtr pclip = REGION_RECTS(pGCclip);
	    int	nclip = REGION_NUM_RECTS(pGCclip);	/* number of clips */
	    int fast =
	      pGC->alu == GXcopy && (pGC->planemask & Allplanes) == Allplanes;
	    tlCancelPixmap(pBitMap);
	    for ( ; --nclip >= 0; pclip++) { /* for each clip rectangle */
	        unsigned char *maskStart = QD_PIX_DATA(pBitMap);
		unsigned char *pdst = QD_PIX_DATA((PixmapPtr)pDrawable)
		  + xOrg + yOrg *  QDPIX_WIDTH((PixmapPtr)pDrawable);
		int skip; /* used for clipping calculations */
		int x1 = 0, x2 = dx; /* coordinates relative to pBitMap */
		int height = dy;
		/* do x clipping */
		skip = xOrg + dx - pclip->x2;
		if (skip > 0) x2 -= skip;
		skip = pclip->x1 - xOrg;
		if (skip > 0) {
		    pdst += skip;
		    x1 += skip;
		}
		/* do y clipping */
		skip = yOrg + dy - pclip->y2;
		if (skip > 0) height -= skip;
		skip = pclip->y1 - yOrg;
		if (skip > 0) {
		    pdst += skip * QDPIX_WIDTH((PixmapPtr)pDrawable);
		    maskStart += skip * pBitMap->devKind;
		    height -= skip;
		}
		skip = x1 - x2; /* -(effective width) */
		if (skip >= 0) continue;
		skip += QDPIX_WIDTH((PixmapPtr)pDrawable);
		for (iy = 0; iy < height; iy++) {
		    if (fast)
		        for (ix = x1; ix < x2; ix++, pdst++) {
			    if (maskStart[ix >> 3] & (1 << (ix&7))) {
			        *pdst = pGC->fgPixel;
			    }
	                }
		    else
		        for (ix = x1; ix < x2; ix++, pdst++) {
			    if (maskStart[ix >> 3] & (1 << (ix&7))) {
			        qddopixel(&pGC->fgPixel, pdst, pGC);
		            }
	                }
		    maskStart += pBitMap->devKind;
		    pdst += skip;
	        } /* for each scanline */
	    } /* for each clip rectangle */
        }
#endif
	else {
#ifdef X11R4
	    /* Otherwise, miPushPixels violates qdGetSpans convention */
	    tlCancelPixmap(pBitMap);
#endif
	    miPushPixels( pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg);
	}
	return;
    }

    /*
     * now do the real work
     */
    qdPushPixelsToScreen(pGC, pBitMap, dx, dy, xOrg, yOrg);
}

#ifdef X11R4
qdUnnaturalPushPixels( pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr		pGC;
    PixmapPtr		pBitMap;
    DrawablePtr 	pDrawable;
    int			dx, dy, xOrg, yOrg;
{
/*
 * We cannot call miPushPixels if pBitMap is in offscreen memory,
 * since it fails to adjust coordinates by pBitMap->drawable.{x,y},
 * thus violating the (new) qdGetSpans interface.
 */
    tlCancelPixmap(pBitMap);
    miPushPixels( pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg);
}
#endif

#if 0
/*
 * This should be faster than miPushPixels to a Pixmap, because this
 * routine can take advantage of knowledge of the dragon-specific
 * pixmap representation.
 *	DON'T NEED SPEED YET, SO THIS IS NOT CALLED	XXX
 */
#define MAX3( a, b, c)	( (a)>(b)&&(a)>(c) ? (a) : (((b)>(c))?(b):(c)))
#define MIN3( a, b, c)	( (a)<(b)&&(a)<(c) ? (a) : (((b)<(c))?(b):(c)))

extern	int	Nplanes;
extern	int	Nentries;
extern	int	Nchannels;

static void
qdPPpixmap( pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr		pGC;
    PixmapPtr		pBitMap;
    PixmapPtr	 	pDrawable;
    int			dx, dy, xOrg, yOrg;
{
    PixmapPtr		pDp = (PixmapPtr) pDrawable; /* destination Pixmap */
    int			ch;	/* channel index */
    int			ic;	/* clip box index */

    /*
     * for each channel in destination pixmap
     */
    for ( ch=0; ch<Nchannels; ch++)
    {
	RegionPtr		pcl;
	register BoxPtr	p1box;
	unsigned char *pd =
	    QD_PIX_DATA(pDp) + ch * QDPIX_WIDTH(pDp) * QDPIX_HEIGHT(pDp);
	 
	/*
	 * for each clipping rectangle
	 */
	pcl = QDGC_COMPOSITE_CLIP(pGC);
	p1box = REGION_RECTS(pcl);
	for (ic = REGION_NUM_RECTS(pcl); --ic >= 0; p1box++)
	{

	    qdPPPix1chan( pGC, pBitMap, pDrawable, ch, 
		    MAX3( 0, xOrg, p1box->x1),
		    MAX3( 0, yOrg, p1box->y1),
		    MIN3( pDrawable->width, xOrg+dx, p1box->x2),
		    MIN3( pDrawable->height, yOrg+dy, p1box->y2),
		    xOrg, yOrg);
	}
    }
}


#define FATAL(x)    \
	{ fprintf(stderr, "fatal error in qdspans, %s\n", x); exit(1) }
#define PIXDEPTH(x) ((x->drawable).depth)

/*
 * box clipped by caller
 * bitmap could be clipped too
 */
static
qdPPPix1chan( pGC, pBitMap, pPixmap, ch, x1, y1, x2, y2, xOrg, yOrg)
    GCPtr		pGC;		/* note that clipping is already done */
    PixmapPtr		pBitMap;
    PixmapPtr		pPixmap;
    int			ch;		/* which group of 8 planes? */
    int			x1, y1, x2, y2;	/* clipping rectangle in pixmap coords*/
    int                 xOrg, yOrg;     /* origin of bitmap within pixmap */
{
#ifdef undef
    int			mask[];

    register int	chmask = 0xff << (ch*8);
    u_char *		destorig = QD_PIX_DATA(pPixmap)
				    [ nch * pPixmap->width * pPixmap->height]; 
    int			swlongs = 
		pBitMap->devKind>>2;	/* width of source bitmap in longs */
    unsigned *		psr;		/* pointer to bitmap source row */
    int			sr;		/* bitmap source row */
    int			sc;		/* bitmap source column */
    int			dr;		/* destination row */
    int			dc;		/* destination column */
    int			bit;		/* effectively, a boolean */
    char		bytetab[256];


    switch (pGC->fillStyle) {
      case FillOpaqueStippled:
      case FillStippled:
      case FillSolid:
        {
	initializeLookup( bytetab, pGC);/*accounts for output state in the GC*/

	/*
	 * for each row in source bitmap
	 */
	for (	sr = y1-yOrg, psr = (unsigned *)pBitmap->devPrivate+swlongs*sr;
		sr < y2-yOrg;
		sr++, psr += swlongs)
	    /*
	     * for each column in source bitmap
	     */
	    for ( sc=x1-xOrg; sc < x2-xOrg; sc++)
	    {
		/*
		 * examine each bit.  "mask" is from maskbits.c
		 */
		bit = psr[ sc>>5] & mask[ sc&0x1f] ? 1 : 0;
		/*
		 * apply alu function and write result
		 */
		pdr[ dc] = bytetab[ pdr[ dc]] [ bit];
	    }
	}
	break;
      case FillTiled:
	break;
    }
#endif
}


static
initializeLookup( bytetab, pGC)
    char	bytetab[];
    GCPtr               pGC;

{
}
#endif
