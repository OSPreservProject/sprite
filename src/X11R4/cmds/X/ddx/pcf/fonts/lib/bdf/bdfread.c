/************************************************************************
Copyright 1989 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

/* $XConsortium: bdftosnf.c,v 1.25 88/10/10 19:37:37 karlton Exp $ */

#include <ctype.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "fosfilestr.h"

#include "bdfint.h"

#define INDICES 256
#define MAXENCODING 0xFFFF

/***====================================================================***/

Bool
bdfReadBitmap(pCI,ppOutPicture,pFile,params,sizes)
CharInfoPtr	pCI;
char		**ppOutPicture;
fosFilePtr	pFile;
BuildParamsPtr	params;
int		sizes[GLYPHPADOPTIONS];
{
int	widthBits,widthBytes,widthHexChars;
int	height,row;
int	i,inLineLen,nextByte;
Bool	badbits;
unsigned char	*pInBits,*picture,*line;


    widthBits=	pCI->metrics.rightSideBearing-pCI->metrics.leftSideBearing;
    height=	pCI->metrics.ascent+pCI->metrics.descent;

    widthBytes = BYTES_PER_ROW(widthBits, params->glyphPad);
    if (widthBytes*height>0) {
	picture=	(unsigned char *)fosAlloc(widthBytes * height);
	*ppOutPicture=	(char *)picture;
    } 
    else {
	picture=	(unsigned char *)NULL;
	*ppOutPicture=	(char *)NULL;
    }

    for (i= 0; i < GLYPHPADOPTIONS; i++) {
	static pad[GLYPHPADOPTIONS] = {1, 2, 4, 8};
	sizes[i] += BYTES_PER_ROW(widthBits, pad[i]) * height;
    }

    badbits= 		FALSE;
    nextByte= 		0;
    widthHexChars=	BYTES_PER_ROW(widthBits,1);

/* 5/31/89 (ef) -- hack, hack, hack.  what *am* I supposed to do with */
/*		0 width characters? */
    for (row=0; row < height; row++) {
	line=	bdfGetLine(pFile);
	if (!line)		break;

	if (widthBits==0) {
	    if ((!line)||(bdfIsPrefix(line,"ENDCHAR")))	break;
	    else					continue;
	}

	pInBits=	line;
	inLineLen=	strlen(pInBits);

	if (inLineLen & 1) {
	    bdfError(pFile,"odd number of characters in hex encoding\n");
	    line[inLineLen++]= '0';
	    line[inLineLen]= '\0';
	}

	inLineLen >>= 1;
	i=	MIN(widthHexChars, inLineLen);
	for (; i > 0; i--, pInBits+=2, nextByte++) {
	    picture[nextByte] = bdfHexByte(pInBits);
	}

	/* pad if line is too short */
	if (inLineLen<widthHexChars) {
	    for (i=widthHexChars-inLineLen;i>0;i--) {
		picture[nextByte++]=	0;
	    }
	}
	else {
	    unsigned char mask= ((unsigned)0x00ff)>>(widthBits&0x7);
	    if ((widthBits&0x7)&&(picture[nextByte-1]&mask)) {
/*		picture[nextByte-1] &= mask;*/
		badbits= TRUE;
	    }
	    else if (inLineLen>widthHexChars) {
		badbits= TRUE;
	    }
	}

	if (widthBytes>widthHexChars) {
	    i= widthBytes-widthHexChars;
	    while (i-->0) {
		picture[nextByte++]= 0;
	    }
	}
    }

    if ((line&&(!bdfIsPrefix(line, "ENDCHAR")))||(height==0))
	line=	bdfGetLine(pFile);

    if ((!line)||(!bdfIsPrefix(line, "ENDCHAR"))) {
	bdfError(pFile,"missing 'ENDCHAR'\n");
	goto BAILOUT;
    }

    if ( picture!=NULL ) {
	if (nextByte != height*widthBytes) {
	    fosInternalError("bytes != rows * bytes_per_row (%d != %d * %d)\n",
					nextByte, height,widthBytes);
	    goto BAILOUT;
	}

	if ((badbits)&&(params->badbitsWarn)) {
	    bdfWarning(pFile,"bits outside bounding box ignored\n");
	}

	if (params->bitOrder == LSBFirst)
	    BitOrderInvert(picture, nextByte);
	if (params->bitOrder != params->byteOrder) {
	    if (params->scanUnit == 2)
		TwoByteInvert(picture, nextByte);
	    else if (params->scanUnit == 4)
		FourByteInvert(picture, nextByte);
	}
    }
    return(TRUE);
BAILOUT:
    if (picture)	fosFree(picture);
    *ppOutPicture=	NULL;
    return(FALSE);
}

