/*
 * mouse.h --
 *
 *	Declarations for things exported by devMouse.c to the rest
 *	of the device module.
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

#ifndef _DEVMOUSE
#define _DEVMOUSE

extern ReturnStatus	DevMouseClose();
extern void		DevMouseInit();
extern void		DevMouseInterrupt();
extern ReturnStatus	DevMouseIOControl();
extern ReturnStatus	DevMouseOpen();
extern ReturnStatus	DevMouseRead();
extern ReturnStatus	DevMouseSelect();
extern ReturnStatus	DevMouseWrite();

#endif /* _DEVMOUSE */
