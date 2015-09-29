
/*	@(#)m68vectors.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * m68vectors.h
 *
 * Defines MC68000/10/20 exception vectors and interrupt handling.
 *
 * To set an interrupt vector, you should use the set_evec routine
 * declared below.  Direct access to vectors from C code is discouraged,
 * since it's hard to take the Vector Base Reg into account.
 */

/*
 * int (*)()
 * set_evec(offset, func)
 * 	int offset;		/* Offset to vector, eg 8 for berr
 * 	int (*func)();		/* Function to call for it
 *
 * Sets the vector at  offset  to  func  and returns the previous value.
 */
int (*set_evec())();

#define	EVEC_RESET	0x000
#define	EVEC_BUSERR	0x008
#define	EVEC_ADDERR	0x00C
#define	EVEC_ILLINST	0x010
#define	EVEC_DIVZERO	0x014
#define	EVEC_CHK	0x018
#define	EVEC_TRAPV	0x01C
#define	EVEC_PRIV	0x020
#define	EVEC_TRACE	0x024
#define	EVEC_LINE1010	0x028
#define	EVEC_LINE1111	0x02C
/*			0x030 */
#define	EVEC_COPROC	0x034
#define	EVEC_FORMAT	0x038
#define	EVEC_UNINITINT	0x03C
#define	EVEC_BOOT_EXEC	0x04
#define	EVEC_MENU_TSTS	0x05
/*
 * vectors 0x40 - 0x5C are "reserved to Motorola")
 */

#define	EVEC_SPURINT	0x060
#define	EVEC_LEVEL1	0x064
#define	EVEC_LEVEL2	0x068
#define	EVEC_LEVEL3	0x06C
#define	EVEC_LEVEL4	0x070
#define	EVEC_LEVEL5	0x074
#define	EVEC_LEVEL6	0x078
#define	EVEC_LEVEL7	0x07C

#define	EVEC_TRAP0	0x080
#define	EVEC_TRAP1	0x084
#define	EVEC_TRAP2	0x088
#define	EVEC_TRAP3	0x08C
#define	EVEC_TRAP4	0x090
#define	EVEC_TRAP5	0x094
#define	EVEC_TRAP6	0x098
#define	EVEC_TRAP7	0x09C
#define	EVEC_TRAP8	0x0A0
#define	EVEC_TRAP9	0x0A4
#define	EVEC_TRAPA	0x0A8
#define	EVEC_TRAPB	0x0AC
#define	EVEC_TRAPC	0x0B0
#define	EVEC_TRAPD	0x0B4
#define	EVEC_TRAPE	0x0B8
#define	EVEC_TRAPF	0x0BC

/*
 * vectors 0xC0 - 0xFC are "reserved to Motorola"
 */

/*
 * vectors 0x100 - 0x3FC are User Interrupt Vectors for I/O
 */

#define	EVEC_LASTVEC	0x3FC
#define	EVEC_AFTER	0x400
#define	NUM_EVECS	256	/* number of exception vectors */


/*
 * Fake exception vector addresses used as "vector offset" values
 * when simulating interrupts (eg K command).
 */
#define	EVEC_KCMD	0x001	/* K1 command causing reset */
#define	EVEC_BOOTING	0x002	/* B command causing reset */
#define	EVEC_DOG	0x003	/* Watchdog reset, not power-on */
#define	EVEC_ABORT	0x07D	/* Abort from keyboard NMI */
#define	EVEC_MEMERR	0x07E	/* Memory error NMI */

/*
 * Define the information stacked by the CPU on a trap.
 *
 * (Much more info is stacked on a Bus Error or Address Error.
 *  See ../h/buserr.h for a complete description.)
 */
struct intstack {
	short	i_sr;		/* Stacked status register */
	long	i_pc;		/* Program counter of error */
	short	i_fvo;		/* Format & vector offset */
};

#define	FVO_FORMAT	0xF000	/* Stack format portion of FVO */
#define	FVO_OFFSET	0x0FFF	/* Vector offset portion of FVO */

#define	FVO_FORMAT_NORMAL	0x0000	/*  8 byte normal frame */
#define	FVO_FORMAT_THROWAWAY	0x1000	/*  8 byte throwaway frame */
#define	FVO_FORMAT_WITH_OLDPC	0x2000	/* 12 byte frame with PC of instruc */
#define	FVO_FORMAT_68010_BERR	0x8000	/* 58 byte 68010 bus error frame */
#define	FVO_FORMAT_COPROC_MID	0x9000	/* 20 byte coproc mid-instr frame */
#define	FVO_FORMAT_SHORT_BERR	0xA000	/* 32 byte 68020 bus error frame */
#define	FVO_FORMAT_LONG_BERR	0xB000	/* 0x5C byte 68020 bus error frame */
