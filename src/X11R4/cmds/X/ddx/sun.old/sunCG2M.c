/*-
 * sunCG2M.c --
 *	Functions to support the sun CG2 board when treated as a monochrome
 *	frame buffer.
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
#ifndef lint
static char rcsid[] =
	"$XConsortium: sunCG2M.c,v 4.18 89/07/22 15:17:33 rws Exp $ SPRITE (Berkeley)";
#endif lint

#include    "sun.h"
#include    "resource.h"

#include    <sys/mman.h>
#include    <pixrect/memreg.h>
#include    <pixrect/cg2reg.h>
#include    <struct.h>

#ifndef _MAP_NEW
extern caddr_t valloc();
#else
extern caddr_t mmap();
#endif  _MAP_NEW

/*-
 * The cg2 frame buffer is divided into several pieces.
 *	1) a stack of 8 monochrome bitplanes
 *	2) an array of 8-bit pixels
 *	3) a union of these two where modifications are done via RasterOp
 *	    chips
 *	4) various control registers
 *	5) a shadow colormap.
 *
 * Each of these things is at a given offset from the base of the 4Mb devoted
 * to each color board. In addition, the mmap() system call insists on the
 * address and the length to be mapped being aligned on 8K boundaries.
 */
struct cg2m_reg {
    char    	  	pad[4096];  /* The status register is at 0x309000 */
				    /* which isn't on an 8K boundary, so we */
				    /* have to pad this thing here to make */
				    /* the mmaping work... */
    union {
	struct cg2statusreg csr;    	/* Control/status register */
	char	  	    pad[4096];	/* This is the amount of room */
					/* dedicated to the status register */
    }	    	  	u_csr;
};

struct cg2m_ppmask {
    union {
	unsigned short	    ppmask; 	/* Per-plane mask */
	char	  	    pad[8192];	/* Padding to keep the length of */
					/* these registers page-aligned... */

    } u_ppmask;
};

struct cg2m_cmap {
    union {
	struct {  	/* Shouldn't these be u_char's??? */
	    u_short	    	redmap[256];	/* Red-component map */
	    u_short	    	greenmap[256];	/* Green-component map */
	    u_short	    	bluemap[256];	/* Blue-component map */
	}   	  	    cmap;
	char	  	    pad[8192];
    } u_cmap;
};

typedef struct cg2m {
    union bitplane	*image;	    /* The first bitplane -- treated as a */
				    /* monochrome frame buffer. */
    struct cg2m_reg *u_csr;		/* the status register */
    struct cg2m_ppmask *u_ppmask;	/* the plane mask register */
    struct cg2m_cmap *u_cmap;		/* the colormap */
} CG2M, CG2MRec, *CG2MPtr;

#define CG2M_IMAGE(fb)	    ((caddr_t)((fb).image))
#define CG2M_IMAGEOFF	    ((off_t)0x00000000)
#define CG2M_IMAGELEN	    (sizeof(union bitplane))
#define CG2M_REG(fb)	    ((caddr_t)((fb).u_csr))
#define CG2M_REGOFF	    ((off_t)0x00308000)
#define CG2M_REGLEN	    (2*8192)
#define CG2M_MASK(fb)       ((caddr_t)((fb).u_ppmask))
#define CG2M_MASKOFF        ((off_t)0x0030A000)
#define CG2M_MASKLEN        (0x2000)
#define CG2M_CMAP(fb)	    ((caddr_t)((fb).u_cmap))
#define CG2M_CMAPOFF	    ((off_t)0x00310000)
#define CG2M_CMAPLEN	    8192

static int  sunCG2MScreenIndex;

/*-
 *-----------------------------------------------------------------------
 * sunCG2MSaveScreen --
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
sunCG2MSaveScreen (pScreen, on)
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
    ((CG2MPtr)sunFbs[pScreen->myNum].fb)->u_csr->u_csr.csr.video_enab = state;
    return( TRUE );
}

/*-
 *-----------------------------------------------------------------------
 * sunCG2MCloseScreen --
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
sunCG2MCloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    Bool    ret;

    pScreen->CloseScreen = (Bool (*)()) pScreen->devPrivates[sunCG2MScreenIndex].ptr;
    ret = (*pScreen->CloseScreen) (i, pScreen);
    (void)(*pScreen->SaveScreen)(pScreen, SCREEN_SAVER_OFF);
    return ret;
}

/*-
 *-----------------------------------------------------------------------
 * sunCG2MInit --
 *	Attempt to find and initialize a cg2 framebuffer used as mono
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Most of the elements of the ScreenRec are filled in. The
 *	video is enabled for the frame buffer...
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
sunCG2MInit (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    if (!mfbScreenInit (pScreen,
			(pointer)(((CG2MPtr)sunFbs[index].fb)->image),
			sunFbs[index].info.fb_width,
			sunFbs[index].info.fb_height,
			monitorResolution, monitorResolution,
			sunFbs[index].info.fb_width))
	return (FALSE);

    pScreen->devPrivates[sunCG2MScreenIndex].ptr = (pointer) pScreen->CloseScreen;
    pScreen->CloseScreen = sunCG2MCloseScreen;
    pScreen->SaveScreen =   sunCG2MSaveScreen;
    pScreen->whitePixel =   0;
    pScreen->blackPixel =   1;

    /*
     * Enable video output...
     */
    sunCG2MSaveScreen(pScreen, SCREEN_SAVER_FORCER);

    return (sunScreenInit(pScreen) && mfbCreateDefColormap(pScreen));
}

