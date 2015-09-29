
/*	@(#)dkio.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Structures and definitions for disk io control commands
 */

/* Disk identification */
struct dk_info {
	int	dki_ctlr;		/* controller address */
	short	dki_unit;		/* unit (slave) address */
	short	dki_ctype;		/* controller type */
	short	dki_flags;		/* flags */
};
/* controller types */
#define	DKC_UNKNOWN	0
#define	DKC_SMD2180	1
#define	DKC_WDC2880	2
#define	DKC_SMD2181	3
#define	DKC_XY440	4
#define	DKC_DSD5215	5
#define	DKC_XY450	6
#define	DKC_SCSI	7

/* flags */
#define	DKI_BAD144	0x01	/* use DEC std 144 bad sector fwding */
#define	DKI_MAPTRK	0x02	/* controller does track mapping */
#define	DKI_FMTTRK	0x04	/* formats only full track at a time */
#define	DKI_FMTVOL	0x08	/* formats only full volume at a time */

/* Definition of a disk's geometry */
struct dk_geom {
	unsigned short	dkg_ncyl;	/* # of data cylinders */
	unsigned short	dkg_acyl;	/* # of alternate cylinders */
	unsigned short	dkg_bcyl;	/* cyl offset (for fixed head area) */
	unsigned short	dkg_nhead;	/* # of heads */
	unsigned short	dkg_bhead;	/* head offset (for Larks, etc.) */
	unsigned short	dkg_nsect;	/* # of sectors per track */
	unsigned short	dkg_intrlv;	/* interleave factor */
	unsigned short	dkg_gap1;	/* gap 1 size */
	unsigned short	dkg_gap2;	/* gap 2 size */
	unsigned short	dkg_extra[10];	/* for compatible expansion */
};

/* disk io control commands */
#define	DKIOCGGEOM	_IOR(d, 2, struct dk_geom)	/* Get geometry */
#define	DKIOCSGEOM	_IOW(d, 3, struct dk_geom)	/* Set geometry */
#define	DKIOCGPART	_IOR(d, 4, struct dk_map)	/* Get partition info */
#define	DKIOCSPART	_IOW(d, 5, struct dk_map)	/* Set partition info */
#define	DKIOCINFO	_IOR(d, 8, struct dk_info)	/* Get info */
