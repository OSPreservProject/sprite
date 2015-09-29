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

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <errno.h>

#define  NEED_EVENTS

#include "Ultrix2.0inc.h"
#include <vaxuba/qduser.h>
#include <vaxuba/qdioctl.h>
#include <vaxuba/qdreg.h>

#include "X.h"
#include "Xproto.h"	/* needed for xEvent */

#include "misc.h"
#include "colormapst.h"

#include "scrnintstr.h"
#include "pixmapstr.h"
#include "inputstr.h"
#include "cursorstr.h"		/* needed by ProcessInputEvents */

#include "mi.h"
#ifdef X11R4
#include "mibstore.h"
#endif
#include "qd.h"
#ifdef X11R4
#include "qdgc.h"
#endif
#include "qdprocs.h"
#include "libtl/tl.h"

static Bool qdSaveScreen();
void	qdFreeResource();
Bool	qdScreenClose();

extern void FatalError();
extern int  qdGetMotionEvents();
static void qdChangePointerControl(), qdChangeKeyboardControl(), qdBell();
static void qdBlockHandler(), QDClick();
extern void NoopDDA();

#ifdef X11R4
extern RegionPtr qdBitmapToRegion();
#endif

extern int errno;

int	fd_qdss;
int     Nplanes;
int     Nentries;
                      /* This variable will be set when tlinit is     */
                      /* called and we determine the number of planes */
int     Nchannels;
unsigned int   Allplanes;

static struct qdmap *	qdmap;
vsEventQueue *		queue;		/* shorthand to a struct in qdmap */
vsBox *			mbox;		/* shorthand to a struct in qdmap */
vsCursor *		mouse;		/* shorthand to a struct in qdmap */

DevicePtr		qdKeyboard;
DevicePtr		qdPointer;
int			lastEventTime;

static int		qLimit;		/* last slot in the event queue */

static int		InitialClickVolume = 20;

extern int		screenIsSaved;	/* written to by DIX! */
extern int		Vaxstar; /* set in libtl/tlinit.c */

static int              dpix = -1, dpiy = -1, dpi = -1;
static char             *blackValue = NULL, *whiteValue = NULL;
static int		class = PseudoColor;

#ifdef DEBUG
int ScreenHeight=864;
#else
#define ScreenHeight 864
#endif

#define NoSuchClass -1

static int
ParseClass(className)
    char *	className;
{
    static char *names[] = {
	"StaticGray", "GrayScale", "StaticColor",
	"PseudoColor", "TrueColor"};
    /* only the ones we support and must be in order from X.h, since
     * return value depends on index into array.
     */
    int i;
    for (i = 0; i < sizeof(names)/sizeof(char *); i++)
    {
	if (strcmp(names[i], className) == 0)
	    return i;
    }
    return NoSuchClass;
}

static Bool
commandLineMatch( argc, argv, pat, pmatch)
    int         argc;		/* may NOT be changed */
    char *      argv[];		/* may NOT be changed */
    char *	pat;
{
    int		ic;

    for ( ic=0; ic<argc; ic++)
	if ( strcmp( argv[ic], pat) == 0)
	    return TRUE;
    return FALSE;
}


static Bool
commandLinePairMatch( argc, argv, pat, pmatch)
    int         argc;		/* may NOT be changed */
    char *      argv[];		/* may NOT be changed */
    char *	pat;
    char **	pmatch;		/* RETURN */
{
    int		ic;

    for ( ic=0; ic<argc; ic++)
	if ( strcmp( argv[ic], pat) == 0)
	{
	    *pmatch = argv[ ic+1];
	    return TRUE;
	}
    return FALSE;
}

static
colorNameToColor( pname, pred, pgreen, pblue)
    char *	pname;
    u_int *	pred;
    u_int *	pgreen;
    u_int *	pblue;
{
    if ( *pname == '#')
    {
	pname++;		/* skip over # */
	sscanf( pname, "%2x", pred);
	*pred <<= 8;

	pname += 2;
	sscanf( pname, "%2x", pgreen);
	*pgreen <<= 8;

	pname += 2;
	sscanf( pname, "%2x", pblue);
	*pblue <<= 8;
    }
    else /* named color */
    {
	*pred = *pgreen = *pblue = 0; /*OsLookupColor thinks these are shorts*/
	OsLookupColor( 0 /*"screen", not used*/, pname, strlen( pname),
		pred, pgreen, pblue);
    }
}

/* there are 5 visuals of depth 8. Count them. */
#define	NUMVISUALS8	5

/* we may ignore the sixth entry if we so chose */
#ifdef BITMAP_VISUAL
#define NUMVISUALS1	1
#else
#define NUMVISUALS1	0
#endif
#define NUMVISUALS (NUMVISUALS8 + NUMVISUALS1)

