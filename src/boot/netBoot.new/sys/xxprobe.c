
/*
 * @(#)xxprobe.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Probe routine for unspecial devices.
 *
 * We attempt an open on each of the standard devices.
 * This requires that the open routines not print a message for failure.
 *
 * We also catch bus errors in case the device's absence causes one.
 */
#include "../h/sunromvec.h"
#include "../h/setbus.h"
#include "../dev/saio.h"

extern int devopen();

int
xxprobe(sip)
	struct saioreq *sip;
{
	int i, r;
	bus_buf busbuf[4];
	register struct boottab *btab;

	btab = sip->si_boottab;

	for (i = 0; i < btab->b_devinfo->d_stdcount; i++) {
		sip->si_ctlr = btab->b_devinfo->d_stdaddrs[i];
		if (setbus(busbuf)) 
			continue;
		r = devopen(sip);
		unsetbus(busbuf);
		if (r >= 0) 
			return r;
	}
	return -1;		/* Not found */
}
