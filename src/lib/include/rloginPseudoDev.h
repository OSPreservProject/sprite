/*
 * rloginPseudoDev.h --
 *
 *	Shared declarations for programs that deal with remote login
 *	pseudo-devices.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/rloginPseudoDev.h,v 1.1 92/04/21 17:32:16 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _RLOGINPSEUDODEV
#define _RLOGINPSEUDODEV

/* 
 * Remote logins create a pseudo-device named ttyXXX, where XXX is some 
 * number to uniquify the name.
 */

#define RLOGIN_PDEV_NAME	"tty"

#endif /* _RLOGINPSEUDODEV */