#ifdef X11R4
#define VREC(class, rMask,gMask,bMask, oRed,oGreen,oBlue, bpRGB, cmpE, nplan)\
  0, class, bpRGB, cmpE, nplan ,rMask,gMask,bMask, oRed,oGreen,oBlue,
#else
#define VREC(class, rMask,gMask,bMask, oRed,oGreen,oBlue, bpRGB, cmpE, nplan)\
  0, 0, class, rMask,gMask,bMask, oRed,oGreen,oBlue, bpRGB, cmpE, nplan,
#endif
/* The RootVisual will be one of the first five of these */
/* On the original GPX, entries 254,255 in the colormap are reservered
   for the cursor.  This isn't so on the Vaxstar 8-plane GPX */
VisualRec visuals8_254[] = {
    /* table must be in same order as constants in X.h */
    /*   class  rMask gMask bMask oRed oGreen oBlue bpRGB cmpE nplan */
    VREC(StaticGray,  0,   0,    0,   0,    0,   0,    8,  254,   8)
    VREC(GrayScale,   0,   0,    0,   0,    0,   0,    8,  254,   8)
    VREC(StaticColor, 0,   0,    0,   0,    0,   0,    8,  254,   8)
    VREC(PseudoColor, 0,   0,    0,   0,    0,   0,    8,  254,   8)
    VREC(TrueColor,   0x70,0xC,  0x03,4,    2,   0,    8,  128,   7)
    VREC(StaticGray,  0,   0,    0,   0,    0,   0,    1,    2,   1)
};

VisualRec visuals8_256[] = {
    /* table must be in same order as constants in X.h */
    /*   class  rMask gMask bMask oRed oGreen oBlue bpRGB cmpE nplan */
    VREC(StaticGray,  0,   0,    0,   0,    0,   0,    8,  256,   8)
    VREC(GrayScale,   0,   0,    0,   0,    0,   0,    8,  256,   8)
    VREC(StaticColor, 0,   0,    0,   0,    0,   0,    8,  256,   8)
    VREC(PseudoColor, 0,   0,    0,   0,    0,   0,    8,  256,   8)
    VREC(TrueColor,   0xE0,0x1C, 0x03,5,    2,   0,    8,  256,   8)
    VREC(StaticGray,  0,   0,    0,   0,    0,   0,    1,    2,   1)
};

VisualRec visuals4[] = {
    /* table must be in same order as constants in X.h */
    /*   class  rMask gMask bMask oRed oGreen oBlue bpRGB cmpE nplan */
    VREC(StaticGray,  0,   0,    0,   0,    0,   0,    8,  16,   4)
    VREC(GrayScale,   0,   0,    0,   0,    0,   0,    8,  16,   4)
    VREC(StaticColor, 0,   0,    0,   0,    0,   0,    8,  16,   4)
    VREC(PseudoColor, 0,   0,    0,   0,    0,   0,    8,  16,   4)
    VREC(TrueColor,   0xc, 0x2,  0x01,2,    1,   0,    8,  16,   4)
    VREC(StaticGray,  0,   0,    0,   0,    0,   0,    1,   2,   1)
};

#ifdef X11R4
static VisualID VIDs[NUMVISUALS8];
#endif

DepthRec depths[] = {
/* depth        numVid          vids */
    1,          NUMVISUALS1,    NULL,
#ifdef X11R4
    8,          NUMVISUALS8,    VIDs,
#else
    8,          NUMVISUALS8,    NULL,
#endif
};

#define NUMDEPTHS       ((sizeof (depths))/(sizeof (DepthRec)))

static int visualIndices[] = {
	NUMVISUALS8,	/* first depth (1) */
	0	/* second depth (8) */
};

