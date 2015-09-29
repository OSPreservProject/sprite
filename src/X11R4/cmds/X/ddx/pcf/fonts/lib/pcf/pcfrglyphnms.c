#include <X11/X.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"
#include "fosfilestr.h"

#define	 PCF_NEED_READERS
#include "pcf.h"
#include "pcfint.h"

/***====================================================================***/

	/*\
	 * Format of a block of Glyph Names is:
	 * GlyphNames	::=	names_format	: CARD32
	 *			num_chars	: CARD32
	 *			offsets		: LISTofCARD32
	 *			size_names	: CARD32
	 *			names		: LISTofSTRING
	 * Size is:	8+(num_chars*4)+4+size_names
	\*/

/***====================================================================***/

int
pcfReadGlyphNames(file,pCS)
fosFilePtr	file;
CharSetPtr	pCS;
{
register int	i;
CARD32	*tmpOffsets;
char	*tmpNames,*name;
int	size,tmp;
Mask	format;

    ASSERT( "pcfReadGlyphNames", sizeof(char *) == sizeof(CARD32) );

    if (pCS->tables&FONT_GLYPH_NAMES) {
	fosError("pcfReadGlyphNames called with glyph names loaded\n");
	return(FALSE);
    }
    format=	fosReadLSB32(file);
    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("Unknown glyph names format 0x%x\n",format&FORMAT_MASK);
	return(FALSE);
    }
    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));

    i= pcfReadInt32(file);

    if (pCS->nChars==0)
	pCS->nChars= i;
    else if (i!=pCS->nChars) {
	fosError("bad count in pcfReadGlyphNames (expected %d, got %d)\n",
							pCS->nChars,i);
	return(FALSE);
    }

    tmpOffsets= (CARD32 *)fosAlloc(pCS->nChars*sizeof(CARD32));
    if (tmpOffsets==NULL) {
	fosError("allocation failure in pcfReadGlyphNames (offsets,%d)\n",
						pCS->nChars*sizeof(CARD32));
	return(FALSE);
    }
    pCS->glyphNames=	(Atom *)tmpOffsets;

    for (i=0;i<pCS->nChars;i++) {
	tmpOffsets[i]=	pcfReadInt32(file);
    }

    size=	pcfReadInt32(file);

    tmpNames= (char *)fosTmpAlloc(size);
    if (tmpNames==NULL) {
	fosError("allocation failure in pcfReadGlyphNames (names, %d)\n",size);
	goto BAILOUT;
    }

    i= fosReadBlock(file,tmpNames,size);
    if (i!=size) {
	fosError("short read in pcfReadGlyphNames (names, %d!=%d)\n",i,size);
	fosTmpFree(tmpNames);
	goto BAILOUT;
    }

    /* be careful here, tmpOffsets == pCS->glyphNames */
    for (i=0;i<pCS->nChars;i++) {
	name= tmpNames+tmpOffsets[i];
	pCS->glyphNames[i]= MakeAtom(name,strlen(name)+1,TRUE);
    }
    fosTmpFree(tmpNames);
    pCS->tables|= FONT_GLYPH_NAMES;

    PCF_SKIP_PADDING(file,i);
    return(TRUE);
BAILOUT:
    fosFree(tmpOffsets);
    pCS->glyphNames= NULL;
    return(FALSE);
}
