
/*      @(#)eeprom.h 1.1 86/09/27 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This structure defines the contents to the EEPROM for the Sun-3.
 * It is divided into a diagnostic section, a reserved section,
 * a ROM section, and a software section (defined in detail
 * elsewhere).
 */
#ifndef _EPROM_
#define _EPROM_

struct	eeprom {
	struct	ee_diag {		/* diagnostic's area of EEPROM */
/* 0x000 */	u_int	eed_test;	/* diagnostic test write area */
/* 0x004 */	u_short	eed_wrcnt[3];	/* diag area write count (3 copies) */
		short	eed_nu1;	/* not used */
/* 0x00c */	u_char	eed_chksum[3];	/* diag area checksum (3 copies) */
		char	eed_nu2;	/* not used */
/* 0x010 */	time_t	eed_hwupdate;	/* date of last hardware update */
/* 0x014 */	char	eed_memsize;	/* MB's of memory in system */
/* 0x015 */	char	eed_memtest;	/* MB's of memory to test on powerup */
/* 0x016 */	char	eed_scrsize;	/* screen size, in pixels */
#define	EED_SCR_1152X900	0x00
#define	EED_SCR_1024X1024	0x12
#define EED_SCR_1600X1280       0x13    /* new for hi rez */
#define EED_SCR_1440X1440       0x14
/* 0x017 */	char	eed_dogaction;	/* action to take on watchdog reset */
#define	EED_DOG_MONITOR		0x00	/* return to monitor command level */
#define	EED_DOG_REBOOT		0x12	/* perform power on reset and reboot */
/* 0x018 */	char	eed_defboot;	/* default boot? */
#define	EED_DEFBOOT		0x00	/* do default boot */
#define	EED_NODEFBOOT		0x12	/* don't do default boot */
/* 0x019 */	char	eed_bootdev[2];	/* name of boot device (e.g. xy, ie) */
/* 0x01b */	char	eed_bootctrl;	/* controller number to boot from */
/* 0x01c */	char	eed_bootunit;	/* unit number to boot from */
/* 0x01d */	char	eed_bootpart;	/* partition number to boot from */
/* 0x01e */	char	eed_kbdtype;	/* non-Sun keyboard type - for OEM's */
#define	EED_KBD_SUN	0		/* one of the Sun keyboards */
/* 0x01f */	char	eed_console;	/* device to use for console */
#define	EED_CONS_BW	0x00		/* use b&w monitor for console */
#define	EED_CONS_TTYA	0x10		/* use tty A port for console */
#define	EED_CONS_TTYB	0x11		/* use tty B port for console */
#define	EED_CONS_COLOR	0x12		/* use color monitor for console */
/* 0x020 */	char	eed_showlogo;	/* display Sun logo? */
#define	EED_LOGO	0x00
#define	EED_NOLOGO	0x12
/* 0x021 */	char	eed_keyclick;	/* keyboard click? */
#define	EED_NOKEYCLICK	0x00
#define	EED_KEYCLICK	0x12
/* 0x022 */	char    eed_diagdev[2]; /* name of boot device (e.g. xy, ie) */
/* 0x024 */	char    eed_diagctrl;   /* controller number to boot from */
/* 0x025 */	char    eed_diagunit;   /* unit number to boot from */
/* 0x026 */	char    eed_diagpart;   /* partition number to boot from */
/* 0x027 */	char    eed_diagnu;     /* not used */
/* 0x028 */	char    eed_diagpath[40]; /* boot path of diagnostic */
/* 0x050 */     char    eed_colsize;   /* number of columns */
/* 0x051 */     char    eed_rowsize;   /* number of rows */
/* 0x052 */	char	eed_nu5[6];	/* not used */
/* 0x058 */	struct	eed_tty_def {	/* tty port defaults */
			char	eet_sel_baud;	/* user specifies baud rate */
#define	EET_DEFBAUD	0x00
#define	EET_SELBAUD	0x12
			u_char	eet_hi_baud;	/* upper byte of baud rate */
			u_char	eet_lo_baud;	/* lower byte of baud rate */
			u_char	eet_rtsdtr;	/* flag for dtr and rts */
#define NO_RTSDTR	0x12
			char	eet_unused[4];
		} eed_ttya_def, eed_ttyb_def;
/* 0x068 */	char	eed_banner[80];	/* banner if not displaying Sun logo */
					/* last two chars must be \r\n (XXX - why not \0?) */
/* 0x0b8 */	u_short	eed_pattern;	/* test pattern - must contain 0xAA55 */
/* 0x0ba */   	short   eed_nu6;        /* not used */
/* 0x0bc */	struct	eed_conf {	/* system configuration, by slot */
			union {
			struct	eec_gen {
				char	eec_type;	/* type of this board */
				char	eec_size[7];	/* size of each entry */
			} eec_gen;

#define	EEC_TYPE_CPU	0x01			/* cpu */
			struct	eec_cpu {
				char	eec_type;	/* type of this board */
				char	eec_cpu_memsize;	/* MB's on cpu */
				int	eec_cpu_unused:6;
				int	eec_cpu_dcp:1;		/* dcp? */
				int	eec_cpu_68881:1;	/* 68881? */
				char	eec_cpu_cachesize;	/* KB's in cache */
			} eec_cpu;

#define	EEC_TYPE_MEM	0x02			/* memory board */
			struct	eec_mem {
				char	eec_type;	/* type of this board */
				char	eec_mem_size;	/* MB's on card */
			} eec_mem;

#define	EEC_TYPE_COLOR	0x03			/* color frame buffer */
			struct	eec_color {
				char	eec_type;	/* type of this board */
				char	eec_color_type;
#define	EEC_COLOR_TYPE_CG2	2	/* cg2 color board */
#define	EEC_COLOR_TYPE_CG3	3	/* cg3 color board */
			} eec_color;

#define	EEC_TYPE_BW	0x04			/* b&w frame buffer */

#define	EEC_TYPE_FPA	0x05			/* floating point accelerator */

#define	EEC_TYPE_DISK	0x06			/* SMD disk controller */
			struct	eec_disk {
				char	eec_type;	/* type of this board */
				char	eec_disk_type;	/* controller type */
				char	eec_disk_ctlr;	/* controller number */
				char	eec_disk_disks;	/* number of disks */
			} eec_disk;

#define	EEC_TYPE_TAPE	0x07			/* 1/2" tape controller */
			struct eec_tape {
				char	eec_type;	/* type of this board */
				char	eec_tape_type;	/* controller type */
#define	EEC_TAPE_TYPE_XT	1	/* Xylogics 472 */
#define	EEC_TAPE_TYPE_MT	2	/* TapeMaster */
				char	eec_tape_ctlr;	/* controller number */
				char	eec_tape_drives;/* number of drives */
			} eec_tape;

#define	EEC_TYPE_ETHER	0x08			/* Ethernet controller */

#define	EEC_TYPE_TTY	0x09			/* terminal multiplexer */
			struct eec_tty {
				char	eec_type;	/* type of this board */
				char	eec_tty_lines;	/* number of lines */
			} eec_tty;

#define	EEC_TYPE_GPGB	0x0a			/* graphics processor/buffer */

#define	EEC_TYPE_DCP	0x0b			/* DCP ??? XXX */

#define	EEC_TYPE_SCSI	0x0c			/* SCSI controller */
			struct	eec_scsi {
				char	eec_type;	/* type of this board */
				char	eec_scsi_type;	/* controller type */
				char	eec_scsi_tapes;	/* number of tapes */
				char	eec_scsi_disks;	/* number of disks */
			} eec_scsi;

#define	EEC_TYPE_END	0xff			/* end of card cage */
			} eec_un;
		} eed_conf[13];
/* 0x124 */	char	eed_resv[0x500-0x124];	/* reserved */
	} ee_diag;

