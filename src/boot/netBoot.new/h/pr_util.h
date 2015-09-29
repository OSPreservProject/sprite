
/*	@(#)pr_util.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Utilities for implementing pixdev operations.
 */

/*
 * Aids to handling overlapping source and destination.
 * Given the from and to pr_pos's, rop_direction tells
 * whether the rasterop is up or down and left or right,
 * encoded as the ROP_UP and ROP_LEFT bits or their absence.
 * The macros rop_is(up|down|left|right) can then be used.
 */
#define	ROP_UP		0x1
#define	ROP_LEFT	0x2

#define	rop_direction(src, so, dst, do)	\
	    (   ( (((dst).x+(do).x) < ((src).x+(so).x)) << 1) | \
	          (((dst).y+(do).y) < ((src).y+(so).y))      )
#define	rop_isleft(dir)		((dir)&ROP_LEFT)
#define	rop_isup(dir)		((dir)&ROP_UP)
#define	rop_isright(dir)	(((dir)&ROP_LEFT)==0)
#define	rop_isdown(dir)		(((dir)&ROP_UP)==0)

/*
 * Aids to producing fast loops, either unrolled or very tight:
 *
 * Cases8(n, op) produces the dense case part of a case statement
 * for the cases [n+1..n+8), repeating ``op'' 1-8 times respectively.
 *
 * Rop_slowloop(n, op) produces a loop to do ``op'' n times, in little space.
 * 
 * Rop_fastloop(n, op) produces a loop to do ``op'' n times, in little time.
 *
 * Loop_d6(label, op) produces a dbra loop to do ``op'' the number of times
 * in register d6 (second non-pointer register variable).
 */
#define cases8(n, op)							\
	    case (n)+8: op; case (n)+7: op; case (n)+6: op;		\
	    case (n)+5: op; case (n)+4: op; case (n)+3: op;		\
	    case (n)+2: op; case (n)+1: op;				\

#define	rop_slowloop(n, op)						\
	{ register int j = n; while (--j >= 0) { op; }}

#define	rop_fastloop(n, op)						\
	{ register int j ;						\
	  for (j = n; j > 15; j -= 16)					\
	    { op; op; op; op; op; op; op; op;				\
	      op; op; op; op; op; op; op; op; }				\
	  switch (j) {							\
		cases8(8, op);						\
		cases8(0, op);						\
		case 0:	break;						\
	  }								\
	}

#define loopd6(label, op)						\
	if (0) {							\
		asm("label:");						\
		op;							\
	};								\
	asm("dbra	d6,label");

/*
 * Alloctype(datatype) allocates a datatype structure using calloc
 * with the appropriate type cast.
 */
#define	alloctype(datatype)						\
	    (datatype *)calloc(1, sizeof (datatype))

/*
 * Pr_product is used when doing multiplications involving pixrects,
 * and casts its arguments to that the compiler will use 16 by 16 multiplies.
 */
#ifdef sun
#define pr_product(a, b)	((short)(a) * (short)(b))
#else
#define pr_product(a, b)	((a) * (b))
#endif

/*
 * Pr_area is the area of a rectangle.
 */
#define pr_area(size) pr_product((size).x, (size).y)

/*
 * Pr_devdata is used to keep track of the valloced/mmapped virtual
 * address of a device to prevent doing it more than necessary.
 */
struct pr_devdata {
	int	fd;	/* fd that pixrect owns and is expected to close */
	int	count;	/* reference count of this device */
	short	*va; 	/* valloced/mmapped virtual address managing */
	int	bytes;	/* size of va for use when unvalloc unmap */
	dev_t	rdev;	/* the device type, fd independent id of device */
	struct	pr_devdata *next;	/* Link to other similar devices */
};

#ifndef KERNEL
struct	pixrect *pr_makefromfd();
#endif !KERNEL

#ifdef	cplus
struct	pixrect *pr_makefromfd(int fd, struct pr_size size, int depth,
	    struct pr_devdata **devdata, struct pr_devdata **curdd,
	    int mmapbytes, privdatabytes, mmapoffsetbytes);
void	pr_unmakefromfd(int fd, struct pr_devdata **devdata);
#endif	cplus
