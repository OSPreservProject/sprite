/*	@(#)bw1var.h 1.1 86/07/07 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#define	bw1_fbfrompr(pr) (((struct bw1pr *)(pr)->pr_data)->bwpr_va)
/*
 * Information pertaining to the Sun-1 frame buffer but not to pixrects in
 * general is stored in the struct pointed to by the pr_data attribute of the
 * pixrect.
 */
struct	bw1pr {
	struct	bw1fb *bwpr_va;
	int	bwpr_fd;
	int	bwpr_flags;
	int	bwpr_mask;		/* unused bit plane mask reg */
	struct	pr_pos bwpr_offset;
};

#define bw1_d(pr) ((struct bw1pr *)(pr)->pr_data)

/*
 * One property of the frame buffer not shared with all pixrects is that
 * it may be operated in either "normal" video (black background) or reverse
 * video (white background).  "Normal" is with respect to the frame buffer
 * hardware, to which a 1 means white.  Most applications will use reverse
 * video.  Reverse video is indicated by setting the BW_REVERSEVIDEO flag
 * in bwpr_flags.
 */
#define	BW_REVERSEVIDEO	1		/* MUST BE CONSTANT 1; see getput */

extern	struct pixrectops bw1_ops;

int	bw1_rop();
#ifndef KERNEL
int	bw1_stencil();
int	bw1_batchrop();
struct	pixrect *bw1_make();
int	bw1_destroy();
int	bw1_get();
int	bw1_put();
int	bw1_vector();
struct	pixrect *bw1_region();
#endif
int	bw1_putcolormap();
int	bw1_putattributes();
#ifndef KERNEL
int	bw1_getcolormap();
int	bw1_getattributes();
#endif
