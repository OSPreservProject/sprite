/*
 * spriteEmuMach.h --
 *
 *	sun3 declarations for the sprited emulator library.
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
 * $Header: /r3/kupfer/spriteserver/src/lib/c/emulator/sun3.md/RCS/spriteEmuMach.h,v 1.1 91/10/04 12:13:39 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SPRITEEMUMACH
#define _SPRITEEMUMACH

#include <sprite.h>

/* 
 * This is the number of words that fork() needs to save to set up the 
 * state of the child.
 */
#define SPRITEEMUMACH_STATE_ARRAY_WORDS	17

extern Address SpriteEmuMach_ChildInit();
extern Address SpriteEmuMach_SaveState();

#endif /* _SPRITEEMUMACH */
