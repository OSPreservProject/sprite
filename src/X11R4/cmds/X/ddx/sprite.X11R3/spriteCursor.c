/*-
 * spriteCursor.c --
 *	Functions for maintaining the Sun software cursor...
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] =
"$Header: /mic/X11R3/src/cmds/Xsp/ddx/sprite/RCS/spriteCursor.c,v 1.12 89/11/18 20:57:19 tve Exp $ SPRITE (Berkeley)";
#endif lint
/*
 * A cursor can be in one of 3 states:
 *	IN  	  It is actually in the frame buffer.
 *	OUT 	  It is not in the frame buffer and the screen should be
 *	    	  consulted for new screenBits before it is replaced.
 *	XING	  It is changing state. Used when calling mfb functions to
 *	    	  play with the cursor. Avoids infinite recursion, you know.
 *	
 * Two references for cursor coordinates are being used in this code. 
 * The outside world (dix code) and most of spriteCursors use the cursor 
 * hot-spot as a reference for the cursor's location. spriteCursor internal 
 * code uses the top-left corner of the cursor glyph as a reference 
 * for painting operations. 
 * 
 */

#define NEED_EVENTS	    	/* For SetCursorPosition */
#include    "spriteddx.h"
#include    <windowstr.h>
#include    <regionstr.h>
#include    <dix.h>
#include    <dixstruct.h>
#include    <opaque.h>
#include    <servermd.h>

static CursorPtr  currentCursor = NullCursor;	/* Cursor being displayed */
static BoxRec  	  currentLimits;		/* Box w/in which the hot spot
						 * must stay. */
/*
 * There are four window functions which bypass the usual GC validation
 * path (PaintWindow{Background,Border}, CopyWindow & ClearToBackground)
 * so we must go out of
 * our way to protect the cursor from them. This is accomplished by
 * intercepting the two screen calls which change the window vectors so
 * we can note when they do and substitute our own function which figures
 * out what's going to be nuked and makes sure the cursor isn't there.
 * The structure for the window is tracked in a somewhat sneaky way:
 * we create a new resource class (not type) and use that to associate
 * the WinPrivRec with the window (using the window's id) in the resource
 * table. This makes it easy to find and has the added benefit of freeing
 * the private data when the window is destroyed.
 */
typedef struct {
    void	(*PaintWindowBackground)();
    void	(*PaintWindowBorder)();
    void	(*CopyWindow)();
    void    	(*SaveAreas)();
    RegionPtr   (*RestoreAreas)();
} WinPrivRec, *WinPrivPtr;

static int	wPrivClass;		/* Resource class for icky private
					 * window structure (WinPrivRec)
					 * needed to protect the cursor
					 * from background/border paintings */

/*-
 *-----------------------------------------------------------------------
 * spriteInitCursor --
 *	Initialize the cursor module...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	currentCursor is set to NullCursor.
 *
 *-----------------------------------------------------------------------
 */
void
spriteInitCursor ()
{
    currentCursor = NullCursor;
    wPrivClass = CreateNewResourceClass();
}

/*-
 *-----------------------------------------------------------------------
 * spriteStencil --
 *	Return the data for a bitmap made by squishing the source bitmap
 *	data through the mask bitmap data. If invert is TRUE, the source
 *	data are inverted before being squished through.
 *
 * Results:
 *	An array of data suitable for passing to PutImage
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static unsigned char *
spriteStencil (source, mask, w, h, invert)
    register unsigned char *source;  	/* Source data */
    register unsigned char *mask;    	/* Mask data */
    int	    	  w;	    	/* Width of both */
    int	    	  h;	    	/* Height of both */
    Bool    	  invert;   	/* Invert source before squishing */
{
    unsigned char    	  *result;
    register unsigned char *r;
    register int  nbytes;

    nbytes = h * PixmapBytePad (w, 1);
    result = (unsigned char *)Xalloc(nbytes);

    for (r = result; nbytes--; source++, r++) {
	*r = (invert ? ~ *source : *source) & (mask ? *mask++ : ~0);
    }

    return (result);
}

/*-
 *-----------------------------------------------------------------------
 * spriteGetPixel --
 *	Given an rgb value, find an equivalent pixel value for it.
 *
 * Results:
 *	The pixel value.
 *
 * Side Effects:
 *	A colormap entry might be allocated...
 *
 *-----------------------------------------------------------------------
 */