/***====================================================================***/

Bool
bdfSkipBitmap(pFile,height)
fosFilePtr	pFile;
int		height;
{
unsigned char	*line;
int		 i=0;

    do {
	line=	bdfGetLine(pFile);
	i++;
    } while ((line)&&(!bdfIsPrefix(line,"ENDCHAR"))&&(i<=height));

    if ((i>1)&&(line)&&(!bdfIsPrefix(line,"ENDCHAR"))) {
	bdfError(pFile,"Error in bitmap, missing 'ENDCHAR'\n");
	bdfInformation(pFile,"height is %d, read %d (%s)\n",height,i,
				((char *)line)?((char *)line):"EOF");
	return(FALSE);
    }
    return(TRUE);
}

/***====================================================================***/

	/*\
	 * WARNING!!!!! If none of the tables this function would read
	 *	are requested, it reads *NOTHING* -- it does NOT skip
	 *	the characters.
	\*/

Bool
bdfReadCharacters(pFile, pFont, pCS, params, tables)
fosFilePtr	 pFile;
EncodedFontPtr	 pFont;
CharSetPtr	 pCS;
BuildParamsPtr	 params;
Mask		 tables;	/* which parts of the font do we save? */
{
unsigned char	*line;
register	 CharInfoPtr ci;
int		 i, ndx, nchars;
int		 char_row, char_col;
int		 numEncodedGlyphs = 0;
CharInfoPtr	*bdfEncoding[INDICES];/* each row contains pointers into ci */
bdfFileState	*pState;

    if (!tables&(FONT_METRICS|FONT_SWIDTHS|FONT_GLYPH_NAMES|
		 FONT_BITMAPS|FONT_BDF_ENCODINGS)) {
	return(TRUE);
    }

    line= bdfGetLine(pFile);
    if ((!line)||(sscanf(line, "CHARS %d", &nchars) != 1)) {
	bdfError(pFile,"bad 'CHARS' in bdf file\n");
	return(FALSE);
    }
    if (nchars < 1) {
	bdfError(pFile,"invalid number of CHARS in BDF file\n");
	return(FALSE);
    }

    tables&= (FONT_LEGAL_TABLES&(~pCS->tables));

    bzero(bdfEncoding,sizeof(bdfEncoding));

    pCS->nChars=	nchars;
    if (tables&FONT_METRICS) {
	ci= 		(CharInfoPtr)fosAlloc(nchars * sizeof(CharInfoRec));
	pCS->ci.pCI=	ci;
	if (ci==NULL) {
	    bdfWarning(pFile,"Couldn't allocate pCI (%d*%d)\n", nchars,
							   sizeof(CharInfoRec));
	    bdfInformation(pFile,"character metrics will not be read\n");
	    tables&= (~FONT_METRICS);
	}
    }
    else {
/* 8/22/89 (ef) -- God this is a hack.  I hate myself */
	static CharInfoRec tmpCI;
	ci= &tmpCI;
    }

    if (tables&FONT_BITMAPS) {
	pCS->pBitOffsets=	(char **)fosAlloc(nchars * sizeof(char *));
	if (pCS->pBitOffsets==NULL) {
	    bdfWarning(pFile,"Couldn't allocate pBitOffsets (%d*%d)\n",
							nchars,sizeof(char *));
	    bdfInformation(pFile,"character bitmaps will not be read\n");
	    tables&= (~FONT_BITMAPS);
	}
    }

    if (tables&FONT_GLYPH_NAMES) {
	pCS->glyphNames=	(Atom *)fosAlloc(nchars * sizeof(Atom));
	if (pCS->glyphNames==NULL) {
	    bdfWarning(pFile,"Couldn't allocate glyphNames (%d*%d)\n",
							nchars,sizeof(Atom));
	    bdfInformation(pFile,"character names will not be read\n");
	    tables&= (~FONT_GLYPH_NAMES);
	}
    }

    if (tables&FONT_SWIDTHS) {
	pCS->sWidth=	(int *)fosAlloc(nchars * sizeof(int));
	if (pCS->sWidth==NULL) {
	    bdfWarning(pFile,"Couldn't allocate sWidth (%d *%d)\n",
							nchars,sizeof(int));
	    bdfInformation(pFile,"scalable widths will not be read\n");
	    tables&= (~FONT_SWIDTHS);
	}
    }

    line= bdfGetLine(pFile);
    for (ndx=0;(ndx<nchars)&&(line)&&(bdfIsPrefix(line,"STARTCHAR"));ndx++) {
        int	t;
	int	ix;	/* counts bytes in a glyph */
	int	wx;	/* x component of width */
	int	wy;	/* y component of width */
	int	bw;	/* bounding-box width */
	int	bh;	/* bounding-box height */
	int	bl;	/* bounding-box left */
	int	bb;	/* bounding-box bottom */
	int	enc, enc2;	/* encoding */
	unsigned char	*p;	/* temp pointer into line*/
	int	bytesperrow, row, hexperrow, perrow, badbits, nextByte;
	unsigned char *picture;	/* for holding the latest glyph */
	char	charName[100];

	if (sscanf(line, "STARTCHAR %s", charName) != 1) {
	    bdfError(pFile,"bad character name in BDF file\n");
	    goto BAILOUT;	/* bottom of function, free and return error */
	}

	if (tables&FONT_GLYPH_NAMES) {
	    pCS->glyphNames[ndx] = bdfForceMakeAtom(charName, NULL);
	}

	line=	bdfGetLine(pFile);
	if ((!line)||(t=sscanf(line, "ENCODING %d %d", &enc, &enc2)) < 1) {
	    bdfError(pFile,"bad 'ENCODING' in BDF file\n");
	    goto BAILOUT;
	}
	if ((enc < -1) || ((t == 2) && (enc2 < -1))) {
	    bdfError(pFile,"bad ENCODING value");
	    goto BAILOUT;
	}

	if (t == 2 && enc == -1)
	    enc = enc2;
	if (enc == -1) {
	    if (params->ignoredCharWarn)
		bdfWarning(pFile,"character '%s' is not in default encoding\n", charName);
	}
	else if (enc > MAXENCODING) {
	    bdfError(pFile,"char '%s' has encoding too large (%d)\n",
							charName, enc);
	    if (params->ignoredCharWarn)
		bdfWarning(pFile,"character '%s' will be ignored\n", charName);
	}
	else if (tables&FONT_BDF_ENCODINGS) {
	    char_row = (enc >> 8) & 0xFF;
	    char_col = enc & 0xFF;
	    if (char_row < pFont->firstRow)	pFont->firstRow= char_row;
	    if (char_row > pFont->lastRow)	pFont->lastRow=	 char_row;
	    if (char_col < pFont->firstCol)	pFont->firstCol= char_col;
	    if (char_col > pFont->lastCol)	pFont->lastCol=	 char_col;
	    if (bdfEncoding[char_row] == (CharInfoPtr *)NULL) {
		bdfEncoding[char_row] =
		    (CharInfoPtr *)fosCalloc(INDICES,sizeof(CharInfoPtr));
		if (bdfEncoding[char_row]) {
		    for (i = 0; i < INDICES; i++) {
			bdfEncoding[char_row][i] = (CharInfoPtr)NULL;
		    }
		}
		else {
		    bdfWarning(pFile,"Couldn't allocate row %d of encoding (%d*%d)\n",
					char_row,INDICES,sizeof(CharInfoPtr));
		    bdfInformation(pFile,"Encoding will be incomplete\n");
		}
	    }
	    if (bdfEncoding[char_row]!=NULL) {
		bdfEncoding[char_row][char_col] = ci;
		numEncodedGlyphs++;
	    }
	}

	line=	bdfGetLine(pFile);
	if ((!line)||(sscanf( line, "SWIDTH %d %d", &wx, &wy) != 2)) {
	    bdfError(pFile,"bad 'SWIDTH'\n");
	    goto BAILOUT;
	}
	if (wy != 0) {
	    bdfError(pFile,"SWIDTH y value must be zero\n");
	    goto BAILOUT;
	}
	if (tables&FONT_SWIDTHS)  {
	    pCS->sWidth[ndx] = wx;
	}

/* 5/31/89 (ef) -- we should be able to ditch the character and recover */
/*		from all of these.					*/
	line=	bdfGetLine(pFile);
	if ((!line)||(sscanf( line, "DWIDTH %d %d", &wx, &wy) != 2)) {
	    bdfError(pFile,"bad 'DWIDTH'\n");
	    goto BAILOUT;
	}
	if (wy != 0) {
	    bdfError(pFile,"DWIDTH y value must be zero\n");
	    goto BAILOUT;
	}

	line=	bdfGetLine(pFile);
	if ((!line)||(sscanf(line,"BBX %d %d %d %d", &bw, &bh, &bl, &bb)!= 4)) {
	    bdfError(pFile,"bad 'BBX'\n");
	    goto BAILOUT;
	}
	if ((bh < 0) || (bw < 0)) {
	    bdfError(pFile,"character '%s' has a negative sized bitmap, %dx%d\n", 
	    	 					 charName, bw, bh);
	    goto BAILOUT;
	}

	line=	bdfGetLine(pFile);
	if ((line)&&(bdfIsPrefix(line, "ATTRIBUTES"))) {
	    for (p = line+strlen("ATTRIBUTES ");
		(*p == ' ') || (*p == '\t');
		p ++)
		/* empty for loop */ ;
	    if (tables&FONT_METRICS) 
		ci->metrics.attributes = bdfHexByte(p)<< 8 + bdfHexByte(p+2);
	    line= bdfGetLine(pFile);
	}
	else if (tables&FONT_METRICS)
	    ci->metrics.attributes = 0;

	if ((!line)||(!bdfIsPrefix(line, "BITMAP"))) {
	    bdfError(pFile,"missing 'BITMAP'\n");
	    goto BAILOUT;
	}


	/* collect data for generated properties */
	pState=	bdfPrivate(pFile);
	if ((strlen(charName) == 1)){
	    if ((charName[0] >='0') && (charName[0] <= '9')) {
		pState->digitWidths += wx;
		pState->digitCount++;
	    } else if (charName[0] == 'x') {
	        pState->exHeight = (bh+bb)<=0? bh : bh+bb ;
	    }
	}

	ci->metrics.leftSideBearing = bl;
	ci->metrics.rightSideBearing = bl+bw;
	ci->metrics.ascent = bh+bb;
	ci->metrics.descent = -bb;
	ci->metrics.characterWidth = wx;
	ci->pPriv=	NULL;

	if (tables&FONT_BITMAPS) 
	    bdfReadBitmap(ci,&pCS->pBitOffsets[ndx],pFile,params,
							pCS->bitmapsSizes);
	else 
	    bdfSkipBitmap(pFile,bh);

	/*\
	 * If we're loading FONT_METRICS, ci points into pCS->ci.pCI
	 * and needs to be incremented to point the the next CharInfo.
	 * if not, it's a (static) local variable and doesn't need to
	 * be incremented.   Did I hear someone scream "Hack?"
	\*/
	if (tables&FONT_METRICS) {
	    ci++;
	}

	line=	bdfGetLine( pFile );	/* get STARTCHAR or ENDFONT */
    }

    if (ndx!=nchars) {
        bdfError(pFile,"%d too few characters\n", nchars-ndx);
	goto BAILOUT;
    }
    if ((line)&&(bdfIsPrefix(line, "STARTCHAR"))) {
	bdfError(pFile,"more characters than specified\n");
	goto BAILOUT;
    }
    if ((!line)||(!bdfIsPrefix(line, "ENDFONT"))) {
        bdfError(pFile,"missing 'ENDFONT'\n");
	goto BAILOUT;
    }

    pCS->tables|= 
	tables&(FONT_METRICS|FONT_SWIDTHS|FONT_GLYPH_NAMES|FONT_BITMAPS);
    if (tables&FONT_BDF_ENCODINGS) {
	if (numEncodedGlyphs == 0)
	    bdfWarning(pFile,"No characters with valid encodings\n");

	pFont->ppCI = (CharInfoPtr *)fosCalloc(n2dChars(pFont),
						sizeof(CharInfoPtr));
	if (pFont->ppCI==NULL) {
	    bdfWarning(pFile,"Couldn't allocate ppCI (%d,%d)\n",
						n2dChars(pFont),
						sizeof(CharInfoPtr));
	    return(TRUE);
	}
	i = 0;
	for (char_row = pFont->firstRow; char_row <= pFont->lastRow; char_row++)
	{
	    if (bdfEncoding[char_row] == (CharInfoPtr *)NULL) {
	 	for (char_col=pFont->firstCol;char_col<=pFont->lastCol;
							char_col++) {
		    pFont->ppCI[i++] = NullCharInfo;
		}
	    }
	    else {
		for (char_col=pFont->firstCol;char_col<=pFont->lastCol;
							char_col++) {
		    pFont->ppCI[i++] = bdfEncoding[char_row][char_col];
		}
		fosFree(bdfEncoding[char_row]);
		bdfEncoding[char_row]=	NULL;
	    }
	}
    }
    return(TRUE);
BAILOUT:
    for (i=0;i<INDICES;i++) {
	if (bdfEncoding[i]!=NULL) {
	    fosFree(bdfEncoding[i]);
	    bdfEncoding[i]= NULL;
	}
    }
    for (i=0;i<pCS->nChars;i++) {
	if (pCS->ci.pCI[i].pPriv) {
	    fosFree(pCS->ci.pCI[i].pPriv);
	    pCS->ci.pCI[i].pPriv= NULL;
	}
    }
    fosFree(pCS->ci.pCI);
    fosFree(pCS->sWidth);
    pCS->ci.pCI=	NULL;
    pCS->sWidth=	NULL;
    return(FALSE);
}

