/*
 * devUart.h --
 *
 * Copyright (C) 1989 by Digital Equipment Corporation, Maynard MA
 *
 *			All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  
 *
 * Digitial disclaims all warranties with regard to this software, including
 * all implied warranties of merchantability and fitness.  In no event shall
 * Digital be liable for any special, indirect or consequential damages or
 * any damages whatsoever resulting from loss of use, data or profits,
 * whether in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of this
 * software.
 *
 * $Header$ SPRITE (DECWRL)
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

extern	void 	Dev_UartInit();
extern	void 	Dev_UartStartTx();

#endif _DEVUART




