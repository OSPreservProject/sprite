/* $XConsortium: mfbpntwin.c,v 5.6 89/11/24 18:03:47 rws Exp $ */
/* Combined Purdue/PurduePlus patches, level 2.0, 1/17/89 */
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

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"

extern void miPaintWindow();

/*
NOTE
   PaintArea32() doesn't need to rotate the tile, since
mfbPositionWIndow() and mfbChangeWindowAttributes() do it.
*/

static void mfbPaintWindow32();

void
mfbPaintWindow(pWin, pRegion, what)
    WindowPtr	pWin;
    RegionPtr	pRegion;
    int		what;
{
    register mfbPrivWin	*pPrivWin;

    pPrivWin = (mfbPrivWin *)(pWin->devPrivates[mfbWindowPrivateIndex].ptr);
    
    switch (what) {
    case PW_BACKGROUND:
	switch (pWin->backgroundState) {
	case None:
	    return;
	case ParentRelative:
	    do {
		pWin = pWin->parent;
	    } while (pWin->backgroundState == ParentRelative);
	    (*pWin->drawable.pScreen->PaintWindowBackground)(pWin, pRegion,
							     what);
	    return;
	case BackgroundPixmap:
	    if (pPrivWin->fastBackground)
	    {
		mfbPaintWindow32(pWin, pRegion, what);
		return;
	    }
	    break;
	case BackgroundPixel:
	    if (pWin->background.pixel)
		mfbSolidWhiteArea(pWin, REGION_NUM_RECTS(pRegion),
				  REGION_RECTS(pRegion), GXset, NullPixmap);
	    else
		mfbSolidBlackArea(pWin, REGION_NUM_RECTS(pRegion),
				  REGION_RECTS(pRegion), GXclear, NullPixmap);
	    return;
    	}
    	break;
    case PW_BORDER:
	if (pWin->borderIsPixel)
	{
	    if (pWin->border.pixel)
		mfbSolidWhiteArea(pWin, REGION_NUM_RECTS(pRegion),
				  REGION_RECTS(pRegion), GXset, NullPixmap);
	    else
		mfbSolidBlackArea(pWin, REGION_NUM_RECTS(pRegion),
				  REGION_RECTS(pRegion), GXclear, NullPixmap);
	    return;
	}
	else if (pPrivWin->fastBorder)
	{
	    mfbPaintWindow32(pWin, pRegion, what);
	    return;
	}
	break;
    }
    miPaintWindow(pWin, pRegion, what);
}

/* Tile Window with a 32 bit wide tile 
   this could call mfbTileArea32, but that has to do a switch on the
rasterop, which seems expensive.
*/
static void
mfbPaintWindow32(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;		
{
    int nbox;		/* number of boxes to fill */
    register BoxPtr pbox;	/* pointer to list of boxes to fill */
    int tileHeight;	/* height of the tile */
    register int srcpix;	/* current row from tile */

    PixmapPtr pPixmap;
    int nlwScreen;	/* width in longwords of the screen's pixmap */
    int w;		/* width of current box */
    register int nlw;	/* loop version of nlwMiddle */
    register unsigned int *p;	/* pointer to bits we're writing */
    register int h;	/* height of current box */
    register int *psrc;	/* pointer to bits in tile */
    int startmask;
    int endmask;	/* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    int nlwExtra;	/* to get from right of box to left of next span */
    
    int y;		/* current scan line */

    unsigned int *pbits;	/* pointer to start of screen */
    mfbPrivWin *pPrivWin;

    pPrivWin = (mfbPrivWin *)(pWin->devPrivates[mfbWindowPrivateIndex].ptr);

    if (what == PW_BACKGROUND)
    {
	tileHeight = pWin->background.pixmap->drawable.height;
	pPixmap = pPrivWin->pRotatedBackground;
    } 
    else
    {
        tileHeight = pWin->border.pixmap->drawable.height;
	pPixmap = pPrivWin->pRotatedBorder;
    } 
    if (!pPixmap)
    {
	miPaintWindow(pWin, pRegion, what);
	return;
    }
    psrc = (int *)(pPixmap->devPrivate.ptr);

    pPixmap = (PixmapPtr)(pWin->drawable.pScreen->devPrivate);
    pbits = (unsigned int *)pPixmap->devPrivate.ptr;
    nlwScreen = (pPixmap->devKind) >> 2;
    nbox = REGION_NUM_RECTS(pRegion);
    pbox = REGION_RECTS(pRegion);

    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	y = pbox->y1;
	p = pbits + (pbox->y1 * nlwScreen) + (pbox->x1 >> 5);

	if ( ((pbox->x1 & 0x1f) + w) < 32)
	{
	    maskpartialbits(pbox->x1, w, startmask);
	    nlwExtra = nlwScreen;
	    while (h--)
	    {
		srcpix = psrc[y%tileHeight];
		y++;
		*p = (*p & ~startmask) | (srcpix & startmask);
		p += nlwExtra;
	    }
	}
	else
	{
	    maskbits(pbox->x1, w, startmask, endmask, nlwMiddle);
	    nlwExtra = nlwScreen - nlwMiddle;

	    if (startmask && endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    srcpix = psrc[y%tileHeight];
		    y++;
		    nlw = nlwMiddle;
		    *p = (*p & ~startmask) | (srcpix & startmask);
		    p++;
		    Duff (nlw, *p++ = srcpix );
		    *p = (*p & ~endmask) | (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else if (startmask && !endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    srcpix = psrc[y%tileHeight];
		    y++;
		    nlw = nlwMiddle;
		    *p = (*p & ~startmask) | (srcpix & startmask);
		    p++;
		    Duff (nlw, *p++ = srcpix );
		    p += nlwExtra;
		}
	    }
	    else if (!startmask && endmask)
	    {
		while (h--)
		{
		    srcpix = psrc[y%tileHeight];
		    y++;
		    nlw = nlwMiddle;
		    Duff (nlw, *p++ = srcpix);
		    *p = (*p & ~endmask) | (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else /* no ragged bits at either end */
	    {
		while (h--)
		{
		    srcpix = psrc[y%tileHeight];
		    y++;
		    nlw = nlwMiddle;
		    Duff (nlw, *p++ = srcpix);
		    p += nlwExtra;
		}
	    }
	}
        pbox++;
    }
}
