/*-
 * sunCG3C.c --
 *	Functions to support the sun CG3 board as a memory frame buffer.
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
static char sccsid[] = "@(#)sunCG3C.c	1.1 11/3/87 Copyright 1987 Sun Micro";
#endif

#include    "sun.h"


#include    <sys/mman.h>
#ifdef sprite
#undef _MAP_NEW
#endif
#include    <kernel/vmMach.h>
#ifndef	sprite
#include    <pixrect/memreg.h>
/*
#include    <sundev/cg4reg.h>
*/
#include    <struct.h>
#endif	sprite

#include <sys/types.h>
#include    "colormap.h"
#include    "colormapst.h"
#include    "resource.h"

#include "sys/ioctl.h"

#include "sys/fb.h"
#undef MAP_NEW

struct fbtype fbType;

/*-
 * The cg3 frame buffer is divided into several pieces.
 *	1) an array of 8-bit pixels
 *	2) a one-bit deep overlay plane
 *	3) an enable plane
 *	4) a colormap and status register
 *
 * XXX - put the cursor in the overlay plane
 */

#define CG3A_HEIGHT      900 
#define CG3A_WIDTH       1152
#define CG3B_HEIGHT	 768
#define CG3B_WIDTH	 1024

typedef struct cg3ac {
#ifdef sparc
	u_char mpixel[128*1024];		/* bit-per-pixel memory */
	u_char epixel[128*1024];		/* enable plane */
#endif
        u_char cpixel[CG3A_HEIGHT][CG3A_WIDTH];   /* byte-per-pixel memory */
} CG3AC, CG3ACRec, *CG3ACPtr;

typedef struct cg3bc {
#ifdef sparc
	u_char mpixel[128*1024];		/* bit-per-pixel memory */
	u_char epixel[128*1024];		/* enable plane */
#endif
        u_char cpixel[CG3B_HEIGHT][CG3B_WIDTH];   /* byte-per-pixel memory */
} CG3BC, CG3BCRec, *CG3BCPtr;

#define CG3AC_IMAGE(fb)      ((caddr_t)(&(fb)->cpixel))
#define CG3AC_IMAGELEN       (((sizeof (CG3AC) + 4095)/4096)*4096)
#define CG3BC_IMAGE(fb)      ((caddr_t)(&(fb)->cpixel))
#define CG3BC_IMAGELEN       (((sizeof (CG3BC) + 4095)/4096)*4096)

static CG3ACPtr CG3ACfb = NULL;
static CG3BCPtr CG3BCfb = NULL;


static int  sunCG3CScreenIndex;

/* XXX - next line means only one CG3 - fix this */
static ColormapPtr sunCG3CInstalledMap;

extern int TellLostMap(), TellGainedMap();

/*ARGSUSED*/
static void
sunCG3CUpdateColormap(pScreen, index, count, rmap, gmap, bmap)
    ScreenPtr	pScreen;
    int		index, count;
    u_char	*rmap, *gmap, *bmap;
{
    fbcmap sunCmap;

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
		FatalError( "sunCG3CUpdateColormap: pw_open failed\n" );
	    pw_setcmsname(pw, "X.V11");
	}
	pw_putcolormap(
	    pw, index, count, &rmap[index], &gmap[index], &bmap[index]);
    }
#endif SUN_WINDOWS

    if (ioctl(sunFbs[pScreen->myNum].fd, FBIOPUTCMAP, &sunCmap) < 0) {
	perror("sunCG3CUpdateColormap");
	FatalError( "sunCG3CUpdateColormap: FBIOPUTCMAP failed\n" );
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG3CSaveScreen --
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
/*ARGSUSED*/
static Bool
sunCG3CSaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    Bool    	  on;
{
    int		state = on;

    switch (on) {
    case SCREEN_SAVER_FORCER:
	SetTimeSinceLastInputEvent();
	state = 1;
	break;
    case SCREEN_SAVER_OFF:
	state = 1;
	break;
    case SCREEN_SAVER_ON:
    default:
	state = 0;
	break;
    }
    (void) ioctl(sunFbs[pScreen->myNum].fd, FBIOSVIDEO, &state);

    return( TRUE );
}

/*-
 *-----------------------------------------------------------------------
 * sunCG3CCloseScreen --
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
sunCG3CCloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    u_char rmap[256], gmap[256], bmap[256];
    Bool ret;

    pScreen->CloseScreen = (Bool (*)()) pScreen->devPrivates[sunCG3CScreenIndex].ptr;
    ret = (*pScreen->CloseScreen) (i, pScreen);

    /* the following 2 lines are to fix rr clear_colormap bug */
    rmap[255] = gmap[255] = bmap[255] = 0;
    sunCG3CUpdateColormap(pScreen, 255, 1, rmap, gmap, bmap);

    sunCG3CInstalledMap = NULL;
    (void)(*pScreen->SaveScreen)(pScreen, SCREEN_SAVER_OFF);
    return ret;
}