static Pixel
spriteGetPixel (pScreen, r, g, b)
    ScreenPtr	  pScreen;  /* Screen to allocate from */
    unsigned short r;	    /* Red value to use */
    unsigned short g;	    /* Green value to use */
    unsigned short b;	    /* Blue value to use */
{
    ColormapPtr cmap;
    Pixel       pix;

    cmap = (ColormapPtr) LookupID(pScreen->defColormap, RT_COLORMAP, RC_CORE);
    if (!cmap) {
	FatalError("Can't find default colormap in spriteGetPixel\n");
    }
    if (AllocColor(cmap, &r, &g, &b, &pix, 0)) {
	FatalError("Can't alloc pixel (%d,%d,%d) in spriteGetPixel\n",
		   r, g, b);
    }
    return (pix);
}
/*-
 *-----------------------------------------------------------------------
 * spriteRealizeCursor --
 *	Realize a cursor for a specific screen. Eventually it will have
 *	to deal with the allocation of a special pixel from the system
 *	colormap, but for now it's fairly simple. Just have to create
 *	pixmaps.
 *
 * Results:
 *
 * Side Effects:
 *	A CrPrivRec is allocated and filled and stuffed into the Cursor
 *	structure given us.
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteRealizeCursor (pScreen, pCursor)
    ScreenPtr	  pScreen;  	/* Screen for which the cursor should be */
				/* realized */
    CursorPtr	  pCursor;  	/* Cursor to realize */
{
    register CrPrivPtr pPriv;
    int         bufWidth;
    GC         *pGC;		/* GC for initializing the source and
				 * invSource pixmaps... */
    unsigned char *stencil;
    BITS32      status;


    pGC = GetScratchGC (1, pScreen);

    pPriv = (CrPrivPtr) Xalloc(sizeof(CrPrivRec));
    pPriv->state = CR_OUT;
    pCursor->devPriv[pScreen->myNum] = (pointer) pPriv;

    bufWidth = 2 * pCursor->width;

    pPriv->fg = spriteGetPixel(pScreen, pCursor->foreRed,
			    pCursor->foreGreen,
			    pCursor->foreBlue);
    pPriv->bg = spriteGetPixel(pScreen, pCursor->backRed,
			    pCursor->backGreen,
			    pCursor->backBlue);

    /*
     * Create the two pixmaps for the off-screen manipulation of the
     * cursor image. The screenBits pixmap is used to hold the contents of
     * the screen before the cursor is put down and the temp pixmap exists
     * to avoid having to create a pixmap each time the cursor is rop'ed
     * in. Both are made the same depth as the screen, for obvious
     * reasons. 
     */
    pPriv->screenBits =
	(PixmapPtr) (*pScreen->CreatePixmap) (pScreen, bufWidth,
					      2 * pCursor->height,
					      pScreen->rootDepth,
					      ZPixmap);
    pPriv->temp =
	(PixmapPtr) (*pScreen->CreatePixmap) (pScreen, bufWidth,
					      2 * pCursor->height,
					      pScreen->rootDepth,
					      ZPixmap);

    /*
     * The source and invSource bitmaps are a bit trickier. The idea is to
     * use the source in a PushPixels operation on the foreground pixel of
     * the cursor and the invSource in a PushPixels on the background
     * pixel of the cursor. The tricky thing is both bitmaps must be
     * created from the source bits after being masked by the mask bits.
     * This must, sadly, be done by hand b/c the ddx interface isn't quite
     * rich enough to push a tile through a bitmap. 
     */
    pPriv->source =
	(PixmapPtr) (*pScreen->CreatePixmap) (pScreen, pCursor->width,
					      pCursor->height, 1,
					      XYBitmap);
    ValidateGC(pPriv->source, pGC);
    stencil = spriteStencil(pCursor->source, pCursor->mask,
			 pCursor->width, pCursor->height,
			 FALSE);
    (*pGC->PutImage) (pPriv->source, pGC, 1,
		      0, 0,
		      pCursor->width, pCursor->height,
		      0,
		      XYPixmap,
		      stencil);
    Xfree(stencil);

    pPriv->srcGC = spriteCreatePrivGC((DrawablePtr)pScreen->devPrivate,
				      GCForeground, &pPriv->fg, &status);
    ValidateGC((DrawablePtr) pScreen->devPrivate, pPriv->srcGC);

    pPriv->invSource =
	(PixmapPtr) (*pScreen->CreatePixmap) (pScreen, pCursor->width,
					      pCursor->height, 1,
					      XYBitmap);
    ValidateGC(pPriv->invSource, pGC);
    stencil = spriteStencil(pCursor->source, pCursor->mask,
			 pCursor->width, pCursor->height,
			 TRUE);
    (*pGC->PutImage) (pPriv->invSource, pGC, 1,
		      0, 0,
		      pCursor->width, pCursor->height,
		      0,
		      XYPixmap,
		      stencil);
    Xfree(stencil);

    pPriv->invSrcGC = spriteCreatePrivGC((DrawablePtr)pScreen->devPrivate,
					GCForeground, &pPriv->bg, &status);
    ValidateGC((DrawablePtr) pScreen->devPrivate, pPriv->invSrcGC);

    FreeScratchGC (pGC);
}

