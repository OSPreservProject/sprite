/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
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

/* $XConsortium: cfbgc.c,v 5.32.1.1 90/03/21 10:16:47 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "region.h"

#include "cfb.h"
#include "mistruct.h"
#include "mibstore.h"

#include "cfbmskbits.h"
#include "cfb8bit.h"

static void cfbValidateGC(), cfbChangeGC(), cfbCopyGC(), cfbDestroyGC();
static void cfbChangeClip(), cfbDestroyClip(), cfbCopyClip();
static cfbDestroyOps();

static GCFuncs cfbFuncs = {
    cfbValidateGC,
    cfbChangeGC,
    cfbCopyGC,
    cfbDestroyGC,
    cfbChangeClip,
    cfbDestroyClip,
    cfbCopyClip,
};

static GCOps	cfbTEOps = {
    cfbSolidFS,
    cfbSetSpans,
    cfbPutImage,
    cfbCopyArea,
    cfbCopyPlane,
    cfbPolyPoint,
    cfbLineSS,
    cfbSegmentSS,
    miPolyRectangle,
#if PPW == 4
    cfbZeroPolyArcSS8Copy,
#else
    miZeroPolyArc,
#endif
    miFillPolygon,
    cfbPolyFillRect,
    cfbPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
#if PPW == 4
    cfbTEGlyphBlt8,
    cfbPolyGlyphBlt8,
    cfbPushPixels8,
#else
    cfbTEGlyphBlt,
    miPolyGlyphBlt,
    mfbPushPixels,
#endif
    NULL,
};

static GCOps	cfbNonTEOps = {
    cfbSolidFS,
    cfbSetSpans,
    cfbPutImage,
    cfbCopyArea,
    cfbCopyPlane,
    cfbPolyPoint,
    cfbLineSS,
    cfbSegmentSS,
    miPolyRectangle,
#if PPW == 4
    cfbZeroPolyArcSS8Copy,
#else
    miZeroPolyArc,
#endif
    miFillPolygon,
    cfbPolyFillRect,
    cfbPolyFillArcSolidCopy,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    miImageGlyphBlt,
#if PPW == 4
    cfbPolyGlyphBlt8,
    cfbPushPixels8,
#else
    miPolyGlyphBlt,
    mfbPushPixels,
#endif
    NULL,
};

static GCOps *
matchCommon (pGC)
    GCPtr   pGC;
{
    if (pGC->lineWidth != 0)
	return 0;
    if (pGC->lineStyle != LineSolid)
	return 0;
    if (pGC->fillStyle != FillSolid)
	return 0;
    if ((pGC->alu != GXcopy) || ((pGC->planemask & PMSK) != PMSK))
	return 0;
    if (pGC->font &&
	(pGC->font->pFI->maxbounds.metrics.rightSideBearing -
         pGC->font->pFI->minbounds.metrics.leftSideBearing) <= 32)
    {
	if (pGC->font->pFI->terminalFont
#if PPW == 4
	    && pGC->font->pFI->maxbounds.metrics.characterWidth >= 4
#endif
	)
	    return &cfbTEOps;
	else
	    return &cfbNonTEOps;
    }
    return 0;
}

Bool
cfbCreateGC(pGC)
    register GCPtr pGC;
{
    cfbPrivGC  *pPriv;

    switch (pGC->depth) {
    case 1:
	return (mfbCreateGC(pGC));
    case PSZ:
	break;
    default:
	ErrorF("cfbCreateGC: unsupported depth: %d\n", pGC->depth);
	return FALSE;
    }
    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;

    /*
     * some of the output primitives aren't really necessary, since they
     * will be filled in ValidateGC because of dix/CreateGC() setting all
     * the change bits.  Others are necessary because although they depend
     * on being a color frame buffer, they don't change 
     */

    pGC->ops = &cfbNonTEOps;
    pGC->funcs = &cfbFuncs;

    /* cfb wants to translate before scan conversion */
    pGC->miTranslate = 1;

    pPriv = (cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr);
    pPriv->rop = pGC->alu;
    pPriv->fExpose = TRUE;
    pPriv->freeCompClip = FALSE;
    pPriv->pRotatedPixmap = (PixmapPtr) NULL;
    return TRUE;
}

