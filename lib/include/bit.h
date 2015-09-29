/*
 * bit.h --
 *
 *	Definition of macros for setting and clearing bits in an array
 *	of integers.
 *
 *	It is assumed that "int" is 32 bits wide.
 *
 * Copyright 1987, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/bit.h,v 1.5 91/07/26 17:13:33 mendel Exp $ SPRITE (Berkeley)
 */

#ifndef _BIT
#define _BIT

#ifndef _SPRITE
#include "sprite.h"
#endif
#include <cfuncproto.h>
#include <bstring.h>

#define BIT_NUM_BITS_PER_INT	32
#define BIT_NUM_BITS_PER_BYTE	8

#define Bit_NumInts(numBits)	\
	(((numBits)+BIT_NUM_BITS_PER_INT -1)/BIT_NUM_BITS_PER_INT)

#define Bit_NumBytes(numBits)	\
	(Bit_NumInts(numBits) * sizeof(int))

#define Bit_Alloc(numBits, bitArrayPtr)  	\
        bitArrayPtr = (int *) malloc((unsigned) Bit_NumBytes(numBits)); \
        Bit_Zero((numBits), (bitArrayPtr))

#define Bit_Free(bitArrayPtr)	\
        free((char *)bitArrayPtr)

#define Bit_Set(numBits, bitArrayPtr) \
	((bitArrayPtr)[(numBits)/BIT_NUM_BITS_PER_INT] |= \
		(1 << ((numBits) % BIT_NUM_BITS_PER_INT)))

#define Bit_IsSet(numBits, bitArrayPtr) \
	((bitArrayPtr)[(numBits)/BIT_NUM_BITS_PER_INT] & \
		(1 << ((numBits) % BIT_NUM_BITS_PER_INT)))

#define Bit_Clear(numBits, bitArrayPtr) \
	((bitArrayPtr)[(numBits)/BIT_NUM_BITS_PER_INT] &= \
		~(1 << ((numBits) % BIT_NUM_BITS_PER_INT)))

#define Bit_IsClear(numBits, bitArrayPtr) \
	(!(Bit_IsSet((numBits), (bitArrayPtr))))

#define Bit_Copy(numBits, srcArrayPtr, destArrayPtr) \
	bcopy((char *) (srcArrayPtr), (char *) (destArrayPtr), \
		Bit_NumBytes(numBits))

#define Bit_Zero(numBits, bitArrayPtr) \
	bzero((char *) (bitArrayPtr), Bit_NumBytes(numBits))

extern int	  Bit_FindFirstSet _ARGS_((int numBits, int *arrayPtr));
extern int	  Bit_FindFirstClear _ARGS_((int numBits, int *arrayPtr));
extern Boolean	  Bit_Intersect _ARGS_((int numBits, int *src1Ptr, int *src2Ptr, int *destPtr));
extern Boolean 	  Bit_Union _ARGS_((int numBits, int *src1Ptr, int *src2Ptr, int *destPtr));
extern Boolean 	  Bit_AnySet _ARGS_((int numBits, int *maskPtr));
extern int  	  *Bit_Expand _ARGS_((int newNumBits, int oldNumBits, int *oldBitPtr));
	 
#endif /* _BIT */
