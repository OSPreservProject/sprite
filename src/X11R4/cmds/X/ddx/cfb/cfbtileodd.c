/*
 * Fill odd tiled rectangles.  Used by both PolyFillRect and PaintWindow.
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

/* $XConsortium: cfbtileodd.c,v 1.5 89/12/13 14:34:01 keith Exp $ */

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
#include "cfb8bit.h"

#define LastTileBits {\
    tmp = bits; \
    if (tileEndPart) \
	bits = (*pSrc & tileEndMask) | BitRight (*pSrcLine, tileEndLeftShift); \
    else \
	bits = *pSrc; \
}

#define ResetTileBits {\
    pSrc = pSrcLine; \
    nlwSrc = widthSrc;\
    if (tileEndPart) { \
	if (PPW - xoff + tileEndPart <= PPW) {\
	    bits = *pSrc++; \
	    nlwSrc--; \
	} else \
	    bits = BitLeft(tmp, tileEndLeftShift) | \
		   BitRight(bits, tileEndRightShift); \
	xoff = (xoff + xoffStep) & PIM; \
	leftShift = xoff << (5-PWSH); \
	rightShift = 32 - leftShift; \
    }\
}

#define NextTileBits {\
    if (nlwSrc == 1) {\
	LastTileBits\
    } else { \
    	if (nlwSrc == 0) {\
	    ResetTileBits\
    	} \
	tmp = bits; \
	bits = *pSrc++; \
    }\
    nlwSrc--; \
}

