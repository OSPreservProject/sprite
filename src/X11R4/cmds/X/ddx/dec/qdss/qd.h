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

#ifndef QD_H
#define QD_H

#include "pixmapstr.h"
#include "regionstr.h"

extern RegionPtr mfbPixmapToRegion();

extern Bool qdScreenInit();

extern void	qdSaveAreas ();
extern void	qdRestoreAreas ();

extern int	DragonPix;	/* max y size for offscreen pixmap */

/*
 * On the VAX with pcc
 *      (unsigned)0xffffffff % 2 == 0xffffffff
 * UMOD does not have a discontinuity at 0, as % does
 */
#define UMOD( dend, dor) \
        ( (dend)%(dor) + (((dend) & 0x80000000) ? (dor) : 0))

/*
 * private data in a PIXMAP
 * NOTE: format is compatible with mfb pixmaps
 */
typedef struct _QDPix {
    PixmapRec pixmap;
#ifndef X11R4
    DDXPointRec		offscreen;	/* x,y at offscreen location */
#endif
    int planes;
    struct _QDPix *nextLRU, *prevLRU;
    struct PixSlot *slot;
} QDPixRec, *QDPixPtr;
#define	NOTOFFSCREEN	0 /* offscreen.y == 0 if not offscreen */

/* If PixmapIsLocked cannot be moved to offscreen.
 * A future version might also disallow moving *from* offscreen memory. */
#ifdef X11R4
#define PixmapIsLocked(pix) ((pix)->pixmap.drawable.class)
#define LockPixmap(pix) ((pix)->pixmap.drawable.class = 1)
#define UnlockPixmap(pix) ((pix)->pixmap.drawable.class = 0)
#else
#define PixmapIsLocked(pix) ((pix)->offscreen.x < 0)
#define LockPixmap(pix) ((pix)->offscreen.x = -1)
#define UnlockPixmap(pix) ((pix)->offscreen.x = 0)
#endif

#ifdef X11R4
/* "data" must be unsigned to to point to bytes in full-depth image */
#define QD_PIX_DATA(p) ((unsigned char*)(p)->devPrivate.ptr)
#define QDWIN_X(win) (win)->drawable.x
#define QDWIN_Y(win) (win)->drawable.y
#define QDWIN_WINSIZE(win) &(win)->winSize
#define QDWIN_CLIPLIST(win) &(win)->clipList
#define QDPIX_WIDTH(pix)  (pix)->drawable.width
#define QDPIX_HEIGHT(pix) (pix)->drawable.height
#define QDPIX_X(qpix) (qpix)->pixmap.drawable.x
#define QDPIX_Y(qpix) (qpix)->pixmap.drawable.y
#else /* R3 */
#define QD_PIX_DATA(p) ((unsigned char*)(p)->devPrivate)
#define QDWIN_X(win) (win)->absCorner.x
#define QDWIN_Y(win) (win)->absCorner.y
#define QDWIN_WINSIZE(win) (win)->winSize
#define QDWIN_CLIPLIST(win) (win)->clipList
#define QDPIX_WIDTH(pix)  (pix)->width
#define QDPIX_HEIGHT(pix) (pix)->height
#define QDPIX_X(qpix) (qpix)->offscreen.x
#define QDPIX_Y(qpix) (qpix)->offscreen.y
#endif
typedef QDPixRec QDPixmapRec;
typedef QDPixPtr QDPixmapPtr;
#define QD_PIX(pix) ((QDPixPtr)(pix))

/*
 * There is only one representation for depth 1 pixmaps (bitmaps).
 *
 * Pad out to 32-bit boundary, like monochrome servers,
 * even though the dragon DMA engine can transfer 16-bit-padded bitmaps.
 */
/*
 * A convenient macro which incorporates the bitmap padding rule:
 */
#define QPPADBYTES( w)	( ((w)+31)>>3 & ~3)	/* return units of bytes */

