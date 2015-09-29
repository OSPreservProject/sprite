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
/* $XConsortium: cfbimage.c,v 1.5 89/11/25 12:26:41 rws Exp $ */

#include "X.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "cfb.h"
#include "cfbmskbits.h"
#include "servermd.h"

extern void miPutImage(), miGetImage(), mfbGetImage();

void
cfbPutImage(dst, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr dst;
    GCPtr	pGC;
    int		depth, x, y, w, h;
    int leftPad;
    unsigned int format;
    char 	*pImage;
{
    PixmapRec	FakePixmap;

    if ((w == 0) || (h == 0))
	return;

    if (format != XYPixmap)
    {
    	FakePixmap.drawable.type = DRAWABLE_PIXMAP;
    	FakePixmap.drawable.class = 0;
    	FakePixmap.drawable.pScreen = dst->pScreen;
    	FakePixmap.drawable.depth = depth;
    	FakePixmap.drawable.bitsPerPixel = depth;
    	FakePixmap.drawable.id = 0;
    	FakePixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
    	FakePixmap.drawable.x = 0;
    	FakePixmap.drawable.y = 0;
    	FakePixmap.drawable.width = w+leftPad;
    	FakePixmap.drawable.height = h;
    	FakePixmap.devKind = PixmapBytePad(FakePixmap.drawable.width, depth);
    	FakePixmap.refcnt = 1;
    	FakePixmap.devPrivate.ptr = (pointer)pImage;
    	((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->fExpose = FALSE;
	if (format == ZPixmap)
	    (void)(*pGC->ops->CopyArea)(&FakePixmap, dst, pGC, leftPad, 0,
					w, h, x, y);
	else
	    (void)(*pGC->ops->CopyPlane)(&FakePixmap, dst, pGC, leftPad, 0,
					 w, h, x, y, 1);
	((cfbPrivGC*)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->fExpose = TRUE;
    }
    else
    {
	miPutImage(dst, pGC, depth, x, y, w, h, leftPad, format,
		   (unsigned char *)pImage);
    }
}

void
cfbGetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine)
    DrawablePtr pDrawable;
    int		sx, sy, w, h;
    unsigned int format;
    unsigned long planeMask;
    pointer	pdstLine;
{
    PixmapRec	FakePixmap;
    BoxRec box;
    DDXPointRec ptSrc;
    RegionRec rgnDst;

    if ((w == 0) || (h == 0))
	return;
    if (pDrawable->depth == 1)
    {
	mfbGetImage(pDrawable, sx, sy, w, h, format, planeMask, pdstLine);
	return;
    }
    if (format == ZPixmap)
    {
    	FakePixmap.drawable.type = DRAWABLE_PIXMAP;
    	FakePixmap.drawable.class = 0;
    	FakePixmap.drawable.pScreen = pDrawable->pScreen;
    	FakePixmap.drawable.depth = pDrawable->depth;
    	FakePixmap.drawable.bitsPerPixel = pDrawable->bitsPerPixel;
    	FakePixmap.drawable.id = 0;
    	FakePixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
    	FakePixmap.drawable.x = 0;
    	FakePixmap.drawable.y = 0;
    	FakePixmap.drawable.width = w;
    	FakePixmap.drawable.height = h;
    	FakePixmap.devKind = PixmapBytePad(w, pDrawable->depth);
    	FakePixmap.refcnt = 1;
    	FakePixmap.devPrivate.ptr = (pointer)pdstLine;
	if ((planeMask & PMSK) != PMSK)
	    bzero((char *)pdstLine, FakePixmap.devKind * h);
        ptSrc.x = sx + pDrawable->x;
        ptSrc.y = sy + pDrawable->y;
        box.x1 = 0;
        box.y1 = 0;
        box.x2 = w;
        box.y2 = h;
        (*pDrawable->pScreen->RegionInit)(&rgnDst, &box, 1);
	cfbDoBitblt(pDrawable, (DrawablePtr)&FakePixmap, GXcopy, &rgnDst,
		    &ptSrc, planeMask);
        (*pDrawable->pScreen->RegionUninit)(&rgnDst);
    }
    else
    {
    	FakePixmap.drawable.type = DRAWABLE_PIXMAP;
    	FakePixmap.drawable.class = 0;
    	FakePixmap.drawable.pScreen = pDrawable->pScreen;
    	FakePixmap.drawable.depth = 1;
    	FakePixmap.drawable.bitsPerPixel = 1;
    	FakePixmap.drawable.id = 0;
    	FakePixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
    	FakePixmap.drawable.x = 0;
    	FakePixmap.drawable.y = 0;
    	FakePixmap.drawable.width = w;
    	FakePixmap.drawable.height = h;
    	FakePixmap.devKind = PixmapBytePad(w, 1);
    	FakePixmap.refcnt = 1;
    	FakePixmap.devPrivate.ptr = (pointer)pdstLine;
        ptSrc.x = sx + pDrawable->x;
        ptSrc.y = sy + pDrawable->y;
        box.x1 = 0;
        box.y1 = 0;
        box.x2 = w;
        box.y2 = h;
        (*pDrawable->pScreen->RegionInit)(&rgnDst, &box, 1);
	cfbCopyImagePlane (pDrawable, (DrawablePtr)&FakePixmap, GXcopy, &rgnDst,
		    &ptSrc, planeMask);
        (*pDrawable->pScreen->RegionUninit)(&rgnDst);
    }
}
