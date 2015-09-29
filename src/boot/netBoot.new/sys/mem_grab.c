
/*
 * @(#)mem_grab.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Memory raster get routine.  Same arguments and restrictions as batchrop, but
 * it copies OUT what batchrop would copy IN.  Count argument ignored.
 *
 * Note the use of this non-portable features:
 *	-- loopd6, which loops over the provided 2nd argument expression
 *	   until register d6 goes from 0 to -1.  Equivalent to:
 *		loopd6(a,b;) --> while((short)d6var-- != 0) b;
 * This is to generate better code.  It should be fixed by
 * fixing the C compiler to generate the better code in the portable case.
 */

#include "../h/types.h"
#include "../h/pixrect.h"
#include "../sun3/memreg.h"
#include "../h/memvar.h"
#include "../h/pr_util.h"

/* Read char from random pixrect into a little pixrect. */
/* Inverse of batchrop. */
/* For the moment, dst and src are reversed in names here. */
prom_mem_grab(dst, op, src, count)
	struct pr_prpos dst;
	int op;
	struct pr_prpos *src;
	short count;
{
	register u_short *sp;
	register char *dp;
	register char *handy;
	register sizex, sizey;	/* sizey must be d6 */
	register vert, dskew;

#ifdef lint
	count = count;  op = op;
#endif
#define dprd ((struct mpr_data *)handy)
	dprd = mpr_d(dst.pr);
	vert = dprd->md_linebytes;
	dp = (char *)mprd_addr(dprd, dst.pos.x, dst.pos.y);
	dskew = mprd_skew(dprd, dst.pos.x, dst.pos.y);
#undef dprd
#define spr ((struct pixrect *)handy)
	spr = src->pr;
	sizex = spr->pr_size.x;
	sizey = spr->pr_size.y;
#define sprd ((struct mpr_data *)handy)
	sprd = mpr_d(spr);
	sp = (u_short *)sprd->md_image;

		if (dskew + sizex <= 16) {
			loopd6(srca, *sp++ = *(u_short *)dp << dskew; dp += vert;)
		} else {
			dskew = 16 - dskew;
			loopd6(srcb, *sp++ = *(u_int *)dp >> dskew; dp += vert;)
		}
}
