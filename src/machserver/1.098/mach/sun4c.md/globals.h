/*	@(#)globals.h 1.8 88/02/08 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

	/*	Sparc floating-point simulator PRIVATE include file. */

/*
 * This whole file refuses to lint.
 */
#ifndef lint
#include <sys/types.h>
#include "sprite.h"
#include "mach.h"

/*
 * A few macro's for unix compatiblity.
 */

#define	regs Mach_RegState
#define	r_pc	pc
#define r_npc	nextPc
#define	r_g1	globals[1]
#define	r_o0	ins[0]

#define	rwindow	Mach_RegWindow
#define	rw_local locals
#define	rw_in	 ins
#define fpu  Mach_RegState
#define fpu_fsr	fsr
#define fpu_regs fregs
#define	_fp_read_pfreg _fp_read_vfreg
#define	_fp_write_pfreg _fp_write_vfreg

enum seg_rw {S_WRITE, S_READ};
#define KERNELBASE	0xc0000000

#define FPU_REGS_TYPE	unsigned

#define FPE_FLTOPERR_TRAP 1
#define FPE_FLTOVF_TRAP 1
#define FPE_FLTUND_TRAP 1
#define FPE_FLTDIV_TRAP 1
#define FPE_FLTINEX_TRAP 1

	/*	PRIVATE CONSTANTS	*/

#define INTEGER_BIAS	   31
#define	SINGLE_BIAS	  127
#define DOUBLE_BIAS	 1023
#define EXTENDED_BIAS	16383

	/* PRIVATE TYPES	*/

#ifdef DEBUG
#define PRIVATE
#else
#define PRIVATE static
#endif

#define	DOUBLE_E(n) (n & 0xfffe) /* More significant word of double. */
#define DOUBLE_F(n) (1+DOUBLE_E(n)) /* Less significant word of double. */
#define	EXTENDED_E(n) (n & 0xfffc) /* Sign/exponent word of extended. */
#define EXTENDED_F(n) (1+EXTENDED_E(n)) /* More significant word of extended significand. */
#define EXTENDED_FLOW(n) (2+EXTENDED_E(n)) /* Less significant word of extended significand. */
#define EXTENDED_U(n) (3+EXTENDED_E(n)) /* Unused word of extended. */

typedef
	struct
	{
	int sign ;
	enum fp_class_type fpclass ;
	int	exponent ;		/* Unbiased exponent. */
	unsigned significand[3] ;	/* Third word is round and sticky. */
	}
	unpacked ;

	/* PRIVATE GLOBAL VARIABLES */

enum fp_direction_type fp_direction ;	/* Current rounding direction. */
enum fp_precision_type fp_precision ;	/* Current extended rounding precision. */

unsigned	_fp_current_exceptions ; /* Current floating-point exceptions. */

struct fpu	* _fp_current_pfregs ;		/* Current pointer to stored f registers. */

void	(* _fp_current_read_freg) () ;	/* Routine to use to read f registers. */
void	(* _fp_current_write_freg) () ;	/* Routine to use to write f registers. */

int		fptrapcode ;		/* Last code for fp trap. */
char		*fptrapaddr ;		/* Last addr for fp trap. */
enum seg_rw	fptraprw ;		/* Last fp fault read/write flag */

	/* PRIVATE FUNCTIONS */

	/* pfreg routines use "physical" FPU registers. */

extern void _fp_read_pfreg ( /* pf, n */ ) ;

/*	FPU_REGS_TYPE *pf		/* Where to put current %fn. */
/*	unsigned n ;			/* Want to read register n. */

extern void _fp_write_pfreg ( /* pf, n */ ) ;

/*	FPU_REGS_TYPE *pf		/* Where to get new %fn. */
/*	unsigned n ;			/* Want to read register n. */

	/* vfreg routines use "virtual" FPU registers at *_fp_current_pfregs. */

extern void _fp_read_vfreg ( /* pf, n */ ) ;

/*	FPU_REGS_TYPE *pf		/* Where to put current %fn. */
/*	unsigned n ;			/* Want to read register n. */

extern void _fp_write_vfreg ( /* pf, n */ ) ;

/*	FPU_REGS_TYPE *pf		/* Where to get new %fn. */
/*	unsigned n ;			/* Want to read register n. */

extern enum ftt_type
_fp_iu_simulator( /* pinst, pregs, pwindow, pfpu */ ) ;
/*	fp_inst_type	pinst;	/* FPU instruction to simulate. */
/*	struct regs	*pregs;	/* Pointer to PCB image of registers. */
/*	struct window	*pwindow;/* Pointer to locals and ins. */
/*	struct fpu	*pfpu;	/* Pointer to FPU register block. */

extern void _fp_unpack ( /* pu, n, type */ ) ;
/*	unpacked	*pu ;	/* unpacked result */
/*	unsigned	n ;	/* register where data starts */
/*	fp_op_type	type ;	/* type of datum */

extern void _fp_pack ( /* pu, n, type */) ;
/*	unpacked	*pu ;	/* unpacked result */
/*	unsigned	n ;	/* register where data starts */
/*	fp_op_type	type ;	/* type of datum */

extern void _fp_unpack_word ( /* pu, n, type */ ) ;
/*	unsigned	*pu ;	/* unpacked result */
/*	unsigned	n ;	/* register where data starts */

extern void _fp_pack_word ( /* pu, n, type */) ;
/*	unsigned	*pu ;	/* unpacked result */
/*	unsigned	n ;	/* register where data starts */

extern void fpu_normalize (/* pu */) ;
/*	unpacked	*pu ;	/* unpacked operand and result */

extern void fpu_rightshift (/* pu, n */) ;
/*	unpacked *pu ; unsigned n ;	*/
/*	Right shift significand sticky by n bits. */

extern void fpu_set_exception(/* ex */) ;
/*	enum fp_exception_type ex ;	/* exception to be set in curexcep */

extern void fpu_error_nan(/* pu */) ;
/*	unpacked *pu ; 			/* Set invalid exception and error nan in *pu */

extern void unpacksingle (/* pu, x */) ;
/*	unpacked	*pu;	/* packed result */
/*	single_type	x;	/* packed single */

extern void unpackdouble (/* pu, x, y */) ;
/*	unpacked	*pu;	/* unpacked result */
/*	double_type	x;	/* packed double */
/*	unsigned	y;	*/

extern void _fp_product ( /* x, y, z */ ) ;
/*
unsig x, y;
unsigned z[2] ;

/*	Computes product x * y into *z[2] */

extern enum fcc_type _fp_compare (/* px, py */) ;

extern void _fp_add(/* px, py, pz */) ;
extern void _fp_sub(/* px, py, pz */) ;
extern void _fp_mul(/* px, py, pz */) ;
extern void _fp_div(/* px, py, pz */) ;
extern void _fp_sqrt(/* px, pz */) ;

extern enum ftt_type	_fp_write_word ( /* caddr_t, value */ ) ;
extern enum ftt_type	_fp_read_word ( /* caddr_t, pvalue */ ) ;
extern enum ftt_type	read_iureg ( /* n, pregs, pwindow, pvalue */ );
#endif /* lint */