#ifdef X11R4
static unsigned long qdGeneration = 0;
int qdGCPrivateIndex = 0;
extern void tlSaveAreas(), tlRestoreAreas();
miBSFuncRec qdBSFuncRec = {
    tlSaveAreas,
    tlRestoreAreas,
    (void (*)()) 0,
    (PixmapPtr (*)()) 0,
    (PixmapPtr (*)()) 0,
};
#endif

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
/* Inits 4 or 8 plane StaticColor, PseudoColor, StaticGray, or GrayScale  */
static Bool
qdVisualInit(index, pScreen, xsize, ysize, dpix, dpiy, width, class)
    int index;
    register ScreenPtr pScreen;

    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;			/* dots per inch */
    int width;			/* pixelwidth of frame buffer */
    int	class;
{
    VisualID	*pVids;
    register PixmapPtr pPixmap;
    int	i, j, curVid;
    ColormapPtr	rootColormap;

    if (Nplanes == 8) 
        pScreen->visuals = (Vaxstar) ? visuals8_256 : visuals8_254;
    else
        pScreen->visuals = visuals4;
#ifdef X11R4
    if (qdGeneration != serverGeneration) {
	mfbGCPrivateIndex = AllocateGCPrivateIndex();
	qdGCPrivateIndex = AllocateGCPrivateIndex();
	qdGeneration = serverGeneration;
	for (i = 0; i < NUMVISUALS8; i++) {
	    pScreen->visuals[i].vid = FakeClientID(0);
	    VIDs[i] = pScreen->visuals[i].vid;
	}
    }
    AllocateGCPrivate(pScreen, mfbGCPrivateIndex, sizeof(mfbPrivGC));
    AllocateGCPrivate(pScreen, qdGCPrivateIndex, sizeof(QDPrivGCRec));
#endif

    pScreen->CreateColormap = qdCreateColormap;
    pScreen->DestroyColormap = qdDestroyColormap;
    pScreen->InstallColormap = qdInstallColormap;
    pScreen->UninstallColormap = qdUninstallColormap;
    pScreen->ListInstalledColormaps = qdListInstalledColormaps;
    pScreen->StoreColors = qdStoreColors;
    pScreen->ResolveColor = qdResolveColor;


    if (Nplanes == 4)
        depths[1].depth = 4;
    /* you can have a server that is StaticGray only, but not
     * StaticColor only */
    if (class == StaticGray) 
    {
	depths[0].numVids = 0;
	depths[1].numVids = 1;
    }
    pScreen->myNum = index;
    pScreen->width = xsize;
    pScreen->height = ysize;
    pScreen->mmWidth = (xsize * 254) / (dpix * 10);
    pScreen->mmHeight = (ysize * 254) / (dpiy * 10);
    pScreen->numDepths = NUMDEPTHS;
    pScreen->allowedDepths = depths;

    pScreen->rootDepth = ((Nplanes == 4) ? 4 : 8);
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->backingStoreSupport = Always;
    pScreen->saveUnderSupport = NotUseful;

    /* cursmin and cursmax are device specific */ 

    pScreen->numVisuals = depths[0].numVids + depths[1].numVids;

    /*  Set up the remaining fields in the visuals[] array & make a
     * RT_VISUALID */

#ifndef X11R4
    for (i = 0; i < NUMDEPTHS; i++) {

	if (depths[i].numVids > 0) {
	   depths[i].vids = pVids = (VisualID *) xalloc(sizeof (VisualID) *
							 depths[i].numVids);
	    for(j = 0; j < depths[i].numVids; j++) {
               int v = j + visualIndices[i];
   	       pVids[j] = pScreen->visuals[v].vid = FakeClientID(0);
	       pScreen->visuals[v].screen = index;
	        AddResource(pScreen->visuals[v].vid, RT_VISUALID,
		    (pointer)&(pScreen->visuals[v]),
		    NoopDDA, RC_CORE);
	    }
	}
    }
#endif

    switch (class) {
    case StaticGray:
    case StaticColor:
    case TrueColor:
	CreateColormap(FakeClientID(0), pScreen, &(pScreen->visuals[class]), 
	    &rootColormap, AllocAll, 0);
	break;
    case PseudoColor:
    case GrayScale:
	CreateColormap(FakeClientID(0), pScreen, &(pScreen->visuals[class]), 
	    &rootColormap, AllocNone, 0);
	break;
    case DirectColor:
	FatalError("Bad visual in qdVisualInit\n");
    }

    pScreen->defColormap = rootColormap->mid;
    pScreen->rootVisual = pScreen->visuals[class].vid;

#ifdef X11R4
    miInitializeBackingStore (pScreen, &qdBSFuncRec);
#endif

    return( TRUE );
}

