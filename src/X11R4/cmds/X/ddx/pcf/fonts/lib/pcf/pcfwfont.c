#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"
#include "fosfilestr.h"

#include "pcf.h"

#define	PCF_NEED_WRITERS
#include "pcfint.h"

/***====================================================================***/

pcfWriteFuncsRec  pcfWriteFuncs[NUM_FONT_TABLES] = {
	{ pcfSizeProperties,	pcfWriteProperties,	0 },
	{ pcfSizeAccelerators,	pcfWriteAccelerators,	0 },
	{ pcfSizeMetrics,	pcfWriteMetrics,	pcfSelectMetricFormat },
	{ pcfSizeBitmaps,	pcfWriteBitmaps,	0 },
	{ pcfSizeMetrics,	pcfWriteMetrics,	pcfSelectMetricFormat },
	{ pcfSizeEncoding,	pcfWriteEncoding,	0 },
	{ pcfSizeSWidths,	pcfWriteSWidths,	0 },
	{ pcfSizeGlyphNames,	pcfWriteGlyphNames,	0 }
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

int
pcfWriteFont(file,pFont,tables,params)
fosFilePtr	file;
EncodedFontPtr	pFont;
Mask		tables;
BuildParamsPtr	params;
{
int		nTables=0,index,i;
int		tmpTables;
TableDesc	toc[NUM_FONT_TABLES];
unsigned	fmtOrder;


    fmtOrder= 0;
    FMT_SET_BIT_ORDER(fmtOrder,params->bitOrder);
    FMT_SET_BYTE_ORDER(fmtOrder,params->byteOrder);
    FMT_SET_GLYPH_PAD(fmtOrder,params->glyphPad);
    FMT_SET_SCAN_UNIT(fmtOrder,params->scanUnit);

    /* set up table of contents */
    nTables= 	0;
    tmpTables=	tables&FONT_LEGAL_TABLES;
    while (tmpTables) {
	toc[nTables].label=	lowbit(tmpTables);
	index= ffs(toc[nTables].label)-1;

	if (pcfWriteFuncs[index].formatFunc) 
	    	toc[nTables].format=	(*pcfWriteFuncs[index].formatFunc)
							      (pFont,fmtOrder);
	else	toc[nTables].format=	fmtOrder;

	toc[nTables].size=	(*pcfWriteFuncs[index].sizeFunc)(pFont,
							   toc[nTables].format);
	tmpTables&= ~toc[nTables].label;
	if (toc[nTables].size>0)
	    nTables++;
    }

    fosWriteLSB32(file,PCF_FILE_VERSION);

    /* write table of contents */
    fosWriteLSB32(file,nTables);
    for (i=0;i<nTables;i++) {
	if (i==0)	toc[i].start= 8+(16*nTables);
	else		toc[i].start= toc[i-1].start+toc[i-1].size;
	fosWriteLSB32(file,toc[i].label);
	fosWriteLSB32(file,toc[i].format);
	fosWriteLSB32(file,toc[i].size);
	fosWriteLSB32(file,toc[i].start);
    }

    /* write the tables */
    for (i=0;i<nTables;i++) {
	if (fosFilePosition(file)!=toc[i].start) {
	    fosInternalError("writing %s table at %d, expected %d\n",
						fontTableName(toc[i].label),
						fosFilePosition(file),
						toc[i].start);
	    if (toc[i].start>fosFilePosition(file)) {
		fosPad(file,toc[i].start-fosFilePosition(file));
	    }
	}
	index= ffs(toc[i].label)-1;
	if ((toc[i].label==FONT_METRICS)||(toc[i].label==FONT_INK_METRICS))
	    (*pcfWriteFuncs[index].writeFunc)(file,pFont,toc[i].label,
							toc[i].format);
	else
	    (*pcfWriteFuncs[index].writeFunc)(file,pFont,toc[i].format);

	tables&= (~toc[i].label);
    }
    return(tables);
}
