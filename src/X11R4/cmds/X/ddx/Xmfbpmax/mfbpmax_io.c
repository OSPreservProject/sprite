/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
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

/* $XConsortium: mfbpmax_io.c,v 1.13 90/01/19 14:56:36 keith Exp $ */
/* $Id: mfbpmax_io.c,v 1.3 91/02/19 21:34:28 kupfer Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#ifdef sprite
#include <dev/graphics.h>
#include <sys/ioctl.h>
#endif
#include <sys/tty.h> 
#include <errno.h>
#include <sys/devio.h> 
#include <machine/pmioctl.h>
#include <machine/dc7085cons.h>

#include "misc.h"
#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "input.h"
#include "cursorstr.h"
#include "regionstr.h"
#include "colormap.h"
#include "resource.h"
#include "servermd.h"
#include "dixstruct.h"

#include "mi.h"
#include "mfb.h"
#include "pm.h"


void pmQueryBestSize();

/*
 * These "statics" imply there will never be more than one "pm" display and
 * mouse attached to a server; otherwise, they would be in SCREEN private info.
 *
 * This is probably the case since the Ultrix driver does not allow one to
 * open the display without also getting at the same file descriptor that
 * handles the mouse and the keyboard. Most of it is merely to pass information
 * to the keyboard and pointer devices. This could be done more cleanly by
 * repeating the ioctl to get the PM_Info.
 */
static PM_Info *	info;
static pmEventQueue *	queue;
static pmBox *		mbox;
static pmCursor	*	mouse;
static char *		bitmap;
static pmEvent *	events;
static pmTimeCoord *	tcs;
static int		fdPM;
static int		qLimit;
static int		lastEventTime;
static DevicePtr	pmKeyboard;
static DevicePtr	pmPointer;
static int		hotX, hotY;
static BoxRec		cursorRange, cursorConstraint;
static int 		mapOnce = 0;
static int		dpix = -1, dpiy = -1, dpi = -1;
static Bool 		SpecificB, SpecificW;
static char		*blackValue, *whiteValue;

#define MAX_LED 4

/* what is this and what does it do ??? */
xColorItem		screenWhite, screenBlack;

/*
 * Statics to contain the current value for the keyboard and pointer devices
 */


/* SaveScreen does blanking, so no need to worry about the interval timer */
 
static Bool
pmSaveScreen(pScreen, on)
    ScreenPtr pScreen;
    int on;
{
    if (on != SCREEN_SAVER_ON)
    {
        lastEventTime = GetTimeInMillis();
	if (ioctl(fdPM, QIOVIDEOON) < 0)
	    ErrorF("pmSaveScreen: failed to turn screen on.\n");
    } else {
	if (ioctl(fdPM, QIOVIDEOOFF) < 0)
	    ErrorF("pmSaveScreen: failed to turn screen off.\n");
    }
    return TRUE;
}

