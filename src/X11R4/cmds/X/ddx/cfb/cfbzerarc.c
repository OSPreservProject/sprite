/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of MIT not be used in
advertising or publicity pertaining to distribution of the
software without specific prior written permission.
M.I.T. makes no representation about the suitability of
this software for any purpose. It is provided "as is"
without any express or implied warranty.

********************************************************/

/* $XConsortium: cfbzerarc.c,v 5.16 89/11/25 15:22:46 rws Exp $ */

/* Derived from:
 * "Algorithm for drawing ellipses or hyperbolae with a digital plotter"
 * by M. L. V. Pitteway
 * The Computer Journal, November 1967, Volume 10, Number 3, pp. 282-289
 */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "cfb.h"
#include "cfbmskbits.h"
#include "mizerarc.h"

#if PPW == 4

extern void miPolyArc(), miZeroPolyArc();

static void
cfbZeroArcSS8Copy(pDraw, pGC, arc)
    DrawablePtr pDraw;
    GCPtr pGC;
    xArc *arc;
{
    miZeroArcRec info;
    Bool do360;
    register int x;
    char *addrb;
    register char *yorgb, *yorgob;
    register unsigned long pixel = pGC->fgPixel;
    register int yoffset;
    int nlwidth, dyoffset;
    register int y, a, b, d, mask;
    register int k1, k3, dx, dy;

    if (pDraw->type == DRAWABLE_WINDOW)
    {
	addrb = (char *)
		(((PixmapPtr)(pDraw->pScreen->devPrivate))->devPrivate.ptr);
	nlwidth = (int)
		(((PixmapPtr)(pDraw->pScreen->devPrivate))->devKind);
    }
    else
    {
	addrb = (char *)(((PixmapPtr)pDraw)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDraw)->devKind);
    }
    do360 = miZeroArcSetup(arc, &info, TRUE);
    yorgb = addrb + ((info.yorg + pDraw->y) * nlwidth);
    yorgob = addrb + ((info.yorgo + pDraw->y) * nlwidth);
    info.xorg += pDraw->x;
    info.xorgo += pDraw->x;
    MIARCSETUP();
    yoffset = y ? nlwidth : 0;
    dyoffset = 0;
    mask = info.initialMask;
    if (!(arc->width & 1))
    {
	if (mask & 2)
	    *(yorgb + info.xorgo) = pixel;
	if (mask & 8)
	    *(yorgob + info.xorgo) = pixel;
    }
    if (!info.end.x || !info.end.y)
    {
	mask = info.end.mask;
	info.end = info.altend;
    }
    if (do360 && (arc->width == arc->height) && !(arc->width & 1))
    {
	register int xoffset = nlwidth;
	char *yorghb = yorgb + (info.h * nlwidth) + info.xorg;
	char *yorgohb = yorghb - info.h;

	yorgb += info.xorg;
	yorgob += info.xorg;
	yorghb += info.h;
	while (1)
	{
	    *(yorgb + yoffset + x) = pixel;
	    *(yorgb + yoffset - x) = pixel;
	    *(yorgob - yoffset - x) = pixel;
	    *(yorgob - yoffset + x) = pixel;
	    if (a < 0)
		break;
	    *(yorghb - xoffset - y) = pixel;
	    *(yorgohb - xoffset + y) = pixel;
	    *(yorgohb + xoffset + y) = pixel;
	    *(yorghb + xoffset - y) = pixel;
	    xoffset += nlwidth;
	    MIARCCIRCLESTEP(yoffset += nlwidth;);
	}
	yorgb -= info.xorg;
	yorgob -= info.xorg;
	x = info.w;
	yoffset = info.h * nlwidth;
    }
    else if (do360)
    {
	while (y < info.h || x < info.w)
	{
	    MIARCOCTANTSHIFT(dyoffset = nlwidth;);
	    *(yorgb + yoffset + info.xorg + x) = pixel;
	    *(yorgb + yoffset + info.xorgo - x) = pixel;
	    *(yorgob - yoffset + info.xorgo - x) = pixel;
	    *(yorgob - yoffset + info.xorg + x) = pixel;
	    MIARCSTEP(yoffset += dyoffset;, yoffset += nlwidth;);
	}
    }
    else
    {
	while (y < info.h || x < info.w)
	{
	    MIARCOCTANTSHIFT(dyoffset = nlwidth;);
	    if ((x == info.start.x) || (y == info.start.y))
	    {
		mask = info.start.mask;
		info.start = info.altstart;
	    }
	    if (mask & 1)
		*(yorgb + yoffset + info.xorg + x) = pixel;
	    if (mask & 2)
		*(yorgb + yoffset + info.xorgo - x) = pixel;
	    if (mask & 4)
		*(yorgob - yoffset + info.xorgo - x) = pixel;
	    if (mask & 8)
		*(yorgob - yoffset + info.xorg + x) = pixel;
	    if ((x == info.end.x) || (y == info.end.y))
	    {
		mask = info.end.mask;
		info.end = info.altend;
	    }
	    MIARCSTEP(yoffset += dyoffset;, yoffset += nlwidth;);
	}
    }
    if ((x == info.start.x) || (y == info.start.y))
	mask = info.start.mask;
    if (mask & 1)
	*(yorgb + yoffset + info.xorg + x) = pixel;
    if (mask & 4)
	*(yorgob - yoffset + info.xorgo - x) = pixel;
    if (arc->height & 1)
    {
	if (mask & 2)
	    *(yorgb + yoffset + info.xorgo - x) = pixel;
	if (mask & 8)
	    *(yorgob - yoffset + info.xorg + x) = pixel;
    }
}

void
cfbZeroPolyArcSS8Copy(pDraw, pGC, narcs, parcs)
    register DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    register xArc *arc;
    register int i;
    BoxRec box;
    RegionPtr cclip;

    cclip = ((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip;
    for (arc = parcs, i = narcs; --i >= 0; arc++)
    {
	if (miCanZeroArc(arc))
	{
	    box.x1 = arc->x + pDraw->x;
	    box.y1 = arc->y + pDraw->y;
	    box.x2 = box.x1 + (int)arc->width + 1;
	    box.y2 = box.y1 + (int)arc->height + 1;
	    if ((*pDraw->pScreen->RectIn)(cclip, &box) == rgnIN)
		cfbZeroArcSS8Copy(pDraw, pGC, arc);
	    else
		miZeroPolyArc(pDraw, pGC, 1, arc);
	}
	else
	    miPolyArc(pDraw, pGC, 1, arc);
    }
}

#endif
