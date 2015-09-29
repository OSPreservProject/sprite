#ifndef lint
static char     sccsid[] = "@(#)div.c 1.6 88/02/08 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * This whole file refuses to lint.
 */
#ifndef lint
#include <sun4/fpu/fpu_simulator.h>
#include <sun4/fpu/globals.h>

PRIVATE unsigned
trial_quotient(remainder, divisor)
	unsigned        remainder[], divisor[];

/*
 * Given remainder[0,1,2,3,4] < 2**16, divisor[0,1,2,3] < 2**16, 2**15 <=
 * divisor[0] < 2**16, computes a trial quotient q < 2**17, such that q =
 * remainder/divisor, and revises remainder.
 */

{
	unsigned        q;
	int             i, k;
	unsigned        t, t1, t2;
	q = ((remainder[0] << 16) + remainder[1]) / divisor[0];

	for (i = 3; i >= 0; i--) {
		t = (q & 0xffff) * divisor[i];
		t1 = t >> 16;
		t2 = t & 0xffff;

		if (remainder[i + 1] < t2) {	/* ripple carry */
			k = i;
			while ((k >= 0) && (remainder[k] == 0))
				remainder[k--] = 0xffff;
			remainder[k]--;
		}
		remainder[i + 1] -= t2;
		remainder[i + 1] &= 0xffff;

		if ((i >= 1) && (remainder[i] < t1)) {	/* ripple carry */
			k = i - 1;
			while ((k >= 0) && (remainder[k] == 0))
				remainder[k--] = 0xffff;
			remainder[k]--;
		}
		remainder[i] -= t1;
		remainder[i] &= 0xffff;

		t = (q >> 16) * divisor[i];
		if (t > 0) {
			if ((i >= 1) && (remainder[i] < t)) {	/* ripple carry */
				k = i - 1;
				while ((k >= 0) && (remainder[k] == 0))
					remainder[k--] = 0xffff;
				remainder[k]--;
			}
			remainder[i] -= t;
			remainder[i] &= 0xffff;
		}
	}

	while (remainder[0] > 0x8000) {	/* Sign reversal occurred - reduce
					 * quotient by 1 - add divisor back
					 * to remainder. */
		q--;
		for (i = 3; i >= 0; i--) {
			if (remainder[i + 1] > (0xffff - divisor[i])) {	/* ripple carry */
				k = i;
				while ((k >= 0) && (remainder[k] == 0xffff))
					remainder[k--] = 0;
				if (k >= 0)
					remainder[k]++;
			}
			remainder[i + 1] += divisor[i];
			remainder[i + 1] &= 0xffff;
		}
	}

	/* Make sure that remainder <= divisor. */
	i = 0;
	while (remainder[i + 1] == divisor[i])
		i++;
	if ((remainder[0] != 0) || (i > 3) || (remainder[i + 1] > divisor[i])) {	/* Remainder too big -
											 * increase quotient by
											 * 1 - subtract divisor
											 * back from remainder. */
		q++;
		for (i = 3; i >= 0; i--) {
			if (remainder[i + 1] < divisor[i]) {	/* ripple carry */
				k = i;
				while ((k >= 0) && (remainder[k] == 0))
					remainder[k--] = 0xffff;
				if (k >= 0)
					remainder[k]--;
			}
			remainder[i + 1] -= divisor[i];
			remainder[i + 1] &= 0xffff;
		}
		i = 0;
		while (remainder[i] == divisor[i])
			i++;
	}
	for (i = 0; i <= 3; i++)
		remainder[i] = remainder[i + 1];
	remainder[4] = 0;

	return q;
}

void
_fp_div(px, py, pz)
	unpacked       *px, *py, *pz;