Bool
pmScreenInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;
    char **argv;
{
    register    PixmapPtr pPixmap;
    ColormapPtr pColormap;
    VisualPtr	pVisual;
    int		i;
    void	ddxUseMsg();

    if (mapOnce)
    {
	/*
	 * The reason you need to do this is so that when the
	 * the server recycles and the system is in video off
	 * state you need something to turn the video on.  Note
	 * that unlike the vaxstar you don't do a fresh open 
	 * and close of the device and thus the video remains
	 * off if you don't explicitly turn it on.
	 */
	if (ioctl(fdPM, QIOVIDEOON) < 0)
	    ErrorF("pmSaveScreen: failed to turn screen on.\n");
    }

    if (!mapOnce) 
    {
        if ((fdPM = open("/dev/mouse", O_RDWR | O_NDELAY, 0)) < 0)
        {
	    ErrorF("couldn't open pm \n");
	    return FALSE;
        }

       if (ioctl(fdPM, QIOCGINFO, &info) < 0)
       {
	   ErrorF("error getting address of info\n");
	   close(fdPM);
	   return FALSE;
       }
        mapOnce = 1;
    }

    queue = &((PM_Info *)info)->qe;
    mbox = &((PM_Info *)info)->mbox;
    mouse = &((PM_Info *)info)->mouse;
    qLimit = queue->eSize - 1;
    events = info->qe.events;
    tcs = info->qe.tcs;
    bitmap = info->bitmap;

    /* discard all the current input events  */
    queue->eHead = queue->eTail;

    cursorRange.x1 = -15;
    cursorRange.x2 = ((PM_Info *)info)->max_x - 1;
    cursorRange.y1 = -15;
    cursorRange.y2 = ((PM_Info *)info)->max_y - 1;

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

    /* video blanking screen saver */
    pScreen->SaveScreen = pmSaveScreen;
    /* pm cursor routines */
    pScreen->RealizeCursor = pmRealizeCursor;
    pScreen->UnrealizeCursor = pmUnrealizeCursor;
    pScreen->DisplayCursor = pmDisplayCursor;
    pScreen->SetCursorPosition = pmSetCursorPosition;
    pScreen->CursorLimits = pmCursorLimits;
    pScreen->PointerNonInterestBox = pmPointerNonInterestBox;
    pScreen->ConstrainCursor = pmConstrainCursor;
    pScreen->RecolorCursor = miRecolorCursor;

    for (i = 1; i < argc; i++ )
    {
        if(strncmp(argv[i], "-bp:", 4) == 0 && atoi(argv[i] + 4) == index)
        {
	    if(++i < argc)
	    {
		blackValue = argv[i];
		SpecificB = TRUE;
	    }
	    else
		ddxUseMsg();
        }
        if(strncmp(argv[i], "-wp:", 4) == 0 && atoi(argv[i] + 4) == index)
        {
	    if(++i < argc)
	    {
		whiteValue = argv[i];
		SpecificW = TRUE;
	    }
	    else
		ddxUseMsg();
        }
    }

    pScreen->blackPixel = 0;
    pScreen->whitePixel = 1;

    if(blackValue)
    {
	if((i = atoi(blackValue)) == 0 || i == 1)
	    pScreen->blackPixel = i;
	else  
	    pmPixelError(index);
    }
    if(whiteValue)
    {
        if((i = atoi(whiteValue)) == 0 || i == 1)
    	    pScreen->whitePixel = i;
        else  
	    pmPixelError(index);
    }

    if (!mfbScreenInit(pScreen, (char *)bitmap, 
			    ((PM_Info *)info)->max_x, 
			    ((PM_Info *)info)->max_y, dpix, dpiy, 2048))
    {
	close (fdPM);
	return FALSE;
    }

    /*
     * pmQueryBestSize also knows about cursor sizes, mash
     * the mfb suplied version
     */
    pScreen->QueryBestSize = pmQueryBestSize;
    /*
     * create and install the default colormap
     */
    if (!mfbCreateDefColormap (pScreen))
    {
	close (fdPM);
	return FALSE;
    }
    return TRUE;
}

pmPixelError(index)
int	index;
{
    ErrorF("Only 0 or 1 are acceptable pixels for device %d\n", index);
}

static void
ChangeLED(led, on)
    int led;
    Bool on ;
{
    struct pm_kpcmd ioc;

    if (led > MAX_LED)    
        return;
    if (on)
        ioc.cmd = LK_LED_ENABLE;
    else
        ioc.cmd = LK_LED_DISABLE;
    if (led == 1)
	ioc.par[0] = LED_1;
    else if (led == 2)
        ioc.par[0] = LED_2;
    else if (led == 3)
        ioc.par[0] = LED_3;
    else if (led == 4)
        ioc.par[0] = LED_4;
    ioc.par[1]  = 0;
    ioc.nbytes = 1;
    ioctl(fdPM, QIOCKPCMD, &ioc);
}

