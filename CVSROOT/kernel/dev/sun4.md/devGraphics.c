/* 
 * devGraphics.c --
 *
 *	This module provides frame buffer device support.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "dev.h"
#include <sys/ioctl.h>
#include "devfb.h"
#include "fs.h"
#include "vmMach.h"
#include "rpc.h"
#include "stdio.h"
#include "sys/types.h"
#include "mon/eeprom.h"
#include "machMon.h"
#include "stdlib.h"
#include "string.h"
#include "bstring.h"

#define EEC_COLOR_TYPE_CG4      4       /* missing in mon/eeprom.h */
#define EEC_COLOR_TYPE_CG6      6       /* missing in mon/eeprom.h */

/*
 * For BW2 frame buffer
 */


#ifdef sun3
#define BW2_FB	((Address) 0x0fe20000)
#elif sun4c
#define BW2_FB	((Address) 0xffd80000)
#elif sun4
#define BW2_FB	((Address) 0xffd40000)
#else
#define	BW2_FB	((Address) NIL)
#endif /* sun3 */


/*
 * For CG4 frame buffer
 */
#ifdef sun3
#define CG4_FB     ((Address) 0x0fd00000)
#define CG4_CM     ((Address) 0x0fe0e000)
#define CG4_OV     ((Address) 0x0fe80000)
#define CG4_EN     ((Address) 0x0fea0000)
#elif sun4c
#define CG4_FB    ((Address) 0xffd80000)
#define CG4_CM    ((Address) 0xffd1c000)
#define CG4_OV    ((Address) 0x0)           /* ??? */
#define CG4_EN    ((Address) 0x0)           /* ??? */
#else
#define CG4_FB    ((Address) NIL)
#define CG4_CM    ((Address) NIL)
#define CG4_OV    ((Address) NIL)
#define CG4_EN    ((Address) NIL)
#endif /* sun3 */
/*
 * For CG6 frame buffer
 */
#ifdef sun4c
#define CG6_FB    ((Address) 0xffd80000)
#define CG6_CM    ((Address) 0xffd1f000)
#else
#define CG6_FB    ((Address) NIL)
#define CG6_CM    ((Address) NIL)
#endif /* sun4c */
/*
 * CG3 frame buffer.
 */
#ifdef sun4c
#define CG3_FB    ((Address) 0xffd80000)
#define CG3_CM    ((Address) 0xffd1c000)
#else
#define CG3_FB    ((Address) 0xffd80000)
#define CG3_CM    ((Address) 0xffd1c000)
#endif /* sun4c */


typedef	struct	FBDevice {
    FBType	type;
    FBInfo	info;
    FBCMap	cmap;
} FBDevice;

/*
 * Associates a string with each fb type so we can use a hack to look up
 * the type per machine from a file rather than the prom...
 */
char	*fbNames[FBTYPE_LASTPLUSONE] = {
	"bwone",
	"cgone",
	"bwtwo",
	"cgtwo",
	"FBTYPE_SUN_GP2",
	"FBTYPE_SUN_CG5",
	"cgthree",
	"FBTYPE_MEMCOLOR",
	"cgfour",
	"FBTYPE_NOTSUN1",
	"FBTYPE_NOTSUN2",
	"FBTYPE_NOTSUN3",
	"cgsix",
	"FBTYPE_SUNROP_COLOR",
	"FBTYPE_SUNFB_VIDEO",
	"FBTYPE_RESERVED5",
	"FBTYPE_RESERVED4",
	"FBTYPE_RESERVED3",
	"FBTYPE_RESERVED2",
	"FBTYPE_RESERVED1"
};

/*
 * Brooktree DAC
 */
volatile struct colormap {
    unsigned int	addr;		/* colormap address register */
    unsigned int	cmap;		/* colormap data register */
    unsigned int	ctrl;		/* control register */
    unsigned int	omap;		/* overlay map data register */
} *fbCmap = (volatile struct colormap *) NIL;

/* Copy of colormap (for CG4 only!) */
static union {
        unsigned char   map[256][3];    /* reasonable way to access */
        unsigned int    raw[256*3/4];   /* unreasonable way used to load h/w */
} fbCmapCopy;


/*
 * Addresses to know for the different frame buffers, overlay planes, etc.
 */
typedef	struct FBAddr {
    Address fb_buffer;		/* kernel virtual address */
    Address fb_overlay;		/* offset? */
    Address fb_enable;		/* offset? */
    Address fb_cmap;		/* cmap */
} FBAddr;

/*
 * Addresses for frame buffer, overlay and enable.  This is in a separate
 * array since it's different per machine type.  I only have it for
 * one machine type right now (sun4c).
 */
