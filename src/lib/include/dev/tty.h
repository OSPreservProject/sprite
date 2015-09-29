/*
 * tty.h --
 *
 *	This file defines the device-dependent IOControl calls and related
 *	structres for "tty" devices.  These devices are supposed to behave
 *	identically to tty's in 4.2 BSD UNIX.  See the UNIX documentation
 *	for detailed documentation.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/dev/RCS/tty.h,v 1.5 90/07/16 22:52:06 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _TTY
#define _TTY

/*
 * Constants:  these are the IOControl operations defined for tty's.
 * For compatibility, the UNIX IOControl names are defined as well
 * as the corresponding Sprite names.
 */

#define IOC_TTY (1<<16)

#define IOC_TTY_GET_PARAMS	(IOC_TTY | 0x2)
#define IOC_TTY_SET_PARAMS	(IOC_TTY | 0x3)
#define IOC_TTY_GETP		IOC_TTY_GET_PARAMS
#define IOC_TTY_SETP		IOC_TTY_SET_PARAMS
#define IOC_TTY_SETN		(IOC_TTY | 0x4)
#define IOC_TTY_EXCL		(IOC_TTY | 0x5)
#define IOC_TTY_NXCL		(IOC_TTY | 0x6)
#define IOC_TTY_HUP_ON_CLOSE	(IOC_TTY | 0x7)
#define IOC_TTY_FLUSH		(IOC_TTY | 0x8)
#define IOC_TTY_INSERT_CHAR	(IOC_TTY | 0x9)
#define IOC_TTY_SET_BREAK	(IOC_TTY | 0xa)
#define IOC_TTY_CLEAR_BREAK	(IOC_TTY | 0xb)
#define IOC_TTY_SET_DTR		(IOC_TTY | 0xc)
#define IOC_TTY_CLEAR_DTR	(IOC_TTY | 0xd)
#define IOC_TTY_GET_TCHARS	(IOC_TTY | 0x11)
#define IOC_TTY_SET_TCHARS	(IOC_TTY | 0x12)
#define IOC_TTY_GET_LTCHARS	(IOC_TTY | 0x13)
#define IOC_TTY_SET_LTCHARS	(IOC_TTY | 0x14)
#define IOC_TTY_BIS_LM		(IOC_TTY | 0x15)
#define IOC_TTY_BIC_LM		(IOC_TTY | 0x16)
#define IOC_TTY_GET_LM		(IOC_TTY | 0x17)
#define IOC_TTY_SET_LM		(IOC_TTY | 0x20)
#define IOC_TTY_GET_DISCIPLINE	(IOC_TTY | 0x21)
#define IOC_TTY_SET_DISCIPLINE	(IOC_TTY | 0x22)
#define IOC_TTY_ADD_EVENT	(IOC_TTY | 0x23)
#define IOC_TTY_GET_TERMIO      (IOC_TTY | 0x24)
#define IOC_TTY_SET_TERMIO      (IOC_TTY | 0x25)
#define IOC_TTY_GET_WINDOW_SIZE	(IOC_TTY | 0x26)
#define IOC_TTY_SET_WINDOW_SIZE (IOC_TTY | 0x27)
#define IOC_TTY_NOT_CONTROL_TTY	(IOC_TTY | 0x28)

/*
 * The data structures defined below just duplicate the 4.2 BSD data
 * structures with the same names.  All of this should be eliminated
 * once the kernel has been changed to use the new ttyDriver.
 */

#ifndef _IOCTL
#include <sys/ioctl.h>
#endif

typedef struct sgttyb Tty_BasicParams;

typedef struct tchars Tty_Chars;

typedef struct ltchars Tty_LocalChars;

#endif _TTY