/***====================================================================***/

Bool
bdfReadHeader(pFile)
fosFilePtr	 pFile;
{
bdfFileState	*pState=	bdfPrivate(pFile);
unsigned char	*line;
char		namebuf[BUFSIZ];
int		tmp;

    line=	bdfGetLine(pFile);
    if ((!line)||(sscanf(line, "STARTFONT %s", namebuf) != 1) ||
		 (!bdfStrEqual(namebuf, "2.1"))) {
	bdfError(pFile,"bad 'STARTFONT'\n");
	return(FALSE);
    }

    line=	bdfGetLine(pFile);
    if ((!line)||(sscanf(line, "FONT %[^\n]", pState->fontName) != 1)) {
	bdfError(pFile,"bad 'FONT'\n");
	return(FALSE);
    }

    line=	bdfGetLine(pFile);
    if ((!line)||(!bdfIsPrefix(line, "SIZE"))) {
	bdfError(pFile,"missing 'SIZE'\n");
	return(FALSE);
    }

    if ((sscanf(line, "SIZE %f%d%d", &pState->pointSize, 
					&pState->resolution, &tmp) != 3)) {
	bdfError(pFile,"bad 'SIZE'\n");
	return(FALSE);
    }
    if ((pState->pointSize < 1)||(pState->resolution<1)||(tmp<1)) {
	bdfError(pFile,"SIZE values must be > 0\n");
	return(FALSE);
    }
    if (pState->resolution != tmp) {
        bdfError(pFile,"x and y resolution must be equal\n");
	return(FALSE);
    }

    line=	bdfGetLine(pFile);
    if ((!line)||(!bdfIsPrefix(line, "FONTBOUNDINGBOX"))) {
	bdfError(pFile,"missing 'FONTBOUNDINGBOX'\n");
	return(FALSE);
    }
    return(TRUE);
}

