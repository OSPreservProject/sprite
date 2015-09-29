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
/* $XConsortium: mfbbitblt.c,v 5.14 90/02/08 13:29:24 rws Exp $ */
#include "X.h"
#include "Xprotostr.h"

#include "miscstruct.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mi.h"

#include "mfb.h"
#include "maskbits.h"


/* CopyArea and CopyPlane for a monchrome frame buffer


    clip the source rectangle to the source's available bits.  (this
avoids copying unnecessary pieces that will just get exposed anyway.)
this becomes the new shape of the destination.
    clip the destination region to the composite clip in the
GC.  this requires translating the destination region to (dstx, dsty).
    build a list of source points, one for each rectangle in the
destination.  this is a simple translation.
    go do the multiple rectangle copies
    do graphics exposures
*/
/** Optimized for drawing pixmaps into windows, especially when drawing into
 ** unobscured windows.  Calls to the general-purpose region code were
 ** replaced with rectangle-to-rectangle clipping comparisions.  This is
 ** possible, since the pixmap is a single rectangle.  In an unobscured
 ** window, the destination clip is also a single rectangle, and region
 ** code can be avoided entirely.  This is a big savings, since the region
 ** code uses XAlloc() and makes many function calls.
 **
 ** In addition, if source is a pixmap, there is no need to call the
 ** expensive miHandleExposures() routine.  Instead, we simply return NULL.
 **
 ** Previously, drawing a pixmap into an unobscured window executed at least
 ** 8 XAlloc()'s, 30 function calls, and hundreds of lines of code.
 **
 ** Now, the same operation requires no XAlloc()'s, no region function calls,
 ** and much less overhead.  Nice for drawing lots of small pixmaps.
 */
 
