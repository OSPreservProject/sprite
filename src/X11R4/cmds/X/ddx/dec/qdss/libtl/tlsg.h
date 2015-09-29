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


#include <vaxuba/qevent.h>

/*
 * VAXstar device map
 */

	struct sgmap {			/* map of register blocks in VAXstar */

	    char *adder;
	    char *fcc;
	    char *vdac;
	    char *cur;
	    char *vrback;
	    char *fiforam;
	    char *red;
	    char *blue;
	    char *green;
	};


/*
 * constants used in font operations
 */

#define CHARS		95			/* # of chars in the font */
#define CHAR_HEIGHT	15			/* char height in pixels */
#define CHAR_WIDTH	8			/* char width in pixels*/
#define FONT_WIDTH	(CHAR_WIDTH * CHARS)	/* font width in pixels */
#define FONT_HEIGHT	32
#define FONT_X		0			/* font's off screen adrs */
#define FONT_Y		(2048 - CHAR_HEIGHT)
#define ROWS		CHAR_HEIGHT

/*
 * constants used for cursor
 */

#define	MAX_CUR_X	1024		/* max cursor x position */
#define	MAX_CUR_Y	864		/* max cursor y position */
#define ACC_OFF 	0x01		/* acceleration is inactive */
#define	CURS_ENB	0x0001		/* cursor on */

/*
 * VAXstar interrupt controller register bits
 *
 */

#define SINT_VS		0x0004
#define SINT_VF		0x0008

/*
 * Cursor Command Register bits
 *
 */

#define	ENPA	0000001
#define	FOPA	0000002
#define	ENPB	0000004
#define	FOPB	0000010
#define	XHAIR 	0000020
#define	XHCLP	0000040
#define	XHCL1	0000100
#define	XHWID	0000200
#define	ENRG1	0000400
#define	FORG1	0001000
#define	ENRG2	0002000
#define	FORG2	0004000
#define	LODSA	0010000
#define	VBHI	0020000
#define	HSHI	0040000
#define	TEST	0100000

/*
 * Line Prameter Register bits
 *
 */

#define	SER_KBD      000000
#define	SER_POINTER  000001
#define	SER_COMLINE  000002
#define	SER_PRINTER  000003
#define	SER_CHARW    000030
#define	SER_STOP     000040
#define	SER_PARENB   000100
#define	SER_ODDPAR   000200
#define	SER_SPEED    006000
#define	SER_RXENAB   010000

/*
 * FCC register and bit definitions
 */

#define	IIDLE		0x0001	/* status of "Pixel transfer idle" interrupt */
#define ENIDLE		0x0002  /* enable "Pixel transfer idle" interrupt */
#define	ENTHRSH		0x0040  /* enable FIFO threshold interrupt */
#define	ITHRESH		0x0080	/* status of "FIFO pass threshold" interrupt */

/*
 * Color board general CSR bit definitions
 */

#define IDLE		0x8000      /* FIFO has data or not */
#define FLUSH		0x4000      /* FIFO complete current operation */
#define	ADDREQ		0x2000      /* ADDER requesting I/O */
#define	PTB_DECOM_ENB	0x1F00      /* host-to-bitmap (use decompression) */
#define PTB_UNPACK_ENB	0x0700      /* host-to-bitmap (unpack bytes) */
#define PTB_ENB		0x0600      /* host-to-bitmap xfer */
#define	BTP_COM_ENB	0x1D00      /* bitmap-to-host (use compression) */
#define BTP_PACK_ENB	0x0500      /* bitmap-to-host (pack bytes) */
#define BTP_ENB		0x0400      /* bitmap-to-host xfer */
/*#define DL_ENB		0x0200      /* display list to ADDER */
/*#define HALT		0x0000      /* halt */
#define	DIAG		0x0080	    /* diagnostic mode */
#define	CTESTH		0x0040	    /* TEST bit from the cursor chip */
#define	INTMPH		0x0020	    /* status bit */
#define	FADR16		0x0002	    /* fifo address bit 16 */
#define	FADR15		0x0001	    /* fifo address bit 15 */


/* Software pointer into the VAXstar FIFO */

extern u_short * sg_next_fifo;

/* VAXstar memcsr bit definitions */

