#ifdef	M25
#ifndef lint
static	char sccsid[] = "@(#)if_le.c 1.1 86/09/27 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*****************************************************************************
 * 10-Dec-85
 * 	ADD signal user that we have lost carrier (or couldn't find it).
 *	    This usually means that the cable is bad
 *
 *****************************************************************************/

/*
 * For Sun-3/25 debugging we want DEBUG on.  For production, turn it off.
 */
/* #define DEBUG1 */
/* #define DEBUG	1 */

#define LANCEBUG 1	/* Rev C Lance chip bug */
#define PROTOSCRATCH 8192

/* 
 * Parameters controlling TIMEBOMB action: times out if chip hangs.
 *
 */
#define	TIMEBOMB	1000000	/* One million times around is all... */

/*
 * Sun Lance Ethernet Controller interface
 */
#include "../dev/saio.h"
#include "../h/param.h"
#include "../h/socket.h"
#include "../dev/if_lereg.h"
#include "../dev/if.h"
#include "../h/in.h"
#include "../dev/if_ether.h"
#include "../h/idprom.h"
#include "../sun3/cpu.map.h"

/* Determine whether we are PROM or not. */
/* #define PROM 1 */

int	lancexmit(), lancepoll(), lancereset();

struct saif leif = {
	lancexmit,
	lancepoll,
	lancereset,
};

#define	LANCERBUFSIZ	1600
#define	LANCETBUFSIZ	1600

struct lance_softc {
/* 	char		es_scrat[PROTOSCRATCH];	/* work space for nd  */
        struct le_device    	*es_lance;	/* Device register address */
	struct ether_addr	es_enaddr;	/* Our Ethernet address */
	struct le_init_block	es_ib;		/* Initialization block */
	u_char			fill[6];
	struct le_md		es_rmd[2];	/* Receive Descriptor Ring */
	struct le_md		es_tmd;		/* Transmit Descriptor Ring */
	u_char			es_rbuf[2][LANCERBUFSIZ]; /* Receive Buffers */
#ifndef PROM
	u_char			es_tbuf[LANCETBUFSIZ];	/* Transmit Buffer */
#endif	PROM
	int			es_next_rmd;	/* Next descriptor in ring */
};

/*
 * Need to initialize the Ethernet control reg to:
 *	Reset is active
 *	Loopback is NOT active
 *	Interrupt enable is not active.
 */
u_long lancestd[] = {VIOPG_AMD_ETHER << BYTES_PG_SHIFT};

struct devinfo lanceinfo = {
	sizeof (struct le_device),
	sizeof (struct lance_softc),
	0,				/* Local bytes (we use dma) */
	1,				/* Standard addr count */
	lancestd,			/* Standard addrs */
	MAP_OBIO,			/* Device type */
	0,				/* transfer size handled by ND */
};

int lanceprobe(), tftpboot(), lanceopen(), lanceclose(), etherstrategy();
int nullsys();

struct boottab ledriver = {
	"le", lanceprobe, tftpboot, lanceopen, lanceclose,
	etherstrategy, "le: Sun/Lance Ethernet", &lanceinfo,
};

extern struct ether_addr etherbroadcastaddr;

/*
 * Probe for device.
 * Must return -1 for failure for monitor probe
 */
lanceprobe(sip)
	struct saioreq *sip;
{
	register short *sp;
	struct idprom id;

	if (IDFORM_1 == idprom(IDFORM_1, &id)
	    && id.id_machine == IDM_SUN3_M25)
		return (0);
	else
		return (-1);
}

/*
 * Open Lance Ethernet nd connection, return -1 for errors.
 */
lanceopen(sip)
	struct saioreq *sip;
{
	register int result;

#ifdef DEBUG1
	printf("le: lanceopen[\n");
#endif DEBUG1
	sip->si_sif = &leif;
	if ( lanceinit(sip) || (result = etheropen(sip)) < 0 ) {
		lanceclose(sip);		/* Make sure we kill chip */
#ifdef DEBUG1
		printf("le: lanceopen --> -1\n");
#endif DEBUG1
		return (-1);
	}
#ifdef DEBUG1
	printf("le: lanceopen --> %x\n", result);
#endif DEBUG1
	return (result);
}

/*
 * Set up memory maps and Ethernet chip.
 * Returns 1 for error (after printing message), 0 for ok.
 */
int
lanceinit(sip)
	struct saioreq *sip;
{
	register struct lance_softc *es;
	int paddr;
	int i;
	struct idprom id;
	
	/* 
	 * Our locals were obtined from DMA space, now put the locals
	 * pointer in the standard place.  This is OK since close()
	 * will only deallocate from devdata if its size in devinfo is >0.
	 */
	es = (struct lance_softc *)sip->si_dmaaddr;
	sip->si_devdata = (caddr_t)es;
	es->es_lance = (struct le_device *) sip->si_devaddr;

	if (IDFORM_1 != idprom(IDFORM_1, &id)) {
		printf("le: No ID PROM\n");
		return 1;
	}

	return lancereset(es, sip);
}

/*
 * Basic Lance initialization
 * Returns 1 for error (after printing message), 0 for ok.
 */
int
lancereset(es, sip)
	register struct lance_softc *es;
	struct saioreq *sip;
{
	register struct le_device *le = es->es_lance;
	register struct le_init_block *ib = &es->es_ib;
	int timeout = TIMEBOMB;
	int i;

#ifdef DEBUG1
	printf("le: lancereset(%x, %x)\n", es, sip);
#endif DEBUG1

	/* Reset the chip */
	le->le_rap = LE_CSR0;
	le->le_csr = LE_STOP;

	/* Perform the basic initialization */
	
	/* Construct the initialization block */
	bzero((caddr_t)&es->es_ib, sizeof (struct le_init_block));

	/* Leave the mode word 0 for normal operating mode */

	myetheraddr(&es->es_enaddr);

	/* Oh, for a consistent byte ordering among processors */
	ib->ib_padr[0] = es->es_enaddr.ether_addr_octet[1];
	ib->ib_padr[1] = es->es_enaddr.ether_addr_octet[0];
	ib->ib_padr[2] = es->es_enaddr.ether_addr_octet[3];
	ib->ib_padr[3] = es->es_enaddr.ether_addr_octet[2];
	ib->ib_padr[4] = es->es_enaddr.ether_addr_octet[5];
	ib->ib_padr[5] = es->es_enaddr.ether_addr_octet[4];

	/* Leave address filter 0 -- we don't want Multicast packets */

	ib->ib_rdrp.drp_laddr = (long)&es->es_rmd[0];
	ib->ib_rdrp.drp_haddr = (long)&es->es_rmd[0] >> 16;
	ib->ib_rdrp.drp_len  = 1;   /* 2 to the 1 power = 2 */
	
	ib->ib_tdrp.drp_laddr = (long)&es->es_tmd;
	ib->ib_tdrp.drp_haddr = (long)&es->es_tmd >> 16;
	ib->ib_tdrp.drp_len  = 0;   /* 2 to the 0 power = 1 */

	/* Clear all the descriptors */
	bzero((caddr_t)es->es_rmd, 2 * sizeof (struct le_md));
	bzero((caddr_t)&es->es_tmd, sizeof (struct le_md));

	/* Give the init block to the chip */
	le->le_rap = LE_CSR1;	/* select the low address register */
	le->le_rdp = (long)ib & 0xffff;

	le->le_rap = LE_CSR2;	/* select the high address register */
	le->le_rdp = ((long)ib >> 16) & 0xff;

	le->le_rap = LE_CSR3;	/* Bus Master control register */
	le->le_rdp = LE_BSWP;

	le->le_rap = LE_CSR0;	/* main control/status register */
	le->le_csr = LE_INIT;

	while( ! (le->le_csr & LE_IDON) ) {
		if (timeout-- <= 0) {
		    printf("le: cannot initialize\n");
		    return (1);
		}
	}
	le->le_csr = LE_IDON;	/* Clear the indication */

	/* Hang out the receive buffers */
	es->es_next_rmd = 0;

	install_buf_in_rmd(es->es_rbuf[0], &es->es_rmd[0]);
	install_buf_in_rmd(es->es_rbuf[1], &es->es_rmd[1]);

	le->le_csr = LE_STRT;

#ifdef DEBUG1
	printf("le: lancereset returns OK\n");
#endif DEBUG1
	return 0;		/* It all worked! */
}

install_buf_in_rmd(buffer, rmd)
	u_char *buffer;
	register struct le_md *rmd;
{
	rmd->lmd_ladr = (u_short)buffer;
	rmd->lmd_hadr = (long)buffer >> 16;
	rmd->lmd_bcnt = -LANCERBUFSIZ;
	rmd->lmd_mcnt = 0;
	rmd->lmd_flags = LMD_OWN;
}

/*
 * Transmit a packet.
 * If possible, just points to the packet without copying it anywhere.
 */
lancexmit(es, buf, count)
	register struct lance_softc *es;
	char *buf;
	int count;
{
	register struct le_device *le = es->es_lance;
	struct le_md *tmd = &es->es_tmd; /* Transmit Msg. Descriptor */
	caddr_t tbuf;
	int timeout = TIMEBOMB;

#ifdef DEBUG2
	printf( "xmit np_blkno %x\n",
		((struct ndpack *)(buf+14))->np_blkno);
#endif DEBUG2
	/*
	 * We assume the buffer is in an area accessible by the chip.
	 * The caller of xmit is currently ndxmit(), which only sends
	 * an nd structure, which we happen to know is allocated in the
	 * right area (in fact, it's part of the struct nd which
	 * is the first thing in our own es structure).  If we wish to
	 * use xmit for some other purpose, the buffer might not be
	 * accessible by the chip, so to be general we ought to copy
	 * into some accessible place.  HOWEVER, PROM space is really tight,
	 * so this generality is not free.
	 */
#ifdef PROM
	tbuf = buf;
	if (((int)tbuf & 0x00F00000) == 0)
		(int)tbuf |= 0x00F00000;
#else  PROM
/* FIXME, constant address masks here! */
	if ( ((int)buf & 0x0F000000) == 0x0F000000) { /* we can point to it */
	    tbuf = buf;
	} else {
	    tbuf = (caddr_t)es->es_tbuf;
	    bcopy((caddr_t)buf, tbuf, count);
	}
#endif PROM
	
	tmd->lmd_hadr = (int)tbuf >> 16;
	tmd->lmd_ladr = (u_short) tbuf;
	tmd->lmd_bcnt = -count;
	
#ifdef notdef
	if (tmd->lmd_bcnt < -MINPACKET)
	    tmd->lmd_bcnt = -MINPACKET;
#endif notdef
	tmd->lmd_flags3 = 0;
	tmd->lmd_flags = LMD_STP | LMD_ENP | LMD_OWN;
	
	le->le_csr = LE_TDMD;

	do {
	    if ( le->le_csr & LE_TINT ) {
		le->le_csr = LE_TINT; /* Clear interrupt */
		break;
	    }
	} while ( --timeout > 0);

/*****************************************************************************
 * 10-Dec-85
 * 	ADD signal user that we have lost carrier (or couldn't find it).
 *	    This usually means that the cable is bad
 *
 *****************************************************************************/

	if (tmd->lmd_flags & LMD_ERR)
	    {
	    if (tmd->lmd_flags3 & TMD_LCAR)  /* (AMD Lance Transmit msg */
					     /*  descriptor - 3 (TMD3)  */
		printf("le: No Carrier\n");
            }
/*****************************************************************************/

	if ( (tmd->lmd_flags & LMD_ERR)
	||   (tmd->lmd_flags3 & TMD_BUFF)
	||   (timeout <= 0) ) {
#ifdef DEBUG
		printf("le: xmit failed - tmd1 flags %x tmd3 %x csr0 %x\n",
	       		tmd->lmd_flags, tmd->lmd_flags3, le->le_csr);
#endif DEBUG
		return (1);
	}

	return (0);
}

int
lancepoll(es, buf)
	register struct lance_softc *es;
	char *buf;
{
	register struct le_device *le = es->es_lance;
	register struct le_md *rmd;
	register struct ether_header *header;
	int length;

#ifdef DEBUG1
	printf("le: poll\n");
#endif DEBUG1
	if ( ! (le->le_csr & LE_RINT)  )
		return (0);		/* No packet yet */

#ifdef DEBUG1
	printf("le: received packet\n");
#endif DEBUG1

	le->le_csr = LE_RINT; 		/* Clear interrupt */

	rmd = &es->es_rmd[es->es_next_rmd];

	if ( (rmd->lmd_flags & ~RMD_OFLO) != (LMD_STP|LMD_ENP) ) {
#ifdef DEBUG
		printf("Receive packet error - rmd flags %x\n",rmd->lmd_flags);
#endif DEBUG
		length = 0;
		goto restorebuf;
	}

	/* Get input data length and a pointer to the ethernet header */

	length = rmd->lmd_mcnt - 4;	/* don't count the 4 CRC bytes */
	header = (struct ether_header *)es->es_rbuf[es->es_next_rmd];

#ifdef LANCEBUG
	/*
	 * Check for unreported packet errors.  Rev C of the LANCE chip
	 * has a bug which can cause "random" bytes to be prepended to
	 * the start of the packet.  The work-around is to make sure that
	 * the Ethernet destination address in the packet matches our
	 * address.
	 */
#define ether_addr_not_equal(a,b)	\
	(  ( *(long  *)(&a.ether_addr_octet[0]) != \
	     *(long  *)(&b.ether_addr_octet[0]) )  \
	|| ( *(short *)(&a.ether_addr_octet[4]) != \
	     *(short *)(&b.ether_addr_octet[4]) )  \
	)

    	if( ether_addr_not_equal(header->ether_dhost, es->es_enaddr)
	&&  ether_addr_not_equal(header->ether_dhost, etherbroadcastaddr) ) {
		printf("le: LANCE Rev C Extra Byte(s) bug; Packet punted\n");
		length = 0;
		/* Don't return directly; restore the buffer first */
	}
#endif LANCEBUG

#ifdef DEBUG2
    	if( header->ether_dhost.ether_addr_octet[0] == 0xff )
		printf("Broadcast packet\n");
	else	printf("recv np_blkno %x\n",
	        	((struct ndpack *)(buf+14))->np_blkno);
#endif DEBUG2
	
	/* Copy packet to user's buffer */
	if ( length > 0 )
		bcopy((caddr_t)header, buf, length);

restorebuf:
	rmd->lmd_mcnt = 0;
	rmd->lmd_flags = LMD_OWN;

	/* Get ready to use the other buffer next time */
	/* What about errors ? */
	es->es_next_rmd = 1 - es->es_next_rmd;

	return (length);
}

/*
 * Close down Lance Ethernet device.
 * On the Model 25, we reset the chip and take it off the wire, since
 * it is sharing main memory with us (occasionally reading and writing),
 * and most programs don't know how to deal with that -- they just assume
 * that main memory is theirs to play with.
 */
lanceclose(sip)
	struct saioreq *sip;
{
	struct lance_softc *es = (struct lance_softc *) sip->si_devdata;
	struct le_device *le = es->es_lance;

	/* Reset the chip */
	le->le_rap = LE_CSR0;
	le->le_csr = LE_STOP;
}
#endif	M25
