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

extern int PixmapUseOffscreen;
#include "scrnintstr.h"

#include "pixmapstr.h"

#include <sys/types.h>

#include "X.h"
#include "windowstr.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "dixstruct.h"

#include "Xproto.h"
#include "Xprotostr.h"
#include "mi.h"
#include "Xmd.h"
#include "servermd.h"

/*
 * driver headers
 */
#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdioctl.h>
#include <vaxuba/qdreg.h>

#include "qd.h"
#include "qdgc.h"

#include "qdprocs.h"

/*
 * used as procedure vector when drawable is an undrawable window
 */
void
qdFSUndrawable()
{
}

#define	PIXDEPTH(x)	((x->drawable).depth)
/* ZCOPY - XXX replaced by does-everything function qddopixel! */
#if NPLANES==24
# define ZCOPY(s,d,sw,dw) \
       {*((unsigned char *) d) = *((unsigned char *) s); \
	*(((unsigned char *) d)+dw) = *(((unsigned char *) s)+sw); \
	*(((unsigned char *) d)+2*(dw)) = *(((unsigned char *) s)+2*(sw));}
#else	/* NPLANES == 8 */
# define ZCOPY(s,d,sw,dw) \
       {*((unsigned char *) d) = *((unsigned char *) s);}
#endif

#if NPLANES==24
#define DOPIXEL(psrc, dst, pGC, delta) qddopixel(psrc, dst, pGC, delta)
#else
#define DOPIXEL(psrc, dst, pGC, delta) qddopixel(psrc, dst, pGC)
#endif


#define u_char	unsigned char

/*
 * GetSpans -- for each span, gets bits from drawable starting at ppt[i]
 * and continuing for pwidth[i] bits
 * Each scanline returned will be server scanline padded, i.e., it will come
 * out to an integral number of longwords.
 */
/*
 * Currently, the caller does an Xfree of the returned value.
 *
 * If Drawable is the screen
 *	hand the bytes back a pixel at a time.
 * If Drawable is a pixmap
 *	hand the bytes back a pixel at a time.
 */

extern	int Nentries;
extern	int Nplanes;
#if NPLANES==24
extern	int Nchannels;
#else
#define Nchannels 1
#endif
extern unsigned int Allplanes;

#ifdef X11R4
/*
 * In R4, we change the interface slightly for GetSpans when pDrawable
 * is a Pixmap: pDrawable->{x,y} must have been pre-addes to the ppt
 * coordinates. For off-screen Pixmaps, this means they have absolute
 * GPX-relative coordinates, which is what we want anyway.
 * This convention is useful, because the mi routines usually add
 * in pDrawable->{x,y} before calling GetSpans. The exception is
 * miPushPixels (since it "knows" that the mask is a Bitmap); we have
 * to be sure it is only called when mask->drawable->{x,y} are both 0.
 */
#endif

#ifdef X11R4
void
qdGetSpans( pDrawable, wMax, ppt, pwidth, nspans,pdstStart)
#else
unsigned int *
qdGetSpans( pDrawable, wMax, ppt, pwidth, nspans)
#endif
    DrawablePtr		pDrawable;	/* drawable from which to get bits */
    int			wMax;		/* largest value of all *pwidths */
    register DDXPointPtr ppt;		/* points to start copying from */
    int			*pwidth;	/* list of number of bits to copy */
    int			nspans;		/* number of scanlines to copy */
#ifdef X11R4
    unsigned int	*pdstStart;	/* where to put the bits */