Bool
qdScreenInit( index, pScr, argc, argv)
    int		index;
    register ScreenPtr pScr;
    int		argc;		/* these two may NOT be changed */
    char *	argv[];
{
    ColormapPtr		pColormap;	/* returned by CreateColormap */
    DepthPtr    	pDepth;
    struct qdinput *	pQdinput;
    char *		pblackname;
    char *		pwhitename;
    char *		pdragonpix;	/* max size offscreen pixmap */
    int			i;

    extern ColormapPtr	pInstalledMap;	/* lives in qdcolor.c */
    extern int		defaultBackingStore;

    /*
     * defaults: may be overridden by command line
     * should allow named colors		XXX
     * should also look at Xdefaults?		XX
     */
    u_int blackred	= 0x0000;
    u_int blackgreen	= 0x0000;
    u_int blackblue	= 0x0000;

    u_int whitered	= 0xffff;
    u_int whitegreen	= 0xffff;
    u_int whiteblue	= 0xffff;

    if ( commandLineMatch( argc, argv, "-C")) /* conform to protocol */
    {
	ErrorF( "Xqdss: using slow, protocol-conforming polygon code");
	slowPolygons();
    }

    /* setup max y size of offscreen pixmaps */
    if ( commandLinePairMatch( argc, argv, "-dp", &pdragonpix))
    {
	sscanf( pdragonpix, "%d", &DragonPix);
	if (DragonPix < 0)
	    DragonPix = 0;
	if (DragonPix > 2048-ScreenHeight)
	    DragonPix = 2048-ScreenHeight; /* full screen - visible screen */
    }
    /*
     * turn off console output, which would hose DMA work in progress.
     * is this sufficient?	XX
     */
    ioctl( fd_qdss, QD_KERN_LOOP);

    /*
     * set keyclick, mouse acceleration and threshold
     */
    {
    char *	clickvolume;
    char *	mouseAcceleration;
    int		ma = 4;
    char *	mouseThreshold;
    int		mt = 4;
    PtrCtrl	ctrl;

    if ( commandLinePairMatch( argc, argv, "c", &clickvolume))
	sscanf( clickvolume, "%d", &InitialClickVolume);
    if ( commandLineMatch( argc, argv, "-c"))
	InitialClickVolume = 0;

    /*
     * calling qdChangePointerControl here may be unclean	XX
     */
    if ( commandLinePairMatch( argc, argv, "-a", &mouseAcceleration))
	sscanf( mouseAcceleration, "%d", &ma);
    if ( commandLinePairMatch( argc, argv, "-t", &mouseThreshold))
	sscanf( mouseThreshold, "%d", &mt);
    ctrl.num = ma;
    ctrl.den = 1;
    ctrl.threshold = mt;
    qdChangePointerControl( (DevicePtr) NULL, &ctrl);
    }

    /*
     * now get the address (in system space) of the event queue and
     * associated headers.
     */
    if ( ioctl( fd_qdss, QD_MAPEVENT, &pQdinput) == -1)
    {
	ErrorF( "qdScreenInit:  QD_MAPEVENT ioctl failed\n");
	return FALSE;
    }

    mouse = (vsCursor *) &pQdinput->curs_pos;
    mbox = (vsBox *) &pQdinput->curs_box;
    queue = (vsEventQueue *) &pQdinput->header;
    qLimit = queue->size - 1;

    if (dpi == -1) /* dpi has not been set */
    {
        if (dpix == -1) /* ie dpix has not been set */
        {
	    if (dpiy == -1)
	    {
	        dpix = 78;
	        dpiy = 78;
	    }
	    else
	        dpix = dpiy;
        }
        else
        { 
	    if (dpiy == -1)
	        dpiy = dpix;
        }
    }
    else
    {
	dpix = dpi;
	dpiy = dpi;
    }

    qdVisualInit(index, pScr, 1024, ScreenHeight, dpix, dpiy, 1024, class);

    /*
     * there is no pixmap corresponding to the root window
     */
    pScr->devPrivate = (pointer) NULL;

    pScr->CreateGC = qdCreateGC;

    pScr->CloseScreen= qdScreenClose;
    pScr->QueryBestSize = qdQueryBestSize;
    pScr->SaveScreen = qdSaveScreen;
    pScr->GetImage = qdGetImage;
    pScr->PointerNonInterestBox = qdPointerNonInterestBox;

    pScr->GetSpans = qdGetSpans;

    pScr->CreateWindow = qdCreateWindow;
    pScr->DestroyWindow = qdDestroyWindow;
    pScr->PositionWindow = qdPositionWindow;
    pScr->ChangeWindowAttributes = qdChangeWindowAttributes;
    pScr->RealizeWindow = qdMapWindow;
    pScr->UnrealizeWindow = qdUnmapWindow;
    pScr->ValidateTree = miValidateTree;
    pScr->WindowExposures = miWindowExposures;

    pScr->CreatePixmap = qdCreatePixmap;
    pScr->DestroyPixmap = qdDestroyPixmap;

    pScr->RealizeFont = qdRealizeFont;
    pScr->UnrealizeFont = qdUnrealizeFont;

    pScr->ConstrainCursor = qdConstrainCursor;
    pScr->CursorLimits = qdCursorLimits;
    pScr->DisplayCursor = qdDisplayCursor;
    pScr->RealizeCursor = qdRealizeCursor;
    pScr->UnrealizeCursor = qdUnrealizeCursor;
    pScr->RecolorCursor = miRecolorCursor;
    pScr->SetCursorPosition = qdSetCursorPosition;

    pScr->RegionCreate = miRegionCreate;	/* only mi from here down */
#ifdef X11R4
    pScr->RegionInit = miRegionInit;
    pScr->RegionUninit = miRegionUninit;
    pScr->BitmapToRegion = qdBitmapToRegion;
    pScr->RectsToRegion = miRectsToRegion;
#endif
    pScr->RegionCopy = miRegionCopy;
    pScr->RegionAppend = miRegionAppend;
    pScr->RegionValidate = miRegionValidate;
    pScr->RegionDestroy = miRegionDestroy;
    pScr->Intersect = miIntersect;
    pScr->Union = miUnion;
    pScr->Subtract = miSubtract;
    pScr->Inverse = miInverse;
    pScr->RegionReset = miRegionReset;
    pScr->TranslateRegion = miTranslateRegion;
    pScr->RectIn = miRectIn;
    pScr->PointInRegion = miPointInRegion;
    pScr->RegionNotEmpty = miRegionNotEmpty;
    pScr->RegionEmpty = miRegionEmpty;
    pScr->RegionExtents = miRegionExtents;
    pScr->SendGraphicsExpose = miSendGraphicsExpose;
    pScr->BlockHandler = qdBlockHandler;
    pScr->WakeupHandler = NoopDDA;
#ifdef X11R4
    pScr->ClearToBackground = miClearToBackground;
    pScr->PaintWindowBackground = miPaintWindow;
    pScr->PaintWindowBorder = miPaintWindow;
    pScr->CopyWindow = qdCopyWindow;
    pScr->SourceValidate = (void *) NULL;  /* pDrawable, x, y, w, h */
#endif

#if 1
    {
    VisualPtr	pVisual;
    for (pVisual = pScr->visuals;
	 pVisual->vid != pScr->rootVisual;
	 pVisual++)
	;

    CreateColormap(pScr->defColormap, pScr, pVisual, &pColormap,
		   AllocNone, 0);
}
#else
    CreateColormap(pScr->defColormap, pScr,
                   LookupID(pScr->rootVisual, RT_VISUALID, RC_CORE),
                   &pColormap, AllocNone, 0);
#endif
    if (class == PseudoColor)
    {
        if (!Vaxstar)
        {
            pScr->blackPixel = 253;
            pScr->whitePixel = 252;
	}
	else
	{
            pScr->blackPixel = (Nplanes == 4 ? 15 : 255);
            pScr->whitePixel = (Nplanes == 4 ? 14 : 254);
	}
        if(blackValue)
            colorNameToColor(blackValue, &blackred, &blackgreen, &blackblue); 

        if(whiteValue)
            colorNameToColor(whiteValue, &whitered, &whitegreen, &whiteblue);

        if ( AllocColor( pColormap,
            (u_short *)&blackred, (u_short *)&blackgreen,
				(u_short *)&blackblue,
                                 &pScr->blackPixel, 0) != Success
          || AllocColor( pColormap,
            (u_short *)&whitered, (u_short *)&whitegreen,
				(u_short *)&whiteblue,
                                 &pScr->whitePixel, 0) != Success)
        {
            ErrorF( "qdScreenInit: AllocColor failed\n");
            return FALSE;
        }
    }
    else   /* the map is filled in with white in pixel N, black in pixel 0 */
    {
            pScr->blackPixel = 0;
            pScr->whitePixel = pScr->visuals[class].ColormapEntries - 1;
    }
    qdInstallColormap(pColormap);
    return TRUE;
}

