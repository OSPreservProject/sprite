/* 
 * userIO.h --
 *
 *	Emulate C library IO routines for use in kernel.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef _userIO
#define _userIO

#include "fs.h"

extern Fs_Stream *open();

#endif _userIO
