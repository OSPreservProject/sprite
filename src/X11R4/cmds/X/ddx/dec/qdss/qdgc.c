/***********************************************************
COPYRIGHT 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
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
#include "gcstruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "mibstore.h"

#include "qd.h"
#include "qdprocs.h"
#include "qdgc.h"

#include "mi.h"

/*
 * The following procedure declarations probably should be in mi.h
 */
extern PixmapPtr mfbCreatePixmap();
extern void miRecolorCursor();
extern void miImageGlyphBlt();

extern void miClearToBackground();
extern void miSaveAreas();
extern Bool miRestoreAreas();
extern void miTranslateBackingStore();

extern void miPolyFillRect();
extern void qdPixFillRect();
extern void miPolyFillArc();

extern RegionPtr miCopyArea();
extern RegionPtr miCopyPlane();
extern void miPolyPoint();
extern void miPutImage();

extern void miMiter();
extern void miNotMiter();
extern void miZeroLine();
extern void miPolySegment();
extern void tlPolySegment();
extern void tlPolySegmentDashed();
extern void tlPolylinesDashed();
extern void miPolyRectangle();
extern void miPolyArc();
extern void miFillPolygon();

extern void miImageText8(), miImageText16();
extern int miPolyText8(), miPolyText16();
extern void  miPolyArc();
extern void  miFillPolyArc();
#ifdef X11R4
extern void  qdUnnaturalPushPixels();
#else
extern void  miPushPixels();
#endif

#include "qdvalidate.h"

#ifdef X11R4
extern void qdSetSpansPix();
static GCOps SolidWinOps = {
    tlSolidSpans,
    qdSetSpansWin,
    qdWinPutImage,
    qdCopyAreaWin,
    qdCopyPlane,
    miPolyPoint,
    tlPolylines,
    tlPolySegment,
    miPolyRectangle,
    miZeroPolyArc,
    qdFillPolygon,
    tlSolidRects, /* dst */
    miPolyFillArc,
    tlPolyText, /* font, dst ? */
    miPolyText16,
    tlImageText, /* font, dst ? */
    miImageText16,
    miImageGlyphBlt,
    qdPolyGlyphBlt,
    qdPushPixels,
    miMiter, /* LineHelper */
    /* devPrivate.val implicitly set to 0 */
};
static GCOps SolidPixOps = {
    qdFSPixSolid,
    qdSetSpansPix,
    qdPixPutImage,
    qdCopyArea,
    qdCopyPlanePix,
    miPolyPoint,
    miZeroLine,
    miPolySegment,
    miPolyRectangle,
    miZeroPolyArc,
    miFillPolygon,
    qdPixFillRect,
    miPolyFillArc,
    qdPolyTextPix,
    miPolyText16,
    qdImageTextPix,
    miImageText16,
    miImageGlyphBlt,
    qdPolyGlyphBlt,
    qdPushPixels,
    miMiter, /* LineHelper */
    /* devPrivate.val implicitly set to 0 */
};
#endif

extern Bool qdNaturalSizePixmap();

struct _GC *InstalledGC;

#ifdef X11R4
void
#else
int
#endif
qdChangeGC(pGC, pQ, maskQ)
     GCPtr pGC;
     GCInterestPtr pQ;
     BITS32 maskQ;
{
    if (maskQ & GCDashList) {
	/* calculate length of dash list */
	int length = 0;
	register unsigned char *pDash = (unsigned char *)pGC->dash;
	register int i = pGC->numInDashList;
	while (--i >= 0)
	    length += (unsigned int) *pDash++;
	if (pGC->numInDashList & 1)
	    length += length;
	GC_DASH_LENGTH(pGC) = length;
	GC_DASH_PLANE(pGC) = -1;
    }
#ifndef X11R4
    else return BadImplementation;
    return Success;
#endif
}

#ifdef X11R4
void qdCopyGC();
static GCFuncs qdFuncs = {
    qdValidateGC,
    qdChangeGC,
    qdCopyGC,
    qdDestroyGC,
    qdChangeClip,
    qdDestroyClip,
    qdCopyClip,
};
#endif

