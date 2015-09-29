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

/* $Header: fontutil.c,v 5.7 89/09/11 15:12:01 erik Exp $ */

#include <X11/X.h>
#include <X11/keysym.h>

#include "fontos.h"
#include "fontstr.h"
#include "fontlib.h"
#include "font.h"

/***====================================================================***/

char *
fontTableName(table)
Mask	table;
{
static char	buf[30];
    if (table!=lowbit(table)) {
	sprintf(buf,"bad table 0x%x\n",table);
	return(buf);
    }
    switch (table) {
	case FONT_PROPERTIES:		return("properties");
	case FONT_ACCELERATORS:		return("accelerators");
	case FONT_METRICS:		return("metrics");
	case FONT_BITMAPS:		return("bitmaps");
	case FONT_INK_METRICS:		return("ink metrics");
	case FONT_BDF_ENCODINGS:	return("encoding");
	case FONT_SWIDTHS:		return("scalable widths");
	case FONT_GLYPH_NAMES:		return("glyph names");
    }
    return("unknown");
}
/***====================================================================***/

/*
 * ISO Latin-1 case conversion routine
 *
 * this routine always null-terminates the result, so
 * beware of too-small buffers
 *
 * stolen from dixutils.c
 */

void
fosCopyISOLatin1Lowered(dest, source, length)
    register unsigned char *dest, *source;
    int length;
{
    register int i;

    for (i = 0; i < length; i++, source++, dest++)
    {
	if ((*source >= XK_A) && (*source <= XK_Z))
	    *dest = *source + (XK_a - XK_A);
	else if ((*source >= XK_Agrave) && (*source <= XK_Odiaeresis))
	    *dest = *source + (XK_agrave - XK_Agrave);
	else if ((*source >= XK_Ooblique) && (*source <= XK_Thorn))
	    *dest = *source + (XK_oslash - XK_Ooblique);
	else
	    *dest = *source;
    }
    *dest = '\0';
}

/***====================================================================***/

