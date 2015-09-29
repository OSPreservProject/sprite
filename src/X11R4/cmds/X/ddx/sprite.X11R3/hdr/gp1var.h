/* @(#)gp1var.h 1.10 88/02/08 SMI */
 
/*
 * Copyright 1985, 1987 by Sun Microsystems, Inc.
 */

#ifndef	gp1var_DEFINED
#define	gp1var_DEFINED

/*
 * The same pixrect data structure is now used for both cg2
 * and gp1 pixrects.
 */

#ifndef	cg2var_DEFINED
#include <pixrect/cg2var.h>
#endif

#define	gp1pr cg2pr

#define gp1_d(pr)		cg2_d(pr)
#define gp1_fbfrompr(pr)	cg2_fbfrompr(pr)

struct gp1_version {
	u_char majrel;
	u_char minrel;
	u_char serialnum;
	u_char flags;
};

#ifndef KERNEL
Pixrect *gp1_make();

extern struct pixrectops gp1_ops;

int gp1_rop();
int gp1_vector();
#endif !KERNEL

#endif	gp1var_DEFINED
