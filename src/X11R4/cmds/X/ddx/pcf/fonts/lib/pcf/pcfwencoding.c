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
	 * Format of an encoding block:
	 * Encoding	::=	encoding_format	:	CARD32
	 *			first_col	:	CARD16
	 *			last_col	:	CARD16
	 *			first_row	:	CARD16
	 *			last_row	:	CARD16
	 *			default_char	:	CARD16
	 *			indices		:	LISTofCARD16
	 *			pad		:	pad to word boundary
	 * Size is:	14+(((last_col-first_col+1)*(last_row-first_row+1))*2)
	 *		padded to multiple of 4 bytes
	\*/


int
pcfSizeEncoding(pFont,format)
EncodedFontPtr	pFont;
Mask		format;
{
int	n= 0;

    if (pFont&&(pFont->pCS)&&((pFont->ppCI)||(pFont->ppInkCI))&&
		((format&FORMAT_MASK)==PCF_DEFAULT_FORMAT)) {
	n=	14+(n2dChars(pFont)*2);
	n+=	pcfAlignPad(n);
    }
    return(n);
}

/***====================================================================***/

int
pcfWriteEncoding(file,pFont,format)
fosFilePtr	file;
EncodedFontPtr	pFont;
Mask		format;
{
int	nChars,i;
CharInfoPtr	pStart,pThis,*ppCI;

    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("fosWriteEncoding: Unknown encoding format 0x%x\n",
							format&FORMAT_MASK);
	return(FALSE);
    }

    fosWriteLSB32(file,format);
    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));

    if ((pFont->pCS->ci.pCI)&&(pFont->ppCI))  {
	pStart=	pFont->pCS->ci.pCI;
	ppCI=	pFont->ppCI;
    }
    else  if ((pFont->pCS->inkci.pCI)&&(pFont->ppInkCI)) {
	pStart= pFont->pCS->inkci.pCI;
	ppCI=	pFont->ppInkCI;
    }
    else {
	fosError("fosWriteEncoding called with invalid encoding\n");
	return(FALSE);
    }

    pcfWriteInt16(file,pFont->firstCol);
    pcfWriteInt16(file,pFont->lastCol);
    pcfWriteInt16(file,pFont->firstRow);
    pcfWriteInt16(file,pFont->lastRow);
    pcfWriteInt16(file,pFont->defaultCh);

    nChars= n2dChars(pFont);
    for (i=0;i<nChars;i++) {
	pThis= pFont->ppCI[i];
	if (pThis==NullCharInfo)	pcfWriteInt16(file,NO_SUCH_CHAR);
	else				pcfWriteInt16(file,pThis-pStart);
    }
    PCF_ADD_PADDING(file,i);
    return(TRUE);
}
