
#include <X11/X.h>
#include <X11/Xatom.h>
#include "misc.h"
#include "font.h"
#include "fontlib.h"
#include "fontos.h"
#include "fosfilestr.h"

#include "pcf.h"
#include "pcfint.h"

/***====================================================================***/

static	pcfOrderFuncs	pcfMSBFuncs = {
    fosReadMSB16,fosReadMSB32,fosWriteMSB16,fosWriteMSB32
};

static	pcfOrderFuncs	pcfLSBFuncs = {
    fosReadLSB16,fosReadLSB32,fosWriteLSB16,fosWriteLSB32
};

/***====================================================================***/

void
pcfSetByteOrder(pFile,order)
fosFilePtr	pFile;
unsigned	order;
{

    if (pFile) {
	if (order==MSBFirst)	pFile->fmtPrivate=	(pointer)&pcfMSBFuncs;
	else			pFile->fmtPrivate=	(pointer)&pcfLSBFuncs;
    }
    return;
}

/***====================================================================***/

void
pcfPrintTOC(nEntries,pToc)
int	 	 nEntries;
TableDesc	*pToc;
{
int	i;

    fosInformation("type              format(order)        start      size\n");
    fosInformation("----              -------------        -----      ----\n");
    for (i=0;i<nEntries;i++) {
	switch (pToc[i].label) {
	    case FONT_PROPERTIES:	fosInformation("properties        "); 
					break;
	    case FONT_ACCELERATORS:	fosInformation("accelerators      "); 
					break;
	    case FONT_METRICS:		fosInformation("metrics           ");
					break;
	    case FONT_BITMAPS:		fosInformation("bitmaps           "); 
					break;
	    case FONT_INK_METRICS:	fosInformation("ink metrics       "); 
					break;
	    case FONT_BDF_ENCODINGS:	fosInformation("bdf encoding      "); 
					break;
	    case FONT_SWIDTHS:		fosInformation("scalable widths   ");
					break;
	    case FONT_GLYPH_NAMES:	fosInformation("glyph names       "); 
					break;
	    default:			fosInformation("<unknown>         "); 
					break;
	}
	fosInformation("0x%8.8x(%c%c%d%d)",pToc[i].format,
			((FMT_BIT_ORDER(pToc[i].format)==MSBFirst)?'m':'l'),
			((FMT_BYTE_ORDER(pToc[i].format)==MSBFirst)?'M':'L'),
			FMT_GLYPH_PAD(pToc[i].format),
			FMT_SCAN_UNIT(pToc[i].format));
	fosInformation("%10d",pToc[i].start);
	fosInformation("%10d\n",pToc[i].size);
    }
    return;
}

/***====================================================================***/

extern	char	*NameForAtom();
extern	void	fosFatalError();

char *
NameForAtomOrNone(atom)
    Atom atom;
{
    static char *nullString = "";
    char *str;

    if (atom == None)
	return nullString;
    str = NameForAtom(atom);
    if (str == NULL)
	fosFatalError("atom lookup failed for %d", atom);
    return str;
}