#endif
{
    static void		qdGSPixFull();
    unsigned int *	pret;
    int x, y;
#if NPLANES==24
    24-plane support has been removed
#else
    DDXPointPtr	  	pptInit;
    int	    	  	*pwidthInit;
    int	    	  	i;
#endif
    if ( pDrawable->type == UNDRAWABLE_WINDOW)
#ifdef X11R4
    {
        pdstStart = (unsigned int *)NULL;
        return ;
    }
#else
	return (unsigned int *)NULL;
#endif
    if ( pDrawable->depth == 1 && pDrawable->type == DRAWABLE_PIXMAP) {
	QDPixPtr pix = (QDPixPtr)pDrawable;
	if (QD_PIX_DATA(&pix->pixmap) == NULL) {
#if 0
#ifndef X11R4
	    x = QDPIX_X(pix);
	    y = QDPIX_Y(pix);
#endif
	    int plane_mask = pix->planes;
	    register unsigned char *dst;
	    char *buf = (char*)ALLOCATE_LOCAL(wMax);
	    i = nspans * PixmapBytePad(wMax, 1);
#ifdef X11R4
            dst = (unsigned char *)pdstStart;
#else
	    dst = (unsigned char *)Xalloc(i);
	    pret = (unsigned int*)dst;
#endif
	    bzero(dst, i);
	    for (i = nspans; --i >= 0; ppt++, pwidth++) {
		register int j;
#ifdef X11R4
		tlgetspan((WindowPtr)0, ppt->x, ppt->y, *pwidth, buf);
#else
		tlgetspan((WindowPtr)0, x + ppt->x, y + ppt->y, *pwidth, buf);
#endif
		for (j = *pwidth; --j >= 0; )
		    if (buf[j] & plane_mask) {
			dst[j>>3] |= 1 << (j & 7);
		    }
		dst += PixmapBytePad(*pwidth, 1);
	    }
	    DEALLOCATE_LOCAL(buf);
#ifdef X11R4
            return;
#else
	    return pret;
#endif
#else   /* 1 */
	    extern int DebugDma;
	    int size = pix->pixmap.devKind * QDPIX_HEIGHT(&pix->pixmap);
	    QD_PIX_DATA(&pix->pixmap) = (pointer)Xalloc(size);
#ifdef DEBUG
	    if (DebugDma > 0)
		printf("[Copy pixmap 0x%x from offscreen]\n", pix);
#endif
	    CopyPixmapFromOffscreen(pix, QD_PIX_DATA(&pix->pixmap));
#endif /* #if 1 */
	}
#ifdef X11R4
        x = QDPIX_X(pix);
	y = QDPIX_Y(pix);
	QDPIX_X(pix) = QDPIX_Y(pix) = 0;
	mfbGetSpans( pDrawable, wMax, ppt, pwidth, nspans, pdstStart);
        QDPIX_X(pix) = x;
	QDPIX_Y(pix) = y;
	return;
#else
	return mfbGetSpans( pDrawable, wMax, ppt, pwidth, nspans);
#endif
    }

    pptInit = ppt;
    pwidthInit = pwidth;

    /*
     * It's a full depth Pixmap or a window.
     * Use an upper bound for the number of bytes to allocate.
     */
    if ( pDrawable->depth == Nplanes)
#ifdef X11R4
        pret = pdstStart;
    else
    {
        pdstStart = (unsigned int *)NULL;
        return;
    }
#else
	pret = (unsigned int *) Xalloc( wMax * nspans);
    else
	return (unsigned int *)NULL;
#endif
    if ( pDrawable->type == DRAWABLE_WINDOW)
    {
	int	ns;			/* span count */
	u_char *  pr = (u_char *) pret;

	for ( ns=0; ns<nspans; ns++, pwidth++, ppt++)
	{
	    /*
	     *  order these come back in: RGBRGB...
	     */
	    tlgetspan((WindowPtr) pDrawable, ppt->x, ppt->y,
		      *pwidth, pr);
	    pr += *pwidth;
	}
#ifndef X11R4
	/*
	 * If the drawable is a window with some form of backing-store, consult
	 * the backing-store module to fetch any invalid spans from the window's
	 * backing-store. The pixmap is made into one long scanline and the
	 * backing-store module takes care of the rest. We do, however, have
	 * to tell the backing-store module exactly how wide each span is, padded
	 * to the correct boundary.
	 */
	if (((WindowPtr)pDrawable)->backingStore != NotUseful)
	{
	    QDPixRec pix[1];

	    pix->pixmap.drawable.type = DRAWABLE_PIXMAP;
	    pix->pixmap.drawable.pScreen = pDrawable->pScreen;
	    pix->pixmap.drawable.depth = pDrawable->depth;
	    pix->pixmap.drawable.serialNumber = NEXT_SERIAL_NUMBER;
	    QDPIX_WIDTH(&pix->pixmap) = wMax;
	    QDPIX_HEIGHT(&pix->pixmap) = 1;
	    pix->pixmap.refcnt = 1;
	    QD_PIX_DATA(&pix->pixmap) = (unsigned char *) pret;
	    QDPIX_Y(pix) = NOTOFFSCREEN;
	    LockPixmap(pix);
	    miBSGetSpans(pDrawable, pix, wMax, pptInit, pwidthInit,
			 pwidth, nspans);
	    UnlockPixmap(pix);
	}
#endif
    }
    else if (QD_PIX_DATA((PixmapPtr)pDrawable) == NULL){ /*offscreen PIXMAP*/
	int	ns;			/* span count */
	u_char *  pr = (u_char *) pret;

	for ( ns=0; ns<nspans; ns++, pwidth++, ppt++)
	{
	    /*
	     *  order these come back in: RGBRGB...
	     */
	    tlgetspan((WindowPtr)NULL,
#ifdef X11R4
		      ppt->x, ppt->y,
#else
		      ppt->x + QDPIX_X((QDPixPtr)pDrawable),
		      ppt->y + QDPIX_Y((QDPixPtr)pDrawable),
#endif
		      *pwidth, pr);
	    pr += *pwidth;
	}
    }
    else	/* DRAWABLE_PIXMAP */
    {
	tlCancelPixmap( (QDPixPtr)pDrawable);
	qdGSPixFull( (PixmapPtr)pDrawable, wMax, ppt, pwidth, nspans, pret);
    }
#ifndef X11R4
    return pret;
#endif
}

