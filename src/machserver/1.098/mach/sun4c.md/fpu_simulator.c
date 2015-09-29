#ifdef sccsid
static char	sccsid[] = "@(#)fpu_simulator.c 1.8 88/02/08 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Main procedures for Sparc FPU simulator. */

/*
 * This whole file refuses to lint.
 */
#ifndef lint
#include <sun4/fpu/fpu_simulator.h>
#include <sun4/fpu/globals.h>
#include <sys/signal.h>

/* PUBLIC FUNCTIONS */

PRIVATE enum ftt_type
_fp_fpu_simulator(inst, pfsr)
	fp_inst_type inst;	/* FPU instruction to simulate. */
	fsr_type	*pfsr;	/* Pointer to image of FSR to read and write. */

{
	unpacked	us1, us2, ud;	/* Unpacked operands and result. */
	unsigned	nrs1, nrs2, nrd;	/* Register number fields. */
	fsr_type	fsr;
	unsigned	usr;
	unsigned	andexcep;

	nrs1 = inst.rs1;
	nrs2 = inst.rs2;
	nrd = inst.rd;
	_fp_current_exceptions = 0;	/* Initialize current exceptions. */
	fsr = *pfsr;
	fp_direction = fsr.rd;	/* Obtain rounding direction. */
	fp_precision = fsr.rp;	/* Obtain rounding direction. */
	switch (inst.opcode) {
	case fmovs:
		_fp_unpack_word(&usr, nrs2);
		_fp_pack_word(&usr, nrd);
		break;
	case fabss:
		_fp_unpack_word(&usr, nrs2);
		usr &= 0x7fffffff;
		_fp_pack_word(&usr, nrd);
		break;
	case fnegs:
		_fp_unpack_word(&usr, nrs2);
		usr ^= 0x80000000;
		_fp_pack_word(&usr, nrd);
		break;
	case fadd:
		_fp_unpack(&us1, nrs1, inst.prec);
		_fp_unpack(&us2, nrs2, inst.prec);
		_fp_add(&us1, &us2, &ud);
		_fp_pack(&ud, nrd, inst.prec);
		break;
	case fsub:
		_fp_unpack(&us1, nrs1, inst.prec);
		_fp_unpack(&us2, nrs2, inst.prec);
		_fp_sub(&us1, &us2, &ud);
		_fp_pack(&ud, nrd, inst.prec);
		break;
	case fmul:
		_fp_unpack(&us1, nrs1, inst.prec);
		_fp_unpack(&us2, nrs2, inst.prec);
		_fp_mul(&us1, &us2, &ud);
		_fp_pack(&ud, nrd, inst.prec);
		break;
	case fdiv:
		_fp_unpack(&us1, nrs1, inst.prec);
		_fp_unpack(&us2, nrs2, inst.prec);
		_fp_div(&us1, &us2, &ud);
		_fp_pack(&ud, nrd, inst.prec);
		break;
	case fcmp:
		_fp_unpack(&us1, nrs1, inst.prec);
		_fp_unpack(&us2, nrs2, inst.prec);
		fsr.fcc = _fp_compare(&us1, &us2, 0);
		break;
	case fcmpe:
		_fp_unpack(&us1, nrs1, inst.prec);
		_fp_unpack(&us2, nrs2, inst.prec);
		fsr.fcc = _fp_compare(&us1, &us2, 1);
		break;
	case fsqrt:
		_fp_unpack(&us1, nrs2, inst.prec);
		_fp_sqrt(&us1, &ud);
		_fp_pack(&ud, nrd, inst.prec);
		break;
	case ftoi:
		_fp_unpack(&us1, nrs2, inst.prec);
		fp_direction = fp_tozero;
		/* Force rounding toward zero. */
		_fp_pack(&us1, nrd, fp_op_integer);
		break;
	case ftos:
		_fp_unpack(&us1, nrs2, inst.prec);
		_fp_pack(&us1, nrd, fp_op_single);
		break;
	case ftod:
		_fp_unpack(&us1, nrs2, inst.prec);
		_fp_pack(&us1, nrd, fp_op_double);
		break;
	case ftox:
		_fp_unpack(&us1, nrs2, inst.prec);
		_fp_pack(&us1, nrd, fp_op_extended);
		break;
	default:
		return (ftt_unimplemented);
	}

	fsr.cexc = _fp_current_exceptions;
	if (_fp_current_exceptions) {	/* One or more exceptions occurred. */
		andexcep = _fp_current_exceptions & fsr.tem;
		if (andexcep != 0) {	/* Signal an IEEE SIGFPE here. */
			if (andexcep & (1 << fp_invalid))
				fptrapcode = FPE_FLTOPERR_TRAP;
			else if (andexcep & (1 << fp_overflow))
				fptrapcode = FPE_FLTOVF_TRAP;
			else if (andexcep & (1 << fp_underflow))
				fptrapcode = FPE_FLTUND_TRAP;
			else if (andexcep & (1 << fp_division))
				fptrapcode = FPE_FLTDIV_TRAP;
			else if (andexcep & (1 << fp_inexact))
				fptrapcode = FPE_FLTINEX_TRAP;
			else
				fptrapcode = 0;
			*pfsr = fsr;
			return (ftt_ieee);
		} else {	/* Just set accrued exception field. */
			fsr.aexc |= _fp_current_exceptions;
		}
	}
	*pfsr = fsr;
	return (ftt_none);
}

