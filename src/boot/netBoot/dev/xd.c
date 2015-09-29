
#ifndef lint
/* static	char sccsid[] = "@(#)xd.c 1.0 86/03/12 Copyr 1986 Sun Micro";*/
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* Standalone driver for Xylogics 751 */


#ifndef M25
#include "../dev/saio.h"
#include "../h/param.h"
#include "../dev/dkbad.h"
#include "../dev/dklabel.h"
#include "../dev/dkio.h"
#include "../h/fsdir.h"
#include "../dev/xdcreg.h"
#include "../dev/xdreg.h"

extern char	msg_nolabel[];

struct xdparam {
	unsigned short	xd_boff;	/* Cyl # starting partition */
 	unsigned short	xd_nsect;	/* Sect/track */
 	unsigned short	xd_ncyl;	/* Cyl/disk */
 	unsigned short	xd_nhead;	/* Heads/cyl */
 	unsigned short	xd_bhead;	/* Base head # for removable disk */
 	unsigned short	xd_drive;	/* Xylogics drive type ID */
	int	xd_nblk;
	int	xd_badaddr;		/* Disk block # of bad-block table */
	struct dkbad xd_bad;
	unsigned char	xd_unit;
};

#define CYL(p)  (p*xdp->xd_nsect*xdp->xd_nhead)     /* cyl to block conv */

#define NSTD 	2
#define TIMEOUT	0x2ffff
unsigned long xdstd[NSTD] = { 0xee80, 0xee88 };

/*
 * Structure of our DMA area
 */
#define MAXBSIZE        8192
struct xddma {
	struct xdiopb	xdiopb;
	char		xdblock[MAXBSIZE];	/* R/W data */
};

/*
 * What resources we need to run
 */
struct devinfo xdinfo = {
	sizeof (struct xddevice), 
	sizeof (struct xddma),
	sizeof (struct xdparam),
	NSTD,
	xdstd,
	MAP_MBIO,
#ifdef BOOTBLOCK
	DEV_BSIZE,
#else
	DEV_BSIZE,	/* MAXBSIZE, */
#endif
};

/*
 * What facilities we export to the world
 */
int	xdopen(), xdstrategy();
extern int	xxboot(), xxprobe();
extern int	nullsys();

struct boottab xddriver = {
	"xd",	xxprobe, xxboot, xdopen, nullsys, xdstrategy,
	"\0", &xdinfo,
}; 


/*
 * Open a xdlogics disk.
 */
xdopen(sip)
	register struct saioreq *sip;
{
	register struct xdparam *xdp;
	register struct xddevice *xdaddr;
	register struct xdiopb *xd;
	struct dk_label *label;
	register int t;
	u_short ppart;
#ifndef BOOTBLOCK
	int xdspin();
#endif !BOOTBLOCK

	xdp = (struct xdparam *)sip->si_devdata;
	xdaddr = (struct xddevice *)sip->si_devaddr;
	xd = &((struct xddma *)sip->si_dmaaddr)->xdiopb;

	xdp->xd_unit = sip->si_unit & 0x03;
	ppart = (sip->si_unit >> 2) & 1;

	/* reset controller */
	xdaddr->xd_csr = XD_RST; 
	DELAY(300);

	xdp->xd_boff  = 0;		/* Don't offset block numbers */
	xdp->xd_nsect = 2;
	xdp->xd_nhead = 2;
	xdp->xd_ncyl = 2;
	xdp->xd_bad.bt_mbz = -1;

#ifndef BOOTBLOCK
	/*
	 * Wait for disk to spin up, if necessary.
	 */
	if (!isspinning(xdspin, (char *)sip, 0))
		return (-1);
#endif !BOOTBLOCK

	label = (struct dk_label *)
		((struct xddma *)sip->si_dmaaddr)->xdblock;
	label->dkl_magic = 0;
	if (xdcmd(XD_READ, sip, 0, (char *)label, 1)) 
		goto badopen;
	if (chklabel(label)) 
		goto badopen;
	if (ppart != label->dkl_ppart)
		goto badopen;
	goto foundlabel;

badopen:
	printf(msg_nolabel);
	return (-1);

foundlabel:
	xdp->xd_nhead = label->dkl_nhead;
	xdp->xd_nsect = label->dkl_nsect;
	xdp->xd_ncyl  = label->dkl_ncyl + label->dkl_acyl;
	xdp->xd_boff  = label->dkl_map[sip->si_boff].dkl_cylno;
	xdp->xd_nblk  = label->dkl_map[sip->si_boff].dkl_nblk;

	if (xdcmd(XD_WPAR, sip, 0, 0, 0))
		printf("xd: init error %x\n", xd->xd_errno);
	/*
	 * Fetch bad block info.
	 */
	xdp->xd_badaddr = ((int)xdp->xd_ncyl * xdp->xd_nhead - 1)
			  * xdp->xd_nsect;
	if (xdcmd(XD_READ, sip, xdp->xd_badaddr, (char *)&xdp->xd_bad, 1))
		printf("xd: no bad block info\n");
	return (0);
}

