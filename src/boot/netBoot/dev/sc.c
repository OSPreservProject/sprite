
#ifndef lint
/* static	char sccsid[] = "@(#)sc.c 1.1 86/09/27 Copyr 1986 Sun Micro"; */
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/types.h"
#include "../h/buf.h"
#include "../dev/screg.h"
#include "../dev/sireg.h"
#include "../dev/scsi.h"
#include "../dev/saio.h"
#include "../h/sunromvec.h"
#include "../h/idprom.h"

/*
 * Low-level routines common to all devices on the SCSI bus
 */

/*
 * Interface to the routines in this module is via a second "sip"
 * structure contained in the caller's local variables.
 *
 * This "sip" must be initialized with sip->si_boottab = scdriver
 * and must then be devopen()ed before any I/O can be done.
 *
 * The device must be closed with devclose().
 */

/* How our addrs look to the SCSI DMA hardware */
#define	SCSI_DMA_ADDR(x)	(((int)x)&0x000FFFFF)

/*
 * The interfaces we export
 */
extern int nullsys();
int scopen();

struct boottab scdriver =
	{"sd",	nullsys,	nullsys,	scopen,		nullsys,
		nullsys,	"",	0};

char seqerr_msg[] = "sequence error";	/* Saves dup copy of string */

/*
 * Record an error message from scsi
 */
sc_error(msg)
	char *msg;
{
	printf("scsi: %s\n", msg);
}

#define	SCSI_MULTI_BASE	0x80000
#define	SCSI_VME_BASE	0x200000
#define	SCSI_SIZE	0x4000		/* incr to next board - OK for VME ? */
#define	SCSI_NSTD	2		/* # of standard address boards */

/*
 * Open the SCSI host adapter.
 * Note that the Multibus address and the VME address
 * are totally different, although both are controller '0'.
 * More fon goes here for the model 25.
 */
int
scopen(sip)
	register struct saioreq	*sip;
{
	register struct scsi_ha_reg *har;
	struct idprom id;
	register int base;
	enum MAPTYPES space;
	char *devalloc();

	if (idprom(IDFORM_1, &id) == IDFORM_1 &&
	    id.id_machine == IDM_SUN2_MULTI) {
		base = SCSI_MULTI_BASE;
		space = MAP_MBMEM;
	} else {
		base = SCSI_VME_BASE;
		space = MAP_VME24A16D;
	}
	if (sip->si_ctlr < SCSI_NSTD)
		sip->si_ctlr = base + sip->si_ctlr * SCSI_SIZE;
	/* now map it in */
	sip->si_devaddr = devalloc(space, sip->si_ctlr,
	    sizeof (struct scsi_ha_reg));
	if (sip->si_devaddr == 0)
		return (-1);

	har = (struct scsi_ha_reg *) sip->si_devaddr;
	har->icr = 0;
	return 0;
}


/*
 * Write a command to the SCSI bus.
 *
 * The supplied sip is the one opened by scopen().
 * DMA is done based on sip->si_ma and sip->si_cc.
 *
 * Returns -1 for error, otherwise returns the residual count not DMAed
 * (zero for success).
 *
 * FIXME, this must be accessed via a boottab vector,
 * to allow host adap to switch.
 * Must pass cdb, scb in sip somewhere...
 */
