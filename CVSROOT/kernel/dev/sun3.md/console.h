/*
 * console.h --
 *
 *	Declarations for things exported by devConsole.c to the rest
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

#ifndef _DEVCONSOLE
#define _DEVCONSOLE

#include "z8530.h"

extern int DevConsoleConvertKeystroke _ARGS_((int value));
extern void DevConsoleInputProc _ARGS_((DevTty *ttyPtr, int value));
extern int DevConsoleRawProc _ARGS_((void *ptr, int operation,
    int inBufSize, char *inBuffer, int outBufSize, char *outBuffer));

#endif /* _DEVCONSOLE */
