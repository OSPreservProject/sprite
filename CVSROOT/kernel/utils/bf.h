/*
 * bf.h --
 *
 *	Macros to manipulate bits in an array of bytes, shorts, or words,
 *	simulating the effect of bit fields.
 *
 * Copyright 1990 Regents of the University of California
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

#ifndef _BF
#define _BF

/*
 * There are a number of formulas that are used to compute which bits
 * within the array are to be manipulated. The following definitions
 * and formulas are used throughout the bf macro package. In the 
 * definitions "left" refers to lower addresses when it refers to array
 * elements, and to the high order bit when it refers to bits within a
 * byte (element).
 *
 *	d = left offset of bit field within the array, in bits.
 *	s = size of the bit field, in bits.
 *	u = size of array elements, in bits.
 *	v = value to assign to or test the bit field with.
 *	x = index of current element of array that is being manipulated,
 *		where x = 0 is the index for the left-most element that
 *		contains part of the bit field.
 *	Dx = left offset of bit field within array element x.
 *	Sx = size of bit field in array element x.
 *	Rx = number of bits in bit field that are to the right of element
 *		x.
 *	Qx = number of bits in element x that are to the right of the
 *		bit field.
 *	Vx = the value of v associated with the bit field within element
 *		x.
 *	Ix = index of element x within the entire array of elements.
 *	Zx = the value of the bit field within element x.
 *
 * For example, consider the following array of bytes, where '|' denotes
 * byte boundaries, '#' denotes the bits of the bit field, and '_' denotes
 * bits that are not in the bit field:
 *
 *	|________|____####|##______| 	(high memory ->)
 *
 * which contains the following values:
 *
 *	|00000000|00001111|11000000|	(binary)
 *
 * Further more let v (the value we want to assign to the bit field) be
 * 101010 binary.  Then we have the following for the variables.  All
 * values are decimal except where noted.
 *	
 *	d = 12
 *	s = 6
 *	u = 8
 *	v = 101010 (binary)
 *	D0 = 4
 *	D1 = 0
 *	S0 = 4
 *	S1 = 2
 *	R0 = 2
 *	R1 = 0
 *	Q0 = 0
 *	Q1 = 6
 *	V0 = 1010 (binary)
 *	V1 = 10000000 (binary)
 *	I0 = 1
 *	I2 = 2
 *	Z0 = 1111 (binary)
 *	Z1 = 11 (binary)
 *
 * The values for d, s, u, and v, are dependent on the format of the
 * bit field.  The values of the other variables can be computed from
 * the following equations.
 *
 * 	D0 = d % u
 *	D1, D2, ... = 0
 *	Sx = min(u - Dx, s - (S0 + S1 + ... + S(x-1)))
 *	Qx = u - (Dx + Sx)
 *	Rx = s - (S0 + S1 + ... + Sx)
 *	Vx = ((v >> Rx) & mask(Sx)) << Qx
 *	Zx = array[Ix] & Vx  (v = the biggest mask in this case)
 *
 * where
 *	min(a,b) computes the minimum of a and b
 *	mask(k) produces a number consisting of x bits set to 1
 *	% is the remainder function.
 */


