/* 
 *  dc7085.h --
 *
 *     	Definitions for the dc7085 chip.
 *
 *	Copyright (C) 1989 Digital Equipment Corporation.
 *	Permission to use, copy, modify, and distribute this software and
 *	its documentation for any purpose and without fee is hereby granted,
 *	provided that the above copyright notice appears in all copies.  
 *	Digital Equipment Corporation makes no representations about the
 *	suitability of this software for any purpose.  It is provided "as is"
 *	without express or implied warranty.
 *
 * $Header$ SPRITE (DECWRL)
 */

/*
 * Define the six registers.
 */
extern volatile unsigned short *devCSRPtr;
extern volatile unsigned short *devRBufPtr;
extern volatile unsigned short *devLPRPtr;
extern volatile unsigned short *devTCRPtr;
extern volatile unsigned short *devMSRPtr;
extern volatile unsigned short *devTDRPtr;

/*
 * Control status register bits.
 */
#define	CSR_TRDY	0x8000
#define CSR_TIE		0x4000
#define	CSR_TX_LINE_NUM	0x0300
#define	CSR_RDONE	0x0080
#define	CSR_RIE		0x0040
#define CSR_MSE		0x0020
#define CSR_CLR		0x0010
#define CSR_MAINT	0x0008

/*
 * Receiver buffer register bits.
 */
#define	RBUF_DVAL		0x8000
#define RBUF_OERR		0x4000
#define RBUF_FERR		0x2000
#define RBUF_PERR		0x1000
#define RBUF_LINE_NUM		0x0300
#define RBUF_LINE_NUM_SHIFT	8
#define RBUF_CHAR		0x00FF

/*
 * Line parameter register bits.
 */
#define	LPR_RXENAB	0x1000
#define	LPR_B4800	0x0C00
#define LPR_B9600	0x0E00
#define LPR_OPAR	0x0080
#define LPR_PARENB	0x0040
#define LPR_2_STOP	0x0020
#define LPR_8_BIT_CHAR	0x0018
#define LPR_7_BIT_CHAR	0x0010
#define LPR_6_BIT_CHAR	0x0008
#define LPR_5_BIT_CHAR	0x0000

/*
 * The four serial ports.
 */
#define	KBD_PORT	0
#define MOUSE_PORT	1
#define MODEM_PORT	2
#define PRINTER_PORT	3

/*
 * Special key values.
 */
#define KEY_SHIFT	0xae
#define KEY_CONTROL	0xaf
#define KEY_UP		0xb3
#define KEY_REPEAT	0xb4
#define KEY_F13		0x73

/*
 * Command characters for the mouse.
 */
#define MOUSE_SELF_TEST		'T'
#define MOUSE_INCREMENTAL	'R'

/*
 * Mouse output bits.
 *
 *     	MOUSE_START_FRAME	Start of report frame bit.
 *	MOUSE_X_SIGN		Sign bit for X.
 *	MOUSE_Y_SIGN		Sign bit for Y.
 *	MOUSE_X_OFFSET		X offset to start cursor at.
 *	MOUSE_Y_OFFSET		Y offset to start cursor at.
 */
#define MOUSE_START_FRAME	0x80
#define MOUSE_X_SIGN		0x10
#define MOUSE_Y_SIGN		0x08
#define	MOUSE_X_OFFSET		212
#define	MOUSE_Y_OFFSET		 34

/* 
 * Definitions for mouse buttons
 */
#define RIGHT_BUTTON		0x01
#define MIDDLE_BUTTON		0x02
#define LEFT_BUTTON		0x04

#define MOTION_BUFFER_SIZE 100

/* 
 * Mouse report structure definition
 */
typedef struct {
	char state;			/* buttons and sign bits	*/
	short dx;			/* delta X since last change	*/
	short dy;			/* delta Y since last change	*/
	char byteCount;			/* mouse report byte count	*/
} MouseReport;

/* 
 * Lk201/301 keyboard
 */
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


