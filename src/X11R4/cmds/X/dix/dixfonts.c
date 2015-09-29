/************************************************************************
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

************************************************************************/

/* $XConsortium: dixfonts.c,v 1.9 89/07/16 17:24:57 rws Exp $ */

#define NEED_REPLIES
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "scrnintstr.h"
#include "resource.h"
#include "dix.h"
#include "cursorstr.h"
#include "misc.h"
#include "opaque.h"

#define QUERYCHARINFO(pci, pr)  *(pr) = (pci)->metrics

extern FontPtr 	defaultFont;

/*
 * adding RT_FONT prevents conflict with default cursor font
 */
Bool
SetDefaultFont( defaultfontname)
    char *	defaultfontname;
{
    FontPtr	pf;

   pf = OpenFont( (unsigned)strlen( defaultfontname), defaultfontname);
    if (!pf || !AddResource(FakeClientID(0), RT_FONT, (pointer)pf))
	return FALSE;
    defaultFont = pf;
    return TRUE;
}

/*
 * Check reference count first, load font only if necessary.
 */
FontPtr 
OpenFont(lenfname, pfontname)
    unsigned	lenfname;
    char *	pfontname;
{
    FontPtr 	pfont;
    int		nscr;
    ScreenPtr	pscr;

    pfont = FontFileLoad(pfontname, lenfname);

    if (pfont == NullFont)
    {
#ifdef notdef
	ErrorF(  "OpenFont: read failed on file %s\n", ppathname);
#endif
	return NullFont;
    }

    if (pfont->refcnt != 0) {
	pfont->refcnt += 1;
	return pfont;
    }

    /*
     * since this font has been newly read off disk, ask each screen to
     * realize it.
     */
    pfont->refcnt = 1;
    for (nscr = 0; nscr < screenInfo.numScreens; nscr++)
    {
	pscr = screenInfo.screens[nscr];
        if ( pscr->RealizeFont)
	    ( *pscr->RealizeFont)( pscr, pfont);
    }
    return pfont;
}

/*
 * Decrement font's ref count, and free storage if ref count equals zero
 */
/*ARGSUSED*/
int
CloseFont(pfont, fid)
    FontPtr 	pfont;
    Font	fid;
{
    int		nscr;
    ScreenPtr	pscr;

    if (pfont == NullFont)
        return(Success);
    if (--pfont->refcnt == 0)
    {
	/*
	 * since the last reference is gone, ask each screen to
	 * free any storage it may have allocated locally for it.
	 */
	for (nscr = 0; nscr < screenInfo.numScreens; nscr++)
	{
	    pscr = screenInfo.screens[nscr];
	    if ( pscr->UnrealizeFont)
		( *pscr->UnrealizeFont)( pscr, pfont);
	}
	if (pfont == defaultFont)
	    defaultFont = NULL;
	FontUnload(pfont);
    }
   return(Success);
}

Bool
DescribeFont(pfontname, lenfname, pfi, ppfp)
    char *pfontname;
    int lenfname;
    FontInfoPtr pfi;
    DIXFontPropPtr *ppfp;	/* return */
{
    FontPtr pfont;
    Bool found;

    found = FontFilePropLoad(pfontname, (unsigned int)lenfname,
			     &pfont, pfi, ppfp);

    if (!found)
	return FALSE;
    if (pfont != NullFont) {	/* need to get it myself */
	*pfi = *pfont->pFI;
	if (pfi->inkMetrics) {
	    pfi->minbounds = *pfont->pInkMin;
	    pfi->maxbounds = *pfont->pInkMax;
	}
	if (pfi->nProps != 0) {
	    *ppfp = (DIXFontPropPtr)xalloc(sizeof(DIXFontProp)*pfi->nProps);
	    if (*ppfp == NullDIXFontProp)
		return FALSE;
	    bcopy((char *)pfont->pFP, (char *)*ppfp,
		  (int)(sizeof(DIXFontProp) * pfi->nProps));
	}
    }

    return TRUE;
}