enum ftt_type
fpu_simulator(pinst, pfsr)
	fp_inst_type	*pinst;	/* Pointer to FPU instruction to simulate. */
	fsr_type	*pfsr;	/* Pointer to image of FSR to read and write. */

/*
 * fpu_simulator simulates FPU instructions only; reads and writes FPU data
 * registers directly.
 */
{
	union {
		int		i;
		fp_inst_type	inst;
	}		kluge;
	enum ftt_type	ftt;

	fptrapaddr = (char *) pinst;	/* bad inst addr in case we trap */
	_fp_current_read_freg = _fp_read_pfreg;
	_fp_current_write_freg = _fp_write_pfreg;
	ftt = _fp_read_word((caddr_t) pinst, &(kluge.i));
	if (ftt != ftt_none)
		return (ftt);
	return _fp_fpu_simulator(kluge.inst, pfsr);
}

enum ftt_type
fp_emulator(pinst, pregs, pwindow, pfpu)
	fp_inst_type	*pinst;	/* Pointer to FPU instruction to simulate. */
	struct regs	*pregs;	/* Pointer to PCB image of registers. */
	struct rwindow	*pwindow;/* Pointer to locals and ins. */
	struct fpu	*pfpu;	/* Pointer to FPU register block. */

/*
 * fp_emulator simulates FPU and CPU-FPU instructions; reads and writes FPU
 * data registers from image in pfpu.
 */

{
	union {
		int		i;
		fp_inst_type	inst;
	}		kluge;
	enum ftt_type	ftt;

	fptrapaddr = (char *) pinst;	/* bad inst addr in case we trap */
	_fp_current_pfregs = pfpu;
	_fp_current_read_freg = _fp_read_vfreg;
	_fp_current_write_freg = _fp_write_vfreg;
	ftt = _fp_read_word((caddr_t) pinst, &(kluge.i));
	if (ftt != ftt_none)
		return (ftt);
	if ((kluge.inst.hibits == 2) && ((kluge.inst.op3 == 0x34) || (kluge.inst.op3 == 0x35))) {
		pregs->r_pc = pregs->r_npc;	/* Do not retry emulated
						 * instruction. */
		pregs->r_npc += 4;
		return _fp_fpu_simulator(kluge.inst, (fsr_type *) & (pfpu->fpu_fsr));
	} else
		return _fp_iu_simulator(kluge.inst, pregs, pwindow, pfpu);
}
#ifndef sprite
#include <sys/param.h>
#include <sys/user.h>
#include <sun4/trap.h>

/*
 * Handle traps for UNIX.
 */
void
fp_traps(ftt, rp)
	register enum ftt_type ftt;	/* trap type */
	register struct regs *rp;	/* ptr to regs fro trap */
{
	extern void trap();

	switch(ftt) {
	case ftt_ieee:
		u.u_code = fptrapcode;
		trap(T_FP_EXCEPTION, rp, fptrapaddr, 0, 0);
		break;
	case ftt_fault:
		trap(T_DATA_FAULT, rp, fptrapaddr, 0, fptraprw);
		break;
	case ftt_alignment:
		trap(T_ALIGNMENT, rp, fptrapaddr, 0, 0);
		break;
	case ftt_unimplemented:
		trap(T_UNIMP_INSTR, rp, fptrapaddr, 0, 0);
		break;
	default:
		/*
		 * We don't expect any of the other types here.
		 */
		panic("fp_traps: bad ftt");
	}
}
#endif
#endif /* lint */