FBAddr	fbaddrs[FBTYPE_LASTPLUSONE] = {
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* bw1 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* cg1 */
    {BW2_FB, (Address) NULL, (Address) NULL, (Address) NULL},	  /* bw2 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* cg2 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* gp2 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* cg5 */
    {CG3_FB, (Address) NULL, (Address) NULL, CG3_CM},		  /* cg3 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* ? */
    {CG4_FB, CG4_OV, CG4_EN, CG4_CM},				  /* cg4 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* cust. */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* cust. */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* cust. */
    {CG6_FB, (Address) NULL, (Address) NULL, CG6_CM},		  /* cg6 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* rop */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* video */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* res5 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* res4 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* res3 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}, /* res2 */
    {(Address) NIL, (Address) NIL, (Address) NIL, (Address) NIL}  /* res1 */
};


/*
 * For convenience we store info about the frame buffer types here.  If we
 * get the info from the prom, we overwrite the stuff here.
 * Array indexed by fb_type, found in fb.h.
 * An example of what would be overwritten is if we have a high resolution
 * b&w screen, the info in the prom will give us
 *	{high resolution bw2, 1280, 1600, 1, 2, 256*1024},	(* bw2h *)
 * instead of the regular bw2.  Also, for cg3, we could have a second type:
 *	{cg3b, 768, 1024, -1, -1, -1} 			(* cg3 B *)
 */
		/* type, height, width, depth, cmsize, size */
FBType	fbarray[FBTYPE_LASTPLUSONE] = {
	{FBTYPE_SUN1BW, -1, -1, -1, -1, -1},			/* bw1 */
	{FBTYPE_SUN1COLOR, -1, -1, -1, -1, -1},			/* cg1 */
	{FBTYPE_SUN2BW, 900, 1152, 1, 2, 128*1024},		/* bw2 */
	{FBTYPE_SUN2COLOR, 900, 1152, 8, -1, -1},		/* cg2 */
	{FBTYPE_SUN2GP, -1, -1, -1, -1, -1},			/* gp2 */
	{FBTYPE_SUN5COLOR, -1, -1, -1, -1, -1},			/* cg5? |bw3? */
	{FBTYPE_SUN3COLOR, 900, 1152, 8, 256, 1024*1024},	/* cg3 A */
	{FBTYPE_MEMCOLOR, -1, -1, -1, -1, -1},			/* ? | bw4? */
	{FBTYPE_SUN4COLOR, 900, 1152, 8, 256, 1024*1024},	/* cg4 */
	{FBTYPE_NOTSUN1, -1, -1, -1, -1, -1},			/* customer */
	{FBTYPE_NOTSUN2, -1, -1, -1, -1, -1},			/* customer */
	{FBTYPE_NOTSUN3, -1, -1, -1, -1, -1},			/* customer */
	{FBTYPE_SUNFAST_COLOR, 900, 1152, 8, 256, 1024*1024},	/* cg6=gx*/
	{FBTYPE_SUNROP_COLOR, -1, -1, -1, -1, -1},
	{FBTYPE_SUNFB_VIDEO, -1, -1, -1, -1, -1},
	{FBTYPE_RESERVED5, -1, -1, -1, -1, -1},			/* don't use */
	{FBTYPE_RESERVED4, -1, -1, -1, -1, -1},			/* don't use */
	{FBTYPE_RESERVED3, -1, -1, -1, -1, -1},			/* don't use */
	{FBTYPE_RESERVED2, -1, -1, -1, -1, -1},			/* don't use */
	{FBTYPE_RESERVED1, -1, -1, -1, -1, -1}			/* don't use */
};

/*
 *
 * Thorsten's notes for the old user-level fbio stuff:
 *
 *
 * Brooktree DAC/colormap information
 *
 * Operation theory (really: guesses)
 *
 * The DAC has four byte size "ports" (cpu accessible registers) which have to
 * be used to access the three internal register files with the 256 colormap
 * RGB entries, a few control registers and 4 overlay RGB entries.
 * To access a control register: write the register number (0-7?) into
 * "addr" (see struct below), then write the value into "ctrl".
 * To access a RGB entry: write the entry number (0-255 or 0-3) into "addr"
 * then write the R, G and B values (in that order) into "cmap" or "omap".
 * After the access, the "addr" is automatically incremented to allow quick
 * updates of successive colormap entries.
 *
 * Now, this was the meat and here comes the spice:
 * On the CG6 board, the ports are on (mod 4)=0 addresses and require
 * longword accesses. This means the value has to go into bits 24-31 of
 * an int which is then written to the chip. (Similarly for reading.)
 * On the CG4 board, things are even better: the ports are on (mod 4)=3
 * addresses and require longword accesses. The value can thus reside
 * in the low 8 bits of an int. However, colormap RGB values behave
 * wonderfully different: one longword write to "cmap" or "omap" is
 * turned (by the hardware) into four byte writes to the chip, the
 * top byte first, the bottom one last. (Dunno 'bout reads.) This means
 * that one write to "cmap" or "omap" writes 4/3 colormap RGB entries.
 * For example, writing a "1" into "addr" and 0x01020304 into "cmap" will
 * set green and blue of color 1 to 0x01 and 0x02 resp. and will set
 * red and green of color 2 to 0x03 and 0x04 resp.!
 *
 * Note: I didn't build these suckers, I just poked at them and guessed
 * the behaviour, and I might be wrong...
 *                      Thorsten von Eicken, 2/19/90
 */

