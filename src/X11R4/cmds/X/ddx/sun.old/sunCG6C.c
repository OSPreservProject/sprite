/*-
 * sunCG6C.c --
 *	Functions to support the sun CG6 board as a memory frame buffer.
 */

/****************************************************************/
/* Modified from  sunCG4C.c for X11R3 by Tom Jarmolowski	*/
/****************************************************************/

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
static char sccsid[] = "@(#)sunCG6C.c	1.4 6/1/87 Copyright 1987 Sun Micro";
#endif

#include    "sun.h"

#ifdef FBTYPE_SUNFAST_COLOR
#include    <sys/mman.h>
#include    <pixrect/memreg.h>
#include    <sundev/cg6reg.h>
#include    "colormap.h"
#include    "colormapst.h"
#include    "resource.h"
#include    <struct.h>

#define	CG6_HEIGHT	900
#define	CG6_WIDTH	1152

typedef struct cg6c {
    u_char cpixel[CG6_HEIGHT][CG6_WIDTH];	/* byte-per-pixel memory */
} CG6, CG6Rec, *CG6Ptr;


#define CG6_IMAGE(fb)	    ((caddr_t)(&(fb)->cpixel))
#define CG6_IMAGEOFF	    ((off_t)0x0)
#define CG6_IMAGELEN	    (((CG6_HEIGHT*CG6_WIDTH + 8191)/8192)*8192)

static int  sunCG6CScreenIndex;

/* XXX - next line means only one CG6 - fix this */
static ColormapPtr sunCG6CInstalledMap;

extern int TellLostMap(), TellGainedMap();

static void
sunCG6CUpdateColormap(pScreen, index, count, rmap, gmap, bmap)
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
		FatalError( "sunCG6CUpdateColormap: pw_open failed\n" );
	    pw_setcmsname(pw, "X.V11");
	}
	pw_putcolormap(
	    pw, index, count, &rmap[index], &gmap[index], &bmap[index]);
    }
#endif SUN_WINDOWS

    if (ioctl(sunFbs[pScreen->myNum].fd, FBIOPUTCMAP, &sunCmap) < 0) {
	perror("sunCG6CUpdateColormap");
	FatalError( "sunCG6CUpdateColormap: FBIOPUTCMAP failed\n" );
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG6CSaveScreen --
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
sunCG6CSaveScreen (pScreen, on)
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
 * sunCG6CCloseScreen --
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
sunCG6CCloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    Bool    ret;

    pScreen->CloseScreen = (Bool (*)()) pScreen->devPrivates[sunCG6CScreenIndex].ptr;
    ret = (*pScreen->CloseScreen) (i, pScreen);
    sunCG6CInstalledMap = NULL;
    (void) (*pScreen->SaveScreen) (pScreen, SCREEN_SAVER_OFF);
    return ret;
}