/*
 * There is only one representation for full-depth pixmaps.
 * This is used when the "format"
 * argument to CreatePixmap() is either XYPixmap or ZPixmap.
 * We can do this because DDX's representation of the pixmap data is hidden
 * DIX code.
 *
 * Pixel data is stored in the order that is natural for
 * the Dragon's DMA engine in Z mode: red bytes, green bytes, then blue bytes.
 * Note that there are no padding bytes whatever.
 */


#if NPLANES==24
#define RED(p)		(p & 0xff)
#define GREEN(p)	(p>>8 & 0xff)
#define BLUE(p)		(p>>16 & 0xff)
#define	TORED(p)	(p & 0xff)
#define	TOGREEN(p)	(p<<8 & 0xff00)
#define	TOBLUE(p)	(((unsigned long) p)<<16 & 0xff0000)
#else	/* NPLANES == 8 */
#define RED(p)		(p & 0xff)
#define GREEN(p)	(p & 0xff)
#define BLUE(p)		(p & 0xff)
#define	TORED(p)	(p & 0xff)
#define	TOGREEN(p)	(p & 0xff)
#define	TOBLUE(p)	(p & 0xff)
#endif


/*
 * For fast text output, we need both a depth 1 pixmap containing
 * the strike-order font, and a horizontal offset table.
 */
typedef struct {
    PixmapPtr	pPixmap;
    int		log2dx;		/* log2 of x dimension of all char cells */
    short	hasKerning;	/* 1 if kerns (=> unsuitable for ImageText) */
    short	leftKern;	/* maximum left kerning (or 0) */
    int		width;		/* "maximum" width */
    short	ascent, descent;
} QDFontRec, *QDFontPtr;

#define QDSLOWFONT	1	/* a QDFontPtr may take this value */

#endif	/* QD_H */

/* ISDRAGONTILE
 *	sets (int) pow2 = 1 if the pixmap is in [4,512] and a power of 2
 *	in width and height.
 */
#define	ISDRAGONTILE(ppixmap,pow2)	\
{	\
	register int	shifted;	\
	pow2 = 1;	/* Yes, is power of 2 */	\
	/* check if tile w,h (belong-to) [4,512] */	\
	if (QDPIX_WIDTH(ppixmap) > 512 || QDPIX_HEIGHT(ppixmap) > 512 || \
		QDPIX_WIDTH(ppixmap) < 4 || QDPIX_HEIGHT(ppixmap) < 4)  \
	    pow2 = 0;	\
	for (shifted = 1; (QDPIX_WIDTH(ppixmap) >> shifted) > 0;	\
		shifted += 1)	\
	{	\
	    if ((QDPIX_WIDTH(ppixmap) >> shifted) << shifted	\
		    != QDPIX_WIDTH(ppixmap))	\
		pow2 = 0;	\
	}	\
	for (shifted = 1; (QDPIX_HEIGHT(ppixmap) >> shifted) > 0;	\
		shifted += 1)	\
	{	\
	    if ((QDPIX_HEIGHT(ppixmap) >> shifted) << shifted	\
		    != QDPIX_HEIGHT(ppixmap))	\
		pow2 = 0;	\
	}	\
}

/*
 * These two macros are used for a destination that is an offscreen Bitmap.
 * The GC is temporarily patched.
 */
#define SETUP_PIXMAP_AS_WINDOW(pDraw, pGC) \
    extern unsigned int Allplanes;\
    unsigned long planemaskSave = pGC->planemask;\
    int fgPixelSave = pGC->fgPixel;\
    int bgPixelSave = pGC->bgPixel;\
    if ((pDraw)->depth == 1) {\
	pGC->fgPixel = pGC->fgPixel ? Allplanes : 0;\
	pGC->bgPixel = pGC->bgPixel ? Allplanes : 0;\
    }\
    pGC->planemask = ((QDPixPtr)pDraw)->planes;

#define CLEANUP_PIXMAP_AS_WINDOW(pGC) \
    pGC->planemask = planemaskSave;\
    pGC->fgPixel = fgPixelSave;\
    pGC->bgPixel = bgPixelSave;

#ifndef X11R4
/* to ease the conversion R3->R4 */
#define REGION_NUM_RECTS(reg) ((reg)->numRects)
#define REGION_RECTS(reg) ((reg)->rects)
#endif
