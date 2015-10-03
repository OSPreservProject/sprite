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

#ifdef _ASM				/* force error if these are used */

#define	DEV_MOUSE_ADDR			%%
#define	DEV_KBD_ADDR			%%
#define	DEV_SERIALA_ADDR		%%
#define	DEV_SERIALB_ADDR		%%
#define	DEV_TIMER_ADDR			%%
#define	DEV_COUNTER_ADDR		%%
#define	DEV_INTERRUPT_REG_ADDR		%%
#define	DEV_FRAME_BUF_ADDR		%%
#define DEV_UART_VECTOR			%%

#else /* _ASM */

/* everything is grabbed from the PROM now! */

#endif /* _ASM */

#endif /* _DEVADDRS */
