/*-
 * spriteBW2.c --
 *	Functions for handling the sprite BWTWO board.
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
 *
 *
 */
#ifndef lint
static char rcsid[] =
	"$Header: /mic/X11R3/src/cmds/Xsp/ddx/sprite/RCS/spriteBW2.c,v 1.8 89/11/18 20:56:37 tve Exp $ SPRITE (Berkeley)";
#endif lint

#include    "spriteddx.h"
#include    "os.h"
#include    "resource.h"

#include    <kernel/devVid.h>
#include    <sys.h>
#include    <kernel/vmSunConst.h>

/*
 * From <sundev/bw2reg.h>
 */
#define BW2_FBSIZE	(128*1024)
#define BW2_WIDTH 	1152
#define BW2_HEIGHT	900

/*
 * Regular bw2 board.
 */
typedef struct bw2 {
    unsigned char	image[BW2_FBSIZE];          /* Pixel buffer */
} BW2, BW2Rec, *BW2Ptr;

/*-
 *-----------------------------------------------------------------------
 * spriteBW2SaveScreen --
 *	Disable the video on the frame buffer to save the screen.
 *
 * Results:
 *	TRUE.
 *
 * Side Effects:
 *	Video enable state changes.
 *
 *-----------------------------------------------------------------------
 */
static Bool
spriteBW2SaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    int    	  on;
{
    if (on != SCREEN_SAVER_ON) {
	SetTimeSinceLastInputEvent();
	screenSaved = FALSE;
	Sys_EnableDisplay(TRUE);
    } else {
	screenSaved = TRUE;
	Sys_EnableDisplay (FALSE);
    }

    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * spriteBW2CloseScreen --
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
spriteBW2CloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    return ((* pScreen->SaveScreen)(pScreen, SCREEN_SAVER_OFF));
}

/*-
 *-----------------------------------------------------------------------
 * spriteBW2ResolveColor --
 *	Resolve an RGB value into some sort of thing we can handle.
 *	Just looks to see if the intensity of the color is greater than
 *	1/2 and sets it to 'white' (all ones) if so and 'black' (all zeroes)
 *	if not.
 *
 * Results:
 *	*pred, *pgreen and *pblue are overwritten with the resolved color.
 *
 * Side Effects:
 *	see above.
 *
 *-----------------------------------------------------------------------
 */
static void
spriteBW2ResolveColor(pred, pgreen, pblue, pVisual)
    unsigned short	*pred;
    unsigned short	*pgreen;
    unsigned short	*pblue;
    VisualPtr		pVisual;
{
    *pred = *pgreen = *pblue = 
        (((39L * (int)*pred +
           50L * (int)*pgreen +
           11L * (int)*pblue) >> 8) >= (((1<<8)-1)*50)) ? ~0 : 0;
}

/*-
 *-----------------------------------------------------------------------
 * spriteBW2CreateColormap --
 *	create a bw colormap
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	allocate two pixels
 *
 *-----------------------------------------------------------------------
 */
void
spriteBW2CreateColormap(pmap)
    ColormapPtr	pmap;
{
    int	red, green, blue, pix;

    /* this is a monochrome colormap, it only has two entries, just fill
     * them in by hand.  If it were a more complex static map, it would be
     * worth writing a for loop or three to initialize it */

    /* this will be pixel 0 */
    pix = 0;
    red = green = blue = ~0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);

    /* this will be pixel 1 */
    red = green = blue = 0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);

}

/*-
 *-----------------------------------------------------------------------
 * spriteBW2DestroyColormap --
 *	destroy a bw colormap
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
void
spriteBW2DestroyColormap(pmap)
    ColormapPtr	pmap;
{
}

/*-
 *-----------------------------------------------------------------------
 * spriteBW2Init --
 *	Attempt to initialize a bw2 framebuffer
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Most of the elements of the ScreenRec are filled in.  The
 *	video is enabled for the frame buffer...
 *
 *-----------------------------------------------------------------------
 */
static Bool
spriteBW2Init (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    xColorItem	  def;
    ColormapPtr	  pColormap;
    
    
    if (!mfbScreenInit(index, pScreen, spriteFbs[index].fb,
		       BW2_WIDTH, BW2_HEIGHT, 90, 90)) {
			   return (FALSE);
    }

    pScreen->SaveScreen = spriteBW2SaveScreen;
    pScreen->ResolveColor = spriteBW2ResolveColor;
    pScreen->CreateColormap = spriteBW2CreateColormap;
    pScreen->DestroyColormap = spriteBW2DestroyColormap;
    pScreen->whitePixel = 0;
    pScreen->blackPixel = 1;

    /*
     * Play with the default colormap to reflect that suns use 1 for black
     * and 0 for white...
     */
    if ((CreateColormap(pScreen->defColormap, pScreen,
			LookupID(pScreen->rootVisual, RT_VISUALID, RC_CORE),
			&pColormap, AllocNone, 0) != Success) ||
	(pColormap == NULL)) {
	    FatalError("Can't create colormap in spriteBW2Init()\n");
    }
    mfbInstallColormap(pColormap);

    /*
     * Enable video output...? 
     */
    spriteBW2SaveScreen(pScreen, SCREEN_SAVER_OFF);

    spriteScreenInit(pScreen);
    return (TRUE);
}

/*-
 *-----------------------------------------------------------------------
 * spriteBW2Probe --
 *	Attempt to find and map a bw2 framebuffer
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Memory is allocated for the frame buffer and the buffer is mapped. 
 *
 *-----------------------------------------------------------------------
 */
Bool
spriteBW2Probe(screenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *screenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* The SUN frame buffer number */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    if(strlen(argv[0]) < 4 || strcmp("Xmfb", argv[0]+strlen(argv[0])-4) != 0)
	return FALSE;

    if (!spriteFbs[index].mapped) {
	pointer	  	pBigFb;
	Sys_MachineInfo	machType;
	Address	  	vidAddr;
	unsigned int	size;

	size = (sizeof(BW2Rec) + VMMACH_SEG_SIZE - 1) / VMMACH_SEG_SIZE;
	size *= VMMACH_SEG_SIZE;
	pBigFb = (pointer) malloc(size + VMMACH_SEG_SIZE);

	if (Sys_GetMachineInfo (sizeof (machType), &machType) != SUCCESS) {
	    return FALSE;
	}
	if (machType.architecture == SYS_SUN2) {
	   vidAddr = (Address)0xec0000;
	} else if (machType.architecture == SYS_SUN3) {
	    vidAddr = (Address)0xfe20000;
	} else if (machType.architecture == SYS_SUN4) {
	    if (machType.type == SYS_SUN_4_C) {
		vidAddr = (Address)0xffd80000;	/* sparc station */
	    } else {
		vidAddr = (Address)0xffd40000;	/* regular sun4 */
	    }
	} else {
	    return FALSE;
	}

	if (Vm_MapKernelIntoUser (vidAddr, size, pBigFb,
		&spriteFbs[index].fb) != SUCCESS) {
	      free ((char *) pBigFb);
	      return (FALSE);
	}
	spriteFbs[index].mapped = TRUE;
    }
    if (AddScreen (spriteBW2Init, argc, argv) > index) {
	screenInfo->screen[index].CloseScreen = spriteBW2CloseScreen;
	return TRUE;
    } else {
	return FALSE;
    }
}
