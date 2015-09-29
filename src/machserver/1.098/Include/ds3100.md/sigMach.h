/*
 * sigMach.h --
 *
 *     Machine dependent data structures and procedure headers exported by the
 *     the signal module.  These are are for the sparc hardware.
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
 * $Header: /sprite/src/lib/include/ds3100.md/RCS/sigMach.h,v 1.1 89/07/08 14:56:07 nelson Exp $ SPRITE (Berkeley)
 */

#ifndef _SIGMACH
#define _SIGMACH

/*
 * The different machine dependent codes for an arithmetic fault.
 */
#define SIG_OVERFLOW	1

/*
 * The different machine dependent codes for an illegal instruction signal
 */
#define SIG_FP_EXCEPTION	4

#endif _SIGMACH
