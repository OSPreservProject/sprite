/*
**	uart.h
**	%Z% %M% %G% %I%
**	
**	Constants and macros for manipulating Spur board uart.
**
**	See Memory System Architecture (MSA) document, section 3.2.
**	(Dec. 87), or the part data sheet for
**	Signetics SCN68681 Dual Universal Asynchronous Receiver/Transmitter
**
**	For our purposes we will only use one of the two channels, channel A.
**	So I will not record definitions for the B channel entities.
*/

/* lets start with the cache controller interrupt status and mask */
/* the uart intr is bit 18 in these registers */

#define ISTATUS_UART_BYTE_ADDR	0xC40	/* RDREG & WRREG addresses */
#define UART_INTR		0x04	/* uart intr if this is set in above */
#define CLR_UART_INTR		UART_INTR /* clear by writing this back */
#define IMASK_UART_BYTE_ADDR	0xD40	/* RDREG & WRREG addresses */
#define TAKE_UART_INTRS		0x04	/* OR this into above mask register */
#define IGNORE_UART_INTRS	~TAKE_UART_INTRS	/* AND into above */

/* and now for the UART itself */

#define UART_BASE 0x00010000	/* physical address base for uart */

/* register address offsets in uart physical address space
   see MSA Table 3.1 */

#define A_CHANNEL	0x0c		/* rd/wr access to RX and TX fifo */
#define B_CHANNEL	0x2c		/* rd/wr access to RX and TX fifo */
#define DATA		A_CHANNEL	/* recommended channel */
#define MODE		0x00		/* rd/wr */
#define BAUD		0x04		/* wr */
#define XFER_STATUS	0x04		/* rd */
#define INTR_STATUS	0x14		/* rd */
#define INTR_MSK	0x14		/* wr */
#define COMMAND		0x08		/* wr */
#define AUX_CMD		0x10		/* wr */
#define IPCR		0x10		/* rd - reports changes in input bits */
#define IVR		0x30		/* rd/wr - intr vector reg */
					/* unused; allows rd/wr uart reg test */


/* mode register setup values
   first write after RESET_MR1 writes MR1, subsequent writes access MR2 */

#define MR1_VAL	0x13	/* 8b, no parity, no flow cntl, rx intr if not empty */
#define MR2_VAL 0x07	/* 1 stop bit, no flow cntl from other end */
#define MR2_LOOPBACK 0x87 /* same as MR2_VAL but in LOOPBACK mode */

/* baud rate values */

#define BAUD_9600	0xbb	/* recommended */
#define BAUD_2400	0x88
#define BAUD_1200	0x66
#define BAUD_110	0x11
#define BAUD_19200	0xcc

/* command register control values: MSA Table 3.2
   there are three independent field, rx cntl, tx cntl, overall cntl */

#define RX_NOOP		0x0
#define RX_ON		0x1
#define RX_OFF		0x2
#define TX_NOOP		0x0
#define TX_ON		0x4
#define TX_OFF		0x8
#define MISC_NOOP	0x00
#define RESET_MR1	0x10	/* next write to MODE register access MR1 */
#define RESET_RX	0x20	/* reset RX: dump fifo, status, etc */
#define RESET_TX	0x30	/* reset TX: dump fifo, status, etc */
#define CLR_ERR_STATUS	0x40	/* reset RXD error bits in status reg */
#define CLR_BRK_STATUS	0x50	/* reset break status in status regs */
#define ENTER_TX_BREAK	0x60	/* start transmitting break forever */
#define EXIT_TX_BREAK	0x70	/* stop transmitting break */

/* status register bit definitions: MSA Table 3.3 */

#define RXD_BREAK		0x80	/* received a break */
#define RXD_MISSING_STOP_BIT	0x40	/* "framing error" on last byte */
#define RXD_PARITY_ERROR	0x20	/* parity, if on, was wrong */
#define RXD_FIFO_OVRFLW		0x10	/* didn't read out fast enough */
#define TX_EMPTY		0x08	/* transmit fifo empty */
#define TX_NOT_FULL		0x04	/* still room left in tx fifo */
#define RX_FULL			0x02	/* no room left in receive fifo */
#define RX_NOT_EMPTY		0x01	/* something in the receive fifo */

