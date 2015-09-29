/*
 * devAddrs.h --
 *
 *     Device and interrupt vector addresses for Sun-4's.
 *
 * Copyright 1985, 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/kernel/dev/sun4c.md/RCS/devAddrs.h,v 1.4 90/11/27 12:47:23 mgbaker Exp $ SPRITE (Berkeley)
 */

#ifndef _DEVADDRS
#define _DEVADDRS

/*
 * Should get these values from PROM using Mach_MonSearchProm().
 */
#define	DEV_KBD_ADDR			0xffee9004
#define	DEV_MOUSE_ADDR			0xffee9000
#define	DEV_SERIALA_ADDR		0xffeeb004
#define	DEV_SERIALB_ADDR		0xffeeb000
#define	DEV_TIMER_ADDR			0xffeef000
#define	DEV_COUNTER_ADDR		0xffeef000
#define	DEV_INTERRUPT_REG_ADDR		0xffeed000
#define	DEV_FRAME_BUF_ADDR		0xffd80000
#define	DEV_DMA_ADDR			0xffeb4dbc
/*
 * Interrupt vector assignments:
 */
#define DEV_UART_VECTOR			30


/*
 * Physical addresses for unmapped devices.
 */
#define	DEV_SCSI_ADDR			0xf8800000
#ifdef NOTDEF
/* This seems already to be mapped at a virtual address. */
#define	DEV_DMA_ADDR			0xf8400000
#endif NOTDEF


#endif /* _DEVADDRS */
