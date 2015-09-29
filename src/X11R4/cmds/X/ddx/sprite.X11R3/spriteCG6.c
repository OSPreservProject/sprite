/*-
 * spriteCG6C.c --
 *	Functions to support the sun CG6 board as a memory frame buffer.
 */

/************************************************************
 * Copyright (c) 1989 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 ********************************************************/

#ifndef	lint
static char rcsid[] =
	"";
#endif

#include    "spriteddx.h"
#include    "os.h"
#include    "resource.h"

#include    <sys.h>
#include    <kernel/vmSunConst.h>

#include    "colormap.h"
#include    "colormapst.h"

#define	CG6_HEIGHT	900
#define	CG6_WIDTH	1152

/* For the SPARCstation 1 */
#define CG6_FB		0xffd80000
#define	CG6_CMAP	0xffd1f000
#define	CG6_FHC		0xffd1c000
#define	CG6_THC		0xffd1d000

/*
 * Colormap stuff
 */
static ColormapPtr spriteCG6InstalledMap;

/*
 * Brooktree DAC
 * Only topmost byte is active (but must be written as longword ...)
 */
typedef volatile struct {
	unsigned int	addr;		/* colormap address register */
	unsigned int	cmap;		/* colormap data register */
	unsigned int	ctrl;		/* control register */
	unsigned int	omap;		/* overlay map data register */
} CG6Cmap;

extern int TellLostMap(), TellGainedMap();

#define	SET_CREG(reg, val)	((reg) = ((val)<<24))
#define SET_CMAP(i, r, g, b)	SET_CREG(cMap->addr, i);	\
				SET_CREG(cMap->cmap, r);	\
				SET_CREG(cMap->cmap, g);	\
				SET_CREG(cMap->cmap, b);
#define	SET_CTRL(reg, val)	SET_CREG(cMap->addr, reg);	\
				SET_CREG(cMap->ctrl, val);
#define SET_OVER(i, r, g, b)	SET_CREG(cMap->addr, i);	\
				SET_CREG(cMap->omap, r);	\
				SET_CREG(cMap->omap, g);	\
				SET_CREG(cMap->omap, b);
static void
spriteCG6UpdateColormap(pScreen, index, count, rmap, gmap, bmap)
    ScreenPtr		pScreen;
    int			index, count;
    unsigned char	*rmap, *gmap, *bmap;
{
    volatile CG6Cmap	*cMap = (CG6Cmap *)spriteFbs[pScreen->myNum].cmap;

    /* update the memory copy */
    rmap+=index; gmap+=index; bmap+=index;
    SET_CREG(cMap->addr, index);
    while(count--) {
	SET_CREG(cMap->cmap, *rmap++);
	SET_CREG(cMap->cmap, *gmap++);
	SET_CREG(cMap->cmap, *bmap++);
    }
}

static void
spriteCG6InitColormap(pScreen)
    ScreenPtr		pScreen;
{
    volatile CG6Cmap	*cMap = (CG6Cmap *)spriteFbs[pScreen->myNum].cmap;

    SET_CTRL(4, 0xff);	/* read mask */
    SET_CTRL(5, 0x00);	/* blink mask */
    SET_CTRL(6, 0x70);	/* command reg */
    SET_CTRL(7, 0x00);	/* test reg */
    SET_OVER(0, 0xff, 0xff, 0x00);	/* overlay colors */
    SET_OVER(1, 0x00, 0xff, 0xff);
    SET_OVER(2, 0xff, 0x00, 0xff);
    SET_OVER(3, 0xff, 0x00, 0x00);
	/* command reg:
	 *	7	4:1 multiplexing
	 *	6	use color palette RAM
	 *	5:4	blink divisor (11 = 65536)
	 *	3:2	blink enable
	 *	1:0	overlay enable
	 */
}