/*ARGSUSED*/
static void
cfbChangeGC(pGC, mask)
    GC		    *pGC;
    BITS32	    mask;
{
    return;
}

static void
cfbDestroyGC(pGC)
    GC 			*pGC;
{
    cfbPrivGC *pPriv;

    pPriv = (cfbPrivGC *)(pGC->devPrivates[cfbGCPrivateIndex].ptr);
    if (pPriv->freeCompClip)
	(*pGC->pScreen->RegionDestroy)(pPriv->pCompositeClip);
    cfbDestroyOps (pGC->ops);
}

/*
 * create a private op array for a gc
 */

static GCOps *
cfbCreateOps (prototype)
    GCOps	*prototype;
{
    GCOps	*ret;
    extern Bool	Must_have_memory;

    /* XXX */ Must_have_memory = TRUE;
    ret = (GCOps *) xalloc (sizeof(GCOps));
    /* XXX */ Must_have_memory = FALSE;
    if (!ret)
	return 0;
    *ret = *prototype;
    ret->devPrivate.val = 1;
    return ret;
}

static
cfbDestroyOps (ops)
    GCOps   *ops;
{
    if (ops->devPrivate.val)
	xfree (ops);
}

/* Clipping conventions
	if the drawable is a window
	    CT_REGION ==> pCompositeClip really is the composite
	    CT_other ==> pCompositeClip is the window clip region
	if the drawable is a pixmap
	    CT_REGION ==> pCompositeClip is the translated client region
		clipped to the pixmap boundary
	    CT_other ==> pCompositeClip is the pixmap bounding box
*/

