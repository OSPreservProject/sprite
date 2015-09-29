/*-
 * sunCG4C.c --
 *	Functions to support the sun CG4 board as a memory frame buffer.
 */

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

#ifndef	lint
static char sccsid[] = "@(#)sunCG4C.c	1.4 6/1/87 Copyright 1987 Sun Micro";
#endif

#include    "sun.h"

#include    <sys/mman.h>
#include    <pixrect/memreg.h>
#include    <sundev/cg4reg.h>
#include    "colormap.h"
#include    "colormapst.h"
#include    "resource.h"
#include    <struct.h>

/*-
 * The cg4 frame buffer is divided into several pieces.
 *	1) an array of 8-bit pixels
 *	2) a one-bit deep overlay plane
 *	3) an enable plane
 *	4) a colormap and status register
 *
 * XXX - put the cursor in the overlay plane
 */
#define	CG4_HEIGHT	900
#define	CG4_WIDTH	1152

typedef struct cg4c {
	u_char mpixel[128*1024];		/* bit-per-pixel memory */
	u_char epixel[128*1024];		/* enable plane */
	u_char cpixel[CG4_HEIGHT][CG4_WIDTH];	/* byte-per-pixel memory */
} CG4C, CG4CRec, *CG4CPtr;

#define CG4C_IMAGE(fb)	    ((caddr_t)(&(fb)->cpixel))
#define CG4C_IMAGEOFF	    ((off_t)0x0)
#define CG4C_IMAGELEN	    (((CG4_HEIGHT*CG4_WIDTH + 8191)/8192)*8192)
#define	CG4C_MONO(fb)	    ((caddr_t)(&(fb)->mpixel))
#define	CG4C_MONOLEN	    (128*1024)
#define	CG4C_ENABLE(fb)	    ((caddr_t)(&(fb)->epixel))
#define	CG4C_ENBLEN	    CG4C_MONOLEN

static CG4CPtr CG4Cfb = NULL;

static int  sunCG4CScreenIndex;

/* XXX - next line means only one CG4 - fix this */
static ColormapPtr sunCG4CInstalledMap;

extern int TellLostMap(), TellGainedMap();

