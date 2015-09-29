/*
 * Poly glyph blt for 8 bit displays.  Accepts
 * an arbitrary font <= 32 bits wide, in Copy mode only.
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
/* $XConsortium: cfbglblt8.c,v 5.6 89/11/21 15:31:12 keith Exp $ */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"cfb.h"
#include	"cfbmskbits.h"
#include	"cfb8bit.h"

#if PPW == 4

#if GLYPHPADBYTES != 4
#define USE_LEFTBITS
#endif

#ifdef USE_LEFTBITS
typedef	unsigned char	*glyphPointer;
extern long endtab[];

#define GlyphBits(bits,width,dst)	getleftbits(bits,width,dst); \
					(dst) &= widthMask; \
					(bits) += widthGlyph;
#define GlyphBitsS(bits,width,dst,off)	GlyphBits(bits,width,dst); \
					dst = BitRight (dst, off);
#else
typedef unsigned long	*glyphPointer;

#define GlyphBits(bits,width,dst)	dst = *bits++;
#define GlyphBitsS(bits,width,dst,off)	dst = BitRight(*bits++, off);
#endif

void
cfbPolyGlyphBlt8 (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    register unsigned long  c;
    register unsigned long  pixel;
    register unsigned long  *dst;
    register glyphPointer   glyphBits;
    register int	    xoff;
    register int	    ewTmp;

    CharInfoPtr		pci;
    FontInfoPtr		pfi = pGC->font->pFI;
    unsigned long	*dstLine;
    unsigned long	*pdstBase;
    int			hTmp;
    int			widthDst;
    int			h;
    int			ew;
    BoxRec		bbox;		/* for clipping */
    int			widthDiff;
    int			w;
#ifdef USE_LEFTBITS
    int			widthGlyph;
    unsigned long	widthMask;
#endif

    pixel = PFILL (pGC->fgPixel);
    
    /* compute an approximate (but covering) bounding box */
    if ((ppci[0]->metrics.leftSideBearing < 0))
	bbox.x1 = ppci[0]->metrics.leftSideBearing;
    else
	bbox.x1 = 0;
    w = 0;
    for (h = 0; h < nglyph; h++)
	w += ppci[h]->metrics.characterWidth;
    bbox.x2 = w - ppci[nglyph-1]->metrics.characterWidth;
    bbox.x2 += ppci[nglyph-1]->metrics.rightSideBearing;
    bbox.y1 = -pfi->maxbounds.metrics.ascent;
    bbox.y2 = pfi->maxbounds.metrics.descent;

    bbox.x1 += x + pDrawable->x;
    bbox.x2 += x + pDrawable->x;
    bbox.y1 += y + pDrawable->y;
    bbox.y2 += y + pDrawable->y;

    switch ((*pGC->pScreen->RectIn)(
                ((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip, &bbox))
    {
      case rgnPART:
	cfbPolyGlyphBlt8Clipped(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
      case rgnOUT:
	return;
    }

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (unsigned long *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthDst = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	pdstBase = (unsigned long *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }
    x += pDrawable->x;
    y += pDrawable->y;
    while (nglyph--)
    {
	pci = *ppci++;
	glyphBits = (glyphPointer) (pglyphBase + pci->byteOffset);
	w = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
	xoff = x + pci->metrics.leftSideBearing;
	dstLine = pdstBase +
	          (y - pci->metrics.ascent) * widthDst + (xoff >> 2);
	xoff &= 0x3;
	ew = (w + xoff + 3) >> 2;
	hTmp = pci->metrics.descent + pci->metrics.ascent;
	dst = dstLine;
#ifdef USE_LEFTBITS
	widthGlyph = GLYPHWIDTHBYTESPADDED (pci);
	widthMask = endtab[w];
#endif
	switch (ew) {
	case 0:
	    break;
	case 1:
	    while (hTmp--)
	    {
		GlyphBitsS(glyphBits, w, c, xoff)
		WriteFourBits (dst, pixel, GetFourBits(c));
	    	dst += widthDst;
	    }
	    break;
	case 2:
	    while (hTmp--)
	    {
		GlyphBitsS(glyphBits, w, c, xoff)
	    	if (c)
	    	{
		    WriteFourBits (dst, pixel, GetFourBits(c));
		    NextFourBits(c);
		    WriteFourBits (dst+1, pixel, GetFourBits(c));
	    	}
	    	dst += widthDst;
	    }
	    break;
	case 3:
	    while (hTmp--)
	    {
		GlyphBitsS(glyphBits, w, c, xoff)
	    	if (c)
	    	{
		    WriteFourBits (dst, pixel, GetFourBits(c));
		    NextFourBits(c);
		    WriteFourBits (dst+1, pixel, GetFourBits(c));
		    NextFourBits(c);
		    WriteFourBits (dst+2, pixel, GetFourBits(c));
	    	}
	    	dst += widthDst;
	    }
	    break;
	case 9:
	    widthDiff = widthDst - ew;
	    dst -= widthDiff;
	    while (hTmp--)
	    {
#if defined(__GNUC__) && defined(mc68020)
		volatile unsigned long c1; /* XXX 1.35 is really stupid */
#else
		unsigned long c1;
#endif

		GlyphBits(glyphBits, w, c)
	    	dst += widthDiff;
	    	if (!c)
	    	{
		    dst += ew;
		    continue;
	    	}
		c1 = BitLeft (c, 32 - xoff);
	    	c = BitRight (c, xoff);
	    	ewTmp = ew - 1;
	    	while (ewTmp--) {
		    WriteFourBits(dst, pixel, GetFourBits(c))
		    dst++;
		    NextFourBits(c);
	    	}
		WriteFourBits(dst, pixel, GetFourBits(c1));
		dst++;
	    }
	    break;
	default:
	    widthDiff = widthDst - ew;
	    dst -= widthDiff;
	    while (hTmp--)
	    {
		GlyphBitsS(glyphBits, w, c, xoff)
	    	dst += widthDiff;
	    	if (!c)
	    	{
		    dst += ew;
		    continue;
	    	}
	    	ewTmp = ew;
	    	while (ewTmp--) {
		    WriteFourBits(dst, pixel, GetFourBits(c))
		    dst++;
		    NextFourBits(c);
	    	}
	    }
	}
	x += pci->metrics.characterWidth;
    }
}

cfbPolyGlyphBlt8Clipped (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    register unsigned long  c;
    register unsigned long  pixel;
    register unsigned long  *dst;
    register glyphPointer   glyphBits;
    register int	    xoff;
    register int	    ewTmp;
    unsigned long	    c1;

    CharInfoPtr		pci;
    FontInfoPtr		pfi = pGC->font->pFI;
    unsigned long	*dstLine;
    unsigned long	*pdstBase;
    unsigned long	*cTmp, *clips;
    int			maxAscent, maxDescent;
    int			hTmp;
    int			widthDst;
    int			ew;
    int			xG, yG;
    BoxPtr		pBox;
    int			numRects;
    int			widthDiff;
    int			w;
    RegionPtr		pRegion;
    int			yBand;
#ifdef USE_LEFTBITS
    int			widthGlyph;
    unsigned long	widthMask;
#endif

    pixel = PFILL (pGC->fgPixel);
    
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (unsigned long *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate.ptr);
	widthDst = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	pdstBase = (unsigned long *)(((PixmapPtr)pDrawable)->devPrivate.ptr);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }
    x += pDrawable->x;
    y += pDrawable->y;
    maxAscent = pfi->maxbounds.metrics.ascent;
    maxDescent = pfi->maxbounds.metrics.descent;

    pRegion = ((cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr))->pCompositeClip;

    pBox = REGION_RECTS(pRegion);
    numRects = REGION_NUM_RECTS (pRegion);
    while (numRects && pBox->y2 <= y - maxAscent)
    {
	++pBox;
	--numRects;
    }
    if (!numRects || pBox->y1 >= y + maxDescent)
	return;
    yBand = pBox->y1;
    while (numRects && pBox->y1 == yBand && pBox->x2 <= x + pfi->minbounds.metrics.leftSideBearing)
    {
	++pBox;
	--numRects;
    }
    if (!numRects)
	return;
    clips = (unsigned long *)ALLOCATE_LOCAL ((maxAscent + maxDescent) *
					     sizeof (unsigned long));
    while (nglyph--)
    {
	pci = *ppci++;
	glyphBits = (glyphPointer) (pglyphBase + pci->byteOffset);
	w = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
	xG = x + pci->metrics.leftSideBearing;
	yG = y - pci->metrics.ascent;
	dstLine = pdstBase + yG * widthDst + (xG >> 2);
	xoff = xG & 0x3;
	ew = (w + xoff + 3) >> 2;
	hTmp = pci->metrics.descent + pci->metrics.ascent;
	widthDiff = widthDst - ew;
	dst = dstLine - widthDiff;
#ifdef USE_LEFTBITS
	widthGlyph = GLYPHWIDTHBYTESPADDED (pci);
	widthMask = endtab[w];
#endif
	switch (cfb8ComputeClipMasks32 (pBox, numRects, xG, yG, w, hTmp, clips))
 	{
	case rgnIN:
	    if (ew <= 8)
	    {
		while (hTmp--)
		{
		    GlyphBitsS(glyphBits, w, c, xoff)
	    	    dst += widthDiff;
	    	    if (!c)
	    	    {
		    	dst += ew;
		    	continue;
	    	    }
	    	    ewTmp = ew;
	    	    while (ewTmp--) {
		    	WriteFourBits (dst, pixel, GetFourBits(c));
		    	dst++;
		    	NextFourBits(c);
	    	    }
		}
	    }
	    else
	    {
		while (hTmp--)
		{
		    GlyphBits(glyphBits, w, c)
	    	    dst += widthDiff;
	    	    if (!c)
	    	    {
		    	dst += ew;
		    	continue;
	    	    }
		    c1 = BitLeft (c, 32 - xoff);
	    	    c = BitRight (c, xoff);
	    	    ewTmp = ew-1;
	    	    while (ewTmp--) {
		    	WriteFourBits (dst, pixel, GetFourBits(c));
		    	dst++;
		    	NextFourBits(c);
	    	    }
		    WriteFourBits(dst, pixel, GetFourBits(c1));
		    dst++;
		}
	    }
	    break;
	case rgnPART:
	    cTmp = clips;
	    if (ew <= 8)
	    {
	    	while (hTmp--)
	    	{
		    GlyphBits(glyphBits, w, c)
		    c = BitRight (c & *cTmp++, xoff);
		    dst += widthDiff;
		    if (!c)
		    {
		    	dst += ew;
		    	continue;
		    }
		    ewTmp = ew;
		    while (ewTmp--) {
			c1 = GetFourBits(c);
			if (c1)
			{
			    WriteFourBits (dst, pixel, c1);
			}
		    	dst++;
		    	NextFourBits(c);
		    }
	    	}
	    }
	    else
	    {
	    	while (hTmp--)
	    	{
		    GlyphBits(glyphBits, w, c)
		    c &= *cTmp++;
		    dst += widthDiff;
		    if (!c)
		    {
		    	dst += ew;
		    	continue;
		    }
		    c1 = BitLeft (c, 32 - xoff);
		    c = BitRight (c, xoff);
		    ewTmp = ew - 1;
		    while (ewTmp--) {
			if (GetFourBits(c))
			{
			    WriteFourBits (dst, pixel, GetFourBits(c));
			}
		    	dst++;
		    	NextFourBits(c);
		    }
		    if (GetFourBits(c1))
		    {
		    	WriteFourBits (dst, pixel, GetFourBits(c1));
		    }
		    dst++;
	    	}
	    }
	    break;
	}
	x += pci->metrics.characterWidth;
    }
    DEALLOCATE_LOCAL (clips);
}

#endif /* PPW == 4 */
