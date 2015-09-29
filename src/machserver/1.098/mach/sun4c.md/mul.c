#ifdef sccsid
static char     sccsid[] = "@(#)mul.c 1.5 88/02/08 SMI";
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

void
_fp_product(x, y, z)
	unsigned        x, y;
	unsigned        z[2];

/* Computes product x * y into *z[2]	 */

{
	union {
		unsigned        ul;
		unsigned short  us[2];
	}
	                kluge;

	unsigned        thi, tlo, tmid, t;
	unsigned short  xus0, xus1, yus0, yus1;

	kluge.ul = x;
	xus0 = kluge.us[0];
	xus1 = kluge.us[1];
	kluge.ul = y;
	yus0 = kluge.us[0];
	yus1 = kluge.us[1];

	thi = xus0 * yus0;
	tlo = xus1 * yus1;

	tmid = xus1 * yus0;
	thi += tmid >> 16;
	tmid = tmid << 16;
	t = tmid + tlo;
	if ((t < tmid) && (t < tlo))
		thi++;
	tlo = t;

	tmid = xus0 * yus1;
	thi += tmid >> 16;
	tmid = tmid << 16;
	t = tmid + tlo;
	if ((t < tmid) && (t < tlo))
		thi++;
	tlo = t;

	z[0] = thi;
	z[1] = tlo;

}

void
_fp_mul(px, py, pz)
	unpacked       *px, *py, *pz;

{
	unpacked       *pt;
	unsigned        t, tacc[2], acc[4];	/* Product accumulator. */

	if ((int) px->fpclass < (int) py->fpclass) {
		pt = px;
		px = py;
		py = pt;
	}
	/* Now class(x) >= class(y).  */

	*pz = *px;
	pz->sign = px->sign ^ py->sign;

	switch (px->fpclass) {
	case fp_quiet:
	case fp_signaling:
	case fp_zero:
		return;
	case fp_infinity:
		if (py->fpclass == fp_zero) {
			fpu_error_nan(pz);
			pz->fpclass = fp_quiet;
		}
		return;
	case fp_normal:
		if (py->fpclass == fp_zero) {
			pz->fpclass = fp_zero;
			return;
		}
	}

	/* Now x and y are both normal or subnormal. */

	pz->exponent = px->exponent + py->exponent + 1;

	_fp_product(px->significand[1], py->significand[1], tacc);
	acc[2] = tacc[0];
	acc[3] = tacc[1];

	_fp_product(px->significand[0], py->significand[0], tacc);
	acc[0] = tacc[0];
	acc[1] = tacc[1];

	_fp_product(px->significand[1], py->significand[0], tacc);
	t = acc[2] + tacc[1];
	if ((t < acc[2]) && (t < tacc[1])) {	/* Propagate carry. */
		acc[1]++;
		if (acc[1] == 0)
			acc[0]++;
	}
	acc[2] = t;
	t = acc[1] + tacc[0];
	if ((t < acc[1]) && (t < tacc[0])) {	/* Propagate carry. */
		acc[0]++;
	}
	acc[1] = t;

	_fp_product(px->significand[0], py->significand[1], tacc);
	t = acc[2] + tacc[1];
	if ((t < acc[2]) && (t < tacc[1])) {	/* Propagate carry. */
		acc[1]++;
		if (acc[1] == 0)
			acc[0]++;
	}
	acc[2] = t;
	t = acc[1] + tacc[0];
	if ((t < acc[1]) && (t < tacc[0])) {	/* Propagate carry. */
		acc[0]++;
	}
	acc[1] = t;

	pz->significand[0] = acc[0];
	pz->significand[1] = acc[1];
	pz->significand[2] = acc[2];
	if (acc[3])
		pz->significand[2] |= 1;

	fpu_normalize(pz);

}
#endif /* lint */
