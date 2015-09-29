#ifdef sccsid
static char     sccsid[] = "@(#)pack.c 1.7 88/03/03 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Pack procedures for Sparc FPU simulator. */

/*
 * This whole file refuses to lint.
 */
#ifndef lint
#include <sun4/fpu/fpu_simulator.h>
#include <sun4/fpu/globals.h>

PRIVATE int
overflow_to_infinity(sign)
	int             sign;

/* Returns 1 if overflow should go to infinity, 0 if to max finite. */

{
	int             inf;

	switch (fp_direction) {
	case fp_nearest:
		inf = 1;
		break;
	case fp_tozero:
		inf = 0;
		break;
	case fp_positive:
		inf = !sign;
		break;
	case fp_negative:
		inf = sign;
		break;
	}
	return (inf);
}

PRIVATE void
round(pu)
	unpacked       *pu;

/* Round according to current rounding mode.	 */

{
	int             increment;	/* boolean to indicate round up */

	if (pu->significand[2] == 0)
		return;
	fpu_set_exception(fp_inexact);
	switch (fp_direction) {
	case fp_nearest:
		increment = pu->significand[2] >= 0x80000000;
		break;
	case fp_tozero:
		increment = 0;
		break;
	case fp_positive:
		increment = (pu->sign == 0) & (pu->significand[2] != 0);
		break;
	case fp_negative:
		increment = (pu->sign != 0) & (pu->significand[2] != 0);
		break;
	}
	if (increment) {
		pu->significand[1]++;
		if (pu->significand[1] == 0) {
			pu->significand[0]++;
			if (pu->significand[0] == 0) {	/* rounding carried out */
				pu->exponent++;
				pu->significand[0] = 0x80000000;
			}
		}
	}
	if ((fp_direction == fp_nearest) && (pu->significand[2] == 0x80000000)) {	/* ambiguous case */
		pu->significand[1] &= 0xfffffffe;	/* force round to even */
	}
}

PRIVATE void
packinteger(pu, px)
	unpacked       *pu;	/* unpacked result */
	int            *px;	/* packed integer */
{
	switch (pu->fpclass) {
	case fp_zero:
		*px = 0;
		break;
	case fp_normal:
		if (pu->exponent >= 32)
			goto overflow;
		fpu_rightshift(pu, 63 - pu->exponent);
		round(pu);
		if (pu->significand[0] != 0)
			goto overflow;
		if (pu->significand[1] >= 0x80000000)
			if ((pu->sign == 0) || (pu->significand[1] > 0x80000000))
				goto overflow;
		*px = pu->significand[1];
		if (pu->sign)
			*px = -*px;
		break;
	case fp_infinity:
	case fp_quiet:
	case fp_signaling:
overflow:
		if (pu->sign)
			*px = 0x80000000;
		else
			*px = 0x7fffffff;
		_fp_current_exceptions &= ~(1 << (int) fp_inexact);
		fpu_set_exception(fp_invalid);
		break;
	}
}

PRIVATE void
packsingle(pu, px)
	unpacked       *pu;	/* unpacked result */
	single_type    *px;	/* packed single */
{
	px->sign = pu->sign;
	switch (pu->fpclass) {
	case fp_zero:
		px->exponent = 0;
		px->significand = 0;
		break;
	case fp_infinity:
infinity:
		px->exponent = 0xff;
		px->significand = 0;
		break;
	case fp_quiet:
	case fp_signaling:
		px->exponent = 0xff;
		px->significand = 0x400000 | (0x3fffff & (pu->significand[0] >> 8));
		break;
	case fp_normal:
		fpu_rightshift(pu, 40);
		pu->exponent += SINGLE_BIAS;
		if (pu->exponent <= 0) {
			px->exponent = 0;
			fpu_rightshift(pu, 1 - pu->exponent);
			round(pu);
			if (pu->significand[1] == 0x800000) {	/* rounded back up to
								 * normal */
				px->exponent = 1;
				px->significand = 0;
				return;
			}
			if (_fp_current_exceptions & (1 << fp_inexact))
				fpu_set_exception(fp_underflow);
			px->significand = 0x7fffff & pu->significand[1];
			return;
		}
		round(pu);
		if (pu->significand[1] == 0x1000000) {	/* rounding overflow */
			pu->significand[1] = 0x800000;
			pu->exponent += 1;
		}
		if (pu->exponent >= 0xff) {
			fpu_set_exception(fp_overflow);
			fpu_set_exception(fp_inexact);
			if (overflow_to_infinity(pu->sign))
				goto infinity;
			px->exponent = 0xfe;
			px->significand = 0x7fffff;
			return;
		}
		px->exponent = pu->exponent;
		px->significand = 0x7fffff & pu->significand[1];
	}
}