	struct	ee_resv {		/* reserved area of EEPROM */
/* 0x500 */	u_short	eev_wrcnt[3];	/* write count (3 copies) */
		short	eev_nu1;	/* not used */
/* 0x508 */	u_char	eev_chksum[3];	/* reserved area checksum (3 copies) */
		char	eev_nu2;	/* not used */
/* 0x50c */	char	eev_resv[0x600-0x50c];
	} ee_resv;

	struct	ee_rom {		/* ROM area of EEPROM */
/* 0x600 */	u_short	eer_wrcnt[3];	/* write count (3 copies) */
		short	eer_nu1;	/* not used */
/* 0x608 */	u_char	eer_chksum[3];	/* ROM area checksum (3 copies) */
		char	eer_nu2;	/* not used */
/* 0x60c */	char	eer_resv[0x700-0x60c];
	} ee_rom;

	/*
	 * The following area is reserved for software (i.e. UNIX).
	 * The actual contents of this area are defined elsewhere.
	 */
#ifndef EE_SOFT_DEFINED
	struct	ee_softresv {		/* software area of EEPROM */
/* 0x700 */	u_short	ees_wrcnt[3];	/* write count (3 copies) */
		short	ees_nu1;	/* not used */
/* 0x708 */	u_char	ees_chksum[3];	/* software area checksum (3 copies) */
		char	ees_nu2;	/* not used */
/* 0x70c */	char	ees_resv[0x800-0x70c];
	} ee_soft;
#else
	struct	ee_soft ee_soft;
#endif
};
#endif !_EPROM_