void
cfbFillBoxTileOdd (pDrawable, nBox, pBox, tile, xrot, yrot)
    DrawablePtr	    pDrawable;
    int		    nBox;	/* number of boxes to fill */
    register BoxPtr pBox;	/* pointer to list of boxes to fill */
    PixmapPtr	    tile;	/* tile */
    int		    xrot, yrot;
{
    int tileWidth;	/* width of tile in pixels */
    int tileHeight;	/* height of the tile */
    int widthSrc;

    int widthDst;		/* width in longwords of the dest pixmap */
    int w;		/* width of current box */
    int h;		/* height of current box */
    unsigned long startmask;
    unsigned long endmask;	/* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    int nlwSrc;		/* number of whole longwords in source */
    
    register int nlw;	/* loop version of nlwMiddle */
    int srcy;		/* current tile y position */
    int srcx;		/* current tile x position */
    int xoffDst, xoffSrc;
    int leftShift, rightShift;

    unsigned long *pDstBase;	/* pointer to start of dest */
    unsigned long *pDstLine;	/* poitner to start of dest box */
    unsigned long *pSrcBase;	/* pointer to start of source */
    unsigned long *pSrcLine;	/* pointer to start of source line */
    register unsigned long *pDst;
    register unsigned long *pSrc;
    register unsigned long bits, tmp;
    int xoffStart, xoff;
    int leftShiftStart, rightShiftStart, nlwSrcStart;
    unsigned long tileEndMask;
    int tileEndLeftShift, tileEndRightShift;
    int	xoffStep;
    int tileEndPart;
    int needFirst;
    unsigned long   narrow[2];
    unsigned long   narrowMask;
    int	    narrowShift;
    Bool    narrowTile;

    tileHeight = tile->drawable.height;
    tileWidth = tile->drawable.width;
    widthSrc = tile->devKind >> 2;
    narrowTile = FALSE;
    if (widthSrc == 1)
    {
	narrowShift = tileWidth;
	narrowMask = cfbendpartial [tileWidth];
	tileWidth *= 2;
	widthSrc = 2;
	narrowTile = TRUE;
    }
    pSrcBase = (unsigned long *)tile->devPrivate.ptr;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pDstBase = (unsigned long *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthDst = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	pDstBase = (unsigned long *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    tileEndPart = tileWidth & PIM;
    tileEndMask = cfbendpartial[tileEndPart];
    tileEndLeftShift = (tileEndPart) << (5-PWSH);
    tileEndRightShift = 32 - tileEndLeftShift;
    xoffStep = PPW - tileEndPart;
    /*
     * current assumptions: tile > 32 bits wide.
     */
    while (nBox--)
    {
	w = pBox->x2 - pBox->x1;
	h = pBox->y2 - pBox->y1;
	srcx = (pBox->x1 - xrot) % tileWidth;
	srcy = (pBox->y1 - yrot) % tileHeight;
	xoffDst = pBox->x1 & PIM;
	if (xoffDst + w < PPW)
	{
	    maskpartialbits(pBox->x1, w, startmask);
	    endmask = 0;
	    nlwMiddle = 0;
	}
	else
	{
	    maskbits (pBox->x1, w, startmask, endmask, nlwMiddle)
	}
	pDstLine = pDstBase + (pBox->y1 * widthDst) + (pBox->x1 >> PWSH);
	pSrcLine = pSrcBase + (srcy * widthSrc);
	xoffSrc = srcx & PIM;
	if (xoffSrc >= xoffDst)
	{
	    xoffStart = xoffSrc - xoffDst;
	    needFirst = 1;
	}
	else
	{
	    xoffStart = PPW - (xoffDst - xoffSrc);
	    needFirst = 0;
	}
	leftShiftStart = (xoffStart) << (5-PWSH);
	rightShiftStart = 32 - leftShiftStart;
	nlwSrcStart = widthSrc - (srcx >> PWSH);
	while (h--)
	{
	    /* XXX only works when narrowShift >= PPW/2 */
	    if (narrowTile)
	    {
		tmp = pSrcLine[0] & narrowMask;
		narrow[0] = tmp | SCRRIGHT (tmp, narrowShift);
		narrow[1] = SCRLEFT (tmp, PPW - narrowShift) |
			    SCRRIGHT(tmp, 2 * narrowShift - PPW);
		pSrcLine = narrow;
	    }
	    xoff = xoffStart;
	    leftShift = leftShiftStart;
	    rightShift = rightShiftStart;
	    nlwSrc = nlwSrcStart;
	    pSrc = pSrcLine + (srcx >> PWSH);
	    pDst = pDstLine;
	    bits = 0;
	    if (needFirst)
	    {
		NextTileBits
	    }
	    if (startmask)
	    {
		NextTileBits
		tmp = BitLeft(tmp, leftShift);
 		if (rightShift != 32)
		    tmp |= BitRight(bits,rightShift);
		*pDst = *pDst & ~startmask | tmp & startmask;
		++pDst;
	    }
	    nlw = nlwMiddle;
	    if (tileEndPart)
	    {
	    	while (nlw--)
	    	{
		    NextTileBits
		    if (rightShift != 32)
		    	*pDst++ = BitLeft(tmp, leftShift) |
			      	  BitRight(bits, rightShift);
		    else
		    	*pDst++ = tmp;
	    	}
	    }
	    else
	    {
		if (leftShift)
		{
		    while (nlw--)
		    {
		    	if (nlwSrc == 0)
		    	{
			    nlwSrc = widthSrc;
			    pSrc = pSrcLine;
		    	}
		    	tmp = bits;
		    	bits = *pSrc++;
			nlwSrc--;
			*pDst++ = BitLeft(tmp, leftShift) |
				  BitRight(bits, rightShift);
		    }
		}
		else
		{
		    while (nlw--)
		    {
		    	if (nlwSrc == 0)
		    	{
			    nlwSrc = widthSrc;
			    pSrc = pSrcLine;
		    	}
			*pDst++ = bits;
		    	bits = *pSrc++;
			nlwSrc--;
		    }
		}
	    }
	    if (endmask)
	    {
		NextTileBits
		if (rightShift == 32)
		    bits = 0;
		*pDst = *pDst & ~endmask |
			 (BitLeft(tmp, leftShift) |
			  BitRight(bits,rightShift)) & endmask;
	    }
	    pDstLine += widthDst;
	    pSrcLine += widthSrc;
	    if (++srcy == tileHeight)
	    {
		srcy = 0;
		pSrcLine = pSrcBase;
	    }
	}
	pBox++;
    }
}
