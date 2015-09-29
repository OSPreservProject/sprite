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

#include "scrnintstr.h"
#include "gcstruct.h"
#ifndef X11R4
#include "fontstr.h"
#else
#include "fontstruct.h"
#include "dixfontstr.h"
#endif /* X11R4 */

#include "qd.h"
#define LOG2OF1024	10

/*
 * must be called only once per font, otherwise storage leak
 */
qdRealizeFont( pscr, pfont)
    ScreenPtr	pscr;
#ifndef X11R4
    EncodedFontPtr	pfont;
#else
    FontPtr	pfont;
#endif /* X11R4 */
{
    pfont->devPriv[ pscr->myNum] = (pointer)NULL;
}

qdUnrealizeFont( pscr, pfont)
    ScreenPtr	pscr;
#ifndef X11R4
    EncodedFontPtr	pfont;
#else
    FontPtr	pfont;
#endif /* X11R4 */
{
    QDFontPtr	pqdfont = (QDFontPtr)pfont->devPriv[ pscr->myNum];

    if ( pqdfont != (QDFontPtr)NULL
      && pqdfont != (QDFontPtr)QDSLOWFONT)
    {
	qdDestroyPixmap( pqdfont->pPixmap);
	Xfree( pqdfont);
    }
}

/*
 * Called by qdValidateGC to create a QDFontRec for fast output.
 *
 * Creates a single plane pixmap in memory, writes the characters of the
 * font into it, and loads the bitmap into the off-screen cache.
 *     
 * pFont must satisfy the various criteria for being loaded off-screen,
 * otherwise the font private field will be set to QDSLOWFONT.
 * The criteria are:
 *	one-byte character code
 *	can be packed side-to-side in one row of off-screen memory
 * 
 * Returns:
 *   0 - if fast output is not possible
 *   1 - if fast PolyText is possible
 *   2 - if fast PolyText AND ImageText are possible (fixed-width font)
 */
int
QDCreateFont( pscr, pFont)
    ScreenPtr	pscr;
#ifndef X11R4
    EncodedFontPtr	pFont;
#else
    FontPtr	pFont;
