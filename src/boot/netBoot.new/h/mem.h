
/*	@(#)mem.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *	defines routines available, and a structure template
 *	for making callout test setups.
 */

struct	mem_call {
	char	*m_name;		/* name for printout?? */
	int	m_size,			/* to compute address with index */
		(*m_fill)(),		/* memory fill routine to use should */
		(*m_check)(),		/* match check routine to use */
		m_p1, m_p2, m_p3, m_p4;	/* parameters to pass after addr,size */
};

extern u_long	obs_value;

int	bpfill(), wpfill(), lpfill();
	bpcheck(), wpcheck(), lpcheck(),
	o_bpcheck(), o_wpcheck(), o_lpcheck();

int	bufill(), wufill(), lufill();
	bucheck(), wucheck(), lucheck(),
	o_bucheck(), o_wucheck(), o_lucheck();

int	brfill(), wrfill(), lrfill();
	brcheck(), wrcheck(), lrcheck(),
	o_brcheck(), o_wrcheck(), o_lrcheck();