Bool
qdCreateGC(pGC)
    GCPtr pGC;
{
#ifdef X11R4
    QDPrivGCPtr		qdPriv;
    mfbPrivGCPtr	mfbPriv;
    pGC->ops = &SolidWinOps;
    pGC->funcs = &qdFuncs;
#else
    QDPrivGCPtr		pPriv;
    GCInterestPtr	pQ;
#endif

    pGC->miTranslate = 0;    /* all qd output routines do window translation */
    pGC->clipOrg.x = pGC->clipOrg.y = 0;
    pGC->clientClip = (pointer)NULL;
    pGC->clientClipType = CT_NONE;

#ifdef X11R4
    qdPriv = (QDPrivGCPtr)pGC->devPrivates[qdGCPrivateIndex].ptr;
    mfbPriv = (mfbPrivGCPtr)pGC->devPrivates[mfbGCPrivateIndex].ptr;
    qdPriv->mask = QD_NEWLOGIC;
    qdPriv->ptresult = (unsigned char *) NULL;
    mfbPriv->freeCompClip = FALSE;
    qdPriv->lastDest = DRAWABLE_WINDOW;
    qdPriv->GCstate = VSFullReset;
    qdPriv->dashLength = 8;
    qdPriv->dashPlane = -1; /* invalid plane number */
#else
    pGC->ChangeClip = qdChangeClip;
    pGC->DestroyClip = qdDestroyClip;
    pGC->CopyClip = qdCopyClip;

    pPriv = (QDPrivGCPtr) Xalloc( sizeof(QDPrivGCRec));
    pPriv->mask = QD_NEWLOGIC;
    pPriv->ptresult = (unsigned char *) NULL;
    pPriv->mfb.pAbsClientRegion = (RegionPtr) NULL;
    pPriv->mfb.pCompositeClip = (RegionPtr) NULL;
    pPriv->mfb.freeCompClip = REPLACE_CC;
    pPriv->lastDest = 2;	/* impossible value */
    pPriv->GCstate = VSFullReset;
    pPriv->dashLength = 8;
    pPriv->dashPlane = -1; /* invalid plane number */
    pGC->devPriv = (pointer)pPriv;
    pGC->devBackingStore = (pointer)NULL;

    pQ = (GCInterestPtr) Xalloc(sizeof(GCInterestRec));
    if (!pQ)
    {
	Xfree(pPriv);
	return FALSE;
    }
     
    /*
     * Now link in this first GCInterest structure
     */
    pGC->pNextGCInterest = pQ;
    pGC->pLastGCInterest = pQ;
    pQ->pNextGCInterest = (GCInterestPtr) &pGC->pNextGCInterest;
    pQ->pLastGCInterest = (GCInterestPtr) &pGC->pNextGCInterest;
    pQ->length = sizeof(GCInterestRec);
    pQ->owner = 0;		/* server owns this */
    pQ->ValInterestMask = ~0;	/* interested in everything at validate time b*/
    pQ->ValidateGC = qdValidateGC;
    pQ->ChangeInterestMask = GCDashList; /* things interesting at change time*/
    pQ->ChangeGC = qdChangeGC;
    pQ->CopyGCSource = (void (*) () ) NULL;
    pQ->CopyGCDest = qdCopyGCDest;
    pQ->DestroyGC = qdDestroyGC;
#endif
    return TRUE;
}

void
#ifdef X11R4
qdDestroyGC( pGC)
#else
qdDestroyGC( pGC, pQ)
    GCInterestPtr       pQ;
#endif
    GCPtr		pGC;
{
#ifdef X11R4
    QDPrivGCPtr qdPriv = (QDPrivGCPtr)pGC->devPrivates[qdGCPrivateIndex].ptr;
    mfbPrivGCPtr mfbPriv =
	(mfbPrivGCPtr)pGC->devPrivates[mfbGCPrivateIndex].ptr;

    if (qdPriv->ptresult)
	Xfree(qdPriv->ptresult);

    if (mfbPriv->freeCompClip)
	(*pGC->pScreen->RegionDestroy)(mfbPriv->pCompositeClip);

    if (pGC->ops->devPrivate.val)
	Xfree(pGC->ops);
#else
    QDPrivGCPtr devPriv = (QDPrivGCPtr)pGC->devPriv;

    if (devPriv->ptresult)
	Xfree(devPriv->ptresult);

    if ( devPriv->mfb.pAbsClientRegion)
	(*pGC->pScreen->RegionDestroy)(
				devPriv->mfb.pAbsClientRegion);
    if (devPriv->mfb.freeCompClip == FREE_CC
     && devPriv->mfb.pCompositeClip)
	(*pGC->pScreen->RegionDestroy)(
				devPriv->mfb.pCompositeClip);
    Xfree( devPriv);
    Xfree( pQ);
#endif
    if (InstalledGC == pGC) InstalledGC = NULL;
}


void
qdDestroyClip( pGC)
    GCPtr	pGC;
{
    if ( pGC->clientClipType == CT_NONE)
	return;
    else if (pGC->clientClipType == CT_PIXMAP)
	qdDestroyPixmap((PixmapPtr)(pGC->clientClip));
    else
	(*pGC->pScreen->RegionDestroy)( (RegionPtr)pGC->clientClip);
    pGC->clientClip = (pointer)NULL;
    pGC->clientClipType = CT_NONE;
}

#ifdef X11R4
RegionPtr
qdBitmapToRegion(pPix)
    PixmapPtr pPix;
{
    tlCancelPixmap(pPix);
    return mfbPixmapToRegion(pPix);
}
#endif

void
qdChangeClip( pGC, type, pvalue, nrects)
    GCPtr	pGC;
    int		type;
    pointer	pvalue;
    int		nrects;
{
    qdDestroyClip(pGC);
    if(type == CT_PIXMAP)
    {
	tlCancelPixmap(pvalue);
	pGC->clientClip = (pointer) mfbPixmapToRegion(pvalue);
	(*pGC->pScreen->DestroyPixmap)(pvalue);
    }
    else if (type == CT_REGION)
    {
	pGC->clientClip = pvalue;
    }
    else if (type != CT_NONE)
    {
#ifdef X11R4
	pGC->clientClip = (pointer) miRectsToRegion(nrects, pvalue, type);
#else
	pGC->clientClip = (pointer) miRectsToRegion(pGC, nrects, pvalue, type);
#endif
	Xfree(pvalue);
    }
    pGC->clientClipType = (type != CT_NONE && pGC->clientClip) ? CT_REGION :
								 CT_NONE;
    pGC->stateChanges |= GCClipMask;
}

