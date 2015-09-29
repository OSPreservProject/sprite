/*
 * spriteEmuMach.h --
 *
 *	DECstation declarations for the sprited emulator library.
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
 * $Header: /r3/kupfer/spriteserver/src/lib/c/emulator/ds3100.md/RCS/spriteEmuMach.h,v 1.1 91/11/14 11:33:49 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _SPRITEEMUMACH
#define _SPRITEEMUMACH

#ifdef LANGUAGE_C
#include <sprite.h>
#endif

/* 
 * The following are offsets into the buffer that holds a new child's
 * state. 
 */
#define STATE_RA	0
#define STATE_GP	4
#define STATE_SP	8
#define STATE_S0	12
#define STATE_S1	16
#define STATE_S2	20
#define STATE_S3	24
#define STATE_S4	28
#define STATE_S5	32
#define STATE_S6	36
#define STATE_S7	40
#define STATE_S8	44
#define STATE_FPA_CTL	48
#define STATE_F20	52
#define STATE_F21	56
#define STATE_F22	60
#define STATE_F23	64
#define STATE_F24	68
#define STATE_F25	72
#define STATE_F26	76
#define STATE_F27	80
#define STATE_F28	84
#define STATE_F29	88
#define STATE_F30	92
#define STATE_F31	96

/* 
 * This is the number of words that fork() needs to save to set up the 
 * state of the child.
 */
#define SPRITEEMUMACH_STATE_ARRAY_WORDS	((STATE_F31 + 4)/4)

#ifdef LANGUAGE_C
extern Address SpriteEmuMach_ChildInit();
extern Address SpriteEmuMach_SaveState();
#endif

#endif /* _SPRITEEMUMACH */