static void
qdGSPixFull( pPix, wMax, ppt, pwidth, nspans, pret)
    PixmapPtr		pPix;
    int			wMax;		/* largest value of all *pwidths */
    register DDXPointPtr ppt;		/* points to start copying from */
    int			*pwidth;	/* list of number of bits to copy */
    int			nspans;		/* number of scanlines to copy */
    int *		pret;		/* return value */
{
    u_char *	ps = QD_PIX_DATA(pPix);
    register u_char *	pd = (u_char *)pret;
    int		ns;				/* span count */
    register int	i;
    register int	pixwidth = QDPIX_WIDTH(pPix);


    for ( ns=0; ns<nspans; ns++, pwidth++, ppt++) {
#if NPLANES==24
	int skip = ppt->y*pixwidth + ppt->x;
	register u_char *psr = ps + skip;			     /*red*/
	register u_char *psg = ps+pixwidth*QDPIX_HEIGHT(pPix)+skip; /*green*/
	register u_char *psb = ps+2*pixwidth*QDPIX_HEIGHT(pPix)+skip; /*blue*/
	for ( i = *pwidth; --i >= 0; )
	{
	    *pd++ = *psr++; 
	    *pd++ = *psg++; 
	    *pd++ = *psb++; 
	}
#else
	bcopy(ps + ppt->y * pixwidth + ppt->x, pd, *pwidth);
	pd += *pwidth;
#endif
    }
}
void
qdSetSpansWin(pDraw, pGC, pPixels, pPoint, pWidth, n, fSorted)
    DrawablePtr pDraw;
    GC *pGC;
    pointer pPixels;
    DDXPointPtr pPoint;
    int *pWidth;
    int n;
    int fSorted;
{
    int		ispan;  /* counts spans */
    int		ipix;	/* indexes pPixels; knows about pixel size */

    if (pWidth[0] <= 0 || n <= 0)
	return;

    for ( ispan = ipix = 0; ispan < n; ipix += pWidth[ispan++]*Nchannels) {
#if NPLANES==24
	/*
	 * convert from rgb, rgb, rgb to
	 * all red, all green, all blue for tlsetspan's convenience
	 */
	unsigned char *	newpix;
	int		j, off;
	register int	isnf;	/* index to server-natural */

	if ( pWidth[ispan] <= 0)
	    continue;
	newpix = (unsigned char *)
	    ALLOCATE_LOCAL(pWidth[ispan] * 3 * sizeof(unsigned char));
	for (j=0, isnf=ipix; j<pWidth[ispan]; j++) {
	    for (off = 0; off < 3; off++) {
		newpix[j+pWidth[ispan]*off] =
		    pPixels[isnf++];
	    }
	}
	tlsetspans(pDraw, pGC, newpix, &pPoint[ispan],
		   &pWidth[ispan], 1, 1);
	DEALLOCATE_LOCAL(newpix);
#else	/* NPLANES == 8 */
	if ( pWidth[ispan] <= 0)
	    continue;
	tlsetspans(pDraw, pGC, &pPixels[ipix], &pPoint[ispan],
		   &pWidth[ispan], 1, 1);
#endif
    }
}

