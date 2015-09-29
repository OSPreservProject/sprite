
/*
 * @(#)dc7085cons.h	4.7	(ULTRIX)	12/16/88
 */

/************************************************************************
 *									*
 *			Copyright (c) 1988 by				*
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
 *   The information in this software is subject to change  without	*
 *   notice  and should not be construed as a commitment by Digital	*
 *   Equipment Corporation.						*
 *									*
 *   Digital assumes no responsibility for the use  or	reliability	*
 *   of its software on equipment which is not supplied by Digital.	*
 *									*
 ************************************************************************
 *
 * dc7085cons.h
 *
 * DC7085 SLU console driver
 *
 * Modification history
 *
 * 16-Dec-1988 - Randall P. Brown
 *	Added the struct for pdma, and included the pdma struct in the
 * 	cn_softc struct.
 *
 * 17-Nov-1988 - Randall P. Brown
 *	Cleaned up file so that names are consistent with spec.
 *
 *  7-Jul-1988 - rsp (Ricky Palmer)
 *	Created file. Contents based on ssreg.h file.
 *
 */

/* Serial line registers */
struct cn_reg {
	u_short DZ_CSR;			/* SLU control status register	*/
	u_short pad1[3];
	union {				/* SLU read buffer/line param.	*/
		u_short DZ_RBUF;
		u_short DZ_LPR;
	} DZ_RBUF_LPR;
	u_short pad2[3];
	u_short DZ_TCR;			/* SLU transmitter control reg. */
	u_short pad3[3];
	union {				/* SLU modem status/txmt reg.	*/
		u_short	DZ_MSR;
		u_short DZ_TDR;
	} DZ_MSR_TDR;
};

#define cncsr		sc_regs->DZ_CSR
#define cndtr		sc_regs->DZ_TCR
#define cnmsr		sc_regs->DZ_MSR_TDR.DZ_MSR
#define cnbrk		sc_regs->DZ_MSR_TDR.DZ_MSR
#define cnrbuf		sc_regs->DZ_RBUF_LPR.DZ_RBUF
#define cnlpr		sc_regs->DZ_RBUF_LPR.DZ_LPR
#define cntcr		sc_regs->DZ_TCR
#define cntbuf		sc_regs->DZ_MSR_TDR.DZ_TDR

#define CONSOLEMAJOR	0
#define CNMAJOR 0
#define NCNLINE 5 /* 4 real lines + silly XCONSDEV */
#define XCONSDEV 4

/* Interrupt controller register bits */
#define SINT_SR 0200			/* Serial line recv/silo full	*/
#define SINT_ST 0100			/* Serial line transmitter done */

/* Control status register definitions (cncsr) */
#define CN_OFF		0x00		/* Modem control off		*/
#define CN_MAINT	0x08		/* Maintenance			*/
#define CN_CLR		0x10		/* Reset dc7085 chip		*/
#define CN_MSE		0x20		/* Master Scan Enable		*/
#define CN_RIE		0x40		/* Receive IE */
#define CN_RDONE	0x80		/* Receiver done		*/
#define CN_TIE		0x4000		/* Trasmit IE */
#define CN_TRDY		0x8000		/* Transmit ready		*/

/* Line parameter register definitions (cnlpr) */
#define BITS5		0x00		/* 5 bit char width		*/
#define BITS6		0x08		/* 6 bit char width		*/
#define BITS7		0x10		/* 7 bit char width		*/
#define BITS8		0x18		/* 8 bit char width		*/
#define TWOSB		0x20		/* two stop bits		*/
#define PENABLE		0x40		/* parity enable		*/
#define OPAR		0x80		/* odd parity			*/
#define CN_B4800	0xc00		/* 4800 BPS speed		*/
#define CN_B9600	0xe00		/* 9600 BPS speed		*/
#define CN_RE		0x1000		/* Receive enable		*/

/* Receiver buffer register definitions (cnrbuf) */
#define CN_PE		0x1000		/* Parity error			*/
#define CN_FE		0x2000		/* Framing error		*/
#define CN_DO		0x4000		/* Data overrun error		*/
#define CN_DVAL		0x8000		/* Receive buffer data valid	*/

/* Line control status definitions (cnlcs) */
#define CN_SR		0x08		/* Secondary Receive		*/
#define CN_CTS		0x10		/* Clear To Send		*/
#define CN_CD		0x20		/* Carrier Detect		*/
#define CN_RI		0x40		/* Ring Indicate		*/
#define CN_DSR		0x80		/* Data Set Ready		*/
#define CN_LE		0x100		/* Line Enable			*/
#define CN_DTR		0x200		/* Data Terminal Ready		*/
#define CN_BRK		0x400		/* Break			*/
#define CN_ST		0x800		/* Secondary Transmit		*/
#define CN_RTS		0x1000		/* Request To Send		*/

/* DM lsr definitions */
#define SML_LE		0x01		/* Line enable			*/
#define SML_DTR		0x02		/* Data terminal ready		*/
#define SML_RTS		0x04		/* Request to send		*/
#define SML_ST		0x08		/* Secondary transmit		*/
#define SML_SR		0x10		/* Secondary receive		*/
#define SML_CTS		0x20		/* Clear to send		*/
#define SML_CAR		0x40		/* Carrier detect		*/
#define SML_RNG		0x80		/* Ring				*/
#define SML_DSR		0x100		/* Data set ready, not DM bit	*/

/* cndtr bits */
#define CN_RDTR 0x400			/* REAL data terminal ready	*/

/* cnmsr bits */
#define CN_RDSR 0x200			/* REAL data set ready bit	*/
#define CN_XMIT CN_RDSR			/* Ready to transmit & rec.	*/

