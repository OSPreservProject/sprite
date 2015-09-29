#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"
#include "fosfilestr.h"

#include "pcf.h"
#include "pcfint.h"

/***====================================================================***/

	/*\
	 * Format of Scalable Widths block:
	 * ScalableWidths	::=	swidths_format	:	CARD32
	 *				num_chars	:	CARD32
	 *				widths		:	LISTofCARD32
	 * Size is:	8+(4*num_chars)
	\*/

int
pcfSizeSWidths(pFont,format)
EncodedFontPtr	 pFont;
Mask		 format;
{
    if (pFont&&(pFont->pCS)&&(pFont->pCS->tables&FONT_SWIDTHS)&&
				((format&FORMAT_MASK)==PCF_DEFAULT_FORMAT))
		return(8+(4*pFont->pCS->nChars));
    else	return(0);
}

/***====================================================================***/

int
pcfWriteSWidths(file,pFont,format)
fosFilePtr	  	 file;
register EncodedFontPtr	 pFont;
Mask			 format;
{
register CharSetPtr	 pCS= pFont->pCS;
register int 		 i;

    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("pcfWriteSWidths: unknown format 0x%x\n",format&FORMAT_MASK);
	return(FALSE);
    }
    if (!(pCS->tables&FONT_SWIDTHS)) {
	fosError("pcfWriteSWidths called without widths loaded\n");
	return(FALSE);
    }
    fosWriteLSB32(file,format);
    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));

    pcfWriteInt32(file,pCS->nChars);
    for (i=0;i<pCS->nChars;i++) {
	pcfWriteInt32(file,pCS->sWidth[i]);
    }
    return(TRUE);
}
