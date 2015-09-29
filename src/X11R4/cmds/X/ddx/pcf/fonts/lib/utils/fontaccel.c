/************************************************************************
Copyright 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
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

************************************************************************/

/* $Header: fontaccel.c,v 5.6 89/09/11 15:11:41 erik Exp $ */

#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"

static void computeInk(), computeInkMetrics(), padGlyphsToTE();

/*
 * Computes accelerators, which by definition can be extracted from
 * the font; therefore a pointer to the font is the only argument.
 *
 * Should also give a final sanity-check to the font, so it can be
 * written to disk.
 *
 * Generates values for the following fields in the CharSet structure:
 *	version
 *	allExist
 *	noOverlap
 *	constantMetrics
 *	constantWidth
 *	terminalFont
 *	inkMetrics
 *	linear
 *	inkInside
 *	minbounds
 *	maxbounds	- except byteOffset 
 *	pInkCI
 *	pInkMin
 *	pInkMax
 */

void
ComputeInfoAccelerators(font, makeTEfonts, inhibitInk, glyphPad)
    EncodedFontPtr font;
    Bool makeTEfonts;
    Bool inhibitInk;
    Bool glyphPad;
{
    register CharSetPtr pCS= font->pCS;

    if (pCS->maxOverlap <= pCS->minbounds.leftSideBearing)
	pCS->noOverlap = TRUE;
    else
	pCS->noOverlap = FALSE;

    /*
     * attempt to make this font a terminal emulator font if
     * it isn't already
     */

    if ( makeTEfonts &&
	 (pCS->minbounds.leftSideBearing >= 0) &&
	 (pCS->maxbounds.rightSideBearing <= pCS->maxbounds.characterWidth) &&
	 (pCS->minbounds.characterWidth == pCS->maxbounds.characterWidth) &&
	 (pCS->maxbounds.ascent <= pCS->fontAscent) &&
	 (pCS->maxbounds.descent <= pCS->fontDescent) &&
	 (pCS->maxbounds.leftSideBearing != 0 ||
	  pCS->minbounds.rightSideBearing != pCS->minbounds.characterWidth ||
	  pCS->minbounds.ascent != pCS->fontAscent ||
	  pCS->minbounds.descent != pCS->fontDescent) )
    {
	padGlyphsToTE (font, pCS->minbounds.characterWidth, glyphPad);
    } 

    if ( (pCS->minbounds.ascent == pCS->maxbounds.ascent) &&
         (pCS->minbounds.descent == pCS->maxbounds.descent) &&
	 (pCS->minbounds.leftSideBearing ==
		pCS->maxbounds.leftSideBearing) &&
	 (pCS->minbounds.rightSideBearing ==
		pCS->maxbounds.rightSideBearing) &&
	 (pCS->minbounds.characterWidth ==
		pCS->maxbounds.characterWidth) &&
	 (pCS->minbounds.attributes == pCS->maxbounds.attributes)) {
	pCS->constantMetrics = TRUE;
	if ( (pCS->maxbounds.leftSideBearing == 0) &&
	     (pCS->maxbounds.rightSideBearing ==
			pCS->maxbounds.characterWidth) &&
	     (pCS->maxbounds.ascent == pCS->fontAscent) &&
	     (pCS->maxbounds.descent == pCS->fontDescent) )
	         pCS->terminalFont = TRUE;
	else
		 pCS->terminalFont = FALSE;
    }
    else {
	pCS->constantMetrics = FALSE;
	pCS->terminalFont = FALSE;
    }

    if (pCS->terminalFont && !inhibitInk) {
	computeInkMetrics(font);
    }
    else {
	pCS->inkMetrics = FALSE;
	font->ppInkCI = (CharInfoPtr *)NULL;
    }

    if( font->lastRow == font->firstRow)
	font->linear = TRUE;
    else
	font->linear = FALSE;

    if (pCS->minbounds.characterWidth == pCS->maxbounds.characterWidth)
	pCS->constantWidth = TRUE;
    else
	pCS->constantWidth = FALSE;

    if ((pCS->minbounds.leftSideBearing >= 0) &&
	(pCS->maxOverlap <= 0) &&
	(pCS->minbounds.ascent >= -pCS->fontDescent) &&
	(pCS->maxbounds.ascent <= pCS->fontAscent) &&
	(-pCS->minbounds.descent <= pCS->fontAscent) &&
	(pCS->maxbounds.descent <= pCS->fontDescent))
	pCS->inkInside = TRUE;
    else
	pCS->inkInside = FALSE;
}