#endif /* X11R4 */
{
    extern PixmapPtr qdCreatePixmap();
    unsigned int ic;
#ifndef X11R4
    CharInfoPtr	*ppci;		/* will be biased by -chfirst */
    CharSetPtr  pcs;
#else
    CharInfoPtr	pci;		/* will be biased by -chfirst */
    FontInfoPtr pfi;
#endif /* X11R4 */
    QDFontPtr	pqdfont;
    GCPtr	pgc;		/* handed to mfb, to build font */
    int		chfirst;
    int		chlast;
    int		paddedbreadth;	/* no character is fatter than this */
    int		cellheight;	/* no character is taller than this */
    int		nrows;
    int         nxbits;
    int         xmask;
    xRectangle	rects[1];
    int		leftKern, width, ascent, descent;
    int		hasKerning = 0;
    XID		gcv[5];

    /*
     * check that this is the first call to QDCreateFont for this font
     * and that the font is suitable for fast output
     */
    if (pFont == NULL)
	return 0;
    pqdfont = (QDFontPtr) pFont->devPriv[pscr->myNum];
    if ((int)pqdfont == QDSLOWFONT)
	return 0; 
#ifndef X11R4
    pcs = pFont->pCS;
#else
    
    pfi = pFont->pFI;

#endif /* X11R4 */
    if (pqdfont != NULL)
	return 2 - pqdfont->hasKerning;
    /* if ( ! pFont->pCS->linear)	NOT SET BY DIX		XXX */
#ifndef X11R4
    if ( pFont->firstRow != pFont->lastRow)
#else
    if ( pFont->pFI->firstRow != pFont->pFI->lastRow)
#endif /* X11R4 */
	return 0;

    /*
     * Create the font bitmap for off-screen caching.
     * Leave spaces for the unused character codes from 0 to chfirst,
     * to simplify the arithmetic.
     */
#ifndef X11R4
    chfirst = pFont->firstCol;
    chlast = pFont->lastCol;
    ppci = pFont->ppCI - chfirst;
#else
    chfirst = pFont->pFI->firstCol;
    chlast = pFont->pFI->lastCol;
    pci = &pFont->pCI[-chfirst];
#endif /* X11R4 */

#ifndef X11R4
    ascent = pcs->fontAscent;
    if (pcs->maxbounds.ascent > ascent) {
#else
    ascent = pfi->fontAscent;
    if (pfi->maxbounds.metrics.ascent > ascent) {
#endif /* X11R4 */
	hasKerning = 1;
#ifndef X11R4
	ascent = pcs->maxbounds.ascent;
#else
	ascent = pfi->maxbounds.metrics.ascent;
#endif /* X11R4 */
    }
#ifndef X11R4
    descent = pcs->fontDescent;
    if (pcs->maxbounds.descent > descent) {
#else
    descent = pfi->fontDescent;
    if (pfi->maxbounds.metrics.descent > descent) {
#endif /* X11R4 */
	hasKerning = 1;
#ifndef X11R4
	descent = pcs->maxbounds.descent;
#else
	descent = pfi->maxbounds.metrics.descent;
#endif /* X11R4 */
    }
    cellheight = ascent + descent;
#ifndef X11R4
    leftKern = - min(pcs->minbounds.leftSideBearing, 0);
    width = max(pcs->maxbounds.rightSideBearing,
		pcs->maxbounds.characterWidth)
#else
    leftKern = - min(pfi->minbounds.metrics.leftSideBearing, 0);
    width = max(pfi->maxbounds.metrics.rightSideBearing,
		pfi->maxbounds.metrics.characterWidth)
#endif /* X11R4 */
	  + leftKern;
    paddedbreadth = power2ceiling(width);
    nrows = paddedbreadth * (chlast+1) / pscr->width +
	   (paddedbreadth * (chlast+1) % pscr->width ? 1 : 0);

    /*
     * POLICY: if the font would be too tall to fit off-screen without crowding
     * the full-depth pixmaps, back out.
     */
    if ( nrows * cellheight > (2048-864)>>1)
    {
	pFont->devPriv[ pscr->myNum] = (pointer)QDSLOWFONT;
	return 0;
    }

    /*
     * allocate the QDFontRec
     * create a depth 1 pixmap and write the characters into it
     */
    pqdfont = (QDFontPtr) Xalloc( sizeof(QDFontRec));
    pFont->devPriv[ pscr->myNum] = (pointer)pqdfont;
    pqdfont->log2dx = ffs( paddedbreadth) - 1;
    pqdfont->pPixmap = qdCreatePixmap( pscr, pscr->width, nrows*cellheight, 1);
    pqdfont->leftKern = leftKern;
    pqdfont->width = width;
    pqdfont->ascent = ascent;
    pqdfont->descent = descent;
    nxbits = LOG2OF1024-pqdfont->log2dx;
    xmask = (1<<nxbits)-1;

    pgc = GetScratchGC( 1, pscr);
    gcv[0] = GXclear;
    gcv[1] = 1;
    gcv[2] = 0;
    gcv[3] = FillSolid;
    gcv[4] = (XID)pFont;
    DoChangeGC(pgc, GCFunction|GCForeground|GCBackground|GCFillStyle|GCFont,
	       gcv, 1);

    ValidateGC( pqdfont->pPixmap, pgc);
    rects->x = rects->y = 0;
    rects->width = QDPIX_WIDTH(pqdfont->pPixmap);
    rects->height = QDPIX_HEIGHT(pqdfont->pPixmap);
    qdPixFillRect(pqdfont->pPixmap, pgc, 1, rects);
    gcv[0] = GXcopy;
    DoChangeGC(pgc, GCFunction, gcv, 0);
    ValidateGC( pqdfont->pPixmap, pgc);
    for ( ic=chfirst; ic<=chlast; ic++)
    {
	miPolyText8( pqdfont->pPixmap, pgc,
		paddedbreadth * (ic & xmask) + leftKern,
		cellheight * (ic >> nxbits) + ascent,
		1, (char *)&ic);
#ifndef X11R4
	if (ppci[ic]->metrics.characterWidth <
	    ppci[ic]->metrics.rightSideBearing)
#else
	if (pci[ic].metrics.characterWidth <
	    pci[ic].metrics.rightSideBearing)
#endif /* X11R4 */
	    hasKerning = 1;
    }
    FreeScratchGC( pgc);
    pqdfont->hasKerning = hasKerning;
    return 2 - hasKerning;
}

/*
 * Called by fast text output routines.
 *
 * Check to see if the font is already off-screen.  If not, create a temporary
 * paint pixmap, load it off-screen, and destroy the paint pixmap.
 *
 * address of the x11 font is used as the unique ID for the off-screen bitmap
 *
 * returns planemask of loaded font
 */
int
LoadFont( pscr, pFont, yaddr)
    ScreenPtr	pscr;
#ifndef X11R4
    EncodedFontPtr	pFont;
#else
    FontPtr	pFont;
#endif /* X11R4 */
    int*	yaddr;	/* RETURN */
{
    QDPixPtr	pxpix =
	QD_PIX(((QDFontPtr)pFont->devPriv[ pscr->myNum])->pPixmap);
    int		planemask;

    /*
     * Hand the memory bitmap to the off-screen mem. allocator
     */
    planemask = tlConfirmPixmap(pxpix);
    *yaddr = QDPIX_Y(pxpix);
    return planemask;
}
