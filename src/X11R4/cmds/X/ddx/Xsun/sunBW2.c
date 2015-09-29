/*-
 * sunBW2.c --
 *	Functions for handling the sun BWTWO board.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 * Copyright (c) 1987 by Adam de Boor, UC Berkeley
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
static char sccsid[] = "%W %G Copyright 1987 Sun Micro";
#endif

/*-
 * Copyright (c) 1987 by Sun Microsystems,  Inc.
 */

#include    "sun.h"
#include    "resource.h"
#include "sys/fb.h"
#include "sys/ioctl.h"
#include "kernel/vmMach.h"
#include "sys/types.h"

extern caddr_t mmap();

#include    <sys/mman.h>
#ifdef sprite
#undef _MAP_NEW
#endif

#ifndef	sprite
#include    <sundev/bw2reg.h>


typedef struct bw2 {
    u_char	image[BW2_FBSIZE];          /* Pixel buffer */
} BW2, BW2Rec, *BW2Ptr;

typedef struct bw2hr {
    u_char	image[BW2_FBSIZE_HIRES];          /* Pixel buffer */
} BW2HR, BW2HRRec, *BW2HRPtr;
#endif	sprite

static int  sunBW2ScreenIndex;

/*-
 *-----------------------------------------------------------------------
 * sunBW2SaveScreen --
 *	Disable the video on the frame buffer to save the screen.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Video enable state changes.
 *
 *-----------------------------------------------------------------------
 */
static Bool
sunBW2SaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    Bool    	  on;
{
    int         state = on;

    if (on != SCREEN_SAVER_ON) {
	SetTimeSinceLastInputEvent();
	state = FBVIDEO_ON;
    } else {
	state = FBVIDEO_OFF;
    }
    (void) ioctl(sunFbs[pScreen->myNum].fd, FBIOSVIDEO, &state);
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * sunBW2CloseScreen --
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
sunBW2CloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    Bool    ret;

    pScreen->CloseScreen = (Bool (*)()) pScreen->devPrivates[sunBW2ScreenIndex].ptr;
    ret = (*pScreen->CloseScreen) (i, pScreen);
    (void) (*pScreen->SaveScreen) (pScreen, SCREEN_SAVER_OFF);
    return ret;
}

/*-
 *-----------------------------------------------------------------------
 * sunBW2Init --
 *	Attempt to find and initialize a bw2 framebuffer
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
/*ARGSUSED*/
static Bool
sunBW2Init (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    if (!mfbScreenInit(pScreen,
		       sunFbs[index].fb,
		       sunFbs[index].info.fb_width,
		       sunFbs[index].info.fb_height,
		       monitorResolution, monitorResolution,
		       sunFbs[index].info.fb_width))
	return (FALSE);

    pScreen->devPrivates[sunBW2ScreenIndex].ptr = (pointer) pScreen->CloseScreen;
    pScreen->CloseScreen = sunBW2CloseScreen;
    pScreen->SaveScreen = sunBW2SaveScreen;
    pScreen->whitePixel = 0;
    pScreen->blackPixel = 1;

    /*
     * Enable video output...? 
     */
    (void) sunBW2SaveScreen(pScreen, SCREEN_SAVER_FORCER);

    return (sunScreenInit(pScreen) && mfbCreateDefColormap(pScreen));
}

/*-
 *-----------------------------------------------------------------------
 * sunBW2Probe --
 *	Attempt to find and initialize a bw2 framebuffer
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Memory is allocated for the frame buffer and the buffer is mapped. 
 *
 *-----------------------------------------------------------------------
 */

/*ARGSUSED*/
Bool
sunBW2Probe(pScreenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
#ifdef	NOTDEF
	fbtype	fbt;
	fbaddr	fba;
	pointer	buffer;

	if(Fb_GTYPE(&fbt) != SUCCESS) {
		return FALSE;
	}
	switch(fbt.fb_type) {
	case FBTYPE_SUN2BW:
		if(Fb_MAP(&fba) != SUCCESS) {
			return FALSE;
		}
		buffer = fba.fb_buffer;
		break;
	case FBTYPE_SUN4COLOR:
		if(Fb_MAP(&fba) != SUCCESS) {
			return FALSE;
		}
		buffer = fba.fb_overlay;
		/* fudge */
		fbt.fb_type = FBTYPE_SUN2BW;
		fbt.fb_depth = 1;
		fbt.fb_cmsize = 2;
		fbt.fb_size = 128*1024;
		break;
	default:
		return FALSE;
	}
#endif NOTDEF

    int         fd;
    struct fbtype fbType;
    int		pagemask, mapsize;
    caddr_t	addr, mapaddr;
#ifdef sprite
    int		sizeToUse;
#endif /* sprite */

    if ((fd = sunOpenFrameBuffer(FBTYPE_SUN2BW, &fbType, index, fbNum,
				 argc, argv)) < 0)
	return FALSE;

    /*
     * It's not precisely clear that we have to round up
     * fb_size to the nearest page boundary but there are
     * rumors that this is a good idea and that it shouldn't
     * hurt anything.
     */
    pagemask = getpagesize() - 1;
    mapsize = (fbType.fb_size + pagemask) & ~pagemask;
    addr = 0;

#ifndef _MAP_NEW
    /*
     * If we are running pre-SunOS 4.0 then we first need to
     * allocate some address range for mmap() to replace.
     */
#ifdef sprite
    sizeToUse = ((mapsize + VMMACH_SEG_SIZE) & ~(VMMACH_SEG_SIZE-1))
	    + VMMACH_SEG_SIZE;
    addr = (caddr_t) malloc(sizeToUse);
    if (addr == NULL) {
#else
    if ((addr = (caddr_t) valloc(mapsize)) == 0) {
#endif /* sprite */
        ErrorF("Could not allocate room for frame buffer.\n");
        (void) close(fd);
        return FALSE;
    }
#endif _MAP_NEW

#ifdef sprite
    addr = (caddr_t) mmap((caddr_t) addr, mapsize,
	    PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0);
    if (addr == (caddr_t) NULL) {
#else
    /*
     * In SunOS 4.0 the standard C library mmap() system call
     * wrapper will automatically add a _MAP_NEW flag for us.
     * In pre-4.0 mmap(), success returned 0 but now it returns the
     * newly mapped starting address. The test for mapaddr
     * being 0 below will handle this difference correctly.
     */
    if ((mapaddr = (caddr_t) mmap(addr, mapsize,
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0)) == (caddr_t) -1) {
#endif /* sprite */
        Error("mapping BW2");
        (void) close(fd);
        return FALSE;
    }

#ifndef sprite
    if (mapaddr == 0)
        mapaddr = addr;
#endif /* sprite */

#ifdef sprite
    sunFbs[index].fb = (pointer) addr;
    /*
     * XXX What do we do here about fudging the FBTYPE_SUN4COLOR to be
     * black and white (as in the notdef'ed section above).?
     */
#else
    sunFbs[index].fb = buffer;
#endif /* sprite */
    sunFbs[index].info = fbType;
    sunFbs[index].EnterLeave = NULL;
    sunFbs[index].fd = fd;
    return TRUE;
}

Bool
sunBW2Create(pScreenInfo, argc, argv)
    ScreenInfo	  *pScreenInfo;
    int	    	  argc;
    char    	  **argv;
{
    if (sunGeneration != serverGeneration)
    {
	sunBW2ScreenIndex = AllocateScreenPrivateIndex();
	if (sunBW2ScreenIndex < 0)
	    return FALSE;
    }
    return (AddScreen(sunBW2Init, argc, argv) >= 0);
}