static void
cfbValidateGC(pGC, changes, pDrawable)
    register GCPtr  pGC;
    Mask	    changes;
    DrawablePtr	    pDrawable;
{
    WindowPtr   pWin;
    int         mask;		/* stateChanges */
    int         index;		/* used for stepping through bitfields */
    int         new_line, new_text, new_fillspans, new_fillrct;
    int		new_rotate;
    int		xrot, yrot;
    /* flags for changing the proc vector */
    cfbPrivGCPtr devPriv;

    new_rotate = pGC->lastWinOrg.x != pDrawable->x ||
		 pGC->lastWinOrg.y != pDrawable->y;

    pGC->lastWinOrg.x = pDrawable->x;
    pGC->lastWinOrg.y = pDrawable->y;
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pWin = (WindowPtr) pDrawable;
    }
    else
    {
	pWin = (WindowPtr) NULL;
    }

    devPriv = ((cfbPrivGCPtr) (pGC->devPrivates[cfbGCPrivateIndex].ptr));

    /*
     * if the client clip is different or moved OR the subwindowMode has
     * changed OR the window's clip has changed since the last validation
     * we need to recompute the composite clip 
     */

    if ((changes & (GCClipXOrigin|GCClipYOrigin|GCClipMask|GCSubwindowMode)) ||
	(pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))
	)
    {
	ScreenPtr pScreen = pGC->pScreen;

	if (pWin) {
	    RegionPtr   pregWin;
	    Bool        freeTmpClip, freeCompClip;

	    if (pGC->subWindowMode == IncludeInferiors) {
		pregWin = NotClippedByChildren(pWin);
		freeTmpClip = TRUE;
	    }
	    else {
		pregWin = &pWin->clipList;
		freeTmpClip = FALSE;
	    }
	    freeCompClip = devPriv->freeCompClip;

	    /*
	     * if there is no client clip, we can get by with just keeping
	     * the pointer we got, and remembering whether or not should
	     * destroy (or maybe re-use) it later.  this way, we avoid
	     * unnecessary copying of regions.  (this wins especially if
	     * many clients clip by children and have no client clip.) 
	     */
	    if (pGC->clientClipType == CT_NONE) {
		if (freeCompClip)
		    (*pScreen->RegionDestroy) (devPriv->pCompositeClip);
		devPriv->pCompositeClip = pregWin;
		devPriv->freeCompClip = freeTmpClip;
	    }
	    else {
		/*
		 * we need one 'real' region to put into the composite
		 * clip. if pregWin the current composite clip are real,
		 * we can get rid of one. if pregWin is real and the
		 * current composite clip isn't, use pregWin for the
		 * composite clip. if the current composite clip is real
		 * and pregWin isn't, use the current composite clip. if
		 * neither is real, create a new region. 
		 */

		(*pScreen->TranslateRegion)(pGC->clientClip,
					    pDrawable->x + pGC->clipOrg.x,
					    pDrawable->y + pGC->clipOrg.y);
						  
		if (freeCompClip)
		{
		    (*pGC->pScreen->Intersect)(devPriv->pCompositeClip,
					       pregWin, pGC->clientClip);
		    if (freeTmpClip)
			(*pScreen->RegionDestroy)(pregWin);
		}
		else if (freeTmpClip)
		{
		    (*pScreen->Intersect)(pregWin, pregWin, pGC->clientClip);
		    devPriv->pCompositeClip = pregWin;
		}
		else
		{
		    devPriv->pCompositeClip = (*pScreen->RegionCreate)(NullBox,
								       0);
		    (*pScreen->Intersect)(devPriv->pCompositeClip,
					  pregWin, pGC->clientClip);
		}
		devPriv->freeCompClip = TRUE;
		(*pScreen->TranslateRegion)(pGC->clientClip,
					    -(pDrawable->x + pGC->clipOrg.x),
					    -(pDrawable->y + pGC->clipOrg.y));
						  
	    }
	}			/* end of composite clip for a window */
	else {
	    BoxRec      pixbounds;

	    /* XXX should we translate by drawable.x/y here ? */
	    pixbounds.x1 = 0;
	    pixbounds.y1 = 0;
	    pixbounds.x2 = pDrawable->width;
	    pixbounds.y2 = pDrawable->height;

	    if (devPriv->freeCompClip)
		(*pScreen->RegionReset)(devPriv->pCompositeClip, &pixbounds);
	    else {
		devPriv->freeCompClip = TRUE;
		devPriv->pCompositeClip = (*pScreen->RegionCreate)(&pixbounds,
								   1);
	    }

	    if (pGC->clientClipType == CT_REGION)
	    {
		(*pScreen->TranslateRegion)(devPriv->pCompositeClip,
					    -pGC->clipOrg.x, -pGC->clipOrg.y);
		(*pScreen->Intersect)(devPriv->pCompositeClip,
				      devPriv->pCompositeClip,
				      pGC->clientClip);
		(*pScreen->TranslateRegion)(devPriv->pCompositeClip,
					    pGC->clipOrg.x, pGC->clipOrg.y);
	    }
	}			/* end of composute clip for pixmap */
    }

    new_line = FALSE;
    new_text = FALSE;
    new_fillspans = FALSE;
    new_fillrct = FALSE;

    mask = changes;
    while (mask) {
	index = lowbit (mask);
	mask &= ~index;

	/*
	 * this switch acculmulates a list of which procedures might have
	 * to change due to changes in the GC.  in some cases (e.g.
	 * changing one 16 bit tile for another) we might not really need
	 * a change, but the code is being paranoid. this sort of batching
	 * wins if, for example, the alu and the font have been changed,
	 * or any other pair of items that both change the same thing. 
	 */
	switch (index) {
	case GCFunction:
	    new_fillrct = TRUE;
	    if (!pGC->lineWidth)
		new_line = TRUE;
	case GCForeground:
	    new_text = TRUE;
	    break;
	case GCPlaneMask:
	    new_text = TRUE;
	    new_fillrct = TRUE;
	    if (!pGC->lineWidth)
		new_line = TRUE;
	    break;
	case GCBackground:
	    new_fillspans = TRUE;
	    break;
	case GCLineStyle:
	case GCLineWidth:
	case GCJoinStyle:
	    new_line = TRUE;
	    break;
	case GCCapStyle:
	    break;
	case GCFillStyle:
	    new_text = TRUE;
	    new_fillspans = TRUE;
	    new_line = TRUE;
	    new_fillrct = TRUE;
	    break;
	case GCFillRule:
	    break;
	case GCTile:
	    new_fillspans = TRUE;
	    new_fillrct = TRUE;
	    break;

	case GCStipple:
	    if (pGC->stipple)
	    {
		int width = pGC->stipple->drawable.width;
		PixmapPtr nstipple;

		if ((width <= 32) && !(width & (width - 1)) &&
		    (nstipple = cfbCopyPixmap(pGC->stipple)))
		{
		    cfbPadPixmap(nstipple);
		    cfbDestroyPixmap(pGC->stipple);
		    pGC->stipple = nstipple;
		}
	    }
	    new_fillspans = TRUE;
	    new_fillrct = TRUE;
	    break;

	case GCTileStipXOrigin:
	    new_rotate = TRUE;
	    break;

	case GCTileStipYOrigin:
	    new_rotate = TRUE;
	    break;

	case GCFont:
	    new_text = TRUE;
	    break;
	case GCSubwindowMode:
	    break;
	case GCGraphicsExposures:
	    break;
	case GCClipXOrigin:
	    break;
	case GCClipYOrigin:
	    break;
	case GCClipMask:
	    break;
	case GCDashOffset:
	    break;
	case GCDashList:
	    break;
	case GCArcMode:
	    break;
	default:
	    break;
	}
    }

    /*
     * If the drawable has changed,  check its depth & ensure suitable
     * entries are in the proc vector. 
     */
    if (pDrawable->serialNumber != (pGC->serialNumber & (DRAWABLE_SERIAL_BITS))) {
	new_fillspans = TRUE;	/* deal with FillSpans later */
    }

    if (new_rotate || new_fillspans)
    {
	Bool new_pix = FALSE;

	xrot = pGC->patOrg.x + pDrawable->x;
	yrot = pGC->patOrg.y + pDrawable->y;

	switch (pGC->fillStyle)
	{
	case FillTiled:
	    if (!pGC->tileIsPixel)
	    {
		int width = pGC->tile.pixmap->drawable.width * PSZ;

		if ((width <= 32) && !(width & (width - 1)))
		{
		    cfbCopyRotatePixmap(pGC->tile.pixmap,
					&devPriv->pRotatedPixmap,
					xrot, yrot);
		    new_pix = TRUE;
		}
	    }
	    break;
#if (PPW == 4)
	case FillStippled:
	case FillOpaqueStippled:
	    {
		int width = pGC->stipple->drawable.width;

		if ((width <= 32) && !(width & (width - 1)))
		{
		    mfbCopyRotatePixmap(pGC->stipple,
					&devPriv->pRotatedPixmap, xrot, yrot);
		    new_pix = TRUE;
		}
	    }
	    break;
#endif
	}
	if (!new_pix && devPriv->pRotatedPixmap)
	{
	    cfbDestroyPixmap(devPriv->pRotatedPixmap);
	    devPriv->pRotatedPixmap = (PixmapPtr) NULL;
	}
    }

    if (new_line || new_fillspans || new_text || new_fillrct)
    {
	GCOps	*newops;

	if (newops = matchCommon (pGC))
 	{
	    if (pGC->ops->devPrivate.val)
		cfbDestroyOps (pGC->ops);
	    pGC->ops = newops;
	    new_line = new_fillspans = new_text = new_fillrct = 0;
	}
 	else
 	{
	    if (!pGC->ops->devPrivate.val)
	    {
		pGC->ops = cfbCreateOps (pGC->ops);
		pGC->ops->devPrivate.val = 1;
	    }
	}
    }

    /* deal with the changes we've collected */

    if (new_line)
    {
	pGC->ops->PolySegment = miPolySegment;
	if (pGC->lineWidth == 0)
	{
#if PPW == 4
	    if ((pGC->lineStyle == LineSolid) && (pGC->fillStyle == FillSolid)
		&& (pGC->alu == GXcopy) && ((pGC->planemask & PMSK) == PMSK))
		pGC->ops->PolyArc = cfbZeroPolyArcSS8Copy;
	    else
#endif
		pGC->ops->PolyArc = miZeroPolyArc;
	}
	else
	    pGC->ops->PolyArc = miPolyArc;
	switch (pGC->lineStyle)
	{
	case LineSolid:
	    if(pGC->lineWidth == 0)
	    {
		if (pGC->fillStyle == FillSolid)
		{
		    pGC->ops->Polylines = cfbLineSS;
		    pGC->ops->PolySegment = cfbSegmentSS;
		}
 		else
		    pGC->ops->Polylines = miZeroLine;
	    }
	    else
		pGC->ops->Polylines = miWideLine;
	    break;
	case LineOnOffDash:
	case LineDoubleDash:
	    if (pGC->lineWidth == 0 && pGC->fillStyle == FillSolid)
	    {
		pGC->ops->Polylines = cfbLineSD;
		pGC->ops->PolySegment = cfbSegmentSD;
	    } else
		pGC->ops->Polylines = miWideDash;
	    break;
	}
    }

    if (new_text && (pGC->font))
    {
        if ((pGC->font->pFI->maxbounds.metrics.rightSideBearing -
             pGC->font->pFI->minbounds.metrics.leftSideBearing) > 32)
        {
            pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
            pGC->ops->ImageGlyphBlt = miImageGlyphBlt;
        }
        else
        {
#if PPW == 4
	    if (pGC->alu == GXcopy &&
		(pGC->planemask & PMSK) == PMSK &&
		pGC->fillStyle == FillSolid)
	    {
		pGC->ops->PolyGlyphBlt = cfbPolyGlyphBlt8;
	    }
	    else
#endif
		pGC->ops->PolyGlyphBlt = miPolyGlyphBlt;
            /* special case ImageGlyphBlt for terminal emulator fonts */
            if (pGC->font->pFI->terminalFont
#if PPW == 4
		&& pGC->font->pFI->maxbounds.metrics.characterWidth >= 4
#endif
		)
	    {
#if PPW == 4
                pGC->ops->ImageGlyphBlt = cfbTEGlyphBlt8;
#else
                pGC->ops->ImageGlyphBlt = cfbTEGlyphBlt;
#endif
	    }
            else
                pGC->ops->ImageGlyphBlt = miImageGlyphBlt;
        }
    }    


    if (new_fillspans) {
	switch (pGC->fillStyle) {
	case FillSolid:
	    pGC->ops->FillSpans = cfbSolidFS;
	    break;
	case FillTiled:
	    pGC->ops->FillSpans = cfbUnnaturalTileFS;
	    break;
	case FillStippled:
	    pGC->ops->FillSpans = cfbUnnaturalStippleFS;
	    break;
	case FillOpaqueStippled:
	    if (pGC->fgPixel == pGC->bgPixel)
		pGC->ops->FillSpans = cfbSolidFS;
	    else
		pGC->ops->FillSpans = cfbUnnaturalStippleFS;
	    break;
	default:
	    FatalError("cfbValidateGC: illegal fillStyle\n");
	}
    } /* end of new_fillspans */

    if (new_fillrct) {
	pGC->ops->PolyFillRect = miPolyFillRect;
	pGC->ops->PolyFillArc = miPolyFillArc;
	pGC->ops->PushPixels = mfbPushPixels;
	switch (pGC->fillStyle)
	{
	case FillSolid:
	    if (((pGC->alu == GXcopy || pGC->alu == GXxor) &&
		((pGC->planemask & PMSK) == PMSK)) ||
		pGC->alu == GXinvert)
	    {
		pGC->ops->PolyFillRect = cfbPolyFillRect;
	    }
	    if (pGC->alu == GXcopy &&
		((pGC->planemask & PMSK) == PMSK))
	    {
		pGC->ops->PushPixels = cfbPushPixels8;
		pGC->ops->PolyFillArc = cfbPolyFillArcSolidCopy;
	    }
	    break;
	case FillTiled:
	    if (pGC->alu == GXcopy && (pGC->planemask & PMSK) == PMSK)
	    {
		pGC->ops->PolyFillRect = cfbPolyFillRect;
	    }
	    break;
#if (PPW == 4)
	case FillStippled:
	case FillOpaqueStippled:
	    if (pGC->alu == GXcopy && (pGC->planemask & PMSK) == PMSK &&
	        devPriv->pRotatedPixmap)
	    {
		pGC->ops->PolyFillRect = cfbPolyFillRect;
	    }
	    break;
#endif
	}
    }
}