#define RXD_ERROR_MASK		0xF0	/* check error bits only */

/* interrupt status register bit definitions: MSA Table 3.4 */
/* note that channel B is the channel we are ignoring in this
   configuration, and channel A is the implied channel elsewhere */

#define CHG_IP_3_0	0x80	/* uart input port bits have changed */
				/* reset by reading IPCR */
#define CHG_B_BREAK	0x40	/* break state changed on channel B */
				/* reset by CLR_BRK_STATUS in command reg */
#define B_FIFO_INTR	0x20	/* channel B fifo rx intr (config by MR1) */
				/* reset by reading from channel B */
#define B_TX_RDY	0x10	/* channel B fifo tx ready for more */
				/* reset by writing to channel B */
#define TIMER_INTR	0x08	/* counter or timer has rolled over */
				/* reset by read at offset 0x3c (stop timer) */
#define CHG_A_BREAK	0x04	/* break state changed on channel A */
#define CHG_BREAK CHG_A_BREAK	/* reset by CLR_BRK_STATUS in command reg */
#define A_FIFO_INTR	0x02	/* channel A fifo rx intr (config by MR1) */
#define FIFO_INTR A_FIFO_INTR	/* reset by reading from channel A */
#define A_TX_RDY	0x01	/* channel A fifo tx ready for more */
#define TX_RDY A_TX_RDY		/* reset by writing to channel A */

/* if only looking at A channel interrupt status */
#define INTR_STATUS_MASK CHG_BREAK|FIFO_INTR|TX_RDY

/* interrupt mask register values */

#define INTR_MASK	CHG_BREAK|FIFO_INTR	/* poll tx only */
#define INTR_MASK_POLL	CHG_BREAK		/* poll both sides */
#define INTR_MASK_DUMB	0x00			/* poll both sides and break */

/* auxiliary control register values */

#define AUX_CMD_VAL	0x80	/* ignore counter/timer and input port bits */
#define AUX_CMD_3.7MHZ_TIMER	0xE0	/* set timer freq to 3.6864 MHz */

/*
** So, lets discuss how to set this thing up and use it.
**
** In physical mode with Interrupts off, do the following:
**
** st_32 TX_OFF|RX_OFF|RESET_MR1 UART_BASE|COMMAND
** st_32 TX_OFF|RX_OFF|RESET_RX UART_BASE|COMMAND
** st_32 TX_OFF|RX_OFF|RESET_TX UART_BASE|COMMAND
** st_32 TX_OFF|RX_OFF|EXIT_TX_BREAK UART_BASE|COMMAND
** st_32 AUX_CMD_VAL UART_BASE|AUX_CMD
** st_32 MR1_VAL UART_BASE|MODE
** st_32 MR2_VAL UART_BASE|MODE
** st_32 BAUD_9600 UART_BASE|BAUD
** st_32 INTR_MASK_DUMB UART_BASE|INTR_MASK	or INTR_MASK_POLL etc
** st_32 TX_OFF|RX_OFF|CLR_ERR_STATUS UART_BASE|COMMAND
** st_32 TX_OFF|RX_OFF|CLR_BRK_STATUS UART_BASE|COMMAND
**
** WRREG ISTATUS_UART_BYTE_ADDR CLR_UART_INTR
**
** RDREG IMASK_UART_BYTE_ADDR
** OR with TAKE_UART_INTRS
** WRREG IMASK_UART_BYTE_ADDR
**
** Turn Interrupts on and cross your fingers ....
**
*/



/* to store to a uart register.
** assumes VOL_TEMP1 has been loaded with 
** Uart base.... */

#define st_uart(value,addr) \
	add_nt	VOL_TEMP2, r0, value ;\
	st_32	VOL_TEMP2, VOL_TEMP1, $addr 


