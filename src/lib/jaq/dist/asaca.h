/*
 * 
 * Copyright (c) 1988 Regents of the University of California
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for educational, research, and non-profit purposes and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of the University of California not be used in advertising
 * or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Permission to incorporate this
 * software into commercial products can be obtained from the Campus
 * Software Office, 295 Evans Hall, University of California, Berkeley,
 * Ca., 94720 provided only that the the requestor give the University
 * of California a free licence to any derived software for educational
 * and research purposes.  The University of California makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 * 
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <termio.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>

/*
 * ASACA (Metrum RSS-600) device driver. Metrum numbers drum bins by
 * drum face designator ('A' - 'L') and bin number (01 - 50) with a
 * maximum of 600 drum bins. Also, there are up to five RSP-2150 tape
 * readers (or, "VLDS"'s) in an RSS-600. We provide a "slot numer"
 * interface to the user and simply number the drum bins as slots 
 * 0 ... 599 and the tape readers as slots 600 ... 604. The mapping,
 * in Metrum RSS-600 terms, is as follows:
 *
 *	Drum face:	Bin number:	VLDS number	Slot number:
 *	*********	**********	***********	************
 *	  'A'		    '01'	   N/A		     0
 *	  'A'		    '02'	   N/A		     1
 *	   .		      .		     .		     .
 *	   .		      .		     .		     .
 *	  'A'		    '50'	   N/A		    49
 *	  'B'		    '01'	   N/A		    50
 *	   .		      .		     .		     .
 *	   .		      .		     .		     .
 *	  'B'		    '50'	   N/A		    99
 *	  'C'		    '00'	   N/A		   100
 *	   .		      .		     .		     .
 *	   .		      .		     .		     .
 *	  'L'		    '50'	   N/A		   599
 *	  N/A		    N/A		     0		   600
 *	  N/A		    N/A		     1		   601
 *	  N/A		    N/A		     2		   602
 *	  N/A		    N/A		     3		   603
 *	  N/A		    N/A		     4		   604
 */

#define	AS_MAXBIN	599
#define	AS_VLDS0	600
#define	AS_VLDS1	601
#define	AS_VLDS2	602
#define	AS_VLDS3	603
#define	AS_VLDS4	604

#define isvlds(x)	((AS_VLDS0 <= (x)) && ((x) <= AS_VLDS4))
#define isbin(x)	((0 <= (x)) && ((x) <= AS_MAXBIN))
#define	unittocmd(x)	('A' + ((x) & 0xff))
#define	unittoface(x)	('A' + ((x) / 50))
#define	unittobin(x)	(((x) % 50) + 1)
#define	unittovlds(x)	('1' + ((x) - AS_VLDS0))

struct ascmd {
	short	as_bin;			/* normally, bin #0 - #599 */
#define	as_src	as_bin			/* slot to move tape FROM */
	short	as_vlds;		/* normally, VLDS #600 - #604 */
#define	as_dest	as_vlds			/* slot to move tape TO */
	short	as_mode;		/* operating mode */
	char	as_barcode[13];		/* barcode */
#define	as_sense as_barcode
	short	as_error;		/* Error level; see below */
};

/*
 * ASACA ioctl's available to applications. The following set of ioctl's
 * correspond, more or less, directly to the actual ASACA commands.
 *
 * NOTE: the driver relies on order of these commands; do not reorder or
 * renumber.
 */
#define	ASIOCSTORE2	_IOWR('z',0, struct ascmd)	/* store w/bar code */
#define	ASIOCLOAD2	_IOWR('z',1, struct ascmd)	/* load w/bar code */
							/* 'C' not used */
#define	ASIOCLOAD1	_IOWR('z',3, struct ascmd)	/* load w/o bar code */
#define	ASIOCEJECT	_IOR('z',4, struct ascmd)	/* Eject */
#define	ASIOCALRMOFF	_IOW('z',5, struct ascmd)	/* Alarm clear */
							/* 'G' not used */
