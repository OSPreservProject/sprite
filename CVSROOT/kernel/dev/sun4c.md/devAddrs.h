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
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVADDRS
#define _DEVADDRS

#define	DEV_KBD_ADDR			0xffd00004
#define	DEV_MOUSE_ADDR			0xffd00000
#define	DEV_SERIALA_ADDR		0xffd02004
#define	DEV_SERIALB_ADDR		0xffd02000
#define	DEV_TIMER_ADDR			0xffd04000
#define	DEV_COUNTER_ADDR		0xffd06000
#define	DEV_INTERRUPT_REG_ADDR		0xffd0a000
#define	DEV_FRAME_BUF_ADDR		0xffd80000
/*
 * Interrupt vector assignments:
 */

#define DEV_UART_VECTOR			30

#endif /* _DEVADDRS */
