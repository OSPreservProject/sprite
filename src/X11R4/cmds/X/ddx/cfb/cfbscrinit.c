/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#include "X.h"
#include "Xmd.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#include "cfb.h"
#include "mi.h"
#include "mistruct.h"
#include "dix.h"
#include "cfbmskbits.h"
#include "mibstore.h"

extern RegionPtr mfbPixmapToRegion();
extern RegionPtr cfbCopyPlane();
extern Bool mfbAllocatePrivates();

extern int defaultColorVisualClass;

#define _BP 8
#define _RZ ((PSZ + 2) / 3)
#define _RS 0
#define _RM ((1 << _RZ) - 1)
#define _GZ ((PSZ - _RZ + 1) / 2)
#define _GS _RZ
#define _GM (((1 << _GZ) - 1) << _GS)
#define _BZ (PSZ - _RZ - _GZ)
#define _BS (_RZ + _GZ)
#define _BM (((1 << _BZ) - 1) << _BS)
#define _CE (1 << _RZ)

static VisualRec visuals[] = {
/* vid  class        bpRGB cmpE nplan rMask gMask bMask oRed oGreen oBlue */
#ifndef STATIC_COLOR
    0,  PseudoColor, _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
    0,  DirectColor, _BP, _CE,       PSZ,  _RM, _GM, _BM, _RS, _GS, _BS,
    0,  GrayScale,   _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
    0,  StaticGray,  _BP,  1<<PSZ,   PSZ,  0,   0,   0,   0,   0,   0,
#endif
    0,  StaticColor, _BP,  1<<PSZ,   PSZ,  _RM, _GM, _BM, _RS, _GS, _BS,
    0,  TrueColor,   _BP, _CE,       PSZ,  _RM, _GM, _BM, _RS, _GS, _BS
};

#define	NUMVISUALS	((sizeof visuals)/(sizeof visuals[0]))

static  VisualID VIDs[NUMVISUALS];

static DepthRec depths[] = {
/* depth	numVid		vids */
    1,		0,		NULL,
    8,		NUMVISUALS,	VIDs
};

#define NUMDEPTHS	((sizeof depths)/(sizeof depths[0]))

int cfbWindowPrivateIndex;
int cfbGCPrivateIndex;
static unsigned long cfbGeneration = 0;

miBSFuncRec cfbBSFuncRec = {
    cfbSaveAreas,
    cfbRestoreAreas,
    (void (*)()) 0,
    (PixmapPtr (*)()) 0,
    (PixmapPtr (*)()) 0,
};

/*ARGSUSED*/
static Bool
cfbCloseScreen (index, pScreen)
    int		index;
    ScreenPtr	pScreen;
{
    xfree (pScreen->devPrivate);
    return TRUE;
}

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
Bool
cfbScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
    register PixmapPtr pPixmap;
    int	i;

    if (cfbGeneration != serverGeneration)
    {
	/*  Set up the visual IDs */
	for (i = 0; i < NUMVISUALS; i++) {
	    visuals[i].vid = FakeClientID(0);
	    VIDs[i] = visuals[i].vid;
	}
	cfbGeneration = serverGeneration;
    }
    if (!mfbAllocatePrivates(pScreen,
			     &cfbWindowPrivateIndex, &cfbGCPrivateIndex))
	return FALSE;
    if (!AllocateWindowPrivate(pScreen, cfbWindowPrivateIndex,
			       sizeof(cfbPrivWin)) ||
	!AllocateGCPrivate(pScreen, cfbGCPrivateIndex, sizeof(cfbPrivGC)))
	return FALSE;
    pScreen->width = xsize;
    pScreen->height = ysize;
    pScreen->mmWidth = (xsize * 254) / (dpix * 10);
    pScreen->mmHeight = (ysize * 254) / (dpiy * 10);
    pScreen->numDepths = NUMDEPTHS;
    pScreen->allowedDepths = depths;

    pScreen->rootDepth = 8;
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->backingStoreSupport = Always;
    pScreen->saveUnderSupport = NotUseful;
    /* let CreateDefColormap do whatever it wants */ 
    pScreen->blackPixel = pScreen->whitePixel = (Pixel) 0;

    /* cursmin and cursmax are device specific */

    pScreen->numVisuals = NUMVISUALS;
    pScreen->visuals = visuals;

    pPixmap = (PixmapPtr ) xalloc(sizeof(PixmapRec));
    if (!pPixmap)
	return FALSE;
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.depth = 8;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.serialNumber = 0;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = xsize;
    pPixmap->drawable.height = ysize;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate.ptr = pbits;
    pPixmap->devKind = PixmapBytePad(width, 8);
    pScreen->devPrivate = (pointer)pPixmap;

    /* anything that cfb doesn't know about is assumed to be done
       elsewhere.  (we put in no-op only for things that we KNOW
       are really no-op.
    */
    pScreen->CreateWindow = cfbCreateWindow;
    pScreen->DestroyWindow = cfbDestroyWindow;
    pScreen->PositionWindow = cfbPositionWindow;
    pScreen->ChangeWindowAttributes = cfbChangeWindowAttributes;
    pScreen->RealizeWindow = cfbMapWindow;
    pScreen->UnrealizeWindow = cfbUnmapWindow;

    pScreen->RealizeFont = mfbRealizeFont;
    pScreen->UnrealizeFont = mfbUnrealizeFont;
    pScreen->CloseScreen = cfbCloseScreen;
    pScreen->QueryBestSize = mfbQueryBestSize;
    pScreen->GetImage = cfbGetImage;
    pScreen->GetSpans = cfbGetSpans;
    pScreen->SourceValidate = (void (*)()) 0;
    pScreen->CreateGC = cfbCreateGC;
    pScreen->CreatePixmap = cfbCreatePixmap;
    pScreen->DestroyPixmap = cfbDestroyPixmap;
    pScreen->ValidateTree = miValidateTree;