/*-
 *-----------------------------------------------------------------------
 * spriteUnrealizeCursor --
 *	Free up the extra state created by spriteRealizeCursor.
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	All the pixmaps etc. created by spriteRealizeCursor are destroyed
 *	and the private structure is freed.
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteUnrealizeCursor (pScreen, pCursor)
    ScreenPtr	  pScreen;  	/* Screen for which to unrealize the cursor */
    CursorPtr	  pCursor;  	/* Cursor to unrealize */
{
    CrPrivPtr   pPriv;

    if (currentCursor == pCursor) {
	spriteRemoveCursor();
	currentCursor = NullCursor;
    }
    pPriv = (CrPrivPtr) pCursor->devPriv[pScreen->myNum];

    (*pScreen->DestroyPixmap) (pPriv->source);
    (*pScreen->DestroyPixmap) (pPriv->invSource);
    (*pScreen->DestroyPixmap) (pPriv->screenBits);
    (*pScreen->DestroyPixmap) (pPriv->temp);
    /*
     * XXX: Deallocate pixels 
     */
    FreeScratchGC(pPriv->srcGC);
    FreeScratchGC(pPriv->invSrcGC);

    Xfree(pPriv);

    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * spriteSetCursorPosition --
 *	Alter the position of the current cursor. The x and y coordinates
 *	are assumed to be legal for the given screen.
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	The pScreen, x, and y fields of the current pointer's private data
 *	are altered to reflect the new values. I.e. moving the cursor to
 *	a different screen moves the pointer there as well. Helpful...
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteSetCursorPosition (pScreen, hotX, hotY, genEvent)
    ScreenPtr	  pScreen;  	/* New screen for the cursor */
    unsigned int  hotX;	    	/* New absolute X coordinate for the cursor */
    unsigned int  hotY;	    	/* New absolute Y coordinate for the cursor */
    Bool    	  genEvent; 	/* Generate a motion event */
{
    DevicePtr	  pDev;
    PtrPrivPtr	  pptrPriv;

    if (currentCursor == NullCursor) {
	return FALSE;
    } else {
	pDev = LookupPointerDevice();
	
	pptrPriv = (PtrPrivPtr) pDev->devicePrivate;

	if (genEvent) {
	    /*
	     * If we have to generate a motion event here, we need to
	     * flush any current motion events to move the cursor to the
	     * right place, etc.
	     */
	    ProcessInputEvents();
	}
	
	if (pScreen == pptrPriv->pScreen) {
	    /*
	     * Cursor staying on the same screen, let spriteMoveCursor
	     * worry about positioning the beastie
	     */
	    spriteMoveCursor (pScreen, hotX, hotY);
	} else {
	    /*
	     * Now nuke the cursor and switch its position to the other
	     * screen.
	     */
	    spriteRemoveCursor();
	    pptrPriv->pScreen = pScreen;
	}
	pptrPriv->x = hotX;
	pptrPriv->y = hotY;

	if (genEvent) {
	    /*
	     * Have to generate a motion event for DIX. We use the time of
	     * the last event plus 1 millisecond since we can't do anything
	     * else useful and I assume there will be more than 1 ms between
	     * events...maybe.
	     */
	    xEvent	motion;
	    
	    motion.u.keyButtonPointer.rootX = hotX;
	    motion.u.keyButtonPointer.rootY = hotY;
	    lastEventTimeMS += 1;
	    motion.u.keyButtonPointer.time = lastEventTimeMS;
	    motion.u.u.type = MotionNotify;
	    (* pDev->processInputProc) (&motion, pDev);
	}
	return TRUE;
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteCursorLimits --
 *	Return a box within which the given cursor may move on the given
 *	screen. We assume that the HotBox is actually on the given screen,
 *	since dix knows that size.
 *
 * Results:
 *	A box for the hot spot corner of the cursor.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
void
spriteCursorLimits (pScreen, pCursor, pHotBox, pResultBox)
    ScreenPtr	  pScreen;  	/* Screen on which limits are desired */
    CursorPtr	  pCursor;  	/* Cursor whose limits are desired */
    BoxPtr  	  pHotBox;  	/* Limits for pCursor's hot point */
    BoxPtr  	  pResultBox;	/* RETURN: limits for hot spot */
{
    *pResultBox = *pHotBox;
}

/*-
 *-----------------------------------------------------------------------
 * spriteDisplayCursor --
 *	Set the current cursor.
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	The value of currentCursor is altered. Note that the cursor is
 *	*not* placed in the frame buffer until RestoreCursor is called.
 *	Instead, the cursor is marked as out, which will cause it to
 *	be replaced.
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteDisplayCursor (pScreen, pCursor)
    ScreenPtr	  pScreen;  	/* Screen on which to display cursor */
    CursorPtr	  pCursor;  	/* Cursor to display */
{
    if (currentCursor) {
	spriteRemoveCursor();
    }

    currentCursor = pCursor;

    ((CrPrivPtr)pCursor->devPriv[pScreen->myNum])->state = CR_OUT;
    spriteCursorGone();

    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * spriteRecolorCursor --
 *	Change the color of a cursor.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If the screen is a color one
 *
 *-----------------------------------------------------------------------
 */
void
spriteRecolorCursor (pScreen, pCursor, displayed)
    ScreenPtr	  pScreen;  	/* Screen for which the cursor is to be */
				/* recolored */
    CursorPtr	  pCursor;  	/* Cursor to recolor */
    Bool    	  displayed;	/* True if pCursor being displayed now */
{
    CrPrivPtr	  pPriv;
    /*
     * XXX: will have to alter the colormap entries of the foreground and
     * background pixels. For now, just change the pixels...
     */

    if (displayed) {
	spriteRemoveCursor();
    }

    pPriv = (CrPrivPtr)pCursor->devPriv[pScreen->myNum];
    pPriv->fg = spriteGetPixel(pScreen,
			    pCursor->foreRed,
			    pCursor->foreGreen,
			    pCursor->foreBlue);
    ChangeGC (pPriv->srcGC, GCForeground, &pPriv->fg);
    ValidateGC (pScreen->devPrivate, pPriv->srcGC);
    
    pPriv->bg = spriteGetPixel (pScreen,
			     pCursor->backRed,
			     pCursor->backGreen,
			     pCursor->backBlue);
    ChangeGC (pPriv->invSrcGC, GCForeground, &pPriv->bg);
    ValidateGC (pScreen->devPrivate, pPriv->invSrcGC);
}

/*-
 *-----------------------------------------------------------------------
 * spritePointerNonInterestBox --
 *	Set up things to ignore uninteresting mouse events. Sorry.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
spritePointerNonInterestBox (pScreen, pBox)
    ScreenPtr	  pScreen;  /* Screen for cursor */
    BoxPtr  	  pBox;	    /* Box inside of which motions are boring */
{
}

/*-
 *-----------------------------------------------------------------------
 * spriteConstrainCursor --
 *	Make it so the current pointer doesn't let the cursor's
 *      hot-spot wander out of the specified box.
 *	
 * Results:
 *	None.
 *
 * Side Effects:
 *	currentLimits is overwritten.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
spriteConstrainCursor (pScreen, pBox)
    ScreenPtr	  pScreen;  	/* Screen to which it should be constrained */
    BoxPtr  	  pBox;	    	/* Box in which... */
{
    currentLimits = *pBox;
}

/*-
 *-----------------------------------------------------------------------
 * spriteRemoveCursor --
 *	Remove the current cursor from the screen, if it hasn't been removed
 *	already.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If the cursor is removed, the state field of the cursor-private
 *	structure is set to CR_OUT. In addition, spriteCursorGone is called.
 *
 *-----------------------------------------------------------------------
 */
void
spriteRemoveCursor ()
{
    CrPrivPtr	  pPriv;    	/* Private data for this cursor */
    ScreenPtr	  pScreen;  	/* Screen on which the cursor is */
    PtrPrivPtr	  pPtrPriv;	/* XXX: Pointer private data */
    DevicePtr	  pDev;
    GC	    	  *pGC;

    if (currentCursor == NullCursor) {
	return;
    }

    pDev = LookupPointerDevice();

    pPtrPriv = (PtrPrivPtr) pDev->devicePrivate;
    pScreen = pPtrPriv->pScreen;
    
    pPriv = (CrPrivPtr)currentCursor->devPriv[pScreen->myNum];

    if (pPriv->state == CR_IN) {
	/*
	 * XXX: Makes use of the devPrivate field of the screen. This *must*
	 * be a pixmap representing the entire screen. How else can I get a
	 * pixmap to draw to?
	 */

	pGC = spriteFbs[pScreen->myNum].pGC;

	pPriv->state = CR_XING;
	pGC->stateChanges |= (GCForeground|GCBackground);	
		/* Need to set some bits */
	ValidateGC((DrawablePtr)pScreen->devPrivate, pGC);
	(* pGC->CopyArea) (pPriv->screenBits,
			   (PixmapPtr)pScreen->devPrivate,
			   pGC,
			   0, 0,
			   pPriv->screenBits->width,
			   pPriv->screenBits->height,
			   pPriv->scrX, pPriv->scrY);
	pPriv->state = CR_OUT;
	spriteCursorGone();
    }
}

/*-
 *-----------------------------------------------------------------------
 * spritePutCursor --
 *	Place the current cursor in the screen at the given coordinates.
 *	screenBits must already have been filled. If 'direct' is FALSE,
 *	the cursor will be drawn into the temp pixmap after the screenBits
 *	have been copied there. The temp pixmap will then be put onto
 *	the screen. Used for more flicker-free cursor motion...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The state field of the private data is set to CR_IN...
 *
 *-----------------------------------------------------------------------
 */

static void
spritePutCursor (pScreen, pPriv, hotX, hotY, direct)
    ScreenPtr	  	pScreen;    	/* Screen on which to put it */
    CrPrivPtr	  	pPriv;	    	/* Cursor-private data */
    int	    	  	hotX;
    int	    	  	hotY;
    Bool    	  	direct;	    	/* TRUE if should put the cursor */
					/* directly onto the screen, not */
					/* into pPriv->temp first... */
{
    register PixmapPtr	pPixmap;
    register GC	  	*pGC;
    int 	  	realW,
			realH;
    int			x = hotX - currentCursor->xhot;
    int 		y = hotY - currentCursor->yhot;
    

    pGC = spriteFbs[pScreen->myNum].pGC;

    if (!direct) {
	pPixmap = pPriv->temp;
	
	/*
	 * Duplicate the contents of the screen
	 */
	pGC->stateChanges |= (GCForeground|GCBackground);
		/* Need to set some bits */
	ValidateGC(pPixmap, pGC);
	(* pGC->CopyArea) (pPriv->screenBits, pPixmap, pGC,
			   0, 0, pPixmap->width, pPixmap->height,
			   0, 0);
	
	/*
	 * First the foreground pixels...
	 * Warning: The srcGC is validated with the screen, so the clip list
	 * here will be wrong, but it shouldn't matter since we never go
	 * outside the pixmap...
	 */
	(* pPriv->srcGC->PushPixels) (pPriv->srcGC, pPriv->source, pPixmap,
				      pPriv->source->width,
				      pPriv->source->height,
				      x - pPriv->scrX, y - pPriv->scrY);

	/*
	 * Then the background pixels
	 */
	(* pPriv->invSrcGC->PushPixels) (pPriv->invSrcGC, pPriv->invSource,
					 pPixmap, pPriv->invSource->width,
					 pPriv->invSource->height,
					 x - pPriv->scrX, y - pPriv->scrY);

	/*
	 * Now put the whole buffer onto the screen
	 */
	pPriv->state = CR_XING;
	realW = min(pPixmap->width, pScreen->width - pPriv->scrX);
	realH = min(pPixmap->height,pScreen->height - pPriv->scrY);

	pGC->stateChanges |= (GCForeground|GCBackground);
		/* Need to set some bits */
	ValidateGC((DrawablePtr)pScreen->devPrivate, pGC);
	(* pGC->CopyArea) (pPixmap, (PixmapPtr)pScreen->devPrivate, pGC,
			   0, 0, realW, realH, pPriv->scrX, pPriv->scrY);
	
	pPriv->state = CR_IN;
    } else {
	pPixmap = (PixmapPtr) pScreen->devPrivate;

	/*
	 * PushPixels can't handle negative x and y.  Therefore
	 * we'll clip here on our own.
	 */
	realW = min(pPriv->source->width,pPixmap->width - x);
	realH = min(pPriv->source->height,pPixmap->height - y);

	pPriv->state = CR_XING;
	/*
	 * First the foreground pixels...
	 */
	(* pPriv->srcGC->PushPixels) (pPriv->srcGC, pPriv->source, pPixmap,
				      realW, realH, x, y);
	
	/*
	 * Then the background pixels
	 */
	(* pPriv->invSrcGC->PushPixels) (pPriv->invSrcGC, pPriv->invSource,
					 pPixmap, realW, realH, x, y);

	pPriv->state = CR_IN;
    }
}


/*-
 *-----------------------------------------------------------------------
 * spriteCursorLoc --
 *	If the current cursor is both on and in the given screen,
 *	fill in the given BoxRec with the extent of the cursor and return
 *	TRUE. If the cursor is either on a different screen or not
 *	currently in the frame buffer, return FALSE.
 *
 * Results:
 *	TRUE or FALSE, as above.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteCursorLoc (pScreen, pBox)
    ScreenPtr	  pScreen;  	/* Affected screen */
    BoxRec	  *pBox;	/* Box in which to place the limits */
{
    PtrPrivPtr	  pptrPriv;
    CrPrivPtr	  pPriv;
    DevicePtr	  pDev;

    if (currentCursor == NullCursor) {
	/*
	 * Firewall: Might be called when initially putting down the cursor
	 */
	return FALSE;
    }

    pDev = LookupPointerDevice();
    pptrPriv = (PtrPrivPtr) pDev->devicePrivate;
    pPriv = (CrPrivPtr) currentCursor->devPriv[pScreen->myNum];

    if ((pptrPriv->pScreen == pScreen) && (pPriv->state == CR_IN)) {
	/*
	 * If the cursor is actually on the same screen, stuff the cursor's
	 * limits in the given BoxRec. Perhaps this should be done when
	 * the cursor is moved? Probably not important...
	 */

	pBox->x1 = pPriv->scrX;
	pBox->y1 = pPriv->scrY;
	pBox->x2 = pPriv->scrX + pPriv->screenBits->width;
	pBox->y2 = pPriv->scrY + pPriv->screenBits->height;
	return TRUE;
    } else {
	return FALSE;
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteRestoreCursor --
 *	Redraw the cursor if it was removed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be replaced and the 'state' field changed.
 *
 *-----------------------------------------------------------------------
 */
void
spriteRestoreCursor()
{
    PtrPrivPtr	  	pptrPriv;
    CrPrivPtr	  	pPriv;
    ScreenPtr	  	pScreen;
    DevicePtr		pDev;
    GC	    	  	*pGC;
    register PixmapPtr	screenBits;
    int	    	  	scrX, scrY;

    pDev = LookupPointerDevice();

    pptrPriv = (PtrPrivPtr) pDev->devicePrivate;
    pScreen = pptrPriv->pScreen;
    pPriv = (CrPrivPtr) currentCursor->devPriv[pScreen->myNum];
    screenBits = pPriv->screenBits;

    if (pPriv->state == CR_OUT) {
	/*
	 * Since the buffer pixmap is four times as large as the cursor and
	 * we would always like to center the thing so as to allow the same
	 * leeway for movement, in spriteMoveCursor, on each side, we place the
	 * top-left corner of the cursor at the intersection of the first
	 * quarter lines by shifting the position of the buffer pixmap
	 */
	scrX = pptrPriv->x - currentCursor->xhot - screenBits->width / 4;
	scrY = pptrPriv->y - currentCursor->yhot - screenBits->height / 4;

	/*
	 * In general we're trying to store some of the bits surrounding the
	 * cursor.  We try to center the cursor in the area we're
         * saving; that is what the previous two lines are doing.  But if we're
         * at the top left already, we won't try to center the cursor; we'll
	 * just save from [0,0].
	 */
	if (scrX < 0) {
	    scrX = 0;
	}
	if (scrY < 0) {
	    scrY = 0;
	}

	pPriv->scrX = scrX;
	pPriv->scrY = scrY;
	
	pGC = spriteFbs[pScreen->myNum].pGC;
	pGC->stateChanges |= (GCForeground|GCBackground);	
		/* Need to set some bits */
	ValidateGC(screenBits, pGC);
	(* pGC->CopyArea) ((DrawablePtr)pScreen->devPrivate,
			   (DrawablePtr)screenBits,
			   pGC,
			   scrX, scrY,
			   screenBits->width, screenBits->height,
			   0, 0);
	spritePutCursor (pScreen, pPriv, pptrPriv->x, pptrPriv->y, TRUE);
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteMoveCursor --
 *	Shift a the current cursor by a given amount. If the change keeps
 *	the cursor within its screenBits pixmap, the whole thing is
 *	simply drawn over the old position. Otherwise, the cursor is
 *	removed and must be redrawn before we sleep. The pointer's
 *	coordinates need not have been updated before this is called.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be removed...
 *
 *-----------------------------------------------------------------------
 */
void
spriteMoveCursor (pScreen, hotX, hotY)
    ScreenPtr	  pScreen;  	/* Screen cursor's currently on */
    int	    	  hotX;	  	/* Cursor's new X coordinate */
    int	    	  hotY;	  	/* Cursor's new Y coordinate */
{
    CrPrivPtr	  pPriv;

    pPriv = (CrPrivPtr) currentCursor->devPriv[pScreen->myNum];

    if (pPriv->state == CR_OUT) {
	return;
    }

    if ((hotX - currentCursor->xhot >= pPriv->scrX) &&
	((hotX - currentCursor->xhot + currentCursor->width) <
	 (pPriv->scrX + pPriv->screenBits->width))&&
	(hotY - currentCursor->yhot >= pPriv->scrY) &&
	((hotY - currentCursor->yhot + currentCursor->height) <
	 (pPriv->scrY + pPriv->screenBits->height))) {
	     /*
	      * If the entire cursor at its new position remains inside the
	      * box buffered in the screenBits pixmap, then its ok to just
	      * place the cursor inside the box and draw the entire box
	      * onto the screen. The hope is that this redrawing, rather than
	      * removing the cursor and redrawing it, will cause it to flicker
	      * less than it did in V10...
	      */
	     spritePutCursor (pScreen, pPriv, hotX, hotY, FALSE);
    } else {
	/*
	 * The cursor is no longer within the screenBits pixmap, so we just
	 * remove it. dix will RestoreCursor() it back onto the screen.
	 */
	spriteRemoveCursor();
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteConstrainXY --
 *	Given an X and Y coordinate, alter them to fit within the current
 *	cursor constraints.  Used by mouse processing code to adjust 
 * 	position reported by hardware.
 *	
 *
 * Results:
 *	The new constrained coordinates. Returns FALSE if the motion
 *	event should be discarded...
 *
 * Side Effects:
 *	guess what?
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteConstrainXY (px, py)
    short	  *px;
    short     	  *py;	
{
    *px = max(currentLimits.x1,min(currentLimits.x2,*px));
    *py = max(currentLimits.y1,min(currentLimits.y2,*py));
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * spritePaintWindow --
 *	Paint either the window's border or background while preserving
 *	the cursor.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor will be removed if it is in the way.
 *
 *-----------------------------------------------------------------------
 */
void
spritePaintWindow (pWin, pRegion, what)
    WindowPtr	pWin;	    	/* Window to paint */
    RegionPtr	pRegion;    	/* Uncovered region in which the painting
				 * should be done */
    int		what;	    	/* PW_BACKGROUND to paint the background
				 * PW_BORDER to paint the border */
{
    BoxRec	cursorBox;  	/* Current cursor area */
    WinPrivPtr	pPriv;	    	/* Private data for the window */
    ScreenPtr	pScreen;    	/* The screen the window is on */

    pScreen = pWin->drawable.pScreen;

    if (spriteCursorLoc (pScreen, &cursorBox)) {
	/*
	 * If the cursor is on the same screen as the window, check the
	 * region to paint for the cursor and remove it as necessary
	 */
	if ((* pScreen->RectIn) (pRegion, &cursorBox) != rgnOUT) {
	    spriteRemoveCursor();
	}
    }

    pPriv = (WinPrivPtr) LookupID (pWin->wid, RT_WINDOW, wPrivClass);
    if (what == PW_BACKGROUND) {
	(* pPriv->PaintWindowBackground) (pWin, pRegion, what);
    } else {
	(* pPriv->PaintWindowBorder) (pWin, pRegion, what);
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteCopyWindow --
 *	Protect the cursor from window copies..
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be removed.
 *
 *-----------------------------------------------------------------------
 */
void
spriteCopyWindow (pWin, ptOldOrg, prgnSrc)
    WindowPtr	  pWin;	    /* Window being copied */
    DDXPointRec	  ptOldOrg; /* Old absolute origin for window */
    RegionPtr	  prgnSrc;  /* Region from which window is being copied. */
{
    BoxRec	cursorBox;
    WinPrivPtr	pPriv;
    ScreenPtr	pScreen;

    pScreen = pWin->drawable.pScreen;

    if (spriteCursorLoc (pScreen, &cursorBox)) {
	/*
	 * If the cursor is on the same screen, compare the box for the
	 * cursor against the original window clip region (prgnSrc) and
	 * the current window clip region (pWin->borderClip) and if it
	 * overlaps either one, remove the cursor. (Should it really be
	 * borderClip?)
	 */
	switch ((* pScreen->RectIn) (prgnSrc, &cursorBox)) {
	    case rgnOUT:
		if ((* pScreen->RectIn) (pWin->borderClip, &cursorBox) ==
		    rgnOUT) {
			break;
		}
	    case rgnIN:
	    case rgnPART:
		spriteRemoveCursor();
	}
    }

    pPriv = (WinPrivPtr) LookupID (pWin->wid, RT_WINDOW, wPrivClass);
    (* pPriv->CopyWindow) (pWin, ptOldOrg, prgnSrc);
}

/*-
 *-----------------------------------------------------------------------
 * spriteSaveAreas --
 *	Keep the cursor from getting in the way of any SaveAreas operation
 *	by backing-store.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be removed.
 *
 *-----------------------------------------------------------------------
 */
void
spriteSaveAreas(pWin)
    WindowPtr	  pWin;
{
    BoxRec  	  cursorBox;
    WinPrivPtr	  pPriv;
    ScreenPtr	  pScreen;

    pScreen = pWin->drawable.pScreen;

    if (spriteCursorLoc(pScreen, &cursorBox)) {
	/*
	 * If the areas are obscured because the window moved, we need to
	 * translate the box to the correct relationship with the region,
	 * which is at the new window coordinates.
	 */
	int dx, dy;

	dx = pWin->absCorner.x - pWin->backStorage->oldAbsCorner.x;
	dy = pWin->absCorner.y - pWin->backStorage->oldAbsCorner.y;

	if (dx || dy) {
	    cursorBox.x1 += dx;
	    cursorBox.y1 += dy;
	    cursorBox.x2 += dx;
	    cursorBox.y2 += dy;
	}
	if ((* pScreen->RectIn) (pWin->backStorage->obscured, &cursorBox) != rgnOUT) {
	    spriteRemoveCursor();
	}
    }
    pPriv = (WinPrivPtr) LookupID (pWin->wid, RT_WINDOW, wPrivClass);
    (* pPriv->SaveAreas) (pWin);
}

/*-
 *-----------------------------------------------------------------------
 * spriteRestoreAreas --
 *	Keep the cursor from getting in the way of any RestoreAreas operation
 *	by backing-store.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be removed.
 *
 *-----------------------------------------------------------------------
 */
RegionPtr
spriteRestoreAreas(pWin)
    WindowPtr	  pWin;
{
    BoxRec  	  cursorBox;
    WinPrivPtr	  pPriv;
    ScreenPtr	  pScreen;

    pScreen = pWin->drawable.pScreen;

    if (spriteCursorLoc(pScreen, &cursorBox)) {
	/*
	 * The exposed region is now window-relative, so we have to make the
	 * cursor box window-relative too.
	 */
	cursorBox.x1 -= pWin->absCorner.x;
	cursorBox.x2 -= pWin->absCorner.x;
	cursorBox.y1 -= pWin->absCorner.y;
	cursorBox.y2 -= pWin->absCorner.y;
	if ((* pScreen->RectIn) (pWin->exposed, &cursorBox) != rgnOUT) {
	    spriteRemoveCursor();
	}
    }
    pPriv = (WinPrivPtr) LookupID (pWin->wid, RT_WINDOW, wPrivClass);
    return (* pPriv->RestoreAreas) (pWin);
}

/*-
 *-----------------------------------------------------------------------
 * spriteCreateWindow --
 *	A way to get our hooks into a new window to keep the cursor from
 *	being obliterated. We first allow the output library for the
 *	screen to install its vectors and then stuff our own in their
 *	places.
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	A WinPrivRec is allocated and stored under the window's ID.
 *	The PaintWindowBackground and PaintWindowBorder and CopyWindow
 *	vectors for the new window are overwritten.
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteCreateWindow (pWin)
    WindowPtr	pWin;
{
    WinPrivPtr	pPriv;

    (* spriteFbs[((DrawablePtr)pWin)->pScreen->myNum].CreateWindow) (pWin);

    pPriv = (WinPrivPtr) Xalloc (sizeof (WinPrivRec));
    pPriv->PaintWindowBackground =  pWin->PaintWindowBackground;
    pPriv->PaintWindowBorder = 	    pWin->PaintWindowBorder;
    pPriv->CopyWindow =     	    pWin->CopyWindow;

    pWin->PaintWindowBackground =   spritePaintWindow;
    pWin->PaintWindowBorder = 	    spritePaintWindow;
    pWin->CopyWindow =	    	    spriteCopyWindow;

    if (pWin->backStorage) {
	pPriv->SaveAreas = pWin->backStorage->SaveDoomedAreas;
	pWin->backStorage->SaveDoomedAreas = spriteSaveAreas;
	pPriv->RestoreAreas = pWin->backStorage->RestoreAreas;
	pWin->backStorage->RestoreAreas = spriteRestoreAreas;
    }
    AddResource (pWin->wid, RT_WINDOW, (pointer)pPriv, Xfree, wPrivClass);
}

/*-
 *-----------------------------------------------------------------------
 * spriteChangeWindowAttributes --
 *	Catch the changing of the background/border functions when a
 *	window is reconfigured.
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	The three vectors we intercept will be restored to the rightful
 *	value if they have been changed.
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteChangeWindowAttributes (pWin, mask)
    WindowPtr	pWin;	    /* Window being altered */
    Mask	mask;	    /* Mask of changes to the window */
{
    WinPrivPtr	pPriv;

    (* spriteFbs[((DrawablePtr)pWin)->pScreen->myNum].ChangeWindowAttributes)
	(pWin, mask);
    
    pPriv = (WinPrivPtr) LookupID (pWin->wid, RT_WINDOW, wPrivClass);

    if (pPriv == (WinPrivPtr)0) {
	FatalError ("spriteChangeWindowAttributes got null private data");
    }

    if ((void (*)())pWin->PaintWindowBackground !=
	(void (*)())spritePaintWindow){
	    pPriv->PaintWindowBackground = pWin->PaintWindowBackground;
	    pWin->PaintWindowBackground = spritePaintWindow;
    }

    if ((void (*)())pWin->PaintWindowBorder !=
	(void (*)())spritePaintWindow) {
	    pPriv->PaintWindowBorder = pWin->PaintWindowBorder;
	    pWin->PaintWindowBorder = spritePaintWindow;
    }

    if ((void (*)())pWin->CopyWindow != (void (*)())spriteCopyWindow) {
	pPriv->CopyWindow = pWin->CopyWindow;
	pWin->CopyWindow = spriteCopyWindow;
    }

    if (pWin->backStorage &&
	((void (*)())pWin->backStorage->SaveDoomedAreas != (void (*)())spriteSaveAreas)){
	    pPriv->SaveAreas = pWin->backStorage->SaveDoomedAreas;
	    pWin->backStorage->SaveDoomedAreas = spriteSaveAreas;
    }
    if (pWin->backStorage &&
	((void (*)())pWin->backStorage->RestoreAreas != (void (*)())spriteRestoreAreas)){
	    pPriv->RestoreAreas = pWin->backStorage->RestoreAreas;
	    pWin->backStorage->RestoreAreas = spriteRestoreAreas;
    }

    return (TRUE);
}