/*-
 *-----------------------------------------------------------------------
 * sunCG6CInstallColormap --
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
sunCG6CInstallColormap(cmap)
    ColormapPtr	cmap;
{
    register int i;
    register Entry *pent;
    register VisualPtr pVisual = cmap->pVisual;
    u_char	  rmap[256], gmap[256], bmap[256];

    if (cmap == sunCG6CInstalledMap)
	return;
    if (sunCG6CInstalledMap)
	WalkTree(sunCG6CInstalledMap->pScreen, TellLostMap,
		 (pointer) &(sunCG6CInstalledMap->mid));
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
    sunCG6CInstalledMap = cmap;
    sunCG6CUpdateColormap(cmap->pScreen, 0, 256, rmap, gmap, bmap);
    WalkTree(cmap->pScreen, TellGainedMap, (pointer) &(cmap->mid));
}

/*-
 *-----------------------------------------------------------------------
 * sunCG6CUninstallColormap --
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
sunCG6CUninstallColormap(cmap)
    ColormapPtr	cmap;
{
    if (cmap == sunCG6CInstalledMap) {
	Colormap defMapID = cmap->pScreen->defColormap;

	if (cmap->mid != defMapID) {
	    ColormapPtr defMap = (ColormapPtr) LookupIDByType(defMapID,
							      RT_COLORMAP);

	    if (defMap)
		(*cmap->pScreen->InstallColormap)(defMap);
	    else
	        ErrorF("sunCG6C: Can't find default colormap\n");
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG6CListInstalledColormaps --
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
sunCG6CListInstalledColormaps(pScreen, pCmapList)
    ScreenPtr	pScreen;
    Colormap	*pCmapList;
{
    *pCmapList = sunCG6CInstalledMap->mid;
    return (1);
}


/*-
 *-----------------------------------------------------------------------
 * sunCG6CStoreColors --
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
sunCG6CStoreColors(pmap, ndef, pdefs)
    ColormapPtr	pmap;
    int		ndef;
    xColorItem	*pdefs;
{
    u_char	rmap[256], gmap[256], bmap[256];
    register int i;

    if (pmap != sunCG6CInstalledMap)
	return;
    while (ndef--) {
	i = pdefs->pixel;
	rmap[i] = pdefs->red >> 8;
	gmap[i] = pdefs->green >> 8;
	bmap[i] = pdefs->blue >> 8;
	sunCG6CUpdateColormap(pmap->pScreen, i, 1, rmap, gmap, bmap);
	pdefs++;
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG6Init --
 *	Attempt to find and initialize a cg6 framebuffer
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
sunCG6CInit (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    if (!cfbScreenInit (pScreen, sunFbs[index].fb,
			sunFbs[index].info.fb_width,
			sunFbs[index].info.fb_height,
			monitorResolution, monitorResolution,
			sunFbs[index].info.fb_width))
    	return (FALSE);

    pScreen->SaveScreen    =            sunCG6CSaveScreen;

    pScreen->devPrivates[sunCG6CScreenIndex].ptr = (pointer) pScreen->CloseScreen;
    pScreen->CloseScreen = sunCG6CCloseScreen;
    
#ifndef STATIC_COLOR
    pScreen->InstallColormap = sunCG6CInstallColormap;
    pScreen->UninstallColormap = sunCG6CUninstallColormap;
    pScreen->ListInstalledColormaps = sunCG6CListInstalledColormaps;
    pScreen->StoreColors = sunCG6CStoreColors;
#endif

    sunCG6CSaveScreen( pScreen, SCREEN_SAVER_FORCER );
    return sunScreenInit (pScreen) &&  cfbCreateDefColormap(pScreen);
}

/*-
 *--------------------------------------------------------------
 * sunCG6CSwitch --
 *      Enable or disable color plane 
 *
 * Results:
 *      Color plane enabled for select =0, disabled otherwise.
 *
 *--------------------------------------------------------------
 */
static void
/*ARGSUSED*/
sunCG6CSwitch (pScreen, select)
    ScreenPtr	pScreen;
    int		select;
{
}

/*-
 *-----------------------------------------------------------------------
 * sunCG6Probe --
 *	Attempt to find and initialize a cg6 framebuffer
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Memory is allocated for the frame buffer and the buffer is mapped.
 *
 *-----------------------------------------------------------------------
 */
Bool
sunCG6CProbe (pScreenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int         fd;
    struct fbtype fbType;
    int		pagemask, mapsize;
    int		imagelen;
    caddr_t	mapaddr;
    caddr_t	addr;

    if ((fd = sunOpenFrameBuffer(FBTYPE_SUNFAST_COLOR, &fbType, index, fbNum,
				 argc, argv)) < 0)
	return FALSE;

    pagemask = getpagesize() - 1;
    imagelen = fbType.fb_width * fbType.fb_height;
    mapsize = (imagelen + pagemask) & ~pagemask;
    addr = 0;

#ifndef	_MAP_NEW
    addr = (caddr_t) valloc(mapsize);
    if (addr == (caddr_t) NULL) {
	ErrorF("Could not allocate room for frame buffer.\n");
	(void) close (fd);
	return FALSE;
    }
#endif	_MAP_NEW

    mapaddr = (caddr_t) mmap((caddr_t) addr,
	     mapsize,
	     PROT_READ | PROT_WRITE,
	     MAP_SHARED, fd, CG6_VADDR_COLOR);


    if (mapaddr == (caddr_t) -1) {
	Error("Mapping cg6c");
	(void) close(fd);
	return FALSE;
    }

    if (mapaddr == 0)
        mapaddr = addr;

    sunFbs[index].fd = fd;
    sunFbs[index].info = fbType;
    sunFbs[index].fb = (pointer) mapaddr;
    sunFbs[index].EnterLeave = sunCG6CSwitch;
    sunSupportsDepth8 = TRUE;
    return TRUE;
}


Bool
sunCG6CCreate(pScreenInfo, argc, argv)
    ScreenInfo	  *pScreenInfo;
    int	    	  argc;
    char    	  **argv;
{
    int i;

    if (sunGeneration != serverGeneration)
    {
	sunCG6CScreenIndex = AllocateScreenPrivateIndex();
	if (sunCG6CScreenIndex < 0)
	    return FALSE;
    }
    i = AddScreen(sunCG6CInit, argc, argv);
    if (i >= 0)
	return TRUE;
    return FALSE;
}
#endif /* FBTYPE_SUNFAST_COLOR */