void
qdSetSpansPix1(pPix, pGC, pPixels, pPoint, pWidth, n, fSorted)
    QDPixPtr pPix;
    GC *pGC;
    pointer pPixels;
    DDXPointPtr pPoint;
    int *pWidth;
    int n;
    int fSorted;
{
    if (QD_PIX_DATA(&pPix->pixmap) == NULL) {
	/* This code could be better, but it shouldn't be used much. */
	PixmapRec	tempBMap;
	SETUP_PIXMAP_AS_WINDOW((DrawablePtr)pPix, pGC);
	pGC->fgPixel = Allplanes;
	pGC->bgPixel = 0;
	QDPIX_HEIGHT(&tempBMap) = 1;
	for ( ; --n >= 0; pWidth++, pPoint++) {
	    int         ic;     /* clip rect index */
	    int	numRects = REGION_NUM_RECTS(QDGC_COMPOSITE_CLIP(pGC));
	    register BoxPtr rects = REGION_RECTS(QDGC_COMPOSITE_CLIP(pGC));
	    BoxPtr newRects;

	    /* create a temporary bitmap and transplant pImage into it  */
	    QDPIX_WIDTH(&tempBMap) = *pWidth;
	    tempBMap.devKind = PixmapBytePad(QDPIX_WIDTH(&tempBMap), 1);
	    QD_PIX_DATA(&tempBMap) = pPixels;

	    for ( ic=0; ic < numRects; ic++, rects++)
		tlBitmapBichrome(pGC, &tempBMap, Allplanes, 0,
				 pPoint->x + QDPIX_X(pPix),
				 pPoint->y + QDPIX_Y(pPix),
				 rects);
	    pPixels += tempBMap.devKind;
	}
	CLEANUP_PIXMAP_AS_WINDOW(pGC);
    }
    else {
	CHECK_MOVED(pGC, &pPix->pixmap.drawable);
	mfbSetSpans(pPix, pGC, pPixels, pPoint, pWidth, n, fSorted);
    }
}

