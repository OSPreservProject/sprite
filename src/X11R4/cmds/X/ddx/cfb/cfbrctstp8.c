/*
 * Fill 32 bit stippled rectangles for 8 bit frame buffers
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

Author: Keith Packard, MIT X Consortium

*/

/* $XConsortium: cfbrctstp8.c,v 1.11 90/02/09 13:12:34 rws Exp $ */

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

#if (PPW == 4)

void
cfb8FillBoxOpaqueStippled32 (pDrawable, nBox, pBox, stipple, fg, bg)
    DrawablePtr	    pDrawable;
    int		    nBox;	/* number of boxes to fill */
    register BoxPtr pBox;	/* pointer to list of boxes to fill */
    PixmapPtr	    stipple;	/* rotated, expanded stipple */
    unsigned long   fg, bg;	/* pixel values */
{
    int *psrc;		/* pointer to bits in stipple, if needed */
    int stippleHeight;	/* height of the stipple */

    int nlwDst;		/* width in longwords of the dest pixmap */
    int w;		/* width of current box */
    register int h;	/* height of current box */
    unsigned long startmask;
    unsigned long endmask;	/* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    register int nlw;			/* loop version of nlwMiddle */
    unsigned long *dstLine;
    register unsigned long *dst;	/* pointer to bits we're writing */
    int y;				/* current scan line */
    int srcy;				/* current stipple position */

    unsigned long *pbits;/* pointer to start of pixmap */
    register unsigned long pixels;	/* bits to write */
    register unsigned long bits;	/* bits from stipple */
    int		  firstStart, lastStop;
    int		    i;

    if (!cfb8CheckPixels (fg, bg))
	cfb8SetPixels (fg, bg);

    stippleHeight = stipple->drawable.height;
    psrc = (int *)stipple->devPrivate.ptr;

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
	dstLine = pbits + (pBox->y1 * nlwDst) + ((pBox->x1 & ~31) >> PWSH);
	if (((pBox->x1 & PIM) + w) <= PPW)
	{
	    maskpartialbits(pBox->x1, w, startmask);
	    endmask = 0;
	}
	else
	{
	    mask32bits (pBox->x1, w, startmask, endmask);
	}
	nlwMiddle = ((((pBox->x2 - 1) | 31) + 1) -  (pBox->x1 & ~31)) >> 5;
	firstStart = (pBox->x1 & 31) >> PWSH;
	lastStop = ((pBox->x2 - 1) & 31) >> PWSH;
	srcy = y % stippleHeight;
	while (h--)
	{
	    bits = psrc[srcy];
	    for (i = 0; i < 8; i++)
	    {
		pixels = GetFourPixels (bits);
		dst = dstLine + i;
		nlw = nlwMiddle;
		if (i < firstStart)
		{
		    dst += 8;
		    nlw--;
		}
		else if (i == firstStart && startmask)
		{
		    *dst = (*dst & ~startmask) | (pixels & startmask);
		    dst += 8;
		    nlw--;
		}
		if (i > lastStop || i == lastStop && endmask)
		    nlw--;
		while (nlw--)
		{
		    *dst = pixels;
		    dst += 8;
		}
		if (i == lastStop && endmask)
		    *dst = (*dst & ~endmask) | (pixels & endmask);
		NextFourBits (bits);
	    }
	    dstLine += nlwDst;
	    srcy++;
	    if (srcy == stippleHeight)
		srcy = 0;
	}
        pBox++;
    }
}