PRIVATE void
packdouble(pu, px, py)
	unpacked       *pu;	/* unpacked result */
	double_type    *px;	/* packed double */
	unsigned       *py;
{
	px->sign = pu->sign;
	switch (pu->fpclass) {
	case fp_zero:
		px->exponent = 0;
		px->significand = 0;
		*py = 0;
		break;
	case fp_infinity:
infinity:
		px->exponent = 0x7ff;
		px->significand = 0;
		*py = 0;
		break;
	case fp_quiet:
	case fp_signaling:
		px->exponent = 0x7ff;
		fpu_rightshift(pu, 11);
		px->significand = 0x80000 | (0x7ffff & pu->significand[0]);
		*py = pu->significand[1];
		break;
	case fp_normal:
		fpu_rightshift(pu, 11);
		pu->exponent += DOUBLE_BIAS;
		if (pu->exponent <= 0) {	/* underflow */
			px->exponent = 0;
			fpu_rightshift(pu, 1 - pu->exponent);
			round(pu);
			if (pu->significand[0] == 0x100000) {	/* rounded back up to
								 * normal */
				px->exponent = 1;
				px->significand = 0;
				*py = 0;
				return;
			}
			if (_fp_current_exceptions & (1 << fp_inexact))
				fpu_set_exception(fp_underflow);
			px->exponent = 0;
			px->significand = 0xfffff & pu->significand[0];
			*py = pu->significand[1];
			return;
		}
		round(pu);
		if (pu->significand[0] == 0x200000) {	/* rounding overflow */
			pu->significand[0] = 0x100000;
			pu->exponent += 1;
		}
		if (pu->exponent >= 0x7ff) {	/* overflow */
			fpu_set_exception(fp_overflow);
			fpu_set_exception(fp_inexact);
			if (overflow_to_infinity(pu->sign))
				goto infinity;
			px->exponent = 0x7fe;
			px->significand = 0xfffff;
			*py = 0xffffffff;
			return;
		}
		px->exponent = pu->exponent;
		px->significand = 0xfffff & pu->significand[0];
		*py = pu->significand[1];
		break;
	}
}

PRIVATE void
packextended(pu, px, py, pz)
	unpacked       *pu;	/* unpacked result */
	extended_type  *px;	/* packed extended */
	unsigned       *py, *pz;
{
	px->sign = pu->sign;
	px->unused = 0;
	switch (pu->fpclass) {
	case fp_zero:
		px->exponent = 0;
		*pz = 0;
		*py = 0;
		break;
	case fp_infinity:
infinity:
		px->exponent = 0x7fff;
		*pz = 0;
		*py = 0;
		break;
	case fp_quiet:
	case fp_signaling:
		px->exponent = 0x7fff;
		*py = 0x40000000 | pu->significand[0];	/* Insure quiet nan. */
		*pz = pu->significand[1];
		break;
	case fp_normal:
		pu->exponent += EXTENDED_BIAS;
		if (pu->exponent < 0) {	/* underflow */
			fpu_rightshift(pu, -pu->exponent);
			round(pu);
			if (pu->significand[0] < 0x80000000) {	/* not rounded back up
								 * to normal */
				if (_fp_current_exceptions & (1 << fp_inexact))
					fpu_set_exception(fp_underflow);
			}
			px->exponent = 0;
			*py = pu->significand[0];
			*pz = pu->significand[1];
			return;
		}
		round(pu);	/* rounding overflow handled in round() */
		if (pu->exponent >= 0x7fff) {	/* overflow */
			fpu_set_exception(fp_overflow);
			fpu_set_exception(fp_inexact);
			if (overflow_to_infinity(pu->sign))
				goto infinity;
			px->exponent = 0x7ffe;	/* overflow to max norm */
			*py = 0xffffffff;
			*pz = 0xffffffff;
			return;
		}
		px->exponent = pu->exponent;
		*py = pu->significand[0];
		*pz = pu->significand[1];
		break;
	}
}

void
_fp_pack(pu, n, type)
	unpacked       *pu;	/* unpacked operand */
	unsigned        n;	/* register where datum starts */
	enum fp_op_type type;	/* type of datum */

{
	switch (type) {
	case fp_op_integer:
		{
			int             x;

			packinteger(pu, &x);
			_fp_current_write_freg(&x, n);
			break;
		}
	case fp_op_single:
		{
			single_type     x;

			packsingle(pu, &x);
			_fp_current_write_freg(&x, n);
			break;
		}
	case fp_op_double:
		{
			double_type     x;
			unsigned        y;

			packdouble(pu, &x, &y);
			_fp_current_write_freg(&x, DOUBLE_E(n));
			_fp_current_write_freg(&y, DOUBLE_F(n));
			break;
		}
	case fp_op_extended:
		{
			extended_type   x;
			unsigned        y, z;
			unsigned        unused = 0;
			unpacked        u;

			switch (fp_precision) {	/* Implement extended
						 * rounding precision mode. */
			case fp_single:
				{
					single_type     tx;

					packsingle(pu, &tx);
					pu = &u;
					unpacksingle(pu, tx);
					break;
				}
			case fp_double:
				{
					double_type     tx;
					unsigned        ty;

					packdouble(pu, &tx, &ty);
					pu = &u;
					unpackdouble(pu, tx, ty);
					break;
				}
			}
			packextended(pu, &x, &y, &z);
			_fp_current_write_freg(&x, EXTENDED_E(n));
			_fp_current_write_freg(&y, EXTENDED_F(n));
			_fp_current_write_freg(&z, EXTENDED_FLOW(n));
			_fp_current_write_freg(&unused, EXTENDED_U(n));
			break;
		}
	}
}

void
_fp_pack_word(pu, n)
	unsigned       *pu;	/* unpacked operand */
	unsigned        n;	/* register where datum starts */
{
	_fp_current_write_freg(pu, n);
}
#endif /* lint */
