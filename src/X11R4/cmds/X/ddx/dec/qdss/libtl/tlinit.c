/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
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

/*

CAUTION:  the 4.3bsd optimizer may break this code if it is changed,
	  as the GPX registers must be accessed with word instructions.
	  By various accidents, the optimizer does not currently
	  replace the boolean operations with byte instructions, but
	  forewarned is forearmed!

	  -R. Swick, per R. Drewry

*/

#include	<sys/types.h>
#include	<sys/file.h>
#include	"Ultrix2.0inc.h"
/* ifdef uVaxII/GPX */
#include	<vaxuba/qduser.h>
#include	<vaxuba/qdreg.h>
#include	<vaxuba/qdioctl.h>
/* endif - uVaxII/GPX */

/* else  Vaxstar */
#include        "tlsg.h"
/* endif - Vaxstar */

#include	"tl.h"

#define QDSS      "/dev/qd"
#define SG	  "/dev/sg"

#ifdef WAKE_USERDMA
static short newwakeflags = WAKE_USERDMA | WAKE_QUEUEEMPTY;
#endif

unsigned     int    Vaxstar = 0; /* Set this flag if Vaxstar */
short * sg_int_flag;
short * change_section;
u_short * sg_next_fifo;

extern char *display;	/* initialized in server/dix/main.c and updated from
			   the command line in server/os/4.2bsd/utils.c */

extern int              Nentries;
extern int	        Nplanes;
extern int	        Nchannels;
extern unsigned	int	Allplanes;

/* uVaxII/GPX functions */
extern int	Enable_dma ();
extern int	Get_dma();
extern int	Flush_dma();
/* endif -  uVaxII/GPX functions */

/* Vaxstar/GPX functions */
extern int	Enable_fifo ();
extern int      Get_fifo();
extern int	Flush_fifo();
/* endif - Vaxstar/GPX functions */

static void InitDragon();

extern 	dft_struct dmafxns; /* Initialize this upon determining device */

