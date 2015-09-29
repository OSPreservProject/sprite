#ifdef sccsid
static char     sccsid[] = "@(#)compare.c 1.4 88/02/08 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc. 
 */

/*
 * This whole file refuses to lint.
 */
#ifndef lint

#include <sun4/fpu/fpu_simulator.h>
#include <sun4/fpu/globals.h>

enum fcc_type
_fp_compare(px, py, strict)
	unpacked       *px, *py;
	int             strict;	/* 0 if quiet NaN unexceptional, 1 if
				 * exceptional */

{
	enum fcc_type   cc;

	if ((px->fpclass == fp_quiet) || (py->fpclass == fp_quiet) ||
	    (px->fpclass == fp_signaling) || (py->fpclass == fp_signaling)) {	/* NaN */
		if (strict)
			fpu_set_exception(fp_invalid);
		cc = fcc_unordered;
	} else if ((px->fpclass == fp_zero) && (py->fpclass == fp_zero))
		cc = fcc_equal;
	/* both zeros */
	else if (px->sign < py->sign)
		cc = fcc_greater;
	else if (px->sign > py->sign)
		cc = fcc_less;
	else {			/* signs the same, compute magnitude cc */
		if ((int) px->fpclass > (int) py->fpclass)
			cc = fcc_greater;
		else if ((int) px->fpclass < (int) py->fpclass)
			cc = fcc_less;
		else
		 /* same classes */ if (px->fpclass == fp_infinity)
			cc = fcc_equal;	/* same infinity */
		else if (px->exponent > py->exponent)
			cc = fcc_greater;
		else if (px->exponent < py->exponent)
			cc = fcc_less;
		else
		 /* equal exponents */ if (px->significand[0] > py->significand[0])
			cc = fcc_greater;
		else if (px->significand[0] < py->significand[0])
			cc = fcc_less;
		else
		 /* equal upper significands */ if (px->significand[1] > py->significand[1])
			cc = fcc_greater;
		else if (px->significand[1] < py->significand[1])
			cc = fcc_less;
		else
			cc = fcc_equal;
		if (px->sign)
			switch (cc) {	/* negative numbers */
			case fcc_less:
				cc = fcc_greater;
				break;
			case fcc_greater:
				cc = fcc_less;
				break;
			}
	}
	return (cc);
}
#endif /* lint */