static void
pmChangeKeyboardControl(device, ctrl)
    DevicePtr device;
    KeybdCtrl *ctrl;
{
#define LK_ENABLE_CLICK 0x1b	/* enable keyclick / set volume	*/
#define LK_DISABLE_CLICK 0x99	/* disable keyclick entirely	*/
#define LK_ENABLE_BELL 0x23	/* enable bell / set volume 	*/

    struct pm_kpcmd ioc;
    int i;

    if (ctrl->click == 0)    /* turn click off */
    {
	ioc.nbytes = 0;
	ioc.cmd = LK_DISABLE_CLICK;
	ioctl(fdPM, QIOCKPCMD, &ioc);
    }
    else 
    {
        int volume;

        volume = 7 - ((ctrl->click / 14) & 7);
	ioc.nbytes = 1;
	ioc.cmd = LK_ENABLE_CLICK;
	ioc.par[0] = volume;
	ioctl(fdPM, QIOCKPCMD, &ioc);
    }

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
pmBell(loud, pDevice)
    int loud;
    DevicePtr pDevice;
{
    struct pm_kpcmd ioc;

/* the lk201 volume is between 7 (quiet but audible) and 0 (loud) */
    loud = 7 - ((loud / 14) & 7);
    ioc.nbytes = 1;
    ioc.cmd = LK_BELL_ENABLE;
    ioc.par[0] = loud;
    ioctl(fdPM, QIOCKPCMD, &ioc);

    ioc.nbytes = 0;
    ioc.cmd = LK_RING_BELL;
    ioctl(fdPM, QIOCKPCMD, &ioc);
}

/*
 * These serve protocol requests, setting/getting acceleration and threshold.
 * X10 analog is "SetMouseCharacteristics".
 */
static void
pmChangePointerControl(device, ctrl)
    DevicePtr device;
    PtrCtrl   *ctrl;
{
    ((PM_Info *)info)->mthreshold = ctrl->threshold;
    if (!(((PM_Info *)info)->mscale = ctrl->num / ctrl->den))
	((PM_Info *)info)->mscale = 1;	/* watch for den > num */
}

static int
pmGetMotionEvents(pDevice, buff, start, stop)
    CARD32 start, stop;
    DevicePtr pDevice;
    xTimecoord *buff;
{
    int count = 0;
    int tcFirst = queue->tcNext;
    int tcLast = (tcFirst) ? tcFirst - 1 : queue->tcSize - 1;
    register pmTimeCoord *Tcs;
    int i;

    for (i == tcFirst; ; i++)
    {
    if (i = queue->tcSize)
	i = 0;
    Tcs = &((pmTimeCoord *)tcs)[i];
    if ((start <= Tcs->time) && (Tcs->time <= stop))
    {
	buff[count].time = Tcs->time;
	buff[count].x = Tcs->x;
	buff[count].y = Tcs->y;
	count++;
    }
    if (i == tcLast)
	return count;
    }
}

int
pmMouseProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    int     i;
    BYTE    map[4];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    pmPointer = pDev;
	    pDev->devicePrivate = (pointer) &queue;
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		pmPointer, map, 3, pmGetMotionEvents, pmChangePointerControl,
		MOTION_BUFFER_SIZE);
	    SetInputCheck(&queue->eHead, &queue->eTail);
	    hotX = hotY = 0;
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice(fdPM);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
	    RemoveEnabledDevice(fdPM);
	    break;
	case DEVICE_CLOSE:
	    break;
    }
    return Success;
}

#define LK_REPEAT_ON  0xe3
#define LK_REPEAT_OFF 0xe1

int
SetLKAutoRepeat (onoff)
    Bool onoff;
{
    extern char *AutoRepeatLKMode();
    extern char *UpDownLKMode();
    
    struct pm_kpcmd ioc;
    register char  *divsets;
    divsets = onoff ? (char *) AutoRepeatLKMode() : (char *) UpDownLKMode();
    ioc.nbytes = 0;
    while (ioc.cmd = *divsets++)
	ioctl(fdPM, QIOCKPCMD, &ioc);
    ioc.cmd = ((onoff > 0) ? LK_REPEAT_ON : LK_REPEAT_OFF);
    return(ioctl(fdPM, QIOCKPCMD, &ioc));
}