/* Monochrome definitions */
#define SELF_TEST	'T'
#define INCREMENTAL	'R'
#define PROMPT		'D'

#define MOUSE_ID	0x2
#define TABLET_ID	0x4

#define START_FRAME	0x80		/* start of report frame bit	*/
#define X_SIGN		0x10		/* sign bit for X		*/
#define Y_SIGN		0x08		/* sign bit for Y		*/

#define UPDATE_POS	0x01

/* Interrupt controller register bits */
#define SINT_VF		010

/* Cursor Command Register bits */
#define ENPA	0000001
#define FOPA	0000002
#define ENPB	0000004
#define FOPB	0000010
#define XHAIR	0000020
#define XHCLP	0000040
#define XHCL1	0000100
#define XHWID	0000200
#define ENRG1	0000400
#define FORG1	0001000
#define ENRG2	0002000
#define FORG2	0004000
#define LODSA	0010000
#define VBHI	0020000
#define HSHI	0040000
#define TEST	0100000


/* Line Prameter Register bits */
#define SER_KBD      000000
#define SER_POINTER  000001
#define SER_COMLINE  000002
#define SER_PRINTER  000003
#define SER_CHARW    000030
#define SER_STOP     000040
#define SER_PARENB   000100
#define SER_ODDPAR   000200
#define SER_SPEED    006000
#define SER_RXENAB   010000

/* Definitions for mouse buttons */
#define EVENT_LEFT_BUTTON	0x01
#define EVENT_MIDDLE_BUTTON	0x02
#define EVENT_RIGHT_BUTTON	0x03
#define RIGHT_BUTTON		0x01
#define MIDDLE_BUTTON		0x02
#define LEFT_BUTTON		0x04

/* Mouse definitions */
#define MOTION_BUFFER_SIZE 100
#define SELF_TEST	'T'

/* Mouse report structure definition */
struct mouse_report {
	char state;			/* buttons and sign bits	*/
	short dx;			/* delta X since last change	*/
	short dy;			/* delta Y since last change	*/
	char bytcnt;			/* mouse report byte count	*/
};


/* Definitions for tablet buttons */
#define EVENT_T_LEFT_BUTTON	0x00
#define EVENT_T_FRONT_BUTTON	0x01
#define EVENT_T_RIGHT_BUTTON	0x02
#define EVENT_T_BACK_BUTTON	0x04
#define T_LEFT_BUTTON		0x02
#define T_FRONT_BUTTON		0x04
#define T_RIGHT_BUTTON		0x08
#define T_BACK_BUTTON		0x10

/* Lk201/301 keyboard */
#define LK_UPDOWN	0x86		/* bits for setting lk201 modes */
#define LK_AUTODOWN	0x82
#define LK_DOWN		0x80
#define LK_DEFAULTS	0xd3		/* reset (some) default settings*/
#define LK_AR_ENABLE	0xe3		/* global auto repeat enable	*/
#define LK_CL_ENABLE	0x1b		/* keyclick enable		*/
#define LK_KBD_ENABLE	0x8b		/* keyboard enable		*/
#define LK_BELL_ENABLE	0x23		/* the bell			*/
#define LK_LED_ENABLE	0x13		/* light led			*/
#define LK_LED_DISABLE	0x11		/* turn off led			*/
#define LK_RING_BELL	0xa7		/* ring keyboard bell		*/
#define LED_1		0x81		/* led bits			*/
#define LED_2		0x82
#define LED_3		0x84
#define LED_4		0x88
#define LED_ALL		0x8f
#define LK_KDOWN_ERROR	0x3d		/* key down on powerup error	*/
#define LK_POWER_ERROR	0x3e		/* keyboard failure on pwrup tst*/
#define LK_OUTPUT_ERROR 0xb5		/* keystrokes lost during inhbt */
#define LK_INPUT_ERROR	0xb6		/* garbage command to keyboard	*/
#define LK_LOWEST	0x56		/* lowest significant keycode	*/
#define LK_DIV6_START	0xad		/* start of div 6		*/
#define LK_DIV5_END	0xb2		/* end of div 5			*/

/* Keycodes for special keys and functions */
#define SHIFT	0xae
#define LOCK	0xb0
#define REPEAT	0xb4
#define CNTRL	0xaf
#define ALLUP	0xb3
#define HOLD	0x56

/* DZ11/32 register device structure */
struct dzdevice {
	short dzcsr;
	short dzrbuf;
	union {
		struct {
			char	dztcr0;
			char	dzdtr0;
			char	dztbuf0;
			char	dzbrk0;
		} dz11;
		struct {
			short	dzlcs0;
			char	dztbuf0;
			char	dzlnen0;
		} dz32;
	} dzun;
};

/* Pseudo DMA structure */
struct cnpdma {
    	char *p_mem;
	char *p_end;
};

/* Driver and data specific structure */
/***** cn_tty must be first *****/
struct	cn_softc {
	struct	tty cn_tty[NCNLINE];	/* Tty structure		*/
	struct	dzdevice sc_dz;		/* DZ11/32 structure		*/
	struct	cnpdma cn_pdma[NCNLINE];/* peudo dma structure		*/
	volatile struct cn_reg *sc_regs; /* PMAX SLU registers		*/
	long	sc_flags[NCNLINE];	/* Flags (one per line)		*/
	long	sc_category_flags[NCNLINE]; /* Category flags (one per line)*/
	u_long	sc_softcnt[NCNLINE];	/* Soft error count total	*/
	u_long	sc_hardcnt[NCNLINE];	/* Hard error count total	*/
	char	sc_device[DEV_SIZE][NCNLINE]; /* Device type string	*/
};

