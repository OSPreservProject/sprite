/*
 * z8530.h --
 *
 *     Types, constants, and macros for the Zilog 8530 SCC chip.
 *
 *     The constants that define the bit patterns for each register are
 *     not explained in this file.  For an explanation of the use of the 
 *     chip and the actual explanation of the meaning of the constants see 
 *     the "Z8030/Z8530 SCC Serial Communication Controller Technical Manual" 
 *     January 1983 edition.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVZ8530
#define _DEVZ8530

#ifndef _DEVTTY
#include "tty.h"
#endif

/*
 *-------------------------------------------------------------------------
 *
 * Read only registers
 *
 *-------------------------------------------------------------------------
 */

/*
 * Bits in read register 0.
 *
 * (See page 4-16)
 */

#define	READ0_RX_READY		0x01	
#define READ0_TIMER_ZERO	0x02
#define	READ0_TX_READY		0x04
#define	READ0_DCD		0x08
#define READ0_SYNC		0x10
#define	READ0_CTS		0x20
#define READ0_TX_UNDERRUN	0x40
#define	READ0_BREAK		0x80

/* 
 * Bits in read register 1.
 *
 * (See page 4-18)
 */

#define	READ1_ALL_SENT		0x01	
#define	READ1_PARITY_ERROR	0x10
#define	READ1_RX_OVERRUN	0x20
#define	READ1_FRAMING_ERROR	0x40

/* 
 * Bits in read register 3.
 *
 * Interrupt pending (IP)  register.  This register can only be read in 
 * channel A.
 *
 * (See page 4-18)
 */

#define READ3_STAT_IP_B		0x01	
#define READ3_TX_IP_B		0x02
#define READ3_RX_IP_B		0x04
#define READ3_STAT_IP_A		0x08
#define READ3_TX_IP_A		0x10
#define READ3_RX_IP_A		0x20

/* 
 * Bits in read register 8.
 *
 * This is the receive data register.
 *
 * (See page 4-19)
 */


/* 
 * Bits in read register 10.
 * 
 * This register is for DPLL and SDLC loop mode status.  Since these modes are
 * not entered, no constants entered here.
 *
 * (See page 4-19)
 */


/*
 *----------------------------------------------------------------------------
 * 
 * Read/write registers
 *
 *----------------------------------------------------------------------------
 */

/* 
 * Bits in read/write register 2.  
 *
 * Interrupt vector
 *
 * (See page 4-4 and 4-19)
 */

/* 
 * Bits in read/write register 12.
 *
 * Lower byte of baud rate constant.
 *
 * (See page 4-13 and 4-20)
 */

/* 
 * Bits in read/write register 13.
 *
 * Upper byte of baud rate constant.
 *
 * (See page 4-13 and 4-20)
 */

/*
 * Bits in read/write register 15
 *
 * (See page 4-15 and 4-20)
 */

#define READWRITE15_TIMER_IE		0x02
#define READWRITE15_DCD_IE		0x08
#define READWRITE15_SYNC_IE		0x10
#define READWRITE15_CTS_IE		0x20
#define READWRITE15_TX_UNDERRUN_IE	0x40
#define READWRITE15_BREAK_IE		0x80

/*
 *---------------------------------------------------------------------------
 *
 * Write-only registers
 *
 *---------------------------------------------------------------------------
 */

/* 
 * Bits for write register 0 
 *
 * (See page 4-1)
 */

#define WRITE0_REG		0x0F	
#define	WRITE0_RESET_STATUS	0x10
#define WRITE0_NEXT_RX_IE	0x20
#define	WRITE0_RESET_TX_INT	0x28
#define	WRITE0_RESET_ERRORS	0x30
#define	WRITE0_CLEAR_INTR	0x38

/* 
 * Bits for write register 1 
 *
 * (See page 4-3)
 */

#define	WRITE1_EXT_IE			0x01
#define	WRITE1_TX_IE			0x02
#define	WRITE1_PARITY_SPECIAL_IE	0x04	
#define WRITE1_FIRST_RX_IE		0x08
#define	WRITE1_RX_IE			0x10

/* 
 * Bits for write register 3 
 *
 * (See page 4-4)
 */

#define	WRITE3_RX_ENABLE	0x01	
#define	WRITE3_AUTO_CD_CTS	0x20
#define WRITE3_RX_5BIT		0x00
#define	WRITE3_RX_6BIT		0x80
#define	WRITE3_RX_7BIT		0x40
#define	WRITE3_RX_8BIT		0xC0

/* 
 * Bits for write register 4 
 *
 * (See page 4-6)
 */

#define	WRITE4_PARITY_ENABLE	0x01	
#define	WRITE4_PARITY_EVEN	0x02
#define	WRITE4_1_STOP		0x04
#define	WRITE4_1_5_STOP		0x08
#define	WRITE4_2_STOP		0x0C
#define WRITE4_X1_CLK		0x00
#define	WRITE4_X16_CLK		0x40
#define WRITE4_X32_CLK		0x80
#define WRITE4_X64_CLK		0xC0

/* 
 * Bits for write register 5 
 *
 * (See page 4-7)
 */

#define	WRITE5_RTS		0x02
#define	WRITE5_TX_ENABLE	0x08
#define	WRITE5_BREAK		0x10
#define	WRITE5_TX_5BIT		0x00
#define	WRITE5_TX_6BIT		0x40
#define	WRITE5_TX_7BIT		0x20
#define	WRITE5_TX_8BIT		0x60
#define	WRITE5_DTR		0x80

/* 
 * Bits for write register 6 
 *
 * Synch characters or SDLC address field.
 *
 * (See page 4-8)
 */

