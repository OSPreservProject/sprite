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
/* $XConsortium: mfbscrinit.c,v 5.10 90/01/23 15:39:27 rws Exp $ */

#include "X.h"
#include "Xproto.h"	/* for xColorItem */
#include "Xmd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "mfb.h"
#include "mistruct.h"
#include "dix.h"
#include "mi.h"
#include "mibstore.h"
#include "servermd.h"

extern RegionPtr mfbPixmapToRegion();

int mfbWindowPrivateIndex;
int mfbGCPrivateIndex;
static unsigned long mfbGeneration = 0;

static VisualRec visual = {
/* vid  class       bpRGB cmpE nplan rMask gMask bMask oRed oGreen oBlue */
   0,   StaticGray, 1,    2,   1,    0,    0,    0,    0,   0,     0
};

static VisualID VID;

static DepthRec depth = {
/* depth	numVid		vids */
    1,		1,		&VID
};

miBSFuncRec mfbBSFuncRec = {
    mfbSaveAreas,
    mfbRestoreAreas,
    (void (*)()) 0,
    (PixmapPtr (*)()) 0,
    (PixmapPtr (*)()) 0,
};

Bool
mfbAllocatePrivates(pScreen, pWinIndex, pGCIndex)
    ScreenPtr pScreen;
    int *pWinIndex, *pGCIndex;
{
    if (mfbGeneration != serverGeneration)
    {
	mfbWindowPrivateIndex = AllocateWindowPrivateIndex();
	mfbGCPrivateIndex = AllocateGCPrivateIndex();
	visual.vid = FakeClientID(0);
	VID = visual.vid;
	mfbGeneration = serverGeneration;
    }
    if (pWinIndex)
	*pWinIndex = mfbWindowPrivateIndex;
    if (pGCIndex)
	*pGCIndex = mfbGCPrivateIndex;
    return (AllocateWindowPrivate(pScreen, mfbWindowPrivateIndex,
				  sizeof(mfbPrivWin)) &&
	    AllocateGCPrivate(pScreen, mfbGCPrivateIndex, sizeof(mfbPrivGC)));
}

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
Bool
mfbScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
    int width;			/* pixel width of frame buffer */
{
    register PixmapPtr pPixmap;

    if 	(!mfbAllocatePrivates(pScreen, (int *)NULL, (int *)NULL))
	return FALSE;
    pPixmap = (PixmapPtr)xalloc(sizeof(PixmapRec));
    if (!pPixmap)
	return FALSE;

    pScreen->width = xsize;
    pScreen->height = ysize;
    pScreen->mmWidth = (xsize * 254) / (dpix * 10);
    pScreen->mmHeight = (ysize * 254) / (dpiy * 10);
    pScreen->numDepths = 1;
    pScreen->allowedDepths = &depth;

    pScreen->rootDepth = 1;
    pScreen->rootVisual = VID;
    pScreen->defColormap = (Colormap) FakeClientID(0);
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->backingStoreSupport = Always;
    pScreen->saveUnderSupport = NotUseful;

    /* cursmin and cursmax are device specific */

    pScreen->numVisuals = 1;
    pScreen->visuals = &visual;

    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.depth = 1;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.serialNumber = 0;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = xsize;
    pPixmap->drawable.height = ysize;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate.ptr = pbits;
    pPixmap->devKind = PixmapBytePad(width, 1);
    pScreen->devPrivate = (pointer)pPixmap;

    /* anything that mfb doesn't know about is assumed to be done
       elsewhere.  (we put in no-op only for things that we KNOW
       are really no-op.
    */
    pScreen->CreateWindow = mfbCreateWindow;
    pScreen->DestroyWindow = mfbDestroyWindow;
    pScreen->PositionWindow = mfbPositionWindow;
    pScreen->ChangeWindowAttributes = mfbChangeWindowAttributes;
    pScreen->RealizeWindow = mfbMapWindow;
    pScreen->UnrealizeWindow = mfbUnmapWindow;

    pScreen->RealizeFont = mfbRealizeFont;
    pScreen->UnrealizeFont = mfbUnrealizeFont;
    pScreen->CloseScreen = mfbCloseScreen;
    pScreen->QueryBestSize = mfbQueryBestSize;
    pScreen->GetImage = mfbGetImage;
    pScreen->GetSpans = mfbGetSpans;
    pScreen->SourceValidate = (void (*)()) 0;
    pScreen->CreateGC = mfbCreateGC;
    pScreen->CreatePixmap = mfbCreatePixmap;
    pScreen->DestroyPixmap = mfbDestroyPixmap;
    pScreen->ValidateTree = miValidateTree;

    pScreen->InstallColormap = mfbInstallColormap;
    pScreen->UninstallColormap = mfbUninstallColormap;
    pScreen->ListInstalledColormaps = mfbListInstalledColormaps;
    pScreen->StoreColors = NoopDDA;
    pScreen->ResolveColor = mfbResolveColor;
    pScreen->CreateColormap = mfbCreateColormap;
    pScreen->DestroyColormap = mfbDestroyColormap;

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
    pScreen->PaintWindowBackground = mfbPaintWindow;
    pScreen->PaintWindowBorder = mfbPaintWindow;
    pScreen->CopyWindow = mfbCopyWindow;
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
    pScreen->blockData = (pointer)NULL;
    pScreen->wakeupData = (pointer)NULL;

    miInitializeBackingStore (pScreen, &mfbBSFuncRec);
#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif
    return TRUE;
}
