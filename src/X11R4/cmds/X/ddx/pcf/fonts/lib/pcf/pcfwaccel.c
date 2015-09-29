#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "fosfilestr.h"

#include "pcf.h"
#include "pcfint.h"

/***====================================================================***/

	/*\
	 * Format of an accelerator block is:
	 * Accelerators	::=	accelerator_format	: CARD32
	 *			no_overlap		: CARD8
	 *			constant_metrics	: CARD8
	 *			terminal_font		: CARD8
	 *			constant_width		: CARD8
	 *			ink_inside		: CARD8
	 *			has_ink_metrics		: CARD8
	 *			draw_direction		: CARD8
	 *			pad			: CARD8
	 *			font_ascent		: INT32
	 *			font_descent		: INT32
	 *			max_overlap		: INT32
	 *			min_bounds		: CharInfo
	 *			max_bounds		: CharInfo
	 *			pad			: Array[52]ofCARD8
	 * Size is:	100 bytes
	\*/

int
pcfSizeAccelerators(pFont,format)
EncodedFontPtr	pFont;
Mask		format;
{
/* 5/5/89 (ef) -- Magic numbers.  Blech */

    if (pFont&&(pFont->pCS)&&(pFont->pCS->tables&FONT_ACCELERATORS)&&
				((format&FORMAT_MASK)==PCF_DEFAULT_FORMAT))
		return(100);
    else	return(0);
}

/***====================================================================***/

int
pcfWriteAccelerators(file,pFont,format)
fosFilePtr	file;
EncodedFontPtr	pFont;
Mask		format;
{
register CharSetPtr	pCS= pFont->pCS;
int	i;

    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("pcfWriteAccelerators: Unknown accelerator format 0x%x\n",
							format&FORMAT_MASK);
	return(FALSE);
    }
    fosWriteLSB32(file,format);

    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));
    if (!(pCS->tables&FONT_ACCELERATORS)) {
	fosError("pcfWriteAccelerators called without loaded accelerators\n");
	return(FALSE);
    }
    fosWriteInt8(file,pCS->noOverlap);
    fosWriteInt8(file,pCS->constantMetrics);
    fosWriteInt8(file,pCS->terminalFont);
    fosWriteInt8(file,pCS->constantWidth);
    fosWriteInt8(file,pCS->inkInside);
    fosWriteInt8(file,pCS->inkMetrics);
    fosWriteInt8(file,pCS->drawDirection);
    fosWriteInt8(file,0);		/* pad byte for natural alignment */
    pcfWriteInt32(file,pCS->fontAscent);
    pcfWriteInt32(file,pCS->fontDescent);
    pcfWriteInt32(file,pCS->maxOverlap);
    pcfWriteInt16(file,pCS->minbounds.leftSideBearing);
    pcfWriteInt16(file,pCS->minbounds.rightSideBearing);
    pcfWriteInt16(file,pCS->minbounds.characterWidth);
    pcfWriteInt16(file,pCS->minbounds.ascent);
    pcfWriteInt16(file,pCS->minbounds.descent);
    pcfWriteInt16(file,pCS->minbounds.attributes);
    pcfWriteInt16(file,pCS->maxbounds.leftSideBearing);
    pcfWriteInt16(file,pCS->maxbounds.rightSideBearing);
    pcfWriteInt16(file,pCS->maxbounds.characterWidth);
    pcfWriteInt16(file,pCS->maxbounds.ascent);
    pcfWriteInt16(file,pCS->maxbounds.descent);
    pcfWriteInt16(file,pCS->maxbounds.attributes);
    /* 5/19/89 (ef) -- magic numbers.  blech */
    fosPad(file,52);
    return(TRUE);
}
