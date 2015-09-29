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
	 * Format of a Block of bitmaps is:
	 * Bitmaps	::=	bitmaps_format	:	CARD32
	 *			num_chars	:	CARD32
	 *			offsets		:	LISTofCARD32
	 *			bitmaps_sizes	:	CARD32[GLYPHPADOPTIONS]
	 *			pictures	:	LISTofGlyph
	 *			pad		:	pad to 4 bytes
	 *
	 * Size is:	8+(4*num_chars)+(4*GLYPHPADOPTIONS)+
	 *			bitmaps_sizes[GLYPH_PAD(pad_order_info)]
	 *		padded to even multiple of 4 bytes
	\*/

/***====================================================================***/

int
pcfReadBitmaps(file,pCS,outPadOrder)
register fosFilePtr	 file;
register CharSetPtr	 pCS;
	 Mask		 outPadOrder;
{
unsigned long	fileFormat;
register int	i;
	 int	n;
 	 CARD32	*offsets;
unsigned char	*tmpGlyphs;
	 int	glyphSize;

    ASSERT( "pcfReadBitmaps", sizeof(CARD32) == sizeof(char *) );

    if (pCS->tables&FONT_BITMAPS) {
	fosInternalError("pcfReadBitmaps called with bitmaps loaded\n");
	return(FALSE);
    }

    fileFormat=			fosReadLSB32(file);
    if ((fileFormat&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("unknown bitmap format 0x%x\n",fileFormat&FORMAT_MASK);
	return(FALSE);
    }
    pcfSetByteOrder(file,FMT_BYTE_ORDER(fileFormat));

    /* 7/7/89 (ef) -- more magic numbers.  are these correct? */
    pCS->pixDepth= 		1;
    pCS->glyphSets=		1;
    n= 				pcfReadInt32(file);

    if (pCS->nChars==0)	
	pCS->nChars= n;
    else if (n!=pCS->nChars) {
	fosInternalError("bad count in pcfReadBitmaps (expected %d, got %d)\n",
								pCS->nChars,n);
	return(FALSE);
    }

    offsets= (CARD32 *)fosAlloc(n*sizeof(CARD32));
    if (offsets==NULL) {
	fosError("allocation failure in pcfReadBitmaps (offsets,%d)\n",
							n*sizeof(CARD32));
	return(FALSE);
    }
    pCS->pBitOffsets= (char **)offsets;

    for (i=0;i<n;i++) {
	offsets[i]=	pcfReadInt32(file);
    }

    for (i=0;i<GLYPHPADOPTIONS;i++) {
	pCS->bitmapsSizes[i]= pcfReadInt32(file);
    }

    glyphSize= pCS->bitmapsSizes[FMT_GLYPH_PAD_INDEX(fileFormat)];
    tmpGlyphs= (unsigned char *)fosAlloc(glyphSize);
    pCS->pBitmaps= (char *)tmpGlyphs;
    if (tmpGlyphs==NULL) {
	fosError("allocation in pcfReadBitmaps (glyphs, %d)\n",glyphSize);
	goto BAILOUT_OFFSETS;
    }
    n= fosReadBlock(file,tmpGlyphs,glyphSize);
    if (n!=glyphSize) {
	fosError("short read in pcfReadBitmaps (glyphs, %d!=%d)\n",n,glyphSize);
	goto BAILOUT_ALL;
    }

    if (FMT_BIT_ORDER(fileFormat)!=FMT_BIT_ORDER(outPadOrder)) {
	BitOrderInvert(tmpGlyphs,glyphSize);
    }
    if ((FMT_BYTE_ORDER(fileFormat)==FMT_BIT_ORDER(fileFormat))!=
		(FMT_BYTE_ORDER(outPadOrder)==FMT_BIT_ORDER(outPadOrder))) {
	switch (FMT_SCAN_UNIT(fileFormat)) {
	    case 1:	break;
	    case 2:	TwoByteInvert(tmpGlyphs,glyphSize); break;
	    case 4:	FourByteInvert(tmpGlyphs,glyphSize); break;
	    /* 5/16/89 (ef) -- what about 8? */
	}
    }
    if (FMT_GLYPH_PAD_INDEX(fileFormat)!=FMT_GLYPH_PAD_INDEX(outPadOrder)) {
	unsigned char	*padTmp,*pOrigGlyph;
	int	padSize= pCS->bitmapsSizes[FMT_GLYPH_PAD_INDEX(outPadOrder)];
	CharInfoPtr	pCI;

	padTmp= (unsigned char *)fosAlloc(padSize);
	if (padTmp==NULL) {
	    fosError("allocation failure in pcfReadBitmaps (repad, %d)\n",
								padSize);
	    goto BAILOUT_ALL;
	}
	pCS->pBitmaps= (char *)padTmp;
	padSize= FMT_GLYPH_PAD(outPadOrder);

	for (i=0;i<pCS->nChars;i++) {
	    pCI= &pCS->ci.pCI[i];
/* 5/31/89 (ef) -- Ok,ok!  It's a hack! */
	    if (BYTES_FOR_GLYPH(pCI,padSize)==0) {
		pCS->pBitOffsets[i]= (char *)NULL;
		continue;
	    }
	    /* offsets and pBitOffsets are pointers to the same thing */
	    pOrigGlyph=	tmpGlyphs+offsets[i];
	    pCS->pBitOffsets[i]= (char *)padTmp;
	    padTmp+=RepadBitmap(pOrigGlyph,padTmp,
	    	    FMT_GLYPH_PAD(fileFormat),FMT_GLYPH_PAD(outPadOrder),
		    pCI->metrics.rightSideBearing-pCI->metrics.leftSideBearing,
		    pCI->metrics.ascent+pCI->metrics.descent);
	    
	}
	fosFree(tmpGlyphs);
    }
    else {
	int padSize= FMT_GLYPH_PAD(outPadOrder);
	for (i=0;i<pCS->nChars;i++) {
/* 5/31/89 (ef) -- hack hack hack */
	    if (BYTES_FOR_GLYPH(&pCS->ci.pCI[i],padSize)==0)
		pCS->pBitOffsets[i]=	(char *)NULL;
	    else
		pCS->pBitOffsets[i]=	(char *)tmpGlyphs+offsets[i];
	}
    }
    pCS->tables|= FONT_BITMAPS;

    PCF_SKIP_PADDING(file,i);
    return(TRUE);

BAILOUT_ALL:
    fosFree(pCS->pBitmaps);
    pCS->pBitmaps= NULL;
BAILOUT_OFFSETS:
    fosFree(pCS->pBitOffsets);
    pCS->pBitOffsets= NULL;
    return(FALSE);
}
