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

/* $Header: fontdump.c,v 5.6 89/08/11 14:29:06 erik Exp $ */

#include <stdio.h>
#include <X11/X.h>
#include "fontstr.h"
#include "fontlib.h"

static int msb[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
static int lsb[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/* XXX
 * DumpBitmaps should be smarter. It does not handle the case when
 * params->bitOrder != params->byteOrder.
 */

void
DumpBitmaps(pFont, params)
    EncodedFontPtr pFont;
    BuildParamsPtr params;
{
    int			ch;	/* current character */
    int			r;	/* current row */
    int			b;	/* current bit in the row */
    CharInfoPtr		pCI = pFont->pCS->ci.pCI;
    int			*masks;	/* one of msb or lsb depending upon params */

    masks = (params->bitOrder == MSBFirst ? msb : lsb);

    for (ch = 0; ch < pFont->pCS->nChars; ch++, pCI++)
    {
	int bpr = BYTES_PER_ROW(GLYPHWIDTHPIXELS(pCI), params->glyphPad);

	printf("\t\t%s\n", NameForAtom(pFont->pCS->glyphNames[ch]));
        for (r=0; r <  pCI->metrics.descent + pCI->metrics.ascent; r++)
        {
	    unsigned char *row;
	    row= ((unsigned char *)pFont->pCS->pBitOffsets[ch]) + (r*bpr);
            for ( b=0;
		b < pCI->metrics.rightSideBearing - pCI->metrics.leftSideBearing;
		b++) {
			if ((r == pCI->metrics.ascent) &&
			    (b == -pCI->metrics.leftSideBearing))
			    putchar(((row[b>>3] & masks[b&7]) ? '*' : '.'));
			else
			    putchar(((row[b>>3] & masks[b&7]) ? '#' : '-'));
	    }
            putchar('\n');
        }
    }
}
static void
DumpCharInfo(ci, name)
    Atom	name;
    xCharInfo	*ci;
{
    if (name != None) {
	printf("\t\t");
    }
    printf ("rbearing\tdescent\t\twidth\t\n");
    if (name != None)
	printf("glyph name\t");
    printf("\tlbearing\tascent\t   attributes\n");

    if (name != None)
	printf("%-15s\t", NameForAtom (name));
    printf ("%4d\t%4d\t%4d\t%4d\t%4d\t0x%x\n",
	    ci->rightSideBearing, ci->leftSideBearing, ci->descent,
	    ci ->ascent, ci->characterWidth, ci->attributes);
}

void
DumpFont(pFont, params, dumpLevel)
    EncodedFontPtr	pFont;
    BuildParamsPtr params;
    DumpLevel	dumpLevel;
{
    CharSetPtr	pCS = pFont->pCS;
    FontPropPtr props = pCS->props;
    int i;

    if ( pFont == NULL)
    {
	fosFatalError("DumpFont: NULL FONT pointer passed\n");
    }
    if ( pCS == NULL)
    {
	fosFatalError("DumpFont: NULL CharSet pointer passed\n");
    }

    if ( pCS->noOverlap)
	printf( " noOverlap");
    if ( pCS->constantMetrics)
	printf( " constantMetrics");
    if ( pCS->terminalFont)
	printf( " terminalFont");
    if ( pFont->linear)
	printf( " linear");
    if ( pCS->constantWidth)
	printf( " constantWidth");
    if ( pCS->inkInside)
	printf( " inkInside");
    if ( pCS->inkMetrics)
	printf( " inkMetrics");
    printf("\nprinting direction: ");
    switch ( pCS->drawDirection)
    {
      case FontLeftToRight:
	printf( "left-to-right\n");
	break;
      case FontRightToLeft:
	printf( "right-to-left\n");
	break;
    }
    printf("first character: 0x%x\n", pFont->firstCol);
    printf("last character:  0x%x\n", pFont->lastCol);

    printf("number of font properties:  %d\n", pCS->nProps);
    for (i=0; i<pCS->nProps; i++) {
	printf("  %-15s  ", NameForAtom(props[i].name));
	if (pFont->pCS->isStringProp[i])
		printf("%s\n", NameForAtom(props[i].value));
	else
		printf("%d\n", props[i].value);
    }

    printf("default character: 0x%x\n", pFont->defaultCh);
    printf("font descent: %d\n", pFont->pCS->fontDescent);
    printf("font ascent: %d\n", pFont->pCS->fontAscent);

    printf("minbounds:\n");
    DumpCharInfo(&pCS->minbounds, None);
    printf("maxbounds:\n");
    DumpCharInfo(&pCS->maxbounds, None);
    if (pFont->pCS->inkMetrics) {
	printf("ink minbounds:\n");
	DumpCharInfo(&pFont->inkMin, None);
	printf("ink maxbounds:\n");
	DumpCharInfo(&pFont->inkMax, None);
    }
    printf("CharSet struct: virtual memory address == %x\tsize == %x\n",
	    pFont->pCS, sizeof(CharSetRec));

    printf("CharInfo array: virtual memory base address == %x\tsize == %x\n",
	    pCS->ci.pCI, sizeof(CharInfoRec)*pCS->nChars);

    if (pCS->inkMetrics)
	printf("ink CharInfo array: virtual memory base address == %x\tsize == %x\n",
	    pCS->inkci.pCI, sizeof(CharInfoRec)*pCS->nChars);

    switch (dumpLevel) {	/* stupid pcc will not compare enums */
	case dumpCharInfo:
	case dumpPictures:
	    for (i = 0; i < pCS->nChars; i++)
		DumpCharInfo(&pCS->ci.pCI[i].metrics, pCS->glyphNames[i]);
	    if (pFont->pCS->inkMetrics) {
		printf( "\nink metrics:\n");
		for (i = 0; i < pCS->nChars; i++)
		    DumpCharInfo(&pCS->ci.pCI[i].metrics, pCS->glyphNames[i]);
	    }
	    if (dumpLevel == dumpPictures)
		DumpBitmaps(pFont, params);
    }
    printf("\n");
}