#define	UNBLANK			0x0020
#define SYNC_ON			0x0008

	struct fcc {

	    unsigned short cbcsr;	/* color board general CSR */
	    unsigned short icsr;	/* interrupt control and status reg. */
	    unsigned short fcsr;	/* fifo control and status register */
	    unsigned short fwused;	/* fifo words used */
	    unsigned short thresh;	/* threshold for comparing with fwused*/
	    unsigned short zoo;		/* not used */
	    unsigned short put;		/* put pointer for fifo control */
	    unsigned short get;		/* get pointer for fifo control */
	    unsigned short diag;	/* data for diagnostics modes */
	    unsigned short cmpa;
	    unsigned short cmpb;
	    unsigned short cmpc;	/* pla address bits */
	    unsigned short pad[116];
	};


/* cursor registers */

	struct	color_cursor {
	    u_short cmdr;		/* command register */
	    u_short xpos;		/* x position Register */
	    u_short ypos;		/* y position register */
	    u_short xmin1;		/* xmin1 active region register */
	    u_short xmax1;		/* xmax1 active region register */
	    u_short ymin1;		/* ymin1 active region register */
	    u_short ymax1;		/* ymax1 active region register */
	    u_short pad[4];
	    u_short xmin2;		/* xmin2 active region register */
	    u_short xmax2;		/* xmax2 active region register */
	    u_short ymin2;		/* ymin2 active region register */
	    u_short ymax2;		/* ymax2 active region register */
	    u_short cmem;		/* cursor memory register */
	};

/*
 * macros to transform device coordinates to hardware cursor coordinates
 */


/*********************************************************************
 *
 *	EVENT QUEUE DEFINITIONS
 *
 *********************************************************************
 * most of the event queue definitions are found in "qevent.h".  But a
 * few things not found there are here.
 */ 	

/* The event queue header */
	
struct sginput {

	    struct _vs_eventqueue header;  /* event queue ring handling */

/* for VS100 and QVSS compatability reasons, additions to this
 * structure must be made below this point.
 */

	    struct _vs_cursor curs_pos;	/* current mouse position */
	    struct _vs_box curs_box;	/* cursor reporting box */

	};
	
/* vse_key field.  definitions for mouse buttons */

#define VSE_LEFT_BUTTON		0
#define VSE_MIDDLE_BUTTON	1
#define VSE_RIGHT_BUTTON	2

/* vse_key field.  definitions for mouse buttons */

#define VSE_T_LEFT_BUTTON	0
#define VSE_T_FRONT_BUTTON	1
#define VSE_T_RIGHT_BUTTON	2
#define VSE_T_BACK_BUTTON	4

#define VSE_T_BARREL_BUTTON	VSE_T_LEFT_BUTTON
#define VSE_T_TIP_BUTTON	VSE_T_FRONT_BUTTON

/*
 * These are the macros to be used for loading and extracting from the event
 * queue.  It is presumed that the macro user will only use the access macros
 * if the event queue is non-empty ( ISEMPTY(eq) == FALSE ), and that the
 * driver will only load the event queue after checking that it is not full
 * ( ISFULL(eq) == FALSE ).  ("eq" is a pointer to the event queue header.)
 *
 *   Before an event access session for a particular event, the macro users
 * must use the xxxBEGIN macro, and the xxxEND macro after an event is through
 * with.  As seen below, the xxxBEGIN and xxxEND macros maintain the event
 * queue access mechanism.
 *
 * First, the macros to be used by the event queue reader 
 */

#define ISEMPTY(eq)	  ((eq)->header.head == (eq)->header.tail)
#define GETBEGIN(eq)	  (&(eq)->header.events[(eq)->header.head]) 

#define GET_X(event)	  ((event)->vse_x)  	     /* get x position */
#define GET_Y(event)	  ((event)->vse_y)  	     /* get y position */
#define GET_TIME(event)	  ((event)->vse_time) 	     /* get time */
#define GET_TYPE(event)	  ((event)->vse_type)	     /* get entry type */
#define GET_KEY(event)	  ((event)->vse_key)  	     /* get keycode */
#define GET_DIR(event)	  ((event)->vse_direction)     /* get direction */
#define GET_DEVICE(event) ((event)->vse_device)        /* get device */

#define GETEND(eq)        (++(eq)->header.head >= (eq)->header.size ? \
			   (eq)->header.head = 0 : 0 )

/*
 * macros to be used by the event queue loader
 */

/* ISFULL yields TRUE if queue is full */

#define ISFULL(eq)	((eq)->header.tail+1 == (eq)->header.head ||   \
			 ((eq)->header.tail+1 == (eq)->header.size &&  \
			  (eq)->header.head == 0))

/* get address of the billet for NEXT event */

#define PUTBEGIN(eq)	(&(eq)->header.events[(eq)->header.tail])

