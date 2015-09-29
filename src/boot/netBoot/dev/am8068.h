
/*	@(#)am8068.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Definitions of the AM Z8068 Data Ciphering Processor chip.
 *
 * This file is used for both standalone code (ROM Monitor, 
 * Diagnostics, boot programs, etc) and for the Unix kernel.  IF YOU HAVE
 * TO CHANGE IT to fit your application, MOVE THE CHANGE BACK TO THE PUBLIC
 * COPY, and make sure the change is upward-compatible.  The last thing we 
 * need is seventeen different copies of this file, like we have with the
 * Sun-1 header files.
 *
 */

/*
 * These are the low-order address bits that define	what register is being
 * accessed by the master port of the DES.
 */
#define	DESR_IO		0	/* Register selector for Input & Output regs */
#define	DESR_CMD_STAT	2	/* Register selector for Command & Status */
#define	DESR_MODE	6	/* Register selector for Mode (R/W) reg */

/*
 * These commands can be written to the Command register of the DES.
 * Some take 8-byte parameters, which must then be written 
 * to the Input register or read from the Output register.
 *
 * Only a subset of the available commands are defined here; if you need
 * something else, see the data sheet and edit it in here.
 */
#define	DESC_RESET	0x00	/* Reset chip */
#define	DESC_STOP	0xE0	/* Stop en/decrypting */
#define	DESC_START	0xC0	/* Start en/decrpyting depending on mode reg */
#define	DESC_START_DEC	0x40	/* Start decrypting, force mode to decrypt */
#define	DESC_START_ENC	0x41	/* Start encrypting, force mode to encrypt */
#define	DESC_LOAD_E_KEY	0x11	/* Load clear encryption key */
#define	DESC_LOAD_D_KEY	0x12	/* Load clear decrpytion key */

/*
 * Status bit masks
 */
#define	DESS_MST_FLAG	0x01	/* Master port busy (MFLG pin) */
#define	DESS_SLAVE_FLAG	0x02	/* Slave port busy (SFLG pin) */
#define	DESS_AUX_FLAG	0x04	/* Aux port busy (AFLG pin) */
#define	DESS_PAR	0x08	/* Parity of last key byte was bad (PAR pin) */
#define	DESS_LPAR	0x10	/* Latched parity-bad bit. */
#define	DESS_BUSY	0x20	/* Chip is enciphering or deciphering things */
#define	DESS_CMD_PEND	0x40	/* Chip awaits data xfer from last cmd */
#define	DESS_STARTED	0x80	/* Start command done since last Stop */

/*
 * Mode register bits
 */
/* Cipher type */
#define	DESM_ECB	0x00	/* Electronic Code Book mode */
#define	DESM_CFB	0x01	/* Cipher Feedback mode */
#define	DESM_CBC	0x02	/* Cipher Block Chain mode */
/* Port configuration */
#define	DESM_ME_SC	0x00	/* Master encrypted, Slave clear */
#define	DESM_MC_SE	0x04	/* Master clear, Slave encrypted */
#define	DESM_M_ONLY	0x08	/* Master port only */
/* Encrypt/Decrypt */
#define	DESM_DECRYPT	0x00	/* Decrypt, please. */
#define	DESM_ENCRYPT	0x10	/* Encrypt, please. */


/*
 * The 8068 on the Sun-2 processor is connected via a kludge interface.
 * The chip expects a multiplexed bus, which we do not have, since we run
 * 68000's.  It latches address information from the data bus with one
 * strobe, then later reads or writes data with another strobe.  We must
 * fake this in software since our hardware does not multiplex the data
 * bus to this chip.  Thus, to touch a register in the DES we must first
 * write a byte containing the address bits to one 68000 address, then
 * read or write the actual data to another 68000 address.  Sort of like
 * all the stupid UARTs in the world.  Like the UARTs, we can't read back
 * the pointer value -- it's invisible state information.
 *
 * Example:  To write a command to the chip:
 * 	struct deschip *dp;
 *	dp->d_selector = DESR_CMD_STAT;	/* Determine which reg to touch * /
 *	dp->d_reg = DESC_RESET;		/* Write to command register * /
 * The selector need not be reloaded for each access if its value would be
 * the same (eg, accessing the same register or the Read instead of Write
 * register of the pair).
 *
 * 6 DES clock cycles (2400ns) must pass after each write to the
 * mode register or reset of the chip.  There is no timing requirement
 * on other accesses, though.
 */
struct deschip {
	unsigned char	d_reg;			/* value -- read or write */
	unsigned char			:8;
	unsigned char	d_selector;		/* Address of reg -- wr only */
	unsigned char			:8;
};

