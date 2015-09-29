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
	 * Format of a block of properties is:
	 *	Properties	::=	props_format	:	CARD32
	 *				num_props	:	CARD32
	 *				properties	:	LISTofProperty
	 *				pad0		:	pad to word
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

int
pcfSizeProperties(pFont,format)
EncodedFontPtr	 pFont;
Mask	 	 format;
{
register CharSetPtr pCS= pFont->pCS;
int		 propStringSize,i,n=0;
FontPropPtr	 pPr;

    if (pFont&&pCS&&(pCS->tables&FONT_PROPERTIES)&&
				((format&FORMAT_MASK)==PCF_DEFAULT_FORMAT)) {
	propStringSize= 0;
	for (i=0,pPr=pCS->props;i<pCS->nProps;i++,pPr++) {
	    propStringSize+= strlen((char *)NameForAtomOrNone(pPr->name))+1;
	    if (pCS->isStringProp[i]) {
		propStringSize+=strlen((char *)NameForAtomOrNone(pPr->value))+1;
	    }
	}
	n=	8+(9*pCS->nProps);
	n+=	pcfAlignPad(n);
	n+=	4+propStringSize+pcfAlignPad(propStringSize);
    }
    return(n);
}

/***====================================================================***/

int
pcfWriteProperties(file,pFont,format)
fosFilePtr	file;
EncodedFontPtr	pFont;
Mask		format;
{
register int	i,tmp;
register CharSetPtr	pCS= pFont->pCS;
register FontPropPtr	pPr;
	int	offset,offset2;

    if ((format&FORMAT_MASK)!=PCF_DEFAULT_FORMAT) {
	fosError("pcfWriteProperties: unknown format 0x%x\n",
							format&FORMAT_MASK);
	return(FALSE);
    }
    if (!(pFont->pCS->tables&FONT_PROPERTIES)) {
	fosError("pcfWriteProperties called without properties loaded\n");
	return(FALSE);
    }
    offset= 0;
    fosWriteLSB32(file,format);

    pcfSetByteOrder(file,FMT_BYTE_ORDER(format));
    pcfWriteInt32(file,pCS->nProps);

    for (i=0,pPr=pCS->props;i<pCS->nProps;i++,pPr++) {
	pcfWriteInt32(file,offset);
	offset+= strlen((char *)NameForAtomOrNone(pPr->name))+1;
	fosWriteInt8(file,pCS->isStringProp[i]);
	if (pCS->isStringProp[i]) {
	    pcfWriteInt32(file,offset);
	    offset+= strlen((char *)NameForAtomOrNone(pPr->value))+1;
	}
	else {
	    pcfWriteInt32(file,pPr->value);
	}
    }

    PCF_ADD_PADDING(file,i);

    offset2= 0;
    pcfWriteInt32(file,offset);
    for (i=0,pPr=pCS->props;i<pCS->nProps;i++,pPr++) {
	register char *str;
	str= 		(char *)NameForAtomOrNone(pPr->name);
	tmp= 		fosWriteBlock(file,str,strlen(str)+1);
	offset2+=	tmp;
	if (pCS->isStringProp[i]) {
	    str=	(char *)NameForAtomOrNone(pPr->value);
	    tmp=	fosWriteBlock(file,str,strlen(str)+1);
	    offset2+= 	tmp;
	}
    }
    if (offset!=offset2) {
	fosError("pcfWriteProperties: internal error! wrote %d, expected %d\n",
							offset2,offset);
    }

    PCF_ADD_PADDING(file,i);
    return(TRUE);
}