void
qdSetSpansPixN(pPix, pGC, pPixels, pPoint, pWidth, n, fSorted)
    PixmapPtr pPix;
    GC *pGC;
    pointer pPixels;
    DDXPointPtr pPoint;
    int *pWidth;
    int n;
    int fSorted;
{
    int		ispan;  /* counts spans */
    int		ipix;	/* indexes pPixels; knows about pixel size */
    int		j;
    int		ic;	/* clip rect index */
    BoxPtr      pc = REGION_RECTS(QDGC_COMPOSITE_CLIP(pGC));
#if NPLANES<24
    int fast;
#endif

    if (pWidth[0] <= 0 || n <= 0)
	return;

    if (QD_PIX_DATA(pPix) == NULL) {
	CHECK_MOVED(pGC, &pPix->drawable);
	qdSetSpansWin(pPix, pGC, pPixels, pPoint, pWidth, n, fSorted);
	return;
    }

    tlCancelPixmap( pPix);
    CHECK_MOVED(pGC, &pPix->drawable);

#if NPLANES<24
    fast = pGC->alu == GXcopy && (pGC->planemask & Allplanes) == Allplanes;
#endif
    /* for each clipping rectangle */
    for ( ic = REGION_NUM_RECTS(QDGC_COMPOSITE_CLIP(pGC)); --ic >= 0; pc++) {
	/* for each scan */
	for (ispan = ipix = 0; ispan < n; ipix += pWidth[ispan++]*Nchannels) {
	    unsigned char *psrc, *pdst;
	    int x, y;
	    y = pPoint[ispan].y;
	    if (pc->y2 <= y || pc->y1 > y)
		continue;
	    x = pPoint[ispan].x;
	    j = x+pWidth[ispan];
	    psrc = &pPixels[ipix];
	    if (x < pc->x1) {
		psrc += Nchannels * (pc->x1 - x);
		x = pc->x1;
	    }
	    if (j > pc->x2) j = pc->x2;
	    j -= x;
	    pdst = QD_PIX_DATA(pPix) + x + y * QDPIX_WIDTH(pPix);
#if NPLANES != 24
	    if (fast) {
		if (j > 0)
		    bcopy(psrc, pdst, j);
	    }
	    else
#endif
	    while ( --j >= 0) { /* each pt */
		if (x >= pc->x2) break;
		DOPIXEL(psrc, pdst, pGC, QDPIX_WIDTH(pPix)*QDPIX_HEIGHT(pPix));
		x++; psrc += Nchannels; pdst++;
	    }    /* for j (inc on scan) */
	}    /* for ispan (scan) */
    }
}

#ifdef X11R4
void
qdSetSpansPix(pPix, pGC, pPixels, pPoint, pWidth, n, fSorted)
    PixmapPtr pPix;
    GC *pGC;
    pointer pPixels;
    DDXPointPtr pPoint;
    int *pWidth;
    int n;
    int fSorted;
{
    if (pPix->drawable.depth > 1)
	qdSetSpansPixN(pPix, pGC, pPixels, pPoint, pWidth, n, fSorted);
    else
	qdSetSpansPix1(pPix, pGC, pPixels, pPoint, pWidth, n, fSorted);
}
#endif


/*
 * FillSpans cases
 */

/*
 * Hack arg list and call qdFillBoxesOddSize.
 * In addition to having to duplicate code, it wins because the
 * region routines called by the primitive rect fill routines
 * can coalesce (thin) spans to larger boxes.
 */
