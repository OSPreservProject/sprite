/*
 * devUart.h --
 *
 *     Types, constants, and macros for the SPUR Signetics SCN68681
 *     Dual Universal Asynchronous Receiver/Transmitter (DUART) chip.

 *     For an explanation of the use of the chip see the "SPUR Memory
 *     System Architecture" (MSA), Report. UCB/CSD 87/394.
 *
 * Copyright 1988 Regents of the University of California
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

#ifndef _DEVUART
#define _DEVUART

/*
 * Routines that access the uart refer to either the A channel or B
 * channel.  This typedef allows them to store the type of channel
 * and pass this information to the initialization routine.
 */
typedef enum {
    DEV_UART_CHANNEL_A,
    DEV_UART_CHANNEL_B,
} Dev_UartChannel;

/*
 * When a single pointer to the uart structure is treated as a ClientData,
 * it is necessary to have a single structure containing both the
 * address of the device registers and the channel to be used.  This way
 * the caller need only handle a single opaque pointer rather than needing
 * to call a machine-dependent routine with or without information defining
 * the channel.
 */
typedef struct {
    Address uartAddress;
    Dev_UartChannel channel;
} Dev_UartInfo;

/*
 * The uart registers are read in physical mode.  The address is a base
 * address for the uart, plus an offset corresponding to the register.
 */
# define Dev_UartWriteReg(base, offset, data) \
	Mach_WritePhysicalWord((base) + (offset), data)

# define Dev_UartReadReg(base, offset) \
	Mach_ReadPhysicalWord((base) + (offset))

#define	DEV_UART_SERIAL_SPEED    9600

extern	void 	Dev_UartInit();
extern	void 	Dev_UartStartTx();
extern	void 	Dev_UartWriteChar();
extern	int 	Dev_UartReadChar();

#endif _DEVUART




