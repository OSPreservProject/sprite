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
	 * Format of a block of properties is:
	 *	Properties	::=	props_format	:	CARD32
	 *				num_props	:	CARD32
	 *				properties	:	LISTofProperty
	 *				pad0		: 	pad to word
	 *				prop_string_size:	CARD32
	 *				prop_strings	:	LISTofSTRING
	 *				pad1		:	pad to word
	 *
	 *	Property	::=	name_offset	:	CARD32
	 *				type		:	CARD8
	 *				value		:	CARD32
	 *
	 * Size is:	8+PAD(9*num_props)+4+PAD(prop_string_size)
	 *
	\*/

/***====================================================================***/

int
pcfReadProperties(file,pCS)
fosFilePtr	file;
CharSetPtr	pCS;
{
register int	i;
register FontPropPtr	 pPr;
unsigned char		*tmpStringBuf= NULL;
	 int	 	 size;
	 Mask		 format;

    if (pCS->tables&FONT_PROPERTIES) {
	fosError("pcfReadProperties called with properties loaded\n");
	return(FALSE);
    }
    format=		fosReadLSB32(file);
    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("Unkown property format 0x%x\n",format&FORMAT_MASK);
	return(FALSE);
    }
    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));

    pCS->nProps=	pcfReadInt32(file);

    size= pCS->nProps*sizeof(FontPropRec);
    pCS->props= 	(FontPropPtr)fosAlloc(size);
    if (pCS->props==NULL) {
	fosError("allocation failure in pcfReadProperties (props, %d)\n",size);
	return(FALSE);
    }

    size= pCS->nProps*sizeof(Bool);
    pCS->isStringProp=	(Bool  *)fosAlloc(size);
    if (pCS->isStringProp==NULL) {
	fosError("allocation failure in pcfReadProperties (isstring, %d)\n",
									size);
	goto BAILOUT;
							
    }

    for (i=0,pPr=pCS->props;i<pCS->nProps;i++,pPr++) {
	pPr->name=	 (Atom)pcfReadInt32(file);
	pCS->isStringProp[i]= fosReadInt8(file);
	pPr->value=	 (Atom)pcfReadInt32(file);
    }
    PCF_SKIP_PADDING(file,i);

    size=		pcfReadInt32(file);
    tmpStringBuf=	(unsigned char *)fosTmpAlloc(size);
    if (tmpStringBuf==NULL) {
	fosError("allocation failure in pcfReadProperties (tmpbuf,%d)\n",size);
	goto BAILOUT;
    }
    i= fosReadBlock(file,tmpStringBuf,size);
    if (i!=size) {
	fosError("short read in pcfReadProperties (tmpbuf, %d!=%d)\n",i,size);
	goto BAILOUT;
    }
    for (i=0,pPr=pCS->props;i<pCS->nProps;i++,pPr++) {
	register char *str;
	str= 		(char *)tmpStringBuf+((CARD32)pPr->name);
	pPr->name=	MakeAtom(str,strlen(str)+1,TRUE);
	if (pCS->isStringProp[i]) {
	    str= 	(char *)tmpStringBuf+((CARD32)pPr->value);
	    pPr->value=	MakeAtom(str,strlen(str)+1,TRUE);
	}
    }
    pCS->tables|= FONT_PROPERTIES;
    fosTmpFree(tmpStringBuf);
    PCF_SKIP_PADDING(file,i);
    return(TRUE);
BAILOUT:
    if (pCS->props) {
	fosFree(pCS->props);
	pCS->props= NULL;
    }
    if (pCS->isStringProp) {
	fosFree(pCS->isStringProp);
	pCS->isStringProp= NULL;
    }
    if (tmpStringBuf) {
	fosTmpFree(tmpStringBuf);
    }
    return(FALSE);
}