void
qdWinFSOddSize( pDraw, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr	pDraw;
    GC		*pGC;
    int		nInit;
    DDXPointPtr	pptInit;
    int		*pwidthInit;
    int		fSorted;
{
    BoxPtr	pdestboxes;
    register BoxPtr	pbox;
    int		nr;

    pdestboxes = pbox = (BoxPtr) ALLOCATE_LOCAL( nInit*sizeof(BoxRec));
    for (nr = nInit; --nr >= 0; pbox++, pptInit++, pwidthInit++)
    {
	pbox->x1 = pptInit->x;
	pbox->x2 = pptInit->x + *pwidthInit;
	pbox->y1 = pptInit->y;
	pbox->y2 = pptInit->y + 1;
    }
    qdPolyFillBoxesOddSize( pDraw, pGC, nInit, pdestboxes);
    DEALLOCATE_LOCAL(pdestboxes);
}

#define	CLIPSPANS \
    register DDXPointPtr	ppt; \
    register int *		pwidth; \
    DDXPointPtr	ppt0; \
    int *	pwidth0; \
    int		n; \
    n = nInit * miFindMaxBand(QDGC_COMPOSITE_CLIP(pGC));\
    if ( n == 0) \
	return; \
    pwidth0 = pwidth = (int *)ALLOCATE_LOCAL( n * sizeof(int)); \
    ppt0 = ppt = (DDXPointRec *)ALLOCATE_LOCAL( n * sizeof(DDXPointRec)); \
    if ( !ppt || !pwidth) \
	FatalError("alloca failed in qd FillSpans.\n"); \
    n = miClipSpans(QDGC_COMPOSITE_CLIP(pGC), \
	pptInit, pwidthInit, nInit, ppt, pwidth, fSorted)

#define CLIPSPANS_FREE \
     DEALLOCATE_LOCAL(ppt0); DEALLOCATE_LOCAL(pwidth0)

#define CALL_SPAN_WITH_DUMMY(FUNC) \
    /* make dummy window and use that as the drawable */\
    SETUP_PIXMAP_AS_WINDOW(&pPix->drawable, pGC);	\
    CHECK_MOVED(pGC, &pPix->drawable);			\
    FUNC(pPix, pGC, nInit, pptInit, pwidthInit, fSorted);\
    CLEANUP_PIXMAP_AS_WINDOW(pGC);

void
qdFSPixSolid(pPix, pGC, nInit, pptInit, pwidthInit, fSorted)
    PixmapPtr	pPix;
    GC		*pGC;
    int		nInit;
    DDXPointPtr	pptInit;
    int		*pwidthInit;
    int		fSorted;
{
    tlSinglePixmap( pPix);
    if (QD_PIX_DATA(pPix) == NULL) {
	CALL_SPAN_WITH_DUMMY(tlSolidSpans);
	return;
    }
    CHECK_MOVED(pGC, &pPix->drawable);
    if (pPix->drawable.depth == 1) {
	void (*func)();
	extern void mfbBlackSolidFS(), mfbWhiteSolidFS(), 
	    mfbInvertSolidFS();
#ifdef X11R4
	switch (mfbReduceRop(pGC->alu, pGC->fgPixel)) {
#else
	switch (ReduceRop(pGC->alu, pGC->fgPixel)) {
#endif
	  case GXclear:	func = mfbBlackSolidFS; break;
	  case GXset:	func = mfbWhiteSolidFS; break;
	  case GXnoop:	func = NoopDDA; break;
	  case GXinvert:func = mfbInvertSolidFS; break;
	}
	(*func)(pPix, pGC, nInit, pptInit, pwidthInit, fSorted);
    }
    else {
	CLIPSPANS;
#if NPLANES<24
        if (pGC->alu == GXcopy && (pGC->planemask & Allplanes) == Allplanes) {
	    register long fg;
	    fg = pGC->fgPixel;
	    fg = (fg << 8) | fg;
	    fg = (fg << 16) | fg;
	    for ( ; --n >= 0; ppt++) {
		register unsigned char *pdst =
		    QD_PIX_DATA(pPix) + ppt->x + ppt->y * QDPIX_WIDTH(pPix);
		register int w = *pwidth++;
		int alDst = (int)pdst & 3;
		/* simplified algorithm from cfbsp.c */
		if (alDst + w <= 4) /* all bits inside same lonword */
		    while ( --w >= 0) *pdst++ = fg;
		else {
		    if (alDst) {
			/* word-align destination */
			alDst = 4 - alDst;
			w -= alDst;
			while (--alDst >= 0) *pdst++ = fg;
		    }
		    /* loop over longwords */
		    alDst = w & 3;
		    w >>= 2;
		    while (--w >= 0)
			*(long*)pdst++ = fg;
		    /* Do ragged right bytes. */
		    w = alDst;
		    while (--w >= 0) *pdst++ = fg;
		}
	    }
        }
        else
#endif
	    for ( ; n > 0; n--, pwidth++, ppt++) {
		unsigned char *pdst =
		    QD_PIX_DATA(pPix) + ppt->x + ppt->y * QDPIX_WIDTH(pPix);
		for ( ; *pwidth; (*pwidth)--, pdst++)
		    DOPIXEL(&pGC->fgPixel, pdst, pGC,
			QDPIX_WIDTH(pPix) * QDPIX_HEIGHT(pPix));
	    }
	CLIPSPANS_FREE;
    }
}

void
qdFSPixTiled(pPix, pGC, nInit, pptInit, pwidthInit, fSorted)
    PixmapPtr	pPix;
    GC		*pGC;
    int		nInit;
    DDXPointPtr	pptInit;
    int		*pwidthInit;
    int		fSorted;
{
    register int	tbase, tinc, tbit;
    unsigned long	scratch;
    CLIPSPANS;

    tlSinglePixmap( pPix);
    if (QD_PIX_DATA(pPix) == NULL) {
	void (*func)();
	extern void tlTiledSpans();
#ifdef X11R4
	if (qdNaturalSizePixmap(pGC->tile.pixmap)) func = tlTiledSpans;
#else
	if (qdNaturalSizePixmap(pGC->tile)) func = tlTiledSpans;
#endif
	else func = qdWinFSOddSize;
	{ CALL_SPAN_WITH_DUMMY((*func)); }
	return;
    }
    
    CHECK_MOVED(pGC, &pPix->drawable);
    if (pPix->drawable.depth == 1) {
	tlCancelPixmap(pGC->stipple);
#ifdef X11R4
	if (QDPIX_WIDTH(pGC->tile.pixmap) == 32)
#else
	if (pGC->tile && QDPIX_WIDTH(pGC->tile) == 32)
#endif
	    mfbTileFS(pPix, pGC, nInit, pptInit, pwidthInit, fSorted);
	else
	    mfbUnnaturalTileFS(pPix, pGC, nInit, pptInit, pwidthInit, fSorted);
    } else {
	CLIPSPANS;
#ifdef DEBUG
#ifdef X11R4
	if (PIXDEPTH(pGC->tile.pixmap) < Nplanes)
#else
	if (PIXDEPTH(pGC->tile) < Nplanes)
#endif
	    FatalError("tile is not full-depth pixmap\n");
#endif
	for ( ; n > 0; n--, ppt++, pwidth++) {
	    unsigned char *pdst =
		QD_PIX_DATA(pPix) + ppt->x + ppt->y * QDPIX_WIDTH(pPix);
#ifdef X11R4
	    tbase = ((ppt->y) % QDPIX_HEIGHT(pGC->tile.pixmap))
		* QPPADBYTES(QDPIX_WIDTH(pGC->tile.pixmap));
#else
	    tbase = ((ppt->y) % QDPIX_HEIGHT(pGC->tile))
		* QPPADBYTES(QDPIX_WIDTH(pGC->tile));
#endif
	    for ( ; *pwidth > 0; (ppt->x)++, *(pwidth)--, pdst++) {
#ifdef X11R4
		tinc = tbase + (ppt->x) % QDPIX_WIDTH(pGC->tile.pixmap);
		ZCOPY(QD_PIX_DATA(pGC->tile.pixmap) + tinc, &scratch,
		  QDPIX_WIDTH(pGC->tile.pixmap) * QDPIX_HEIGHT(pGC->tile), 1);
#else
		tinc = tbase + (ppt->x) % QDPIX_WIDTH(pGC->tile);
		ZCOPY(QD_PIX_DATA(pGC->tile) + tinc, &scratch,
		      QDPIX_WIDTH(pGC->tile) * QDPIX_HEIGHT(pGC->tile), 1);
#endif
		DOPIXEL(&scratch, pdst, pGC,
			QDPIX_WIDTH(pPix) * QDPIX_HEIGHT(pPix));
	    }
	}
	CLIPSPANS_FREE;
    }
}

void
qdFSPixStippleorOpaqueStip( pPix, pGC, nInit, pptInit, pwidthInit, fSorted)
    PixmapPtr	pPix;
    GC		*pGC;
    int		nInit;
    DDXPointPtr	pptInit;
    int		*pwidthInit;
    int		fSorted;
{
    int	tbase;	/* address of first byte in a stipple row */
    int	tinc;	/* byte address within a stipple row */
    int	tbit;	/* bit address within a stipple byte */

    tlSinglePixmap( pPix);
    if (QD_PIX_DATA(pPix) == NULL) {
	void (*func)();
	extern void tlStipSpans(), tlOpStipSpans();
	if (!qdNaturalSizePixmap(pGC->stipple))
	    func = qdWinFSOddSize;
	else if (pGC->fillStyle == FillStippled)
	    func = tlStipSpans;
	else
	    func = tlOpStipSpans;
	{ CALL_SPAN_WITH_DUMMY((*func)); }
	return;
    }

    CHECK_MOVED(pGC, &pPix->drawable);
    if (pPix->drawable.depth == 1) {
	/* simplified simulation of stuff done by mfbValidateGC */
#ifdef X11R4
	int rop = mfbReduceRop(pGC->alu,pGC->fgPixel);
#else
	int rop = ReduceRop(pGC->alu,pGC->fgPixel);
#endif
	tlCancelPixmap(pGC->stipple);
	if ( pGC->fillStyle == FillStippled) {
#ifdef X11R4
	    ((mfbPrivGCPtr)pGC->devPrivates[mfbGCPrivateIndex].ptr)->rop = rop;
#else
	    ((QDPrivGCPtr) pGC->devPriv)->mfb.rop = rop;
#endif
	    mfbUnnaturalStippleFS( pPix, pGC, nInit, pptInit, pwidthInit, fSorted);
	}
	else {
	    extern int InverseAlu[16];
	    if (pGC->fgPixel == pGC->bgPixel) ;
	    else if (pGC->fgPixel) rop = pGC->alu;
	    else rop = InverseAlu[pGC->alu];
#ifdef X11R4
	    ((mfbPrivGCPtr)pGC->devPrivates[mfbGCPrivateIndex].ptr)->ropOpStip = rop;
#else
	    ((QDPrivGCPtr) pGC->devPriv)->mfb.ropOpStip = rop;
#endif
	    mfbUnnaturalTileFS( pPix, pGC, nInit, pptInit, pwidthInit, fSorted);
	}
	return;
    } else {
	CLIPSPANS;
#ifdef DEBUG
	if (PIXDEPTH(pGC->stipple) != 1)
	    FatalError("stipple is not bitmap\n");
#endif
	for ( ; n > 0; n--, ppt++, pwidth++) {
	    unsigned char *pdst =
		QD_PIX_DATA(pPix) + ppt->x + ppt->y * QDPIX_WIDTH(pPix);
	    tbase = UMOD(ppt->y-pGC->patOrg.y, QDPIX_HEIGHT(pGC->stipple))
		* QPPADBYTES(QDPIX_WIDTH(pGC->stipple));
	    for ( ; *pwidth > 0; ppt->x++, (*pwidth)--, pdst++) {
		tbit = ppt->x + pGC->patOrg.x;
		tinc = tbase + UMOD(tbit, QDPIX_WIDTH(pGC->stipple)) / 8;
		tbit &= 7;
		if ( pGC->fillStyle == FillStippled) {
		    if ((QD_PIX_DATA(pGC->stipple)[tinc] >> tbit) & 1)
			DOPIXEL(&pGC->fgPixel, pdst, pGC,
				QDPIX_WIDTH(pPix)*QDPIX_HEIGHT(pPix));
		}
		else {	/* FillOpaqueStippled */
		    if ((QD_PIX_DATA(pGC->stipple)[tinc] >> tbit) & 1)
			DOPIXEL(&pGC->fgPixel, pdst, pGC,
				QDPIX_WIDTH(pPix) * QDPIX_HEIGHT(pPix));
		    else
			DOPIXEL(&pGC->bgPixel, pdst, pGC,
				QDPIX_WIDTH(pPix) * QDPIX_HEIGHT(pPix));
		}
	    }
	}
	CLIPSPANS_FREE;
    }
}
