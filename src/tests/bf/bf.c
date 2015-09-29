/* 
 * bf.c --
 *
 *	This program is a test of the "bf" macros.  Nothing is printed if
 *	all errors are successful, otherwise an error message is printed.
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

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.3 90/01/12 12:03:36 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "kernel/bf.h"


/*
 *----------------------------------------------------------------------
 *
 * FirstBitInFour --
 *
 *	Returns the offset, from the left, of the first bit that is
 *	set in the 4 least significant bits of an integer.
 *
 * Results:
 *	Left offset of first set bit, -1 if there isn't one.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
FirstBitInFour(value)
    unsigned int value;		/* The value in which to find the bit. */
{
    static int	first[16] = {-1, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};

    return first[(value & 0xf)];
}

/*
 *----------------------------------------------------------------------
 *
 * FirstBitSet --
 *
 *	Returns the left offset of the first bit that is set in the 
 *	given array of bytes.
 *
 * Results:
 *	Left offsett of first set bit, -1 if there isn't one.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
FirstBitSet(size, bytePtr)
    int			size;		/* Number of bytes in the array. */
    unsigned char	*bytePtr; 	/* The array of bytes. */
{
    int		offset;
    int	i;

    for (i = 0; i < size; i++) {
	offset = FirstBitInFour(((bytePtr[i]) >> 4) & 0xf);
	if (offset != -1) {
	    return (offset + (i * 8));
	}
	offset = FirstBitInFour((bytePtr[i]) & 0xf);
	if (offset != -1) {
	    return (offset + 4 + (i * 8));
	}
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * FirstUnsetBitInFour --
 *
 *	Returns the offset, from the left, of the first bit that is
 *	unset in the 4 least significant bits of an integer.
 *
 * Results:
 *	Left offset of first unset bit, -1 if there isn't one.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
FirstUnsetBitInFour(value)
    unsigned int value;		/* The value in which to find the bit. */
{
    static int	first[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, -1};

    return first[(value & 0xf)];
}

/*
 *----------------------------------------------------------------------
 *
 * FirstBitUnset --
 *
 *	Returns the left offset of the first bit that is unset in the 
 *	given array of bytes.
 *
 * Results:
 *	Left offsett of first unset bit, -1 if there isn't one.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
FirstBitUnset(size, bytePtr)
    int			size;		/* Number of bytes in the array. */
    unsigned char	*bytePtr; 	/* The array of bytes. */
{
    int		offset;
    int	i;

    for (i = 0; i < size; i++) {
	offset = FirstUnsetBitInFour(((bytePtr[i]) >> 4) & 0xf);
	if (offset != -1) {
	    return (offset + (i * 8));
	}
	offset = FirstUnsetBitInFour((bytePtr[i]) & 0xf);
	if (offset != -1) {
	    return (offset + 4 + (i * 8));
	}
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * NumBitsSetInFour --
 *
 *	Returns the number of bits set in the 4 least significant bits
 *	of the value.
 *
 * Results:
 *	The number of bits that are set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NumBitsSetInFour(value)
    unsigned int	value; 	/* The value in which to count set bits. */
{
    static int set[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

    return (set[(value) & 0xf]);
}


/*
 *----------------------------------------------------------------------
 *
 * NumBitsSet --
 *
 *	Returns the number of bits set in the given array of bytes.
 *
 * Results:
 *	Number of bits set in the given string of bytes.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NumBitsSet(size, bytePtr)
    int			size;		/* Number of bytes in the array. */
    unsigned char	*bytePtr; 	/* The array of bytes. */
{
    int		i;
    int		count = 0;

    for (i = 0; i < size; i++) {
	count += NumBitsSetInFour(bytePtr[i]);
	count += NumBitsSetInFour(bytePtr[i] >> 4);
    }
    return count;
}

/*
 *----------------------------------------------------------------------
 *
 * NumBitsSetContig --
 *
 *	Returns the number of contiguous set bits starting at the given
 *	left offset in the given array of bytes.
 *
 * Results:
 *	The number of contiguous set bits.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NumBitsSetContig(offset, size, bytePtr)
    int			offset;		/* Left offset of start. */
    int			size;		/* Number of bytes in the array. */
    unsigned char	*bytePtr;	/* The array of bytes. */
{
    int 	index;
    int		i;
    int		j;
    int		curOffset;
    int		count = 0; 

    index = offset / 8;
    curOffset = offset & 0x7;

    for( i = index; i < size; i++) {
	for(j = 0; j < 8 - curOffset; j++) {
	    if ((bytePtr[i] << (curOffset + j) & 0x80) == 0) {
		return count;
	    }
	    count++;
	}
	curOffset = 0;
    }
    return count;
}


/*
 *----------------------------------------------------------------------
 *
 * NumBitsUnsetContig --
 *
 *	Returns the number of contiguous unset bits starting at the given
 *	left offset in the given array of bytes.
 *
 * Results:
 *	The number of contiguous unset bits.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
NumBitsUnsetContig(offset, size, bytePtr)
    int			offset;		/* Left offset of start. */
    int			size;		/* Number of bytes in the array. */
    unsigned char	*bytePtr;	/* The array of bytes. */
{
    int 	index;
    int		i;
    int		j;
    int		curOffset;
    int		count = 0; 

    index = offset / 8;
    curOffset = offset & 0x7;

    for( i = index; i < size; i++) {
	for(j = 0; j < 8 - curOffset; j++) {
	    if ((bytePtr[i] << (curOffset + j) & 0x80) != 0) {
		return count;
	    }
	    count++;
	}
	curOffset = 0;
    }
    return count;
}



/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Tries  Set,  Test, and  Get on all possible bit fields
 *	of size 1 through 16 in a piece of memory of a given size.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int		argc;
    char	**argv;
{

    unsigned char	bits[8];
    unsigned short	*shortPtr = (unsigned short *) bits;
    unsigned int	*wordPtr = (unsigned int *) bits;

    int 	offset;
    int		size;
    int		bitsSet;
    int		loffset;
    int		contig;
    int		test;
    int		value;
    int 	i;

    /*
     * Test setting of bits, all possible offsets and sizes, in an array
     * of bytes. 
     */
    bzero(bits, sizeof(bits));
    for (size = 1; size < 16; size ++) {
	for (offset = 0; (offset + size) <= sizeof(bits) * 8; offset++) {
	    ByteSet(bits, offset, size, (1 << size) - 1);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != size) {
		printf("B<%d, %d>, Incorrect number of bits set: %d != %d\n", 
		    offset, size, bitsSet, size);
	    }
	    loffset = FirstBitSet(sizeof(bits), bits);
	    if (loffset != offset) {
		printf("B<%d, %d>, Incorrect offset: %d != %d\n", 
		    offset, size, loffset, offset);
	    }
	    contig = NumBitsSetContig(loffset, sizeof(bits), bits);
	    if (contig != size) {
		printf("B<%d, %d>, Incorrect contig bits set: %d != %d\n", 
		    offset, size, contig, size);
	    }
	    test = ByteTest(bits, offset, size, (1 << size) - 1);
	    if (test != 1) {
		printf("B<%d, %d>,  Test failed\n", offset, size);
	    }
	    value = ByteGet(bits, offset, size);
	    if (value != (1 << size) - 1) {
		printf("B<%d, %d>,  Get failed: %d != %d\n", 
		    offset, size, value, (1 << size) - 1);
	    }
	    ByteSet(bits, offset, size, 0);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != 0) {
		printf("B<%d, %d>, %d bits still set\n", offset, size, bitsSet);
	    }
	}
    }
    /*
     * Test unsetting of bits, all possible offsets and sizes, in an array
     * of bytes. 
     */
    for (i = 0; i < sizeof(bits); i++) {
	bits[i] = 0xff;
    }
    for (size = 1; size < 16; size ++) {
	for (offset = 0; (offset + size) <= sizeof(bits) * 8; offset++) {
	     ByteSet(bits, offset, size, 0);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != (sizeof(bits) * 8) - size) {
		printf("B<%d, %d>, Incorrect number of bits unset: %d != %d\n", 
		    offset, size, bitsSet, size);
	    }
	    loffset = FirstBitUnset(sizeof(bits), bits);
	    if (loffset != offset) {
		printf("B<%d, %d>, Incorrect offset: %d != %d\n", 
		    offset, size, loffset, offset);
	    }
	    contig = NumBitsUnsetContig(loffset, sizeof(bits), bits);
	    if (contig != size) {
		printf("B<%d, %d>, Incorrect contig bits unset: %d != %d\n", 
		    offset, size, contig, size);
	    }
	    test =  ByteTest(bits, offset, size, 0);
	    if (test != 1) {
		printf("B<%d, %d>,  Test failed\n", offset, size);
	    }
	    value =  ByteGet(bits, offset, size);
	    if (value != 0) {
		printf("B<%d, %d>,  Get failed: %d != %d\n", 
		    offset, size, value, 0);
	    }
	     ByteSet(bits, offset, size, (1 << size) - 1);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != (sizeof(bits) * 8)) {
		printf("B<%d, %d>, %d bits still unset\n", offset, size, 
		    (sizeof(bits) * 8) - bitsSet);
	    }
	}
    }
    /*
     * Test setting of bits, all possible offsets and sizes, in an array
     * of shorts. 
     */
    bzero(bits, sizeof(bits));
    for (size = 1; size < 16; size ++) {
	for (offset = 0; (offset + size) <= sizeof(bits) * 8; offset++) {
	     HalfwordSet(shortPtr, offset, size, (1 << size) - 1);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != size) {
		printf("H<%d, %d>, Incorrect number of bits set: %d != %d\n", 
		    offset, size, bitsSet, size);
	    }
	    loffset = FirstBitSet(sizeof(bits), bits);
	    if (loffset != offset) {
		printf("H<%d, %d>, Incorrect offset: %d != %d\n", 
		    offset, size, loffset, offset);
	    }
	    contig = NumBitsSetContig(loffset, sizeof(bits), bits);
	    if (contig != size) {
		printf("H<%d, %d>, Incorrect contig bits set: %d != %d\n", 
		    offset, size, contig, size);
	    }
	    test =  HalfwordTest(shortPtr, offset, size, (1 << size) - 1);
	    if (test != 1) {
		printf("H<%d, %d>,  Test failed\n", offset, size);
	    }
	    value =  HalfwordGet(shortPtr, offset, size);
	    if (value != (1 << size) - 1) {
		printf("H<%d, %d>,  Get failed: %d != %d\n", 
		    offset, size, value, (1 << size) - 1);
	    }
	     HalfwordSet(shortPtr, offset, size, 0);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != 0) {
		printf("H<%d, %d>, %d bits still set\n", offset, size, bitsSet);
	    }
	}
    }
    /*
     * Test clearing of bits, all possible offsets and sizes, in an array
     * of shorts. 
     */
    for (i = 0; i < sizeof(bits); i++) {
	bits[i] = 0xff;
    }
    for (size = 1; size < 16; size ++) {
	for (offset = 0; (offset + size) <= sizeof(bits) * 8; offset++) {
	     HalfwordSet(shortPtr, offset, size, 0);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != (sizeof(bits) * 8) - size) {
		printf("H<%d, %d>, Incorrect number of bits unset: %d != %d\n", 
		    offset, size, bitsSet, size);
	    }
	    loffset = FirstBitUnset(sizeof(bits), bits);
	    if (loffset != offset) {
		printf("H<%d, %d>, Incorrect offset: %d != %d\n", 
		    offset, size, loffset, offset);
	    }
	    contig = NumBitsUnsetContig(loffset, sizeof(bits), bits);
	    if (contig != size) {
		printf("H<%d, %d>, Incorrect contig bits unset: %d != %d\n", 
		    offset, size, contig, size);
	    }
	    test =  HalfwordTest(shortPtr, offset, size, 0);
	    if (test != 1) {
		printf("H<%d, %d>,  Test failed\n", offset, size);
	    }
	    value =  HalfwordGet(shortPtr, offset, size);
	    if (value != 0) {
		printf("H<%d, %d>,  Get failed: %d != %d\n", 
		    offset, size, value, 0);
	    }
	     HalfwordSet(shortPtr, offset, size, (1 << size) - 1);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != (sizeof(bits) * 8)) {
		printf("H<%d, %d>, %d bits still unset\n", offset, size, 
		    (sizeof(bits) * 8) - bitsSet);
	    }
	}
    }
    /*
     * Test setting of bits, all possible offsets and sizes, in an array
     * of words. 
     */
    bzero(bits, sizeof(bits));
    for (size = 1; size < 16; size ++) {
	for (offset = 0; (offset + size) <= sizeof(bits) * 8; offset++) {
	     WordSet(wordPtr, offset, size, (1 << size) - 1);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != size) {
		printf("W<%d, %d>, Incorrect number of bits set: %d != %d\n", 
		    offset, size, bitsSet, size);
	    }
	    loffset = FirstBitSet(sizeof(bits), bits);
	    if (loffset != offset) {
		printf("W<%d, %d>, Incorrect offset: %d != %d\n", 
		    offset, size, loffset, offset);
	    }
	    contig = NumBitsSetContig(loffset, sizeof(bits), bits);
	    if (contig != size) {
		printf("W<%d, %d>, Incorrect contig bits set: %d != %d\n", 
		    offset, size, contig, size);
	    }
	    test =  WordTest(wordPtr, offset, size, (1 << size) - 1);
	    if (test != 1) {
		printf("W<%d, %d>,  Test failed\n", offset, size);
	    }
	    value =  WordGet(wordPtr, offset, size);
	    if (value != (1 << size) - 1) {
		printf("W<%d, %d>,  Get failed: %d != %d\n", 
		    offset, size, value, (1 << size) - 1);
	    }
	     WordSet(wordPtr, offset, size, 0);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != 0) {
		printf("W<%d, %d>, %d bits still set\n", offset, size, bitsSet);
	    }
	}
    }
    /*
     * Test clearing of bits, all possible offsets and sizes, in an array
     * of words. 
     */
    for (i = 0; i < sizeof(bits); i++) {
	bits[i] = 0xff;
    }
    for (size = 1; size < 16; size ++) {
	for (offset = 0; (offset + size) <= sizeof(bits) * 8; offset++) {
	     WordSet(wordPtr, offset, size, 0);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != (sizeof(bits) * 8) - size) {
		printf("W<%d, %d>, Incorrect number of bits unset: %d != %d\n", 
		    offset, size, bitsSet, size);
	    }
	    loffset = FirstBitUnset(sizeof(bits), bits);
	    if (loffset != offset) {
		printf("W<%d, %d>, Incorrect offset: %d != %d\n", 
		    offset, size, loffset, offset);
	    }
	    contig = NumBitsUnsetContig(loffset, sizeof(bits), bits);
	    if (contig != size) {
		printf("W<%d, %d>, Incorrect contig bits unset: %d != %d\n", 
		    offset, size, contig, size);
	    }
	    test =  WordTest(wordPtr, offset, size, 0);
	    if (test != 1) {
		printf("W<%d, %d>,  Test failed\n", offset, size);
	    }
	    value =  WordGet(wordPtr, offset, size);
	    if (value != 0) {
		printf("W<%d, %d>,  Get failed: %d != %d\n", 
		    offset, size, value, 0);
	    }
	     WordSet(wordPtr, offset, size, (1 << size) - 1);
	    bitsSet = NumBitsSet(sizeof(bits), bits);
	    if (bitsSet != (sizeof(bits) * 8)) {
		printf("W<%d, %d>, %d bits still unset\n", offset, size, 
		    (sizeof(bits) * 8) - bitsSet);
	    }
	}
    }
}

