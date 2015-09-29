#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"
#include "fosfilestr.h"

#define PCF_NEED_READERS

#include "pcf.h"
#include "pcfint.h"

/***====================================================================***/

	/*\
	 * Format of a CharMetrics block is:
	 * CharMetrics	::=	metrics_format		: CARD32	(0)
	 *			num_chars		: CARD32
	 *			metrics			: LISTofCharInfo
	 *
	 * CharInfo	::=	left_side_bearing	: INT16
	 *			right_side_bearing	: INT16
	 *			character_width		: INT16
	 *			ascent			: INT16
	 *			descent			: INT16
	 *			attributes		: CARD16
	 * Size is:	8+num_chars*sizeof(CharInfo)
	 *		(sizeof(CharInfo)==12 bytes)
	\*/

/***====================================================================***/

int
pcfReadMetrics(file,pCS,which,ppCI)
fosFilePtr	 file;
CharSetPtr	 pCS;
unsigned	 which;
CharInfoPtr	*ppCI;
{
register	int		 i;
register	CharInfoPtr	 pCI= *ppCI;
Mask		format;

    if ((pCS->tables&which)||(pCI!=NULL)) {
	fosError("pcfReadMetrics called with metrics loaded\n");
	return(FALSE);
    }

    format=	fosReadLSB32(file);
    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));

    if ((format&FORMAT_MASK)==PCF_COMPRESSED_METRICS) {
	return(pcfReadCompressedMetrics(file,pCS,which,ppCI,format));
    }
    else if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("Unknown metric format 0x%x\n",format);
	return(FALSE);
    }

    i= pcfReadInt32(file);
    if		(pCS->nChars==0)	pCS->nChars= i;
    else if	(pCS->nChars!=i) {
	fosError("bad char count (%d, expected %d) in pcfReadMetrics (0x%x)\n",
						i,pCS->nChars,which);
	return(FALSE);
    }

    *ppCI= pCI=	(CharInfoPtr)fosCalloc(pCS->nChars,sizeof(CharInfoRec));
    if (pCI==NULL) {
	fosError("allocation failure in pcfReadMetrics (pCI,%d,0x%x)\n",
				pCS->nChars*sizeof(CharInfoRec),which);
	return(FALSE);
    }

    for (i=0;i<pCS->nChars;i++,pCI++) {
	pCI->metrics.leftSideBearing=	pcfReadInt16(file);
	pCI->metrics.rightSideBearing=	pcfReadInt16(file);
	pCI->metrics.characterWidth=	pcfReadInt16(file);
	pCI->metrics.ascent=		pcfReadInt16(file);
	pCI->metrics.descent=		pcfReadInt16(file);
	pCI->metrics.attributes=	pcfReadInt16(file);
	pCI->pPriv=			NULL;
    }
    pCS->tables|= which;
    return(TRUE);
}

/***====================================================================***/


	/*\
	 * Format of a Compressed CharMetrics block is:
	 * CharMetrics	::=	metrics_format		: CARD32 (0x000001xx)
	 *			num_chars		: CARD16
	 *			metrics			: LISTofSmallCharInfo
	 *			pad			: pad to word boundary
	 *
	 * SmallCharInfo ::=	left_side_bearing	: INT8
	 *			right_side_bearing	: INT8
	 *			character_width		: INT8
	 *			ascent			: INT8
	 *			descent			: INT8
	 * Size is:	num_chars*sizeof(SmallCharInfo)
	 *		(sizeof(CharInfo)==5 bytes)
	\*/

/* 6/23/89 (ef) -- what about systems without signed characters? */

	/*\
	 * This is a little tricky.   pcfReadMetrics reads the format
	 * word and dispatches as appropriate, so the first word in the
	 * format described above has already been read
	\*/

int
pcfReadCompressedMetrics(file,pCS,which,ppCI)
fosFilePtr	 file;
CharSetPtr	 	 pCS;
unsigned		 which;
CharInfoPtr		*ppCI;
{
register	int		 i;
register	CharInfoPtr	 pCI= *ppCI;

    i=	pcfReadInt16(file);
    if		(pCS->nChars==0)	pCS->nChars= i;
    else if	(pCS->nChars!=i) {
	fosError("bad char count (%d, expected %d) in pcfReadMetrics (0x%x)\n",
						i,pCS->nChars,which);
	return(FALSE);
    }

    *ppCI= pCI=	(CharInfoPtr)fosCalloc(pCS->nChars,sizeof(CharInfoRec));
    if (pCI==NULL) {
	fosError("allocation failure in pcfReadMetrics (pCI,%d,0x%x)\n",
				pCS->nChars*sizeof(CharInfoRec),which);
	return(FALSE);
    }

    for (i=0;i<pCS->nChars;i++,pCI++) {
	pCI->metrics.leftSideBearing=	(((CARD32)fosReadInt8(file))&0xff)-128;
	pCI->metrics.rightSideBearing=	(((CARD32)fosReadInt8(file))&0xff)-128;
	pCI->metrics.characterWidth=	(((CARD32)fosReadInt8(file))&0xff)-128;
	pCI->metrics.ascent=		(((CARD32)fosReadInt8(file))&0xff)-128;
	pCI->metrics.descent=		(((CARD32)fosReadInt8(file))&0xff)-128;
	pCI->metrics.attributes=	0;
	pCI->pPriv=			NULL;
    }
    pCS->tables|= which;
    PCF_SKIP_PADDING(file,i);
    return(TRUE);
}
