/*
 * devAddrs.h --
 *
 *     Addresses of devices on the Sun.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVADDRS
#define _DEVADDRS
#ifdef SUN3
#define	DEV_KBD_ADDR			0xfe00000
#define	DEV_ZILOG_SERIAL_ADDR		0xfe02000
#define	DEV_TIMER_ADDR			0xfe06000
#define	DEV_INTERRUPT_REG_ADDR		0xfe0a000
#define DEV_VIDEO_MEM_ADDR		0xfe20000
#else
#define	DEV_KBD_ADDR			0xeec000
#define	DEV_TIMER_ADDR			0xee0000
#define	DEV_ZILOG_SERIAL_ADDR		0xeec800
#define DEV_VIDEO_MEM_ADDR		0xec0000
#define DEV_VIDEO_CTRL_REG_ADDR		0xEE3800
#endif
#endif _DEVADDRS
