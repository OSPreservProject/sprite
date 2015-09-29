#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"
#include "fosfilestr.h"

#define	PCF_NEED_READERS

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
	 *			default_ch	:	CARD16
	 *			indices		:	LISTofCARD16
	 *			pad		:	(pad to 4 bytes)
	 * Size is:	14+(((last_col-first_col+1)*(last_row-first_row+1))*2)
	 *		padded to 4 bytes.
	\*/


/***====================================================================***/

int
pcfReadEncoding(file,pFont)
fosFilePtr 	file;
EncodedFontPtr	pFont;
{
int	nChars,i,tmp;
int	doInks= FALSE;
Mask	format;

    format=	fosReadLSB32(file);
    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("unknown bdf encoding format (0x%x)\n",format&FORMAT_MASK);
	return(FALSE);
    }

    if (pFont->pCS==NULL)
	return(FALSE);

    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));
    pFont->firstCol=	pcfReadInt16(file);
    pFont->lastCol=	pcfReadInt16(file);
    pFont->firstRow=	pcfReadInt16(file);
    pFont->lastRow=	pcfReadInt16(file);
    pFont->defaultCh=	pcfReadInt16(file);

    nChars= n2dChars(pFont);

    if (pFont->pCS->ci.pCI!=NULL) {
	pFont->ppCI= (CharInfoPtr *)fosCalloc(nChars,sizeof(CharInfoPtr));
	if (!pFont->ppCI) {
	    fosError("allocation failure in pcfReadEncoding (ppCI)\n");
	    return(FALSE);
	}
    }

    if (pFont->pCS->inkMetrics&&(pFont->pCS->inkci.pCI)) {
	pFont->ppInkCI= (CharInfoPtr *)fosCalloc(nChars,sizeof(CharInfoPtr));
	if (!pFont->ppInkCI) {
	    fosError("allocation failure in pcfReadEncoding (ppInkCI)\n");
	    goto BAILOUT;
	}
	doInks= TRUE;
    }
    else {
	pFont->ppInkCI= pFont->ppCI;
    }

    pFont->inkMin= pFont->pCS->minbounds;
    pFont->inkMax= pFont->pCS->maxbounds;

    /* bail if no metrics */
    if ((!pFont->ppCI)&&(!pFont->ppInkCI)) {
	fosSkip(file,nChars*2);
	return(TRUE);
    }

    for (i=0;i<nChars;i++) {
	tmp=	pcfReadInt16(file);

	if (tmp==NO_SUCH_CHAR) {
	    pFont->ppCI[i]= 		  NullCharInfo;
	    if (doInks)
		pFont->ppInkCI[i]= NullCharInfo;
	}
	else if (tmp<pFont->pCS->nChars) {
	    pFont->ppCI[i]= &pFont->pCS->ci.pCI[tmp];
	    if (doInks) 
		pFont->ppInkCI[i]= &pFont->pCS->inkci.pCI[tmp];
	}
	else {
	    fosError("Illegal char index in pcfReadEncoding (%d>%d)\n",
							tmp,nChars);
	    goto BAILOUT;
	}
    }

    if (pFont->defaultCh!=NO_SUCH_CHAR) {
	/* 5/23/89 (ef) -- XXX! this is wrong!! get correct index */
	pFont->pDefChar= pFont->ppCI[pFont->defaultCh];
    }

    PCF_SKIP_PADDING(file,i);
    return(TRUE);
BAILOUT:
    if ((doInks)&&(pFont->ppInkCI)) {
	fosFree(pFont->ppInkCI);
	pFont->ppInkCI= NULL;
    }
    if (pFont->ppCI) {
	fosFree(pFont->ppCI);
	pFont->ppCI= NULL;
    }
    return(FALSE);
}
