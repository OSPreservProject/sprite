/*
 * byte.h --
 *
 *	This file defines the facilities provided for copying,
 *	comparing, and zeroing byte arrays.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _BYTE
#define _BYTE

#include "sprite.h"

extern void Byte_Copy();
extern void Byte_Fill();
extern void Byte_Zero();
extern Boolean Byte_Compare();

/*
 * Byte_FillBuffer is used to copy a value into a buffer and advance
 * the pointer into the buffer by the size of the object copied.
 * Note that pointer must be defined as a character pointer because it is
 * advanced by sizeof(type) characters.
 *
 * Byte_EmptyBuffer assigns into a variable, given a pointer and the type
 * of the variable, then advances the pointer.
 */

#define Byte_FillBuffer(pointer, type, value) \
		* ((type *) pointer) = (value) ; \
		pointer += sizeof(type)

#define Byte_EmptyBuffer(pointer, type, dest) \
		dest = * ((type *) pointer); \
		pointer += sizeof(type)

/*
 * Character arrays (strings & character buffers) are set up to be
 * padded to make sure that integers are aligned on even addresses.
 * Byte_AlignAddr rounds lengths to the next even number.
 */

#define Byte_AlignAddr(address) ((address + 1) & ~1)

#endif _BYTE