void
#ifdef X11R4
qdCopyGC( pgcSrc, maskQ, pgcDst)
#else
qdCopyGCDest( pgcDst, pQ, maskQ, pgcSrc)
    GCInterestPtr	pQ;
#endif
    GCPtr		pgcDst;
    int                 maskQ;
    GCPtr		pgcSrc;
{
#ifdef X11R4
    if (maskQ & GCClipMask) {
	if (pgcDst->clientClipType == CT_PIXMAP)
	    ((PixmapPtr)pgcDst->clientClip)->refcnt++;
	else if (pgcDst->clientClipType == CT_REGION) {
	    RegionPtr pClip = (RegionPtr) pgcDst->clientClip;
	    pgcDst->clientClip =
		(pointer)(* pgcDst->pScreen->RegionCreate)(NULL, 1);
	    (* pgcDst->pScreen->RegionCopy)(pgcDst->clientClip, pClip);
	}
    }
#else
    RegionPtr		pregionsrc = (RegionPtr) pgcSrc->clientClip;

    if ( ! (maskQ & GCClipMask)
	|| pgcDst->clientClipType != CT_PIXMAP)
	return;

    pgcDst->clientClip =
	(pointer) miRegionCreate(REGION_RECTS(pregionsrc),
				 REGION_NUM_RECTS(pregionsrc));
    miRegionCopy( (RegionPtr) pgcDst->clientClip, pregionsrc);
#endif
}

void
DoNothing()
{
}

int
IntDoNothing()
{
}

/*
 * Install a vector to mi code for polygon filling, so that the set of pixels
 * touched conforms to the protocol specification.
 * The default is to use the fast dragon polygons, violating the protocol
 * specification.
 */
#ifndef X11R4
static PFN usedFillPolygon = qdFillPolygon;
#endif
slowPolygons()	/* called from qdScreenInit */
{
#ifdef X11R4
    SolidWinOps.FillPolygon = miFillPolygon;
#else
    usedFillPolygon = miFillPolygon;
#endif
}

/*
 * Clipping conventions
 *	if the drawable is a window
 *	    CT_REGION ==> pCompositeClip really is the composite
 *	    CT_other ==> pCompositeClip is the window clip region
 *	if the drawable is a pixmap
 *	    CT_REGION ==> pCompositeClip is the translated client region
 *		clipped to the pixmap boundary
 *	    CT_other ==> pCompositeClip is the pixmap bounding box
 */
void
#ifdef X11R4
qdValidateGC( pGC, changes, pDrawable)
#else
qdValidateGC( pGC, pQ, changes, pDrawable)
    GCInterestPtr	pQ;
#endif
    register GCPtr	pGC;
    Mask		changes; /* this arg should equal pGC->stateChanges */
    DrawablePtr		pDrawable;
{
    mfbPrivGCPtr		devPriv;
    WindowPtr	pWin;
    RegionPtr	pReg;
    DDXPointRec	oldOrg;		/* origin of thing GC was last used with */
    unsigned long	procChanges = 0;	/* for mibstore */
    unsigned long	vdone = 0;	/* vector already validated */
	/* vdone is a field of VECMAX bits indicating validation complete */

    /* throw away offscreen pixmap if it about to be changed. */
    if (pDrawable->type == DRAWABLE_PIXMAP) {
	if (QDPIX_Y((QDPixPtr) pDrawable) == NOTOFFSCREEN)
	    tlConfirmPixmap((QDPixPtr)pDrawable);
	tlSinglePixmap((QDPixPtr)pDrawable);
    }

    oldOrg = pGC->lastWinOrg;

    if (InstalledGC == pGC) InstalledGC = NULL; /* overly conservative */

#ifdef X11R4
    pGC->lastWinOrg.x = pDrawable->x;
    pGC->lastWinOrg.y = pDrawable->y;
#endif
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pWin = (WindowPtr)pDrawable;
#ifndef X11R4
	pGC->lastWinOrg = pWin->absCorner;
#endif
    }
    else
    {
	pWin = (WindowPtr)NULL;
#ifndef X11R4
	if (pDrawable->type == DRAWABLE_PIXMAP)
	    pGC->lastWinOrg = ((QDPixPtr)pDrawable)->offscreen;
	else {
	    pGC->lastWinOrg.x = 0;
	    pGC->lastWinOrg.y = 0;
	}
#endif
    }
#ifdef X11R4
    devPriv = ((mfbPrivGCPtr) (pGC->devPrivates[mfbGCPrivateIndex].ptr));
#else
    devPriv = ((mfbPrivGCPtr) (pGC->devPriv));
