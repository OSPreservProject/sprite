/*
 * devAddrs.h --
 *
 *     Addresses and vector numbers for Sun-3 devices.
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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVADDRS
#define _DEVADDRS

/*
 * Addresses of devices:
 */

#define	DEV_KBD_ADDR			0xfe00004
#define DEV_MOUSE_ADDR			0xfe00000
#define DEV_SERIALA_ADDR		0xfe06004
#define DEV_SERIALB_ADDR		0xfe06000
#define	DEV_TIMER_ADDR			0xfe06000
#define	DEV_INTERRUPT_REG_ADDR		0xfe0a000
#define DEV_VIDEO_MEM_ADDR		0xfe20000

/*
 * Interrupt vector assignments:
 */

#define DEV_UART_VECTOR			30

#endif /* _DEVADDRS */