/*-
 *-----------------------------------------------------------------------
 * spriteCG6SaveScreen --
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
spriteCG6SaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    Bool    	  on;
{
    volatile CG6Cmap	*cMap = (CG6Cmap *)spriteFbs[pScreen->myNum].cmap;

    if (on != SCREEN_SAVER_ON) {
	SetTimeSinceLastInputEvent();
	screenSaved = FALSE;
	if(spriteCG6InstalledMap) {
	    Entry *pent = spriteCG6InstalledMap->red;
	    if(pent->fShared) {
	    	SET_CMAP(0, pent->co.shco.red->color >> 8,
	    		    pent->co.shco.green->color >> 8,
			    pent->co.shco.blue->color >> 8);
	    } else {
		SET_CMAP(0, pent->co.local.red >> 8,
			    pent->co.local.red >> 8,
			    pent->co.local.red >> 8);
	    }
	} else {
	    SET_CMAP(0, 0, 0, 0xff);
	}
	SET_CTRL(4, 0xff);
    } else {
	screenSaved = TRUE;
	SET_CMAP(0, 0, 0, 0);
	SET_CTRL(4, 0x00);
    }
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * spriteCG6CloseScreen --
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
spriteCG6CloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    volatile CG6Cmap	*cMap = (CG6Cmap *)spriteFbs[pScreen->myNum].cmap;

    spriteCG6InstalledMap = NULL;
    SET_CMAP(0x00, 0x00, 0x00, 0x00);
    SET_CMAP(0xff, 0xff, 0xff, 0xff);
    return ((* pScreen->SaveScreen)(pScreen, SCREEN_SAVER_OFF));
}

/*-
 *-----------------------------------------------------------------------
 * spriteCG6InstallColormap --
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
spriteCG6InstallColormap(cmap)
    ColormapPtr	cmap;
{
    register int	i;
    register Entry	*pent = cmap->red;
    unsigned char	rmap[256], gmap[256], bmap[256];

    if(cmap == spriteCG6InstalledMap)
	return;
    if(spriteCG6InstalledMap)
	WalkTree(spriteCG6InstalledMap->pScreen, TellLostMap,
		 (char *) &(spriteCG6InstalledMap->mid));
    for(i=0; i<cmap->pVisual->ColormapEntries; i++) {
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
	pent++;
    }
    spriteCG6InstalledMap = cmap;
    spriteCG6UpdateColormap(cmap->pScreen, 0, 256, rmap, gmap, bmap);
    WalkTree(cmap->pScreen, TellGainedMap, (char *) &(cmap->mid));
}

/*-
 *-----------------------------------------------------------------------
 * spriteCG6UninstallColormap --
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
spriteCG6UninstallColormap(cmap)
    ColormapPtr	cmap;
{
    if(cmap == spriteCG6InstalledMap) {
	Colormap defMapID = cmap->pScreen->defColormap;

	if (cmap->mid != defMapID) {
	    ColormapPtr defMap =
		(ColormapPtr)LookupID(defMapID, RT_COLORMAP, RC_CORE);

	    if (defMap)
		spriteCG6InstallColormap(defMap);
	    else
	        ErrorF("spriteCG6: Can't find default colormap\n");
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteCG6ListInstalledColormaps --
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
spriteCG6ListInstalledColormaps(pScreen, pCmapList)
    ScreenPtr	pScreen;
    Colormap	*pCmapList;
{
    *pCmapList = spriteCG6InstalledMap->mid;
    return (1);
}


/*-
 *-----------------------------------------------------------------------
 * spriteCG6StoreColors --
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
spriteCG6StoreColors(pmap, ndef, pdefs)
    ColormapPtr	pmap;
    int		ndef;
    xColorItem	*pdefs;
{
    switch(pmap->class) {
    case PseudoColor:
	if(pmap == spriteCG6InstalledMap) {
	    /* We only have a single colormap */
	    unsigned char	rmap[256], gmap[256], bmap[256];
	    int			index;

	    while (ndef--) {
		index = pdefs->pixel&0xff;
		rmap[index] = (pdefs->red) >> 8;
		gmap[index] = (pdefs->green) >> 8;
		bmap[index] = (pdefs->blue) >> 8;
	 	spriteCG6UpdateColormap(pmap->pScreen,
				      index, 1, rmap, gmap, bmap);
		pdefs++;
	    }
	}
	break;
    case DirectColor:
    default:
	ErrorF("spriteCG6StoreColors: bad class %d\n", pmap->class);
	break;
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteCG6ResolvePseudoColor --
 *	Adjust specified RGB values to closest values hardware can do.
 *
 * Results:
 *	Args are modified.
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
spriteCG6ResolvePseudoColor(pRed, pGreen, pBlue, pVisual)
    CARD16	*pRed, *pGreen, *pBlue;
    VisualPtr	pVisual;
{
    *pRed &= 0xff00;
    *pGreen &= 0xff00;
    *pBlue &= 0xff00;
}

