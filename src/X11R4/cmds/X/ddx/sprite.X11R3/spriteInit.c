/*-
 * spriteInit.c --
 *	Initialization functions for screen/keyboard/mouse, etc.
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
"$Header: /mic/X11R3/src/cmds/Xsp/ddx/sprite/RCS/spriteInit.c,v 1.8 89/11/18 20:57:30 tve Exp $ SPRITE (Berkeley)";
#endif lint

#include    "spriteddx.h"
#include    <servermd.h>
#include    "opaque.h"

extern void spriteMouseProc();
extern void spriteKbdProc();
extern Bool spriteBW2Probe();
extern Bool spriteCG4Probe();
extern Bool spriteCG6Probe();

extern GCPtr CreateScratchGC();

	/* What should this *really* be? */
#define MOTION_BUFFER_SIZE 0

/*
 * Data describing each type of frame buffer. The probeProc is called to
 * see if such a device exists and to do what needs doing if it does. devName
 * is the expected name of the device in the file system. Note that this only
 * allows one of each type of frame buffer. This may need changing later.
 */
static struct {
    Bool    (*probeProc)();
} spriteFbData[] = {
    spriteBW2Probe, spriteCG4Probe, spriteCG6Probe,
};

/*
 * NUMSCREENS is the number of supported frame buffers (i.e. the number of
 * structures in spriteFbData which have an actual probeProc).
 */
#define NUMSCREENS (sizeof(spriteFbData)/sizeof(spriteFbData[0]))
#define NUMDEVICES 2

fbFd	spriteFbs[NUMSCREENS];  /* Space for descriptors of open frame buffers */

static PixmapFormatRec	formats[] = {
    1, 1, BITMAP_SCANLINE_PAD,	/* 1-bit deep */
    8, 8, BITMAP_SCANLINE_PAD,	/* 8-bit deep */
};
#define NUMFORMATS	sizeof(formats)/sizeof(formats[0])

/*-
 *-----------------------------------------------------------------------
 * InitOutput --
 *	Initialize screenInfo for all actually accessible framebuffers.
 *	I kept this like the sun version just because you never know when
 *	support for multiple video devices might be added to Sprite...
 *
 * Results:
 *	screenInfo init proc field set
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */

InitOutput(screenInfo, argc, argv)
    ScreenInfo 	  *screenInfo;
    int     	  argc;
    char    	  **argv;
{
    int     	  i, index;

    screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    screenInfo->numPixmapFormats = NUMFORMATS;
    for (i=0; i< NUMFORMATS; i++)
    {
        screenInfo->formats[i] = formats[i];
    }

    for (i = 0, index = 0; i < NUMSCREENS; i++) {
	if ((*spriteFbData[i].probeProc) (screenInfo, index, i, argc, argv)) {
	    /* This display exists OK */
	    index++;
	} else {
	    /* This display can't be opened */
	    ;
	}
    }
    if (index == 0)
	FatalError("Can't find any displays\n");

    spriteInitCursor();
}

