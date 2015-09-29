
/*	@(#)machdep.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *	various handy constants for mapping and self-abuse 
 */
#define ADDRSIZE	(0x100000000)	/* 32-bit addresses */
#define ADDRMASK	(ADDRSIZE - 1)
#define SEGSIZE		(0x20000)	/* 128k segments*/
#define SEGMASK		(SEGSIZE - 1)
#define SEGSHIFT	(17)
#define PAGESIZE	(0x2000)	/* 8k pages */
#define PAGEMASK	(PAGESIZE -1)
#define PAGESHIFT	(13)
/*
 *	handy macros
 */
#define BTOP(x)		((u_long)(x)>>PAGESHIFT)	/* byte to page */
#define PTOB(x)		((u_long)(x)<<PAGESHIFT)	/* page to byte */
#define GETPHYS(x)	((PTOB(getpgreg(x))& ADDRMASK)+((u_long)(x)&PAGEMASK))
#define ISVALID(x)	((getpgreg(x) & (u_long)0x80000000) == 0x80000000)
/*
 *	for parity(gen,check)  mnemonics
 */
#define PAR_GEN		(1)	/* like in parity(PAR_GEN or ! PAR_GEN */
#define PAR_CHECK	(1)	/* ditto */
/*
 *	offsets into fc3 accesses
 */
#define ID_OFF		(0x0)		/* id prom */
#define PG_OFF		(0x10000000)	/* page map offset in fc3 space */
#define SM_OFF		(0x20000000)	/* segment map */
#define CX_OFF		(0x30000000)	/* context registers */
#define ENABLE_OFF	(0x40000000)	/* system enable register */
#define UDVMA_OFF	(0x50000000)	/* user dvma enable */
#define BERR_OFF	(0x60000000)	/* buserror register */
#define LED_OFF		(0x70000000)	/* led (diagnostic register) */
/*
 *	physical addresses of devices
 */
#define pa_kbm		(0x00000000)	/* keyboard mouse scc */
#define pa_scc		(0x00020000)	/* scc */
#define pa_eeprom	(0x00040000)	/* eeprom */
#define pa_clock	(0x00060000)
#define pa_parity	(0x00080000)
#define pa_interrupt	(0x000a0000)
#define pa_ethernet	(0x000c0000)
#define pa_color	(0x000e0000)
#define pa_eprom	(0x00100000)
/*
 *	assembly setups (for internal use)
 *		users should use get/set/fc3()
 */
#define	MOVC(from, to)	asm("	movc	from, to")
#define	MOVL(from, to)	asm("	movl	from, to")
#define MOVSB(from, to)	asm("	movsb	from, to")
#define MOVSW(from, to)	asm("	movsw	from, to")
#define MOVSL(from, to)	asm("	movsl	from, to")
/*
 *	permissions available
 */
#define PMP_ALL		(3)	/* real loose - valid, write, supervisor */

/*
 *	memory spaces available
 */
enum pm_type {		/* possible address spaces on board */
	PM_MEM		= 0,		/* on-board memory */
	PM_IO		= 1,		/* on-board I/O */
	PM_16VME	= 2,		/* 16bit bus memory (MB or VME) */
	PM_32VME	= 3		/* 32bit bus memory (MB or VME) */
};
/*
 *		These are the layouts of various MMU registers in
 *	both whole entry layout, and bitfield definitions.  To use,
 *	pass and return ??_reg.??_whole, and to play with afterwards,
 *	use ??_reg.??_field.??_whatever.
 *		The machdep.c routines are defined with ??_size
 *	usage (in and ??out)
 *
 *			WARNING WARNING
 *
 *	 DO NOT ATTEMPT TO PASS OR RETURN THE STRUCTS, the compiler
 *	has bugs on things which are not long aligned.
 */
typedef u_char	cx_size;
typedef u_char	sm_size;
typedef u_long	pg_size;

union cx_reg {
	cx_size	cx_whole;		/* supv and user contexts */
	struct cx_field {
		unsigned		:4;
		unsigned		:1;
		unsigned	context	:3;	/* context [0..7] */
	} cx_field;
};

union sm_reg {
	sm_size	sm_whole;		/* segment entry */
	struct sm_field {
		u_char		sm_pmeg;	/* pmeg group */
	} sm_field;
};

union pg_reg {
	pg_size	pg_whole;		/* page map entry */
	struct pg_field{
		unsigned	pg_valid:1;		/* valid bit */
		unsigned	pg_permission:2;	/* write/sup */
		unsigned	pg_cache:1;		/* cache */
		enum pm_type	pg_space:2;		/* address space */
		unsigned	pg_accessed:1;		/* page accessed?? */
		unsigned	pg_modified:1;		/* page modified?? */
		unsigned	:5;
		unsigned	pg_pagenum:19;		/* page number */
	} pg_field;
};
/*
 *	some handy typedefs
 */