static void
cfbDestroyClip(pGC)
    GCPtr	pGC;
{
    if(pGC->clientClipType == CT_NONE)
	return;
    else if (pGC->clientClipType == CT_PIXMAP)
    {
	cfbDestroyPixmap((PixmapPtr)(pGC->clientClip));
    }
    else
    {
	/* we know we'll never have a list of rectangles, since
	   ChangeClip immediately turns them into a region 
	*/
        (*pGC->pScreen->RegionDestroy)(pGC->clientClip);
    }
    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;
}

static void
cfbChangeClip(pGC, type, pvalue, nrects)
    GCPtr	pGC;
    int		type;
    pointer	pvalue;
    int		nrects;
{
    cfbDestroyClip(pGC);
    if(type == CT_PIXMAP)
    {
	pGC->clientClip = (pointer) (*pGC->pScreen->BitmapToRegion)((PixmapPtr)pvalue);
	(*pGC->pScreen->DestroyPixmap)(pvalue);
    }
    else if (type == CT_REGION) {
	/* stuff the region in the GC */
	pGC->clientClip = pvalue;
    }
    else if (type != CT_NONE)
    {
	pGC->clientClip = (pointer) (*pGC->pScreen->RectsToRegion)(nrects,
						    (xRectangle *)pvalue,
						    type);
	xfree(pvalue);
    }
    pGC->clientClipType = (type != CT_NONE && pGC->clientClip) ? CT_REGION :
								 CT_NONE;
    pGC->stateChanges |= GCClipMask;
}

static void
cfbCopyClip (pgcDst, pgcSrc)
    GCPtr pgcDst, pgcSrc;
{
    RegionPtr prgnNew;

    switch(pgcSrc->clientClipType)
    {
      case CT_PIXMAP:
	((PixmapPtr) pgcSrc->clientClip)->refcnt++;
	/* Fall through !! */
      case CT_NONE:
        cfbChangeClip(pgcDst, (int)pgcSrc->clientClipType, pgcSrc->clientClip,
		      0);
        break;
      case CT_REGION:
        prgnNew = (*pgcSrc->pScreen->RegionCreate)(NULL, 1);
        (*pgcSrc->pScreen->RegionCopy)(prgnNew,
                                       (RegionPtr)(pgcSrc->clientClip));
        cfbChangeClip(pgcDst, CT_REGION, (pointer)prgnNew, 0);
        break;
    }
}

/*ARGSUSED*/
static void
cfbCopyGC (pGCSrc, changes, pGCDst)
    GCPtr	pGCSrc;
    Mask 	changes;
    GCPtr	pGCDst;
{
    return;
}
