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
	 * Format of a CharMetrics block is:
	 * CharMetrics	::=	metrics_format		: CARD32 (0x000000xx)
	 *			num_chars		: CARD32
	 *			metrics			: LISTofCharInfo
	 *
	 * CharInfo	::=	left_side_bearing	: INT16
	 *			right_side_bearing	: INT16
	 *			character_width		: INT16
	 *			ascent			: INT16
	 *			descent			: INT16
	 *			attributes		: CARD16
	 * Size is:	8+(num_chars*sizeof(CharInfo))
	 *		(sizeof(CharInfo)==12 bytes)
	\*/

/***====================================================================***/

#define	VALUE_OK(v)	((((v)+128)<256)&&(((v)+128)>=0))

int
pcfSelectMetricFormat(pFont,format)
EncodedFontPtr	 pFont;
Mask	 	 format;
{
CharSetPtr	 pCS;

    pCS=	pFont->pCS;
    if ((!pCS)||(pCS->nChars<1)||
		(!(pCS->tables&(FONT_METRICS|FONT_INK_METRICS))))  {
	return(format);
    }
    if (!VALUE_OK(pCS->maxbounds.leftSideBearing))	return(format);
    if (!VALUE_OK(pCS->maxbounds.rightSideBearing))	return(format);
    if (!VALUE_OK(pCS->maxbounds.ascent))		return(format);
    if (!VALUE_OK(pCS->maxbounds.descent))		return(format);
    if (!VALUE_OK(pCS->maxbounds.characterWidth))	return(format);
    if (pCS->maxbounds.attributes!=0)			return(format);
    if (!VALUE_OK(pCS->minbounds.leftSideBearing))	return(format);
    if (!VALUE_OK(pCS->minbounds.rightSideBearing))	return(format);
    if (!VALUE_OK(pCS->minbounds.ascent))		return(format);
    if (!VALUE_OK(pCS->minbounds.descent))		return(format);
    if (!VALUE_OK(pCS->minbounds.characterWidth))	return(format);
    if (pCS->minbounds.attributes!=0)			return(format);
    return(PCF_COMPRESSED_METRICS|(format&ORDER_PAD_MASK));
}

/***====================================================================***/

int
pcfSizeMetrics(pFont,format)
EncodedFontPtr	 pFont;
Mask	 	 format;
{
int	n= 0;

    if ((pFont)&&(pFont->pCS)&&
			(pFont->pCS->tables&(FONT_METRICS|FONT_INK_METRICS))) {
	if ((format&FORMAT_MASK)==PCF_COMPRESSED_METRICS) {
	    n=	6+(pFont->pCS->nChars*5);
	    n+=	pcfAlignPad(n);
	}
	else	if ((format&FORMAT_MASK)==PCF_DEFAULT_FORMAT) {
	    n=	8+(pFont->pCS->nChars*sizeof(xCharInfo));
	}
    }
    return(n);
}

/***====================================================================***/

int
pcfWriteMetrics(file,pFont,which,format)
fosFilePtr	 file;
EncodedFontPtr	 pFont;
Mask		 which;
Mask		 format;
{
register	int		 i;
CharSetPtr	 	 pCS= pFont->pCS;
register CharInfoPtr	 pCI;

    if (!(pCS->tables&which)) {
	fosError("pcfWriteMetrics called without metrics loaded\n");
	return(FALSE);
    }

    if ((format&FORMAT_MASK)==PCF_COMPRESSED_METRICS) 
	return(pcfWriteCompressedMetrics(file,pFont,which,format));
    else if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("pcfWriteMetrics: unknown metrics format 0x%x\n",
						format&FORMAT_MASK);
	return(FALSE);
    }


    if		(which==FONT_METRICS)			pCI=	pCS->ci.pCI;
    else if	(which==FONT_INK_METRICS) {
	if ((pCS->inkMetrics)&&(pCS->inkci.pCI!=NULL))	pCI=	pCS->inkci.pCI;
	else						pCI=	pCS->ci.pCI;
    }
    else {
	fosInternalError("pcfWriteMetrics: illegal set of metrics (0x%x)\n",
									which);
	return(FALSE);
    }

    fosWriteLSB32(file,format);

    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));

    pcfWriteInt32(file,pCS->nChars);
    for (i=0;i<pCS->nChars;i++,pCI++) {
	pcfWriteInt16(file,pCI->metrics.leftSideBearing);
	pcfWriteInt16(file,pCI->metrics.rightSideBearing);
	pcfWriteInt16(file,pCI->metrics.characterWidth);
	pcfWriteInt16(file,pCI->metrics.ascent);
	pcfWriteInt16(file,pCI->metrics.descent);
	pcfWriteInt16(file,pCI->metrics.attributes);
    }
    return(TRUE);
}


/***====================================================================***/

	/*\
	 * Format of a Compressed CharMetrics block is:
	 * CharMetrics	::=	metrics_format		: CARD32 (0x000001xx)
	 *			num_chars		: CARD16
	 *			metrics			: LISTofSmallCharInfo
	 *
	 * SmallCharInfo ::=	left_side_bearing	: INT8
	 *			right_side_bearing	: INT8
	 *			character_width		: INT8
	 *			ascent			: INT8
	 *			descent			: INT8
	 *			pad			: pad to word boundary
	 * Size is:	num_chars*sizeof(SmallCharInfo)
	 *		(sizeof(CharInfo)==5 bytes)
	\*/

/***====================================================================***/

int
pcfWriteCompressedMetrics(file,pFont,which,format)
fosFilePtr	 file;
EncodedFontPtr	 pFont;
Mask		 which;
Mask		 format;
{
register	int		 i;
CharSetPtr	 	 pCS= pFont->pCS;
register CharInfoPtr	 pCI;

    if		(which==FONT_METRICS)		pCI= pCS->ci.pCI;
    else if	(which==FONT_INK_METRICS)	pCI= pCS->inkci.pCI;
    else {
	fosInternalError("pcfWriteMetrics: illegal set of metrics (0x%x)\n",
									which);
	return(FALSE);
    }

    fosWriteLSB32(file,format);
    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));
    pcfWriteInt16(file,pCS->nChars);
    for (i=0;i<pCS->nChars;i++,pCI++) {
	fosWriteInt8(file,(pCI->metrics.leftSideBearing+128)&0xff);
	fosWriteInt8(file,(pCI->metrics.rightSideBearing+128)&0xff);
	fosWriteInt8(file,(pCI->metrics.characterWidth+128)&0xff);
	fosWriteInt8(file,(pCI->metrics.ascent+128)&0xff);
	fosWriteInt8(file,(pCI->metrics.descent+128)&0xff);
    }
    PCF_ADD_PADDING(file,i);
    return(TRUE);
}