xdstrategy(sip, rw)
	struct saioreq *sip;
	int rw;
{
	register int cmd = (rw == WRITE) ? XD_WRITE : XD_READ;
	register int boff;
	register struct xdparam *xdp = (struct xdparam *)sip->si_devdata;
	register short nsect;
#ifndef BOOTBLOCK
	char *ma;
	int i, bn;
#endif BOOTBLOCK

	boff = CYL(xdp->xd_boff);
#ifdef BOOTBLOCK
	/* assert si_cc == DEV_BSIZE */
	if (xdcmd(cmd, sip, sip->si_bn + boff, sip->si_ma, 1))
		return (-1);
	return (sip->si_cc);
#else BOOTBLOCK
	nsect = sip->si_cc / DEV_BSIZE;
	if (sip->si_bn + nsect > xdp->xd_nblk)
		nsect = xdp->xd_nblk - sip->si_bn;
	if (nsect == 0)
		return (0);
	if (xdcmd(cmd, sip, sip->si_bn+boff, sip->si_ma, nsect) == 0)
		return (nsect*DEV_BSIZE);
	/*
	 * Large transfer failed, now do one at a time
	 */
	bn = sip->si_bn + boff;
	ma = sip->si_ma;
	for (i = 0; i < nsect; i++) {
		if (xdcmd(cmd, sip, bn, ma, 1))
			return (-1);
		bn++;
		ma += DEV_BSIZE;
	}
	return (nsect*DEV_BSIZE);
#endif	BOOTBLOCK
}

#ifndef BOOTBLOCK
/*
 * This routine is called from isspinning() as the test condition.
 */
int
xdspin(sip, dummy)
	struct saioreq *sip;
	int dummy;
{
	register struct xdparam *xdp = (struct xdparam *)sip->si_devdata;
	register struct xdiopb *xd = &((struct xddma *)sip->si_dmaaddr)->xdiopb;

	(void) xdcmd(XD_RESTORE, sip, 0, (char *)0, 0);
	return ((xd->xd_dstat & XD_READY) ? 1 : 0);
}
#endif !BOOTBLOCK

