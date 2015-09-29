
/*	@(#)diag.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Header file for the results from the Sun-2 power-up diagnostics.
 * 
 * All these results are returned in registers which are stored by
 * assembler code into the struct diag_state in global memory.
 *
 * The <ds_damages> field provides summary information about the
 * state of the world.  Each 1-bit indicates that something is working.
 */

#ifndef CXBIT
/*
 * Damage control bits in diag_state.ds_damages; if off, indicates failure.
 */
#define PROMBIT 7		/* PROMs unreadable */
#define	CXBIT 8			/* context register value */
#define	SEGBIT 9		/* segment map entry */
#define	PMBIT 10		/* page map entry */
#define	MEMBIT 11		/* page of memory */
#define	DESBIT 12		/* DES chip has error (notfound not an error) */
#define	DESFOUND 13		/* DES chip was found */
/* The following bits are also cleared in the relevant page's pmap entry. */
#define	MCONBIT 25		/* memory failed constant data test */
#define	MADDRBIT 26		/* Mem failed address independence test */
#define	MTRANBIT 27		/* Mem failed in a transient way */


struct diag_state {
	unsigned long	ds_d0;
	unsigned long	ds_d1;
	unsigned long	ds_damages;	/* Damage control summary bits */
	unsigned long	ds_errcount;	/* Total # of errors encountered */
	unsigned long	ds_tranerrs;	/* Total # of early transient errors */
	unsigned long	ds_d5;
	unsigned long	ds_d6;
	unsigned long	ds_d7;
	/* Next four entries are tied together and apply to the most recent
	   error found -- not necessarily the most severe or basic. */
	char	*ds_message;		/* Error message string */
	unsigned long	ds_addr;	/* Failing address */
	unsigned long	ds_wrote;	/* Data we wrote to address */
	unsigned long	ds_read;	/* Data we read back from address */
	/* End of entries for most recent error */
	unsigned long	ds_a4;
	unsigned long	ds_a5;
	unsigned long	ds_mainmemsize;
	unsigned long	ds_a7;
};

/*
 * How long to loop (dbra to self) while holding an LED value steady.
 */
#define	LEDQUICK	50000
#define	LEDLONG		250000

#define DIAGREG         0x70000000
#endif
