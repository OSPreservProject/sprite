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

/***====================================================================***/

int
pcfReadSWidths(file,pCS)
fosFilePtr		file;
register CharSetPtr	pCS;
{
register int i;
int	nRead= 0;
Mask	format;

    if (pCS->tables&FONT_SWIDTHS) {
	fosError("pcfReadSWidths called with s-widths loaded\n");
	return(FALSE);
    }
    format=	fosReadLSB32(file);
    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("Unknown scalable width format 0x%x\n",format&FORMAT_MASK);
	return(FALSE);
    }
    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));
    i=	pcfReadInt32(file);

    if (pCS->nChars==0)
	pCS->nChars= i;
    else if (i!=pCS->nChars) {
	fosError("bad count in pcfReadSWidths (expected %d, got %d)\n",
								pCS->nChars,i);
	return(FALSE);
    }

    pCS->sWidth= (int *)fosCalloc(pCS->nChars,sizeof(int));
    if (pCS->sWidth==NULL) {
	fosError("allocation failure in pcfReadSWidths (%d)\n",
						pCS->nChars*sizeof(int));
	return(FALSE);
    }
    for (i=0;i<pCS->nChars;i++) {
	pCS->sWidth[i]= pcfReadInt32(file);
    }
    pCS->tables|= FONT_SWIDTHS;
    return(TRUE);
}