void
QueryFont( pf, pr, nprotoxcistructs)
    FontPtr 		pf;
    xQueryFontReply *	pr;	/* caller must allocate this storage */
    int		nprotoxcistructs;
{
    FontInfoPtr 	pfi = pf->pFI;
    CharInfoPtr 	pci;
    DIXFontProp *	pfp;
    int		ct;
    xFontProp *	prfp;
    xCharInfo *	prci;

    /* pr->length set in dispatch */
    pr->minCharOrByte2 = pfi->firstCol;
    pr->defaultChar = pfi->chDefault;
    pr->maxCharOrByte2 = pfi->lastCol;
    pr->drawDirection = pfi->drawDirection;
    pr->allCharsExist = pfi->allExist;
    pr->minByte1 = pfi->firstRow;
    pr->maxByte1 = pfi->lastRow;
    pr->fontAscent = pfi->fontAscent;
    pr->fontDescent = pfi->fontDescent;

    QUERYCHARINFO( pf->pInkMin, &pr->minBounds); 
    QUERYCHARINFO( pf->pInkMax, &pr->maxBounds); 

    pr->nFontProps = pfi->nProps; 
    pr->nCharInfos = nprotoxcistructs; 


    for ( ct=0,
	    pfp=pf->pFP,
	    prfp=(xFontProp *)(&pr[1]);
	  ct < pfi->nProps;
	  ct++, pfp++, prfp++)
    {
	prfp->name = pfp->name;
	prfp->value = pfp->value;
    }

    for ( ct=0,
	    pci = &pf->pInkCI[0],
	    prci=(xCharInfo *)(prfp);
	  ct<nprotoxcistructs;
	  ct++, pci++, prci++)
	QUERYCHARINFO( pci, prci);
}

void
queryCharInfo( pci, pr)
    CharInfoPtr 		pci;
    xCharInfo *		pr;	/* protocol packet to fill in */
{
    QUERYCHARINFO(pci, pr);
}

/* text support routines. A charinfo array builder, and a bounding */
/* box calculator */

void
GetGlyphs(font, count, chars, fontEncoding, glyphcount, glyphs)
    FontPtr font;
    unsigned long count;
    register unsigned char *chars;
    FontEncoding fontEncoding;
    unsigned long *glyphcount;	/* RETURN */
    CharInfoPtr glyphs[];	/* RETURN */
{
    CharInfoPtr		pCI = font->pCI;
    FontInfoPtr		pFI = font->pFI;
    unsigned int	firstCol = pFI->firstCol;
    unsigned int	numCols = pFI->lastCol - firstCol + 1;
    unsigned int	firstRow = pFI->firstRow;
    unsigned int	numRows = pFI->lastRow - firstRow + 1;
    unsigned int	chDefault = pFI->chDefault;
    unsigned int	cDef = chDefault - firstCol;
    register unsigned long	i;
    unsigned long		n;
    register unsigned int	c;
    register CharInfoPtr	ci;

    n = 0;
    switch (fontEncoding) {

	case Linear8Bit:
	case TwoD8Bit:
	    if (pFI->allExist && (cDef < numCols)) {
		for (i=0; i < count; i++) {

		    c = (*chars++) - firstCol;
		    if (c >= numCols) {
			c = cDef;
		    }
		    ci = &pCI[c];
		    glyphs[i] = ci;
		}
		n = count;
	    } else {
		for (i=0; i < count; i++) {
    
		    c = (*chars++) - firstCol;
		    if (c < numCols) {
			ci = &pCI[c];
			if (ci->exists) {glyphs[n++] = ci; continue;}
		    }
    
		    if (cDef < numCols) {
			ci = &pCI[cDef];
			if (ci->exists) glyphs[n++] = ci;
		    }
		}
	    }
	    break;

	case Linear16Bit:
	    if (pFI->allExist && (cDef < numCols)) {
		for (i=0; i < count; i++) {

		    c = *chars++ << 8;
		    c = (c | *chars++) - firstCol;
		    if (c >= numCols) {
			c = cDef;
		    }
		    ci = &pCI[c];
		    glyphs[i] = ci;
		}
		n = count;
	    } else {
		for (i=0; i < count; i++) {
    
		    c = *chars++ << 8;
		    c = (c | *chars++) - firstCol;
		    if (c < numCols) {
			ci = &pCI[c];
			if (ci->exists) {glyphs[n++] = ci; continue;}
		    }
    
		    if (cDef < numCols) {
			ci = &pCI[cDef];
			if (ci->exists) glyphs[n++] = ci;
		    }
		}
	    }
	    break;

	case TwoD16Bit:
	    for (i=0; i < count; i++) {
		register unsigned int row;
		register unsigned int col;

		row = (*chars++) - firstRow;
		col = (*chars++) - firstCol;
		if ((row < numRows) && (col < numCols)) {
		    c = row*numCols + col;
		    ci = &pCI[c];
		    if (ci->exists) {glyphs[n++] = ci; continue;}
		}

		row = (chDefault >> 8)-firstRow;
		col = (chDefault & 0xff)-firstCol;
		if ((row < numRows) && (col < numCols)) {
		    c = row*numCols + col;
		    ci = &pCI[c];
		    if (ci->exists) glyphs[n++] = ci;
		}
	    }
	    break;
    }
    *glyphcount = n;
}


