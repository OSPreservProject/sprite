#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"
#include "fosfilestr.h"

#define PCF_NEED_WRITERS

#include "pcf.h"
#include "pcfint.h"

/***====================================================================***/

	/*\
	 * Format of a Block of bitmaps is:
	 * Bitmaps	::=	bitmaps_format	:	CARD32
	 *			num_chars	:	CARD32
	 *			offsets		:	LISTofCARD32
	 *			bitmaps_sizes	:	CARD32[GLYPHPADOPTIONS]
	 *			pictures	:	LISTofGlyph
	 *			pad		:	pad to word boundary
	 *
	 * Size is:	8+(4*num_chars)+(4*GLYPHPADOOPTIONS)+
	 *				bitmapsSizes[INDEX(format)]
	 *		padded to multiple of 4 bytes
	\*/

int
pcfSizeBitmaps(pFont,format)
EncodedFontPtr 	 pFont;
unsigned long	 format;
{
int	n= 0;

    if (pFont&&(pFont->pCS)&&(pFont->pCS->tables&FONT_BITMAPS)&&
				((format&FORMAT_MASK)==PCF_DEFAULT_FORMAT)) {
	n= 8+(4*pFont->pCS->nChars)+(4*GLYPHPADOPTIONS)+
		pFont->pCS->bitmapsSizes[FMT_GLYPH_PAD_INDEX(format)];

	n+=	pcfAlignPad(n);
    }
    return(n);
}

/***====================================================================***/

int
pcfWriteBitmaps(file,pFont,format)
register fosFilePtr	 	 file;
	 EncodedFontPtr	 	 pFont;
	 unsigned long	 	 format;
{
register CharSetPtr	 pCS= pFont->pCS;
register int	i;
register int	off;
	 int	n,glyphPad;

    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("pcfWriteBitmaps, Unknown bitmap format 0x%x\n",
						format&FORMAT_MASK);
	return(FALSE);
    }
    if (!(pCS->tables&FONT_BITMAPS)) {
	fosError("pcfWriteBitmaps called without loaded bitmaps\n");
	return(FALSE);
    }
    fosWriteLSB32(file,format);

    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));
    pcfWriteInt32(file,pCS->nChars);

    glyphPad= FMT_GLYPH_PAD(format);
    off= 0;
    for (i=0;i<pCS->nChars;i++) {
	if (pCS->pBitOffsets[i]) {
	    pcfWriteInt32(file,off);
	    off+= BYTES_FOR_GLYPH(&pCS->ci.pCI[i],glyphPad);
	}
	else {
	    if (BYTES_FOR_GLYPH(&pCS->ci.pCI[i],glyphPad)!=0) {
		fosInternalError("pcfWriteBitmaps, chars without bitmaps not implemented\n");
	    }
	    pcfWriteInt32(file,off);
	}
    }

    for (i=0;i<GLYPHPADOPTIONS;i++) {
	pcfWriteInt32(file,pCS->bitmapsSizes[i]);
    }

    off= 0;
    for (i=0;i<pCS->nChars;i++) {
	if (pCS->pBitOffsets[i]) {
	    n= BYTES_FOR_GLYPH(&pCS->ci.pCI[i],glyphPad);
	    if (fosWriteBlock(file,pCS->pBitOffsets[i],n)!=n) {
		fosError("pcfWriteBitmaps: error writing glyph %d\n",i);
	    }
	    off+= n;
	}
    }

    if (off!=pCS->bitmapsSizes[FMT_GLYPH_PAD_INDEX(format)]) {
	fosError("pcfWriteBitmaps: internal error! wrote %d, expected %d\n",
			off,pCS->bitmapsSizes[FMT_GLYPH_PAD_INDEX(format)]);
    }
    PCF_ADD_PADDING(file,i);
    return(TRUE);
}
