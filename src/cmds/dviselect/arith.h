/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

/*
 * Arithmetic with scaled dimensions
 *
 * From `TEX.WEB' by D. E. Knuth:
 *
 * `The principal computations performed by TeX are done entirely in terms
 * of integers less than 2^31 in magnitude; and divisions are done only when
 * both divident and divisor are nonnegative.  Thus, the arithemtic specified
 * in this program can be carried out in exactly the same way on a wide
 * variety of computers, including some small ones.  Why?  Because the
 * arithmetic calculations need to be spelled out precisely in order to
 * guarantee that TeX will produce identical output on different machines.
 * If some quantities were rounded differently in different implementations,
 * we would find that line breaks and even page breaks might occur in
 * different places.  Hence the arithmetic of TeX has been designed with
 * care, and systems that claim be be implementations of TeX82 should follow
 * precisely the calculations as they appear in the present program.'
 *
 * Thus, this follows the given implementation with few (no) optimizations.
 */

/*
 * We do fixed-point arithmetic on `scaled integers'.  These should
 * be (at least) 32 bits.  Note also that it is assumed that certain
 * `int' values may be stored in these (usually small, or else one
 * of the magic 1<<n values).
 */
typedef i32 scaled;

#define UNITY	  (1<<16)	/* 2^{16}, or 1.00000 */
#define TWO	  (1<<17)	/* 2^{17}, or 2.00000 */
#define MAXSCALED ((1<<30)-1)	/* maximum scaled value, 2^{30}-1 */

/*
 * This works on two's complement machines.  If you are not on one of those,
 * you will need something different here.
 */
#define odd(n) ((n) & 1)

/* half of an integer, unambiguous with respect to signed odd numbers */
#define half(x) (odd (x) ? ((x) + 1) / 2 : (x) / 2)

int ArithError;			/* set true whenever an arithmetic overflow
				   occurs */

char *UnScale ();		/* return a pointer to the external
				   representation of a `scaled'.
				   (Alas, since I am returning it in
				   static storage, you can only use
				   it once without copying.) */
