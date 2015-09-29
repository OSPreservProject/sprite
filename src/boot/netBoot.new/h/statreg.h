
/*	@(#)statreg.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * statreg.h
 *
 * Bit definitions for MC68000 family processor status register (SR)
 * and other assorted registers.
 *
 * Taken from MC68020 Microprocessor User's Manual.
 */

#define	SR_C		0x1	/* C (carry) bit */
#define	SR_V		0x2	/* V (overflow) bit */
#define	SR_Z		0x4	/* Z (zero) bit */
#define	SR_N		0x8	/* N (negative) bit */
#define	SR_X		0x10	/* X (extend) bit */

#define	SR_CCODES	(SR_C|SR_V|SR_Z|SR_N|SR_X)	/* condition codes */

#define	SR_INTMASK	0x0700	/* interrupt level */

#define	SR_MASTER	0x1000	/* master versus interrupt stack bit */
#define	SR_SUPERVISOR	0x2000	/* supervisor state bit */
#define	SR_TRACE_FLOW	0x4000	/* trace trap on flow of control change */
#define	SR_TRACE	0x8000	/* trace trap after all instructions */
/* Note, both trace bits should not be on at once. */

/*
 * Cache control register bits
 */
#define	CACR_CLEAR	0x8	/* w/o Clear entire cache. */
#define	CACR_CLEAR_ENT	0x4	/* w/o Clear one entry based on CAAR */
#define	CACR_FREEZE	0x2	/* r/w Freeze cache, no new replacements */
#define	CACR_ENABLE	0x1	/* r/w Enable the cache for operation */
