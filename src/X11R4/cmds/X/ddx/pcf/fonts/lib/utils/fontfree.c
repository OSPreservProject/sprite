
#include <stdio.h>
#include <X11/X.h>

#include "fontstr.h"
#include "fontlib.h"

/***====================================================================***/

void
fontFree(pFont)
EncodedFontPtr	pFont;
{
    if (pFont) {
	if (pFont->pCS) {
	    pFont->pCS->refcnt--;
	    if (pFont->pCS->refcnt==0) {
		fontUnload(pFont,FONT_EVERYTHING);
		fosFree(pFont->pCS);
		pFont->pCS=	NULL;
	    }
	}
	fosFree(pFont);
    }
    return;
}

/***====================================================================***/

void
fontUnload(pFont,tables)
EncodedFontPtr	pFont;
Mask	tables;
{
CharSetPtr	pCS= pFont->pCS;

    tables&= pCS->tables;
    if (tables&FONT_BITMAPS) {
	if (pCS->pBitmaps) {
	    fosFree(pCS->pBitmaps);
	    if (pCS->pBitOffsets) {
		fosFree(pCS->pBitOffsets);
	    }
	}
	else if (pCS->pBitOffsets) {
	    int i;
	    for (i=0;i<pCS->nChars;i++) {
		if (pCS->pBitOffsets[i]!=NULL) {
		    fosFree(pCS->pBitOffsets[i]);
		    pCS->pBitOffsets[i]= NULL;
		}
	    }
	    fosFree(pCS->pBitOffsets);
	}
	pCS->pBitmaps=		NULL;
	pCS->pBitOffsets=	NULL;
	pCS->tables&=		(~FONT_BITMAPS);
    }

    if ((tables&FONT_SWIDTHS)&&(pCS->sWidth)) {
	fosFree(pCS->sWidth);
	pCS->sWidth= 	NULL;
	pCS->tables&=	(~FONT_SWIDTHS);
    }

    if (tables&FONT_INK_METRICS) {
	if ((pFont->ppCI!=pFont->ppInkCI)&&(pFont->ppInkCI)) {
	    fosFree(pFont->ppInkCI);
	}
	pFont->ppInkCI=	NULL;

	if (pCS->inkci.pCI) {
	    fosFree(pCS->inkci.pCI);
	}
	pCS->inkci.pCI= NULL;

	pCS->tables&= (~FONT_INK_METRICS);
    }

    if (tables&FONT_PROPERTIES) {
	if (pCS->props)
	    fosFree(pCS->props);
	pCS->props= NULL;
	if (pCS->isStringProp)
	    fosFree(pCS->isStringProp);
	pCS->isStringProp=	NULL;
	pCS->nProps=		0;
	pCS->tables&=		(~FONT_PROPERTIES);
    }

    if (tables&FONT_METRICS) {
	if (pCS->ci.pCI)
	    fosFree(pCS->ci.pCI);
	pCS->ci.pCI= NULL;
	pCS->tables&= (~FONT_METRICS);

	if (pFont->ppInkCI==pFont->ppCI)
	    pFont->ppInkCI= NULL;

	if (pFont->ppCI) {
	    fosFree(pFont->ppCI);
	    pFont->ppCI= NULL;
	}
    }

    if (tables&FONT_GLYPH_NAMES) {
	if (pCS->glyphNames)
	    fosFree(pCS->glyphNames);
	pCS->glyphNames=	NULL;
	pCS->tables&=		(~FONT_GLYPH_NAMES);
    }

    if (!(tables&(FONT_ALL_METRICS|FONT_SWIDTHS|FONT_GLYPH_NAMES|FONT_BITMAPS)))
	pCS->nChars=	0;
    return;
}
