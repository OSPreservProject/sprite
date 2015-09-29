/* @(#)cg8var.h	1.7 of 2/27/89 SMI */

/*
 * Copyright 1988 by Sun Microsystems, Inc.
 */

#ifndef cg8var_DEFINED
#define cg8var_DEFINED

/* <sundev/p4reg.h> and <pixrect/memvar.h> included */
#include <sundev/cg8reg.h>
#include <pixrect/cg4var.h>	       /* for struct cg4fb */

/* FBIOSATTR device specific array indices, copied from cg4var.h */
#define FB_ATTR_CG8_SETOWNER_CMD	0	/* 1 indicates PID is valid */
#define	FB_ATTR_CG8_SETOWNER_PID	1	/* new owner of device */


#define CG8_NFBS	3

#define	CG8_PRIMARY	CG4_PRIMARY      /* primary pixrect */
#define CG8_OVERLAY_CMAP CG4_OVERLAY_CMAP
#define CG8_UPDATE_PENDING 0x4	/* used by kernel */
#define CG8_KERNEL_UPDATE 0x8	/* in kernel mode */
#define	CG8_24BIT_CMAP		0x10
#define	CG8_SLEEPING		0x20

#define cg8_data cg4_data
#define cg8_d(pr)	((struct cg8_data *)((pr)->pr_data))

extern struct pixrectops cg8_ops;

int             cg8_putcolormap ();
int             cg8_putattributes ();

#ifndef KERNEL

Pixrect        *cg8_make ();
int		cg8_destroy ();
Pixrect        *cg8_region ();
int             cg8_getcolormap ();
int             cg8_getattributes ();

#endif	!KERNEL

#endif cg8var_DEFINED
