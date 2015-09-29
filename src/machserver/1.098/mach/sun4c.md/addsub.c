#ifdef sccsid
static char     sccsid[] = "@(#)addsub.c 1.5 88/02/08 SMI";
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

PRIVATE void
true_add(px, py, pz)
	unpacked       *px, *py, *pz;

{
	unpacked       *pt;

	if ((int) px->fpclass < (int) py->fpclass) {	/* Reverse. */
		pt = py;
		py = px;
		px = pt;
	}
	/* Now class(x) >= class(y). */
	switch (px->fpclass) {
	case fp_quiet:		/* NaN + x -> NaN */
	case fp_signaling:	/* NaN + x -> NaN */
	case fp_infinity:	/* Inf + x -> Inf */
	case fp_zero:		/* 0 + 0 -> 0 */
		*pz = *px;
		return;
	default:
		if (py->fpclass == fp_zero) {
			*pz = *px;
			return;
		}
	}
	/* Now z is normal or subnormal. */
	/* Now y is normal or subnormal. */
	if (px->exponent < py->exponent) {	/* Reverse. */
		pt = py;
		py = px;
		px = pt;
	}
	/* Now class(x) >= class(y). */
	pz->fpclass = px->fpclass;
	pz->sign = px->sign;
	pz->exponent = px->exponent;

	if (px->exponent == py->exponent) {	/* no pre-alignment required */
		pz->significand[2] = 0;
		pz->significand[1] = px->significand[1] + py->significand[1];
		if ((pz->significand[1] < px->significand[1]) &&
		    (pz->significand[1] < py->significand[1]))
			py->significand[0]++;
		pz->significand[0] = px->significand[0] + py->significand[0];
	} else {		/* pre-alignment required */
		fpu_rightshift(py, pz->exponent - py->exponent);
		pz->significand[2] = py->significand[2];
		pz->significand[1] = px->significand[1] + py->significand[1];
		if ((pz->significand[1] < px->significand[1]) &&
		    (pz->significand[1] < py->significand[1]))
			py->significand[0]++;	/* Carry out occurred. */
		pz->significand[0] = px->significand[0] + py->significand[0];
		if ((pz->significand[0] >= px->significand[0]) ||
		    (pz->significand[0] >= py->significand[0])) {	/* No carry out
									 * occurred. */
			return;
		}
	}
	/* Handle carry out of msb. */
	fpu_rightshift(pz, 1);
	pz->significand[0] |= 0x80000000;	/* Carried out bit. */
	pz->exponent++;		/* Renormalize. */
	return;
}

PRIVATE void
true_sub(px, py, pz)
	unpacked       *px, *py, *pz;

{
	unpacked       *pt;

	if ((int) px->fpclass < (int) py->fpclass) {	/* Reverse. */
		pt = py;
		py = px;
		px = pt;
	}
	/* Now class(x) >= class(y). */
	*pz = *px;		/* Tentative difference: x. */
	switch (pz->fpclass) {
	case fp_quiet:		/* NaN - x -> NaN */
	case fp_signaling:	/* NaN - x -> NaN */
		return;
	case fp_infinity:	/* Inf - x -> Inf */
		if (py->fpclass == fp_infinity) {
			fpu_error_nan(pz);	/* Inf - Inf -> NaN */
			pz->fpclass = fp_quiet;
		}
		return;
	case fp_zero:		/* 0 + 0 -> 0 */
		pz->sign = (fp_direction == fp_negative);
		return;
	default:
		if (py->fpclass == fp_zero)
			return;
	}

	/* x and y are both normal or subnormal. */

	if (px->exponent < py->exponent) {	/* Reverse. */
		pt = py;
		py = px;
		px = pt;
	}
	/* Now exp(x) >= exp(y). */
	pz->fpclass = px->fpclass;
	pz->sign = px->sign;
	pz->exponent = px->exponent;

	if (px->exponent == py->exponent) {	/* no pre-alignment required */

		pz->significand[2] = 0;
		if (px->significand[1] >= py->significand[1]) {
			pz->significand[1] = px->significand[1] - py->significand[1];
		} else {
			pz->significand[1] = (0xffffffff - py->significand[1]) + (1 + px->significand[1]);
			px->significand[0]--;
		}
		if (px->significand[0] >= py->significand[0]) {
			pz->significand[0] = px->significand[0] - py->significand[0];
			if ((pz->significand[0] | pz->significand[1]) == 0) {	/* exact zero result */
				pz->sign = (fp_direction == fp_negative);
				pz->fpclass = fp_zero;
				return;
			}
		} else {	/* sign reversal occurred */
			pz->sign = py->sign;
			if (pz->significand[1] != 0) {	/* complement lower
							 * significand. */
				pz->significand[1] = 0x00000000 - pz->significand[1];
				pz->significand[0] = py->significand[0] - px->significand[0] - 1;
			} else
				pz->significand[0] = py->significand[0] - px->significand[0];
		}
	} else {		/* pre-alignment required */
		fpu_rightshift(py, pz->exponent - py->exponent);
		if (py->significand[2] == 0)
			pz->significand[2] = 0;
		else {		/* Propagate carry from non-zero bits shifted
				 * out. */
			pz->significand[2] = 1 + (0xffffffff - py->significand[2]);
			px->significand[1]--;
			if (px->significand[1] == 0xffffffff)
				px->significand[0]--;
		}
		if (px->significand[1] >= py->significand[1]) {
			pz->significand[1] = px->significand[1] - py->significand[1];
		} else {
			pz->significand[1] = (0xffffffff - py->significand[1]) + (1 + px->significand[1]);
			px->significand[0]--;
		}
		pz->significand[0] = px->significand[0] - py->significand[0];
	}
	fpu_normalize(pz);
	return;
}

void
_fp_add(px, py, pz)
	unpacked       *px, *py, *pz;

{
	if (px->sign == py->sign)
		true_add(px, py, pz);
	else
		true_sub(px, py, pz);
}

void
_fp_sub(px, py, pz)
	unpacked       *px, *py, *pz;

{
	py->sign = 1 - py->sign;
	if (px->sign == py->sign)
		true_add(px, py, pz);
	else
		true_sub(px, py, pz);
}
#endif /* lint */