typedef union cx_reg	cx_t;
typedef union sm_reg	sm_t;
typedef union pg_reg	pg_t;
/*
 *	the similar definitions of things in fc3 space that are not
 *	MMU related, same caveats apply as above
 */
typedef u_char	id_size, led_size;
typedef u_char	berr_size, enable_size, interrupt_size, parity_size;

union id_reg {
	id_size	id_whole;	/* id prom */
	struct id_field {
		u_char	id_byte;
	} id_field;
};
/*
 * the setledreg routine will invert the given led pattern,
 * because 0=lit, so passed lit=1
 */
union led_reg {
	led_size	led_whole;	/* led (diagnostic) register */
	struct led_field {
		unsigned	led_d7	:1;	/* individual bits inverted */
		unsigned	led_d6	:1;
		unsigned	led_d5	:1;
		unsigned	led_d4	:1;
		unsigned	led_d3	:1;
		unsigned	led_d2	:1;
		unsigned	led_d1	:1;
		unsigned	led_d0	:1;
	} led_field;
};

union berr_reg {
	berr_size	berr_whole;	/* bus error register */
	struct berr_field {
		unsigned	berr_invalidpg	:1;
		unsigned	berr_proterr	:1;	/* protection error */
		unsigned	berr_timeout	:1;	/* timeout error */
		unsigned	berr_vmebuserr	:1;	/* p1 bus master */
		unsigned			:4;	/* reserved */
	} berr_field;
};

union enable_reg {
	enable_size	enable_whole;	/* system enable register */
	struct enable_field {
		unsigned	enable_diag	:1;	/* diag switch */
		unsigned			:1;	/* reserved */
		unsigned	enable_vcopy	:1;	/* copy mode video fb */
		unsigned	enable_video	:1;	/* video display */
		unsigned	enable_cache	:1;	/* external cache */
		unsigned	enable_dvma	:1;	/* system dvma */
		unsigned	enable_fpp	:1;	/* floating point */
		unsigned	enable_normal	:1;	/* 0 = bootstate */
	} enable_field;
};

union interrupt_reg {
	interrupt_size	interrupt_whole;	/* interrupt register */
	struct interrupt_field {
		unsigned	enable_todint7	:1;	/* int 7 (nmi,tod) */
		unsigned			:1;	/* reserved */
		unsigned	enable_todint5	:1;	/* int 5 (tod) */
		unsigned	enable_vidint4	:1;	/* int 4 video */
		unsigned	enable_swint3	:1;	/* int 3 sw */
		unsigned	enable_swint2	:1;	/* int 2 sw */
		unsigned	enable_swint1	:1;	/* int 1 sw */
		unsigned	enable_ints	:1;	/* enable all ints */
	} interrupt_field;
};

union parity_reg {
	parity_size	parity_whole;	/* parity register */
	struct parity_field {
		unsigned	par_int		:1;	/* */
		unsigned	par_int_ena	:1;	/* */
		unsigned	par_gen		:1;	/* */
		unsigned	par_check	:1;	/* */
		unsigned	parerr24	:1;	/* */
		unsigned	parerr16	:1;	/* */
		unsigned	parerr08	:1;	/* */
		unsigned	parerr00	:1;	/* */
	} parity_field;
};

/*
 *	more less handy typedefs
 */

typedef	union id_reg		id_t;
typedef	union led_reg		led_t;
typedef	union berr_reg		berr_t;
typedef	union enable_reg	enable_t;

/*
 *	definition of the exception vector table
 *	accessed with ex_vector->?????
 */
struct mc68k_vector {
	char	*e_initsp;	/* 0  initial stack pointer */
	int	(*e_initpc)();	/* 1  initial program counter */
	int	(*e_buserr)();	/* 2  buserror */
	int	(*e_addrerr)();	/* 3  address error */
	int	(*e_illinst)();	/* 4  illegal instruction */
	int	(*e_zerodiv)();	/* 5  divide by zero */
	int	(*e_chk)();	/* 6  CHK instruction */
	int	(*e_trapv)();	/* 7  TRAPV instruction */
	int	(*e_priv)();	/* 8  privilege violation */
	int	(*e_trace)();	/* 9  trace */
	int	(*e_line10)();	/* 10 line 1010 emulator */
	int	(*e_line15)();	/* 11 line 1111 emulator */
	int	(*e_res0[2])();	/* 12-13 reserved by Motorola */
	int	(*e_format)();	/* 14 format error */
	int	(*e_uninit)();	/* 15 uninitialized interrupt vector */
	int	(*e_res1[8])();	/* 16-23 reserved blah blah */
	int	(*e_int[8])();	/* 24 is spurious, 25-31 are auto level 1-7*/
	int	(*e_trap[16])();	/* 32-47 are trap#n vectors */
	int	(*e_res2[16])();	/* 48-63 are reserved */
	int	(*e_user[192])();	/* 64-255 are user int vectors */
};
#define spurious	e_int[0]
#define	ex_vector	((struct mc68k_vector *)0)
