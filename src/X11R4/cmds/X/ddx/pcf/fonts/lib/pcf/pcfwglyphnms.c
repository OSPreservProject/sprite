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
	 * Format of a block of Glyph Names is:
	 * GlyphNames	::=	names_format	: CARD32
	 *			num_chars	: CARD32
	 *			offsets		: LISTofCARD32
	 *			size_names	: CARD32
	 *			names		: LISTofSTRING
	 *			pad		: pad to word boundary
	 * Size is:	8+(num_chars*4)+4+size_names
	 *		padded to multiple of 4 bytes
	\*/

int
pcfSizeGlyphNames(pFont,format)
EncodedFontPtr	pFont;
Mask		format;
{
register CharSetPtr	pCS= pFont->pCS;
int	i,glyphNamesSize,n= 0;

 
    if (pFont&&pCS&&(pCS->tables&FONT_GLYPH_NAMES)&&
				((format&FORMAT_MASK)==PCF_DEFAULT_FORMAT)){
	glyphNamesSize= 0;
	for (i=0;i<pCS->nChars;i++) {
	    glyphNamesSize+= strlen(NameForAtomOrNone(pCS->glyphNames[i]))+1;
	}
	n=	8+(4*pCS->nChars)+4+glyphNamesSize;
	n+=	pcfAlignPad(n);
     }
     return(n);
}

/***====================================================================***/

int
pcfWriteGlyphNames(file,pFont,format)
fosFilePtr	file;
EncodedFontPtr	pFont;
Mask		format;
{
register CharSetPtr	 pCS= pFont->pCS;
register int		 i,off;
char	*name;

    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("pcfWriteGlyphNames: unknown format 0x%x\n",
					format&FORMAT_MASK);
	return(FALSE);
    }
    if (!pCS->tables&FONT_GLYPH_NAMES) {
	fosError("pcfWriteGlyphNames called with no names loaded\n");
	return(FALSE);
    }
    fosWriteLSB32(file,format);

    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));
    pcfWriteInt32(file,pCS->nChars);

    off= 0;
    for (i=0;i<pCS->nChars;i++) {
	pcfWriteInt32(file,off);
	off+= strlen(NameForAtomOrNone(pCS->glyphNames[i]))+1;
    }
    pcfWriteInt32(file,off);

    off= 0;
    for (i=0;i<pCS->nChars;i++) {
	name= (char *)NameForAtomOrNone(pCS->glyphNames[i]);
	off+= fosWriteBlock(file,name,strlen(name)+1);
    }
    PCF_ADD_PADDING(file,i);
    return(TRUE);
}