#endif

    /*
     * if the client clip is different or moved OR the subwindowMode has
     * changed OR the window's clip has changed since the last validation
     * we need to recompute the composite clip 
     */

    if ((changes & 
	    (GCClipXOrigin | GCClipYOrigin | GCClipMask | GCSubwindowMode))
    ||(pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))) {
#ifndef X11R4
	/*
	 * if there is a client clip (always a region, for us) AND it has
	 * moved or is different OR the window has moved we need to
	 * (re)translate it. 
	 */
	if ((pGC->clientClipType == CT_REGION) &&
	    ((changes & (GCClipXOrigin | GCClipYOrigin | GCClipMask)) ||
	     (oldOrg.x != pGC->lastWinOrg.x) || (oldOrg.y != pGC->lastWinOrg.y)
	     )
	    ) {
	    /* retranslate client clip */
	    if ( ! devPriv->pAbsClientRegion)
		devPriv->pAbsClientRegion = miRegionCreate(NULL, 1);
	    miRegionCopy(devPriv->pAbsClientRegion,(RegionPtr)pGC->clientClip);

	    miTranslateRegion(devPriv->pAbsClientRegion,
			      pGC->lastWinOrg.x + pGC->clipOrg.x,
			      pGC->lastWinOrg.y + pGC->clipOrg.y);
	}
#endif /* X11R4 */
	if (pWin)
	{
	    int freeTmpClip, freeCompClip;
	    RegionPtr pregWin;	/* clip for this window, without client clip */

	    if (pGC->subWindowMode == IncludeInferiors)
	    {
	        pregWin = NotClippedByChildren( pWin);
#ifdef X11R4
		freeTmpClip = TRUE;
#else
		freeTmpClip = FREE_CC;		/* 1 */
#endif
	    }
	    else   /* ClipByChildren */
	    {
#ifdef X11R4
	        pregWin = &pWin->clipList;
		freeTmpClip = FALSE;
#else
	        pregWin = pWin->clipList;
		freeTmpClip = REPLACE_CC;	/* 0 */
#endif
	    }

	    freeCompClip = devPriv->freeCompClip;

	    /* 
	     * if there is no client clip, we can get by with
	     * just keeping the pointer we got, and remembering
	     * whether or not should destroy (or maybe re-use)
	     * it later.  this way, we avoid unnecessary copying
	     * of regions.  (this wins especially if many clients clip
	     * by children and have no client clip.)
	     */
	    if ( pGC->clientClipType == CT_NONE)
	    {
#ifdef X11R4
		if ( freeCompClip)
#else
		if ( freeCompClip == FREE_CC)
#endif
		    (*pGC->pScreen->RegionDestroy)( devPriv->pCompositeClip);
		devPriv->pCompositeClip = pregWin;
		devPriv->freeCompClip = freeTmpClip;
	    }
	    else	/* client clipping enabled */
	    {
		/*
		 * We need one 'real' region to put into the composite clip.
		 * If pregWin and the current composite clip 
		 *  are real, we can get rid of one.
		 * If pregWin is real and the current composite
		 *  clip isn't, use pregWin for the composite clip.
		 * If the current composite clip is real and
		 *  pregWin isn't, use the current composite clip.
		 * If neither is real, create a new region.
		 */
#ifdef X11R4
		miTranslateRegion(pGC->clientClip,
				  pDrawable->x + pGC->clipOrg.x,
				  pDrawable->y + pGC->clipOrg.y);
		if (!freeCompClip)
		    if (freeTmpClip)
			devPriv->pCompositeClip = pregWin;
		    else
			devPriv->pCompositeClip = miRegionCreate(NullBox, 0);
		miIntersect(
			    devPriv->pCompositeClip,
			    pregWin,
			    pGC->clientClip);
		if (freeCompClip && freeTmpClip)
		    (*pGC->pScreen->RegionDestroy)(pregWin);
                devPriv->freeCompClip = TRUE;
		miTranslateRegion(pGC->clientClip,
				  -(pDrawable->x + pGC->clipOrg.x),
				  -(pDrawable->y + pGC->clipOrg.y));
#else
		if (freeCompClip == REPLACE_CC)
		    if (freeTmpClip == FREE_CC)
			devPriv->pCompositeClip = pregWin;
		    else
			devPriv->pCompositeClip = miRegionCreate(NULL, 1);
		miIntersect(
			    devPriv->pCompositeClip,
			    pregWin,
			    devPriv->pAbsClientRegion);
		if (freeCompClip == FREE_CC && freeTmpClip == FREE_CC)
		    (*pGC->pScreen->RegionDestroy)(pregWin);
                devPriv->freeCompClip = FREE_CC;
#endif
	    }
	} /* end of composite clip for a window */
	else	/* output to a pixmap */
	{
	    BoxRec pixbounds;

	    pixbounds.x1 = QDPIX_X((QDPixPtr)pDrawable);
	    pixbounds.y1 = QDPIX_Y((QDPixPtr)pDrawable);
	    pixbounds.x2 = pixbounds.x1 + QDPIX_WIDTH((PixmapPtr)pDrawable);
	    pixbounds.y2 = pixbounds.y1 + QDPIX_HEIGHT((PixmapPtr)pDrawable);

#ifdef X11R4
	    if (devPriv->freeCompClip)
#else
	    if (devPriv->freeCompClip == FREE_CC)
#endif
	        miRegionReset( devPriv->pCompositeClip, &pixbounds);
	    else
	    {
#ifdef X11R4
		devPriv->freeCompClip = TRUE;
#else
		devPriv->freeCompClip = FREE_CC;
#endif
		devPriv->pCompositeClip = miRegionCreate(&pixbounds, 1);
	    }

	    if (pGC->clientClipType == CT_REGION)
	    {
		miTranslateRegion(devPriv->pCompositeClip,
				  -(pixbounds.x1 + pGC->clipOrg.x),
				  -(pixbounds.y1 + pGC->clipOrg.y));
		miIntersect(
			devPriv->pCompositeClip,
			devPriv->pCompositeClip,
#ifdef X11R4
			pGC->clientClip);
#else
			devPriv->pAbsClientRegion);
#endif
		miTranslateRegion(devPriv->pCompositeClip,
				  pixbounds.x1 + pGC->clipOrg.x,
				  pixbounds.y1 + pGC->clipOrg.y);
	    }
	} /* end of composite clip for pixmap */
    }

    /*
     * invalidate the version of this stipple in the off screen cache
     */


    if (pGC->stateChanges & GCStipple && pGC->stipple)
	tlSinglePixmap ((QDPixPtr)pGC->stipple);