#ifdef	STATIC_COLOR
    pScreen->InstallColormap = cfbInstallColormap;
    pScreen->UninstallColormap = cfbUninstallColormap;
    pScreen->ListInstalledColormaps = cfbListInstalledColormaps;
    pScreen->StoreColors = NoopDDA;
#endif
    pScreen->ResolveColor = cfbResolveColor;

    pScreen->RegionCreate = miRegionCreate;
    pScreen->RegionInit = miRegionInit;
    pScreen->RegionCopy = miRegionCopy;
    pScreen->RegionDestroy = miRegionDestroy;
    pScreen->RegionUninit = miRegionUninit;
    pScreen->Intersect = miIntersect;
    pScreen->Inverse = miInverse;
    pScreen->Union = miUnion;
    pScreen->Subtract = miSubtract;
    pScreen->RegionReset = miRegionReset;
    pScreen->TranslateRegion = miTranslateRegion;
    pScreen->RectIn = miRectIn;
    pScreen->PointInRegion = miPointInRegion;
    pScreen->WindowExposures = miWindowExposures;
    pScreen->PaintWindowBackground = cfbPaintWindow;
    pScreen->PaintWindowBorder = cfbPaintWindow;
    pScreen->CopyWindow = cfbCopyWindow;
    pScreen->ClearToBackground = miClearToBackground;

    pScreen->RegionNotEmpty = miRegionNotEmpty;
    pScreen->RegionEmpty = miRegionEmpty;
    pScreen->RegionExtents = miRegionExtents;
    pScreen->RegionAppend = miRegionAppend;
    pScreen->RegionValidate = miRegionValidate;
    pScreen->BitmapToRegion = mfbPixmapToRegion;
    pScreen->RectsToRegion = miRectsToRegion;
    pScreen->SendGraphicsExpose = miSendGraphicsExpose;

    pScreen->BlockHandler = NoopDDA;
    pScreen->WakeupHandler = NoopDDA;
    pScreen->blockData = (pointer)0;
    pScreen->wakeupData = (pointer)0;

    pScreen->CreateColormap = cfbInitializeColormap;
    pScreen->DestroyColormap = NoopDDA;

    pScreen->defColormap = FakeClientID(0);
    if (defaultColorVisualClass < 0)
    {
	i = 0;
    }
    else
    {
	for (i = 0;
	     (i < NUMVISUALS) && (visuals[i].class != defaultColorVisualClass);
	     i++)
	    ;
	if (i >= NUMVISUALS)
	    i = 0;
    }
    pScreen->rootVisual = visuals[i].vid;
    miInitializeBackingStore (pScreen, &cfbBSFuncRec);
    mfbRegisterCopyPlaneProc (pScreen, cfbCopyPlane);
#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif
    return TRUE;
}
