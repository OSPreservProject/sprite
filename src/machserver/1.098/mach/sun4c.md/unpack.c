#ifdef sccsid
static char     sccsid[] = "@(#)unpack.c 1.5 88/02/08 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

/* Unpack procedures for Sparc FPU simulator. */

/*
 * This whole file refuses to lint.
 */
#ifndef lint
#include <sun4/fpu/fpu_simulator.h>
#include <sun4/fpu/globals.h>

PRIVATE void
leftshift(pu, n)
	unpacked       *pu;
	unsigned        n;

/* Left shift significand by n bits.  Affect all classes.	 */

{
	while (n >= 32) {	/* big shift */
		(*pu).significand[0] = (*pu).significand[1];
		(*pu).significand[1] = (*pu).significand[2];
		(*pu).significand[2] = 0;
		n -= 32;
	}
	while (n > 0) {		/* small shift */
		pu->significand[0] = pu->significand[0] << 1;
		if (pu->significand[1] & 0x80000000)
			pu->significand[0] |= 1;
		pu->significand[1] = pu->significand[1] << 1;
		if (pu->significand[2] & 0x80000000)
			pu->significand[1] |= 1;
		pu->significand[2] = pu->significand[2] << 1;
		n -= 1;
	}
}

PRIVATE void
unpackinteger(pu, x)
	unpacked       *pu;	/* unpacked result */
	int             x;	/* packed integer */
{
	if (x == 0) {
		pu->sign = 0;
		pu->fpclass = fp_zero;
	} else {
		(*pu).sign = x < 0;
		(*pu).fpclass = fp_normal;
		(*pu).exponent = INTEGER_BIAS;
		(*pu).significand[0] = (x >= 0) ? x : -x;
		(*pu).significand[1] = 0;
		(*pu).significand[2] = 0;
		fpu_normalize(pu);
	}
}

void
unpacksingle(pu, x)
	unpacked       *pu;	/* unpacked result */
	single_type     x;	/* packed single */
{
	(*pu).sign = x.sign;
	pu->significand[1] = 0;
	pu->significand[2] = 0;
	if (x.exponent == 0) {	/* zero or sub */
		if (x.significand == 0) {	/* zero */
			pu->fpclass = fp_zero;
			return;
		} else {	/* subnormal */
			pu->fpclass = fp_normal;
			pu->exponent = -SINGLE_BIAS;
			pu->significand[0] = x.significand << 9;
			fpu_normalize(pu);
			return;
		}
	} else if (x.exponent == 0xff) {	/* inf or nan */
		if (x.significand == 0) {	/* inf */
			pu->fpclass = fp_infinity;
			return;
		} else {	/* nan */
			if ((x.significand & 0x400000) != 0) {	/* quiet */
				pu->fpclass = fp_quiet;
			} else {/* signaling */
				pu->fpclass = fp_signaling;
				fpu_set_exception(fp_invalid);
			}
			pu->significand[0] = 0x40000000 | (x.significand << 8);
			return;
		}
	}
	(*pu).exponent = x.exponent - SINGLE_BIAS;
	(*pu).fpclass = fp_normal;
	(*pu).significand[0] = 0x80000000 | (x.significand << 8);
}

void
unpackdouble(pu, x, y)
	unpacked       *pu;	/* unpacked result */
	double_type     x;	/* packed double */
	unsigned        y;
{
	(*pu).sign = x.sign;
	pu->significand[1] = y;
	pu->significand[2] = 0;
	if (x.exponent == 0) {	/* zero or sub */
		if ((x.significand == 0) && (y == 0)) {	/* zero */
			pu->fpclass = fp_zero;
			return;
		} else {	/* subnormal */
			pu->fpclass = fp_normal;
			pu->exponent = 12 - DOUBLE_BIAS;
			pu->significand[0] = x.significand;
			fpu_normalize(pu);
			return;
		}
	} else if (x.exponent == 0x7ff) {	/* inf or nan */
		if ((x.significand == 0) && (y == 0)) {	/* inf */
			pu->fpclass = fp_infinity;
			return;
		} else {	/* nan */
			if ((x.significand & 0x80000) != 0) {	/* quiet */
				pu->fpclass = fp_quiet;
			} else {/* signaling */
				pu->fpclass = fp_signaling;
				fpu_set_exception(fp_invalid);
			}
			pu->significand[0] = 0x80000 | x.significand;
			leftshift(pu, 11);
			return;
		}
	}
	(*pu).exponent = x.exponent - DOUBLE_BIAS;
	(*pu).fpclass = fp_normal;
	(*pu).significand[0] = 0x100000 | x.significand;
	leftshift(pu, 11);
}

PRIVATE void
unpackextended(pu, x, y, z)
	unpacked       *pu;	/* unpacked result */
	extended_type   x;	/* packed extended */
	unsigned        y, z;
{
	(*pu).sign = x.sign;
	(*pu).fpclass = fp_normal;
	(*pu).exponent = x.exponent - EXTENDED_BIAS;
	(*pu).significand[0] = y;
	(*pu).significand[1] = z;
	(*pu).significand[2] = 0;
	if (x.exponent < 0x7fff) {	/* zero, normal, or subnormal */
		if ((z == 0) && (y == 0)) {	/* zero */
			pu->fpclass = fp_zero;
			return;
		} else {	/* normal or subnormal */
			fpu_normalize(pu);
			return;
		}
	} else {	/* inf or nan */
		if ((z == 0) && (y == 0)) {	/* inf */
			pu->fpclass = fp_infinity;
			return;
		} else {	/* nan */
			if ((y & 0x40000000) != 0) {	/* quiet */
				pu->fpclass = fp_quiet;
			} else {/* signaling */
				pu->fpclass = fp_signaling;
				fpu_set_exception(fp_invalid);
			}
			pu->significand[0] |= 0x40000000; /* make quiet */
			return;
		}
}
}

void
_fp_unpack(pu, n, dtype)
	unpacked       *pu;	/* unpacked result */
	unsigned        n;	/* register where data starts */
	enum fp_op_type dtype;	/* type of datum */

{
	freg_type       f, fy, fz;

	switch ((int) dtype) {
	case fp_op_integer:
		_fp_current_read_freg(&f, n);
		unpackinteger(pu, f.int_reg);
		break;
	case fp_op_single:
		_fp_current_read_freg(&f, n);
		unpacksingle(pu, f.single_reg);
		break;
	case fp_op_double:
		_fp_current_read_freg(&f, DOUBLE_E(n));
		_fp_current_read_freg(&fy, DOUBLE_F(n));
		unpackdouble(pu, f.double_reg, fy.unsigned_reg);
		break;
	case fp_op_extended:
		_fp_current_read_freg(&f, EXTENDED_E(n));
		_fp_current_read_freg(&fy, EXTENDED_F(n));
		_fp_current_read_freg(&fz, EXTENDED_FLOW(n));
		unpackextended(pu, f.extended_reg, fy.unsigned_reg, fz.unsigned_reg);
		break;
	}
}

void
_fp_unpack_word(pu, n)
	unsigned       *pu;	/* unpacked result */
	unsigned        n;	/* register where data starts */
{
	_fp_current_read_freg(pu, n);
}
#endif /* lint */
