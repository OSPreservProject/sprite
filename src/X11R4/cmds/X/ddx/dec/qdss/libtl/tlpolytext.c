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

/*
 * SETVIPER does not yet initialize LU_R4, upon which INITTEXT* depends,
 * so we do the following hacks:				XX
 */
#if NPLANES==8
#undef NPLANES
#define NPLANES 24
#define GETGREENBYTE( p)	((p)<<8)
#else
#define GETGREENBYTE( p)        (p)
#endif

#include "X.h"
#include "Xproto.h"		/* required by fontstruct.h */

#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#ifndef X11R4
#include "fontstr.h"
#else
#include "fontstruct.h"
#include "dixfontstr.h"
#endif /* X11R4 */

#include "qd.h"
#include "qdgc.h"

#include <sys/types.h>
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdreg.h>

#include "tl.h"
#include "tltemplabels.h"

#define LOG2OF1024	10
#define u_char unsigned char
#define TEXT_INIT_NEEDED 10
#define MAGIC_POS 0

#ifndef PROCNAME
# define PROCNAME tlPolyText
#endif

extern int	Nplanes;

#ifdef IMAGE
void
#else	/* PolyText returns width */
int
#endif
PROCNAME( pDraw, pGC, x0, y0, nChars, pStr)
    DrawablePtr pDraw;		/* Not used; location is found in
				 * pGC->lastWinOrg, which works for
				 * both Windows and Pixmaps. */
    GCPtr       pGC;
    int		x0, y0;		/* origin */
    int		nChars;
    char *	pStr;
{
    DDXPointRec		absCorner;
#ifndef X11R4
    EncodedFontPtr	pFont = pGC->font;
    CharSetPtr		pcs = pFont->pCS;
    int			chfirst = pFont->firstCol;
    int			chlast = pFont->lastCol;
    CharInfoPtr 	*ppCI = pFont->ppCI - chfirst;
#else
    FontPtr		pFont = pGC->font;
    FontInfoPtr		pfi = pFont->pFI;
    int			chfirst = pfi->firstCol;
    int			chlast = pfi->lastCol;
    CharInfoPtr 	pCI;
#endif /* X11R4 */
    int			fontY;	/* Y address of off-screen font */
    int			fontPmask /* plane address of off-screen font */
			    = LoadFont( pDraw->pScreen, pFont, &fontY);
    RegionPtr		pcompclip = QDGC_COMPOSITE_CLIP(pGC);
    struct DMAreq *	pRequest;
    BoxPtr		pboxes;	/* a temporary */
    QDFontPtr		qdfont =
			    (QDFontPtr) pFont->devPriv[ pGC->pScreen->myNum];
    register unsigned short *p;
    register u_char *	pstr;
    register int	nxbits = LOG2OF1024-qdfont->log2dx;
    int			width = qdfont->width & 0x3fff;
    register int	leftKern = qdfont->leftKern;
    int			log2dx = qdfont->log2dx;
    int 		height = qdfont->ascent + qdfont->descent;
    int			ic;
    register int	xc;
    int			maxcharblits = (MAXDMAWORDS-TEXT_INIT_NEEDED)/3;

    if (nChars == 0)
#ifdef IMAGE
	return;
#else
	return x0;    

#ifndef SOLID
    INSTALL_FILLSTYLE(pGC, pDraw);
#endif
#endif

#ifdef X11R4
    pCI = &pFont->pCI[-chfirst];
    pfi = pFont->pFI;
#endif /* X11R4 */

    absCorner = pGC->lastWinOrg;
#ifdef USE_TRANS
#define TRANS(x_or_y, val) ((val)+absCorner.x_or_y)
#else
#define TRANS(x_or_y, val) (val)
    x0 += absCorner.x;
    y0 += absCorner.y;
#endif

    SETTRANSLATEPOINT(TRANS(x,0), TRANS(y,0));

# ifdef IMAGE
    Need_dma(6);
    *p++ = JMPT_SET_MASKED_ALU;
    *p++ = pGC->planemask;
    *p++ = LF_SOURCE | FULL_SRC_RESOLUTION;
    *p++ = JMPT_SETFOREBACKCOLOR;
    *p++ = pGC->fgPixel;
    *p++ = pGC->bgPixel;
# else
    Need_dma(5);
    *p++ = JMPT_SET_MASKED_ALU;
    *p++ = pGC->planemask;
    *p++ = umtable[ pGC->alu];
    *p++ = JMPT_SETCOLOR;
    *p++ = pGC->fgPixel;
# endif
    Confirm_dma();

#ifndef IMAGE
    xc = MAGIC_POS; /* to detect if no painting was done, due to clipping */
#endif

    for (ic = REGION_NUM_RECTS(pcompclip), pboxes = REGION_RECTS(pcompclip);
	 --ic >= 0; pboxes++) {
	int			nch = nChars;
#ifdef IMAGE
	unsigned short *	px2clip;
#endif

	/* do trivial-reject Y clipping */
        if ( TRANS(y,y0) - qdfont->ascent		>= pboxes->y2
	  || TRANS(y,y0) + qdfont->descent		< pboxes->y1) 
	    continue;

	pstr = (u_char *) pStr;
	xc = x0;

	while( nch) {
	    /*
	     * The SETCLIP and INITTEXT* calls could be moved
	     * outside this loop - except then the "sleazoid hack"
	     * would break. In practice, there will only
	     * be one iteration anyway.
	     */
	    int n = min( nch, maxcharblits);
	    nch -= n;
	    Need_dma(TEXT_INIT_NEEDED + 3*n);
	    *p++ = JMPT_SETCLIP;
	    *p++ = pboxes->x1;
#ifdef IMAGE
	    px2clip = p;	  /* set up for ultimate sleazoid hack */
#endif
	    *p++ = pboxes->x2;
	    *p++ = pboxes->y1;
	    *p++ = pboxes->y2;

#ifdef IMAGE
	    *p++ = JMPT_INITTEXTTERM;
#else
#ifdef SOLID
	    *p++ = JMPT_INITTEXTSOLID;
#else
	    *p++ = JMPT_INITTEXTMASK;
#endif
#endif
	    *p++ = width;				/*dx*/
	    *p++ = height;				/*dy*/
	    *p++ = fontPmask;
	    *p++ = y0 - qdfont->ascent & 0x3fff;
#ifdef IMAGE
#ifndef X11R4
	    if (leftKern == 0 && width == pcs->minbounds.characterWidth)
#else
	    if (leftKern == 0 && width == pfi->minbounds.metrics.characterWidth)
#endif /* X11R4 */
	      while ( n--) {
		register unsigned int ch = *pstr++;

		if ( ch < chfirst || ch > chlast) {
#ifndef X11R4
		    ch = pFont->defaultCh;		/* step on the arg */
#else
		    ch = pFont->pFI->chDefault;		/* step on the arg */
#endif /* X11R4 */
		    if ( ch < chfirst || ch > chlast) continue;
		}
		*p++ = (ch << log2dx) & 1023;	/* source X */
		*p++ = (ch >> nxbits) * height + fontY;	/* source Y */
		*p++ = xc & 0x3fff;			/* destination X */
		xc += width;
	    }
	    else
#endif /* IMAGE */
	    while ( n--) {
		register unsigned int ch = *pstr++;

		if ( ch < chfirst || ch > chlast) {
#ifndef X11R4
		    ch = pFont->defaultCh;		/* step on the arg */
#else
		    ch = pFont->pFI->chDefault;	/* step on the arg */
#endif /* X11R4 */
		    if ( ch < chfirst || ch > chlast) continue;
		}
#ifndef SOLID
		/* There seems to be a hardware problem such that
		 * you cannot do left edge clipping if you are also using
		 * both source cycles.
		 */
		if (TRANS(x,xc) - leftKern < pboxes->x1) {
		    int skip = pboxes->x1 - TRANS(x,xc) + leftKern;
		    if (skip < width) {
			Confirm_dma();
			Need_dma(8);
			*p++ = JMPT_INITTEXTMASK;
			*p++ = (width - skip) & 0x3fff;			/*dx*/
			*p++ = height & 0x3fff;				/*dy*/
			*p++ = fontPmask;
			*p++ = y0 - qdfont->ascent & 0x3fff;
			*p++ = ((ch << log2dx) & 1023) + skip;	/* source X */
			*p++ = (ch >> nxbits) * height + fontY;	/* source Y */
			*p++ = (pboxes->x1 - TRANS(x, 0)) & 0x3fff;
			Confirm_dma();
			Need_dma(5 + 3*n);
		    }
		} else
#endif
		{
		    *p++ = (ch << log2dx) & 1023;	/* source X */
		    *p++ = (ch >> nxbits) * height + fontY;	/* source Y */
		    *p++ = xc - leftKern & 0x3fff;	/* destination X */
		}
#ifndef X11R4
		xc += ppCI[ch]->metrics.characterWidth;
#else
		xc += pCI[ch].metrics.characterWidth;
#endif /* X11R4 */
	    }
#ifdef IMAGE
	    /*
	     * Ultimate sleazoid hack to clip off ragged, background-colored,
	     * right-hand side of glyphs.
	     */
	    *px2clip = min( *px2clip, TRANS(x,xc));
#endif
	    Confirm_dma();
	}
    }
#ifdef IMAGE
    Need_dma(1);
    *p++ = JMPT_RESET_FORE_BACK;
    Confirm_dma();
#else
    Need_dma(2);
    *p++ = JMPT_SETMASK;
    *p++ = 0xFFFF;
    Confirm_dma();
    if (xc == MAGIC_POS) {
	xc = x0;
	pstr = (u_char *) pStr;
	while ( nChars--) {
	    ic = *pstr++;
	    if ( ic >= chfirst && ic <= chlast)
#ifndef X11R4
		xc += ppCI[ic]->metrics.characterWidth;
#else
		xc += pCI[ic].metrics.characterWidth;
#endif /* X11R4 */
  	}
    }
#ifdef USE_TRANS
    return xc;
#else
    return xc - absCorner.x;
#endif
#endif
}