/*
 * forward declarations for internal routines
 */
#ifdef sun4c
static int SearchProm _ARGS_((unsigned int node));
#endif
static int GetFBType _ARGS_((void));
static ReturnStatus PutCmap _ARGS_((int whichFb, FBCMap *cmap));
static ReturnStatus GetCmap _ARGS_((int whichFb, FBCMap *cmap));
static ReturnStatus SVideo _ARGS_((int whichFb, int *statePtr));
static ReturnStatus GVideo _ARGS_((int whichFb, int *statePtr));
static ReturnStatus InitCmap _ARGS_((FBDevice *devPtr));




/*
 *----------------------------------------------------------------------
 *
 * DevFBOpen --
 *
 *      Open the system frame buffer device.
 *
 * Results:
 *      SUCCESS         - the device was opened.
 *      FAILURE         - something went wrong.
 *
 * Side effects:
 *      The device is opened.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevFBOpen(devicePtr, useFlags, token, flagsPtr)
    Fs_Device	*devicePtr;	/* Device info, unit number, etc. */
    int		useFlags;	/* Flags from the stream being opened. */
    Fs_NotifyToken	token;	/* Call-back token for input, unused here. */
    int		*flagsPtr;	/* OUT: Device open flags. */
{
    FBDevice		*devPtr;
    extern	int	GetFBType();
    FBType	*typePtr;
    int		machArch;
    int		machType;
    int		whichFb = -1;
    struct	eeprom	*eeprom;
    struct	eed_conf	*eeconf;
    int		i;


    devPtr = (FBDevice *) malloc(sizeof (FBDevice));
    bzero((char *) devPtr, sizeof (FBDevice));

    machArch = Mach_GetMachineArch();
    machType = Mach_GetMachineType();

    devPtr->type.fb_type = -1;

    switch (machArch) {
    case SYS_SUN4:
	if ((machType & SYS_SUN_ARCH_MASK) == SYS_SUN_4_C ) {
#ifndef PROM_1_4
	    whichFb = GetFBType();
#else /* PROM_1_4 */
	    /*
	     * Boing can't handle getting this out of the prom.
	     */
	    whichFb = FBTYPE_SUN2BW;
#endif /* PROM_1_4 */
	    if (whichFb == -1) {
		whichFb = FBTYPE_SUN2BW;
	    }
	    break;
	}
	/* Fall through for SYS_SUN4. */
    case SYS_SUN3:
	eeprom = (struct eeprom *)EEPROM_BASE;
	eeconf = &(eeprom->ee_diag.eed_conf[0]);
	for (i = 0; i < MAX_SLOTS; i++, eeconf++) {
#ifdef  FOOBAR
	    printf("card type %d\n", eeconf->eec_un.eec_type);
#endif  FOOBAR
	    /* end of card cage? */
	    if (eeconf->eec_un.eec_type == EEC_TYPE_END) {
		break;
	    }
	    /* color display? */
	    if (eeconf->eec_un.eec_type == EEC_TYPE_COLOR) {
#ifdef  FOOBAR
		printf("\tcolor type %d\n",
			eeconf->eec_un.eec_color.eec_color_type);
#endif  FOOBAR
		switch (eeconf->eec_un.eec_color.eec_color_type) {
		case EEC_COLOR_TYPE_CG4:
		    whichFb = FBTYPE_SUN4COLOR;
		    break;
		case EEC_COLOR_TYPE_CG6:
		    whichFb = FBTYPE_SUNFAST_COLOR;
		    break;
		default:
		    ; /* just ignore... */
		}
	    }
	    /* b/w display? (note: give preference to color) */
	    if (whichFb == -1 && eeconf->eec_un.eec_type == EEC_TYPE_BW) {
		if (eeprom->ee_diag.eed_scrsize == EED_SCR_1600X1280) {
		    whichFb = FBTYPE_SUN2BW;
		    fbarray[whichFb].fb_height = 1280;
		    fbarray[whichFb].fb_width = 1600;
		    fbarray[whichFb].fb_size = 256 * 1024;
		} else {
		    whichFb = FBTYPE_SUN2BW;
		}
	    }
	}
	/* assume b/w as default */
	if (whichFb == -1) {
	    whichFb = FBTYPE_SUN2BW;
	}
	if ((whichFb == FBTYPE_SUN2BW) &&
		(eeprom->ee_diag.eed_scrsize == EED_SCR_1600X1280)) {
	    whichFb = FBTYPE_SUN2BW;
	    fbarray[whichFb].fb_height = 1280;
	    fbarray[whichFb].fb_width = 1600;
	    fbarray[whichFb].fb_size = 256 * 1024;
	}
	break;

    case SYS_DS3100:
	printf("Can't do FB stuff for ds3100's yet.\n");
	return FAILURE;
    default:
	printf("FB stuff won't handle this machine type yet.\n");
	return FAILURE;
    }

    if (whichFb < 0 || whichFb >= FBTYPE_LASTPLUSONE) {
	printf("FB type is out of range.\n");
	return FAILURE;
    }
    typePtr = &fbarray[whichFb];
    devPtr->type.fb_type = typePtr->fb_type;

    devicePtr->data = (ClientData) devPtr;

    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * DevFBIOControl --
 *
 *      Perform device-specific functions with the frame buffer.
 *
 * Results:
 *      GEN_NOT_IMPLEMENTED if io control not supported.  GEN_INVALID_ARG
 *	if something else went wrong.  SUCCESS otherwise, with the type
 *	of the frame buffer described in the out-going buffer.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
DevFBIOControl(devicePtr, ioctlPtr, replyPtr)
    Fs_Device	*devicePtr;	/* Handle for device. */
    Fs_IOCParam	*ioctlPtr;	/* Standard I/O Control parameter block. */
    Fs_IOReply	*replyPtr;	/* Size of outBuffer and returned signal. */
{
    FBDevice	*devPtr;
    FBType	*typePtr;

    devPtr = (FBDevice *) (devicePtr->data);

    switch (ioctlPtr->command) {
    case FBIOGTYPE:
	if (ioctlPtr->outBufSize < sizeof (FBType)) {
	    printf("Bad outbuf size.\n");
	    return GEN_INVALID_ARG;
	}
	if (devPtr->type.fb_type < 0 ||
		devPtr->type.fb_type >= FBTYPE_LASTPLUSONE) {
	    printf("fbtype was bad.\n");
	    return FAILURE;
	}
	typePtr = &(fbarray[devPtr->type.fb_type]);
	/* copy to outbuf */
	devPtr->type.fb_height = typePtr->fb_height;
	devPtr->type.fb_width = typePtr->fb_width;
	devPtr->type.fb_depth = typePtr->fb_depth;
	devPtr->type.fb_cmsize = typePtr->fb_cmsize;
	devPtr->type.fb_size = typePtr->fb_size;

	bcopy((char *) &(devPtr->type), (char *) ioctlPtr->outBuffer,
		sizeof (FBType));

	break;
#ifdef NOTDEF
    /*
     * Does this ioctl really exist?  Is it used?
     */
    case FBIOGINFO:
	if (ioctlPtr->outBufSize < sizeof (FBInfo)) {
	    return GEN_INVALID_ARG;
	}
	devPtr->info.fb_physaddr = XX;
	devPtr->info.fb_hwwidth = XX;
	devPtr->info.fb_hwheight = XX;
	devPtr->info.fb_addrdelta = XX;
	devPtr->info.fb_ropaddr = XX;
	devPtr->info.fb_unit = XX;

	break;
#endif /* NOTDEF */
    case FBIOPUTCMAP:
	if (ioctlPtr->inBufSize < sizeof (FBCMap)) {
	    printf("Bad inbuf size.\n");
	    return GEN_INVALID_ARG;
	}

	if (devPtr->type.fb_type < 0 ||
		devPtr->type.fb_type >= FBTYPE_LASTPLUSONE) {
	    printf("fbtype was bad.\n");
	    return FAILURE;
	}
	if (fbCmap == (struct colormap *) NIL) {
	    if (InitCmap(devPtr) != SUCCESS) {
		printf("Couldn't initialize colormap.\n");
		return FAILURE;
	    }
	}

	if (PutCmap(devPtr->type.fb_type,
		(FBCMap *) ioctlPtr->inBuffer) != SUCCESS) {
	    return FAILURE;
	}

	break;

    case FBIOGETCMAP:
	if (ioctlPtr->outBufSize < sizeof (FBCMap)) {
	    printf("Bad outbuf size.\n");
	    return GEN_INVALID_ARG;
	}
	if (devPtr->type.fb_type < 0 ||
		devPtr->type.fb_type >= FBTYPE_LASTPLUSONE) {
	    printf("fbtype was bad.\n");
	    return FAILURE;
	}
	if (fbCmap == (struct colormap *) NIL) {
	    if (InitCmap(devPtr) != SUCCESS) {
		printf("Couldn't initialize colormap.\n");
		return FAILURE;
	    }
	}
	if (GetCmap(devPtr->type.fb_type,
		(FBCMap *) ioctlPtr->outBuffer) != SUCCESS) {
	    return FAILURE;
	}

	break;

    case FBIOSVIDEO:
	if (ioctlPtr->inBufSize < sizeof (int)) {
	    printf("Bad inbuf size.\n");
	    return GEN_INVALID_ARG;
	}

	if (devPtr->type.fb_type < 0 ||
		devPtr->type.fb_type >= FBTYPE_LASTPLUSONE) {
	    printf("fbtype was bad.\n");
	    return FAILURE;
	}

	/*
	 * If the display is a color display, initialize the color map first.
	 */
	switch (devPtr->type.fb_type) {
	case FBTYPE_SUN1COLOR:
	case FBTYPE_SUN2COLOR:
	case FBTYPE_SUN2GP:		/* color? */
	case FBTYPE_SUN5COLOR:
	case FBTYPE_SUN3COLOR:
	case FBTYPE_MEMCOLOR:
	case FBTYPE_SUN4COLOR:
	case FBTYPE_SUNFAST_COLOR:
	case FBTYPE_SUNROP_COLOR:
	case FBTYPE_SUNFB_VIDEO:	/* color? */
	    if (fbCmap == (struct colormap *) NIL) {
		if (InitCmap(devPtr) != SUCCESS) {
		    printf("Couldn't initialize colormap.\n");
		    return FAILURE;
		}
	    }
	    break;
	default:
	    /* Do nothing. */
	    break;
	}

	if (SVideo(devPtr->type.fb_type,
		(int *) ioctlPtr->inBuffer) != SUCCESS) {
	    return FAILURE;
	}

	break;

    case FBIOGVIDEO:
	if (ioctlPtr->outBufSize < sizeof (int)) {
	    printf("Bad outbuf size.\n");
	    return GEN_INVALID_ARG;
	}

	if (devPtr->type.fb_type < 0 ||
		devPtr->type.fb_type >= FBTYPE_LASTPLUSONE) {
	    printf("fbtype was bad.\n");
	    return FAILURE;
	}
	/*
	 * If the display is a color display, initialize the color map first.
	 */
	switch (devPtr->type.fb_type) {
	case FBTYPE_SUN1COLOR:
	case FBTYPE_SUN2COLOR:
	case FBTYPE_SUN2GP:		/* color? */
	case FBTYPE_SUN5COLOR:
	case FBTYPE_SUN3COLOR:
	case FBTYPE_MEMCOLOR:
	case FBTYPE_SUN4COLOR:
	case FBTYPE_SUNFAST_COLOR:
	case FBTYPE_SUNROP_COLOR:
	case FBTYPE_SUNFB_VIDEO:	/* color? */
	    if (fbCmap == (struct colormap *) NIL) {
		if (InitCmap(devPtr) != SUCCESS) {
		    printf("Couldn't initialize colormap.\n");
		    return FAILURE;
		}
	    }
	    break;
	default:
	    /* Do nothing. */
	    break;
	}

	if (GVideo(devPtr->type.fb_type,
		(int *) ioctlPtr->outBuffer) != SUCCESS) {
	    return FAILURE;
	}
	
	break;

    case FBIOSATTR:
    case FBIOGATTR:
#ifdef NOTDEF
    case FBIOGVERTICAL:
#endif NOTDEF
	printf("Not implemented.\n");
	return GEN_NOT_IMPLEMENTED;
    default:
	printf("Default: invalid arg.\n");
	return GEN_INVALID_ARG;
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * DevFBClose --
 *
 *      Close the frame buffer.
 *
 * Results:
 *      SUCCESS         - always returned.
 *
 * Side effects:
 *      The frame buffer is "closed".
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
ReturnStatus
DevFBClose(devicePtr, useFlags, openCount, writerCount)
    Fs_Device   *devicePtr;		/* Information about device. */
    int         useFlags;		/* Indicates whether stream being
					 * closed was for reading and/or
					 * writing:  OR'ed combination of
					 * FS_READ and FS_WRITE. */
    int         openCount;		/* # of times this particular stream
					 * is still open. */
    int         writerCount;		/* # of times this particular stream
					 * is still open for writing. */
{
    if ((openCount == 0) && (devicePtr->data != (ClientData) NIL)) { 
	free((Address) devicePtr->data);
    }

    return SUCCESS;
}

char	searchBuffer[1024];


/*
 *----------------------------------------------------------------------
 *
 * SearchProm --
 *
 *	Search through the prom devices to find which frame buffer we have.
 *
 * Results:
 *	The frame buffer type.
 *
 * Side effects:
 *	Info about the device from the prom gets put into the fb table.
 *
 *----------------------------------------------------------------------
 */
#ifdef sun4c
static int
SearchProm(node)
    unsigned	int		node;
{
    unsigned	int		newNode;
    int		length = 0;
    struct	config_ops	*configPtr;
    int		i;
    int		whichFb;

    configPtr = romVectorPtr->v_config_ops;
    while (node != 0) {
	length = configPtr->devr_getproplen(node, "name");
	if (length > 0) {
	    if (length > sizeof (searchBuffer)) {
		panic("SearchProm: buffer too small.\n");
	    }
	    configPtr->devr_getprop(node, "name", searchBuffer);
	    for (i = 0; i < FBTYPE_LASTPLUSONE; i++) {
		if (strcmp(searchBuffer, fbNames[i]) == 0) {
		    whichFb = i;
		    /* fill it in */
		    length = configPtr->devr_getproplen(node, "address");
		    if (length <= 0) {
			printf("No address found for frame buffer in prom.\n");
		    } else {
			configPtr->devr_getprop(node, "address", searchBuffer);
			if (fbaddrs[i].fb_buffer !=
				(Address) (*(int *) searchBuffer)) {
#ifdef DEBUG
			    printf("Updating address for %s to 0x%x.\n",
				    fbNames[whichFb], *(int *) searchBuffer);
#endif /* DEBUG */
			    fbaddrs[i].fb_buffer =
				    (Address) (*(int *) searchBuffer);
			}
		    }
		    length = configPtr->devr_getproplen(node, "height");
		    if (length <= 0) {
			printf("No height found for frame buffer in prom.\n");
		    } else {
			configPtr->devr_getprop(node, "height", searchBuffer);
			if (fbarray[i].fb_height != *(int *) searchBuffer) {
#ifdef DEBUG
			    printf("Updating height for %s to 0x%x.\n",
				    fbNames[whichFb], *(int *) searchBuffer);
#endif /* DEBUG */
			    fbarray[i].fb_height = *(int *) searchBuffer;
			}
		    }
		    length = configPtr->devr_getproplen(node, "width");
		    if (length <= 0) {
			printf("No width found for frame buffer in prom.\n");
		    } else {
			configPtr->devr_getprop(node, "width", searchBuffer);
			if (fbarray[i].fb_width != *(int *) searchBuffer) {
#ifdef DEBUG
			    printf("Updating width for %s to 0x%x.\n",
				    fbNames[whichFb], *(int *) searchBuffer);
#endif /* DEBUG */
			    fbarray[i].fb_width = *(int *) searchBuffer;
			}
		    }
		    length = configPtr->devr_getproplen(node, "depth");
		    if (length <= 0) {
			printf("No depth found for frame buffer in prom.\n");
		    } else {
			configPtr->devr_getprop(node, "depth", searchBuffer);
			if (fbarray[i].fb_depth != *(int *) searchBuffer) {
#ifdef DEBUG
			    printf("Updating depth for %s to 0x%x.\n",
				    fbNames[whichFb], *(int *) searchBuffer);
#endif /* DEBUG */
			    fbarray[i].fb_depth = *(int *) searchBuffer;
			}
		    }
		    return whichFb;
		}
	    }
	}
	newNode = configPtr->devr_child(node);
	whichFb = SearchProm(newNode);
	if (whichFb >= 0) {
	    return whichFb;
	}
	node = configPtr->devr_next(node);
    }
    return -1;
}
#endif /* sun4c */


/*
 *----------------------------------------------------------------------
 *
 * GetFBType --
 *
 *	Temporary routine to find frame buffer type from a file rather than
 *	from the prom.
 *
 * Results:
 *	Integer representing frame buffer type.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
static int
GetFBType()
{
#ifdef sun4c
    struct	config_ops	*configPtr;
    unsigned	int		node;

    configPtr = romVectorPtr->v_config_ops;
    /*
     * Find a frame buffer type:  First get the root node id of the tree of
     * devices in the prom.  Then traverse it depth-first to find some frame
     * buffer.
     */
    node = configPtr->devr_next(0);

    return SearchProm(node);
#else
    return -1;
#endif /* sun4c */
}



/*
 *----------------------------------------------------------------------
 *
 * DevFBMMap --
 *
 *	Map a device into user space.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	Device area made accessible to user.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
ReturnStatus
DevFBMMap(devicePtr, startAddr, length, offset, newAddrPtr)
    Fs_Device	*devicePtr;
    Address	startAddr;
    int		length;
    int		offset;
    Address	*newAddrPtr;
{
    FBDevice	*devPtr;
    Address	kernelAddr;	/* Virtual address in kernel of device. */
    Address	kernelAlignedAddr;	/* Aligned virtual addr in kernel. */
    int		numBytes;
    ReturnStatus	status;

    devPtr = (FBDevice *) (devicePtr->data);

    kernelAddr = (Address) (fbaddrs[devPtr->type.fb_type].fb_buffer);
    if (kernelAddr == (Address) NIL) {
	printf("FB device has no kernel address.\n");
	return FAILURE;
    }
    /*
     * Must align user address forwards rather than backwards.  They must
     * have allocated an extra segment.
     */
    kernelAlignedAddr = (Address)
	    (((unsigned int)kernelAddr) & ~(VMMACH_SEG_SIZE - 1));
    numBytes = ((unsigned int)((kernelAddr + length) - kernelAlignedAddr) +
	    (VMMACH_SEG_SIZE - 1)) & ~(VMMACH_SEG_SIZE - 1);
    status = VmMach_IntMapKernelIntoUser((unsigned int) kernelAlignedAddr,
	    numBytes, (unsigned int) startAddr, newAddrPtr);
    if (status != SUCCESS) {
	return status;
    }
#ifdef DEBUG
    printf("Real VA would be 0x%x\n", *newAddrPtr);
#endif /* DEBUG */

    return SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * PutCmap --
 *
 *	Update the hardware colormap.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	Memory updated.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
PutCmap(whichFb, cmap)
    int		whichFb;
    FBCMap	*cmap;
{
    unsigned char   *uPtr;
    unsigned int    *iPtr;
    int             c;
    int             index = cmap->index;
    int             count = cmap->count;
    unsigned char   *rmap = cmap->red;
    unsigned char   *gmap = cmap->green;
    unsigned char   *bmap = cmap->blue;

    if(index < 0) {
	printf("cmap index was < 0.\n");
	return FAILURE;
    }

    /* Handle colors 0..255 */
    if (index >= 0 && index < 256) {
	if (index+count > 256) {
	    count = 256 - index;
	}
	if (whichFb == FBTYPE_SUN4COLOR || whichFb == FBTYPE_SUN3COLOR) {
	    /* update the memory copy */
	    uPtr = &fbCmapCopy.map[index][0];
	    for (c = count; c != 0; --c) {
		*uPtr++ = *rmap++;
		*uPtr++ = *gmap++;
		*uPtr++ = *bmap++;
	    }

	    /* update DAC: weird 4/3 entries per word mapping */
#define D4M3(x) ((((x)>>2)<<1) + ((x)>>2))      /* (x/4)*3 */
#define D4M4(x) ((x)&~0x3)                      /* (x/4)*4 */
	    iPtr = &fbCmapCopy.raw[D4M3(index)];
	    fbCmap->addr = D4M4(index);
	    for (c = D4M3(index+count-1) - D4M3(index) + 3; c != 0; --c) {
		fbCmap->cmap = *iPtr++;
	    }
	} else { /* FBTYPE_SUNFAST_COLOR */
	    /* update the chip */
	    fbCmap->addr = index << 24;
	    for (c = count; c != 0; --c) {
		fbCmap->cmap = (unsigned int)(*rmap++) << 24;
		fbCmap->cmap = (unsigned int)(*gmap++) << 24;
		fbCmap->cmap = (unsigned int)(*bmap++) << 24;
	    }
	}

	/* What's left? */
	index += count;
	count = cmap->count-count;
    }

    /* Any overlay color changes? */
    if (index >= 256 && count > 0) {
/******* dunno how to do that */
    }
    return SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * GetCmap --
 *
 *	Return the hardware colormap.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	Memory updated.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GetCmap(whichFb, cmap)
    int		whichFb;
    FBCMap	*cmap;
{
    unsigned char   *uPtr;
    int             c;
    int             index = cmap->index;
    int             count = cmap->count;
    unsigned char   *rmap = cmap->red;
    unsigned char   *gmap = cmap->green;
    unsigned char   *bmap = cmap->blue;

    if(index < 0) {
	return FAILURE;
    }

    /* Handle colors 0..255 */
    if(index >= 0 && index < 256) {
	if(index+count > 256) {
	    count = 256 - index;
	}
	if (whichFb == FBTYPE_SUN4COLOR || whichFb == FBTYPE_SUN3COLOR) {
	    /* copy from the memory copy */
	    uPtr = &fbCmapCopy.map[index][0];
	    c = count;
	    while(c--) {
		*rmap++ = *uPtr++;
		*gmap++ = *uPtr++;
		*bmap++ = *uPtr++;
	    }
	} else { /* FBTYPE_SUNFAST_COLOR */
	    /* get it from the chip */
	    fbCmap->addr = index << 24;
	    c = count;
	    while(c--) {
		*rmap++ = (unsigned char)(fbCmap->cmap >> 24);
		*gmap++ = (unsigned char)(fbCmap->cmap >> 24);
		*bmap++ = (unsigned char)(fbCmap->cmap >> 24);
	    }
	}

	/* What's left? */
	index += count;
	count = cmap->count-count;
    }

    /* Any overlay color requested? */
    if(index >= 256 && count > 0) {
/******* dunno how to do that */
    }

    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * SVideo --
 *
 *	Turn on or off the video.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	Video turned on or off.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
SVideo(whichFb, statePtr)
    int		whichFb;
    int		*statePtr;
{
    int		onOff;

    onOff = *statePtr;
    switch (whichFb) {
    case FBTYPE_SUN2BW:
/******* dunno how to do that */
	return SUCCESS;
    case FBTYPE_SUN4COLOR:
    case FBTYPE_SUNFAST_COLOR:
    case FBTYPE_SUN3COLOR:
	/* get colormap access */
	if(fbCmap == (struct colormap *) NIL) {
	    printf("Colormap not yet set.\n");
	    return FAILURE;
	}

	/* Twiddle command registers to turn video off */
	if (onOff) {
	     onOff = ~0;
	}
	if(whichFb == FBTYPE_SUNFAST_COLOR) {
	    fbCmap->addr = 0x04 << 24;      /* read mask */
	    fbCmap->ctrl = onOff;           /* color planes */
	} else {
	    /* overlay off for blanking */
	    fbCmap->addr = 0x06;            /* command reg */
	    fbCmap->ctrl = 0x70|(onOff&3);  /* overlay plane */
	    /* read mask off for blanking */
	    fbCmap->addr = 0x04;            /* read mask */
	    fbCmap->ctrl = onOff;           /* color planes */
	    if(!onOff) {
		/* color 0 -> black for blanking */
		fbCmap->addr = 0x00;
		fbCmap->cmap = 0x00000000;
	    } else {
		/* restore colors */
		fbCmap->addr = 0x00;
		fbCmap->cmap = fbCmapCopy.raw[0];
		fbCmap->cmap = fbCmapCopy.raw[1];
		fbCmap->cmap = fbCmapCopy.raw[2];
	    }
	}
	return SUCCESS;
    default:
	return FAILURE;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GVideo --
 *
 *	Get the on/off status of the video.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	Video status returned in out parameter.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
GVideo(whichFb, statePtr)
    int		whichFb;
    int		*statePtr;
{
    int		rmask;

    switch (whichFb) {
    case FBTYPE_SUN2BW:
	/* Don't know how to do this one. */
	return SUCCESS;
    case FBTYPE_SUN4COLOR:
    case FBTYPE_SUNFAST_COLOR:
    case FBTYPE_SUN3COLOR:
	/* get colormap access */
	if(fbCmap == (struct colormap *) NIL) {
	    printf("Colormap not yet set.\n");
	    return FAILURE;
	}
	/* Look hard at the control registers */
	if(whichFb == FBTYPE_SUNFAST_COLOR) {
	    fbCmap->addr = 0x04 << 24;      /* read mask */
	    rmask = fbCmap->ctrl >> 24;     /* color planes */
	} else {
	    fbCmap->addr = 0x04;            /* read mask */
	    rmask = fbCmap->ctrl;           /* color planes */
	}
	*statePtr = (rmask & 0xff) ? 1 : 0;
	break;
    default:
	return FAILURE;
    }
    return SUCCESS;
}

/*
 *----------------------------------------------------------------------
 *
 * InitCmap --
 *
 *	Initialize the colormap.
 *
 * Results:
 *	SUCCESS or FAILURE.
 *
 * Side effects:
 *	Colormap data structures initialized.
 *
 *----------------------------------------------------------------------
 */
static ReturnStatus
InitCmap(devPtr)
    FBDevice	*devPtr;
{
    int		c;
    int		whichFb;

    whichFb = devPtr->type.fb_type;
    fbCmap = fbaddrs[whichFb].fb_cmap;

    if ((whichFb != FBTYPE_SUN4COLOR) && (whichFb != FBTYPE_SUNFAST_COLOR) &&
	    (whichFb != FBTYPE_SUN3COLOR)) {
	printf("Wrong fb type to have a colormap.\n");
	return FAILURE;
    }
    /* Init colormap copy, overlay, etc.. */
    if(whichFb == FBTYPE_SUNFAST_COLOR) {
	fbCmap->addr = 0x06 << 24;      /* command register address */
	fbCmap->ctrl = 0x70 << 24;      /* disable cursor overlay */
    } else {
	for (c = 0; c < 256 * 3 / 4; c++) {
	    fbCmapCopy.raw[c] = fbCmap->cmap;
	}
	fbCmap->addr = 0x06;            /* command register address */
	fbCmap->ctrl = 0x73;            /* enable overlay */
	/* set overlay colors: bg: blue, fg: white */
	fbCmap->addr = 0x00;
	fbCmap->omap = 0x00000000;
	fbCmap->omap = 0x00ff0000;
	fbCmap->omap = 0x00ffffff;
    }

    return SUCCESS;
}