#ifdef X11R4
    if ( pGC->stateChanges & GCTile && !pGC->tileIsPixel)
    	tlSinglePixmap ((QDPixPtr)pGC->tile.pixmap);
#else
    if ( pGC->stateChanges & GCTile && pGC->tile)
    	tlSinglePixmap ((QDPixPtr)pGC->tile);
#endif
	
    /* VALIDATION */
    {
#ifdef X11R4
	QDPrivGCPtr qdPriv =
	    (QDPrivGCPtr)pGC->devPrivates[qdGCPrivateIndex].ptr;
	int fillStyle; /* -1 if not natural size, else pGC->fillStyle */
	GCOps *ops;
	int new_fill = changes & GCFillStyle+GCTile+GCStipple;
	int new_line =
	    changes & GCLineWidth+GCLineStyle+GCJoinStyle+GCDashList; 
	int new_text = changes & GCFont;
	int fontKind;
	if (new_text) {
	    fontKind = QDCreateFont(pGC->pScreen,pGC->font);
	    if (fontKind == 2 && !pGC->ops->devPrivate.val)
	        new_text = 0;
	}

	if ( pGC->stateChanges &
	    (GCFunction|GCPlaneMask|GCForeground|GCBackground|GCFillStyle))
	    qdPriv->mask |= QD_NEWLOGIC;
	if (qdPriv->lastDest != pDrawable->type) {
	    ops = pDrawable->type == DRAWABLE_PIXMAP ? &SolidPixOps
		: &SolidWinOps;
	    if (pGC->ops->devPrivate.val) {
		new_fill = 1;
		new_line = 1;
		new_text = 1;
		*pGC->ops = *ops;
		pGC->ops->devPrivate.val = 1;
	    }
	    pGC->ops = ops;
	    qdPriv->lastDest = pDrawable->type;
	}
	if (new_fill | new_line | new_text) {
	    if (!pGC->ops->devPrivate.val) { /* allocate private ops */
		ops = (GCOps *) Xalloc (sizeof *ops);
		*ops = *pGC->ops;
		pGC->ops = ops;
		ops->devPrivate.val = 1;
	    }
	    ops = pGC->ops;
	    if (pGC->lineWidth != 0) {
	        ops->PolyArc = miPolyArc;
	    }
	    if (new_fill | new_line) {
		switch ( pGC->fillStyle) {
		  case FillSolid:
		    fillStyle = FillSolid;
		    break;
		  case FillOpaqueStippled:
		  case FillStippled:
		    if (qdNaturalSizePixmap(pGC->stipple))
			fillStyle = pGC->fillStyle;
		    else
			fillStyle = -1;
		    break;
		  case FillTiled:
		    fillStyle =
			qdNaturalSizePixmap(pGC->tile) ? FillTiled : -1;
		    break;
		}
		/* set op->FillSpans, op->PolyFillRect */
		if (pDrawable->type == DRAWABLE_WINDOW) {
		    static PFN WinFillSpanTab[5] = {
			qdWinFSOddSize,
			tlSolidSpans,
			tlTiledSpans,
			tlStipSpans,
			tlOpStipSpans};
		    static PFN WinPolyFillRectTab[5] = {
			qdPolyFillRectOddSize,
			tlSolidRects,
			tlTiledRects,
			tlStipRects,
			tlOpStipRects};
		    ops->FillSpans = WinFillSpanTab[fillStyle + 1];
		    ops->PolyFillRect = WinPolyFillRectTab[fillStyle + 1];
		}
		else if (pDrawable->type == DRAWABLE_PIXMAP) {
		    static PFN PixFillSpanTab[4] = {
			qdFSPixSolid,
			qdFSPixTiled,
			qdFSPixStippleorOpaqueStip,
			qdFSPixStippleorOpaqueStip};
		    ops->FillSpans = PixFillSpanTab[pGC->fillStyle];
		    if (fillStyle >= 0)
		        ops->PolyFillRect = qdPixFillRect;
		    else
		        ops->PolyFillRect = miPolyFillRect ;
		}
		else {
		    ops->FillSpans = qdFSUndrawable;
		    ops->PolyFillRect = miPolyFillRect;
		}
		/* set op->PushPixels */
		if (fillStyle < 0) /* unnatural size */
		    ops->PushPixels = qdUnnaturalPushPixels;
		else
		    ops->PushPixels = qdPushPixels;
	    }
	    /* set ops->Polylines, ops->PolySegment */
	    if (new_line | new_fill) {
		if (pGC->lineStyle != LineSolid) {
		    static PFN DoubleDashLineFuncs[4] = {
			tlPolylinesDashed,	/* Solid */
			tlPolylines,		/* Tiled */
			miWideDash,		/* Stippled */
			tlPolylines,		/* OpaqueStippled */
		    };
		    if (pGC->lineWidth != 0 || fillStyle < FillSolid
		      || pDrawable->type != DRAWABLE_WINDOW
		      || GC_DASH_LENGTH(pGC) > 1024) {
			ops->Polylines = (PFN)miWideDash;
		        ops->PolySegment = miPolySegment;
		    }
		    else if (pGC->lineStyle == LineOnOffDash) {
			ops->Polylines = tlPolylinesDashed;
			ops->PolySegment = tlPolySegmentDashed;
		    }
		    else {
		        static PFN DoubleDashSegmentFuncs[4] = {
			    tlPolySegmentDashed,	/* Solid */
			    tlPolySegment,		/* Tiled */
			    miPolySegment,		/* Stippled */
			    tlPolySegment,		/* OpaqueStippled */
			};
			ops->Polylines = DoubleDashLineFuncs[fillStyle];
			ops->PolySegment = DoubleDashSegmentFuncs[fillStyle];
		    }
		}
		else if (pGC->lineWidth != 0) {
		    ops->Polylines = (PFN)miWideLine;
		    ops->PolySegment = miPolySegment;
		}
		else if ((fillStyle < FillSolid )
		  || (pDrawable->type != DRAWABLE_WINDOW))
		    ops->Polylines = miZeroLine;
		    ops->PolySegment = miPolySegment;
		}
		else {
		    ops->Polylines = tlPolylines;
		    ops->PolySegment = tlPolySegment;
		}
	    }

	    if (new_fill | new_text) {
		if (fontKind && fillStyle >= 0) {
		    static int (* PolyTextFuncs[])() = {
			IntDoNothing, /* UNDRAWABLE_WINDOW */
			tlPolyText,	/* DRAWABLE_WINDOW */
			qdPolyTextPix };	/* DRAWABLE_PIXMAP */
                    ops->PolyText8 = PolyTextFuncs[pDrawable->type + 1];
		}
                else
		    ops->PolyText8 = miPolyText8;

		switch (fontKind) {
		    extern void qdImageTextKerned();
		    static PFN ImageTextFuncs[3] = {
			DoNothing, tlImageText, qdImageTextPix };
		    static PFN KernedTextFuncs[3] = {
			DoNothing, qdImageTextKerned, qdImageTextPix };
		  case 1:
		    ops->ImageText8 = KernedTextFuncs[pDrawable->type + 1];
		    break;
		  case 2:
                    ops->ImageText8 = ImageTextFuncs[pDrawable->type + 1];
		    break;
		  default:
		    ops->ImageText8 = miImageText8;
		}
	    }
	}