unsigned char _reverse_byte[0x100] = {
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

/*
 *	Invert bit order within each BYTE of an array.
 */
void
BitOrderInvert(buf, nbytes)
    register unsigned char *buf;
    register int nbytes;
{
    register unsigned char *rev = _reverse_byte;
    for (; --nbytes >= 0; buf++)
	*buf = rev[*buf];
}

/*
 *	Invert byte order within each 16-bits of an array.
 */
void
TwoByteInvert(buf, nbytes)
    register unsigned char *buf;
    register int nbytes;
{
    register unsigned char c;

    for (; nbytes > 0; nbytes -= 2, buf += 2) {
	c = *buf;
	*buf = *(buf+1);
	*(buf+1) = c;
    }
}

/*
 *	Invert byte order within each 32-bits of an array.
 */
void
FourByteInvert(buf, nbytes)
    register unsigned char *buf;
    register int nbytes;
{
    register unsigned char c;

    for (; nbytes > 0; nbytes -= 4, buf += 4) {
	c = *buf;
	*buf = *(buf+3);
	*(buf+3) = c;
	c = *(buf+1);
	*(buf+1) = *(buf+2);
	*(buf+2) = c;
    }
}

/*
 *	Repad a bitmap
 */

int
RepadBitmap(pSrc,pDest,srcPad,destPad,width,height)
char		*pSrc,*pDest;
unsigned	 srcPad,destPad;
int		 width,height;
{
int	srcWidthBytes,destWidthBytes;
int	row,col;
char	*pTmpSrc,*pTmpDest;

    switch (srcPad) {
	case 1:	srcWidthBytes=	(width+7)>>3; break;
	case 2:	srcWidthBytes=	((width+15)>>4)<<1; break;
	case 4:	srcWidthBytes=	((width+31)>>5)<<2; break;
	case 8:	srcWidthBytes=	((width+63)>>6)<<3; break;
	default:    fosError("repad bitmap called with illegal srcPad (%d)\n",
							srcPad);
    		    return(0);
    }
    switch (destPad) {
	case 1:	destWidthBytes=	(width+7)>>3; break;
	case 2:	destWidthBytes=	((width+15)>>4)<<1; break;
	case 4:	destWidthBytes=	((width+31)>>5)<<2; break;
	case 8:	destWidthBytes=	((width+63)>>6)<<3; break;
	default:    fosError("repad bitmap called with illegal destPad (%d)\n",
							destPad);
    		    return(0);
    }

    width= MIN(srcWidthBytes,destWidthBytes);
    pTmpSrc= pSrc;	pTmpDest= pDest;
    for (row=0;row<height;row++) {
	for (col=0;col<width;col++) {
	    *pTmpDest++ = *pTmpSrc++;
	}
	pTmpDest+= destWidthBytes-width;
	pTmpSrc+=  srcWidthBytes-width;
    }
    return(destWidthBytes*height);
}

/***====================================================================***/

static unsigned char bits_per_byte[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

int
fontComputeWeight(font, params) 
register EncodedFontPtr	font;
	 BuildParamsPtr	params;
{
    int i;
    int width = 0, area, bits = 0;
    register unsigned char *p, *end;
    register CharInfoPtr	ci;

    if ((!(font->pCS->tables&(FONT_METRICS|FONT_BITMAPS)))||
				(font->pCS->pBitOffsets==NULL)) {
	return(0);
    }

    ci = font->pCS->ci.pCI;
    for (i = 0; i < font->pCS->nChars; i++, ci++) {
	width += ci->metrics.characterWidth;
	p=	(unsigned char *)font->pCS->pBitOffsets[i];
	end=	(unsigned char *)p+BYTES_FOR_GLYPH(ci,params->glyphPad);
	for (; p < end; p++)
	    bits += bits_per_byte[*p];
    }
    area = width*(font->pCS->fontAscent+font->pCS->fontDescent);
    if (area == 0) return 0;
    return (int)((bits*1000.0)/area);
}

/***====================================================================***/

/* text support routines. A charinfo array builder, and a bounding */
/* box calculator */

void
GetGlyphs(font, count, chars, fontEncoding, glyphcount, glyphs)
EncodedFontPtr		 font;
unsigned long		 count;
register unsigned char	*chars;
FontEncoding 		 fontEncoding;
unsigned long 		*glyphcount;	/* RETURN */
CharInfoPtr		 glyphs[];	/* RETURN */
{
CharInfoPtr		*ppCI = font->ppCI;
unsigned int		 firstCol = font->firstCol;
register unsigned int	 numCols = font->lastCol - firstCol + 1;
unsigned int		 firstRow = font->firstRow;
unsigned int		 numRows = font->lastRow - firstRow + 1;
CharInfoPtr		 pCDef = font->pDefChar;
register unsigned long	 i;
register unsigned long	 n;
register unsigned int	 c;
register CharInfoPtr	 pci;

    n = 0;
    switch (fontEncoding) {

	case Linear8Bit:
	case TwoD8Bit:
	    if (font->allExist && (pCDef != NullCharInfo)) {
		for (i=0; i < count; i++) {

		    c = (*chars++) - firstCol;
		    if (c >= numCols) {
			glyphs[i] = pCDef;
		    }
		    else glyphs[i] = ppCI[c];
		}
		n = count;
	    } else {
		for (i=0; i < count; i++) {
    
		    c = (*chars++) - firstCol;
		    if (c < numCols) {
			pci = ppCI[c];
			if (pci != NullCharInfo ) {glyphs[n++] = pci; continue;}
		    }
    
		    if (pCDef != NullCharInfo)
			glyphs[n++] = pCDef;
		}
	    }
	    break;

	case Linear16Bit:
	    if (font->allExist && (pCDef!=NullCharInfo)) {
		for (i=0; i < count; i++) {

		    c = *chars++ << 8;
		    c = (c | *chars++) - firstCol;
		    if (c >= numCols) {
			glyphs[i] = pCDef;
		    }
		    else
			glyphs[i] = ppCI[c];
		}
		n = count;
	    } else {
		for (i=0; i < count; i++) {
    
		    c = *chars++ << 8;
		    c = (c | *chars++) - firstCol;
		    if (c < numCols) {
			pci = ppCI[c];
			if (pci != NullCharInfo) {glyphs[n++] = pci; continue;}
		    }
    
		    if (pCDef != NullCharInfo)
			glyphs[n++] = pCDef;
		}
	    }
	    break;

	case TwoD16Bit:
	    for (i=0; i < count; i++) {
		register unsigned int row;
		register unsigned int col;

		row = (*chars++) - firstRow;
		col = (*chars++) - firstCol;
		if ((row < numRows) && (col < numCols)) {
		    c = row*numCols + col;
		    pci = ppCI[c];
		    if (pci != NullCharInfo) {glyphs[n++] = pci; continue;}
		}

		if (pCDef!=NullCharInfo)
		    glyphs[n++] = pCDef;
	    }
	    break;
    }
    *glyphcount = n;
}


void
QueryGlyphExtents(font, charinfo, count, info)
    EncodedFontPtr font;
    CharInfoPtr *charinfo;
    unsigned long count;
    register ExtentInfoRec *info;
{
    register CharInfoPtr *ppCI= charinfo;
    register unsigned long i;

    info->drawDirection = font->pCS->drawDirection;

    info->fontAscent = font->pCS->fontAscent;
    info->fontDescent = font->pCS->fontDescent;

    if (count != 0) {

	info->overallAscent  = (*ppCI)->metrics.ascent;
	info->overallDescent = (*ppCI)->metrics.descent;
	info->overallLeft    = (*ppCI)->metrics.leftSideBearing;
	info->overallRight   = (*ppCI)->metrics.rightSideBearing;
	info->overallWidth   = (*ppCI)->metrics.characterWidth;

	if (font->pCS->constantMetrics && font->pCS->noOverlap) {
	    info->overallWidth *= count;
	    info->overallRight += (info->overallWidth -
				   (*ppCI)->metrics.characterWidth);
	    return;
	}
	for (i = count, ppCI++; --i != 0; ppCI++) {
	    info->overallAscent = MAX(
	        info->overallAscent,
		(*ppCI)->metrics.ascent);
	    info->overallDescent = MAX(
	        info->overallDescent,
		(*ppCI)->metrics.descent);
	    info->overallLeft = MIN(
		info->overallLeft,
		info->overallWidth+(*ppCI)->metrics.leftSideBearing);
	    info->overallRight = MAX(
		info->overallRight,
		info->overallWidth+(*ppCI)->metrics.rightSideBearing);
	    /* yes, this order is correct; overallWidth IS incremented last */
	    info->overallWidth += (*ppCI)->metrics.characterWidth;
	}

    } else {

	info->overallAscent  = 0;
	info->overallDescent = 0;
	info->overallWidth   = 0;
	info->overallLeft    = 0;
	info->overallRight   = 0;

    }
}

#ifndef X11R4

void
QueryTextExtents(font, count, chars, info)
EncodedFontPtr	 font;
unsigned long	 count;
unsigned char	*chars;
ExtentInfoRec	*info;
{
CharInfoPtr	*ppCI= (CharInfoPtr *)fosTmpAlloc(count*sizeof(CharInfoPtr));
unsigned long	 n;
CharInfoPtr	*ppOldCI;

    if(!ppCI)
	return;
    ppOldCI=	font->ppCI;
    font->ppCI=	font->ppInkCI;
    /* temporarily stuff in Ink metrics */
    if (font->lastRow == 0)
	GetGlyphs(font, count, chars, Linear16Bit, &n, ppCI);
    else
	GetGlyphs(font, count, chars, TwoD16Bit, &n, ppCI);
    /* restore real glyph metrics */
    font->ppCI=	ppOldCI;

    QueryGlyphExtents(font, ppCI, n, info);

    fosTmpFree(ppCI);
    return;
}
#else /* X11R4 */
Bool
QueryTextExtents(font, count, chars, info)
    EncodedFontPtr font;
    unsigned long count;
    unsigned char *chars;
    ExtentInfoRec *info;
{
    CharInfoPtr *charinfo;
    unsigned long n;
    CharInfoPtr	*ppOldCI;
    Bool oldCM;

    charinfo = (CharInfoPtr *)fosTmpAlloc(count*sizeof(CharInfoPtr));
    if(!charinfo)
	return FALSE;
    ppOldCI = font->ppCI;
    /* kludge, temporarily stuff in Ink metrics */
    font->ppCI = font->ppInkCI;
    if (font->lastRow == 0)
	GetGlyphs(font, count, chars, Linear16Bit, &n, charinfo);
    else
	GetGlyphs(font, count, chars, TwoD16Bit, &n, charinfo);
    /* restore real glyph metrics */
    font->ppCI = ppOldCI;
    oldCM = font->pCS->constantMetrics;
    /* kludge, ignore bitmap metric flag */
    font->pCS->constantMetrics = FALSE;
    QueryGlyphExtents(font, charinfo, n, info);
    /* restore bitmap metric flag */
    font->pCS->constantMetrics = oldCM;
    fosTmpFree(charinfo);
    return TRUE;
}
#endif

/***====================================================================***/

#ifdef DEBUG
void
debugGlyph(msg,pCS,fmt,bitmap)
char		*msg;
CharSetPtr	pCS;
unsigned	fmt;
unsigned char	*bitmap;
{
int  width= pCS->ci.pCI[0].metrics.rightSideBearing-
		pCS->ci.pCI[0].metrics.leftSideBearing;
int  height= pCS->ci.pCI[0].metrics.ascent+
		pCS->ci.pCI[0].metrics.descent;
int	row,col;

    printf("%s\n",msg);
    printf("width/height= %d/%d ",width,height);
    switch (FMT_GLYPH_PAD((fmt))) {
	case 1: width= (width+7)>>3; break;
	case 2: width= ((width+15)>>4)<<1; break;
	case 4: width= ((width+31)>>5)<<2; break;
	case 8: width= ((width+63)>>6)<<3; break;
    }
    printf("(%d/%d)\n",width,height);

    printf("format: bit: %s, byte: %s,  pad: %d, unit %d\n",
		(FMT_BIT_ORDER(fmt)==MSBFirst?"MSB":"LSB"),
		(FMT_BYTE_ORDER(fmt)==MSBFirst?"MSB":"LSB"),
		FMT_GLYPH_PAD(fmt),FMT_SCAN_UNIT(fmt));
    for (row=0;row<height;row++) {
	for (col=0;col<width;col++)
	    printf("%2.2x",*bitmap++);
	printf("\n");
    }
}

#endif /* DEBUG */