tlInit()
{
    register int i;

    /* ifdef Vaxstar */
    u_short save_cbcsr;
    /* endif - Vaxstar */

    char qdssDev[16];

    strcpy (qdssDev, QDSS);
    strcat (qdssDev, display);

    fd_qdss = open( qdssDev, O_RDWR);
    if ( fd_qdss >= 0) 
	Vaxstar = 0;
    else{ /* Alternate device */
    	strcpy (qdssDev, SG);
    	strcat (qdssDev, "0"); /* no muc's for sg */
    	fd_qdss = open (qdssDev, O_RDWR);
    	if (fd_qdss < 0){
    		ErrorF ("Neither a qdss nor a sg device\n");
    		return -1;
    	}
    	Vaxstar = 1;
    }

    /* Set up Function definitions */
    /* Enable_dma/Enable_fifo      */
    /* Get_dma/Get_fifo	       */
    /* Flush_dma/Flush_fifo	       */
    if (!Vaxstar)	{
    	dmafxns.enable  = Enable_dma;
    	dmafxns.get     = Get_dma;
    	dmafxns.flush   = Flush_dma;
    }else{
    	dmafxns.enable  = Enable_fifo;
    	dmafxns.get     = Get_fifo;
    	dmafxns.flush   = Flush_fifo;
    }

    Nplanes = QzQdGetNumPlanes();
    Nentries = ((Nplanes == 4) ?  4 : 8);
    Nchannels = Nplanes/Nentries;
    Allplanes = (1 << Nplanes) - 1;

    /* Get access to device registers and memory space */
    if (!Vaxstar){
        if (ioctl (fd_qdss, QD_MAPDEVICE, &Qdss) == -1) {
       	    ErrorF("Couldn't get QDSS device map.\n");
            close (fd_qdss);
    	    return -1;
        }
    }else{
	if (ioctl (fd_qdss, SG_MAPDEVICE, &Sg) == -1) {
	    ErrorF ("Couldn't get SG device map.\n");
	    close (fd_qdss);
	    return -1;
	}
    }

    if (!Vaxstar) {
        Adder    = (struct adder *)Qdss.adder;
        Dga      = (struct dga *)Qdss.dga;
        Duart    = (struct duart *)Qdss.duart;
        Template = (char *)Qdss.template;
        Memcsr   = (char *)Qdss.memcsr;
        Redmap   = (char *)Qdss.red;
        Bluemap  = (char *)Qdss.blue;
        Greenmap = (char *)Qdss.green;
    }else{
	Adder    = (struct adder *)Sg.adder;
	Fcc      = (char *)Sg.fcc;
	Vdac     = (char *)Sg.vdac;
	Cur      = (char *)Sg.cur;
	Vrback   = (char *)Sg.vrback;
	Fiforam  = (char *)Sg.fiforam;
	Redmap   = (char *)Sg.red;
	Bluemap  = (char *)Sg.blue;
	Greenmap = (char *)Sg.green;
    }	

    /* Clear Screen and set up hardware and software defaults */
    ioctl (fd_qdss, QD_SET);
    ioctl (fd_qdss, QD_CLRSCRN);

    /* Get access to shared DMA buffer */
    if (!Vaxstar) {
        if (ioctl (fd_qdss, QD_MAPIOBUF, &DMAheader) == -1) {
            ErrorF("InitQdss: Couldn't get DMA buffer.\n");
            close (fd_qdss);
    	    return -1;
        }
    }else{
        if (ioctl (fd_qdss, QD_MAPIOBUF, &FIFOheader) == -1) {
            ErrorF("InitSg: Couldn't get FIFO buffer.\n");
            close (fd_qdss);
    	    return -1;
        }
    }

    AdderPtr = (unsigned short *) Adder;


    /* put the adder in a reasonable state.
       NOTE: this is dangerous; it goes and stomps on the adder 
       registers to make sure DMA can start.  if all the bits
       in the request_enable register are 0, no DMA will ever occur.
       Wait for the ADDRESS_COMPLETE bit to be set in the status
       register, and then turn it on in the request_enable
       register, which makes the magic ANDs and multi-input OR
       come true.
       As a fail-safe, we force ADDRESS_COMPLETE true with a
       cancel command, since nothing should be going on now
       anyway.
    */

    Adder->command = CANCEL;
    /* NOTE some compilers will optimize this out... */
    while ((Adder->status & ADDRESS_COMPLETE) == 0)
        ;
    Adder->request_enable |= ADDRESS_COMPLETE;

    /* initialize the registers */
    /* Treat initialization of 4 planes and 8 planes distinctly */
    InitDragon(Nplanes);

    if (!Vaxstar)
        LoadTemplate(Template);
    else{
	save_cbcsr = (u_short)(((struct fcc *)Fcc)->cbcsr);
	((struct fcc *)Fcc)->cbcsr |= 3;
	LoadTemplate(Fiforam);
	((struct fcc *)Fcc)->cbcsr =(ushort) save_cbcsr;
    }
	
    /* make sure select doesn't wake us as often */
#ifdef QD_GETWAKEUP
    ioctl(fd_qdss, QD_GETWAKEUP, &oldwakeflags);
    ioctl(fd_qdss, QD_SETWAKEUP, &newwakeflags);
#endif

    (dmafxns.enable)();
    return fd_qdss;
}


/*
    since we use QD_SET (so as not to clear the screen) in InitQdss()
    we need to initialize the addre and viper registers ourselves.
    this could be a template routine.
InitDragon(), wait_status(), and write_ID() are lifted from the
qdss device driver, with minor modifications.
*/