xdcmd(cmd, sip, bno, buf, nsect)
	int cmd;
	struct saioreq *sip;
	register int bno;
	char *buf;
	int nsect;
{
	register int i, t;
	register struct xdparam *xdp = (struct xdparam *)sip->si_devdata;
	register struct xdiopb *xd = &((struct xddma *)sip->si_dmaaddr)->xdiopb;
	register struct xddevice *xdaddr = (struct xddevice *)sip->si_devaddr;
	register char *bp = ((struct xddma *)sip->si_dmaaddr)->xdblock;
	unsigned short cylno, sect, head;
	int error, errcnt = 0;
	int timeout = 0;
	int firstime = 1;

	cylno = bno / CYL(1);
	sect = bno % xdp->xd_nsect;
	head = (bno / xdp->xd_nsect) % xdp->xd_nhead;

	if (cmd == XD_WRITE && buf != bp)	/* Many just use common buf */
		bcopy(buf, bp, nsect*DEV_BSIZE);

retry:
	bzero((char *)xd, sizeof (struct xdiopb));
	xd->xd_cmd = cmd >> 8;
	switch (cmd) {
		case XD_WPAR:
		    xd->xd_subfunc = XD_DRIVE;
		    xd->xd_drparam = 0;		
		    xd->xd_nsect = (short) ((xdp->xd_nsect - 1) << 8);
		    xd->xd_cylinder = xdp->xd_ncyl - 1;
		    xd->xd_head = xdp->xd_nhead - 1;
		    xd->xd_sector = xdp->xd_nsect - 1;
		    break;

		default:
		    xd->xd_subfunc = 0;
		    xd->xd_nsect = nsect;
		    xd->xd_cylinder = cylno;
		    xd->xd_head = head;
		    xd->xd_sector = sect;
		    xd->xd_bufaddr = (int) (MB_DMA_ADDR(bp));
	}
	xd->xd_unit = xdp->xd_unit & 3;
	xd->xd_intpri = 0;			/* disable interrupt */
	xd->xd_bufmod = XD_ADDRMOD24;		/* standard supervisory */

	t = (int) (MB_DMA_ADDR(xd));
	xdaddr->xd_iopbaddr1 = (int) t;
	xdaddr->xd_iopbaddr2 = (int) t >> 8;
	xdaddr->xd_iopbaddr3 = (int) t >> 16;
	xdaddr->xd_iopbaddr4 = (int) t >> 24;
	xdaddr->xd_modifier = XD_ADDRMOD24;
	xdaddr->xd_csr = XD_AIO;

	while (!(xdaddr->xd_csr & XD_RIO) && timeout < TIMEOUT)
		timeout++;	
	
	if (timeout == TIMEOUT) {
		/*
		 * kludge for the 751 prototype. this
		 * doesn't hurt in the normal case and we
		 * didn't wanted to risk another prom release.
		 * so here it is.
		 */
		if (firstime && cmd == XD_RESTORE) {
			xdaddr->xd_csr = XD_RST;
			DELAY(300);
			firstime = 0;
			timeout = 0;
			goto retry;
		} 
		printf("disk ctlr timeout cmd %x\n", cmd);
		xdaddr->xd_csr = XD_RST;	 /* reset the controller */
		return(-1);
	}

	if (xdaddr->xd_csr & XD_FERR) {
	    xdaddr->xd_csr = XD_RST;
	    DELAY(300);
	    xd->xd_iserr = 1;
	}

	xdaddr->xd_csr = XD_CLRIO;	/* clear rio bit */
	xdaddr->xd_csr = XD_CLRBS;	/* release register */

	if (xd->xd_iserr && xd->xd_errno != XDE_FECC) {
		if (nsect != 1)		/* only try hard on single sectors */
			return (-1);
		error = xd->xd_errno;
		if ((i = xdfwd(xdp, cylno, head, sect)) != 0)
			return xdcmd(cmd, sip, i, buf, 1);
		/* Attempt to reset the error condition */
		if (cmd != XD_RESTORE) {
			xdaddr->xd_csr = XD_RST; /* reset the controller */
			DELAY(300);		 /* and then the drive */
			(void) xdcmd(XD_RESTORE, sip, 0, (char *)0, 1);
		}
		if (++errcnt < 5)
			goto retry;
		if (bno != 0)	/* drive type probe */
			printf("xd: error %x bno %d\n", error, bno);
		return (-1);			/* Error */
	}

	if (cmd == XD_READ && buf != bp)	/* Many just use common buf */
		bcopy(bp, buf, nsect*DEV_BSIZE);

	return (0);
}

xdfwd(xdp, cn, tn, sn)
	register struct xdparam *xdp;
	int cn, tn, sn;
{
	register struct dkbad *bt = &xdp->xd_bad;
	register int i;

	if (bt->bt_mbz != 0)	/* not initialized */
		return (0);
	for (i=0; i<126; i++)		/* FIXME constant */
		if (bt->bt_bad[i].bt_cyl == cn &&
		    bt->bt_bad[i].bt_trksec == (tn<<8)+sn) {
			return (xdp->xd_badaddr - i - 1);
		}
	return (0);
}
#endif M25
