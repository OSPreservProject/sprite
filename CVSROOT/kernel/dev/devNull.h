/*
 * devNull.h --
 *
 *	Declarations of  procedures for the /dev/null device.
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

#ifndef _DEVNULL
#define _DEVNULL

/*
 * Forward Declarations.
 */
extern ReturnStatus Dev_NullOpen();
extern ReturnStatus Dev_NullRead();
extern ReturnStatus Dev_NullWrite();
extern ReturnStatus Dev_NullIOControl();
extern ReturnStatus Dev_NullClose();
extern ReturnStatus Dev_NullSelect();

#endif /* _DEVNULL */