static void
sunCG4CUpdateColormap(pScreen, index, count, rmap, gmap, bmap)
    ScreenPtr	pScreen;
    int		index, count;
    u_char	*rmap, *gmap, *bmap;
{
    struct fbcmap sunCmap;

    sunCmap.index = index;
    sunCmap.count = count;
    sunCmap.red = &rmap[index];
    sunCmap.green = &gmap[index];
    sunCmap.blue = &bmap[index];

#ifdef SUN_WINDOWS
    if (sunUseSunWindows()) {
	static Pixwin *pw = 0;

	if (! pw) {
	    if ( ! (pw = pw_open(windowFd)) )
		FatalError( "sunCG4CUpdateColormap: pw_open failed\n" );
	    pw_setcmsname(pw, "X.V11");
	}
	pw_putcolormap(
	    pw, index, count, &rmap[index], &gmap[index], &bmap[index]);
    }
#endif SUN_WINDOWS

    if (ioctl(sunFbs[pScreen->myNum].fd, FBIOPUTCMAP, &sunCmap) < 0) {
	perror("sunCG4CUpdateColormap");
	FatalError( "sunCG4CUpdateColormap: FBIOPUTCMAP failed\n" );
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CSaveScreen --
 *	Preserve the color screen by turning on or off the video
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Video state is switched
 *
 *-----------------------------------------------------------------------
 */
static Bool
sunCG4CSaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    Bool    	  on;
{
    int		state = on;

    if (on != SCREEN_SAVER_ON) {
	SetTimeSinceLastInputEvent();
	state = 1;
    } else {
	state = 0;
    }
    (void) ioctl(sunFbs[pScreen->myNum].fd, FBIOSVIDEO, &state);
    return( TRUE );
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CCloseScreen --
 *	called to ensure video is enabled when server exits.
 *
 * Results:
 *	Screen is unsaved.
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
sunCG4CCloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    Bool    ret;

    pScreen->CloseScreen = (Bool (*)()) pScreen->devPrivates[sunCG4CScreenIndex].ptr;
    ret = (*pScreen->CloseScreen) (i, pScreen);
    sunCG4CInstalledMap = NULL;
    (void) (*pScreen->SaveScreen) (pScreen, SCREEN_SAVER_OFF);
    return ret;
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CInstallColormap --
 *	Install given colormap.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Existing map is uninstalled.
 *	All clients requesting ColormapNotify are notified
 *
 *-----------------------------------------------------------------------
 */
static void
sunCG4CInstallColormap(cmap)
    ColormapPtr	cmap;
{
    register int i;
    register Entry *pent;
    register VisualPtr pVisual = cmap->pVisual;
    u_char	  rmap[256], gmap[256], bmap[256];

    if (cmap == sunCG4CInstalledMap)
	return;
    if (sunCG4CInstalledMap)
	WalkTree(sunCG4CInstalledMap->pScreen, TellLostMap,
		 (pointer) &(sunCG4CInstalledMap->mid));
    if ((pVisual->class | DynamicClass) == DirectColor) {
	for (i = 0; i < 256; i++) {
	    pent = &cmap->red[(i & pVisual->redMask) >>
			      pVisual->offsetRed];
	    rmap[i] = pent->co.local.red >> 8;
	    pent = &cmap->green[(i & pVisual->greenMask) >>
				pVisual->offsetGreen];
	    gmap[i] = pent->co.local.green >> 8;
	    pent = &cmap->blue[(i & pVisual->blueMask) >>
			       pVisual->offsetBlue];
	    bmap[i] = pent->co.local.blue >> 8;
	}
    } else {
	for (i = 0, pent = cmap->red;
	     i < pVisual->ColormapEntries;
	     i++, pent++) {
	    if (pent->fShared) {
		rmap[i] = pent->co.shco.red->color >> 8;
		gmap[i] = pent->co.shco.green->color >> 8;
		bmap[i] = pent->co.shco.blue->color >> 8;
	    }
	    else {
		rmap[i] = pent->co.local.red >> 8;
		gmap[i] = pent->co.local.green >> 8;
		bmap[i] = pent->co.local.blue >> 8;
	    }
	}
    }
    sunCG4CInstalledMap = cmap;
    sunCG4CUpdateColormap(cmap->pScreen, 0, 256, rmap, gmap, bmap);
    WalkTree(cmap->pScreen, TellGainedMap, (pointer) &(cmap->mid));
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CUninstallColormap --
 *	Uninstall given colormap.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	default map is installed
 *	All clients requesting ColormapNotify are notified
 *
 *-----------------------------------------------------------------------
 */
static void
sunCG4CUninstallColormap(cmap)
    ColormapPtr	cmap;
{
    if (cmap == sunCG4CInstalledMap) {
	Colormap defMapID = cmap->pScreen->defColormap;

	if (cmap->mid != defMapID) {
	    ColormapPtr defMap = (ColormapPtr) LookupIDByType(defMapID,
							      RT_COLORMAP);

	    if (defMap)
		(*cmap->pScreen->InstallColormap)(defMap);
	    else
	        ErrorF("sunCG4C: Can't find default colormap\n");
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CListInstalledColormaps --
 *	Fills in the list with the IDs of the installed maps
 *
 * Results:
 *	Returns the number of IDs in the list
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
sunCG4CListInstalledColormaps(pScreen, pCmapList)
    ScreenPtr	pScreen;
    Colormap	*pCmapList;
{
    *pCmapList = sunCG4CInstalledMap->mid;
    return (1);
}


/*-
 *-----------------------------------------------------------------------
 * sunCG4CStoreColors --
 *	Sets the pixels in pdefs into the specified map.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
static void
sunCG4CStoreColors(pmap, ndef, pdefs)
    ColormapPtr	pmap;
    int		ndef;
    xColorItem	*pdefs;
{
    u_char	rmap[256], gmap[256], bmap[256];
    register int i;

    if (pmap != sunCG4CInstalledMap)
	return;
    while (ndef--) {
	i = pdefs->pixel;
	rmap[i] = pdefs->red >> 8;
	gmap[i] = pdefs->green >> 8;
	bmap[i] = pdefs->blue >> 8;
	sunCG4CUpdateColormap(pmap->pScreen, i, 1, rmap, gmap, bmap);
	pdefs++;
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CInit --
 *	Attempt to find and initialize a cg4 framebuffer used as mono
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Most of the elements of the ScreenRec are filled in. Memory is
 *	allocated for the frame buffer and the buffer is mapped. The
 *	video is enabled for the frame buffer...
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
sunCG4CInit (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    if (!cfbScreenInit (pScreen, (pointer)CG4Cfb->cpixel,
			sunFbs[index].info.fb_width,
			sunFbs[index].info.fb_height,
			monitorResolution, monitorResolution,
			sunFbs[index].info.fb_width))
    	return (FALSE);
    
    pScreen->SaveScreen = sunCG4CSaveScreen;
    
    pScreen->devPrivates[sunCG4CScreenIndex].ptr = (pointer) pScreen->CloseScreen;
    pScreen->CloseScreen = sunCG4CCloseScreen;
    
#ifndef STATIC_COLOR
    pScreen->InstallColormap = sunCG4CInstallColormap;
    pScreen->UninstallColormap = sunCG4CUninstallColormap;
    pScreen->ListInstalledColormaps = sunCG4CListInstalledColormaps;
    pScreen->StoreColors = sunCG4CStoreColors;
#endif
    
    sunCG4CSaveScreen( pScreen, SCREEN_SAVER_FORCER );
    return (sunScreenInit(pScreen) && cfbCreateDefColormap(pScreen));
}

/*-
 *--------------------------------------------------------------
 * sunCG4CSwitch --
 *      Enable or disable color plane 
 *
 * Results:
 *      Color plane enabled for select =0, disabled otherwise.
 *
 *--------------------------------------------------------------
 */
static void
sunCG4CSwitch (pScreen, select)
    ScreenPtr  pScreen;
    u_char     select;
{
    int index;
    register int    *j, *end;

    index = pScreen->myNum;
    CG4Cfb = (CG4CPtr) sunFbs[index].fb;

    j = (int *) CG4Cfb->epixel;
    end = j + (128 / sizeof (int)) * 1024;
    if (!select)                         
      while (j < end)
	*j++ = 0;
    else
      while (j < end)
	*j++ = ~0;
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CProbe --
 *	Attempt to find and initialize a cg4 framebuffer used as mono
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Memory is allocated for the frame buffer and the buffer is mapped.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
Bool
sunCG4CProbe (pScreenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int         fd;
    struct fbtype fbType;

    if ((fd = sunOpenFrameBuffer(FBTYPE_SUN4COLOR, &fbType, index, fbNum,
				 argc, argv)) < 0)
	return FALSE;

#ifdef	_MAP_NEW
    if ((int)(CG4Cfb = (CG4CPtr) mmap((caddr_t) 0,
	     CG4C_MONOLEN + CG4C_ENBLEN + CG4C_IMAGELEN,
	     PROT_READ | PROT_WRITE,
	     MAP_SHARED | _MAP_NEW, fd, 0)) == -1) {
	Error("Mapping cg4c");
	(void) close(fd);
	return FALSE;
    }
#else	_MAP_NEW
    CG4Cfb = (CG4CPtr) valloc(CG4C_MONOLEN + CG4C_ENBLEN + CG4C_IMAGELEN);
    if (CG4Cfb == (CG4CPtr) NULL) {
	ErrorF("Could not allocate room for frame buffer.\n");
	return FALSE;
    }

    if (mmap((caddr_t) CG4Cfb, CG4C_MONOLEN + CG4C_ENBLEN + CG4C_IMAGELEN,
	     PROT_READ | PROT_WRITE,
	     MAP_SHARED, fd, 0) < 0) {
	Error("Mapping cg4c");
	(void) close(fd);
	return FALSE;
    }
#endif	_MAP_NEW

    sunFbs[index].fd = fd;
    sunFbs[index].info = fbType;
    sunFbs[index].fb = (pointer) CG4Cfb;
    sunFbs[index].EnterLeave = sunCG4CSwitch;
    sunSupportsDepth8 = TRUE;
    return TRUE;
}

Bool
sunCG4CCreate(pScreenInfo, argc, argv)
    ScreenInfo	  *pScreenInfo;
    int	    	  argc;
    char    	  **argv;
{
    int i;

    if (sunGeneration != serverGeneration)
    {
	sunCG4CScreenIndex = AllocateScreenPrivateIndex();
	if (sunCG4CScreenIndex < 0)
	    return FALSE;
    }
    i = AddScreen(sunCG4CInit, argc, argv);
    if (i >= 0)
    {
	/* Now set the enable plane for screen 0 */
	sunCG4CSwitch(pScreenInfo->screens[i], i != 0);
	return TRUE;
    }
    return FALSE;
}