static BuildParamsRec naturalParams = {
	DEFAULTGLPAD,
	DEFAULTBITORDER,
	DEFAULTBYTEORDER,
	DEFAULTSCANUNIT,
	0,0,0,0
};

static void
computeInkMetrics (font)
    EncodedFontPtr font;
{
    register int i;

    font->pCS->inkci.pCI = (CharInfoPtr) fosAlloc (
	(font->pCS->nChars) * sizeof (CharInfoRec));
    if (font->pCS->inkci.pCI==NULL) {
	fosWarning("Couldn't allocate inkmetrics (%d*%d)\n",font->pCS->nChars,
							   sizeof(CharInfoRec));
	fosInformation("Font will have no ink metrics\n");
	font->pCS->inkMetrics=	FALSE;
	font->pCS->tables&= (~FONT_INK_METRICS);
	return;
    }
    for (i = 0; i < font->pCS->nChars; i++)
	computeInk(&font->pCS->ci.pCI[i], &font->pCS->inkci.pCI[i], 
					font->pCS->pBitOffsets[i],
					&naturalParams);
    font->pCS->inkMetrics = TRUE;
    font->pCS->tables|= FONT_INK_METRICS;
    return;
}

static unsigned char ink_mask[8] = {
     0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,
};

static void
computeInk (pCI, pInkCI, pBitmap, params)
    CharInfoPtr	 pCI, pInkCI;
    unsigned char *pBitmap;
    BuildParamsPtr params;
{
    int 		leftBearing, ascent, descent;
    register int	vpos, hpos, bpos;
    int			bitmapByteWidth, bitmapByteWidthPadded;
    int			bitmapBitWidth;
    int			span;
    register unsigned char	*p;
    register int	bmax;
    register unsigned char	charbits;
    
    pInkCI->metrics.characterWidth = pCI->metrics.characterWidth;
    pInkCI->metrics.attributes = pCI->metrics.attributes;
    pInkCI->pPriv=		NULL;

    leftBearing = pCI->metrics.leftSideBearing;
    ascent = pCI->metrics.ascent;
    descent = pCI->metrics.descent;
    bitmapBitWidth = GLYPHWIDTHPIXELS (pCI);
    bitmapByteWidth = GLYPHWIDTHBYTES (pCI);
    bitmapByteWidthPadded = BYTES_PER_ROW (bitmapBitWidth, params->glyphPad);
    span = bitmapByteWidthPadded - bitmapByteWidth;

    p = pBitmap;
    for (vpos = descent + ascent; --vpos >= 0;)
    {
	for (hpos = bitmapByteWidth; --hpos >= 0;)
 	{
	    if (*p++ != 0)
	        goto found_ascent;
	}
	p += span;
    }
    /*
     * special case -- font with no bits gets all zeros
     */
    pInkCI->metrics.leftSideBearing = leftBearing;
    pInkCI->metrics.rightSideBearing = leftBearing;
    pInkCI->metrics.ascent = 0;
    pInkCI->metrics.descent = 0;
    return;
found_ascent:
    pInkCI->metrics.ascent = vpos - descent + 1;

    p = pBitmap+bitmapByteWidthPadded*(descent+ascent-1)+bitmapByteWidth;

    for (vpos = descent + ascent; --vpos >= 0;)
    {
	for (hpos = bitmapByteWidth; --hpos >= 0;)
 	{
	    if (*--p != 0)
	        goto found_descent;
	}
	p -= span;
    }
found_descent:
    pInkCI->metrics.descent = vpos - ascent + 1;
    
    bmax = 8;
    for (hpos = 0; hpos < bitmapByteWidth; hpos++)
    {
	charbits = 0;
	p = pBitmap + hpos;
	for (vpos = descent + ascent; --vpos >= 0; p += bitmapByteWidthPadded)
	    charbits |= *p;
	if (charbits) 
	{
	    if (hpos == bitmapByteWidth - 1)
		bmax = bitmapBitWidth - (hpos << 3);
	    p = ink_mask;
	    for (bpos = bmax; --bpos >= 0;)
	    {
		if (charbits & *p++)
		    goto found_left;
	    }
	}
    }
found_left:
    pInkCI->metrics.leftSideBearing = leftBearing + (hpos << 3) + bmax - bpos - 1;

    bmax = bitmapBitWidth - ((bitmapByteWidth - 1) << 3);
    for (hpos = bitmapByteWidth; --hpos >= 0;)
    {
	charbits = 0;
	p = pBitmap + hpos;
	for (vpos = descent + ascent; --vpos >= 0; p += bitmapByteWidthPadded)
	    charbits |= *p;
	if (charbits)
	{
	    p = ink_mask + bmax;
	    for (bpos = bmax; --bpos >= 0;)
	    {
		if (charbits & *--p)
		    goto found_right;
	    }
	}
	bmax = 8;
    }
found_right:
    pInkCI->metrics.rightSideBearing = leftBearing + (hpos << 3) + bpos + 1;
}