/***====================================================================***/

Bool
bdfSkipProperties(pFile)
fosFilePtr	 pFile;
{
int		 nProps;
unsigned char	 *line;

    line=	bdfGetLine(pFile);
    if ((!line)||(!bdfIsPrefix(line, "STARTPROPERTIES"))) {
	bdfError(pFile,"missing 'STARTPROPERTIES'\n");
	return(FALSE);
    }
    if (sscanf(line, "STARTPROPERTIES %d", &nProps) != 1) {
	bdfError(pFile,"bad 'STARTPROPERTIES'\n");
	return(FALSE);
    }

    while (nProps-- > 0) {
	line=	bdfGetLine(pFile);
	if ((!line)||(bdfIsPrefix(line, "ENDPROPERTIES"))) {
	    bdfError(pFile,"%d too few properites\n", nProps + 1);
	    return(FALSE);
	}
    }
    line=	bdfGetLine(pFile);
    if ((!line)||(!bdfIsPrefix(line, "ENDPROPERTIES"))) {
	bdfError(pFile,"missing 'ENDPROPERTIES'\n");
	return(FALSE);
    }
    return(TRUE);
}

/***====================================================================***/

Bool
bdfReadProperties(pFile, pFont, pCS)
fosFilePtr	 pFile;
EncodedFontPtr	 pFont;
CharSetPtr	 pCS;
{
int		 nProps, nextProp;
Bool		*stringProps;
FontPropPtr	 props;
char		 namebuf[BUFSIZ],secondbuf[BUFSIZ], thirdbuf[BUFSIZ];
unsigned char	*line;
bdfFileState	*pState;

    line=	bdfGetLine(pFile);
    if ((!line)||(!bdfIsPrefix(line, "STARTPROPERTIES"))) {
	bdfError(pFile,"missing 'STARTPROPERTIES'\n");
	return(FALSE);
    }
    if (sscanf(line, "STARTPROPERTIES %d", &nProps) != 1) {
	bdfError(pFile,"bad 'STARTPROPERTIES'\n");
	return(FALSE);
    }

    pCS->isStringProp=	NULL;
    pCS->props=		NULL;

    stringProps = (Bool *)fosCalloc((nProps+BDF_GENPROPS),sizeof(Bool));
    pCS->isStringProp=	stringProps;
    if (stringProps==NULL) {
	bdfWarning(pFile,"Couldn't allocate stringProps (%d*%d)\n",
					(nProps+BDF_GENPROPS),sizeof(Bool));
	bdfInformation(pFile,"Font properties not loaded\n");
	goto BAILOUT;
    }

    pCS->props= props=	(FontPropPtr)fosCalloc((nProps+BDF_GENPROPS),
							sizeof(FontPropRec));
    if (props==NULL) {
	bdfWarning(pFile,"Couldn't allocate props (%d*%d)\n", (nProps+BDF_GENPROPS,
							sizeof(FontPropRec)));
	bdfInformation(pFile,"Font properties not loaded\n");
	fosFree(stringProps);
	pCS->isStringProp=	stringProps;
	goto BAILOUT;
    }

    nextProp=	0;
    while (nProps-- > 0) {
	line=	bdfGetLine(pFile);
	if ((line==NULL)||(bdfIsPrefix(line, "ENDPROPERTIES"))) {
	    bdfError(pFile,"%d too few properites\n", nProps + 1);
	    goto BAILOUT;
	}
	
	/* 9/1/89 (ef) -- hack, hack, hack. skip leading whitespace */
	while ((*line)&&(isspace(*line)))	line++;
	switch (sscanf(line, "%s%s%s", namebuf, secondbuf, thirdbuf) ) {
	    case 1: /* missing required parameter value */
		bdfError(pFile,"missing '%s' parameter value\n", namebuf);
		goto BAILOUT;
		break;

 	    case 2:
		/*
		 * Possibilites include:
		 * valid quoted string with no white space
	         * valid integer value 
		 * invalid value
		 */
		if( secondbuf[0] == '"'){
		    stringProps[nextProp]= TRUE;
		    props[nextProp].value= 
				bdfGetPropertyValue(line+strlen(namebuf)+1);
		    break;
		} 
		else if( bdfIsInteger(secondbuf) ){
		    stringProps[nextProp]=	FALSE;
		    props[nextProp].value=	atoi(secondbuf);
		    break;
		}
		else {
		    bdfError(pFile,"invalid '%s' parameter value\n", namebuf);
		    goto BAILOUT;
		}

	    case 3:
		/* 
		 * Possibilites include:
		 * valid quoted string with some white space
		 * invalid value (reject even if second string is integer)
		 */
		if( secondbuf[0] == '"'){
		    stringProps[nextProp]= TRUE;
		    props[nextProp].value= 
				bdfGetPropertyValue( line+strlen(namebuf)+1);
		    break;
		}
		else {
		    bdfError(pFile,"invalid '%s' parameter value\n",namebuf);
		    goto BAILOUT;
		}
	}
	props[nextProp].name=	bdfForceMakeAtom(namebuf, NULL);
	if (props[nextProp].name == None) {
	    bdfError(pFile,"Empty property name.\n");
	    goto BAILOUT;
	}
	if (!bdfSpecialProperty(pFont, pCS, nextProp, bdfPrivate(pFile)))
	    nextProp++;
    }

    line=	bdfGetLine(pFile);
    if (!bdfIsPrefix(line, "ENDPROPERTIES")) {
	bdfError(pFile,"missing 'ENDPROPERTIES'\n");
	goto BAILOUT;
    }

    pState=	bdfPrivate(pFile);
    if (!pState->haveFontAscent || !pState->haveFontDescent) {
	bdfError(pFile,"missing 'FONT_ASCENT' or 'FONT_DESCENT' properties\n");
	goto BAILOUT;
    }

    if (!pState->pointSizeProp) {
	props[nextProp].name=	bdfForceMakeAtom("POINT_SIZE", NULL);
	props[nextProp].value=	(INT32)(pState->pointSize*10.0);
	stringProps[nextProp]=	FALSE;
	pState->pointSizeProp=	&props[nextProp];
	nextProp++;
    }
    if (!pState->fontProp) {
	props[nextProp].name=	bdfForceMakeAtom("FONT", NULL);
	props[nextProp].value=	(INT32)bdfForceMakeAtom(pState->fontName, NULL);
	stringProps[nextProp]=	TRUE;
	pState->fontProp=	&props[nextProp];
	nextProp++;
    }
    if (!pState->weightProp) {
	props[nextProp].name=	bdfForceMakeAtom( "WEIGHT", NULL );
	props[nextProp].value=	-1;	/* computed later */
	stringProps[nextProp]=	FALSE;
	pState->weightProp=	&props[nextProp];
	nextProp++;
    }
    if (!pState->resolutionProp) {
	props[nextProp].name=	bdfForceMakeAtom( "RESOLUTION", NULL );
	props[nextProp].value=	(INT32)((pState->resolution*100.0)/72.27);
	stringProps[nextProp]=	FALSE;
	pState->resolutionProp=	&props[nextProp];
	nextProp++;
    }
    if (!pState->xHeightProp) {
	props[nextProp].name=	bdfForceMakeAtom( "X_HEIGHT", NULL );
	props[nextProp].value= 	-1;	/* computed later */
	stringProps[nextProp]= 	FALSE;
	pState->xHeightProp= 	&props[nextProp];
	nextProp++;
    }
    if (!pState->quadWidthProp) {
	props[nextProp].name=	bdfForceMakeAtom( "QUAD_WIDTH", NULL );
	props[nextProp].value=	-1;	/* computed later */
	stringProps[nextProp]=	FALSE;
	pState->quadWidthProp=	&props[nextProp];
	nextProp++;
    }
    pCS->nProps = nextProp;
    pCS->tables |= FONT_PROPERTIES;
    return(TRUE);
BAILOUT:
    if (pCS->isStringProp)	fosFree(pCS->isStringProp);
    if (pCS->props)		fosFree(pCS->props);
    pCS->isStringProp=	NULL;
    pCS->props=		NULL;
    while ((line)&&(!bdfIsPrefix(line, "ENDPROPERTIES"))) {
	line=	bdfGetLine(pFile);
    }
    return(FALSE);
}