#else /* !X11R4 */
	register /* char */	ibit;	/* index of changed bit */
	register char	*plvec;	/* pointer to list of vector indexes */
	unsigned long	privState;
	int fillStyle; /* -1 if not natural size, else pGC->fillStyle */
	PFN	*gcVecs;
	/*
	 * this array maps between position in the GC and
	 * interest bits for miValidateBackingStore.  It
	 * is dependent on the ordering of functions in the GC
	 */
	static unsigned long miMap[32] = {
		MIBS_FILLSPANS, MIBS_SETSPANS, MIBS_PUTIMAGE,
		MIBS_COPYAREA, MIBS_COPYPLANE, MIBS_POLYPOINT,
		MIBS_POLYLINES, MIBS_POLYSEGMENT, MIBS_POLYRECTANGLE,
		MIBS_POLYARC, MIBS_FILLPOLYGON, MIBS_POLYFILLRECT,
		MIBS_POLYFILLARC, MIBS_POLYTEXT8, MIBS_POLYTEXT16,
		MIBS_IMAGETEXT8, MIBS_IMAGETEXT16,
		MIBS_IMAGEGLYPHBLT, MIBS_POLYGLYPHBLT,
		MIBS_PUSHPIXELS
	};

		/* cast function pointers to array entries of func list */
	gcVecs = (PFN *) (&pGC->FillSpans);
		/* tag private change bits on msb end of changes */
	if ( pGC->stateChanges &
	    (GCFunction|GCPlaneMask|GCForeground|GCBackground|GCFillStyle))
	    ((QDPrivGCPtr)devPriv)->mask |= QD_NEWLOGIC;

	privState = ((QDPrivGCPtr)devPriv)->GCstate
		    | ((((QDPrivGCPtr)devPriv)->lastDest == pDrawable->type ? 0
			: VSDest));
	switch ( pGC->fillStyle) {
	  case FillSolid:
	    fillStyle = FillSolid;
	    break;
	  case FillOpaqueStippled:
	  case FillStippled:
	    fillStyle= qdNaturalSizePixmap(pGC->stipple) ? pGC->fillStyle : -1;
	    break;
	  case FillTiled:
	    fillStyle = qdNaturalSizePixmap(pGC->tile) ? FillTiled : -1;
	    break;
	}

	switch (privState)
	{
	  case 0:
	  case VSFullReset:
	    break;
	  case VSDest:
	  case VSFullReset|VSDest:
	    ((QDPrivGCPtr)devPriv)->lastDest = pDrawable->type;
	    break;
	  default:
	    FatalError("weird qdss private validation factor, %ld\n",
		((QDPrivGCPtr)devPriv)->GCstate);
	    break;
	}
	pGC->stateChanges |= privState << (GCLastBit+1);
	((QDPrivGCPtr)devPriv)->GCstate = 0;
	for (ibit = 0; ibit < GCLastBit + VSNewBits + 1; ibit++)
	{
	    PFN gcFunc;
		/* step through change bits */
	  if (!(pGC->stateChanges & (1L<<ibit)))
	      continue;	/* change bit not set */
	  for (plvec = tlChangeVecs[ibit].pivec; *plvec != VECNULL; plvec++)
	  {
	    if (vdone & (1L<<(*plvec)))	/* if already validated */
		continue;
	    switch (*plvec) {
	      case VECFillSpans:
		if (pDrawable->type == DRAWABLE_WINDOW) {
		    static PFN WinFillSpanTab[5] = {
			(PFN)qdWinFSOddSize,
			(PFN)tlSolidSpans,
			(PFN)tlTiledSpans,
			(PFN)tlStipSpans,
			(PFN)tlOpStipSpans};
		    gcFunc = WinFillSpanTab[fillStyle + 1];
		}
		else if (pDrawable->type == DRAWABLE_PIXMAP) {
		    static PFN PixFillSpanTab[4] = {
			(PFN)qdFSPixSolid,
			(PFN)qdFSPixTiled,
			(PFN)qdFSPixStippleorOpaqueStip,
			(PFN)qdFSPixStippleorOpaqueStip};
		    gcFunc = PixFillSpanTab[pGC->fillStyle];
		}
		else
		    gcFunc = (PFN)qdFSUndrawable;
		break;
	      case VECSetSpans:
		if (pDrawable->type == DRAWABLE_WINDOW)
		    gcFunc = (PFN)qdSetSpansWin;
		else if (pDrawable->type == UNDRAWABLE_WINDOW)
		    gcFunc = (PFN)DoNothing;
		else if (pDrawable->depth == 1)
		    gcFunc = (PFN)qdSetSpansPix1;
		else
		    gcFunc = (PFN)qdSetSpansPixN;
		break;
	      case VECPutImage:
		{
		    static PFN PutImageFuncs[3] = {
			(PFN)DoNothing,		/* UNDRAWABLE_WINDOW */
			(PFN)qdWinPutImage,	/* DRAWABLE_WINDOW */
			(PFN)qdPixPutImage };	/* DRAWABLE_PIXMAP */
                    gcFunc = PutImageFuncs[pDrawable->type + 1];
		}
		break;
	      case VECCopyArea:
		{
		    static PFN CopyAreaFuncs[3] = {
			(PFN)DoNothing,		/* UNDRAWABLE_WINDOW */
			(PFN)qdCopyAreaWin,	/* DRAWABLE_WINDOW */
			(PFN)qdCopyArea };	/* DRAWABLE_PIXMAP */
                    gcFunc = CopyAreaFuncs[pDrawable->type + 1];
		}
		break;
	      case VECCopyPlane:
		{
		    static PFN CopyPlaneFuncs[3] = {
			(PFN)DoNothing,		/* UNDRAWABLE_WINDOW */
			(PFN)qdCopyPlane,	/* DRAWABLE_WINDOW */
			(PFN)qdCopyPlanePix };	/* DRAWABLE_PIXMAP */
                    gcFunc = CopyPlaneFuncs[pDrawable->type + 1];
		}
		break;
	      case VECPolyPoint:
		gcFunc = (PFN)miPolyPoint;
		break;
	      case VECPolylines:
		if (pGC->lineStyle != LineSolid) {
		    static PFN DoubleDashLineFuncs[4] = {
			tlPolylinesDashed,	/* Solid */
			tlPolylines,		/* Tiled */
			miWideDash,		/* Stippled */
			tlPolylines,		/* OpaqueStippled */
		    };
		    if (pGC->lineWidth != 0 || fillStyle < FillSolid
		     || pDrawable->type != DRAWABLE_WINDOW
		     || GC_DASH_LENGTH(pGC) > 1024)
			gcFunc = (PFN)miWideDash;
		    else if (pGC->lineStyle == LineOnOffDash)
			gcFunc = tlPolylinesDashed;
		    else
			gcFunc = DoubleDashLineFuncs[pGC->fillStyle];
		}
		else if (pGC->lineWidth != 0) gcFunc = (PFN)miWideLine;
		else if (fillStyle < FillSolid
		     || pDrawable->type != DRAWABLE_WINDOW)
		     gcFunc = (PFN)miZeroLine;
		else gcFunc = (PFN)tlPolylines;
		break;
	      case VECPolySegment:
		if (pGC->lineWidth != 0 || fillStyle < 0
		    || pDrawable->type != DRAWABLE_WINDOW)
		    gcFunc = miPolySegment;
		else if (pGC->lineStyle == LineSolid)
		    gcFunc = tlPolySegment;
		else if (GC_DASH_LENGTH(pGC) > 1024)
		    gcFunc = miPolySegment;
		else if (pGC->lineStyle == LineOnOffDash)
		    gcFunc = tlPolySegmentDashed;
		else {
		    static PFN DoubleDashSegmentFuncs[4] = {
			tlPolySegmentDashed,	/* Solid */
			tlPolySegment,		/* Tiled */
			miPolySegment,		/* Stippled */
			tlPolySegment,		/* OpaqueStippled */
		    };
		    gcFunc = DoubleDashSegmentFuncs[pGC->fillStyle];
		}
		break;
	      case VECPolyRectangle:
                gcFunc = miPolyRectangle;
		break;
	      case VECPolyArc:
                gcFunc = miPolyArc;
		break;
	      case VECFillPolygon:
		if (pDrawable->type == DRAWABLE_WINDOW)
		    gcFunc = usedFillPolygon;
		else gcFunc = (PFN)miFillPolygon;
		break;
	      case VECPolyFillRect:
		if (pDrawable->type == DRAWABLE_WINDOW) {
		    static PFN WinPolyFillRectTab[5] = {
			(PFN)qdPolyFillRectOddSize,
			(PFN)tlSolidRects,
			(PFN)tlTiledRects,
			(PFN)tlStipRects,
			(PFN)tlOpStipRects};
		    gcFunc = WinPolyFillRectTab[fillStyle + 1];
		}
		else if (fillStyle >= 0 && pDrawable->type == DRAWABLE_PIXMAP)
		    gcFunc = (PFN)qdPixFillRect;
		else
		    gcFunc = (PFN)miPolyFillRect;
		break;
	      case VECPolyFillArc:
                gcFunc = (PFN)miPolyFillArc;
		break;
	      case VECPolyText8:
		if (QDCreateFont(pGC->pScreen,pGC->font) && fillStyle >= 0) {
		    static PFN PolyTextFuncs[3] = {
			(PFN)DoNothing,	/* UNDRAWABLE_WINDOW */
			(PFN)tlPolyText,	/* DRAWABLE_WINDOW */
			(PFN)qdPolyTextPix };	/* DRAWABLE_PIXMAP */
                    gcFunc = PolyTextFuncs[pDrawable->type + 1];
		}
                else
		    gcFunc = (PFN)miPolyText8;
		break;
	      case VECPolyText16:
                gcFunc = (PFN)miPolyText16;
		break;
	      case VECImageText8:
		switch (QDCreateFont(pGC->pScreen,pGC->font)) {
		    extern void qdImageTextKerned();
		    static PFN ImageTextFuncs[3] = {
			DoNothing, tlImageText, qdImageTextPix };
		    static PFN KernedTextFuncs[3] = {
			DoNothing, qdImageTextKerned, qdImageTextPix };
		  case 1:
		    gcFunc = KernedTextFuncs[pDrawable->type + 1];
		    break;
		  case 2:
                    gcFunc = ImageTextFuncs[pDrawable->type + 1];
		    break;
		  default:
		    gcFunc = miImageText8;
		}
		break;
	      case VECImageText16:
                gcFunc = (PFN)miImageText16;
		break;
	      case VECImageGlyphBlt:
                gcFunc = (PFN)miImageGlyphBlt;
		break;
	      case VECPolyGlyphBlt:
                gcFunc = (PFN)qdPolyGlyphBlt;
		break;
	      case VECPushPixels:
		if (fillStyle < 0) /* unnatural size */
		    gcFunc = (PFN)miPushPixels;
		else
		    gcFunc = (PFN)qdPushPixels;
		break;
	      case VECLineHelper:
		if (pGC->joinStyle == JoinMiter) gcFunc = (PFN)miMiter;
		else gcFunc = (PFN)miNotMiter;    
		break;
	    }
	    gcVecs[*plvec] = gcFunc;
	    procChanges |= miMap[*plvec];
	    vdone |= 1L<<(*plvec);	/* indicate valid done */
	  }	/* for changed vectors */
	}	/* for GC + devPriv changed fields */
    }

