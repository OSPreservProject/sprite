/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
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

******************************************************************/

#include "X.h"
#include "Xproto.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "qd.h"
#include "qdgc.h"
#ifndef X11R4
#include "fontstr.h"
#endif /* X11R4 */
#include "servermd.h"
#ifdef X11R4
#include "fontstruct.h"
#include "dixfontstr.h"
#endif /* X11R4 */

void
qdImageTextPix( pDraw, pGC, x0, y0, nChars, pStr)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x0, y0;
    int		nChars;
    char *	pStr;
{
    CHECK_MOVED(pGC, pDraw);
    if (QD_PIX_DATA((PixmapPtr)pDraw) == NULL) {
	/* make dummy window and use that as the drawable */
	SETUP_PIXMAP_AS_WINDOW(pDraw, pGC);
	/*
	 * tlImageText is somewhat faster than qdImageTextKerned,
	 * but in the interest of simplicity, we just go for the more
	 * general case, without testing to see if that is needed.
	 */
	qdImageTextKerned(pDraw, pGC, x0, y0, nChars, pStr);
	CLEANUP_PIXMAP_AS_WINDOW(pGC);
    }
    else
	miImageText8( pDraw, pGC, x0, y0, nChars, pStr);
}

int
qdPolyTextPix( pDraw, pGC, x0, y0, nChars, pStr)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x0, y0;
    int 	nChars;
    char *	pStr;
{
    CHECK_MOVED(pGC, pDraw);
    if (QD_PIX_DATA((PixmapPtr)pDraw) == NULL) {
	int width;
	/* make dummy window and use that as the drawable */
	SETUP_PIXMAP_AS_WINDOW(pDraw, pGC);
	width = tlPolyText( pDraw, pGC, x0, y0, nChars, pStr);
	CLEANUP_PIXMAP_AS_WINDOW(pGC);
	return width;
    }
    else
	return miPolyText8( pDraw, pGC, x0, y0, nChars, pStr);
}

qdImageTextKerned( pDraw, pGC, x0, y0, nChars, pStr)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x0, y0;
    int 	nChars;
    char *	pStr;
{
    xRectangle rect;
    register unsigned char *p;
    register int i, w;
#ifndef X11R4
    EncodedFontPtr pFont = pGC->font;
    CharSetPtr pcs = pFont->pCS;
    int chfirst = pFont->firstCol;
    int chlast = pFont->lastCol;
    register CharInfoPtr *ppCI = pFont->ppCI - chfirst;
#else
    FontPtr pFont = pGC->font;
    FontInfoPtr pfi = pFont->pFI;
    int chfirst = pFont->pFI->firstCol;
    int chlast = pFont->pFI->lastCol;
    register CharInfoPtr pCI;
#endif /* X11R4 */
    int saveAlu = pGC->alu;
    int saveFillStyle = pGC->fillStyle;
    int saveFgPixel = pGC->fgPixel;
    pGC->fillStyle = FillSolid;

#ifdef X11R4
    pCI = &pFont->pCI[-chfirst];

#endif /* X11R4 */
    /* calculate width of string pStr */
    for (w = 0, i = nChars, p = (unsigned char*)pStr; --i >= 0; ) {
	register ch = *p++;
	if (ch < chfirst || ch > chlast) {
#ifndef X11R4
	    ch = pFont->defaultCh;
#else
	    ch = pFont->pFI->chDefault;
#endif /* X11R4 */
	    if (ch < chfirst || ch > chlast) continue;
	}
#ifndef X11R4
	w += ppCI[ch]->metrics.characterWidth;
#else
	w += pCI[ch].metrics.characterWidth;
#endif /* X11R4 */
    }

    rect.x = x0;
#ifndef X11R4
    rect.y = y0 - pcs->fontAscent;
#else
    rect.y = y0 - pfi->fontAscent;
#endif /* X11R4 */
    rect.width = w;
#ifndef X11R4
    rect.height = pcs->fontAscent + pcs->fontDescent;
#else
    rect.height = pfi->fontAscent + pfi->fontDescent;
#endif /* X11R4 */
    if (saveFillStyle != FillSolid) InstalledGC = NULL;
    pGC->alu = GXcopy;
    pGC->fgPixel = pGC->bgPixel;
    tlSolidRects(pDraw, pGC, 1, &rect);
    pGC->fgPixel = saveFgPixel;
    tlPolyText( pDraw, pGC, x0, y0, nChars, pStr);
    if (saveFillStyle != FillSolid) InstalledGC = NULL;
    pGC->alu = saveAlu;
    pGC->fillStyle = saveFillStyle;
}