void
cfb8FillBoxTransparentStippled32 (pDrawable, nBox, pBox, stipple, fg)
    DrawablePtr	    pDrawable;
    int		    nBox;	/* number of boxes to fill */
    BoxPtr 	    pBox;	/* pointer to list of boxes to fill */
    PixmapPtr	    stipple;	/* rotated, expanded stipple */
    unsigned long   fg;		/* pixel value */
{
    int *psrc;		/* pointer to bits in stipple, if needed */
    int stippleHeight;	/* height of the stipple */

    int nlwDst;		/* width in longwords of the dest pixmap */
    int w;		/* width of current box */
    register int h;	/* height of current box */
    unsigned long startmask;
    unsigned long endmask;	/* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    register int nlw;			/* loop version of nlwMiddle */
    unsigned long *dstLine;
    register unsigned long *dst;	/* pointer to bits we're writing */
    int y;				/* current scan line */
    int srcy;				/* current stipple position */

    unsigned long *pbits;/* pointer to start of pixmap */
    register unsigned long pixels;	/* bits to write */
    register unsigned long bits;	/* bits from stipple */
    register unsigned long   mask;
    int		  firstStart, lastStop;
    int		    i, maxi;

    fg = PFILL (fg);

    stippleHeight = stipple->drawable.height;
    psrc = (int *)stipple->devPrivate.ptr;

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
    	dstLine = pbits + (pBox->y1 * nlwDst) + ((pBox->x1 & ~31) >> PWSH);
	if (((pBox->x1 & PIM) + w) <= PPW)
	{
	    maskpartialbits(pBox->x1, w, startmask);
	    endmask = 0;
	}
	else
	{
	    mask32bits (pBox->x1, w, startmask, endmask);
	}
    	nlwMiddle = ((((pBox->x2 - 1) | 31) + 1) -  (pBox->x1 & ~31)) >> 5;
    	firstStart = (pBox->x1 & 31) >> PWSH;
    	lastStop = ((pBox->x2 - 1) & 31) >> PWSH;
    	srcy = y % stippleHeight;
	maxi = 8;
	if (maxi > nlwDst)
	    maxi = nlwDst;
    	while (h--)
    	{
	    bits = psrc[srcy];
	    for (i = 0; i < maxi; i++)
	    {
	    	mask = cfb8PixelMasks[GetFourBits (bits)];
		pixels = fg & mask;
	    	dst = dstLine + i;
	    	nlw = nlwMiddle;
	    	if (i < firstStart)
	    	{
		    dst += 8;
		    nlw--;
	    	}
	    	else if (i == firstStart && startmask)
	    	{
		    *dst = (*dst & ~(mask&startmask)) |
			    (pixels & startmask);
		    dst += 8;
		    nlw--;
	    	}
	    	if (i > lastStop || i == lastStop && endmask)
		    nlw--;
#ifdef AVOID_MEMORY_READ
#define BitLoop(body) \
    while (nlw--) {\
	body \
	dst += 8; \
    }
#if (BITMAP_BIT_ORDER == MSBFirst)
		switch (GetFourBits(bits)) {
	    	case 0:
	    	    break;
	    	case 1:
	    	    BitLoop (((char *) (dst))[3] = (fg);)
	    	    break;
	    	case 2:
	    	    BitLoop (((char *) (dst))[2] = (fg);)
	    	    break;
	    	case 3:
	    	    BitLoop (((short *) (dst))[1] = (fg);)
	    	    break;
	    	case 4:
	    	    BitLoop (((char *) (dst))[1] = (fg);)
	    	    break;
	    	case 5:
	    	    BitLoop (((char *) (dst))[3] = (fg);
			     ((char *) (dst))[1] = (fg);)
	    	    break;
	    	case 6:
	    	    BitLoop (((char *) (dst))[2] = (fg);
			     ((char *) (dst))[1] = (fg);)
	    	    break;
	    	case 7:
	    	    BitLoop (((short *) (dst))[1] = (fg);
			     ((char *) (dst))[1] = (fg);)
	    	    break;
	    	case 8:
	    	    BitLoop (((char *) (dst))[0] = (fg);)
	    	    break;
	    	case 9:
	    	    BitLoop (((char *) (dst))[3] = (fg);
			     ((char *) (dst))[0] = (fg);)
	    	    break;
	    	case 10:
	    	    BitLoop (((char *) (dst))[2] = (fg);
			     ((char *) (dst))[0] = (fg);)
	    	    break;
	    	case 11:
	    	    BitLoop (((short *) (dst))[1] = (fg);
			     ((char *) (dst))[0] = (fg);)
	    	    break;
	    	case 12:
	    	    BitLoop (((short *) (dst))[0] = (fg);)
	    	    break;
	    	case 13:
	    	    BitLoop (((char *) (dst))[3] = (fg);
			     ((short *) (dst))[0] = (fg);)
	    	    break;
	    	case 14:
	    	    BitLoop (((char *) (dst))[2] = (fg);
			     ((short *) (dst))[0] = (fg);)
	    	    break;
	    	case 15:
	    	    BitLoop (((long *) (dst))[0] = (fg);)
	    	    break;
		}
#else
	    	switch (GetFourBits(bits)) {
	    	case 0:
	    	    break;
	    	case 1:
	    	    BitLoop (((char *) (dst))[0] = (fg);)
	    	    break;
	    	case 2:
	    	    BitLoop (((char *) (dst))[1] = (fg);)
	    	    break;
	    	case 3:
	    	    BitLoop (((short *) (dst))[0] = (fg);)
	    	    break;
	    	case 4:
	    	    BitLoop (((char *) (dst))[2] = (fg);)
	    	    break;
	    	case 5:
	    	    BitLoop (((char *) (dst))[0] = (fg);
			     ((char *) (dst))[2] = (fg);)
	    	    break;
	    	case 6:
	    	    BitLoop (((char *) (dst))[1] = (fg);
			     ((char *) (dst))[2] = (fg);)
	    	    break;
	    	case 7:
	    	    BitLoop (((short *) (dst))[0] = (fg);
			     ((char *) (dst))[2] = (fg);)
	    	    break;
	    	case 8:
	    	    BitLoop (((char *) (dst))[3] = (fg);)
	    	    break;
	    	case 9:
	    	    BitLoop (((char *) (dst))[0] = (fg);
			     ((char *) (dst))[3] = (fg);)
	    	    break;
	    	case 10:
	    	    BitLoop (((char *) (dst))[1] = (fg);
			     ((char *) (dst))[3] = (fg);)
	    	    break;
	    	case 11:
	    	    BitLoop (((short *) (dst))[0] = (fg);
			     ((char *) (dst))[3] = (fg);)
	    	    break;
	    	case 12:
	    	    BitLoop (((short *) (dst))[1] = (fg);)
	    	    break;
	    	case 13:
	    	    BitLoop (((char *) (dst))[0] = (fg);
			     ((short *) (dst))[1] = (fg);)
	    	    break;
	    	case 14:
	    	    BitLoop (((char *) (dst))[1] = (fg);
			     ((short *) (dst))[1] = (fg);)
	    	    break;
	    	case 15:
	    	    BitLoop (((long *) (dst))[0] = (fg);)
	    	    break;
	    	}
#endif
#else
	    	while (nlw--)
	    	{
		    *dst = (*dst & ~mask) | pixels;
		    dst += 8;
	    	}
#endif
	    	if (i == lastStop && endmask)
		    *dst = (*dst & ~(mask &endmask)) |
			    (pixels & endmask);
	    	NextFourBits (bits);
	    }
	    dstLine += nlwDst;
	    srcy++;
	    if (srcy == stippleHeight)
	    	srcy = 0;
    	}
    	pBox++;
    }
}

#endif