#define ADDER_GOOD 0
#define ADDER_BAD  1
static void
InitDragon(numplanes)
	int numplanes;
{

	register VOLATILE struct adder *adder;
	register struct dga *dga;
	short *memcsr;

	register int i;		/* general purpose variables */
	int status;

	short top;		/* clipping/scrolling boundaries */
	short bottom;
	short right;
	short left;

	adder = Adder;
	if (!Vaxstar){
	    dga = Dga;
	    memcsr = (short *)Memcsr;
	
	    dga->csr &= ~(DMA_IE | 0x700);/* halt DMA and kill the intrpts */
	    *memcsr = SYNC_ON;	/* blank screen and turn off LED's */
	}else{
	    change_section = 
		(short *)
		&(((struct FIFOreq_header *)FIFOheader)->change_section);
	    sg_int_flag = 
		(short *)
		&(((struct FIFOreq_header *)FIFOheader)->sg_int_flag);
	    sg_next_fifo = 
		(unsigned short *)
		&(((struct FIFOreq_header *)FIFOheader)->sg_next_fifo);
	    fcc_cbcsr = (struct fcc *)Fcc;
	    Fcc = (char *)fcc_cbcsr;
	    SG_Fifo = (unsigned short *)Fiforam;
            DMAdev_reg = (short *)Vdac;
	    FIFOheader = 0;
	    FIFOflag = 0;
	    ((struct fcc *)Fcc)->cbcsr &=  ~3;
        }
	adder->command = CANCEL;

/* set monitor timing */

	adder->x_scan_count_0 = 0x2800;
	adder->x_scan_count_1 = 0x1020;
	adder->x_scan_count_2 = 0x003A;
	adder->x_scan_count_3 = 0x38F0;
	adder->x_scan_count_4 = 0x6128;
	adder->x_scan_count_5 = 0x093A;
	adder->x_scan_count_6 = 0x313C;
	adder->sync_phase_adj = 0x0100;
	adder->x_scan_conf = 0x00C8;

/* got a bug in secound pass ADDER! lets take care of it */

	/* normally, just use the code in the following bug fix code, but to 
	* make repeated demos look pretty, load the registers as if there was
	* no bug and then test to see if we are getting sync */

	adder->y_scan_count_0 = 0x135F;
	adder->y_scan_count_1 = 0x3363;
	adder->y_scan_count_2 = 0x2366;
	adder->y_scan_count_3 = 0x0388;

	/* if no sync, do the bug fix code */

	if (wait_status(adder, VSYNC) == ADDER_BAD) {

	    /* first load all Y scan registers with very short frame and
	    * wait for scroll service.  This guarantees at least one SYNC 
	    * to fix the pass 2 Adder initialization bug (synchronizes
	    * XCINCH with DMSEEDH) */

	    adder->y_scan_count_0 = 0x01;
	    adder->y_scan_count_1 = 0x01;
	    adder->y_scan_count_2 = 0x01;
	    adder->y_scan_count_3 = 0x01;

	    wait_status(adder, VSYNC);	/* delay at least 1 full frame time */
	    wait_status(adder, VSYNC);

	    /* now load the REAL sync values (in reverse order just to
	    *  be safe.  */

	    adder->y_scan_count_3 = 0x0388;
	    adder->y_scan_count_2 = 0x2366;
	    adder->y_scan_count_1 = 0x3363;
	    adder->y_scan_count_0 = 0x135F;
	}
	adder->y_limit = 1728;

	if (!Vaxstar)
		/* turn off leds and turn on video */
		*memcsr = SYNC_ON | UNBLANK;	
	/* ifdef Vaxstar ??? */

/* zero the index registers */

	adder->x_index_pending = 0;
	adder->y_index_pending = 0;
	adder->x_index_new = 0;
	adder->y_index_new = 0;
	adder->x_index_old = 0;
	adder->y_index_old = 0;
	adder->pause = 0;

/* scale factor = unity */
	adder->fast_scale = UNITY;
	adder->slow_scale = UNITY;

/* initialize plane addresses for vipers */
	if (numplanes == 24) {
	    write_ID(adder, RED_UPDATE, 0x0);
	    write_ID(adder, BLUE_UPDATE, 0x0);
	    for (i = 0; i < Nentries; i++) {
		write_ID(adder, GREEN_UPDATE, (1 << i));
		write_ID(adder, PLANE_ADDRESS, i | ZBLOCK_GREEN);
	    }

	    write_ID(adder, GREEN_UPDATE, 0x0);
	    for (i = 0; i < Nentries; i++) {
		write_ID(adder, RED_UPDATE, (1 << i));
		write_ID(adder, PLANE_ADDRESS, i | ZBLOCK_RED);
	    }

	    write_ID(adder, RED_UPDATE, 0x0);
	    for (i = 0; i < Nentries; i++) {
		write_ID(adder, BLUE_UPDATE, (1 << i));
		write_ID(adder, PLANE_ADDRESS, i | ZBLOCK_BLUE);
	    }

	    /* initialize the external registers. */
	    write_ID(adder, RED_UPDATE, ((1 << Nentries) - 1));
	    write_ID(adder, RED_SCROLL, ((1 << Nentries) - 1)); 
	    write_ID(adder, GREEN_UPDATE, ((1 << Nentries) - 1));
	    write_ID(adder, GREEN_SCROLL, ((1 << Nentries) - 1)); 
	    write_ID(adder, BLUE_UPDATE, ((1 << Nentries) - 1));
	    write_ID(adder, BLUE_SCROLL, ((1 << Nentries) - 1)); 
	}
	else {
/*
 *  Make a 24 bit system look like an 8/4 bit system
 */
#ifdef COMPATIBILITY
	    write_ID(adder, RED_UPDATE, 0x0);
	    write_ID(adder, BLUE_UPDATE, 0x0);
	    for (i = 0; i < Nentries; i++) {
		write_ID(adder, GREEN_UPDATE, (1 << i));
		write_ID(adder, PLANE_ADDRESS, i | ZBLOCK_GREEN);
	    }

	    write_ID(adder, GREEN_UPDATE, 0x0);
	    for (i = 0; i < Nentries; i++) {
		write_ID(adder, RED_UPDATE, (1 << i));
		write_ID(adder, PLANE_ADDRESS, i | ZBLOCK_GREEN);
	    }

	    write_ID(adder, RED_UPDATE, 0x0);
	    for (i = 0; i < Nentries; i++) {
		write_ID(adder, BLUE_UPDATE, (1 << i));
		write_ID(adder, PLANE_ADDRESS, i | ZBLOCK_GREEN);
	    }

	    /* initialize the external registers. */
	    write_ID(adder, RED_UPDATE, ((1 << Nentries) - 1));
	    write_ID(adder, RED_SCROLL, ((1 << Nentries) - 1)); 
	    write_ID(adder, GREEN_UPDATE, ((1 << Nentries) - 1));
	    write_ID(adder, GREEN_SCROLL, ((1 << Nentries) - 1)); 
	    write_ID(adder, BLUE_UPDATE, ((1 << Nentries) - 1));
	    write_ID(adder, BLUE_SCROLL, ((1 << Nentries) - 1)); 
#else
	    for (i = 0; i < Nentries; i++) {
		write_ID(adder, GREEN_UPDATE, (1 << i));
		write_ID(adder, PLANE_ADDRESS, i | ZBLOCK_GREEN);
	    }
	    write_ID(adder, GREEN_UPDATE, ((1 << Nentries) - 1));
	    write_ID(adder, GREEN_SCROLL, ((1 << Nentries) - 1)); 
#endif
	}

	/* initialize viper Z registers */
	for (i=0; i<Nchannels; i++)
	    Viper_regz(FOREGROUND_COLOR_Z, 0xffff, i);
	for (i=0; i<Nchannels; i++)
	    Viper_regz(BACKGROUND_COLOR_Z, 0x0000, i);
	for (i=0; i<Nchannels; i++)
	    Viper_regz(SOURCE_Z, 0xffff, i);
	for (i=0; i<Nchannels; i++)
	    Viper_regz(SCROLL_FILL_Z, 0xffff, i);


	/* initialize resolution mode */
	write_ID(adder, MEMORY_BUS_WIDTH, 0x000C);     /* bus width = 16 */
	write_ID(adder, RESOLUTION_MODE, 0x0000);      /* one bit/pixel */

	/* initialize ocr registers to normal mode */
	write_ID(adder, SRC1_OCR_A, EXT_NONE|INT_SOURCE|NO_ID|BAR_SHIFT_DELAY);
	write_ID(adder, SRC2_OCR_A, EXT_NONE|INT_SOURCE|NO_ID|BAR_SHIFT_DELAY);
	write_ID(adder, DST_OCR_A, EXT_NONE|INT_NONE|NO_ID|NO_BAR_SHIFT_DELAY);
	write_ID(adder, DST_OCR_B, EXT_NONE|INT_NONE|NO_ID|NO_BAR_SHIFT_DELAY);

	/* initialize viper registers */
	write_ID(adder, SCROLL_CONSTANT, SCROLL_ENABLE|VIPER_LEFT|VIPER_UP);
	write_ID(adder, SCROLL_FILL, 0x0000);
	write_ID(adder, RIGHT_SCROLL_MASK, 0x0000);

	/* PTOBXY uses LF_R1 */
	write_ID(adder, LU_FUNCTION_R1, FULL_SRC_RESOLUTION | LF_SOURCE);
	write_ID(adder, LU_FUNCTION_R2, FULL_SRC_RESOLUTION | LF_SOURCE);
	write_ID(adder, LU_FUNCTION_R3, FULL_SRC_RESOLUTION | LF_SOURCE);
	write_ID(adder, LU_FUNCTION_R4, FULL_SRC_RESOLUTION | LF_SOURCE);

        /* set clipping and scrolling limits to full screen */
	for ( i = 1000, adder->status = 0
	    ; i > 0  &&  !((status = adder->status) & ADDRESS_COMPLETE)
	    ; --i);

	if (i == 0)
	    ErrorF("InitDragon: timeout on ADDRESS_COMPLETE\n");

	top = 0;
	bottom = 2048;
	left = 0;
	right = 1024;

	adder->x_clip_min = left;
	adder->x_clip_max = right;
	adder->y_clip_min = -864 & 0x3fff;	/* bmk's black arts */
	adder->y_clip_max = bottom;

	adder->scroll_x_min = left;
	adder->scroll_x_max = right;
	adder->scroll_y_min = top;
	adder->scroll_y_max = bottom;

	wait_status(adder, VSYNC);	/* wait at LEAST 1 full frame */
	wait_status(adder, VSYNC);

	adder->x_index_pending = left;
	adder->y_index_pending = top;
	adder->x_index_new = left;
	adder->y_index_new = top;
	adder->x_index_old = left;
	adder->y_index_old = top;

	adder->error_1 = 0;
	adder->error_2 = 0;
	adder->fast_dest_dy = 0;
	adder->slow_dest_dx = 0;
	adder->rasterop_mode = DST_WRITE_ENABLE|DST_INDEX_ENABLE|NORMAL;

	for ( i = 1000, adder->status = 0
	    ; i > 0  &&  !((status = adder->status) & ADDRESS_COMPLETE)
	    ; --i);

	if (i == 0)
	    ErrorF("InitDragon: timeout on ADDRESS_COMPLETE\n");

	write_ID(adder, LEFT_SCROLL_MASK, 0x0000);
	write_ID(adder, RIGHT_SCROLL_MASK, 0x0000);
	write_ID(adder, MASK_2, 0xFFFF);
	write_ID(adder, MASK_1, 0xFFFF);
}