int
pmKeybdProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    KeySymsRec keySyms;
    CARD8 modMap[MAP_LENGTH];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    pmKeyboard = pDev;
	    pDev->devicePrivate = (pointer) & queue;
	    GetLK201Mappings( &keySyms, modMap);
	    InitKeyboardDeviceStruct(
		    pmKeyboard, &keySyms, modMap, pmBell,
		    pmChangeKeyboardControl);
	    
	    /* Free the key sym mapping space allocated by GetLK201Mappings. */
	    Xfree(keySyms.map);

	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice(fdPM);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
	    RemoveEnabledDevice(fdPM);
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}

/*
 * The driver has been set up to put events in the queue that are identical
 * in shape to the events that the DDX layer has to deliver to ProcessInput
 * in DIX.
 */
extern int screenIsSaved;

void
ProcessInputEvents()
{
#define DEVICE_KEYBOARD 2
    pmEvent e;
    xEvent x;
    register int    i;

    i = queue->eHead;
    while (i != queue->eTail)
    {
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	e = *((pmEvent *) & ((pmEvent *)events)[i]);
	x.u.keyButtonPointer.rootX = e.x + hotX;
	x.u.keyButtonPointer.rootY = e.y + hotY;
	x.u.keyButtonPointer.time = lastEventTime = e.time;
	x.u.u.detail = e.key;

	if (e.device == DEVICE_KEYBOARD)
	{
	    switch (e.type)
	    {
		case BUTTON_DOWN_TYPE: 
		    x.u.u.type = KeyPress;
		    (*pmKeyboard->processInputProc) (&x, pmKeyboard, 1);
		    break;
		case BUTTON_UP_TYPE: 
		    x.u.u.type = KeyRelease;
		    (*pmKeyboard->processInputProc) (&x, pmKeyboard, 1);
		    break;
		default: 	       /* hopefully BUTTON_RAW_TYPE */
		    ProcessLK201Input(&x, pmKeyboard);
	    }
	}
	else
	{
	    switch (e.type)
	    {
		case BUTTON_DOWN_TYPE: 
		    x.u.u.type = ButtonPress;
		    break;
		case BUTTON_UP_TYPE: 
		    x.u.u.type = ButtonRelease;
		    break;
		case MOTION_TYPE: 
		    x.u.u.type = MotionNotify;
		    break;
		default: 
		    FatalError("Unknown event from mouse driver");
	    }
	    (*pmPointer->processInputProc) (&x, pmPointer, 1);
	}

	if (i == qLimit){
	    i = queue->eHead = 0;
	}else{
	    i = ++queue->eHead;
	}
    }
#undef DEVICE_KEYBOARD
}

TimeSinceLastInputEvent()
{
    if (lastEventTime == 0)
	lastEventTime = GetTimeInMillis();
    return GetTimeInMillis() - lastEventTime;
}

/*
 * set the bounds in the device for this particular cursor
 */
static void
pmConstrainCursor( pScr, pBox)
    ScreenPtr	pScr;
    BoxPtr	pBox;
{
    cursorConstraint = *pBox;
    if (((PM_Info *)info))
    {
	((PM_Info *)info)->max_cur_x = pBox->x2 - hotX - 1;
	((PM_Info *)info)->max_cur_y = pBox->y2 - hotY - 1;
	((PM_Info *)info)->min_cur_x = pBox->x1 - hotX;
	((PM_Info *)info)->min_cur_y = pBox->y1 - hotY;
    }
}

