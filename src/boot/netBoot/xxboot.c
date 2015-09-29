
/*
 * @(#)xxboot.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * General boot code for routines which implement the "standalone
 * driver" boot interface.
 */

#include "saio.h"
#include "bootparam.h"
#include "sunromvec.h"

extern char *devalloc();
extern char *resalloc();
extern int tftpboot();

int
xxboot(bp)
	register struct bootparam *bp;
{
	struct saioreq req;
	int blkno;
	char *addr;

	req.si_ctlr = bp->bp_ctlr;
	req.si_unit = bp->bp_unit;
	req.si_boff = (daddr_t)bp->bp_part;
	req.si_boottab = bp->bp_boottab;

	if (devopen(&req))	/* Do all the hard work */
		return -1;

	for (blkno = 1, addr = (char *)LOADADDR;
	     blkno <= BBSIZE/DEV_BSIZE;
	     blkno++, addr += DEV_BSIZE) {
		req.si_bn = blkno;
		req.si_cc = DEV_BSIZE;
		req.si_ma = addr;
		if (req.si_cc != (*bp->bp_boottab->b_strategy)(&req, READ))
			return -1;
	}
	return LOADADDR;
}

int
devopen(sip)
	register struct saioreq *sip;
{
	register struct devinfo *dp;
	char *a;

	sip->si_devaddr = sip->si_devdata = sip->si_dmaaddr = (char *)0;
	dp = sip->si_boottab->b_devinfo;
	if (dp) {
		/* Map controller number into controller address */
		if (sip->si_ctlr < dp->d_stdcount) {
			sip->si_ctlr = (int)((dp->d_stdaddrs)[sip->si_ctlr]);
		}
		/* Map in device itself */
		if (dp->d_devbytes) {
			a = devalloc(dp->d_devtype, sip->si_ctlr,
				dp->d_devbytes);
			if (!a)
				return (-1);
			sip->si_devaddr = a;
		}
		if (dp->d_dmabytes) {
			a = resalloc(RES_DMAMEM, dp->d_dmabytes);
			if (!a) 
				return (-1);
			sip->si_dmaaddr = a;
		}
		if (dp->d_localbytes) {
			a = resalloc(RES_MAINMEM, dp->d_localbytes);
			if (!a) 
				return (-1);
			sip->si_devdata = a;
		}
	}
	return ((sip->si_boottab->b_open)(sip));
}

/*
 * Close device, release resources.
 * FIXME, prom HAS no resources!!!
 */
int
devclose(sip)
	struct saioreq *sip;
{
	return ((*sip->si_boottab->b_close)(sip));
}

int
ttboot(bp)
        register struct bootparam *bp;
{
        struct saioreq req;
        int blkno;
	register int len;
        char *addr;
	register int result = -1;

        req.si_ctlr = bp->bp_ctlr;
        req.si_unit = bp->bp_unit;
        req.si_boff = (daddr_t)bp->bp_part;
	req.si_boottab = bp->bp_boottab;

        if (devopen(&req))      /* Do all the hard work */
                return -1;

        for (blkno = 1, addr = (char *)LOADADDR;; blkno++, addr += len) {
                req.si_bn = blkno;
                req.si_cc = 32768;
                req.si_ma = addr;
		len = (*bp->bp_boottab->b_strategy)(&req, READ);
		if (len == 0) {
			if (blkno != 1)
				result = LOADADDR;
			break;
		}
        }
	(*bp->bp_boottab->b_close)(&req);
        return result;
}

int
tftpboot(bp)
        register struct bootparam *bp;
{
        struct saioreq req;
        int blkno;
        char *addr;
 
        req.si_ctlr = bp->bp_ctlr;
        req.si_unit = bp->bp_unit;
        req.si_boff = (daddr_t)bp->bp_part;
        req.si_boottab = bp->bp_boottab;
 
        if (devopen(&req))      /* Do all the hard work */
                return -1;
 
	if (tftpload(&req) == -1){
		return(-1);
	} else {
		return LOADADDR;
	}
}