void
poll_status(adder, mask)
    register struct adder *adder;
    register int mask;
{
    POLL_STATUS(adder, mask);
}

wait_status(adder, mask)
    register VOLATILE struct adder *adder;
    register int mask;
{
	register short status;
	int i;

	for ( i = 10000, adder->status = 0
	    ; i > 0  &&  !((status = adder->status) & mask)
	    ; --i);

	if (i == 0)
	    return(ADDER_BAD);
	else
	    return(ADDER_GOOD);

}

write_ID(adder, address, data)
    register VOLATILE struct adder *adder;
    register short address;
    register short data;
{
	register int	i;
	register VOLATILE unsigned short 	*status;

	i = 100000;
	status = &(Adder->status);

	/*
	 * WARNING: Before you change the next line, remember to
	 *	check the code that the compiler generates to 
	 *	make sure it's reading the status register each
	 *	time through the loop.  Certain C compiers
	 *	generate code which loads the value into
	 *	a register once, and never actually read it 
	 *	thereafter.
	 */
	while ( ((*status & (ADDRESS_COMPLETE | TX_READY)) != 
		(ADDRESS_COMPLETE | TX_READY))
		&& (i > 0)
	      )
	     	i--;

	if (!(i))
	{
		ErrorF("Timed out in write_ID: 0x%x\n", address);
	}

	adder->id_data = data;
	adder->command = ID_LOAD | address;
}