void
#ifndef X11R4
qdPolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci)
#else
qdPolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
#endif /* X11R4 */
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
#ifdef X11R4
    unsigned char *pglyphBase;  /* start of array of glyphs */

#endif /* X11R4 */
{
    int width, height;
    int nbyLine;			/* bytes per line of padded pixmap */
#ifndef X11R4
    EncodedFontRec *pfont;
#else
    FontPtr pfont;
#endif /* X11R4 */
    register int i;
    register int j;
    unsigned char *pbits;		/* buffer for PutImage */
    register unsigned char *pb;		/* temp pointer into buffer */
    register CharInfoPtr pci;		/* currect char info */
    register unsigned char *pglyph;	/* pointer bits in glyph */
    int gWidth, gHeight;		/* width and height of glyph */
    register int nbyGlyphWidth;		/* bytes per scanline of glyph */
    int nbyPadGlyph;			/* server padded line of glyph */
    QDPixRec dummyPixmap[1];

    if ((pDrawable->type == DRAWABLE_WINDOW) &&
	(pGC->miTranslate))
    {
	x += pGC->lastWinOrg.x;
	y += pGC->lastWinOrg.y;
    }

    pfont = pGC->font;
#ifndef X11R4
    width = pfont->pCS->maxbounds.rightSideBearing - 
	pfont->pCS->minbounds.leftSideBearing;
    height = pfont->pCS->maxbounds.ascent + pfont->pCS->maxbounds.descent;
#else
    width = pfont->pFI->maxbounds.metrics.rightSideBearing - 
	pfont->pFI->minbounds.metrics.leftSideBearing;
    height = pfont->pFI->maxbounds.metrics.ascent +
	pfont->pFI->maxbounds.metrics.descent;
#endif /* X11R4 */

    nbyLine = PixmapBytePad(width, 1);
    pbits = (unsigned char *)ALLOCATE_LOCAL(height*nbyLine);
    if (!pbits)
        return ;

    dummyPixmap->pixmap.drawable.type = DRAWABLE_PIXMAP;
    dummyPixmap->pixmap.drawable.depth = 1;
    dummyPixmap->pixmap.drawable.pScreen = pDrawable->pScreen;
    QDPIX_X(dummyPixmap) = 0;
    QDPIX_Y(dummyPixmap) = 0;
    dummyPixmap->planes = 0;

    while(nglyph--) {
	pci = *ppci++;
#ifndef X11R4
	pglyph = (unsigned char *)pci->pPriv;
#else
        pglyph = pglyphBase + pci->byteOffset;
#endif /* X11R4 */
	gWidth = GLYPHWIDTHPIXELS(pci);
	gHeight = GLYPHHEIGHTPIXELS(pci);
	nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(pci);
	nbyPadGlyph = PixmapBytePad(gWidth, 1);

	for (i=0, pb = pbits; i<gHeight; i++, pb = pbits+(i*nbyPadGlyph))
	    for (j = 0; j < nbyGlyphWidth; j++)
		*pb++ = *pglyph++;

	QDPIX_WIDTH(&dummyPixmap->pixmap) = gWidth;
	QDPIX_HEIGHT(&dummyPixmap->pixmap) = gHeight;
	dummyPixmap->pixmap.devKind = PixmapBytePad(gWidth, 1);
	QD_PIX_DATA(&dummyPixmap->pixmap) = pbits;
	qdPushPixels(pGC, &dummyPixmap->pixmap, pDrawable,
		     gWidth, gHeight,
		     x + pci->metrics.leftSideBearing,
		     y - pci->metrics.ascent);
	/* hack to prevent CopyPixmapFromOffscreen */
	QD_PIX_DATA(&dummyPixmap->pixmap) = (unsigned char *)1;
	tlCancelPixmap(dummyPixmap);
	x += pci->metrics.characterWidth;
    }
    DEALLOCATE_LOCAL(pbits);
}