/***====================================================================***/

#define	ISBITON(x, line)	((line)[(x)/8] & (1 << (7-((x)%8))))
#define	SETBIT(x, line)		((line)[(x)/8] |= (1 << (7-((x)%8))))

void
padGlyph(pCI, oldglyph, newglyph, width, ascent, descent, glyphPad)
register CharInfoPtr 	 pCI;
unsigned char		*oldglyph;
unsigned char		*newglyph;
int		 	 width, ascent, descent;
int 		 	 glyphPad;
{
int		 x, y;
int		 dx, dy;
unsigned char	*inline, *outline;
int		 inwidth,widthBits;

    dx=		pCI->metrics.leftSideBearing; 
    dy=		ascent - pCI->metrics.ascent;
    bzero(newglyph, width * (ascent + descent));
    inline=	oldglyph;
    outline=	newglyph + dy * width;
    inwidth=	BYTES_PER_ROW(pCI->metrics.characterWidth, glyphPad);
    for (y = 0; y < pCI->metrics.ascent + pCI->metrics.descent; y++) {
	widthBits= pCI->metrics.rightSideBearing-pCI->metrics.leftSideBearing;
	for (x= 0;x<widthBits;x++) {
	    if (ISBITON (x, inline))
		SETBIT (x + dx, outline);
	}
	inline += inwidth;
	outline += width;
    }
    pCI->metrics.leftSideBearing=	0;
    pCI->metrics.rightSideBearing=	pCI->metrics.characterWidth;
    pCI->metrics.ascent=		ascent;
    pCI->metrics.descent=		descent;
    return;
}

/***====================================================================***/

/*
 * given a constant width font, convert it to a terminal emulator font
 */

static void
padGlyphsToTE(font, width, glyphPad)
EncodedFontPtr	font;
int	width;
int	glyphPad;
{
char		*newGlyphs;
int		 glyphwidth, glyphascent, glyphdescent, glyphsize;
Mask		 oldminattr, oldmaxattr;
register int	 i;

    glyphwidth=		BYTES_PER_ROW(width, glyphPad);
    glyphascent=	font->pCS->fontAscent;
    glyphdescent=	font->pCS->fontDescent;
    glyphsize=		glyphwidth * (glyphascent + glyphdescent);
    newGlyphs=		(char *)fosAlloc(font->pCS->nChars * glyphsize);

/* 6/22/89 (ef) -- should this return an error? */
    if (!newGlyphs)
	return;

    for (i = 0; i < font->pCS->nChars; i++) {
	padGlyph (&font->pCS->ci.pCI[i], 
			  &newGlyphs[glyphsize*i], font->pCS->pBitOffsets[i],
			  glyphwidth, glyphascent, glyphdescent, glyphPad);
	if ((font->pCS->pBitmaps==NULL)&&(font->pCS->pBitOffsets[i]!=NULL)) {
	    fosFree(font->pCS->pBitOffsets[i]);
	}
	font->pCS->pBitOffsets[i]=	(char *)&newGlyphs[glyphsize*i];
    }

    /* the metrics should now be constant */
    oldminattr=	font->pCS->minbounds.attributes;
    oldmaxattr=	font->pCS->maxbounds.attributes;
    font->pCS->minbounds=	font->pCS->inkci.pCI[0].metrics;
    font->pCS->maxbounds=	font->pCS->inkci.pCI[0].metrics;
    font->pCS->minbounds.attributes=	oldminattr;
    font->pCS->maxbounds.attributes=	oldmaxattr;
    if (font->pCS->pBitmaps!=NULL) {
	fosFree(font->pCS->pBitmaps);
    }
    font->pCS->pBitmaps=	newGlyphs;
    return;
}

