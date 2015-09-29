#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"
#include "fosfilestr.h"

#include "pcf.h"

#define	PCF_NEED_READERS
#include "pcfint.h"

/***====================================================================***/

pcfReadFunc	pcfReadFuncs[NUM_FONT_TABLES] = {
	pcfReadProperties,
	pcfReadAccelerators,
	pcfReadMetrics,
	pcfReadBitmaps,
	pcfReadMetrics,
	pcfReadEncoding,
	pcfReadSWidths,
	pcfReadGlyphNames
};

/***====================================================================***/

	/*\
	 * Format of font file is;
	 * FontFile	::=	version_num		: Format
         *			table_of_contents	: DescTables
	 *			tables			: LISTofFontTable
	 *
	 * DescTables	::=	num_tables		: CARD32
	 *			tables			: LISTofDescTable
	 *
	 * DescTable	::=	table_type		: CARD32
	 *			table_format		: CARD32
	 *			table_size		: CARD32
	 *			table_offset		: CARD32
	 *
	 * FontTable	::=	accelerators		: Accelerators
	 * FontTable	::=	font_metrics		: CharMetrics
	 * FontTable	::=	ink_metrics		: CharMetrics
	 * FontTable	::=	pictures		: GlyphPictures
	 * FontTable	::=	names			: GlyphNames
	 * FontTable	::=	scalable_widths		: ScalableWidths
	 * FontTable	::=	property_list		: Properties
	 * FontTable	::=	encodings		: Encodings
	\*/

Mask
pcfReadFont(file,ppFont,ppCS,tables,params)
fosFilePtr	 file;
EncodedFontPtr	*ppFont;
CharSetPtr	*ppCS;
Mask		 tables;
BuildParamsPtr	 params;
{
EncodedFontPtr	pFont;
CharSetPtr	pCS;
TableDesc	toc[NUM_FONT_TABLES];
int		i,index,nTables;
int		nRead;
Mask		allocated= 0;
unsigned long	format=0;
int		position;

    tables&= FONT_LEGAL_TABLES;
    if (!tables) 
	return(0);

    FMT_SET_BIT_ORDER(format,params->bitOrder);
    FMT_SET_BYTE_ORDER(format,params->byteOrder);
    FMT_SET_GLYPH_PAD(format,params->glyphPad);
    FMT_SET_SCAN_UNIT(format,params->scanUnit);

    if (ppFont&&(*ppFont))	pFont= *ppFont;
    else			pFont= NullEncodedFont;
    if ((tables&FONT_BDF_ENCODINGS)&&(!pFont)) {
	pFont= (EncodedFontPtr)fosAlloc(sizeof(EncodedFontRec));
	if (pFont==NULL) {
	    fosError("allocation failure in pcfReadFont (pFont)\n");
	    return(tables);
	}
	allocated|= FONT_BDF_ENCODINGS;
	bzero(pFont,sizeof(EncodedFontRec));
    }

    if ((!ppCS)||(!*ppCS)) {
	if ((pFont)&&(pFont->pCS)) {
	    pCS= pFont->pCS;
	}
	else {
	    pCS= (CharSetPtr)fosAlloc(sizeof(CharSetRec));
	    if (pCS==NULL) {
		fosError("allocation failure in pcfReadFont (pCS)\n");
		goto BAILOUT;
	    }
	    allocated|= FONT_ACCELERATORS;
	    bzero(pCS,sizeof(CharSetRec));
	    if (pFont) {
		pFont->pCS=	pCS;
		pCS->refcnt=	1;
	    }
	}
    }
    else  {
	pCS= *ppCS;
    }
    tables&= (~pCS->tables);

    /* verify version number */
    i= fosReadLSB32(file);
    if (i!=PCF_FILE_VERSION) {
	fosError("incorrect font file version (expected 0x%xd, found 0x%x)\n",
						PCF_FILE_VERSION,i);
	goto BAILOUT;
    }
    /* read table of contents */
    nTables= fosReadLSB32(file);

    for (i=0;(i<nTables)&&(i<MAX_FONT_TABLES);i++) {
	toc[i].label=	fosReadLSB32(file);
	toc[i].format=	fosReadLSB32(file);
	toc[i].size=	fosReadLSB32(file);
	toc[i].start=	fosReadLSB32(file);
    }
    if (nTables>MAX_FONT_TABLES) {
	fosInternalError("nTables>MAX_FONT_TABLES.  ignoring last %d tables\n",
						nTables-MAX_FONT_TABLES);
	fosSkip(file,(nTables-MAX_FONT_TABLES)*16);
    }

    /* read the tables */
    for (i=0;i<nTables;i++) {
	position= fosFilePosition(file);
	if (position!=toc[i].start) {
            fosInternalError("pcfReadFont reading %s at %d, expected %d\n",
						fontTableName(toc[i].label),
					   	position,toc[i].start);
	    if (toc[i].start>position)
		fosSkip(file,toc[i].start-position);
	}

	if (tables&toc[i].label) {
	    pcfReadFunc	rfunc;

	    index= ffs(toc[i].label)-1;
	    rfunc= pcfReadFuncs[index];
	    switch (toc[i].label) {
		case FONT_BDF_ENCODINGS:
		    if ((*rfunc)(file,pFont))
			tables&=	(~FONT_BDF_ENCODINGS);
		    break;
		case FONT_METRICS:
		    (*rfunc)(file,pCS,FONT_METRICS,&pCS->ci.pCI);
		    break;
		case FONT_INK_METRICS:
		    (*rfunc)(file,pCS,FONT_INK_METRICS,&pCS->inkci.pCI);
		    break;
		case FONT_BITMAPS:
		    (*rfunc)(file,pCS,format);
		    break;
		default:
		    (*rfunc)(file,pCS);
		    break;
	    }
	}
	else {
	    fosSkip(file,toc[i].size);
	}
    }
    if (ppFont)	*ppFont= pFont;
    if (ppCS)	*ppCS= pCS;

    /* if there *are* no ink metrics, and we wanted them, they're as good */
    /* as read (as read as they're going to get)                          */
    if ((!pCS->tables&FONT_INK_METRICS)&&(!pCS->inkMetrics)) {
	tables&= (~FONT_INK_METRICS);
    }

    tables&= (~pCS->tables);
    return(tables);
BAILOUT:
    if (allocated&FONT_ACCELERATORS) {
	if ((pFont)&&(pFont->pCS==pCS))	pFont->pCS= NULL;
	fosFree(pCS);
    }
    if (allocated&FONT_BDF_ENCODINGS)	fosFree(pFont);
    return(tables);
}
