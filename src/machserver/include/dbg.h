/*
 * dbg.h --
 *
 *	Stub debugger declarations.  This file may get more useful 
 *	stuff put in it if we ever want to run the Sprite server 
 *	standalone.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/include/RCS/dbg.h,v 1.3 92/04/02 18:46:43 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _DBG
#define _DBG

#include <sprite.h>
#include <stdlib.h>

/* Hack for source compatibility. */
#define dbg_UsingNetwork	FALSE

/*
 * Debugger using syslog to dump output of call command or not.
 */
#if 0
extern	Boolean	dbg_UsingSyslog;
#endif
#define dbg_UsingSyslog	FALSE

/*
 * Macro to get the debugger invoked.  Unlike in native Sprite, this
 * call isn't continuable.
 */

#define DBG_CALL	abort()

#endif /* _DBG */
