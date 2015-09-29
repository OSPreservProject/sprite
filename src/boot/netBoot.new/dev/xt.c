#ifndef	M25
#ifndef lint
/* static	char sccsid[] = "@(#)xt.c 1.1 86/09/27 Copyr 1986 Sun Micro"; */
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Standalone driver for Xylogics 472 Tape Controller
 */

#include "../dev/saio.h"
#include "../dev/xycreg.h"
#include "../dev/xtreg.h"


#define NXTADDR	2
unsigned long xtaddrs[] = { 0xEE60, 0xEE68 };

#define MAXXTREC	(20*1024)	/* max size tape rec allowed */

struct xtdma {
	struct xtiopb	xtiopb;
	char 		xtblock[MAXXTREC];
};

/* Define resources needed by this driver */
struct devinfo xtinfo = {
	sizeof(struct xydevice), sizeof (struct xtdma), 0,
	NXTADDR, xtaddrs, MAP_MBIO,
};

int	xtstrategy(), xtopen(), xtclose();
extern int	nullsys(), ttboot();

struct boottab xtdriver = {
	"xt",	nullsys, ttboot, xtopen, xtclose, xtstrategy,
	"xt: Xylogics 472 tape",	&xtinfo,
};

xtopen(sip)
	register struct saioreq *sip;
{
	register skip;
	register struct xydevice *xyaddr;

	xyaddr = (struct xydevice *) sip->si_devaddr;
	if (pokec((char *)xyaddr, 0))
		return (-1);
	skip = xyaddr->xy_resupd;	/* controller reset */
	sip->si_devdata = 0;		/* eof flag */
	xtcmd(sip, XT_SEEK, XT_REW);
	skip = sip->si_boff;
	while (skip--) {
		sip->si_cc = 0;
		xtcmd(sip, XT_SEEK, XT_FILE);
	}
	return (0);
}

xtclose(sip)
	register struct saioreq *sip;
{

	sip->si_devdata = 0;		/* eof flag */
	xtcmd(sip, XT_SEEK, XT_REW);
}

xtstrategy(sip, rw)
	struct saioreq *sip;
	int rw;
{
	int func = (rw == WRITE) ? XT_WRITE : XT_READ;
	int n, occ, cc;
	char *oma;

	cc = occ = sip->si_cc;
	oma = sip->si_ma;

	if (sip->si_devdata) {		/* eof */
		sip->si_devdata = 0;
		return (0);
	}
	while (cc > 0) {
		if (cc > MAXXTREC)
			sip->si_cc = MAXXTREC;
		else
			sip->si_cc = cc;
		n = xtcmd(sip, func, 0);
		if (n <= 0)
			break;
		sip->si_ma += n;
		cc -= n;
	}
	if (n == -1)
		return (-1);
	if (n == 0 && cc != occ)
		sip->si_devdata = (char *)1;	/* EOF */
	if (cc < 0)
		cc = 0;
	n = occ - cc;
	sip->si_cc = occ;
	sip->si_ma = oma;
	return (n);
}

xtcmd(sip, func, subfunc)
	register struct saioreq *sip;
{
	register struct xtiopb *xt = &((struct xtdma *)sip->si_dmaaddr)->xtiopb;
	register struct xydevice *xyaddr = (struct xydevice *)sip->si_devaddr;
	char *xtbuf = ((struct xtdma *)sip->si_dmaaddr)->xtblock;
	int err, t;

	bzero((char *)xt, sizeof (struct xtiopb));
	xt->xt_reloc = 1;
	xt->xt_autoup = 1;
	xt->xt_cmd = func;
	xt->xt_subfunc = subfunc;
	xt->xt_unit = sip->si_unit & 3;
	xt->xt_throttle = 5;
	switch (func) {

	case XT_READ:
		xt->xt_cnt = sip->si_cc;
		xt->xt_swab = 1;
		xt->xt_retry = 1;
		break;

	case XT_WRITE:
		xt->xt_cnt = sip->si_cc;
		bcopy((char *)sip->si_ma, xtbuf, xt->xt_cnt);
		xt->xt_swab = 1;
		xt->xt_retry = 1;
		break;

	default:
		xt->xt_cnt = 1;
		break;
	}
	xt->xt_bufoff = XYOFF(xtbuf);
	xt->xt_bufrel = XYREL(xyaddr, xtbuf);

	t = XYREL(xyaddr, (char *)xt);
	xyaddr->xy_iopbrel[0] = t >> 8;
	xyaddr->xy_iopbrel[1] = t;
	xyaddr->xy_iopboff[0] = ((int)xt) >> 8;
	xyaddr->xy_iopboff[1] = (int)xt;
	xyaddr->xy_csr = XY_GO;

	do {
		DELAY(30);
	} while (xyaddr->xy_csr & XY_BUSY);
	err = xt->xt_errno;

	if (err != XTE_NOERROR && err != XTE_SHORTREC && err != XTE_LONGREC) {
		if (err == XTE_EOF || err == XTE_EOT)
			return (0);
		/* Note: controller does retries for us */
		printf("xt hard err %x\n", err);
		return (-1);
	}
	if (func == XT_READ)
		bcopy(xtbuf, (char *)sip->si_ma, xt->xt_acnt);
	return (xt->xt_acnt);
}
#endif	M25
