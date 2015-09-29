#ifdef sccsid
static char     sccsid[] = "@(#)utility.c 1.7 88/02/08 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Utility functions for Sparc FPU simulator. */

/*
 * This whole file refuses to lint.
 */
#ifndef lint
#include <sun4/fpu/fpu_simulator.h>
#include <sun4/fpu/globals.h>

void
_fp_read_vfreg(pf, n)
	FPU_REGS_TYPE  *pf;	/* Old freg value. */
	unsigned        n;	/* Want to read register n. */

{
	*pf = _fp_current_pfregs->fpu_regs[n];
}

void
_fp_write_vfreg(pf, n)
	FPU_REGS_TYPE  *pf;	/* New freg value. */
	unsigned        n;	/* Want to read register n. */

{
	_fp_current_pfregs->fpu_regs[n] = *pf;
}

void
fpu_normalize(pu)
	unpacked       *pu;

/* Normalize a number.  Does not affect zeros, infs, or NaNs. */

{
	if ((*pu).fpclass == fp_normal) {
		if (((*pu).significand[0] | (*pu).significand[1] | (*pu).significand[2]) == 0) {	/* true zero */
			(*pu).fpclass = fp_zero;
			return;
		}
		while ((*pu).significand[0] == 0) {
			(*pu).significand[0] = (*pu).significand[1];
			(*pu).significand[1] = (*pu).significand[2];
			(*pu).significand[2] = 0;
			(*pu).exponent = (*pu).exponent - 32;
		}
		while ((*pu).significand[0] < 0x80000000) {
			pu->significand[0] = pu->significand[0] << 1;
			if (pu->significand[1] >= 0x80000000)
				pu->significand[0] += 1;
			pu->significand[1] = pu->significand[1] << 1;
			if (pu->significand[2] >= 0x80000000)
				pu->significand[1] += 1;
			pu->significand[2] = pu->significand[2] << 1;
			(*pu).exponent = (*pu).exponent - 1;
		}
	}
}

void
fpu_rightshift(pu, n)
	unpacked       *pu;
	int             n;

/* Right shift significand sticky by n bits.  */

{
	if (n >= 66) {		/* drastic */
		if (((*pu).significand[0] | (*pu).significand[1] | (*pu).significand[2]) == 0) {	/* really zero */
			pu->fpclass = fp_zero;
			return;
		} else {
			pu->significand[2] = 0x20000000;
			pu->significand[1] = 0;
			pu->significand[0] = 0;
			return;
		}
	}
	while (n >= 32) {	/* big shift */
		if (pu->significand[2] != 0)
			pu->significand[1] |= 1;
		(*pu).significand[2] = (*pu).significand[1];
		(*pu).significand[1] = (*pu).significand[0];
		(*pu).significand[0] = 0;
		n -= 32;
	}
	while (n > 0) {		/* small shift */
		if (pu->significand[2] & 1)
			pu->significand[2] |= 2;
		pu->significand[2] = pu->significand[2] >> 1;
		if (pu->significand[1] & 1)
			pu->significand[2] |= 0x80000000;
		pu->significand[1] = pu->significand[1] >> 1;
		if (pu->significand[0] & 1)
			pu->significand[1] |= 0x80000000;
		pu->significand[0] = pu->significand[0] >> 1;
		n -= 1;
	}
}

void
fpu_set_exception(ex)
	enum fp_exception_type ex;

/* Set the exception bit in the current exception register. */

{
	_fp_current_exceptions |= 1 << (int) ex;
}

void
fpu_error_nan(pu)
	unpacked       *pu;

{				/* Set invalid exception and error nan in *pu */

	fpu_set_exception(fp_invalid);
	pu->significand[0] = 0xffffffff;
	pu->significand[1] = 0xffffffff;
	pu->sign = 0;		/* MC68881 always returns +NaN, so so do we. */
}

#ifdef DEBUG
void
display_unpacked(pu)
	unpacked       *pu;

/* Print out unpacked record.	 */

{
	(void) printf(" unpacked ");
	if (pu->sign)
		(void) printf("-");
	else
		(void) printf("+");

	switch (pu->fpclass) {
	case fp_zero:
		(void) printf("0");
		break;
	case fp_normal:
		(void) printf("normal");
		break;
	case fp_infinity:
		(void) printf("Inf");
		break;
	case fp_quiet:
	case fp_signaling:
		(void) printf("nan");
		break;
	}
	(void) printf(" %X %X %X exponent %X \n", pu->significand[0], pu->significand[1], pu->significand[2],
		      pu->exponent);
}
#endif
#endif /* lint */