/*-
 *-----------------------------------------------------------------------
 * spriteCG6Init --
 *	Attempt to find and initialize a cg6 framebuffer used as mono
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
spriteCG6Init (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    CARD16	zero = 0, ones = ~0;

    if (!cfbScreenInit (index, pScreen, spriteFbs[index].fb,
			    CG6_WIDTH, CG6_HEIGHT, 100))
	return (FALSE);

    pScreen->SaveScreen    =            spriteCG6SaveScreen;
    pScreen->RecolorCursor = 	    	spriteRecolorCursor;

#ifndef STATIC_COLOR
    pScreen->InstallColormap = spriteCG6InstallColormap;
    pScreen->UninstallColormap = spriteCG6UninstallColormap;
    pScreen->ListInstalledColormaps = spriteCG6ListInstalledColormaps;
    pScreen->StoreColors = spriteCG6StoreColors;
    pScreen->ResolveColor = spriteCG6ResolvePseudoColor;
#endif

    spriteCG6InitColormap(pScreen);
    {
	ColormapPtr cmap = (ColormapPtr)LookupID(pScreen->defColormap,
		RT_COLORMAP, RC_CORE);

	if (!cmap)
	    FatalError("Can't find default colormap\n");
	if (AllocColor(cmap, &ones, &ones, &ones, &(pScreen->whitePixel), 0)
	    || AllocColor(cmap, &zero, &zero, &zero, &(pScreen->blackPixel), 0))
		FatalError("Can't alloc black & white pixels in cfbScreeninit\n");
	spriteCG6InstallColormap(cmap);
    }

    spriteCG6SaveScreen( pScreen, SCREEN_SAVER_OFF );
    spriteScreenInit (pScreen);

    return (TRUE);
}

/*-
 *--------------------------------------------------------------
 * spriteCG6Switch --
 *      Enable or disable color plane 
 *
 * Results:
 *      Color plane enabled for select =0, disabled otherwise.
 *
 *--------------------------------------------------------------
 */
static void
spriteCG6Switch ()
{
}

/*-
 *-----------------------------------------------------------------------
 * spriteCG6Probe --
 *	Attempt to find and initialize a cg6 framebuffer used as mono
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
spriteCG6Probe(screenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *screenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    Sys_MachineInfo	machType;
    pointer	  	pFb, pCm;	/* preallocated VM */
    Address	  	vFb, vCm;	/* kernel virtual addresses to poke */
    unsigned int	bCm, oCm;	/* base&offset for segment for cmap */
    unsigned int	sFb;		/* how much mem to alloc */

    if(strlen(argv[0]) < 4 || strcmp("Xcg6", argv[0]+strlen(argv[0])-4) != 0)
	return FALSE;

    if(!spriteFbs[index].mapped) {
	if(Sys_GetMachineInfo(sizeof(machType), &machType) != SUCCESS) {
	    return FALSE;
	}
	if(machType.architecture == SYS_SUN4) {
	    if(machType.type == SYS_SUN_4_C) {
		vFb = (Address)CG6_FB;		/* sparc station */
		vCm = (Address)CG6_CMAP;
	    }
	} else {
	    return FALSE;
	}

	sFb = (CG6_HEIGHT*CG6_WIDTH + VMMACH_SEG_SIZE - 1) / VMMACH_SEG_SIZE;
	sFb *= VMMACH_SEG_SIZE;
	pFb = (pointer)malloc(sFb + VMMACH_SEG_SIZE);
	pCm = (pointer)malloc(2*VMMACH_SEG_SIZE);
	oCm = (unsigned int)vCm -
		((unsigned int)vCm)/VMMACH_SEG_SIZE*VMMACH_SEG_SIZE;
	vCm = (Address)(((unsigned int)vCm)/VMMACH_SEG_SIZE*VMMACH_SEG_SIZE);

	if(Vm_MapKernelIntoUser(vFb, sFb, pFb, &spriteFbs[index].fb) != SUCCESS)
	      { perror("VmMap"); return (FALSE); }
	if(Vm_MapKernelIntoUser(vCm, VMMACH_SEG_SIZE, pCm, &bCm) != SUCCESS)
	      return (FALSE);
	spriteFbs[index].cmap = (pointer)(bCm+oCm);
	spriteFbs[index].mapped = TRUE;
    }
    if(AddScreen(spriteCG6Init, argc, argv) > index) {
	screenInfo->screen[index].CloseScreen = spriteCG6CloseScreen;
	return TRUE;
    } else {
	return FALSE;
    }
}
