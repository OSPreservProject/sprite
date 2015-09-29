
 /* @(#)pmreg.h		"@(#)pmreg.h	4.5	(ULTRIX)	11/17/88" */

/************************************************************************
 *									*
 *			Copyright (c) 1986 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *									*
 *   This software is furnished under a license and may be used and	*
 *   copied  only  in accordance with the terms of such license and	*
 *   with the  inclusion  of  the  above  copyright  notice.   This	*
 *   software  or  any	other copies thereof may not be provided or	*
 *   otherwise made available to any other person.  No title to and	*
 *   ownership of the software is hereby transferred.			*
 *									*
 *   This software is  derived  from  software  received  from  the	*
 *   University	of   California,   Berkeley,   and   from   Bell	*
 *   Laboratories.  Use, duplication, or disclosure is  subject  to	*
 *   restrictions  under  license  agreements  with  University  of	*
 *   California and with AT&T.						*
 *									*
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************/

/***********************************************************************
 *
 * Modification History:
 *
 * 17-Nov-88  -- Vasudev K. Bhandarkar
 *
 *	Got rid of TTY definitions
 *
 * 14-Aug-86  -- refiey (Ali Rafieymehr)
 *	Tablet support.
 *
 *  5-Aug-86  -- rafiey (Ali Rafieymehr)
 *	Changes for real VAXstar bitmap graphics driver.
 *
 * 18-Jun-86  -- rafiey (Ali Rafieymehr)
 *	Created this header file for the VAXstar monochrome display driver.
 *	Derived from qvreg.h.
 *
 **********************************************************************/

/*
 * VAXstar Monochrome definitions.
 */

#define	SELF_TEST	'T'
#define	INCREMENTAL	'R'
#define	PROMPT		'D'

#define	MOUSE_ID	0x2
#define TABLET_ID	0x4

#define START_FRAME	0x80		/* start of report frame bit */
#define X_SIGN		0x10		/* sign bit for X */
#define Y_SIGN		0x08		/* sign bit for Y */

#define	XOFFSET		212
#define	YOFFSET		 34

#define	UPDATE_POS	0x01

/*
 * VAXstar interrupt controller register bits
 *
 */

#define SINT_VF		010

/*
 * Cursor Command Register bits
 *
 */

#define	ENPA	0000001
#define	FOPA	0000002
#define	ENPB	0000004
#define	FOPB	0000010
#define XHAIR	0000020
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
 * Mouse definitions
 *
 */

#define MOTION_BUFFER_SIZE 100
#define	SELF_TEST	'T'

#define EVENT_T_LEFT_BUTTON	0x00
#define EVENT_T_FRONT_BUTTON	0x01
#define EVENT_T_RIGHT_BUTTON	0x02
#define EVENT_T_BACK_BUTTON	0x04

/* puck buttons */

#define T_LEFT_BUTTON		0x02
#define T_FRONT_BUTTON		0x04
#define T_RIGHT_BUTTON		0x08
#define T_BACK_BUTTON		0x10

/*
 * Lk201 keyboard 
 */

#define LK_DOWN 	0x80
#define LED_ALL 	0x8f
#define LK_KDOWN_ERROR	0x3d		/* key down on powerup error */
#define LK_OUTPUT_ERROR	0xb5		/* keystrokes lost during inhibit */
#define LK_LOWEST	0x56		/* lowest significant keycode */
#define LK_DIV6_START	0xad		/* start of div 6 */
#define LK_DIV5_END	0xb2		/* end of div 5 */

/*
 * Keycodes for special keys and functions
 */

#define SHIFT	0xae
#define LOCK	0xb0
#define REPEAT	0xb4
#define CNTRL	0xaf
#define ALLUP	0xb3
#define	HOLD	0x56

#define	CONSOLEMAJOR	0
#define	PM_MAJOR	10
#define CONS_DEV	0x01
#define GRAPHIC_DEV	0x02

/********************************************************
 *							*
 *  These are the physical registers in the memory map	*
 *							*
 ********************************************************/
#define SERIAL_REG	0x1c000000
#define CURSOR_REG	0x11000000
#define FRAME_BUF	0x0fc00000
#define VDAC_REG	0x12000000
#define PLANE_REG	0x10000000

/********************************************************
 * Following allow pmputc to function in		*
 * in physical mode (during a crash dump).		*
 * One way transition, can't go back to virtual.	*
 ********************************************************/
#define KEYBD_BASE (*(struct cn_reg*)PHYS_TO_K1(SERIAL_REG))
#define KEYBD_ADDR &KEYBD_BASE
#define PCC_BASE (*(struct pcc_regs*)PHYS_TO_K1(CURSOR_REG))
#define PCC_ADDR &PCC_BASE
#define BITMAP_BASE (*(u_int *)PHYS_TO_K1(FRAME_BUF))
#define BITMAP_ADDR &BITMAP_BASE
#define VDAC_BASE (*(vdac_regs *)PHYS_TO_K1(VDAC_REG))
#define VDAC_ADDR &VDAC_BASE
#define PLANE (*(short *)PHYS_TO_K1(PLANE_REG))
#define PLANE_ADDR &PLANE

struct pcc_regs {
        u_short 	cmdr;   /* 0 */
	u_short		pad1;
	u_short 	xpos;   /* 1 */
	u_short		pad2;
	u_short 	ypos;   /* 2 */
	u_short		pad3;
	u_short 	xmin1;  /* 3 */
	u_short		pad4;
	u_short 	xmax1;  /* 4 */
	u_short		pad5;
	u_short 	ymin1;  /* 5 */
	u_short		pad6;
	u_short 	ymax1;  /* 6 */
	u_short		pad7;
	u_short         unused7; /* 7 */
	u_short         padunused7; 
	u_short         unused8; /* 8 */
	u_short         padunused8; 
	u_short         unused9; /* 9 */
	u_short         padunused9; 
	u_short         unusedA; /* A */
	u_short         padunusedA; 
	u_short  	xmin2;   /* B */
	u_short		pad8;
	u_short		xmax2;   /* C */
	u_short		pad9;
	u_short		ymin2;   /* D */
	u_short		pada;
	u_short		ymax2;   /* E */
	u_short		padb;
	u_short 	memory;  /* F */
	u_short		padc;
};

/*  structure declaration for VDAC, regs aligned on word boundries */

typedef volatile struct {
    u_char   map_wr;
    u_char   pad0;
    u_short  pad1;

    u_char   map_ram;
    u_char   pad2;
    u_short  pad3;

    u_char   mask;
    u_char   pad4;
    u_short  pad5;

    u_char   map_rd;
    u_char   pad6;
    u_short  pad7;

    u_char   over_wr;
    u_char   pad8;
    u_short  pad9;

    u_char   over_regs;
    u_char   pad10;
    u_short  pad11;

    u_char   reserved;
    u_char   pad12;
    u_short  pad13;

    u_char   over_rd;
    u_char   pad14;
    u_short  pad15;
}vdac_regs ;