static Bool
pmSetCursorPosition( pScr, newx, newy, generateEvent)
    ScreenPtr		pScr;
    unsigned int	newx, newy;
    Bool		generateEvent;
{
    pmCursor	pmCPos;
    xEvent	motion;

    pmCPos.x = (short)((int)newx - hotX);
    pmCPos.y = (short)((int)newy - hotY);

    if ( ioctl(fdPM, QIOCPMSTATE, &pmCPos) < 0)
    {
	ErrorF( "error warping cursor\n");
	return FALSE;
    }
    if (generateEvent)
    {
	if (queue->eHead != queue->eTail)
	    ProcessInputEvents();
	motion.u.keyButtonPointer.rootX = newx;
	motion.u.keyButtonPointer.rootY = newy;
	motion.u.keyButtonPointer.time = currentTime.milliseconds;
	motion.u.u.type = MotionNotify;
	(*pmPointer->processInputProc) (&motion, pmPointer, 1);
    }

    return TRUE;
}

static Bool
pmDisplayCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    int		x, y;
    /*
     * load the cursor
     */
    if ((hotX != pCurs->bits->xhot) || (hotY != pCurs->bits->yhot))
    {
	x = (int)mouse->x + hotX;
	y = (int)mouse->y + hotY;
	hotX = pCurs->bits->xhot;
	hotY = pCurs->bits->yhot;
	pmSetCursorPosition(pScr, x, y, FALSE);
	pmConstrainCursor(pScr, &cursorConstraint);
		/* to update constraints in driver */
    }
    if ( ioctl( fdPM, QIOWCURSOR, pCurs->devPriv[ pScr->myNum]) < 0)
    {
	ErrorF( "error loading bits of new cursor\n");
        return FALSE;
    }
    return TRUE;
}

/*
 */
static Bool
pmRealizeCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;	/* a SERVER-DEPENDENT cursor */
{
    int	forecolor =
	 ( pCurs->foreRed || pCurs->foreGreen || pCurs->foreBlue)?~0:0;
    int	backcolor = ~forecolor;
    register short	*a, *b;	/* vaxstar-defined */
    register int *	mask;	/* server-defined */
    register int *	src;	/* server-defined */
    int		i;
    int		cursorBytes = 32*sizeof(unsigned short);
    int		lastRow = ((pCurs->bits->height < 16) ? pCurs->bits->height : 16);
    register unsigned short widthmask = (1<<pCurs->bits->width)-1;
				/* used to mask off beyond the edge of the
				   real mask and source bits
				*/

    pCurs->devPriv[ pScr->myNum] = (pointer)Xalloc(cursorBytes);
    bzero((char *)pCurs->devPriv[ pScr->myNum], cursorBytes);

    /*
     * munge the SERVER-DEPENDENT, device-independent cursor bits into
     * what the device wants, which is 32 contiguous shorts.
     *
     * cursor hardware has "A" and "B" bitmaps
     * logic table is:
     *
     *		A	B	cursor
     *
     *		0	0	transparent
     *		1	0	xor (not used)
     *		0	1	black
     *		1	1	white
     */

    /*
     * "a" bitmap = image 
     */
    /*
     * "b" bitmap can be same as "mask", providing "a" is never on when
     *  "b" is off.
     */
    for ( i=0,
	  a = (short *)pCurs->devPriv[pScr->myNum],
	  b = ((short *)pCurs->devPriv[pScr->myNum]) + 16,
	/* XXX assumes server bitmap pad is size of int, 
	   and cursor is < 32 bits wide */
	  src = (int *)pCurs->bits->source,
	  mask = (int *)pCurs->bits->mask;

	  i < lastRow;

	  i++, a++, b++, src++, mask++)
    {
	*a = ((*src & forecolor) | (~*src & backcolor)) & *mask;
	*b = *mask;
	*a &= widthmask;
	*b &= widthmask;
    }
    return TRUE;
}

static Bool
pmUnrealizeCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    Xfree( pCurs->devPriv[ pScr->myNum]);
    return TRUE;
}

