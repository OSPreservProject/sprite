/*
 * compatInt.h --
 *
 *	Declarations of routines used to implement Unix system calls
 *	in terms of Sprite ones.  When Sprite gets converted to
 *	implement the Unix system calls directly, this file should
 *	go away.
 *
 * Copyright 1986, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Compat: proto.h,v 1.3 86/02/14 09:47:40 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _COMPATINT
#define _COMPATINT

#include "sprite.h"

/*
 * UNIX_ERROR is the value Unix system calls return upon error.
 * UNIX_SUCCESS is the value Unix system calls return if they don't
 * return anything interesting and there has been no error.
 */

#define UNIX_ERROR -1
#define UNIX_SUCCESS 0

/*
 * Unix error code is stored in external variable errno.
 */

extern int errno;

/*
 * Define a few routines to map Sprite constants to Unix and vice-versa.
 */

extern ReturnStatus Compat_MapCode();
extern ReturnStatus Compat_UnixSignalToSprite();
extern ReturnStatus Compat_SpriteSignalToUnix();
extern ReturnStatus Compat_UnixSigMaskToSprite();
extern ReturnStatus Compat_SpriteSigMaskToUnix();
extern ReturnStatus Compat_GetSigMask();

#endif _COMPATINT