#define	ASIOCBCREAD1	_IOWR('z',7, struct ascmd)	/* Bar code read (50) */
#define	ASIOCMANUAL	_IOW('z',8, struct ascmd)	/* Manual enable */
#define	ASIOCDRUMSET	_IOWR('z',9, struct ascmd)	/* Set drum face */
#define	ASIOCBCRS	_IOW('z',10, struct ascmd)	/* Bar code read stop */
#define	ASIOCSIDEIND	_IOW('z',11, struct ascmd)	/* Side door ind. */
#define	ASIOCBCREAD2	_IOWR('z',12, struct ascmd)	/* Bar code read (1)*/
#define	ASIOCINIT	_IO('z',13)			/* Initialize */
							/* 'O' reserved */
#define	ASIOCMOVE1	_IOWR('z',15, struct ascmd)	/* VLDS - VLDS move */
#define	ASIOCMOVE2	_IOWR('z',16, struct ascmd)	/* bin - bin move*/
#define	ASIOCMOVE3	_IOWR('z',17, struct ascmd)	/* move handler */
#define	ASIOCSENSE	_IOR('z',18, struct ascmd)	/* Door/Carrier sense */
#define	ASIOCINJECT	_IOR('z',19, struct ascmd)	/* Inject */
#define	ASIOCDOORLOCK	_IOW('z',20, struct ascmd)	/* Side door lock */
#define	ASIOCSTORE1	_IOWR('z',21, struct ascmd)	/* store w/o bar code */
#define	ASIOCDETECT	_IOWR('z',22, struct ascmd)	/* Cassette detect */
#define	ASIOCFIPDISP	_IOW('z',23, struct ascmd)	/* FIP display */
#define	ASIOCALRMON	_IOW('z',24, struct ascmd)	/* Alarm on */
#define	ASIOCDOOROPEN	_IO('z',25)			/* Front door open */
/*
 * End of order-specific ioctl's. The following ioctl's give applications a
 * more  "generic" tape and handler "MOVE" interface. "as_src" and "as_dest"
 * must be set to either a drum bin number or a VLDS number for ASIOCMVTAPE,
 * while ASIOCMVHDLR only checks "as_dest". ASIOCMVTAPE_BC does barcode
 * checks on move; no need to mess around with "as_mode"
 */
#define	ASIOCMVTAPE	_IOWR('z',26, struct ascmd)
#define	ASIOCMVTAPE_BC	_IOWR('z',27, struct ascmd)
#define	ASIOCMVHDLR	_IOWR('z',28, struct ascmd)


/*
 * Error codes
 */
#define	AS_ENOERR	0
#define	AS_EWARN	1
#define	AS_EFATAL	2

/*
 * Special tokens
 */
#define	ASHDR		'O'	/* Header char. of ASACA cmds */
#define	ASTERM		'>'	/* Terminator char. of ASACA cmds */
#define	ASALRMALL	'0'	/* turn all alarams on/off */
#define	ASALRM1		'1'	/* turn alarm 1 on/off */
#define	ASALRM2		'2'	/*  "     "   2  "  "  */
#define	ASALRM3		'3'	/*  "     "   3  "  "  */
#define	ASON		'N'	/* "ON" or "ENABLE" */
#define	ASOFF		'F'	/* "OFF" or "DISABLE" */
#define	ASUNLOCK	'0'	/* unlock door */
#define	ASLOCK		'1'	/* lock door */
#define	ASBARCHK	'Y'	/* perform barcode check */
#define	ASNOBARCHK	'N'	/* no barcode chk */

#ifdef	DRIVER
/*
 * Termio struct for ASACA serial line. Need to to a TIOCSETA operation on
 * the ASACA serial port prior to operation.
 */
struct termio	asaca_tio =
{
    /* c_iflag  */ IGNBRK,
    /* c_oflag  */ 0,
    /* c_cflag  */ B2400 | CS8 | CREAD | PARENB | PARODD | HUPCL | CLOCAL,
    /* c_lflag  */ 0,
    /* c_line   */ 0,
    /* c_cc     */ {
    /*   VINTR    */    0,
    /*   VQUIT    */    0,
    /*   VERASE   */    0,
    /*   VKILL    */    0,
    /*   VEOF     */    0,
    /*   VEOL     */    0,
    /*   VEOL2    */    0,
    /*   VSWITCH  */    0,
    /*   VMIN     */    0,
    /*   VTIME    */  100,      /* wait 10s for a character */ }
};
#else	DRIVER
extern struct termio asaca_tio;
#endif	DRIVER
