
/*	@(#)exception.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#define	SHORT_FORMAT	0x0
#define	LONG_FORMAT	0x8

struct ex_info {
	u_short		e_sr;		/* status register */
	u_char		*e_pc;		/* program counter */
	unsigned	e_format:4;	/* 0 is short, 1000 is long */
	unsigned	e_offset:12;	/* vector offset */
	unsigned	e_rr:1;		/* rerun flag 0 = cpu 1 = software */
	unsigned	:1;		/* MBZ */
	unsigned	e_if:1;		/* was inst fetch */
	unsigned	e_df:1;		/* was data fetch to input buffer */
	unsigned	e_rm:1;		/* was read/modify/write cycle */
	unsigned	e_hb:1;		/* high byte transfer on data */
	unsigned	e_by:1;		/* 0 is word, 1 is HB selects byte */
	unsigned	e_rw:1;		/* read write flag 0 write 1 read */
	unsigned	:5;		/* MBZ */
	unsigned	e_fc:3;		/* function code */
	u_char		*e_fault;	/* fault address */
	unsigned	:16;		/* not used */
	u_short		e_dataout;	/* data output buffer */
	unsigned	:16;		/* not used */
	u_short		e_datain;	/* data input buffer */
	unsigned	:16;		/* not used */
	u_short		e_inst;		/* instruction input buffer */
	u_short		e_internal[16];	/* internal state info */
};
	
extern int		exception_print;	/* non-zero means print */
extern int		ex_handler();		/* used to declare vector */
extern int		(*exception_handler)();	/* user routine to call with
						 * ex_info struct as arg 
						 * zero implies no use */