/*-
 *-----------------------------------------------------------------------
 * sunCG2MProbe --
 *	Attempt to find and initialize a cg2 framebuffer used as mono
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
sunCG2MProbe (pScreenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int		i;
    int         fd;
    struct fbtype fbType;
    static CG2MRec	  CG2Mfb;

    if ((fd = sunOpenFrameBuffer(FBTYPE_SUN2COLOR, &fbType, index, fbNum, 
		argc, argv)) < 0)
	return FALSE;

#ifdef	_MAP_NEW
    if ((int)(CG2Mfb.image = (union bitplane *) mmap ((caddr_t) 0,
	      CG2M_IMAGELEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED | _MAP_NEW, fd, CG2M_IMAGEOFF)) == -1) {
		  Error ("Mapping cg2m.image");
		  goto bad;
    }
    if ((int)(CG2Mfb.u_csr = (struct cg2m_reg *) mmap ((caddr_t) 0,
	      CG2M_REGLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED | _MAP_NEW, fd, CG2M_REGOFF)) == -1) {
		  Error ("Mapping cg2m.reg");
		  goto bad;
    }
    if ((int)(CG2Mfb.u_ppmask = (struct cg2m_ppmask *) mmap ((caddr_t) 0,
	      CG2M_MASKLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED | _MAP_NEW, fd, CG2M_MASKOFF)) == -1) {
		  Error ("Mapping cg2m.reg");
		  goto bad;
    }
    if ((int)(CG2Mfb.u_cmap = (struct cg2m_cmap *) mmap ((caddr_t) 0,
	      CG2M_CMAPLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED | _MAP_NEW, fd, CG2M_CMAPOFF)) != -1) {
		  goto ok;
    }
    Error ("Mapping cg2m.cmap");
#else
    CG2Mfb.image = (union bitplane *)valloc (CG2M_IMAGELEN + CG2M_REGLEN +
			    CG2M_MASKLEN + CG2M_CMAPLEN);
    CG2Mfb.u_csr = (struct cg2m_reg *) ((char *)CG2Mfb.image +
			    CG2M_IMAGELEN);
    CG2Mfb.u_ppmask = (struct cg2m_ppmask *) ((char *)CG2Mfb.u_csr +
			    CG2M_REGLEN);
    CG2Mfb.u_cmap = (struct cg2m_cmap *) ((char *)CG2Mfb.u_ppmask +
			    CG2M_MASKLEN);
    if (CG2Mfb.image == (union bitplane *) NULL) {
	ErrorF ("Could not allocate room for frame buffer.\n");
	return FALSE;
    }

    if (mmap (CG2M_IMAGE(CG2Mfb), CG2M_IMAGELEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED, fd, CG2M_IMAGEOFF) < 0) {
		  Error ("Mapping cg2m.image");
		  goto bad;
    }
    if (mmap (CG2M_REG(CG2Mfb), CG2M_REGLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED, fd, CG2M_REGOFF) < 0) {
		  Error ("Mapping cg2m.reg");
		  goto bad;
    }
    if (mmap (CG2M_MASK(CG2Mfb), CG2M_MASKLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED, fd, CG2M_MASKOFF) < 0) {
		  Error ("Mapping cg2m.mask");
		  goto bad;
    }       
    if (mmap (CG2M_CMAP(CG2Mfb), CG2M_CMAPLEN, PROT_READ | PROT_WRITE,
	      MAP_SHARED, fd, CG2M_CMAPOFF) >= 0) {
		  goto ok;
    }
    Error ("Mapping cg2m.cmap");
#endif	_MAP_NEW
bad:
    (void) close (fd);
    return FALSE;

ok:
    /*
     * Enable only the first plane and make all even pixels be white,
     * while all odd pixels are black.
     */
    CG2Mfb.u_ppmask->u_ppmask.ppmask = 1;
    CG2Mfb.u_csr->u_csr.csr.update_cmap = 0;
    for ( i=0; i<256; i+=2 ) {
	CG2Mfb.u_cmap->u_cmap.cmap.redmap[i] =
	    CG2Mfb.u_cmap->u_cmap.cmap.greenmap[i] =
		CG2Mfb.u_cmap->u_cmap.cmap.bluemap[i] = 255;
	CG2Mfb.u_cmap->u_cmap.cmap.redmap[i+1] =
	    CG2Mfb.u_cmap->u_cmap.cmap.greenmap[i+1] =
		CG2Mfb.u_cmap->u_cmap.cmap.bluemap[i+1] = 0;
    }
    CG2Mfb.u_csr->u_csr.csr.update_cmap = 1;

    sunFbs[index].fd = fd;
    sunFbs[index].info = fbType;
    sunFbs[index].fb = (pointer) &CG2Mfb;
    sunFbs[index].EnterLeave = NoopDDA;
    return TRUE;
}

/*ARGSUSED*/
Bool
sunCG2MCreate(pScreenInfo, argc, argv)
    ScreenInfo	  *pScreenInfo;
    int	    	  argc;
    char    	  **argv;
{
    if (sunGeneration != serverGeneration)
    {
	sunCG2MScreenIndex = AllocateScreenPrivateIndex();
	if (sunCG2MScreenIndex < 0)
	    return FALSE;
    }
    return (AddScreen(sunCG2MInit, argc, argv) >= 0);
}