{
	unsigned        quotient[5];	/* Quotient accumulator. */
	unsigned        remainder[5];	/* Remainder accumulator. */
	unsigned        divisor[5];

	*pz = *px;
	pz->sign = px->sign ^ py->sign;

	if ((py->fpclass == fp_quiet) || (py->fpclass == fp_signaling)) {
		*pz = *py;
		return;
	}
	switch (px->fpclass) {
	case fp_quiet:
	case fp_signaling:
		return;
	case fp_zero:
	case fp_infinity:
		if (px->fpclass == py->fpclass) {	/* 0/0 or inf/inf */
			fpu_error_nan(pz);
			pz->fpclass = fp_quiet;
		}
		return;
	case fp_normal:
		switch (py->fpclass) {
		case fp_zero:	/* number/0 */
			fpu_set_exception(fp_division);
			pz->fpclass = fp_infinity;
			return;
		case fp_infinity:	/* number/inf */
			pz->fpclass = fp_zero;
			return;
		}
	}

	/* Now x and y are both normal or subnormal. */

	pz->exponent = px->exponent - py->exponent;

	remainder[0] = px->significand[0] >> 16;
	remainder[1] = px->significand[0] & 0xffff;
	remainder[2] = px->significand[1] >> 16;
	remainder[3] = px->significand[1] & 0xffff;
	remainder[4] = 0;
	divisor[0] = py->significand[0] >> 16;
	divisor[1] = py->significand[0] & 0xffff;
	divisor[2] = py->significand[1] >> 16;
	divisor[3] = py->significand[1] & 0xffff;
	divisor[4] = 0;

	quotient[0] = trial_quotient(remainder, divisor);
	quotient[1] = trial_quotient(remainder, divisor);
	pz->significand[0] = (quotient[0] << 16) + quotient[1];
	quotient[2] = trial_quotient(remainder, divisor);
	quotient[3] = trial_quotient(remainder, divisor);
	pz->significand[1] = (quotient[2] << 16) + quotient[3];
	if (quotient[2] > 0xffff)
		pz->significand[0]++;

	if (remainder[0] >= 0x8000) {	/* remainder is large enough to force
					 * round upward. */
		pz->significand[2] = 0xc0000000;
	} else {		/* Remainder < 1.0 <= divisor so compare
				 * 2*remainder to divisor. */
		int             i;
		for (i = 0; i <= 3; i++) {
			remainder[i] <<= 1;
			if (remainder[i] >= 0x10000) {	/* Shift out. */
				remainder[i] &= 0xffff;
				remainder[i - 1] |= 1;
			}
		}
		for (i = 0; i <= 3; i++) {	/* compare divisor to
						 * 2*remainder */
			if (divisor[i] > remainder[i])
				goto gt;
			if (divisor[i] < remainder[i])
				goto lt;
		}
		/*
		 * Can't fall through to here - remainder can't be exactly
		 * half divisor.
		 */

gt:				/* divisor is greater than 2 * remainder */
		pz->significand[2] = remainder[0] | remainder[1] | remainder[2] | remainder[3];
		if (pz->significand[2] & 1)
			pz->significand[2] |= 1;	/* Save lsb. */
		pz->significand[2] >>= 1;	/* Right shift for leading 0
						 * round bit; rest of word
						 * will be zero if remainder
						 * is exactly zero. */
		goto round;

lt:				/* divisor is less than 2 * remainder. */
		pz->significand[2] = 0xc0000000;	/* Therefore round
							 * upward. */
		goto round;
	}

round:

	if (quotient[0] > 0xffff) {	/* 1 <= quotient < 2 */
		fpu_rightshift(pz, 1);
		pz->significand[0] |= 0x80000000;
	} else
		/* 0.5 <= quotient < 1 */
		pz->exponent--;

}

#define ADDBIT(X,I) /* Adds  2**-i to x, an array of 16-bit chunks. */ \
	(X)[((I)-1) / 16] += 0x8000 >> ((I-1) % 16)

#define SUBBIT(X,I) /* Adds -2**-i to x, an array of 16-bit chunks. */ \
	(X)[((I)-1) / 16] -= 0x8000 >> ((I-1) % 16)


