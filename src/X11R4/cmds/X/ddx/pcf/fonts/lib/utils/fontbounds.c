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

/* $Header: fontbounds.c,v 5.4 89/09/11 15:11:50 erik Exp $ */

#include <X11/X.h>
#include "fontstr.h"
#include "fontlib.h"

static xCharInfo initMinMetrics = {
	MAXSHORT, MAXSHORT, MAXSHORT, MAXSHORT, MAXSHORT, 0xFFFF};
static xCharInfo initMaxMetrics = {
	MINSHORT, MINSHORT, MINSHORT, MINSHORT, MINSHORT, 0x0000};

#define MINMAX(field) \
	if (minbounds->field > ci->metrics.field) \
	     minbounds->field = ci->metrics.field; \
	if (maxbounds->field < ci->metrics.field) \
	     maxbounds->field = ci->metrics.field;

/*
 * Computes min and max bounds for a set of glyphs,
 * maxoverlap (right bearing extent beyond character width,
 * and number of nonexistant characters (all zero metrics).
 */
void
ComputeFontBounds(pCS)
CharSetPtr	pCS;
{
register xCharInfo	*minbounds = &pCS->minbounds;
register xCharInfo	*maxbounds = &pCS->maxbounds;
CharInfoPtr		pCI;
int			nchars;
register CharInfoPtr 	ci;
register int 		i;

    if (!(pCS->tables&FONT_METRICS)) {
#ifndef notdef
	fosWarning("ComputeFontBounds called with no metrics\n");
#endif
	return;
    }

    nchars=	pCS->nChars;
    pCI=	pCS->ci.pCI;
    pCS->maxOverlap = MINSHORT;

    *minbounds = initMinMetrics;
    *maxbounds = initMaxMetrics;

    ci = pCI;
    for (i = 0; i < nchars; i++, ci++) {
	MINMAX(ascent);
	MINMAX(descent);
	MINMAX(leftSideBearing);
	MINMAX(rightSideBearing);
	MINMAX(characterWidth);
	minbounds->attributes &= ci->metrics.attributes;
	maxbounds->attributes |= ci->metrics.attributes;
	pCS->maxOverlap = MAX(
	    pCS->maxOverlap,
	    ci->metrics.rightSideBearing - ci->metrics.characterWidth);
    }
}

void
ComputeFontAccelerators(font)
    register EncodedFontPtr font;
{
    register xCharInfo *minbounds = &font->inkMin;
    register xCharInfo *maxbounds = &font->inkMax;
    register CharInfoPtr ci;
    register int r, c;
    register int cols = font->lastRow - font->firstRow + 1;
    
    if ((font->ppCI==NULL)||(font->pCS==NULL)) {
#ifndef NOTDEF
	fosWarning("ComputeFontAccelerators called with no metrics\n");
#endif
	return;
    }

    *minbounds = initMinMetrics;
    *maxbounds = initMaxMetrics;

    font->linear = (font->firstRow == font->lastRow);
    font->allExist = TRUE;
    for (r = 0; r < font->lastRow - font->firstRow + 1; r++)
	for (c = 0; c < cols; c++) {
	    if ((ci = font->ppCI[r*cols + c]) == NullCharInfo)
		font->allExist = FALSE;
	    else {
		MINMAX(ascent);
		MINMAX(descent);
		MINMAX(leftSideBearing);
		MINMAX(rightSideBearing);
		MINMAX(characterWidth);
		minbounds->attributes &= ci->metrics.attributes;
		maxbounds->attributes |= ci->metrics.attributes;
	    }
	}
    r = font->defaultCh>> 8;
    c = font->defaultCh&0xFF;
    if ((font->defaultCh!= NO_SUCH_CHAR) &&
	(r >= font->firstRow) && (r <= font->lastRow) &&
	(c >= font->firstCol) && (c <= font->lastCol))
	    font->pDefChar=	font->ppCI[r*cols + c];
    else
	    font->pDefChar=	NullCharInfo;
}