Viper_regz (address, data, zblock)
short address;
short data;
short zblock;
{
	register int	i;
	register VOLATILE unsigned short 	*status;

	i = 100000;
	status = &(Adder->status);

	while ( ((*status & (ADDRESS_COMPLETE | TX_READY)) != 
		(ADDRESS_COMPLETE | TX_READY))
		&& (i > 0)
	      )
	     	i--;

	if (!(i))
	{
		ErrorF("Timed out in Viper Z Load.\n");
	}

	Adder->id_data = data;
	Adder->command = (VIPER_Z_LOAD | address | zblock);
}

tlCleanup()
{
    extern int errno;

    dmafxns.flush (TRUE);

    errno = 0;
    ioctl( fd_qdss, QD_KERN_UNLOOP);
#ifdef QD_SETWAKEUP
    ioctl( fd_qdss, QD_SETWAKEUP, &oldwakeflags);
#endif
    close( fd_qdss);
    return errno;
}

#ifdef DEBUG
int GetAdder(reg) /* only works for the few readable registers */
{
    register VOLATILE unsigned short *adder = AdderPtr;
    return (int)adder[reg];
}
int SetAdder(reg, val)
{
    register VOLATILE unsigned short *adder = AdderPtr;
    adder[reg] = (unsigned short)val;
}
#endif