#endif /* !X11R4 */

#ifndef X11R4
    /*
     * If this GC has ever been used with a window with backing-store enabled,
     * we must call miVaidateBackingStore to keep the backing-store module
     * up-to-date, should this GC be used with that drawable again. In addition,
     * if the current drawable is a window and has backing-store enabled, we
     * also call miValidateBackingStore to give it a chance to get its hooks in.
     */
    if (pGC->devBackingStore ||
	(pWin && (pWin->backingStore != NotUseful)))
    {
	miValidateBackingStore(pDrawable, pGC, procChanges);
    }
#endif
}

void
qdCopyClip( pgcDst, pgcSrc)
    GCPtr	pgcDst, pgcSrc;
{
    RegionPtr	prgnNew;

    switch( pgcSrc->clientClipType)
    {
      case CT_PIXMAP:
	((PixmapPtr) pgcSrc->clientClip)->refcnt++;
	/* Fall through !! */
      case CT_NONE:
        qdChangeClip( pgcDst, pgcSrc->clientClipType, pgcSrc->clientClip, 0);
        break;
      case CT_REGION:
        prgnNew = (* pgcSrc->pScreen->RegionCreate)(NULL, 1);
        (* pgcSrc->pScreen->RegionCopy)(prgnNew,
                                       (RegionPtr)(pgcSrc->clientClip));
        qdChangeClip( pgcDst, CT_REGION, prgnNew, 0);
        break;
    }
}

void
qdClipMoved(pGC, x, y)
    GCPtr pGC;
    int x, y;
{
    int xDelta = x - pGC->lastWinOrg.x;
    int yDelta = y - pGC->lastWinOrg.y;
    pGC->lastWinOrg.x = x;
    pGC->lastWinOrg.y = y;
    miTranslateRegion(QDGC_COMPOSITE_CLIP(pGC), xDelta, yDelta);
#ifndef X11R4
    if (((mfbPrivGCPtr)pGC->devPriv)->pAbsClientRegion)
	miTranslateRegion(((mfbPrivGCPtr)pGC->devPriv)->pAbsClientRegion,
			  xDelta, yDelta);
#endif
}
