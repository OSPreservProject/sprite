/*
 * devCCC.h --
 *
 *	Declarations of data and procedures for the SPUR CC device.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVCC
#define _DEVCC

/*
** Current Counter Mode
*/
extern int dev_CurrentCounterMode;


/*
 * Forward Declarations.
 */
extern ReturnStatus Dev_CCOpen();
extern ReturnStatus Dev_CCRead();
extern ReturnStatus Dev_CCWrite();
extern ReturnStatus Dev_CCIOControl();
extern ReturnStatus Dev_CCClose();
extern ReturnStatus Dev_CCSelect();
extern ReturnStatus Dev_CCReopen();

extern ReturnStatus Dev_PCCOpen();
extern ReturnStatus Dev_PCCRead();
extern ReturnStatus Dev_PCCWrite();
extern ReturnStatus Dev_PCCIOControl();
extern ReturnStatus Dev_PCCClose();
extern ReturnStatus Dev_PCCSelect();
extern ReturnStatus Dev_PCCReopen();

#endif _DEVCC
