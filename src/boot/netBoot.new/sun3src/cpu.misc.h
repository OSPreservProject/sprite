
/*	@(#)cpu.misc.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Miscellaneous information about the Sun-3.
 *
 * This file is used for both standalone code (ROM Monitor, 
 * Diagnostics, boot programs, etc) and for the Unix kernel.  IF YOU HAVE
 * TO CHANGE IT to fit your application, MOVE THE CHANGE BACK TO THE PUBLIC
 * COPY, and make sure the change is upward-compatible.  The last thing we 
 * need is seventeen different copies of this file, like we have with the
 * Sun-1 header files.
 */


/*
 * The on-board ZSCC (Serial Communications Controller) chips are fed with
 * a PCLK (processor clock) value of 4.9152MHz or about 204ns.  (This must
 * be known when calculating time constants for the baud rate generator.)
 */
#define	ZSCC_PCLK	4915200


/*
 * The Sun-3 architecture uses 68020's which bring out 32 address lines.
 */
#define	ADDR_LEN	8	/* Hex characters for printing addresses */
#define BYTE_LEN	2	/* Hex characters for printing bytes */
#define	WORD_LEN	4	/* Hex characters for printing words */
#define	LONG_LEN	8	/* Hex characters for printing longwords */
#define	EEPROM_SIZE	2048	/* EEPROM size = 2 K bytes */
#define EEPROM_LEN	3	/* Hex characters for EEPROM addresses */