/* 
 * Bits for write register 7 
 *
 * Sync character or SDLC flag.
 *
 * (See page 4-8)
 */

/* 
 * Bits for write register 8 
 *
 * Transmit buffer.
 *
 * (See page 4-9)
 */

/* 
 * Bits for write register 9 
 *
 * Master interrupt control.
 *
 * (See page 4-9)
 */

#define WRITE9_VECTOR_INCL_STAT	0x01	
#define WRITE9_NO_VECTOR	0x02
#define WRITE9_DIS_LOWER_CHAIN	0x04
#define WRITE9_MASTER_IE	0x08
#define WRITE9_STAT_HIGH	0x10
#define WRITE9_RESET_CHAN_B	0x40
#define WRITE9_RESET_CHAN_A	0x80
#define WRITE9_RESET_WORLD	0xC0

/* 
 * Bits for write register 10 
 *
 * Miscellaneous transmitter/receiver control bits.
 *
 * (See page 4-10)
 */

/* 
 * Bits for write register 11 
 *
 * (See page 4-11 - 4-13)
 */

#define WRITE11_TRXC_XTAL	0x00	
#define WRITE11_TRXC_XMIT	0x01
#define WRITE11_TRXC_BAUD	0x02
#define WRITE11_TRXC_DPLL	0x03
#define WRITE11_TRXC_OUT_ENA	0x04
#define WRITE11_TXCLK_RTXC	0x00
#define WRITE11_TXCLK_TRXC	0x08
#define WRITE11_TXCLK_BAUD	0x10
#define WRITE11_TXCLK_DPLL	0x18
#define WRITE11_RXCLK_RTXC	0x00
#define WRITE11_RXCLK_TRXC	0x20
#define WRITE11_RXCLK_BAUD	0x40
#define WRITE11_RXCLK_DPLL	0x60
#define WRITE11_RTXC_XTAL	0x80

/* bits in WR14 -- misc control bits, and DPLL control */
#define WRITE14_BAUD_ENABLE	0x01
#define WRITE14_BAUD_FROM_PCLK	0x02
#define WRITE14_DTR_IS_REQUEST	0x04
#define WRITE14_AUTO_ECHO	0x08
#define WRITE14_LOCAL_LOOPBACK	0x10
#define WRITE14_DPLL_NOP	0x00
#define WRITE14_DPLL_SEARCH	0x20
#define WRITE14_DPLL_RESET	0x40
#define WRITE14_DPLL_DISABLE	0x60
#define WRITE14_DPLL_SRC_BAUD	0x80
#define WRITE14_DPLL_SRC_RTXC	0xA0
#define WRITE14_DPLL_FM		0xC0
#define WRITE14_DPLL_NRZI	0xE0


/*
 *-------------------------------------------------------------------
 * 
 * Zilog device struct used to access read register 0.
 *
 *-------------------------------------------------------------------
 */


typedef struct {
	unsigned char	control;
	unsigned char	:8;		
	unsigned char	data;
	unsigned char	:8;	
} DevZ8530Device;

/*
 * The following macro can be used to generate the baud rate generator's
 * time constants.  The parameters are the input clock to the baud rate
 * generator (eg, 5000000 for 5MHz) and the desired baud rate.  This 
 * macro assumes that the clock needed is 16 times the desired baud rate.
 */

#define ZilogTimeConst(inputClock, baudRate) (( inputClock / (2*baudRate*16)) - 2)

#define	PCLK		(19660800/4)
#define	ZILOG_SPEED(n)	ZilogTimeConst(PCLK, n)

/*
 *-----------------------------------------------------------------
 *
 * For each channel of a Z8530 chip there is one structure of the
 * following form, which provides information about the channel.
 *
 *-----------------------------------------------------------------
 */

typedef struct {
    char *name;				/* Name to use for device in console
					 * error messages. */
    DevZ8530Device *address;		/* Address of device registers for
					 * this channel. */
    DevTty *ttyPtr;			/* Information about the logical
					 * terminal associated with the line. */
    int vector;				/* Interrupt vector to use for the
					 * channel. */
    int baud;				/* Current baud rate for channel
					 * (9600 means 9600 baud). */
    int wr3;				/* Value for write register 3
					 * (receiver characteristics). */
    int wr5;				/* Value for write register 5
					 * (transmitter characteristics). */
    void (*inputProc)();		/* Procedure to call at interrupt time
					 * to take input character.   See
					 * DevTtyInputChar for example of
					 * inputProc's structure. */
    ClientData inputData;		/* Argument to pass to inputProc. */
    int (*outputProc)();		/* Procedure to call at interrupt time
					 * to get next output character.   See
					 * DevTtyOutputChar for example of
					 * outputProc's calling structure. */
    ClientData outputData;		/* Argument to pass to outputProc. */
    int oldRr0;				/* Previous value of read reg. 0;  used
					 * to detect changes in status bits. */
    int flags;				/* See below for definitions. */
} DevZ8530;

/*
 * Flag values for DevZ8530 structures:
 *
 * Z_CHANNEL_B:				1 means channel B, 0 means A.
 * Z_BREAK:				1 means break condition has been
 *					detected but not yet cleared.
 * Z_INACTIVE:				1 means device is not in use, so don't
 *					ignore it.
 */

#define Z_CHANNEL_A		0
#define Z_CHANNEL_B		1
#define Z_BREAK			2
#define Z_INACTIVE		4

/*
 * Exported procedures:
 */

extern void		DevZ8530Activate(void *ptr);
extern Boolean		DevZ8530Interrupt(ClientData	clientData);
extern int		DevZ8530RawProc(void *ptr, int operation, 
				int inBufSize, char *inBuffer, int outBufSize,
				char *outBuffer);

#endif /* _DEVZ8530 */