#define PUT_X(event, value)  	((event)->vse_x = value)    /* put X pos */
#define PUT_Y(event, value)   	((event)->vse_y = value)    /* put Y pos */
#define PUT_TIME(event, value)	((event)->vse_time = value)   /* put time */
#define PUT_TYPE(event, value)	((event)->vse_type = value) /* put type */
#define PUT_KEY(event, value)	((event)->vse_key = value) /* put input key */
#define PUT_DIR(event, value)	((event)->vse_direction = value) /* put dir */
#define PUT_DEVICE(event, value) ((event)->vse_device = value)   /* put dev */

#define PUTEND(eq)     (++(eq)->header.tail >= (eq)->header.size ?  \
			(eq)->header.tail = 0 : 0) 

/******************************************************************
 *
 *	DMA I/O DEFINITIONS
 *
 ******************************************************************/

/*
 * The DMA request queue is implemented as a ring buffer of "DMAreq"
 * structures.  The queue is accessed using ring indices located in the
 * "DMAreq_header" structure.  Access is implemented using access macros
 * similar to the event queue access macros above.
 */

	struct FIFOreq {

	    short FIFOtype;		/* FIFO type code */
	    char  *bufp;		/* virtual adrs of buffer */
	    int   length;	        /* transfer buffer length */
	};

/* DMA type command codes */

#define DISPLIST	1	/* display list DMA */
#define PTOB		2	/* 1 plane Qbus to bitmap DMA */
#define BTOP		3	/* 1 plane bitmap to Qbus DMA */

/* DMA done notification code */

#define FIFO_EMPTY	0x01	/* DONE when FIFO becomes empty */
#define COUNT_ZERO	0x02	/* DONE when count becomes zero */
#define WORD_PACK	0x04    /* program the gate array for word packing */
#define BYTE_PACK	0x08	/* program gate array for byte packing */
#define REQUEST_DONE	0x100	/* clear when driver has processed request */
#define HARD_ERROR	0x200   /* DMA hardware error occurred */

/* DMA request queue is a ring buffer of request structures */

	struct FIFOreq_header {

	    short status;	    /* master FIFO status word */
	    int shared_size;	    /* size of shared memory in bytes */
	    struct FIFOreq *FIFOreq;  /* start address of request queue */
	    int used;		    /* # of queue entries currently used */
	    int size;		    /* # of "FIFOreq"'s in the request queue */
	    int oldest;		    /* index to oldest queue'd request */
	    int newest;		    /* index to newest queue'd request */
	    short  change_section;
	    short  sg_int_flag;
	    u_short sg_next_fifo;
	};

/* bit definitions for DMAstatus word in DMAreq_header */

#define	DMA_ACTIVE	0x0004		/* DMA in progress */
#define DMA_ERROR	0x0080		/* DMA hardware error */
#define DMA_IGNORE	0x0002		/* flag to ignore this interrupt */

/*
 * macros for DMA request queue fiddling
 */

	/* DMA status set/check macros */

#define DMA_SETACTIVE(header)   ((header)->status |= DMA_ACTIVE)
#define DMA_CLRACTIVE(header)	((header)->status &= ~DMA_ACTIVE)
#define DMA_ISACTIVE(header)    ((header)->status & DMA_ACTIVE)

#define DMA_SETERROR(header)    ((header)->status |= DMA_ERROR)
#define DMA_CLRERROR(header)    ((header)->status &= ~DMA_ERROR)
#define DMA_ISERROR(header)     ((header)->status & DMA_ERROR)

#define DMA_SETIGNORE(header)	((header)->status |= DMA_IGNORE)
#define DMA_CLRIGNORE(header)   ((header)->status &= ~DMA_IGNORE)
#define DMA_ISIGNORE(header)    ((header)->status & DMA_IGNORE)

/* yields TRUE if queue is empty (ISEMPTY) or full (ISFULL) */

#define DMA_ISEMPTY(header)	((header)->used == 0)
#define DMA_ISFULL(header)	((header)->used >= (header)->size)

/* returns address of the billet for next (PUT)
 * or oldest (GET) request
 */

#define FIFO_PUTBEGIN(header)	(&(header)->FIFOreq[(header)->newest])
#define FIFO_GETBEGIN(header)  	(&(header)->FIFOreq[(header)->oldest])

#define DMA_PUTBEGIN(header)	(&(header)->DMAreq[(header)->newest])
#define DMA_GETBEGIN(header)  	(&(header)->DMAreq[(header)->oldest])

/* does queue access pointer maintenance */

#define DMA_GETEND(header)      (++(header)->oldest >= (header)->size    \
				  ? (header)->oldest = 0 : 0);		 \
				--(header)->used;

