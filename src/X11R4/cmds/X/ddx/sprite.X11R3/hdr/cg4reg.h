/* @(#)cg4reg.h 1.7 88/06/09 SMI */

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

#ifndef	cg4reg_DEFINED
#define	cg4reg_DEFINED

/*
 * Prism frame buffer hardware definitions.
 *
 * The cg4 device has an overlay plane, enable plane and eight planes
 * of color/grayscale pixel read/write memory.  There are no rasterop chips 
 * or planes register.  All memory is on board memory.
 *
 * There are two flavors of cg4.  The A type has AMD DACS in the I/O space
 * and a status register.  The B type has a Brooktree BT458 DAC in the
 * memory space.
 */

/* number of colormap entries */
#define CG4_CMAP_ENTRIES	256

#ifdef sun3

/* number of known cg4 types */
#define	CG4_NTYPES		2

/* A type (AMD DAC) definitions */
#define	CG4A			0

/* frame buffer and color map addresses */
#define CG4A_ADDR_OVERLAY	0xFF000000
#define CG4A_ADDR_ENABLE	0xFE400000
#define CG4A_ADDR_COLOR		0xFE800000
#define CG4A_ADDR_CMAP		0x000E0000

/* frame buffer and color map address spaces */
#define CG4A_PGT_FB		PGT_OBMEM
#define CG4A_PGT_CMAP		PGT_OBIO

/* colormap/status register structure */
struct cg4a_cmap {
	u_char red[CG4_CMAP_ENTRIES];
	u_char green[CG4_CMAP_ENTRIES];
	u_char blue[CG4_CMAP_ENTRIES];
	u_char status;
#define CG4A_STATUS_FIRSTHALF	0x80
#define CG4A_STATUS_TOOLATE	0x40
};

/* B type (Brooktree DAC) definitions */

/* frame buffer and color map addresses */
#define CG4B_ADDR_OVERLAY	0xFF400000
#define CG4B_ADDR_ENABLE	0xFF600000
#define CG4B_ADDR_COLOR		0xFF800000
#define CG4B_ADDR_CMAP		0xFF200000

/* frame buffer and color map address spaces */
#define CG4B_PGT_FB		PGT_OBMEM
#define CG4B_PGT_CMAP		PGT_OBMEM

#endif sun3
#ifdef sun4

/* number of known cg4 types */
#define	CG4_NTYPES		1

/* B type (Brooktree DAC) definitions */

/* frame buffer and color map addresses */
#define CG4B_ADDR_OVERLAY	0xFB400000
#define CG4B_ADDR_ENABLE	0xFB600000
#define CG4B_ADDR_COLOR		0xFB800000
#define CG4B_ADDR_CMAP		0xFB200000

/* frame buffer and color map address spaces */
#define CG4B_PGT_FB		PGT_OBIO
#define CG4B_PGT_CMAP		PGT_OBIO

#endif sun4
#ifdef sun3x

/* number of known cg4 types */
#define	CG4_NTYPES		1

/* B type (Brooktree DAC) definitions */

/* frame buffer and color map addresses */
#define CG4B_ADDR_OVERLAY	0x50400000
#define CG4B_ADDR_ENABLE	0x50600000
#define CG4B_ADDR_COLOR		0x50800000
#define CG4B_ADDR_CMAP		0x50200000

/* frame buffer and color map address spaces -- not used on sun3x */
#define CG4B_PGT_FB		0
#define CG4B_PGT_CMAP		0

#endif sun3x


/* colormap structure */
struct cg4b_cmap {
	u_char	addr;		/* address register */
	char	fill0[3];
	u_char	cmap;		/* color map data register */
	char	fill1[3];
	u_char	ctrl;		/* control register */
	char	fill2[3];
	u_char	omap;		/* overlay map data register */
	char	fill3[3];
};

#endif	!cg4reg_DEFINED