PRIVATE
dosqrt(px, pz)
	unpacked       *px, *pz;
{
	int             i, j, negative, borrow;
	unsigned        quotient[5];	/* Quotient accumulator. */
	unsigned        remainder[5];	/* Remainder accumulator. */

	for (i = 0; i <= 4; i++) {
		quotient[i] = 0;
	}
	remainder[0] = (px->significand[0] - 0x40000000) >> 16;
	remainder[1] = px->significand[0] & 0xffff;
	remainder[2] = px->significand[1] >> 16;
	remainder[3] = px->significand[1] & 0xffff;
	remainder[4] = px->significand[2] >> 16;

	negative = 0;
	for (i = 1; i <= 65; i++) {	/* Generate one more bit of quotient. */
		for (j = 0; j <= 4; j++) {	/* Double remainder. */
			remainder[j] <<= 1;
		}
		if (negative) {	/* Remainder was negative - add quotient. */
			for (j = 0; j <= 4; j++) {
				remainder[j] -= quotient[j];
			}
			SUBBIT(remainder, i + 1);
			SUBBIT(remainder, i + 2);
			for (j = 4; j >= 1; j--) {	/* Force
							 * borrows/carries. */
				while (remainder[j] > 0x20000) {
					remainder[j] += 0x10000;
					remainder[j - 1] -= 1;
				}
				while (remainder[j] > 0xffff) {
					remainder[j] -= 0x10000;
					remainder[j - 1] += 1;
				}
			}
			borrow = 0;
			while (remainder[0] > 0xffff) {
				remainder[0] += 0x10000;
				borrow++;
			}
			if ((remainder[0] | remainder[1] | remainder[2] | remainder[3] | remainder[4]) == 0) {	/* Remainder is exactly
														 * zero. */
				negative = 0;
			} else if (borrow) {	/* Remainder has changed
						 * sign. */
				negative = 0;
				j = 4;
				while (remainder[j] == 0)
					j--;
				remainder[j] = 0x10000 - remainder[j];
				for (j--; j >= 0; j--) {	/* Complement remainder. */
					remainder[j] = 0xffff - remainder[j];
				}
			}
		} else {	/* Remainder is positive - subtract quotient. */
			ADDBIT(quotient, i);	/* Adjust quotient. */
			for (j = 0; j <= 4; j++) {
				remainder[j] -= quotient[j];
			}
			SUBBIT(remainder, i + 2);
			for (j = 4; j >= 1; j--) {	/* Force
							 * borrows/carries. */
				while (remainder[j] > 0x20000) {
					remainder[j] += 0x10000;
					remainder[j - 1] -= 1;
				}
				while (remainder[j] > 0xffff) {
					remainder[j] -= 0x10000;
					remainder[j - 1] += 1;
				}
			}
			borrow = 0;
			while (remainder[0] > 0xffff) {
				remainder[0] += 0x10000;
				borrow++;
			}
			if (borrow) {	/* Remainder has changed sign. */
				negative = 1;
				j = 4;
				while (remainder[j] == 0)
					j--;
				remainder[j] = 0x10000 - remainder[j];
				for (j--; j >= 0; j--) {	/* Complement remainder. */
					remainder[j] = 0xffff - remainder[j];
				}
			}
		}
	}
	/* Quotient so far is exact iff remainder+quotient+2**-67 <= 0. */
	if (negative) {		/* Maybe exact. */
		for (j = 0; j <= 4; j++) {
			remainder[j] -= quotient[j];
		}
		SUBBIT(remainder, 67);
		for (j = 4; j >= 1; j--) {	/* Force borrows. */
			while (remainder[j] > 0xffff) {
				remainder[j] += 0x10000;
				remainder[j - 1] -= 1;
			}
		}
		if ((remainder[1] | remainder[2] | remainder[3] | remainder[4]) == 0) {	/* Remainder is exactly
											 * zero. */
			negative = 0;
		} else if (remainder[0] > 0xffff) {	/* Remainder has changed
							 * sign. */
			negative = 0;
			j = 4;
			while (remainder[j] == 0)
				j--;
			remainder[j] = -remainder[j];
			for (; j >= 0; j--) {	/* Complement remainder. */
				remainder[j] = 0xffffffff - remainder[j];
			}
		}
	}
	if (!negative &&
	    ((remainder[0] | remainder[1] | remainder[2] | remainder[3] | remainder[4]) != 0)) {	/* Remainder is neither
													 * negative or zero and
													 * therefore inexact. */
		quotient[4] |= 1;
	}
	pz->significand[0] = ((quotient[0]) << 16) | quotient[1];
	pz->significand[1] = ((quotient[2]) << 16) | quotient[3];
	pz->significand[2] = ((quotient[4]) << 16);
}

void
_fp_sqrt(px, pz)
	unpacked       *px, *pz;

{				/* *pz gets sqrt(*px) */


	*pz = *px;
	switch (px->fpclass) {
	case fp_quiet:
	case fp_signaling:
	case fp_zero:
		return;
	case fp_infinity:
		if (px->sign == 1) {	/* sqrt(-inf) */
			fpu_error_nan(pz);
			pz->fpclass = fp_quiet;
		}
		return;
	case fp_normal:
		if (px->sign == 1) {	/* sqrt(-norm) */
			fpu_error_nan(pz);
			pz->fpclass = fp_quiet;
			return;
		}
	}

	/* Now x is normal. */

	if (px->exponent & 1) {	/* sqrt(1.f * 2**odd) = sqrt (0.5f) *
				 * 2**(odd+1)/2 */
		pz->exponent = (px->exponent - 1) / 2;
	} else {		/* sqrt(1.f * 2**even) = sqrt (1.f) *
				 * 2**(even)/2 */
		fpu_rightshift(px, 1);	/* denormalize by 1 */
		pz->exponent = px->exponent / 2;
	}
	/*
	 * Exponent is now correct unless sqrt rounds up to 2.0 - which can
	 * happen in round to positive infinity mode.
	 */

	dosqrt(px, pz);

}
#endif /* lint */
