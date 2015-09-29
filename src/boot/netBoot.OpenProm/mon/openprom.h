/*
 * @(#)openprom.h 1.10 90/08/22 SMI
 * Copyright (c) 1989-1990 Sun Microsystems, Inc.
 *
 *
 * This file defines the interface between the Open Boot Prom Monitor
 * and the programs that may request its services.
 * This interface, defined as a vector of pointers to useful things,
 * has been traditionally referred to as `the romvec'.
 *
 * The address of the romvec is passed as the first argument to the standalone
 * program, obviating the need for the address to be at a known fixed
 * location.
 * Typically, the stand-alone's srt0.s file (which contains the _start entry)
 * would take care of all of `this'.
 * In SPARC assembler, `this' would be:
 *
 *	.data
 *	.global _romp
 * _romp:
 *	.word 0
 *	.text
 * _start:
 *	set	_romp, %o1	! any register other than %o0 will probably do
 *	st	%o0, [%o1]	! save it away
 *	.......			! set up your stack, etc......
 */

#ifndef _mon_openprom_h
#define	_mon_openprom_h

#ifndef _TYPES_
#include <sys/types.h>
#endif _TYPES_

#ifndef OPENPROMS
#  define OPENPROMS			/* for inquisitive minds */
#endif  !OPENPROMS
#define	ROMVEC_MAGIC	0x10010407	/* hmmm */
#define	ROMVEC_VERSION 	1
#define	PLUGIN_VERSION	2
#define	ROMVEC_BLKSIZE	512

typedef u_int ihandle_t;
typedef u_int phandle_t;

extern struct sunromvec *romp;		/* as per the above discussion */

/*
 * Memory layout stuff
 */
struct memlist {
	struct memlist	*next;		/* link to next list element */
	u_int		address;	/* starting address of memory segment */
	u_int		size;		/* size of same */
};


struct sunromvec {
	u_int	v_magic;		/* magic mushroom */
	u_int	v_romvec_version;	/* Version number of "romvec" */
	u_int	v_plugin_version;	/* Plugin Architecture version */
	u_int	v_mon_id;		/* version # of monitor firmware */
	struct memlist	**v_physmemory;	/* V0: total physical memory list */
	struct memlist	**v_virtmemory;	/* V0: taken virtual memory list */
	struct memlist	**v_availmemory; /* V0: available physical memory */
	struct config_ops *v_config_ops; /* dev_info configuration access */
	/*
	 * storage device access facilities
	 */
	char	**v_bootcommand;	/* expanded with PROM defaults */
	u_int	(*v_open)(/* char *name */);
	u_int	(*v_close)(/* ihandle_t fileid */);
	/*
	 * block-oriented device access
	 */
	u_int	(*v_read_blocks)();	/* V0 */
	u_int	(*v_write_blocks)();	/* V0 */
	/*
	 * network device access
	 */
	u_int	(*v_xmit_packet)();	/* V0 */
	u_int	(*v_poll_packet)();	/* V0 */
	/*
	 * byte-oriented device access
	 */
	u_int	(*v_read_bytes)();	/* V0 */
	u_int	(*v_write_bytes)();	/* V0 */

	/*
	 * 'File' access - i.e., Tapes for byte devices, TFTP
	 * for network devices
	 */
	u_int	(*v_seek)();
	/*
	 * single character I/O
	 */
	u_char	*v_insource;		/* Current source of input */
	u_char	*v_outsink;		/* Currrent output sink */
	u_char	(*v_getchar)();		/* Get a character from input */
	void	(*v_putchar)();		/* Put a character to output sink. */
	int	(*v_mayget)();		/* Maybe get a character, or "-1". */
	int	(*v_mayput)();		/* Maybe put a character, or "-1". */
	/*
	 * Frame buffer
	 */
	void	(*v_fwritestr)();	/* write a string to framebuffer */
	/*
	 * Miscellaneous Goodies
	 */
	void	(*v_boot_me)();		/* reboot machine */
	int	(*v_printf)();		/* handles format string plus 5 args */
	void	(*v_abortent)();	/* Entry for keyboard abort. */
	int	*v_nmiclock;		/* Counts in milliseconds. */
	void	(*v_exit_to_mon)();	/* Exit from user program. */
	void	(**v_vector_cmd)();	/* Handler for the vector */
	void	(*v_interpret)();	/* interpret forth string */

#ifdef SAIO_COMPAT
	struct bootparam **v_bootparam;	/*
					 * V0: boot parameters and `old' style
					 * device access
					 */
#else
	int	v_fill0;
#endif SAIO_COMPAT

	u_int	(*v_mac_address)(/* int fd; caddr_t buf */);
					/* Copyout ether address */
	char	**bootpath;		/* V2: Full path name of boot device */
	char	**bootargs;		/* V2: Boot cmd line after dev spec */

	ihandle_t *op_stdin;		/* V2: Console input device */
	ihandle_t *op_stdout;		/* V2: Console output device */

	phandle_t (*op_phandle)(/* ihandle_t ihandle */);
					/* V2: Convert ihandle to phandle */

	caddr_t	(*op_alloc)(/* caddr_t virthint, u_int size */);
					/* V2: Allocate physical memory */

	void	(*op_free)(/* caddr_t virt, u_int size */);
					/* V2: Deallocate physical memory */

	caddr_t	(*op_map)(/* caddr_t virthint, u_int space, u_int phys,
		    u_int size */);	/* V2: Create device mapping */

	void	(*op_unmap)(/* caddr_t virt, u_int size */);
					/* V2: Destroy device mapping */

	ihandle_t (*op_open)(/* char *name */);
	u_int	(*op_close)(/* ihandle_t fileid */);
	int	(*op_read)(/* ihandle_t fileid, caddr_t buf, u_int len */);
	int	(*op_write)(/* ihandle_t fileid, caddr_t buf, u_int len */);
	int	(*op_seek)(/* ihandle_t fileid, u_int offsh, u_int offsl */);
	void	(*op_chain)(/* caddr_t virt, u_int size, caddr_t entry,
		    caddr_t argaddr, u_int arglen */);
	void	(*op_release)(/* caddr_t virt, u_int size */);

	int	*v_reserved[15];
	/*
	 * Beginning of machine-dependent portion
	 */
#include <machine/romvec.h>
};

/*
 * XXX - This is not, strictly speaking, correct: we should never
 * depend upon "magic addresses". However, the MONSTART/MONEND
 * addresses are wired into too many standalones, and they are
 * not likely to change in any event.
 */
#define	MONSTART	(0xffd00000)
#define	MONEND		(0xfff00000)

#endif _mon_openprom_h
