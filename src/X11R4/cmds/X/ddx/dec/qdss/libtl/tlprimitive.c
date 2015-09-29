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

/* tlprimitive
 *	used by tldrawshapes for generic packet building
 */
extern int PixmapUseOffscreen;
void
PROCNAME (pDraw, pGC,
#ifdef	D_SPANS
	    nshapes, pshape, pWidth, fSorted)
#else	/* rects */
	    nshapes, pshape)
#endif

/* parameters declarations: */
    DrawablePtr	pDraw;
    GCPtr	pGC;
#ifdef	D_SPANS
    int         nshapes;
    DDXPointPtr pshape;
    int         *pWidth;
    int         fSorted;
#else	/* rects */
    int			nshapes;
    xRectangle *	pshape;
#endif

{
    register unsigned short     *p;
    RegionPtr pSaveGCclip = QDGC_COMPOSITE_CLIP(pGC);
    register BoxPtr pclip = REGION_RECTS(pSaveGCclip); /* step through clip */
    int		nclip = REGION_NUM_RECTS(pSaveGCclip);	/* number of clips */
    int		mask;	/* planes used by offscreen dragon tile */
    register DDXPointPtr	trans = &pGC->lastWinOrg;
#ifdef	D_SPANS
    register short	newx1;	/* used in pre-clipping */
    register int	*pwid;	/* step through widths */
    register DDXPointPtr
#else	/* rects */
    register short	tmp1, tmp2;	/* used in pre-clipping */
    register xRectangle *
#endif
	parg;		/* step through argument shapes */
    int	narg;		/* number of argument shapes */

#ifdef D_TILE
    INSTALL_FILLSTYLE(pGC, pDraw);
#endif

#ifndef NEW_TRANS
#define NEW_TRANS
#endif
#if defined(D_TILE) || defined(D_SPANS) || defined(NEW_TRANS)
    SETTRANSLATEPOINT(trans->x, trans->y);
#else
    SETTRANSLATEPOINT(0, 0);
#endif

    Need_dma(6+NCOLORSHORTS);
    *p++ = JMPT_SET_MASKED_ALU;
    *p++ = pGC->planemask;
    *p++ = umtable[pGC->alu];
    *p++ = MAC_SETCOLOR;
    SETCOLOR(p, pGC->fgPixel);
    *p++ = JMPT_RESETCLIP;
#ifndef D_TILE
#  ifdef	D_SPANS
    *p++ = JMPT_SOLIDSPAN;
#  else		/* rects */
    *p++ = JMPT_SOLIDRECT;
#endif
#else /*D_TILE*/
#  ifdef	D_SPANS
    *p++ = JMPT_TILESPAN;
#  else		/* rects */
    *p++ = JMPT_TILERECT;
#endif
#endif /*D_TILE*/
    Confirm_dma();

#define	ZERORLESS(x)	\
	(((short) (x)) <= 0)

#ifdef	D_SPANS
    for ( ; nclip > 0; nclip--, pclip++) {
	narg = nshapes;
	parg = pshape;
	pwid = pWidth;
	do {
	    register int i = MAXDMAWORDS/3;
	    if (i > narg) i = narg;
	    Need_dma(3*i);
	    narg -= i;
	    for (; --i >= 0; pwid++, parg++) {
		*p++ = (newx1 = (parg->x+trans->x < pclip->x1)
			? pclip->x1-trans->x : parg->x);
		if (parg->y+trans->y < pclip->y1
		    || parg->y+trans->y >= pclip->y2)
		    p -= 1;
		else {
		    *p++ = parg->y;
		    if (ZERORLESS(*p++ = ((parg->x+*pwid+trans->x < pclip->x2)
			   ? parg->x+(*pwid) : pclip->x2-trans->x) - newx1) )
			p -= 3;
		}
	    }
	    Confirm_dma();
	} while (narg > 0);
    }
#else	/* rects */
    /* check that alu is "idempotent" e.g. Clear, Copy (and not e.g. Xor) */
    if (nclip > 1 && ((1<<pGC->alu) & ~0x4F44) && nshapes > 1) {
	int maxBoxes = min(req_buf_size, MAXDMAPACKET/sizeof(short)) >> 2;
#ifdef X11R4
	RegionPtr argRegion;
	extern RegionPtr miRectsToRegion();
	argRegion = miRectsToRegion(nshapes, pshape, CT_REGION);
	miTranslateRegion(argRegion, trans->x, trans->y);
#else
	RegionRec argRegion[1];
	argRegion->size = nshapes;
	argRegion->numRects = nshapes;
	argRegion->rects = (BoxPtr) Xalloc(nshapes * sizeof(BoxRec));
	for (narg = nshapes, parg = pshape, pclip = REGION_RECTS(argRegion);
	  --narg >= 0; parg++, pclip++) {
	    pclip->x2 = (pclip->x1 = parg->x + trans->x) + parg->width;
	    pclip->y2 = (pclip->y1 = parg->y + trans->y) + parg->height;
	}
	miRegionValidate(argRegion);
#endif
	miIntersect(argRegion, argRegion, QDGC_COMPOSITE_CLIP(pGC));
	pclip = REGION_RECTS(argRegion);
	nclip = REGION_NUM_RECTS(argRegion);
        while (nclip > 0) {
	    if (nclip < maxBoxes) {
		tmp2 = nclip;
		nclip = 0;
	    }
	    else {
		tmp2 = maxBoxes;
		nclip -= tmp2;
	    }
	    Need_dma(4 * tmp2);
	    for (; --tmp2 >= 0; pclip++) {
#if defined(D_TILE) || defined(NEW_TRANS)
		*p++ = pclip->x1 - trans->x;
		*p++ = pclip->y1 - trans->y;
#else
		*p++ = pclip->x1;
		*p++ = pclip->y1;
#endif
		*p++ = pclip->x2 - pclip->x1;
		*p++ = pclip->y2 - pclip->y1;
	    }
	    Confirm_dma();
	}
#ifdef X11R4
	miRegionDestroy(argRegion);
#else
	Xfree(argRegion->rects);
#endif
    } else {
	for ( ; --nclip >= 0; pclip++) { /* for each clip rectangle */
#ifdef NEW_TRANS
	    BoxRec relClip;
	    relClip.x1 = pclip->x1 - trans->x;
	    relClip.x2 = pclip->x2 - trans->x;
	    relClip.y1 = pclip->y1 - trans->y;
	    relClip.y2 = pclip->y2 - trans->y;
#endif
	    narg = nshapes;
	    parg = pshape;
	    do { /* for each bunch of input rects */
		register int i = MAXDMAWORDS>>2;
		if (i > narg) i = narg;
		Need_dma(4*i);
		narg -= i;
		for ( ; --i >= 0; parg++) { /* for each input rect */
#ifdef NEW_TRANS
		    /* tmp1/tmp2 is left/right border, clipped*/

		    tmp1 = parg->x;
		    tmp2 = tmp1 + (int)parg->width;
		    if (tmp1 < relClip.x1) tmp1 = relClip.x1;
		    if (tmp2 > relClip.x2) tmp2 = relClip.x2;
		    tmp2 -= tmp1; /* make tmp2 into width */
		    if (tmp2 <= 0) continue;
		    *p++ = tmp1; /* x (relative, clipped) */
		    /*tmp1/tmp2 is upper/lower border and clipped*/
		    tmp1 = parg->y;
		    if (tmp1 < relClip.y1) tmp1 = relClip.y1;
		    *p++ = tmp1; /* y (relative, clipped) */
		    *p++ = tmp2; /* width (clipped) */
		    tmp2 = parg->y + (int)parg->height;
		    if (tmp2 > relClip.y2) tmp2 = relClip.y2;
		    tmp2 -= tmp1; /* make tmp2 into height */
		    *p++ = tmp2; /* height */
		    if (tmp2 <= 0) p -= 4;
#else /* !NEW */
		    /* tmp1/tmp2 is left/right border, translated and clipped*/

		    tmp1 = parg->x + trans->x;
		    tmp2 = tmp1 + (int)parg->width;
		    if (tmp1 < pclip->x1) tmp1 = pclip->x1;
		    if (tmp2 > pclip->x2) tmp2 = pclip->x2;
		    tmp2 -= tmp1; /* make tmp2 into width */
		    if (tmp2 <= 0) continue;
#ifdef D_TILE
		    *p++ = tmp1 - trans->x; /* x (relative, clipped) */
#else
		    *p++ = tmp1; /* x (absolute, clipped) */
#endif
		    /*tmp1/tmp2 is upper/lower border, translated and clipped*/
		    tmp1 = parg->y + trans->y;
		    if (tmp1 < pclip->y1) tmp1 = pclip->y1;

#ifdef D_TILE
		    *p++ = tmp1 - trans->y; /* y (relative, clipped) */
#else
		    *p++ = tmp1; /* y (absolute, clipped) */
#endif
		    *p++ = tmp2; /* width (clipped) */
		    tmp2 = parg->y + trans->y + (int)parg->height;
		    if (tmp2 > pclip->y2) tmp2 = pclip->y2;
		    tmp2 -= tmp1; /* make tmp2 into height */
		    *p++ = tmp2; /* height */
		    if (tmp2 <= 0) p -= 4;
#endif /* !NEW */
		}  /* end: for each input rect */
		Confirm_dma();
	    } while (narg > 0); /* end: for each bunch of input rects */
	} /* end: for each clip rectangle */
    } /* end: if */
#endif
#ifdef D_TILE
    Need_dma(2);
    *p++ = JMPT_SETMASK;
    *p++ = 0xFFFF;
    Confirm_dma();
#endif
}
#undef	PROCNAME