Bool
qdScreenClose( index, pScr)
    int index;		/* NOT USED */
    ScreenPtr pScr;	/* NOT USED */
{
    int	errno;

    if ( (errno=tlCleanup()) != 0)
    {
	ErrorF("Closing QDSS yielded %d\n", errno);
	return (FALSE);
    }
    return (TRUE);
}

extern struct qdmap	Qdss;

static Bool
qdSaveScreen(pScr, on)
    ScreenPtr pScr;   /* NOT USED */
    Bool on;
{
    if (on != SCREEN_SAVER_ON)
    {
        lastEventTime = GetTimeInMillis();
	if (!Vaxstar)
	    *(short *) Qdss.memcsr = UNBLANK | SYNC_ON; 
	else
	    ioctl(fd_qdss, SG_VIDEOON, &Sg);
    }
    else
    {
	if (!Vaxstar)
	    *(short *) Qdss.memcsr = SYNC_ON; 
	else
	    ioctl(fd_qdss, SG_VIDEOOFF, &Sg);
    }
    return TRUE;
}


void
qdFreeResource( p, id)
    pointer p;
    int id;
{
    Xfree(p);
}

int
qdMouseProc( pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff;
    int argc;
    char *argv[];
{
    BYTE map[4];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    qdPointer = pDev;
	    pDev->devicePrivate = (pointer) &queue;
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
	      qdPointer, map, 3, qdGetMotionEvents, qdChangePointerControl, 0);
	    SetInputCheck(&queue->head, &queue->tail);
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice( fd_qdss);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
/*	RemoveEnabledDevice( fd_qdss);   */
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;

}