RegionPtr
mfbCopyArea(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty)
register DrawablePtr pSrcDrawable;
register DrawablePtr pDstDrawable;
register GC *pGC;
int srcx, srcy;
int width, height;
int dstx, dsty;
{
    RegionPtr prgnSrcClip;	/* may be a new region, or just a copy */
    Bool freeSrcClip = FALSE;

    RegionPtr prgnExposed;
    RegionRec rgnDst;
    DDXPointPtr pptSrc;
    register DDXPointPtr ppt;
    register BoxPtr pbox;
    int i;
    register int dx;
    register int dy;
    xRectangle origSource;
    DDXPointRec origDest;
    int numRects;
    BoxRec fastBox;
    int fastClip = 0;		/* for fast clipping with pixmap source */
    int fastExpose = 0;		/* for fast exposures with pixmap source */

    origSource.x = srcx;
    origSource.y = srcy;
    origSource.width = width;
    origSource.height = height;
    origDest.x = dstx;
    origDest.y = dsty;

    if ((pSrcDrawable != pDstDrawable) &&
	pSrcDrawable->pScreen->SourceValidate)
	(*pSrcDrawable->pScreen->SourceValidate) (pSrcDrawable, srcx, srcy, width, height);

    srcx += pSrcDrawable->x;
    srcy += pSrcDrawable->y;

    /* clip the source */

    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	if ((pSrcDrawable == pDstDrawable) &&
	    (pGC->clientClipType == CT_NONE))
	{
	    prgnSrcClip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
	}
	else
	{
	    /* Pixmap sources generate simple exposure events */
	    fastExpose = 1;

	    /* Pixmap is just one clipping rectangle so we can avoid
	       allocating a full-blown region. */
	    fastClip = 1;

	    fastBox.x1 = srcx;
	    fastBox.y1 = srcy;
	    fastBox.x2 = srcx + width;
	    fastBox.y2 = srcy + height;
	    
	    /* Left and top are already clipped, so clip right and bottom */
	    if (fastBox.x2 > pSrcDrawable->x + (int) pSrcDrawable->width)
	      fastBox.x2 = pSrcDrawable->x + (int) pSrcDrawable->width;
	    if (fastBox.y2 > pSrcDrawable->y + (int) pSrcDrawable->height)
	      fastBox.y2 = pSrcDrawable->y + (int) pSrcDrawable->height;
	}
    }
    else
    {
	if (pGC->subWindowMode == IncludeInferiors)
	{
	    if ((pSrcDrawable == pDstDrawable) &&
		(pGC->clientClipType == CT_NONE))
	    {
		prgnSrcClip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
	    }
	    else
	    {
		prgnSrcClip = NotClippedByChildren((WindowPtr)pSrcDrawable);
		freeSrcClip = TRUE;
	    }
	}
	else
	{
	    prgnSrcClip = &((WindowPtr)pSrcDrawable)->clipList;
	}
    }

    /* Don't create a source region if we are doing a fast clip */
    if (!fastClip)
    {
	BoxRec srcBox;

	srcBox.x1 = srcx;
	srcBox.y1 = srcy;
	srcBox.x2 = srcx + width;
	srcBox.y2 = srcy + height;
	
	(*pGC->pScreen->RegionInit)(&rgnDst, &srcBox, 1);
	(*pGC->pScreen->Intersect)(&rgnDst, &rgnDst, prgnSrcClip);
    }
    
    dstx += pDstDrawable->x;
    dsty += pDstDrawable->y;

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	if (!((WindowPtr)pDstDrawable)->realized)
	{
	    if (!fastClip)
		(*pGC->pScreen->RegionUninit)(&rgnDst);
	    if (freeSrcClip)
		(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
	    return NULL;
	}
    }

    dx = srcx - dstx;
    dy = srcy - dsty;

    /* Translate and clip the dst to the destination composite clip */
    if (fastClip)
    {
	RegionPtr cclip;

        /* Translate the region directly */
        fastBox.x1 -= dx;
        fastBox.x2 -= dx;
        fastBox.y1 -= dy;
        fastBox.y2 -= dy;

	/* If the destination composite clip is one rectangle we can
	   do the clip directly.  Otherwise we have to create a full
	   blown region and call intersect */
	cclip = ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip;
        if (REGION_NUM_RECTS(cclip) == 1)
        {
	    BoxPtr pBox = REGION_RECTS(cclip);
	  
	    if (fastBox.x1 < pBox->x1) fastBox.x1 = pBox->x1;
	    if (fastBox.x2 > pBox->x2) fastBox.x2 = pBox->x2;
	    if (fastBox.y1 < pBox->y1) fastBox.y1 = pBox->y1;
	    if (fastBox.y2 > pBox->y2) fastBox.y2 = pBox->y2;

	    /* Check to see if the region is empty */
	    if (fastBox.x1 >= fastBox.x2 || fastBox.y1 >= fastBox.y2)
		(*pGC->pScreen->RegionInit)(&rgnDst, NullBox, 0);
	    else
		(*pGC->pScreen->RegionInit)(&rgnDst, &fastBox, 1);
	}
        else
	{
	    /* We must turn off fastClip now, since we must create
	       a full blown region.  It is intersected with the
	       composite clip below. */
	    fastClip = 0;
	    (*pGC->pScreen->RegionInit)(&rgnDst, &fastBox,1);
	}
    }
    else
    {
        (*pGC->pScreen->TranslateRegion)(&rgnDst, -dx, -dy);
    }

    if (!fastClip)
    {
	(*pGC->pScreen->Intersect)(&rgnDst,
				   &rgnDst,
				 ((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->pCompositeClip);
    }

    /* Do bit blitting */
    numRects = REGION_NUM_RECTS(&rgnDst);
    if (numRects)
    {
	if(!(pptSrc = (DDXPointPtr)ALLOCATE_LOCAL(numRects *
						  sizeof(DDXPointRec))))
	{
	    (*pGC->pScreen->RegionUninit)(&rgnDst);
	    if (freeSrcClip)
		(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
	    return NULL;
	}
	pbox = REGION_RECTS(&rgnDst);
	ppt = pptSrc;
	for (i = numRects; --i >= 0; pbox++, ppt++)
	{
	    ppt->x = pbox->x1 + dx;
	    ppt->y = pbox->y1 + dy;
	}
    
	if (pGC->planemask & 1)
	    mfbDoBitblt(pSrcDrawable, pDstDrawable, pGC->alu, &rgnDst, pptSrc);
	DEALLOCATE_LOCAL(pptSrc);
    }

    prgnExposed = NULL;
    if (((mfbPrivGC *)(pGC->devPrivates[mfbGCPrivateIndex].ptr))->fExpose) {
        /* Pixmap sources generate a NoExposed (we return NULL to do this) */
        if (!fastExpose)
	    prgnExposed =
		miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
				  origSource.x, origSource.y,
				  (int)origSource.width,
				  (int)origSource.height,
				  origDest.x, origDest.y, (unsigned long)0);
	}
    (*pGC->pScreen->RegionUninit)(&rgnDst);
    if (freeSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
    return prgnExposed;
}

/* DoBitblt() does multiple rectangle moves into the rectangles
   DISCLAIMER:
   this code can be made much faster; this implementation is
designed to be independent of byte/bit order, processor
instruction set, and the like.  it could probably be done
in a similarly device independent way using mask tables instead
of the getbits/putbits macros.  the narrow case (w<32) can be
subdivided into a case that crosses word boundaries and one that
doesn't.

   we have to cope with the dircetion on a per band basis,
rather than a per rectangle basis.  moving bottom to top
means we have to invert the order of the bands; moving right
to left requires reversing the order of the rectangles in
each band.

   if src or dst is a window, the points have already been
translated.
*/

#ifdef FASTGETBITS
#define getunalignedword(psrc, x, dst) { \
	register int _tmp; \
	FASTGETBITS(psrc, x, 32, _tmp); \
	dst = _tmp; \
}
#else
#define getunalignedword(psrc, x, dst) \
{ \
    dst = (SCRLEFT((unsigned) *(psrc), (x))) | \
	  (SCRRIGHT((unsigned) *((psrc)+1), 32-(x))); \
}
#endif  /* FASTGETBITS */

#include "fastblt.h"

mfbDoBitblt(pSrcDrawable, pDstDrawable, alu, prgnDst, pptSrc)
DrawablePtr pSrcDrawable;
DrawablePtr pDstDrawable;
unsigned char alu;
RegionPtr prgnDst;
DDXPointPtr pptSrc;
{
    unsigned int *psrcBase, *pdstBase;	
				/* start of src and dst bitmaps */
    int widthSrc, widthDst;	/* add to get to same position in next line */

    register BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNew1, pptNew2;
				/* shuffling boxes entails shuffling the
				   source points too */
    int w, h;
    int xdir;			/* 1 = left right, -1 = right left/ */
    int ydir;			/* 1 = top down, -1 = bottom up */

    unsigned int *psrcLine, *pdstLine;	
				/* pointers to line with current src and dst */
    register unsigned int *psrc;/* pointer to current src longword */
    register unsigned int *pdst;/* pointer to current dst longword */

				/* following used for looping through a line */
    unsigned int startmask, endmask;	/* masks for writing ends of dst */
    int nlMiddle;		/* whole longwords in dst */
    register int nl;		/* temp copy of nlMiddle */
    register unsigned int tmpSrc;
				/* place to store full source word */
    register int xoffSrc;	/* offset (>= 0, < 32) from which to
			           fetch whole longwords fetched 
				   in src */
    int nstart;			/* number of ragged bits at start of dst */
    int nend;			/* number of ragged bits at end of dst */
    int srcStartOver;		/* pulling nstart bits from src
				   overflows into the next word? */
    int careful;


    if (pSrcDrawable->type == DRAWABLE_WINDOW)
    {
	psrcBase = (unsigned int *)
		(((PixmapPtr)(pSrcDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthSrc = (int)
		   ((PixmapPtr)(pSrcDrawable->pScreen->devPrivate))->devKind
		    >> 2;
    }
    else
    {
	psrcBase = (unsigned int *)(((PixmapPtr)pSrcDrawable)->devPrivate.ptr);
	widthSrc = (int)(((PixmapPtr)pSrcDrawable)->devKind) >> 2;
    }

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (unsigned int *)
		(((PixmapPtr)(pDstDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthDst = (int)
		   ((PixmapPtr)(pDstDrawable->pScreen->devPrivate))->devKind
		    >> 2;
    }
    else
    {
	pdstBase = (unsigned int *)(((PixmapPtr)pDstDrawable)->devPrivate.ptr);
	widthDst = (int)(((PixmapPtr)pDstDrawable)->devKind) >> 2;
    }

    /* XXX we have to err on the side of safety when both are windows,
     * because we don't know if IncludeInferiors is being used.
     */
    careful = ((pSrcDrawable == pDstDrawable) ||
	       ((pSrcDrawable->type == DRAWABLE_WINDOW) &&
		(pDstDrawable->type == DRAWABLE_WINDOW)));

    pbox = REGION_RECTS(prgnDst);
    nbox = REGION_NUM_RECTS(prgnDst);

    pboxNew1 = NULL;
    pptNew1 = NULL;
    pboxNew2 = NULL;
    pptNew2 = NULL;
    if (careful && (pptSrc->y < pbox->y1))
    {
        /* walk source botttom to top */
	ydir = -1;
	widthSrc = -widthSrc;
	widthDst = -widthDst;

	if (nbox > 1)
	{
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if(!pboxNew1)
		return;
	    pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pptNew1)
	    {
	        DEALLOCATE_LOCAL(pboxNew1);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox)
	    {
	        while ((pboxNext >= pbox) && 
		       (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
	        pboxTmp = pboxNext+1;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp <= pboxBase)
	        {
		    *pboxNew1++ = *pboxTmp++;
		    *pptNew1++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew1 -= nbox;
	    pbox = pboxNew1;
	    pptNew1 -= nbox;
	    pptSrc = pptNew1;
        }
    }
    else
    {
	/* walk source top to bottom */
	ydir = 1;
    }

    if (careful && (pptSrc->x < pbox->x1))
    {
	/* walk source right to left */
        xdir = -1;

	if (nbox > 1)
	{
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNew2 || !pptNew2)
	    {
		if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
		if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
		if (pboxNew1)
		{
		    DEALLOCATE_LOCAL(pptNew1);
		    DEALLOCATE_LOCAL(pboxNew1);
		}
	        return;
	    }
	    pboxBase = pboxNext = pbox;
	    while (pboxBase < pbox+nbox)
	    {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;
	        pboxTmp = pboxNext;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp != pboxBase)
	        {
		    *pboxNew2++ = *--pboxTmp;
		    *pptNew2++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew2 -= nbox;
	    pbox = pboxNew2;
	    pptNew2 -= nbox;
	    pptSrc = pptNew2;
	}
    }
    else
    {
	/* walk source left to right */
        xdir = 1;
    }


    /* special case copy */
    if (alu == GXcopy)
    {
	register unsigned int bits;
	register unsigned int bits1;
	int xoffSrc, xoffDst;
	int	leftShift, rightShift;

	while(nbox--)
	{
	    w = pbox->x2 - pbox->x1;
	    h = pbox->y2 - pbox->y1;

	    if (ydir == -1) /* start at last scanline of rectangle */
	    {
	        psrcLine = psrcBase + ((pptSrc->y+h-1) * -widthSrc);
	        pdstLine = pdstBase + ((pbox->y2-1) * -widthDst);
	    }
	    else /* start at first scanline */
	    {
	        psrcLine = psrcBase + (pptSrc->y * widthSrc);
	        pdstLine = pdstBase + (pbox->y1 * widthDst);
	    }
	    if ((pbox->x1 & 0x1f) + w <= 32)
	    {
		pdst = pdstLine + (pbox->x1 >> 5);
		psrc = psrcLine + (pptSrc->x >> 5);
		xoffSrc = pptSrc->x & 0x1f;
		xoffDst = pbox->x1 & 0x1f;
		while (h--)
		{
		    getandputbits(psrc, xoffSrc, xoffDst, w, pdst);
		    psrc += widthSrc;
		    pdst += widthDst;
		}
	    }
	    else
	    {
	    	maskbits(pbox->x1, w, startmask, endmask, nlMiddle);
	    	if (xdir == 1)
	    	{
	    	    xoffSrc = pptSrc->x & 0x1f;
	    	    xoffDst = pbox->x1 & 0x1f;
		    pdstLine += (pbox->x1 >> 5);
		    psrcLine += (pptSrc->x >> 5);
		    if (xoffSrc == xoffDst)
		    {
	    	    	while (h--)
	    	    	{
		    	    psrc = psrcLine;
		    	    pdst = pdstLine;
		    	    pdstLine += widthDst;
		    	    psrcLine += widthSrc;
			    if (startmask)
			    {
			    	*pdst = (*pdst & ~startmask) | (*psrc++ & startmask);
			    	pdst++;
			    }
			    nl = nlMiddle;

#ifdef LARGE_INSTRUCTION_CACHE
#ifdef FAST_CONSTANT_OFFSET_MODE

			    psrc += nl & (UNROLL-1);
			    pdst += nl & (UNROLL-1);

#define BodyOdd(n) pdst[-n] = psrc[-n];
#define BodyEven(n) pdst[-n] = psrc[-n];

#define LoopReset \
    pdst += UNROLL; \
    psrc += UNROLL;

#else

#define BodyOdd(n)  *pdst++ = *psrc++;
#define BodyEven(n) BodyOdd(n)

#define LoopReset   ;

#endif
			    PackedLoop

#undef BodyOdd
#undef BodyEven
#undef LoopReset

#else
			    DuffL(nl, label1, *pdst++ = *psrc++;)
#endif
			    if (endmask)
			    	*pdst = (*pdst & ~endmask) | (*psrc++ & endmask);
		    	}
		    }
		    else
		    {
		    	if (xoffSrc > xoffDst)
			{
			    leftShift = (xoffSrc - xoffDst);
			    rightShift = 32 - leftShift;
			}
		    	else
			{
			    rightShift = (xoffDst - xoffSrc);
			    leftShift = 32 - rightShift;
			}
		    	while (h--)
		    	{
			    psrc = psrcLine;
			    pdst = pdstLine;
			    pdstLine += widthDst;
			    psrcLine += widthSrc;
			    bits = 0;
			    if (xoffSrc > xoffDst)
			    	bits = *psrc++;
			    if (startmask)
			    {
			    	bits1 = SCRLEFT(bits,leftShift);
			    	bits = *psrc++;
			    	bits1 |= SCRRIGHT(bits,rightShift);
			    	*pdst = (*pdst & ~startmask) |
				    	(bits1 & startmask);
			    	pdst++;
			    }
			    nl = nlMiddle;

#ifdef LARGE_INSTRUCTION_CACHE
			    bits1 = bits;
#ifdef FAST_CONSTANT_OFFSET_MODE

			    psrc += nl & (UNROLL-1);
			    pdst += nl & (UNROLL-1);

#define BodyOdd(n) \
    bits = psrc[-n]; \
    pdst[-n] = BitLeft(bits1, leftShift) | BitRight(bits, rightShift);

#define BodyEven(n) \
    bits1 = psrc[-n]; \
    pdst[-n] = BitLeft(bits, leftShift) | BitRight(bits1, rightShift);

#define LoopReset \
    pdst += UNROLL; \
    psrc += UNROLL;

#else

#define BodyOdd(n) \
    bits = *psrc++; \
    *pdst++ = BitLeft(bits1, leftShift) | BitRight(bits, rightShift);
			   
#define BodyEven(n) \
    bits1 = *psrc++; \
    *pdst++ = BitLeft(bits, leftShift) | BitRight(bits1, rightShift);

#define LoopReset   ;

#endif	/* !FAST_CONSTANT_OFFSET_MODE */

			    PackedLoop

#undef BodyOdd
#undef BodyEven
#undef LoopReset
    
#else
			    DuffL (nl,label2,
				bits1 = BitLeft(bits, leftShift);
				bits = *psrc++;
				*pdst++ = bits1 | BitRight(bits, rightShift);
			    )
#endif
			    if (endmask)
			    {
			    	bits1 = SCRLEFT(bits, leftShift);
			    	if (SCRLEFT(endmask, rightShift))
			    	{
				    bits = *psrc++;
				    bits1 |= SCRRIGHT(bits, rightShift);
			    	}
			    	*pdst = (*pdst & ~endmask) |
				    	(bits1 & endmask);
			    }
		    	}
		    }
	    	}
	    	else	/* xdir == -1 */
	    	{
	    	    xoffSrc = (pptSrc->x + w - 1) & 0x1f;
	    	    xoffDst = (pbox->x2 - 1) & 0x1f;
		    pdstLine += ((pbox->x2-1) >> 5) + 1;
		    psrcLine += ((pptSrc->x+w - 1) >> 5) + 1;
		    if (xoffSrc == xoffDst)
		    {
	    	    	while (h--)
	    	    	{
		    	    psrc = psrcLine;
		    	    pdst = pdstLine;
		    	    pdstLine += widthDst;
		    	    psrcLine += widthSrc;
			    if (endmask)
			    {
			    	pdst--;
			    	*pdst = (*pdst & ~endmask) | (*--psrc & endmask);
			    }
			    nl = nlMiddle;

#ifdef LARGE_INSTRUCTION_CACHE
#ifdef FAST_CONSTANT_OFFSET_MODE
			    psrc -= nl & (UNROLL - 1);
			    pdst -= nl & (UNROLL - 1);

#define BodyOdd(n) pdst[n-1] = psrc[n-1];

#define BodyEven(n) pdst[n-1] = psrc[n-1];

#define LoopReset \
    pdst -= UNROLL;\
    psrc -= UNROLL;

#else

#define BodyOdd(n)  *--pdst = *--psrc;
#define BodyEven(n) BodyOdd(n)
#define LoopReset   ;

#endif
			    PackedLoop

#undef BodyOdd
#undef BodyEven
#undef LoopReset

#else
			    DuffL(nl,label3, *--pdst = *--psrc;)
#endif
			    if (startmask)
			    {
			    	--pdst;
			    	*pdst = (*pdst & ~startmask) | (*--psrc & startmask);
			    }
		    	}
		    }
		    else
		    {
			if (xoffDst > xoffSrc)
			{
			    rightShift = (xoffDst - xoffSrc);
			    leftShift = 32 - rightShift;
			}
			else
			{
		    	    leftShift = (xoffSrc - xoffDst);
		    	    rightShift = 32 - leftShift;
			}
	    	    	while (h--)
	    	    	{
		    	    psrc = psrcLine;
		    	    pdst = pdstLine;
		    	    pdstLine += widthDst;
		    	    psrcLine += widthSrc;
			    bits = 0;
			    if (xoffDst > xoffSrc)
				bits = *--psrc;
			    if (endmask)
			    {
			    	bits1 = SCRRIGHT(bits, rightShift);
			    	bits = *--psrc;
			    	bits1 |= SCRLEFT(bits, leftShift);
			    	pdst--;
			    	*pdst = (*pdst & ~endmask) |
				    	(bits1 & endmask);
			    }
			    nl = nlMiddle;

#ifdef LARGE_INSTRUCTION_CACHE
			    bits1 = bits;
#ifdef FAST_CONSTANT_OFFSET_MODE
			    psrc -= nl & (UNROLL - 1);
			    pdst -= nl & (UNROLL - 1);

#define BodyOdd(n) \
    bits = psrc[n-1]; \
    pdst[n-1] = BitRight(bits1, rightShift) | BitLeft(bits, leftShift);

#define BodyEven(n) \
    bits1 = psrc[n-1]; \
    pdst[n-1] = BitRight(bits, rightShift) | BitLeft(bits1, leftShift);

#define LoopReset \
    pdst -= UNROLL; \
    psrc -= UNROLL;

#else

#define BodyOdd(n) \
    bits = *--psrc; \
    *--pdst = BitRight(bits1, rightShift) | BitLeft(bits, leftShift);

#define BodyEven(n) \
    bits1 = *--psrc; \
    *--pdst = BitRight(bits, rightShift) | BitLeft(bits1, leftShift);

#define LoopReset   ;

#endif

			    PackedLoop

#undef BodyOdd
#undef BodyEven
#undef LoopReset

#else
			    DuffL (nl, label4,
				bits1 = BitRight(bits, rightShift);
				bits = *--psrc;
				*--pdst = bits1 | BitLeft(bits, leftShift);
			    )
#endif

			    if (startmask)
			    {
			    	bits1 = SCRRIGHT(bits, rightShift);
			    	if (SCRRIGHT (startmask, leftShift))
			    	{
				    bits = *--psrc;
				    bits1 |= SCRLEFT(bits, leftShift);
			    	}
			    	--pdst;
			    	*pdst = (*pdst & ~startmask) |
				    	(bits1 & startmask);
			    }
		    	}
		    }
	    	}
	    }
	    pbox++;
	    pptSrc++;
	}
    }
    else /* do some rop */
    {
        while (nbox--)
        {
	    w = pbox->x2 - pbox->x1;
	    h = pbox->y2 - pbox->y1;

	    if (ydir == -1) /* start at last scanline of rectangle */
	    {
	        psrcLine = psrcBase + ((pptSrc->y+h-1) * -widthSrc);
	        pdstLine = pdstBase + ((pbox->y2-1) * -widthDst);
	    }
	    else /* start at first scanline */
	    {
	        psrcLine = psrcBase + (pptSrc->y * widthSrc);
	        pdstLine = pdstBase + (pbox->y1 * widthDst);
	    }

	    /* x direction doesn't matter for < 1 longword */
	    if (w <= 32)
	    {
	        int srcBit, dstBit;	/* bit offset of src and dst */

	        pdstLine += (pbox->x1 >> 5);
	        psrcLine += (pptSrc->x >> 5);
	        psrc = psrcLine;
	        pdst = pdstLine;

	        srcBit = pptSrc->x & 0x1f;
	        dstBit = pbox->x1 & 0x1f;

	        while(h--)
	        {
		    getandputrop(psrc, srcBit, dstBit, w, pdst, alu)
		    pdst += widthDst;
		    psrc += widthSrc;
	        }
	    }
	    else
	    {
	        maskbits(pbox->x1, w, startmask, endmask, nlMiddle)
	        if (startmask)
		    nstart = 32 - (pbox->x1 & 0x1f);
	        else
		    nstart = 0;
	        if (endmask)
	            nend = pbox->x2 & 0x1f;
	        else
		    nend = 0;

	        xoffSrc = ((pptSrc->x & 0x1f) + nstart) & 0x1f;
	        srcStartOver = ((pptSrc->x & 0x1f) + nstart) > 31;

	        if (xdir == 1) /* move left to right */
	        {
	            pdstLine += (pbox->x1 >> 5);
	            psrcLine += (pptSrc->x >> 5);

		    while (h--)
		    {
		        psrc = psrcLine;
		        pdst = pdstLine;

		        if (startmask)
		        {
			    getandputrop(psrc, (pptSrc->x & 0x1f), 
					 (pbox->x1 & 0x1f), nstart, pdst, alu)
			    pdst++;
			    if (srcStartOver)
			        psrc++;
		        }

			/* special case for aligned operations */
			if (xoffSrc == 0)
			{
			    nl = nlMiddle;
			    while (nl--)
			    {
				DoRop (*pdst, alu, *psrc++, *pdst);
 				pdst++;
			    }
			}
 			else
			{
			    nl = nlMiddle + 1;
			    while (--nl)
		            {
				getunalignedword (psrc, xoffSrc, tmpSrc)
				DoRop (*pdst, alu, tmpSrc, *pdst);
				pdst++;
				psrc++;
			    }
			}

		        if (endmask)
		        {
			    getandputrop0(psrc, xoffSrc, nend, pdst, alu);
		        }

		        pdstLine += widthDst;
		        psrcLine += widthSrc;
		    }
	        }
	        else /* move right to left */
	        {
	            pdstLine += (pbox->x2 >> 5);
	            psrcLine += (pptSrc->x+w >> 5);
		    /* if fetch of last partial bits from source crosses
		       a longword boundary, start at the previous longword
		    */
		    if (xoffSrc + nend >= 32)
		        --psrcLine;

		    while (h--)
		    {
		        psrc = psrcLine;
		        pdst = pdstLine;

		        if (endmask)
		        {
			    getandputrop0(psrc, xoffSrc, nend, pdst, alu);
		        }

		        nl = nlMiddle + 1;
		        while (--nl)
		        {
			    --psrc;
			    --pdst;
			    getunalignedword(psrc, xoffSrc, tmpSrc)
			    DoRop(*pdst, alu, tmpSrc, *pdst);
		        }

		        if (startmask)
		        {
			    if (srcStartOver)
			        --psrc;
			    --pdst;
			    getandputrop(psrc, (pptSrc->x & 0x1f), 
					 (pbox->x1 & 0x1f), nstart, pdst, alu)
		        }

		        pdstLine += widthDst;
		        psrcLine += widthSrc;
		    }
	        } /* move right to left */
	    }
	    pbox++;
	    pptSrc++;
        } /* while (nbox--) */
    }

    /* free up stuff */
    if (pboxNew2)
    {
	DEALLOCATE_LOCAL(pptNew2);
	DEALLOCATE_LOCAL(pboxNew2);
    }
    if (pboxNew1)
    {
	DEALLOCATE_LOCAL(pptNew1);
	DEALLOCATE_LOCAL(pboxNew1);
    }
}

/*
 * Allow devices which use mfb for 1-bit pixmap support
 * to register a function for n-to-1 copy operations, instead
 * of falling back to miCopyPlane
 */

static unsigned long	copyPlaneGeneration;
static int		copyPlaneScreenIndex = -1;

Bool
mfbRegisterCopyPlaneProc (pScreen, proc)
    ScreenPtr	pScreen;
    RegionPtr	(*proc)();
{
    if (copyPlaneGeneration != serverGeneration)
    {
	copyPlaneScreenIndex = AllocateScreenPrivateIndex();
	if (copyPlaneScreenIndex < 0)
	    return FALSE;
	copyPlaneGeneration = serverGeneration;
    }
    pScreen->devPrivates[copyPlaneScreenIndex].ptr = (pointer) proc;
    return TRUE;
}

/*
    if fg == 1 and bg ==0, we can do an ordinary CopyArea.
    if fg == bg, we can do a CopyArea with alu = mfbReduceRop(alu, fg)
    if fg == 0 and bg == 1, we use the same rasterop, with
	source operand inverted.

    CopyArea deals with all of the graphics exposure events.
    This code depends on knowing that we can change the
alu in the GC without having to call ValidateGC() before calling
CopyArea().

*/

RegionPtr
mfbCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, plane)
DrawablePtr pSrcDrawable, pDstDrawable;
register GC *pGC;
int srcx, srcy;
int width, height;
int dstx, dsty;
unsigned long plane;
{
    int alu;
    RegionPtr	prgnExposed;
    RegionPtr	(*copyPlane)();

    if (pSrcDrawable->depth != 1)
    {
	if (copyPlaneScreenIndex >= 0 &&
	    (copyPlane = (RegionPtr (*)()) 
		pSrcDrawable->pScreen->devPrivates[copyPlaneScreenIndex].ptr)
	    )
	{
	    return (*copyPlane) (pSrcDrawable, pDstDrawable,
			   pGC, srcx, srcy, width, height, dstx, dsty, plane);
	}
	else
	    return miCopyPlane(pSrcDrawable, pDstDrawable,
			   pGC, srcx, srcy, width, height, dstx, dsty, plane);
    }
    if (plane != 1)
	return NULL;

    if ((pGC->fgPixel == 1) && (pGC->bgPixel == 0))
    {
	prgnExposed = (*pGC->ops->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
    }
    else if (pGC->fgPixel == pGC->bgPixel)
    {
	alu = pGC->alu;
	pGC->alu = mfbReduceRop(pGC->alu, pGC->fgPixel);
	prgnExposed = (*pGC->ops->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
	pGC->alu = alu;
    }
    else /* need to invert the src */
    {
	alu = pGC->alu;
	pGC->alu = InverseAlu[alu];
	prgnExposed = (*pGC->ops->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
	pGC->alu = alu;
    }
    return prgnExposed;
}
