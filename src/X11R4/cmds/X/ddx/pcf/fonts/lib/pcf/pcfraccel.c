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
	 * Accelerators	::=	accel_format		: CARD32	(0)
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

/***====================================================================***/

int
pcfReadAccelerators(file,pCS)
fosFilePtr	file;
CharSetPtr	pCS;
{
Mask		format;

    if (pCS->tables&FONT_ACCELERATORS) {
	fosInternalError("pcfReadAccelerators called with accelerators loaded\n");
	return(FALSE);
    }
    format=				fosReadLSB32(file);
    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("Unknown accelerator format 0x%x\n",(format&FORMAT_MASK));
	return(FALSE);
    }
    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));
    pCS->noOverlap=			fosReadInt8(file);
    pCS->constantMetrics=		fosReadInt8(file);
    pCS->terminalFont=			fosReadInt8(file);
    pCS->constantWidth=			fosReadInt8(file);
    pCS->inkInside=			fosReadInt8(file);
    pCS->inkMetrics=			fosReadInt8(file);
    pCS->drawDirection=			fosReadInt8(file);
    /* natural alignment */		fosReadInt8(file);
    pCS->fontAscent=			pcfReadInt32(file);
    pCS->fontDescent=			pcfReadInt32(file);
    pCS->maxOverlap=			pcfReadInt32(file);
    pCS->minbounds.leftSideBearing=	pcfReadInt16(file);
    pCS->minbounds.rightSideBearing=	pcfReadInt16(file);
    pCS->minbounds.characterWidth=	pcfReadInt16(file);
    pCS->minbounds.ascent=		pcfReadInt16(file);
    pCS->minbounds.descent=		pcfReadInt16(file);
    pCS->minbounds.attributes=		pcfReadInt16(file);
    pCS->maxbounds.leftSideBearing=	pcfReadInt16(file);
    pCS->maxbounds.rightSideBearing=	pcfReadInt16(file);
    pCS->maxbounds.characterWidth=	pcfReadInt16(file);
    pCS->maxbounds.ascent=		pcfReadInt16(file);
    pCS->maxbounds.descent= 		pcfReadInt16(file);
    pCS->maxbounds.attributes=		pcfReadInt16(file);
    /* pad */
    fosSkip(file,52);
    /* 6/23/89 (ef) -- magic numbers.  blech */
    pCS->tables|= FONT_ACCELERATORS;
    return(TRUE);
}