int
qdKeybdProc( pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff;
    int argc;
    char *argv[];
{
    KeySymsRec keySyms;
    CARD8 modMap[MAP_LENGTH];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    qdKeyboard = pDev;
	    pDev->devicePrivate = (pointer) & queue;
            GetLK201Mappings( &keySyms, modMap);
	    InitKeyboardDeviceStruct(
		    qdKeyboard, &keySyms, modMap, qdBell,
		    qdChangeKeyboardControl);
            QDClick( pDev, InitialClickVolume);

	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice( fd_qdss);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
/*	    RemoveEnabledDevice( fd_qdss);  */
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}


#ifdef USE_FUNKY_MALLOC
extern int shouldDumpArena;
#endif

/*
 * processes all the pending input events
 */
void
ProcessInputEvents()
{
#define DEVICE_KEYBOARD 2
    register int    i;
    register    vsEvent * pE;
    xEvent	x;
    int     nowInCentiSecs, nowInMilliSecs, adjustCentiSecs;
    struct timeval  tp;
    int     needTime = 1;
    extern CursorRec CurrentCurs;	/* defined in qdcursor.c */
    extern int hotX, hotY;

#ifdef USE_FUNKY_MALLOC
    if (shouldDumpArena) {
	dumpArena();
    }
#endif
    i = queue->head;
    while (i != queue->tail)
    {
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	pE = &queue->events[i];
	x.u.keyButtonPointer.rootX = pE->vse_x + hotX;
	x.u.keyButtonPointer.rootY = pE->vse_y + hotY;

	if (sizeof(pE->vse_time) == 4)
	    x.u.keyButtonPointer.time = lastEventTime = pE->vse_time;
	else {
	    /* 
	     * The following silly looking code is because the old version of the
	     * driver only delivers 16 bits worth of centiseconds. We are supposed
	     * to be keeping time in terms of 32 bits of milliseconds.
	     */
	    if (needTime)
	    {
		needTime = 0;
		gettimeofday(&tp, 0);
		nowInCentiSecs = ((tp.tv_sec * 100) + (tp.tv_usec / 10000)) & 0xFFFF;
	    /* same as driver */
		nowInMilliSecs = (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
	    /* beware overflow */
	    }
	    if ((adjustCentiSecs = nowInCentiSecs - pE->vse_time) < -20000)
		adjustCentiSecs += 0x10000;
	    else
		if (adjustCentiSecs > 20000)
		    adjustCentiSecs -= 0x10000;
	    x.u.keyButtonPointer.time = lastEventTime =
		nowInMilliSecs - adjustCentiSecs * 10;
	}

	if ((pE->vse_type == VSE_BUTTON) &&
	    (pE->vse_device == DEVICE_KEYBOARD))
	{					/* better be a button */
	    x.u.u.detail = pE->vse_key;
	    switch (pE->vse_direction)
	    {
		case VSE_KBTDOWN: 
		    x.u.u.type = KeyPress;
		    (qdKeyboard->processInputProc)(&x, qdKeyboard, 1);
		    break;
		case VSE_KBTUP: 
		    x.u.u.type = KeyRelease;
		    (qdKeyboard->processInputProc)(&x, qdKeyboard, 1);
		    break;
		default: 	       /* hopefully BUTTON_RAW_TYPE */
		    ProcessLK201Input(&x, qdKeyboard);
	    }
	}
	else
	{
	    if (pE->vse_type == VSE_BUTTON)
	    {
		if (pE->vse_direction == VSE_KBTDOWN)
		    x.u.u.type = ButtonPress;
		else
		    x.u.u.type = ButtonRelease;
		/* mouse buttons numbered from one */
		x.u.u.detail = pE->vse_key + 1;
	    }
	    else {
#ifndef NO_EVENT_COMPRESSION
		int j = (i == qLimit) ? 0 : i + 1;
		/*
		 * to get here we knew that 
		 *
		 *     (vse_type != VSE_BUTTON || 
		 *      vse_device != DEVICE_KEYBOARD) && 
		 *     (vse_type != VSE_BUTTON)
		 *
		 * which means that for the next event to be a mouse
		 * motion, it must satisfy vse_type != VSE_BUTTON
		 *
		 * XXX -- We should implement motion history since we are 
		 * throwing device events away....
		 */
		if (j != queue->tail &&
		    queue->events[j].vse_type != VSE_BUTTON)
		  goto next;		/* sometimes the dragon wins */

#endif
		/* tell the server that the mouse moved */
		x.u.u.type = MotionNotify;
	    }
	    (* qdPointer->processInputProc)(&x, qdPointer, 1);
	}

      next:
	if (i == qLimit)
	    i = queue->head = 0;
	else
	    i = ++queue->head;
    }
    dmafxns.flush ( FALSE);	/* necessary for interaction. due to grabs? */
#undef DEVICE_KEYBOARD
}

TimeSinceLastInputEvent()
{
    if (lastEventTime == 0)
	lastEventTime = GetTimeInMillis();
    return GetTimeInMillis() - lastEventTime;
}

static void
qdChangePointerControl(pDevice, ctrl)
    DevicePtr pDevice;
    PtrCtrl   *ctrl;
{
    struct prg_cursor	curs;

    curs.threshold = ctrl->threshold;
    if ( (curs.acc_factor = ctrl->num / ctrl->den) == 0)
	curs.acc_factor = 1;	/* watch for den > num */
    ioctl( fd_qdss, QD_PRGCURSOR, &curs);
}

int
qdGetMotionEvents( pDevice, buff, start, stop)
    DevicePtr	pDevice;
    CARD32	start, stop;
    xTimecoord *buff;
{
    return 0;
}

#define MAX_LED 3  /* only 3 LED's can be set by user; Lock LED is controlled by server */
static void
ChangeLED( led, on)
    int		led;
    Bool	on;
{
    struct prgkbd ioc;

    switch (led) {
       case 1:
	  ioc.param1 = LED_1;
	  break;
       case 2:
          ioc.param1 = LED_2;
	  break;
       case 3:
          /* the keyboard's LED_3 is the Lock LED, which the server owns.
             So the user's LED #3 maps to the keyboard's LED_4. */
          ioc.param1 = LED_4;
	  break;
       default:
	  return;   /* out-of-range LED value */
	  }
    ioc.cmd = on ? LK_LED_ENABLE : LK_LED_DISABLE;
    ioc.param2  = 0;
    ioctl(fd_qdss, QD_PRGKBD, &ioc);
}

SetLockLED (on)
    Bool on;
    {
    struct prgkbd ioc;
    ioc.cmd = on ? LK_LED_ENABLE : LK_LED_DISABLE;
    ioc.param1 = LED_3;
    ioc.param2 = 0;
    ioctl(fd_qdss, QD_PRGKBD, &ioc);
    }

static void
qdChangeKeyboardControl(device, ctrl)
    DevicePtr	device;
    KeybdCtrl *	ctrl;
{
    int i;

    QDClick( device, ctrl->click);

    /* ctrl->bell: the DIX layer handles the base volume for the bell */
    
    /* ctrl->bell_pitch: as far as I can tell, you can't set this on lk201 */

    /* ctrl->bell_duration: as far as I can tell, you can't set this  */

    /* LEDs */
    for (i=1; i<=MAX_LED; i++)
        ChangeLED(i, (ctrl->leds & (1 << (i-1))));

    /* ctrl->autoRepeat: I'm turning it all on or all off.  */

    SetLKAutoRepeat(ctrl->autoRepeat);
}

static void
QDClick( device, click)
    DevicePtr   device;
    int		click;
{
#define LK_ENABLE_CLICK 0x1b	/* enable keyclick / set volume	*/
#define LK_DISABLE_CLICK 0x99	/* disable keyclick entirely	*/
#define LK_ENABLE_BELL 0x23	/* enable bell / set volume 	*/

    struct prgkbd ioc;

    if (click == 0)    /* turn click off */
    {
	ioc.cmd = LK_DISABLE_CLICK;
	ioc.param1 = 0;
    }
    else 
    {
        int volume;

	volume = 7 - ((click / 14) & 0x7);
	ioc.cmd = LK_ENABLE_CLICK;
	ioc.param1 = volume | LAST_PARAM;
    }
    ioc.param2 = 0;
    ioctl(fd_qdss, QD_PRGKBD, &ioc);

}

static void
qdBell( loud, pDevice)
    int		loud;
    DevicePtr	pDevice;
{
    struct prgkbd ioc;

    /*
     * the lk201 volume is between 7 (quiet but audible) and 0 (loud)
     */

    if (loud == 0)
	return;
	
    loud = 7 - ((loud / 14) & 7);

    ioc.cmd = LK_ENABLE_BELL;
    ioc.param1 = loud | LAST_PARAM;
    ioc.param2 = 0;

    ioctl( fd_qdss, QD_PRGKBD, &ioc);
    ioc.cmd = LK_RING_BELL | LAST_PARAM;
    ioc.param1 = 0;
    ioctl( fd_qdss, QD_PRGKBD, &ioc);
}

#define LK_REPEAT_ON  0xe3
#define LK_REPEAT_OFF 0xe1

int
SetLKAutoRepeat (onoff)
    Bool onoff;
{
    extern char *AutoRepeatLKMode();
    extern char *UpDownLKMode();
    
    struct prgkbd	ioc;
    register char *	divsets;

    ioc.param1 = 0;
    divsets = onoff ? (char *) AutoRepeatLKMode() : (char *) UpDownLKMode();
    while (ioc.cmd = *divsets++)
	ioctl(fd_qdss, QD_PRGKBD, &ioc);
    ioc.cmd = ((onoff > 0) ? LK_REPEAT_ON : LK_REPEAT_OFF);
    return( ioctl( fd_qdss, QD_PRGKBD, &ioc));
}

static void
qdBlockHandler( iscr, data)
    int		iscr;
    pointer	data;
{
    dmafxns.flush ( FALSE);
}

/*
 * DDX - specific abort routine.  Called by AbortServer().
 */
void
AbortDDX()
{
}

/* Called by GiveUp(). */
void
ddxGiveUp()
{
}

int
ddxProcessArgument (argc, argv, i)
    int	argc;
    char *argv[];
    int	i;
{
    int			argind=i;
    int			skip;
    static int		Once=0;
    void		ddxUseMsg();

    skip = 0;
    if (!Once)
    {
        blackValue = NULL;
        whiteValue = NULL;
	Once = 1;
    }

    if (strcmp( argv[argind], "-dpix") == 0)
    {
	if (++argind < argc)
	{
	    dpix = atoi(argv[argind]);
	    skip = 2;
	}
	else
	    return 0;	/* failed to parse */
    }
    else if (strcmp( argv[argind], "-dpiy") == 0)
    {
	if (++argind < argc)
	{
	    dpiy = atoi(argv[argind]);
	    skip = 2;
	}
	else
	    return 0;
    }
    else if (strcmp( argv[argind], "-dpi") == 0)
    {
	if (++argind < argc)
	{
	    dpi = atoi(argv[argind]);
	    dpix = dpi;
	    dpiy = dpi;
	    skip = 2;
	}
	else
	    return 0;
    }
    else 
    if(strcmp(argv[argind], "-bp") == 0 )
    {
	    if(++argind < argc)
	    {
		blackValue = argv[argind];
		skip = 2;
	    }
	    else
	        return 0;
    }
    else
    if(strcmp(argv[argind], "-wp") == 0 )
    {
	    if(++argind < argc)
	    {
		whiteValue = argv[argind];
		skip = 2;
	    }
	    else
	        return 0;
    }
    else
    if (strcmp(argv[argind], "-class") == 0 )
    {
	    if(++argind < argc)
	    {
		class = ParseClass(argv[argind]);
		if (class == NoSuchClass)
		    return 0;
		skip = 2;
	    }
	    else
	        return 0;
    }
#ifdef USE_FUNKY_MALLOC
    if (strcmp(argv[argind],"-plumber")==0) {
	    if (++argind < argc) {
		setupPlumber(argv[argind]);
		skip = 2;
	    }
	    else {
		ddxUseMsg();
		skip = 1;
	    }
    }
#endif
    return skip;
}

void
ddxUseMsg()
{
    ErrorF ("\n");
    ErrorF ("\n");
    ErrorF ("Device Dependent Usage\n");
    ErrorF ("\n");
    ErrorF ("-dpix <n>          Dots per inch, x coordinate\n");
    ErrorF ("-dpiy <n>          Dots per inch, y coordinate\n");
    ErrorF ("-dpi <n>           Dots per inch, x and y coordinates\n");
    ErrorF ("                   (overrides -dpix and -dpiy above)\n");
    ErrorF ("-bp #XXX           color for BlackPixel for screen\n");
    ErrorF ("-wp #XXX           color for WhitePixel for screen\n");
    ErrorF ("-class <classname> type of Visual for root window\n");
    ErrorF ("       one of StaticGray, StaticColor, PseudoColor,\n");
    ErrorF ("       GrayScale, or even TrueColor!\n");
#ifdef USE_FUNKY_MALLOC
    ErrorF ("-plumber file	Set plumber to dump to file when signalled\n");
#endif
}