int
scdoit(cdb, scb, sip)
	register struct scsi_cdb *cdb;
	register struct scsi_scb *scb;
	register struct saioreq *sip;
{
	register char *cp;
	register int i, b;
	register struct scsi_ha_reg *har;

	har = (struct scsi_ha_reg *) sip->si_devaddr;

	/* select controller */
	har->data = 1 << sip->si_unit;		/* Set target on SCSI bus */

	i = 100000;
	for(;;) {
		if ((har->icr & ICR_BUSY) == 0) {
			break;
		}
		if (i-- <= 0) {
			sc_error("bus continuously busy");
			har->icr = ICR_RESET;
			DELAY(50);
			har->icr = 0;
			return (-1);
		}
	}
	har->icr = ICR_SELECT;
	if (sc_wait(har, ICR_BUSY) == 0) {
		/*
		 * No response to select.  Reset bus and get out.
		 */
		sc_error("no such controller on SCSI bus");
		har->icr = ICR_RESET;
		DELAY(50);
		har->icr = 0;
		return (-1);
	}
	/* pass command */
	har->icr = ICR_WORD_MODE | ICR_DMA_ENABLE;
	har->dma_addr = SCSI_DMA_ADDR(sip->si_ma);
	har->dma_count = ~sip->si_cc;
	cp = (char *) cdb;
	for (i = 0; i < sizeof (struct scsi_cdb); i++) {
		if (sc_putbyte(har, ICR_COMMAND, *cp++) == 0) {
			return (-1);
		}
	}
	/* wait for command completion */
	if (sc_wait(har, ICR_INTERRUPT_REQUEST) == 0) {
		return (-1);
	}
	/* handle odd length dma transfer, adjust dma count */
	if (har->icr & ICR_ODD_LENGTH) {
		if ((cdb->cmd == SC_REQUEST_SENSE) || (cdb->cmd == SC_READ)) {
		        cp = (char *)sip->si_ma + sip->si_cc - 1;
		        *cp = (char)har->data;
			har->dma_count = ~(~har->dma_count - 1);
		} else {
			har->dma_count = ~(~har->dma_count + 1);
		}
	}
	/* get status */
	cp = (char *) scb;
	i = 0;
	for (;;) {
		b = sc_getbyte(har, ICR_STATUS);
		if (b == -1) {
			break;
		}
		if (i < STATUS_LEN) {
			cp[i++] = b;
		}
	}
	b = sc_getbyte(har, ICR_MESSAGE_IN);
	if (b != SC_COMMAND_COMPLETE) {
		if (b >= 0) {	/* if not, sc_getbyte already printed msg */
			sc_error("invalid message");
		}
		return (-1);
	}
	return (sip->si_cc - ~har->dma_count);
}


/*
 * Wait for a condition on the scsi bus.
 */
int
sc_wait(har, cond)
	register struct scsi_ha_reg *har;
{
	register int icr, count;

	if (cond == ICR_INTERRUPT_REQUEST) {
		count = 1000000;
	} else {
		count = 10000;
	}
	while (((icr = har->icr) & cond) != cond) {
		if (--count <= 0) {
			sc_error("timeout");
			return (0);
		}
		if (icr & ICR_BUS_ERROR) {
			sc_error("bus error");
			return (0);
		}
		DELAY(5000);
	}
	return (1);
}


/*
 * Put a byte into the scsi command register.
 */
int
sc_putbyte(har, bits, c)
	register struct scsi_ha_reg *har;
{
	if (sc_wait(har, ICR_REQUEST) == 0) {
		return (0);
	}
	if ((har->icr & ICR_BITS) != bits) {
		sc_error(seqerr_msg);
		return (0);
	}
	har->cmd_stat = c;
	return (1);
}


/*
 * Get a byte from the scsi command/status register.
 * <bits> defines the bits we want to see on in the ICR.
 * If <bits> is wrong, we print a message -- unless <bits> is ICR_STATUS.
 * This hack is because scdoit keeps calling getbyte until it sees a non-
 * status byte; this is not an error in sequence as long as the next byte
 * has MESSAGE_IN tags.  FIXME.
 */
int
sc_getbyte(har, bits)
	register struct scsi_ha_reg *har;
{
	if (sc_wait(har, ICR_REQUEST) == 0) {
		return (-1);
	}
	if ((har->icr & ICR_BITS) != bits) {
		if (bits != ICR_STATUS) sc_error(seqerr_msg);	/* Hack hack */
		return (-1);
	}
	return (har->cmd_stat);
}
