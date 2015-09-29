/*
 *
 */

#ifndef _BITVEC
#define _BITVEC

#define BIT_VEC_SIZE 4

typedef int *BitVec;

#define VecIndex(w, b)	(((w) << 5) | b)
#define WordIndex(i)	((i) >> 5)
#define BitIndex(i)	((i) & 0x1F)
#define VecSize(n)	(((n)/32 + 1) * sizeof(int))

#define SetBit(bitVec, i) ( (bitVec)[WordIndex(i)] |= 1 << BitIndex(i) )
#define ClrBit(bitVec, i) ( (bitVec)[WordIndex(i)] &= ~(1 << BitIndex(i)) )
#define IsSet(bitVec, i) ( (bitVec)[WordIndex(i)] >> BitIndex(i) & 1 )

#define FOR_ALL_VEC(bitVec, i, n)	\
    for ((i) = -1; ((i) = GetBitIndex((bitVec), (i), (n))) != -1;)

extern BitVec MakeBitVec _ARGS_((int n));
extern void ClearBitVec _ARGS_((BitVec bitVec, int n));
extern int GetBitIndex _ARGS_((BitVec bitVec, int i, int n));

#endif /* _BITVEC */