/*-
 *-----------------------------------------------------------------------
 * InitInput --
 *	Initialize all supported input devices...what else is there
 *	besides pointer and keyboard?
 *	NOTE: InitOutput must have already been called.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Two DeviceRec's are allocated and registered as the system pointer
 *	and keyboard devices.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
InitInput(argc, argv)
    int	    argc;
    char    **argv;
{
    DevicePtr p, k;
    static int  zero = 0;
    
    p = AddInputDevice(spriteMouseProc, TRUE);

    k = AddInputDevice(spriteKbdProc, TRUE);

    RegisterPointerDevice(p, MOTION_BUFFER_SIZE);
    RegisterKeyboardDevice(k);

    spriteCheckInput = 0;
    SetInputCheck (&zero, &spriteCheckInput);

    screenInfo.screen[0].blockData =
	screenInfo.screen[0].wakeupData = (pointer)k;
}

/*-
 *-----------------------------------------------------------------------
 * spriteQueryBestSize --
 *	Supposed to hint about good sizes for things.
 *
 * Results:
 *	Perhaps change *pwidth (Height irrelevant)
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
spriteQueryBestSize(class, pwidth, pheight)
    int      	  class;	/* Object class being queried */
    short   	  *pwidth;	/* Width of object */
    short   	  *pheight;	/* Height of object */
{
    unsigned int  width,
		  test;

    switch(class) {
      case CursorShape:
      case TileShape:
      case StippleShape:
	  width = *pwidth;
	  if (width > 0) {
	      /*
	       * Return the closes power of two not less than what they gave
	       * me
	       */
	      test = 0x80000000;
	      /*
	       * Find the highest 1 bit in the width given
	       */
	      while(!(test & width)) {
		 test >>= 1;
	      }
	      /*
	       * If their number is greater than that, bump up to the next
	       *  power of two
	       */
	      if((test - 1) & width) {
		 test <<= 1;
	      }
	      *pwidth = test;
	  }
	  /*
	   * We don't care what height they use
	   */
	  break;
    }
}

/*-
 *-----------------------------------------------------------------------
 * spriteScreenInit --
 *	Things which must be done for all types of frame buffers...
 *	Should be called last of all.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The graphics context for the screen is created. The CreateGC,
 *	CreateWindow and ChangeWindowAttributes vectors are changed in
 *	the screen structure.
 *
 *-----------------------------------------------------------------------
 */
void
spriteScreenInit (pScreen)
    ScreenPtr	  pScreen;
{
    fbFd    	  *fb;
    DrawablePtr	  pDrawable;
    BITS32  	  junk;

    fb = &spriteFbs[pScreen->myNum];

    /*
     * We need a GC for the cursor functions. We also don't want to
     * have to allocate one each time. Note that it is allocated before
     * CreateGC is intercepted so there are no extra indirections when
     * drawing the cursor...
     */
    fb->pGC = CreateScratchGC (pScreen, pScreen->rootDepth);
    fb->pGC->graphicsExposures = FALSE;
    
    /*
     * Preserve the "regular" functions
     */
    fb->CreateGC =	    	    	pScreen->CreateGC;
    fb->CreateWindow = 	    	    	pScreen->CreateWindow;
    fb->ChangeWindowAttributes =    	pScreen->ChangeWindowAttributes;
    fb->GetImage =	    	    	pScreen->GetImage;
    fb->GetSpans =	    	    	pScreen->GetSpans;

    /*
     * Interceptions
     */
    pScreen->CreateGC =	    	    	spriteCreateGC;
    pScreen->CreateWindow = 	    	spriteCreateWindow;
    pScreen->ChangeWindowAttributes = 	spriteChangeWindowAttributes;
    pScreen->QueryBestSize =	    	spriteQueryBestSize;
    pScreen->GetImage =	    	    	spriteGetImage;
    pScreen->GetSpans =	    	    	spriteGetSpans;

    /*
     * Cursor functions
     */
    pScreen->RealizeCursor = 	    	spriteRealizeCursor;
    pScreen->UnrealizeCursor =	    	spriteUnrealizeCursor;
    pScreen->DisplayCursor = 	    	spriteDisplayCursor;
    pScreen->SetCursorPosition =    	spriteSetCursorPosition;
    pScreen->CursorLimits = 	    	spriteCursorLimits;
    pScreen->PointerNonInterestBox = 	spritePointerNonInterestBox;
    pScreen->ConstrainCursor = 	    	spriteConstrainCursor;
    pScreen->RecolorCursor = 	    	spriteRecolorCursor;

    /*
     * Set pixel values for sun's view of the world...
     */
    pScreen->whitePixel = 0;
    pScreen->blackPixel = 1;

    /*
     *	Block/Unblock handlers
     */
    screenInfo.screen[0].BlockHandler = spriteBlockHandler;
    screenInfo.screen[0].WakeupHandler = spriteWakeupHandler;
}