/*-
 *-----------------------------------------------------------------------
 * sunCG3CInstallColormap --
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
/*ARGSUSED*/
static void
sunCG3CInstallColormap(cmap)
    ColormapPtr	cmap;
{
    register int i;
    register Entry *pent;
    register VisualPtr pVisual = cmap->pVisual;
    u_char	  rmap[256], gmap[256], bmap[256];

    if (cmap == sunCG3CInstalledMap)
	return;
    if (sunCG3CInstalledMap)
	WalkTree(sunCG3CInstalledMap->pScreen, TellLostMap,
		 (char *) &(sunCG3CInstalledMap->mid));
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
    sunCG3CInstalledMap = cmap;
    sunCG3CUpdateColormap(cmap->pScreen, 0, 256, rmap, gmap, bmap);
    WalkTree(cmap->pScreen, TellGainedMap, (char *) &(cmap->mid));
}

/*-
 *-----------------------------------------------------------------------
 * sunCG3CUninstallColormap --
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
/*ARGSUSED*/
static void
sunCG3CUninstallColormap(cmap)
    ColormapPtr	cmap;
{
    if (cmap == sunCG3CInstalledMap) {
	Colormap defMapID = cmap->pScreen->defColormap;

	if (cmap->mid != defMapID) {
	    ColormapPtr defMap = (ColormapPtr) LookupIDByType(defMapID,
							      RT_COLORMAP);

	    if (defMap)
		(*cmap->pScreen->InstallColormap)(defMap);
	    else
	        ErrorF("sunCG3C: Can't find default colormap\n");
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG3CListInstalledColormaps --
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
sunCG3CListInstalledColormaps(pScreen, pCmapList)
    ScreenPtr	pScreen;
    Colormap	*pCmapList;
{
    *pCmapList = sunCG3CInstalledMap->mid;
    return (1);
}


/*-
 *-----------------------------------------------------------------------
 * sunCG3CStoreColors --
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
/*ARGSUSED*/
static void
sunCG3CStoreColors(pmap, ndef, pdefs)
    ColormapPtr	pmap;
    int		ndef;
    xColorItem	*pdefs;
{
    u_char	rmap[256], gmap[256], bmap[256];
    register int i;
    register Entry *pent;

    if (pmap != sunCG3CInstalledMap)
	return;
    while (ndef--) {
	i = pdefs->pixel;
	rmap[i] = pdefs->red >> 8;
	gmap[i] = pdefs->green >> 8;
	bmap[i] = pdefs->blue >> 8;
	sunCG3CUpdateColormap(pmap->pScreen, i, 1, rmap, gmap, bmap);
	pdefs++;
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG3CInit --
 *	Attempt to find and initialize a cg3 framebuffer
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
sunCG3CInit (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    if (!cfbScreenInit (pScreen,
#ifndef sprite
			(sunFbs[index].info.fb_width == CG3A_WIDTH) ?
			(int *) CG3ACfb->cpixel : (int *) CG3BCfb->cpixel,
#else
			sunFbs[index].fb,
#endif
			sunFbs[index].info.fb_width,
			sunFbs[index].info.fb_height,
			monitorResolution, monitorResolution,
			sunFbs[index].info.fb_width))
	return (FALSE);

    pScreen->SaveScreen = sunCG3CSaveScreen;
    pScreen->devPrivates[sunCG3CScreenIndex].ptr = (pointer) pScreen->CloseScreen;
    pScreen->CloseScreen = sunCG3CCloseScreen;

#ifndef STATIC_COLOR
    pScreen->InstallColormap = sunCG3CInstallColormap;
    pScreen->UninstallColormap = sunCG3CUninstallColormap;
    pScreen->ListInstalledColormaps = sunCG3CListInstalledColormaps;
    pScreen->StoreColors = sunCG3CStoreColors;
#endif

    sunCG3CSaveScreen( pScreen, SCREEN_SAVER_FORCER );
    return (sunScreenInit(pScreen) && cfbCreateDefColormap(pScreen));
}

/*-
 *-----------------------------------------------------------------------
 * sunCG3CProbe --
 *	Attempt to find and initialize a cg3 framebuffer
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
sunCG3CProbe (pScreenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int         fd;
    struct fbtype fbType;


    printf("sunCG3CProbe: fbNum = %d , index = %d, argc= %d\n",
	fbNum, index, argc);

    if ((fd = sunOpenFrameBuffer(FBTYPE_SUN3COLOR, &fbType, index, fbNum,
				 argc, argv)) < 0)
	return FALSE;

#ifdef	_MAP_NEW
    if (fbType.fb_width == CG3A_WIDTH) {
	if ((int)(CG3ACfb = (CG3ACPtr) mmap((caddr_t) 0,
	     CG3AC_IMAGELEN,
	     PROT_READ | PROT_WRITE,
	     MAP_SHARED | _MAP_NEW, fd, 0)) == -1) {
	    Error("Mapping cg3c");
	    (void) close(fd);
	    return FALSE;
	}
    }
    else if (fbType.fb_width == CG3B_WIDTH) {
	if ((int)(CG3BCfb = (CG3BCPtr) mmap((caddr_t) 0,
	     CG3BC_IMAGELEN,
	     PROT_READ | PROT_WRITE,
	     MAP_SHARED | _MAP_NEW, fd, 0)) == -1) {
	    Error("Mapping cg3c");
	    (void) close(fd);
	    return FALSE;
	}
    }
    else {
	    Error("Mapping cg3c");
	    (void) close(fd);
	    return FALSE;
    }
#else	_MAP_NEW
    if (fbType.fb_width == CG3A_WIDTH) {
#ifdef sprite
	int	sizeToUse;

	sizeToUse = ((fbType.fb_size + VMMACH_SEG_SIZE) & ~(VMMACH_SEG_SIZE-1))
		+ VMMACH_SEG_SIZE;
	CG3ACfb = (CG3ACPtr) malloc(sizeToUse);
	printf("original CG3ACfb addr: 0x%x, original size: 0x%x\n",
		CG3ACfb, sizeToUse);
#else
	CG3ACfb = (CG3ACPtr) valloc(CG3AC_MONOLEN + 
	    CG3AC_ENBLEN + CG3AC_IMAGELEN);
#endif /* sprite */
	if (CG3ACfb == (CG3ACPtr) NULL) {
	    ErrorF("Could not allocate room for frame buffer.\n");
	    return FALSE;
	}

#ifdef sprite
	CG3ACfb = (CG3ACPtr) mmap((caddr_t) CG3ACfb, fbType.fb_size,
	    PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0);
	if (CG3ACfb == (CG3ACPtr) NULL) {
#else
	if (mmap((caddr_t) CG3ACfb, CG3AC_MONOLEN + 
	    CG3AC_ENBLEN + CG3AC_IMAGELEN,
	    PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0) < 0) {
#endif /* sprite */
	    Error("Mapping cg3c");
	    (void) close(fd);
	    return FALSE;
	} else {
	    printf("new addr for CG3ACfb: 0x%x\n", CG3ACfb);
	}
    }
    else if (fbType.fb_width == CG3B_WIDTH) {
#ifdef sprite
	int	sizeToUse;

	sizeToUse = ((fbType.fb_size + VMMACH_SEG_SIZE) & ~(VMMACH_SEG_SIZE-1))
		+ VMMACH_SEG_SIZE;
	CG3BCfb = (CG3BCPtr) malloc(sizeToUse);
	printf("original CG3BCfb addr: 0x%x, original size: 0x%x\n",
		CG3BCfb, sizeToUse);
#else
	CG3BCfb = (CG3BCPtr) valloc(CG3BC_MONOLEN + 
	    CG3BC_ENBLEN + CG3BC_IMAGELEN);
#endif /* sprite */
	if (CG3BCfb == (CG3BCPtr) NULL) {
	    ErrorF("Could not allocate room for frame buffer.\n");
	    return FALSE;
	}
#ifdef sprite
	CG3BCfb = (CG3BCPtr) mmap((caddr_t) CG3BCfb, fbType.fb_size,
	    PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0);
	if (CG3BCfb == (CG3BCPtr) NULL) {
#else
	if (mmap((caddr_t) CG3BCfb, CG3BC_MONOLEN + 
	    CG3BC_ENBLEN + CG3BC_IMAGELEN,
	    PROT_READ | PROT_WRITE,
	    MAP_SHARED, fd, 0) < 0) {
#endif /* sprite */
	    Error("Mapping cg3c");
	    (void) close(fd);
	    return FALSE;
	} else {
	    printf("new addr for CG3BCfb: 0x%x\n", CG3BCfb);
	}
    }
    else {
	    Error("Mapping cg3c");
	    (void) close(fd);
	    return FALSE;
    }
#endif	_MAP_NEW

    sunFbs[index].info = fbType;
/*  sunFbs[index].EnterLeave = sunCG3CSwitch;	*/
#ifndef sprite
    sunFbs[index].fb = (pointer) fb_Addr.fb_buffer;
#else
    if (fbType.fb_width == CG3A_WIDTH) {
	sunFbs[index].fb = (pointer) CG3ACfb;
    } else if (fbType.fb_width == CG3B_WIDTH) {
	sunFbs[index].fb = (pointer) CG3BCfb;
    }
#endif /*sprite*/
    sunFbs[index].fd = fd;
    sunSupportsDepth8 = TRUE;
    return TRUE;
}

/*ARGSUSED*/
Bool
sunCG3CCreate(pScreenInfo, argc, argv)
    ScreenInfo	  *pScreenInfo;
    int	    	  argc;
    char    	  **argv;
{
    if (sunGeneration != serverGeneration)
    {
	sunCG3CScreenIndex = AllocateScreenPrivateIndex();
	if (sunCG3CScreenIndex < 0)
	    return FALSE;
    }
    return (AddScreen(sunCG3CInit, argc, argv) >= 0);
}
