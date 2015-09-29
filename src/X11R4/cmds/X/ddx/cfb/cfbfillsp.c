/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

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

/* $XConsortium: cfbfillsp.c,v 5.7 89/11/24 18:09:00 rws Exp $ */

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

extern void mfbInvertSolidFS(), mfbBlackSolidFS(), mfbWhiteSolidFS();

/* scanline filling for color frame buffer
   written by drewry, oct 1986 modified by smarks
   changes for compatibility with Little-endian systems Jul 1987; MIT:yba.

   these routines all clip.  they assume that anything that has called
them has already translated the points (i.e. pGC->miTranslate is
non-zero, which is howit gets set in cfbCreateGC().)

   the number of new scnalines created by clipping ==
MaxRectsPerBand * nSpans.

    FillSolid is overloaded to be used for OpaqueStipple as well,
if fgPixel == bgPixel.  
Note that for solids, PrivGC.rop == PrivGC.ropOpStip


    FillTiled is overloaded to be used for OpaqueStipple, if
fgPixel != bgPixel.  based on the fill style, it uses
{RotatedTile, gc.alu} or {RotatedStipple, PrivGC.ropOpStip}
*/

#ifdef	notdef
#include	<stdio.h>
static
dumpspans(n, ppt, pwidth)
    int	n;
    DDXPointPtr ppt;
    int *pwidth;
{
    fprintf(stderr,"%d spans\n", n);
    while (n--) {
	fprintf(stderr, "[%d,%d] %d\n", ppt->x, ppt->y, *pwidth);
	ppt++;
	pwidth++;
    }
    fprintf(stderr, "\n");
}
#endif

