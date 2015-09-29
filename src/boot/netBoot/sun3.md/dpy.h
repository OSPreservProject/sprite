
/*	@(#)dpy.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* dpy header file */

/* shades */
#define	CLEAR		0x0
#define	GRAY		0xaaaa
#define	FILLED		0xffff

/* character dimensions (Gallant font) */
#define	CHRHEIGHT	22
#define	CHRWIDTH	12
#define	CHRSHORTS	22	/* including top & bottom blank lines */
#define	CHRBYTES	(CHRSHORTS*2)

/* Terminal emulator screen size in characters */
#define	TOP		0
#ifdef SIRIUS
#define BOTTOM          (gp->g_bottom)
#define	RIGHT		(gp->g_right)
#else
#define	BOTTOM		34
#define RIGHT           80
#endif SIRIUS
#define	LEFT		0

/*
 * Sun-2 frame buffer constants
 */

/* RasterOp functions and negator */
#define	POX_SRC		PIX_SRC
#define	POX_DST		PIX_DST
#define	POX_CLR		PIX_CLR
#define	POX_SET		PIX_SET
#define	POX_NOT(x)	PIX_NOT(x)

/*
 * Support both frame buffers, use variables for this stuff.
 */
#define SCRWIDTH	(gp->g_scrwidth)
#define	SCRHEIGHT	(gp->g_scrheight)
#define	WINTOP		(gp->g_wintop)
#define WINLEFT		(gp->g_winleft)

#ifdef SIRIUS
#define WINBOT		(gp->g_winbot)
#endif SIRIUS

/* Size of bit map used to store compressed font */
/* CHRHEIGHT pixel rows, except top and bottom (-2), times # chars in font 
   (96) divided by bits per byte (8).					    */
#define	FBITMAPSIZE	((CHRHEIGHT-2)*96/8)

/* cursor states */
#define	NOCURSOR	0	/* No cursor is visible */
#define	BLOCKCURSOR	1	/* Cursor is an inverting block */

/* terminal states */
#define	ALPHA		0	/* In normal state -- display chars */
#define	SKIPPING	1	/* In CSI sequence to be skipped */
#define	ESCBRKT		2	/* In CSI sequence */
#define	GRAPH		3	/* In graphics mode */
#define	PNT		4	/* In point-plotting mode */
#define	INC		5	/* In incremental plot mode */
#define	ESC		0x80	/* OR-ed in if an ESC char seen */

/* Bresenham code constants - keep up to date with bres.s */
#define	BRESIZ		10	/* number of shorts in bres code */
#define	BRESMINOR	0	/* location of minor command in bres code */
#define	BRESMAJOR	1	/* location of major command in bres code */

/* Easy access to dpy's variables in globram */
#define	fbpr		(gp->g_fbpr)
#define	fbpos		(gp->g_fbpos)
#define	fbdata		(gp->g_fbdata)
#define	charpr		(gp->g_charpr)
#define	charpos		(gp->g_charpos)
#define	chardata	(gp->g_chardata)
#define	GXBase		((int)(gp->g_fbdata.md_image))
#define	fbaddr		((int)(gp->g_fbdata.md_image))
#define	font		(gp->g_font)
#define	ax		(gp->g_ax)
#define	ay		(gp->g_ay)
#define	gx		(gp->g_gx)
#define	gy		(gp->g_gy)
#define	gxhi		(gp->g_gxhi)
#define	gxlo		(gp->g_gxlo)
#define	gyhi		(gp->g_gyhi)
#define	gylo		(gp->g_gylo)
#define	escape		(gp->g_escape)
#define	chrfunc		(gp->g_chrfunc)
#define	vectfunc	(gp->g_vectfunc)
#define	fillfunc	(gp->g_fillfunc)
#define	state		(gp->g_state)
#define	pendown		(gp->g_pendown)
#define	cursor		(gp->g_cursor)
#define	yloseen		(gp->g_yloseen)
#define	ac		(gp->g_ac)
#define	ac0		(gp->g_ac0)
#define	acinit		(gp->g_acinit)
#define	bresw(n)	(gp->g_bresw[n])
#define	dcax		(gp->g_dcax)
#define	dcay		(gp->g_dcay)
#define	dcok		(gp->g_dcok)
#define	scrlins		(gp->g_scrlins)