/***====================================================================***/

int
bdfReadFont(input,ppFont,ppCS,tables,params)
fosFilePtr	 input;
EncodedFontPtr	*ppFont;
CharSetPtr	*ppCS;
Mask		 tables;
BuildParamsPtr	 params;
{
EncodedFontPtr	pFont;
CharSetPtr	pCS;
int		i;
bdfFileState	state;
int		freePCS=	FALSE;
int		freePFont=	FALSE;
static	int	been_here=	0;

    /* 5/31/89 (ef) -- there has to be a better way.  sigh. */
    if (!been_here) {
	BDFA_FONT_ASCENT=	bdfForceMakeAtom("FONT_ASCENT", NULL);
	BDFA_FONT_DESCENT=	bdfForceMakeAtom("FONT_DESCENT", NULL);
	BDFA_DEFAULT_CHAR=	bdfForceMakeAtom("FONT_CHAR", NULL);
    }
    tables&= FONT_LEGAL_TABLES;
    if (!tables)
	return(0);

    if (ppFont&&(*ppFont))	pFont=	*ppFont;
    else			pFont=	NULL;

    if ((tables&FONT_BDF_ENCODINGS)&&(!pFont)) {
	pFont= (EncodedFontPtr)fosAlloc(sizeof(EncodedFontRec));
	freePFont= TRUE;
	/* 6/8/89 (ef) -- deal with allocation failure */
	bzero(pFont,sizeof(EncodedFontRec));

	pFont->firstRow=	pFont->firstCol = INDICES - 1;
	pFont->defaultCh=	NO_SUCH_CHAR;
    }

    if ((!ppCS)||(!*ppCS)) {
	if ((pFont)&&(pFont->pCS)) {
	    pCS= pFont->pCS;
	}
	else {
	    pCS= (CharSetPtr)fosAlloc(sizeof(CharSetRec));
	    /* 6/8/89 (ef) -- deal with allocation failure */
	    bzero(pCS,sizeof(CharSetRec));
	    if (pFont) {
		pFont->pCS=	pCS;
		pCS->refcnt=	1;
	    }
	    freePCS= TRUE;

	    pCS->pixDepth=	1;
	    pCS->glyphSets=	1;
	    pCS->drawDirection=	FontLeftToRight;
	    for (i = 0; i < GLYPHPADOPTIONS; i++) {
		pCS->bitmapsSizes[i] = 0;
	    }
	}
    }
    else {
	pCS=	*ppCS;
    }
    tables&= (~pCS->tables);

    bzero(&state,sizeof(bdfFileState));
    state.fileName=	(char *)input->fmtPrivate;
    input->fmtPrivate=	(pointer)&state;

    if (!bdfReadHeader(input))					goto BAILOUT;
    if ( tables & FONT_PROPERTIES ) {
	if (!bdfReadProperties(input, pFont, pCS))	goto BAILOUT;
    }
    else bdfSkipProperties( input );

    if ((tables)&&(!bdfReadCharacters(input, pFont, pCS, params, tables)))
	goto BAILOUT;

    if ((pFont)&&(tables&FONT_ACCELERATORS)) {
	ComputeFontBounds(pCS);
	ComputeInfoAccelerators(
	    pFont, params->makeTEfonts, params->inhibitInk, params->glyphPad);
	ComputeFontAccelerators(pFont);
	pCS->tables |= FONT_ACCELERATORS;
    }

    /* generate properties */
    if (tables&FONT_PROPERTIES) {
	if (state.xHeightProp && (state.xHeightProp->value == -1))
	    state.xHeightProp->value=	(state.exHeight ?
						state.exHeight :
						pCS->minbounds.ascent);

	if (state.quadWidthProp && (state.quadWidthProp->value == -1))
	    state.quadWidthProp->value=	state.digitCount ?
		(INT32)((float)state.digitWidths/(float)state.digitCount) :
		(pCS->minbounds.characterWidth+pCS->maxbounds.characterWidth)/2;

	if (state.weightProp && (state.weightProp->value == -1))
	    state.weightProp->value=	fontComputeWeight(pFont, params);
    }

    if (ppFont)	*ppFont=	pFont;
    if (ppCS)	*ppCS=	 	pCS;

    if (pFont && pFont->ppCI) 
	tables&=	(~FONT_BDF_ENCODINGS);

    /* if there *are* no ink metrics, and we wanted them, they're as good */
    /* as read (as read as they're going to get)                          */
    if ((!(pCS->tables&FONT_INK_METRICS)&&(!pCS->inkMetrics))) {
	tables&= (~FONT_INK_METRICS);
    }
    input->fmtPrivate=	NULL;

    return (tables&(~pCS->tables));
BAILOUT:
    if (freePCS) {
	bzero(pCS,sizeof(CharSetRec));
   	fosFree(pCS);
    }
    if (freePFont) {
	bzero(pFont,sizeof(EncodedFontRec));
	fosFree(pFont);
    }
    input->fmtPrivate=	NULL;
    return(tables);
}