void
QueryGlyphExtents(font, charinfo, count, info)
    FontPtr font;
    CharInfoPtr *charinfo;
    unsigned long count;
    register ExtentInfoRec *info;
{
    register CharInfoPtr *ci = charinfo;
    register unsigned long i;

    info->drawDirection = font->pFI->drawDirection;

    info->fontAscent = font->pFI->fontAscent;
    info->fontDescent = font->pFI->fontDescent;

    if (count != 0) {

	info->overallAscent  = (*ci)->metrics.ascent;
	info->overallDescent = (*ci)->metrics.descent;
	info->overallLeft    = (*ci)->metrics.leftSideBearing;
	info->overallRight   = (*ci)->metrics.rightSideBearing;
	info->overallWidth   = (*ci)->metrics.characterWidth;

	if (font->pFI->constantMetrics && font->pFI->noOverlap) {
	    info->overallWidth *= count;
	    info->overallRight += (info->overallWidth -
				   (*ci)->metrics.characterWidth);
	    return;
	}
	for (i = count, ci++; --i != 0; ci++) {
	    info->overallAscent = max(
	        info->overallAscent,
		(*ci)->metrics.ascent);
	    info->overallDescent = max(
	        info->overallDescent,
		(*ci)->metrics.descent);
	    info->overallLeft = min(
		info->overallLeft,
		info->overallWidth+(*ci)->metrics.leftSideBearing);
	    info->overallRight = max(
		info->overallRight,
		info->overallWidth+(*ci)->metrics.rightSideBearing);
	    /* yes, this order is correct; overallWidth IS incremented last */
	    info->overallWidth += (*ci)->metrics.characterWidth;
	}

    } else {

	info->overallAscent  = 0;
	info->overallDescent = 0;
	info->overallWidth   = 0;
	info->overallLeft    = 0;
	info->overallRight   = 0;

    }
}

Bool
QueryTextExtents(font, count, chars, info)
    FontPtr font;
    unsigned long count;
    unsigned char *chars;
    ExtentInfoRec *info;
{
    CharInfoPtr *charinfo;
    unsigned long n;
    CharInfoPtr	oldCI;
    Bool oldCM;

    charinfo = (CharInfoPtr *)ALLOCATE_LOCAL(count*sizeof(CharInfoPtr));
    if(!charinfo)
	return FALSE;
    oldCI = font->pCI;
    /* kludge, temporarily stuff in Ink metrics */
    font->pCI = font->pInkCI;
    if (font->pFI->lastRow == 0)
	GetGlyphs(font, count, chars, Linear16Bit, &n, charinfo);
    else
	GetGlyphs(font, count, chars, TwoD16Bit, &n, charinfo);
    /* restore real glyph metrics */
    font->pCI = oldCI;
    oldCM = font->pFI->constantMetrics;
    /* kludge, ignore bitmap metric flag */
    font->pFI->constantMetrics = FALSE;
    QueryGlyphExtents(font, charinfo, n, info);
    /* restore bitmap metric flag */
    font->pFI->constantMetrics = oldCM;
    DEALLOCATE_LOCAL(charinfo);
    return TRUE;
}
