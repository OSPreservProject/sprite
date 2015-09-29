
/*	@(#)memvar.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * A memory pixrect is a special type of pixrect.  Its image resides in
 * memory, in a publicly known format permitting more direct access to the
 * image than possible with the general pixrectops.
 *
 * In the memory pixrect the image is stored in consecutive
 * memory locations, across the row from left to right, and then from top
 * row to bottom.  Row boundaries come at shortword boundaries whence
 * rows have an even number of bytes.
 *
 * The depth of a pixel is the number of bits required to represent it.
 * Pixels are placed in consecutive fields of width the depth of each pixel,
 * with placement being independent of word or byte boundaries.
 */
struct mpr_data {
	int	md_linebytes;	/* number of bytes from one line to next */
	short	*md_image;	/* word address */
	struct	pr_pos md_offset;
	short	md_primary;
	short	md_flags;	/* Flag bits, see below */
};

/*
 * Bits from md_flags:
 */
/* 				Reverse video on this pixrect.  This bit
 *				can only be on when MP_DISPLAY is on */
#define MP_REVERSEVIDEO	1
/*				This pixrect is a bw2 device, not plain mem */
#define MP_DISPLAY	2

#define mpr_d(pr)							\
	((struct mpr_data *)(pr)->pr_data)
#define MPR_LINEBITPAD	16
				/* Each line (row) of the pixrect is padded
				 *   to be a multiple of this many bits */
#define	mpr_linebytes(x, depth)						\
	( ((pr_product(x, depth) + (MPR_LINEBITPAD-1)) >> 3) &~ 1)
#define mpr_prlinebytes(mpr)						\
	mpr_linebytes((mpr)->pr_size.x, (mpr)->pr_depth)
#define	mpr_mdlinebytes(mpr)						\
	(mpr_d(mpr)->md_linebytes)

#define mprd_addr(mprd, xo, yo)						\
	((short *)(							\
	    (int)(mprd)->md_image                                       \
	    + pr_product((mprd)->md_linebytes,(mprd)->md_offset.y+(yo)) \
	    + (((mprd)->md_offset.x+(xo)) >> 3) &~ 1))
#define mprd8_addr(mprd, xo, yo, d)					\
	 ((u_char *)(							\
	     (int)(mprd)->md_image					\
	    + pr_product((mprd)->md_linebytes,(mprd)->md_offset.y+(yo)) \
	    + (pr_product((mprd)->md_offset.x+(xo), (d)) >> 3) )  )

#define	mprd_skew(mprd, xo, yo)						\
	(((mprd)->md_offset.x + (xo)) & 15)
#define	mprs_addr(mprs)		_mprs_addr((struct pr_prpos *)&(mprs))
#define	mprs8_addr(mprs)	_mprs8_addr((struct pr_prpos *)&(mprs))
#define	mprs_skew(mprs)		_mprs_skew((struct pr_prpos *)&(mprs))
short	*_mprs_addr();
u_char	*_mprs8_addr();
int	_mprs_skew();

/*
 * Static pixrects.  A pixrect may be created at compile time using the
 * mpr_static macro as part of the static declarations of a program.  Thus
 * mpr_static(cursor, 16, 16, 1, rawcursordata);
 * will declare and initialize (using rawcursordata) the storage needed
 * for a pixrect that may be referred to as 'cursor' subsequently in the
 * same file, or as &cursor if a pointer to that pixrect is called for rather
 * than the pixrect itself.
 */

/* First a pair of utility macros that allow concatenation in a fashion that
 * won't annoy lint (These belong in a standard header file!):
 */

#define IDENT(x)	x
#define	CAT(a,b)	IDENT(a)b

#define mpr_static(name, w, h, d, image)				\
	struct	mpr_data CAT(name,_data) =				\
	    {mpr_linebytes(w,d), (short *)(image), {0, 0}, 0, 0};	\
	extern struct pixrectops mem_ops;				\
	struct	pixrect name =						\
	    {&mem_ops, w, h, d, (caddr_t)&CAT(name,_data)}

/*
 * During rop calls need to determine if dst/src is something that
 * mem_rop() can handle.  Use the following macro to find out.
 */
#define	MP_NOTMPR(pr)	((pr)->pr_ops->pro_rop != mem_rop)

int	mem_rop();
#ifndef KERNEL
int	mem_stencil();
int	mem_batchrop();
struct	pixrect *mem_create();		/* General mpr create routine */
struct	pixrect *mem_point();		/* Even more general mpr create */
int	mem_destroy();
int	mem_get();
int	mem_put();
int	mem_vector();
struct	pixrect *mem_region();
#endif KERNEL
int	mem_putcolormap();
int	mem_putattributes();
#ifndef KERNEL
int	mem_getcolormap();
int	mem_getattributes();
#endif KERNEL

#define	MEM_CODESIZE		50
#define	MEM_CODESIZETOUNROLL	20
extern int	(*mem_lrcode())(), (*mem_rlcode())();
