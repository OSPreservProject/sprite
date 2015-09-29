
/*	@(#)pginit.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * pginit.h -- structure used to define large page map areas for initialization
 */
struct pginit {
	char		*pi_addr;	/* Virtual address to map at */
	unsigned short	pi_incr;	/* page # increment each time */
	struct pgmapent	pi_pm;		/* The first page map entry */
};

#define PGINITEND	0x8000		/* incr value to end table */
#define PGINITSKIP	0x8001		/* incr value to skip to next entry */