void
cfbSolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    DDXPointPtr ppt;		/* pointer to list of start points */
    int *pwidth;		/* pointer to list of n widths */
    int *addrlBase;		/* pointer to start of bitmap */
    int nlwidth;		/* width in longwords of bitmap */
    register int *addrl;	/* pointer to current longword in bitmap */
    register int width;		/* current span width */
    register int nlmiddle;
    register int fill;
    register int x;
    register int startmask;
    register int endmask;
    int rop;			/* rasterop */
    int planemask;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;

    if (!(planemask = pGC->planemask))
	return;

    n = nInit * miFindMaxBand(((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip);
    pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
#ifdef	notdef
    dumpspans(n, pptInit, pwidthInit);
#endif
    pwidth = pwidthFree;
    ppt = pptFree;
    n = miClipSpans(((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip,
		     pptInit, pwidthInit, nInit,
		     ppt, pwidth, fSorted);

#ifdef	notdef
    dumpspans(n, ppt, pwidth);
#endif
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	nlwidth = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    rop = pGC->alu;
    fill = PFILL(pGC->fgPixel);
    planemask = PFILL(planemask);

    if (rop == GXcopy && (planemask & PMSK) == PMSK)
    {
	while (n--)
	{
	    x = ppt->x;
	    addrl = addrlBase + (ppt->y * nlwidth);
	    ++ppt;
	    width = *pwidth++;
	    if (!width)
		continue;
#if PPW == 4
	    if (width <= 4)
	    {
		register char	*addrb;

		addrb = ((char *) addrl) + x;
		while (width--)
		    *addrb++ = fill;
	    }
#else
	    if ( ((x & PIM) + width) <= PPW)
	    {
		addrl += x >> PWSH;
		maskpartialbits(x, width, startmask);
		*addrl = (*addrl & ~startmask) | (fill & startmask);
	    }
#endif
	    else
	    {
		addrl += x >> PWSH;
		maskbits(x, width, startmask, endmask, nlmiddle);
		if ( startmask ) {
		    *addrl = *addrl & ~startmask | fill & startmask;
		    ++addrl;
		}
		while ( nlmiddle-- )
		    *addrl++ = fill;
		if ( endmask )
		    *addrl = *addrl & ~endmask | fill & endmask;
	    }
	}
    }
    else if ((rop == GXxor && (planemask & PMSK) == PMSK) || rop == GXinvert)
    {
	if (rop == GXinvert)
	    fill = planemask;
	while (n--)
	{
	    x = ppt->x;
	    addrl = addrlBase + (ppt->y * nlwidth);
	    ++ppt;
	    width = *pwidth++;
	    if (!width)
		continue;
#if PPW == 4
	    if (width <= 4)
	    {
		register char	*addrb;

		addrb = ((char *) addrl) + x;
		while (width--)
		    *addrb++ ^= fill;
	    }
#else
	    if ( ((x & PIM) + width) <= PPW)
	    {
		addrl += x >> PWSH;
		maskpartialbits(x, width, startmask);
		*addrl ^= (fill & startmask);
	    }
#endif
	    else
	    {
		addrl += x >> PWSH;
		maskbits(x, width, startmask, endmask, nlmiddle);
		if ( startmask )
		    *addrl++ ^= (fill & startmask);
		while ( nlmiddle-- )
		    *addrl++ ^= fill;
		if ( endmask )
		    *addrl ^= (fill & endmask);
	    }
	}
    }
    else
    {
    	while (n--)
    	{
	    x = ppt->x;
	    addrl = addrlBase + (ppt->y * nlwidth) + (x >> PWSH);
	    ++ppt;
	    width = *pwidth++;
	    if (width)
	    {
	    	if ( ((x & PIM) + width) <= PPW)
	    	{
		    maskpartialbits(x, width, startmask);
		    *addrl = *addrl & ~(planemask & startmask) |
			     DoRop(rop, fill, *addrl) & (planemask & startmask);
	    	}
	    	else
	    	{
		    maskbits(x, width, startmask, endmask, nlmiddle);
		    if ( startmask ) {
			*addrl = *addrl & ~(planemask & startmask) |
			         DoRop (rop, fill, *addrl) & (planemask & startmask);
		    	++addrl;
		    }
		    while ( nlmiddle-- ) {
			*addrl = (*addrl & ~planemask) |
				 DoRop (rop, fill, *addrl) & planemask;
		    	++addrl;
		    }
		    if ( endmask ) {
			*addrl = *addrl & ~(planemask & endmask) |
			         DoRop (rop, fill, *addrl) & (planemask & endmask);
		    }
	    	}
	    }
    	}
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}


/* Fill spans with tiles that aren't 32 bits wide */
void
cfbUnnaturalTileFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
    int		iline;		/* first line of tile to use */
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int		*addrlBase;	/* pointer to start of bitmap */
    int		 nlwidth;	/* width in longwords of bitmap */
    register int *pdst;		/* pointer to current word in bitmap */
    register int *psrc;		/* pointer to current word in tile */
    register int nlMiddle;
    register int startmask;
    PixmapPtr	pTile;		/* pointer to tile we want to fill with */
    int		w, width, x, tmpSrc, srcStartOver, nstart, nend;
    int		xSrc, ySrc;
    int 	endmask, tlwidth, rem, tileWidth, *psrcT, rop;
    int		tileHeight;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip);
    pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
    pwidth = pwidthFree;
    ppt = pptFree;
    n = miClipSpans(((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    if (pGC->fillStyle == FillTiled)
    {
	pTile = pGC->tile.pixmap;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }
    else
    {
	pTile = pGC->stipple;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }

    xSrc = pDrawable->x;
    ySrc = pDrawable->y;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	nlwidth = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    tileWidth = pTile->drawable.width;
    tileHeight = pTile->drawable.height;

    /* this replaces rotating the tile. Instead we just adjust the offset
     * at which we start grabbing bits from the tile.
     * Ensure that ppt->x - xSrc >= 0 and ppt->y - ySrc >= 0,
     * so that iline and xrem always stay within the tile bounds.
     */
    xSrc += (pGC->patOrg.x % tileWidth) - tileWidth;
    ySrc += (pGC->patOrg.y % tileHeight) - tileHeight;

    while (n--)
    {
	iline = (ppt->y - ySrc) % tileHeight;
        pdst = addrlBase + (ppt->y * nlwidth) + (ppt->x >> PWSH);
        psrcT = (int *) pTile->devPrivate.ptr + (iline * tlwidth);
	x = ppt->x;

	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
		rem = (x - xSrc) % tileWidth;
		psrc = psrcT + rem / PPW;
	        w = min(tileWidth, width);
		w = min(w,tileWidth-rem);
#ifdef notdef
		if((rem = x % tileWidth) != 0)
		{
		    w = min(min(tileWidth - rem, width), PPW);
		    /* we want to grab from the end of the tile.  Figure
		     * out where that is.  In general, the start of the last
		     * word of data on this scanline is tlwidth -1 words 
		     * away. But if we want to grab more bits than we'll
		     * find on that line, we have to back up 1 word further.
		     * On the other hand, if the whole tile fits in 1 word,
		     * let's skip the work */ 
		    endinc = tlwidth - 1 - (tileWidth-rem) / PPW;

		    if(endinc)
		    {
			if((rem & PIM) + w > tileWidth % PPW)
			    endinc--;
		    }

		    getbits(psrc + endinc, rem & PIM, w, tmpSrc);
		    putbitsrop(tmpSrc, (x & PIM), w, pdst, 
			pGC->planemask, rop);
		    if((x & PIM) + w >= PPW)
			pdst++;
		}
		else
#endif /* notdef */
		if(((x & PIM) + w) <= PPW)
		{
		    getbits(psrc, (rem & PIM), w, tmpSrc);
		    putbitsrop(tmpSrc, x & PIM, w, pdst, 
			pGC->planemask, rop);
		    if ((x & PIM) + w == PPW) ++pdst;
		}
		else
		{
		    maskbits(x, w, startmask, endmask, nlMiddle);

	            if (startmask)
		        nstart = PPW - (x & PIM);
	            else
		        nstart = 0;
	            if (endmask)
	                nend = (x + w)  & PIM;
	            else
		        nend = 0;

	            srcStartOver = nstart + (rem & PIM) > PLST;

		    if(startmask)
		    {
			getbits(psrc, rem & PIM, nstart, tmpSrc);
			putbitsrop(tmpSrc, x & PIM, nstart, pdst, 
			    pGC->planemask, rop);
			pdst++;
			if(srcStartOver)
			    psrc++;
		    }
		    nstart = (nstart + rem) & PIM;
		    while(nlMiddle--)
		    {
			    getbits(psrc, nstart, PPW, tmpSrc);
			    putbitsrop( tmpSrc, 0, PPW,
				pdst, pGC->planemask, rop );
			    pdst++;
			    psrc++;
		    }
		    if(endmask)
		    {
			getbits(psrc, nstart, nend, tmpSrc);
			putbitsrop(tmpSrc, 0, nend, pdst, 
			    pGC->planemask, rop);
		    }
		 }
		 x += w;
		 width -= w;
	    }
	}
	ppt++;
	pwidth++;
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}


/* Fill spans with stipples that aren't 32 bits wide */
void
cfbUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int		iline;		/* first line of tile to use */
    int		*addrlBase;	/* pointer to start of bitmap */
    int		 nlwidth;	/* width in longwords of bitmap */
    register int *pdst;		/* pointer to current word in bitmap */
    PixmapPtr	pStipple;	/* pointer to stipple we want to fill with */
    register int w;
    int		width,  x, xrem, xSrc, ySrc;
    unsigned int tmpSrc, tmpDst1, tmpDst2;
    int 	stwidth, stippleWidth, *psrcS, rop, stiprop;
    int		stippleHeight;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    unsigned int fgfill, bgfill;

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip);
    pwidthFree = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    pptFree = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) DEALLOCATE_LOCAL(pptFree);
	if (pwidthFree) DEALLOCATE_LOCAL(pwidthFree);
	return;
    }
    pwidth = pwidthFree;
    ppt = pptFree;
    n = miClipSpans(((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);
    rop = pGC->alu;
    if (pGC->fillStyle == FillStippled) {
	switch (rop) {
	    case GXand:
	    case GXcopy:
	    case GXnoop:
	    case GXor:
		stiprop = rop;
		break;
	    default:
		stiprop = rop;
		rop = GXcopy;
	}
    }
    fgfill = PFILL(pGC->fgPixel);
    bgfill = PFILL(pGC->bgPixel);

    /*
     *  OK,  so what's going on here?  We have two Drawables:
     *
     *  The Stipple:
     *		Depth = 1
     *		Width = stippleWidth
     *		Words per scanline = stwidth
     *		Pointer to pixels = pStipple->devPrivate.ptr
     */
    pStipple = pGC->stipple;

    if (pStipple->drawable.depth != 1) {
	FatalError( "Stipple depth not equal to 1!\n" );
    }

    stwidth = pStipple->devKind >> 2;
    stippleWidth = pStipple->drawable.width;
    stippleHeight = pStipple->drawable.height;

    /*
     *	The Target:
     *		Depth = PSZ
     *		Width = determined from *pwidth
     *		Words per scanline = nlwidth
     *		Pointer to pixels = addrlBase
     */
    xSrc = pDrawable->x;
    ySrc = pDrawable->y;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	nlwidth = (int)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    /* this replaces rotating the stipple. Instead we just adjust the offset
     * at which we start grabbing bits from the stipple.
     * Ensure that ppt->x - xSrc >= 0 and ppt->y - ySrc >= 0,
     * so that iline and xrem always stay within the stipple bounds.
     */
    xSrc += (pGC->patOrg.x % stippleWidth) - stippleWidth;
    ySrc += (pGC->patOrg.y % stippleHeight) - stippleHeight;

    while (n--)
    {
	iline = (ppt->y - ySrc) % stippleHeight;
	x = ppt->x;
	pdst = addrlBase + (ppt->y * nlwidth);
        psrcS = (int *) pStipple->devPrivate.ptr + (iline * stwidth);

	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
	        int xtemp, tmpx;
		register unsigned int *ptemp;
		register int *pdsttmp;
		/*
		 *  Do a stripe through the stipple & destination w pixels
		 *  wide.  w is not more than:
		 *	-	the width of the destination
		 *	-	the width of the stipple
		 *	-	the distance between x and the next word 
		 *		boundary in the destination
		 *	-	the distance between x and the next word
		 *		boundary in the stipple
		 */

		/* width of dest/stipple */
                xrem = (x - xSrc) % stippleWidth;
	        w = min((stippleWidth - xrem), width);
		/* dist to word bound in dest */
		w = min(w, PPW - (x & PIM));
		/* dist to word bound in stip */
		w = min(w, 32 - (x & 0x1f));

	        xtemp = (xrem & 0x1f);
	        ptemp = (unsigned int *)(psrcS + (xrem >> 5));
		tmpx = x & PIM;
		pdsttmp = pdst + (x>>PWSH);
		switch ( pGC->fillStyle ) {
		    case FillOpaqueStippled:
			getstipplepixels(ptemp, xtemp, w, 0, &bgfill, &tmpDst1);
			getstipplepixels(ptemp, xtemp, w, 1, &fgfill, &tmpDst2);
			break;
		    case FillStippled:
			/* Fill tmpSrc with the source pixels */
			getbits(pdsttmp, tmpx, w, tmpSrc);
			getstipplepixels(ptemp, xtemp, w, 0, &tmpSrc, &tmpDst1);
			if (rop != stiprop) {
			    putbitsrop(fgfill, 0, w, &tmpSrc, pGC->planemask, stiprop);
			} else {
			    tmpSrc = fgfill;
			}
			getstipplepixels(ptemp, xtemp, w, 1, &tmpSrc, &tmpDst2);
			break;
		}
		tmpDst2 |= tmpDst1;
		putbitsrop(tmpDst2, tmpx, w, pdsttmp, pGC->planemask, rop);
		x += w;
		width -= w;
	    }
	}
#ifdef	notdef
	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
		psrc = psrcS;
	        w = min(min(stippleWidth, width), PPW);
		if((rem = x % stippleWidth) != 0)
		{
		    w = min(min(stippleWidth - rem, width), PPW);
		    /* we want to grab from the end of the tile.  Figure
		     * out where that is.  In general, the start of the last
		     * word of data on this scanline is stwidth -1 words 
		     * away. But if we want to grab more bits than we'll
		     * find on that line, we have to back up 1 word further.
		     * On the other hand, if the whole tile fits in 1 word,
		     * let's skip the work */ 
		    endinc = stwidth - 1 - w / PPW;

		    if(endinc)
		    {
			if((rem & 0x1f) + w > stippleWidth % PPW)
			    endinc--;
		    }

		    getbits(psrc + endinc, rem & PIM, w, tmpSrc);
		    putbitsrop(tmpSrc, (x & PIM), w, pdst, 
			pGC->planemask, rop);
		    if((x & PIM) + w >= PPW)
			pdst++;
		}

		else if(((x & PIM) + w) < PPW)
		{
		    getbits(psrc, 0, w, tmpSrc);
		    putbitsrop(tmpSrc, x & PIM, w, pdst, 
			pGC->planemask, rop);
		}
		else
		{
		    maskbits(x, w, startmask, endmask, nlMiddle);

	            if (startmask)
		        nstart = PPW - (x & PIM);
	            else
		        nstart = 0;
	            if (endmask)
	                nend = (x + w)  & PIM;
	            else
		        nend = 0;

	            srcStartOver = nstart > PLST;

		    if(startmask)
		    {
			getbits(psrc, 0, nstart, tmpSrc);
			putbitsrop(tmpSrc, (x & PIM), nstart, pdst, 
			    pGC->planemask, rop);
			pdst++;
			if(srcStartOver)
			    psrc++;
		    }
		     
		    while(nlMiddle--)
		    {
			/*
			    getbits(psrc, nstart, PPW, tmpSrc);
			    putbitsrop( tmpSrc, 0, PPW,
				pdst, pGC->planemask, rop );
			*/
			switch ( pGC->fillStyle ) {
			    case FillStippled:
				getstipplepixels( psrc, j, 4, 1, pdst, tmp1 );
				break;
			    case FillOpaqueStippled:
				getstipplepixels( psrc, j, 4, 1, pdst, tmp1 );
				break;
			}
			pdst++;
			psrc++;
		    }
		    if(endmask)
		    {
			getbits(psrc, nstart, nend, tmpSrc);
			putbitsrop(tmpSrc, 0, nend, pdst, 
			    pGC->planemask, rop);
		    }
		 }
		 x += w;
		 width -= w;
	    }
	}
#endif
	ppt++;
	pwidth++;
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}