#define DMA_PUTEND(header)     	(++(header)->newest >= (header)->size    \
				  ? (header)->newest = 0 : 0);		 \
				++(header)->used;

/******************************************************************
 *
 *	COLOR MAP WRITE BUFFER DEFINITIONS
 *
 ******************************************************************/

#define LOAD_COLOR_MAP	0x0001

/******************************************************************
 *
 *	SCROLL ASSIST DEFINITIONS
 *
 ******************************************************************/

#define LOAD_REGS	0x0001
#define LOAD_INDEX	0x0002

/******************************************************************
 *
 *	MOUSE/TABLET/KBD PROGRAMMING DEFINITIONS
 *
 ******************************************************************/

/*
 * LK201 programmming definitions
 */

#define LK_UPDOWN 	0x86		/* bits for setting lk201 modes */
#define LK_AUTODOWN 	0x82
#define LK_DOWN 	0x80
#define LK_DEFAULTS 	0xD3		/* reset (some) default settings */
#define LK_AR_ENABLE 	0xE3		/* global auto repeat enable */
#define LK_CL_ENABLE 	0x1B		/* keyclick enable */
#define LK_KBD_ENABLE 	0x8B		/* keyboard enable */
#define LK_BELL_ENABLE 	0x23		/* the bell */
#define LK_RING_BELL 	0xA7		/* ring keyboard bell */

#define LK_LED_ENABLE 	0x13		/* light led */
#define LK_LED_DISABLE 	0x11		/* turn off led */
#define LED_1 		0x81		/* led bits */
#define LED_2 		0x82
#define LED_3 		0x84
#define LED_4 		0x88
#define LED_ALL 	0x8F
#define LK_LED_HOLD	LED_4
#define LK_LED_LOCK	LED_3
#define LK_LED_COMPOSE	LED_2
#define LK_LED_WAIT 	LED_1

#define LK_KDOWN_ERROR	0x3D		/* key down on powerup error */
#define LK_POWER_ERROR 	0x3E		/* keyboard failure on powerup test */
#define LK_OUTPUT_ERROR	0xB5		/* keystrokes lost during inhibit */
#define LK_INPUT_ERROR 	0xB6		/* garbage command to keyboard */
#define LK_LOWEST	0x56		/* lowest significant keycode */
#define LK_DIV6_START	0xAD		/* start of div 6 */
#define LK_DIV5_END	0xB2		/* end of div 5 */

#define LAST_PARAM	0x80		/* "no more params" bit */

/*
 * "special" LK-201 keys
 */

#define SHIFT		174
#define LOCK		176
#define REPEAT		180
#define CNTRL		175
#define ALLUP		179
#define HOLD		 86

/*
 * cursor programming structure
 */

/*
 * mouse definitions
 */

#define INCREMENTAL	'R'		/* stream mode reports (55 hz) */
#define PROMPT_MODE	'D'		/* report when prompted */
#define REQUEST_POS	'P'		/* request position report */
#define SELF_TEST	'T'		/* request self test */

#define MOUSE_ID	0x2		/* mouse ID in lo 4 bits */

#define START_FRAME	0x80		/* start of report frame bit */
#define X_SIGN		0x10		/* position sign bits */
#define Y_SIGN		0x08

#define RIGHT_BUTTON	0x01		/* mouse buttons */
#define MIDDLE_BUTTON	0x02
#define LEFT_BUTTON	0x04


/*
 * tablet command/interface definitions
 */

#define T_STREAM	'R'		/* continuous stream report mode */
#define T_POINT	 	'D'		/* enter report-on-request mode */
#define T_REQUEST	'P'		/* request position report */

#define T_BAUD		'B'		/* increase baud to 9600 from 4800 */
#define T_RATE_55	'K'		/* report rate: 55/sec */
#define T_RATE_72	'L'		/* report rate: 72/sec */
#define T_RATE_120	'M'		/* report rate: 120/sec (9600 only) */

#define T_TEST		SELF_TEST	/* do self test */

#define TABLET_ID	0x4		/* tablet ID in lo 4 bits */

#define T_START_FRAME	0x80		/* start of report frame bit */
#define T_PROXIMITY	0x01		/* state pointer in proximity */

#define T_LEFT_BUTTON	0x02		/* puck buttons */
#define T_FRONT_BUTTON	0x04
#define T_RIGHT_BUTTON	0x08
#define T_BACK_BUTTON	0x10

#define T_BARREL_BUTTON T_LEFT_BUTTON		/* stylus buttons */
#define T_TIP_BUTTON	T_FRONT_BUTTON

