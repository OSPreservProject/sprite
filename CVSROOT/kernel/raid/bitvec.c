/*
 *
 */

#include "bitvec.h"

BitVec
MakeBitVec(n)
    int n;
{
    BitVec vec;
    int size = (n/32 + 1) * sizeof(int);

    vec = (BitVec) malloc((unsigned) size);
    bzero((char *) vec, size);
    return vec;
}

GetBitIndex(bitVec, i, n)
    BitVec bitVec;
    int    i;
    int    n;
{
    int	   wordIndex;
    register int   bitIndex;
    register int   word;

    for (wordIndex = WordIndex(i+1); wordIndex <= WordIndex(n-1); wordIndex++){
	if (bitVec[wordIndex] != 0) {
	    word = (unsigned) bitVec[wordIndex];
	    bitIndex = 0;
	    if (!(word & 0x0000ffff)) {
		bitIndex += 16;
		word = word >> 16;
	    }
	    if (!(word & 0x00ff)) {
		bitIndex += 8;
		word = word >> 8;
	    }
	    if (!(word & 0x0f)) {
		bitIndex += 4;
		word = word >> 4;
	    }
	    if (!(word & 0x03)) {
		bitIndex += 2;
		word = word >> 2;
	    }
	    if (!(word & 0x01)) {
		bitIndex += 1;
	    }
	    ClrBit(bitVec, VecIndex(wordIndex, bitIndex));
	    return VecIndex(wordIndex, bitIndex);
	}
    }
    return -1;
}
