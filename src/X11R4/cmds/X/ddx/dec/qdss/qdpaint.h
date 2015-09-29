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

#include "scrnintstr.h"

/*
 * this will be the general output writing mode
 */
#if NPLANES==24
# define PAINT_TYPE      X3_PAINT_RGB
#else	/* NPLANES==8 */
# define PAINT_TYPE      X3_PAINT_GREY
#endif

extern PaintGC *pPaintGC;

/*
 * The point structures are actually the same
 */
#define DDXPointsToPaintPoints( pXPt, pPaintPt)  \
	(pPaintPt) = (X3ScreenPt *)(pXPt);

#if NPLANES==24
# define XpixvalTopaintRGBpixel( xpix, RGB)  \
    (RGB).red =   RED(xpix);  \
    (RGB).green = GREEN(xpix);  \
    (RGB).blue =  BLUE(xpix);
#else	/* NPLANES==8 */
# define XpixvalTopaintRGBpixel( xpix, RGB)  \
    (RGB).red =	  (xpix) & 0x0000ff;  \
    (RGB).green = (xpix) & 0x0000ff;  \
    (RGB).blue =  (xpix) & 0x0000ff;
#endif

/*
 * state setting macros
 */
#if NPLANES==24

# define SETPIXVAL( pPaintGC, xpix) \
    { \
	RGBpixel	paintpixel; \
 \
	paintpixel.red = RED( xpix); \
	paintpixel.green = GREEN( xpix); \
	paintpixel.blue = BLUE( xpix); \
	X3SETTypedPixval( (pPaintGC), PAINT_TYPE, &paintpixel); \
	((QzQdGC *)(pPaintGC)->devptr)->r = pPaintGC->DrawInfo.pixval.red; \
	((QzQdGC *)(pPaintGC)->devptr)->g = pPaintGC->DrawInfo.pixval.green; \
	((QzQdGC *)(pPaintGC)->devptr)->b = pPaintGC->DrawInfo.pixval.blue; \
    }
# define SETPAINTGCMASK( pXGC, pPGC) \
	((QzQdGC *)(pPGC)->devptr)->rmask = RED( (pXGC)->planemask); \
	((QzQdGC *)(pPGC)->devptr)->gmask = GREEN( (pXGC)->planemask); \
	((QzQdGC *)(pPGC)->devptr)->bmask = BLUE( (pXGC)->planemask);

#else /* NPLANES==8 */

# define SETPIXVAL( pPaintGC, xpix) \
    { \
	X3SETTypedPixval( (pPaintGC), PAINT_TYPE, &xpix); \
	((QzQdGC *)(pPaintGC)->devptr)->r = \
	((QzQdGC *)(pPaintGC)->devptr)->g = \
	((QzQdGC *)(pPaintGC)->devptr)->b = *((char *) &xpix); \
    }
# define SETPAINTGCMASK( pXGC, pPGC) \
	((QzQdGC *)(pPGC)->devptr)->gmask = ( (pXGC)->planemask); \

#endif


/*
 * Use
 *	X3SETUsrSrcPixelType( (pPGC), X3_PAINT_NULL) 
 * when paint routine should get pixel values out of the GC
 */

/* fills in the tile filed of a pixmap and returns it */
extern int SetTile(/* pPixmap */);

/* state setting macro
 *
 * copy relevant parts of X11 GC to a paintGC
 */
/*
 * The extreme ends of the hardware color map are unusable, therefore the
 * GXclear and GXset rasterops must be coerced into GXcopy operations
 * with the appropriate constant source.
 *
 * According to the definition of rasterops, GXclear and GXset should
 * produce absolute black and white, rather than BlackPixel and WhitePixel,
 * but this would make Alis look bad, hence the hack.
 */

#define SETPAINTGC( pXGC, pPGC, transx, transy) \
    { \
	X3ScreenRect *	pP; \
	RGBpixel	forergb; \
	RGBpixel	backrgb; \
	  \
	XpixvalTopaintRGBpixel( pXGC->fgPixel, forergb); \
	XpixvalTopaintRGBpixel( pXGC->bgPixel, backrgb); \
	/* \
	 * foreground pixel \
	 */ \
	SETPIXVAL((pPGC), pXGC->fgPixel); \
	/* \
	 * background pixel \
	 */ \
	((QzQdGC *)(pPGC)->devptr)->backpixval.red = backrgb.red; \
	((QzQdGC *)(pPGC)->devptr)->backpixval.green = backrgb.green; \
	((QzQdGC *)(pPGC)->devptr)->backpixval.blue = backrgb.blue; \
	/* \
	 * clipping state \
	 */ \
	pP = (X3ScreenRect *) alloca( \
	    ((QDPrivGCPtr)(pXGC)->devPriv)->pCompositeClip->size \
						* sizeof(X3ScreenRect)); \
	x11clipTopaintclip( \
		((QDPrivGCPtr)(pXGC)->devPriv)->pCompositeClip->rects, pP, \
		((QDPrivGCPtr)(pXGC)->devPriv)->pCompositeClip->numRects); \
	X3SETClip( (pPGC), \
		((QDPrivGCPtr)(pXGC)->devPriv)->pCompositeClip->numRects, pP);\
	/* \
	 * translate point \
	 */ \
	pPaintGC->Window.translate.y = transy; \
	pPaintGC->Window.translate.x = transx; \
	/* \
	 * update method - X11 GX codes are the same as X10 \
	 */ \
	if ( (pXGC->alu) == GXclear)  \
	{  \
	    SETPIXVAL((pPGC), (pXGC)->pScreen->blackPixel); \
	    X3SETPaintMethod((pPGC), umtable[GXcopy]);  \
	}  \
	else if ( (pXGC->alu) == GXset)  \
	{  \
	    SETPIXVAL((pPGC), (pXGC)->pScreen->whitePixel); \
	    X3SETPaintMethod((pPGC), umtable[GXcopy]);  \
	}  \
	else X3SETPaintMethod((pPGC), umtable[(pXGC->alu)]); \
	/*  \
	 * previously called: QzQdValidateGC( pPGC); \
	 * plane mask  \
	 * \
	 * unfortunately QzQdValidateGC doesn't allow single planes to be \
	 * masked individually \
	 */ \
	SETPAINTGCMASK( pXGC, pPGC) \
    }