/*
 *----------------------------------------------------------------------
 *
 * BfMin --
 *
 *	Compute the minimum of two numbers.
 *
 * Results:
 *	The minimum of the two parameters.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfMin(a, b)    ((a) < (b) ? (a) : (b))


/*
 *----------------------------------------------------------------------
 *
 * BfMask --
 *
 *	Creates a mask (all bits are 1) with the number of bits indicated
 *	by the size parameter.
 *
 * Results:
 *	The mask value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfMask(size)   ((1 << (size)) - 1)


/*
 *----------------------------------------------------------------------
 *
 * BfDx --
 *
 *	Computes the value Dx
 *
 * Results:
 *	Dx
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfDx(u, x, d)  (((x) == 0) ? ((d) % (u)) : 0)


/*
 *----------------------------------------------------------------------
 *
 * BfS0, BfS1, BfS2, BfS3, BfS4 --
 *
 *	Computes the values S0, S1, S2, S3, and S4.
 *
 * Results:
 *	S0 or S1 or S2 or S3 or S4.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfS0(u, d, s)     (BfMin(u - BfDx(u, 0, d), s))

#define BfS1(u, d, s)     (BfMin(u - BfDx(u, 1, d), (s) - BfSumS0(u, d, s)))

#define BfS2(u, d, s)     (BfMin(u - BfDx(u, 2, d), (s) - BfSumS1(u, d, s)))

#define BfS3(u, d, s)     (BfMin(u - BfDx(u, 3, d), (s) - BfSumS2(u, d, s)))

#define BfS4(u, d, s)     (BfMin(u - BfDx(u, 4, d), (s) - BfSumS3(u, d, s)))

/*
 *----------------------------------------------------------------------
 *
 * BfByteSx, BfHalfwordSx, BfWordSx --
 *
 *	Computes Sx. There are three routines for the three possible
 *	values of u.
 *
 * Results:
 *	Sx.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfByteSx(x, d, s) 	\
    (((x) == 0) ? BfS0(8, d, s) : (((x) == 1) ? BfS1(8, d, s) : BfS2(8, d, s)))

#define BfHalfwordSx(x, d, s) 	\
    (((x) == 0) ? BfS0(16, d, s) : BfS1(16, d, s))

#define BfWordSx(x, d, s)  \
    (((x) == 0) ? BfS0(32, d, s) : BfS1(32, d, s))


/*
 *----------------------------------------------------------------------
 *
 * BfSumS0, BfSumS1, BfSumS2, BfSumS3,  BfSumS4 --
 *
 *	Computes S0 + S1 + ... SN for some N.
 *
 * Results:
 *	The sum from 0 to N of Si.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


#define BfSumS0(u, d, s)  (BfS0(u, d, s))


#define BfSumS1(u, d, s)  (BfS0(u, d, s) + BfS1(u, d, s))


#define BfSumS2(u, d, s)  (BfSumS1(u, d, s) + BfS2(u, d, s))


#define BfSumS3(u, d, s)  (BfSumS2(u, d, s) + BfS3(u, d, s))


#define BfSumS4(u, d, s)  (BfSumS3(u, d, s) + BfS4(u, d, s))


/*
 *----------------------------------------------------------------------
 *
 * BfSumByteSx, BfSumHalfwordSx, BfSumWordSx --
 *
 *	Computes the sum of S0 + S1 + ... + Sx.  There is one routine for 
 *	each value of u.
 *
 * Results:
 *	The sum.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfSumByteSx(x, d, s) \
    (((x) == 0) ? BfSumS0(8, d, s) : (((x) == 1) ? BfSumS1(8, d, s) : \
	BfSumS2(8, d, s)))


#define BfSumHalfwordSx(x, d, s) \
    (((x) == 0) ? BfSumS0(16, d, s) : BfSumS1(16, d, s))


#define BfSumWordSx(x, d, s)  \
    (((x) == 0) ? BfSumS0(32, d, s) : BfSumS1(32, d, s))


/*
 *----------------------------------------------------------------------
 *
 *  BfByteRx, BfHalfwordRx, BfWordRx--
 *
 *	Computes Rx.
 *
 * Results:
 *	Rx
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfByteRx(x, d, s)   ((s) - BfSumByteSx(x, d, s))
#define BfHalfwordRx(x, d, s)   ((s) - BfSumHalfwordSx(x, d, s))
#define BfWordRx(x, d, s)   ((s) - BfSumWordSx(x, d, s))


/*
 *----------------------------------------------------------------------
 *
 * BfByteQx, BfHalfwordQx, BfWordQx --
 *
 *	Computes Qx.
 *
 * Results:
 *	Qx.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfByteQx(x, d, s) (8 - (BfDx(8, x, d) + BfByteSx(x, d, s)))
#define BfHalfwordQx(x, d, s) (16 - (BfDx(16, x, d) + BfHalfwordSx(x, d, s)))
#define BfWordQx(x, d, s) (32 - (BfDx(32, x, d) + BfWordSx(x, d, s)))


/*
 *----------------------------------------------------------------------
 *
 * BfByteVx, BfHalfwordVx, BfWordVx --
 *
 *	Computes Vx
 *
 * Results:
 *	Vx.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfByteVx(x, d, s, v) \
    ((((v) >> BfByteRx(x, d, s)) & BfMask(BfByteSx(x, d, s))) << \
	BfByteQx(x, d, s))

#define BfHalfwordVx(x, d, s, v) \
    ((((v) >> BfHalfwordRx(x, d, s)) & BfMask(BfHalfwordSx(x, d, s))) << \
	BfHalfwordQx(x, d, s))

#define BfWordVx(x, d, s, v) \
    ((((v) >> BfWordRx(x, d, s)) & BfMask(BfWordSx(x, d, s))) << \
	BfWordQx(x, d, s))


/*
 *----------------------------------------------------------------------
 *
 * BfIx --
 *
 *	Computes Ix
 *
 * Results:
 *	Ix
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfIx(u, x, d) (((d) / (u)) + (x))


/*
 *----------------------------------------------------------------------
 *
 * BfByteZx, BfHalfwordZx, BfWordZx --
 *
 *	Computes Zx.
 *
 * Results:
 *	Zx.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfByteZx(ptr, x, d, s) \
    ((ptr)[BfIx(8, x, d)] & BfByteVx(x, d, s, 0xffffffff))

#define BfHalfwordZx(ptr, x, d, s) \
    ((ptr)[BfIx(16, x, d)] & BfHalfwordVx(x, d, s, 0xffffffff))

#define BfWordZx(ptr, x, d, s) \
    ((ptr)[BfIx(32, x, d)] & BfWordVx(x, d, s, 0xffffffff))


/*
 *----------------------------------------------------------------------
 *
 * BfByteSetx, BfHalfwordSetx, BfWordSetx --
 *
 *	Stores the value Vx into the xth element containing the bit field.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define BfByteSetx(ptr, x, d, s, v)  {					\
    if ((v) != BfMask(s)) {						\
	(ptr)[BfIx(8, x, d)] &= ~BfByteVx(x, d, s, 0xffffffff); 	\
    }									\
    (ptr)[BfIx(8, x, d)] |= BfByteVx(x, d, s, v); 			\
}

#define BfHalfwordSetx(ptr, x, d, s, v)  {				\
    if ((v) != BfMask(s)) {						\
	(ptr)[BfIx(16, x, d)] &= ~BfHalfwordVx(x, d, s, 0xffffffff); 	\
    }									\
    (ptr)[BfIx(16, x, d)] |= BfHalfwordVx(x, d, s, v);			\
}

#define BfWordSetx(ptr, x, d, s, v)  {					\
    if ((v) != BfMask(s)) {						\
	((ptr)[BfIx(32, x, d)]) &= ~BfWordVx(x, d, s, 0xffffffff); 	\
    }									\
    ((ptr)[BfIx(32, x, d)]) |= BfWordVx(x, d, s, v); 			\
}


/*
 *----------------------------------------------------------------------
 *
 * BfByteTestx, BfHalfwordTestx, BfWordTestx --
 *
 *	Tests the xth element containing the bit field against the value
 *	Vx.
 *
 * Results:
 *	1 if Zx == Vx, 0 otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


#define BfByteTestx(ptr, x, d, s, v) \
    (BfByteZx(ptr, x, d, s) == BfByteVx(x, d, s, v))

#define BfHalfwordTestx(ptr, x, d, s, v) \
    (BfHalfwordZx(ptr, x, d, s) == BfHalfwordVx(x, d, s, v))

#define BfWordTestx(ptr, x, d, s, v) \
    (BfWordZx(ptr, x, d, s) == BfWordVx(x, d, s, v))


/*
 *----------------------------------------------------------------------
 *
 * Bf_ByteSet, Bf_HalfwordSet, Bf_WordSet --
 *
 *	Set the bit field defined by ptr, d and s with the value v.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define Bf_ByteSet(ptr, d, s, v)  {				\
    BfByteSetx(ptr, 0, d, s, v);				\
    if (BfByteRx(0, d, s) > 0) {				\
	BfByteSetx(ptr, 1, d, s, v);				\
	if (BfByteRx(1, d, s) > 0) {				\
	    BfByteSetx(ptr, 2, d, s, v);			\
	}							\
    }								\
}

#define Bf_HalfwordSet(ptr, d, s, v)  {				\
    BfHalfwordSetx(ptr, 0, d, s, v);				\
    if (BfHalfwordRx(0, d, s) > 0) {				\
	BfHalfwordSetx(ptr, 1, d, s, v);			\
    }								\
}

#define Bf_WordSet(ptr, d, s, v)  {				\
    BfWordSetx(ptr, 0, d, s, v);				\
    if (BfWordRx(0, d, s) > 0) {				\
	BfWordSetx(ptr, 1, d, s, v);				\
    }								\
}


/*
 *----------------------------------------------------------------------
 *
 * Bf_ByteTest, Bf_HalfwordTest, Bf_WordTest --
 *
 *	Compare the bitfield defined by ptr, d, and s with the value v.
 *
 * Results:
 *	1 if the value in the bitfield equals v, 0 otherwise
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define Bf_ByteTest(ptr, d, s, v) 				\
    ((BfByteRx(0, d, s) <= 0) ? 				\
	(BfByteTestx(ptr, 0, d, s, v)) : 			\
	((BfByteRx(1, d, s) <= 0) ?				\
	    (BfByteTestx(ptr, 0, d, s, v) &&			\
	    BfByteTestx(ptr, 1, d, s, v)) :			\
	    (BfByteTestx(ptr, 0, d, s, v) &&			\
	    BfByteTestx(ptr, 1, d, s, v) &&			\
	    BfByteTestx(ptr, 2, d, s, v))))

#define Bf_HalfwordTest(ptr, d, s, v) 				\
    ((BfHalfwordRx(0, d, s) <= 0) ? 				\
	(BfHalfwordTestx(ptr, 0, d, s, v)) :			\
	(BfHalfwordTestx(ptr, 0, d, s, v) &&			\
	BfHalfwordTestx(ptr, 1, d, s, v)))

#define Bf_WordTest(ptr, d, s, v) 				\
    ((BfWordRx(0, d, s) <= 0) ? 				\
	(BfWordTestx(ptr, 0, d, s, v)) :			\
	(BfWordTestx(ptr, 0, d, s, v) &&			\
	BfWordTestx(ptr, 1, d, s, v)))



/*
 *----------------------------------------------------------------------
 *
 * Bf_ByteGet, Bf_HalfwordGet, Bf_WordGet --
 *
 *	Returns the value in the bitfield defined by ptr, d and s.
 *
 * Results:
 *	The value of the bitfield.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#define Bf_ByteGet(ptr, d, s)	 				\
    ((BfByteRx(0, d, s) == 0) ? 				\
	(BfByteZx(ptr, 0, d, s) >> BfByteQx(0, d, s)) : 	\
	((BfByteZx(ptr, 0, d, s) << BfByteRx(0, d, s)) |	\
	((BfByteRx(1, d, s) == 0) ?   				\
	    (BfByteZx(ptr, 1, d, s) >> BfByteQx(1, d, s)) : 	\
	    ((BfByteZx(ptr, 1, d, s) << BfByteRx(1, d, s)) |	\
	    (BfByteZx(ptr, 2, d, s) >> BfByteQx(2, d, s))))))

#define Bf_HalfwordGet(ptr, d, s)	 				\
    ((BfHalfwordRx(0, d, s) == 0) ? 					\
	(BfHalfwordZx(ptr, 0, d, s) >> BfHalfwordQx(0, d, s)) : 	\
	((BfHalfwordZx(ptr, 0, d, s) << BfHalfwordRx(0, d, s)) |	\
	    (BfHalfwordZx(ptr, 1, d, s) >> BfHalfwordQx(1, d, s))))

#define Bf_WordGet(ptr, d, s)	 				\
    ((BfWordRx(0, d, s) == 0) ? 				\
	(BfWordZx(ptr, 0, d, s) >> BfWordQx(0, d, s)) : 	\
	((BfWordZx(ptr, 0, d, s) << BfWordRx(0, d, s)) |	\
	    (BfWordZx(ptr, 1, d, s) >> BfWordQx(1, d, s))))

#endif /* _BF */