static void
pmPointerNonInterestBox( pScr, pBox)
    ScreenPtr	pScr;
    BoxPtr	pBox;
{
    ((PM_Info *)info)->mbox.bottom = pBox->y2;
    ((PM_Info *)info)->mbox.top = pBox->y1;
    ((PM_Info *)info)->mbox.left = pBox->x1;
    ((PM_Info *)info)->mbox.right = pBox->x2;
}

/*
 * pm cursor top-left corner can now go to negative coordinates
 */
static void
pmCursorLimits( pScr, pCurs, pHotBox, pPhysBox)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
    BoxPtr	pHotBox;
    BoxPtr	pPhysBox;	/* return value */
{
    pPhysBox->x1 = max( pHotBox->x1, cursorRange.x1 + (int) pCurs->bits->xhot);
    pPhysBox->y1 = max( pHotBox->y1, cursorRange.y1 + (int) pCurs->bits->yhot);
    pPhysBox->x2 = min( pHotBox->x2, cursorRange.x2 + 1);
    pPhysBox->y2 = min( pHotBox->y2, cursorRange.y2 + 1);
}

static void
pmQueryBestSize(class, pwidth, pheight)
    int class;
    short *pwidth;
    short *pheight;
{
    unsigned width, test;

    if (*pwidth > 0)
    {
      switch(class)
      {
        case CursorShape:
	  *pwidth = 16;
	  *pheight = 16;
	  break;
        case TileShape:
        case StippleShape:
	  width = *pwidth;
	  /* Return the closes power of two not less than what they gave me */
	  test = 0x80000000;
	  /* Find the highest 1 bit in the width given */
	  while(!(test & width))
	     test >>= 1;
	  /* If their number is greater than that, bump up to the next
	   *  power of two */
	  if((test - 1) & width)
	     test <<= 1;
	  *pwidth = test;
	  /* We don't care what height they use */
	  break;
       }
    }
}

SetLockLED (on)
    Bool on;
    {
    struct pm_kpcmd ioc;
    ioc.cmd = on ? LK_LED_ENABLE : LK_LED_DISABLE;
    ioc.par[0] = LED_3;
    ioc.par[1] = 0;
    ioc.nbytes = 1;
    ioctl(fdPM, QIOCKPCMD, &ioc);
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
    int argc;
    char *argv[];
    int i;
{
    int			argind=i;
    int			skip;
    static int		Once=0;
    void		ddxUseMsg();

    skip = 0;
    if (!Once)
    {
        blackValue = NULL;
        SpecificB = FALSE;
        whiteValue = NULL;
        SpecificW = FALSE;
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
	{
	    ddxUseMsg();
	    skip = 1;
	}
    }
    else if (strcmp( argv[argind], "-dpiy") == 0)
    {
	if (++argind < argc)
	{
	    dpiy = atoi(argv[argind]);
	    skip = 2;
	}
	else
	{
	    ddxUseMsg();
	    skip = 1;
	}
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
	{
	    ddxUseMsg();
	    skip = 1;
	}
    }
    else 
    if(strcmp(argv[argind], "-bp") == 0 && !SpecificB)
    {
	    if(++argind < argc)
	    {
		blackValue = argv[argind];
		skip = 2;
	    }
	    else
	    {
		ddxUseMsg();
		skip = 1;
	    }
    }
    else
    if(strcmp(argv[argind], "-wp") == 0 && !SpecificW)
    {
	    if(++argind < argc)
	    {
		whiteValue = argv[argind];
		skip = 2;
	    }
	    else
	    {
		ddxUseMsg();
		skip = 1;
	    }
    }
       
    return skip;

}

void
ddxUseMsg()
{
    ErrorF ("Device Dependent Usage\n");
    ErrorF ("-dpi #		Dots per inch, x and y coordinates\n");
    ErrorF ("-dpix #		Dots per inch, x coordinate\n");
    ErrorF ("-dpiy #		Dots per inch, y coordinate\n");
    ErrorF ("-bp color BlackPixel for screen\n");
    ErrorF ("-wp color BlackPixel for screen\n");
}
