
/*
 * @(#)common.c 1.1 86/09/27
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Common code for various bootstrap routines.
 */

/*RJHG************************************************************************
 *RJHG*	20-Dec-85
 *RJHG*		CHANGE isspinning to exit to monitor if fail instead of
 *RJHG*		       returning an error.  This done to make this look
 *RJHG*		       like the Sun-2 monitor.  This may not be the best
 *RJHG*		       place to put this effect but it is the recommended
 *RJHG*		       place so we'll leave it here till we can get it changed
 *RJHG*
 *RJHG************************************************************************/

#include "../sun3/cpu.addrs.h"
#include "../dev/saio.h"
#include "../h/globram.h"
#ifndef major
/* Kludge avoids defining types.h twice...once in globram (maybe), once here */
#include "../h/types.h"
#endif major
#include "../dev/dklabel.h"

char msg_spinup[] = "\n\007Waiting for disk to spin up...\n\n\
Please start it, if necessary, -OR- press any key to quit.\n\n";
char msg_nolabel[] = "No label found - attempting boot anyway.\n";
char msg_noctlr[] = "No controller at mbio %x\n";

bzero(p, n)
	register char *p;
	register int n;
{
	register char zeero = 0;

	while (n > 0)
		*p++ = zeero, n--;	/* Avoid clr for 68000, still... */
}

bcopy(src, dest, count)
	register char *src, *dest;
	register short count;
{
	count--;
	do {
		*dest++ = *src++;
	} while (--count != -1);
}

chklabel(label)
	register struct dk_label *label;
{
	register int count, sum = 0;
	register short *sp;

	if (label->dkl_magic != DKL_MAGIC) {
		printf(msg_nolabel);
		return (1);
	}

	count = sizeof (struct dk_label) / sizeof (short);
	sp = (short *)label;
	while (count--) 
		sum ^= *sp++;
	if (sum != 0) {
		printf("Corrupt label\n");
		return (1);
	}
	return (0);
}

/*
 * Returns 0 if disk is apparently not spinning, after waiting awhile.
 * Returns 1 if disk is already spinning on entry to this routine.
 * Returns 2 if disk starts spinning sometime after entry to this routine.
 * 
 * If on initial entry, after a suitable delay, the disk is not spinning,
 * we produce a message to remind the user to turn on the disk.
 */
isspinning(isready, addr, data) 
	int (*isready)();
	char *addr;
	int data;
{
	long t;

	/*
	 * Wait for disk to spin up, if necessary.
	 */
	if ((*isready)(addr, data)) return 1;
	DELAY(1000000);			/* RTZ can take a long time */
	if ((*isready)(addr, data)) return 2;
	while (mayget() != -1)		/* clear typeahead */
		;
	printf(msg_spinup);
	t = (3*60*1000) + gp->g_nmiclock;	/* quitting time in 3 minutes. */
	while (!(*isready)(addr, data)) {
		if (mayget() != -1)
/*RJHG*//*		return 0; Removed to allow a call to exit_to_mon() */
/*RJHG*/            {
/*RJHG*/	    exit_to_mon(); /* Go to monitor on fail (like Sun-2) */
                    }
		if (gp->g_nmiclock > t) {
			printf("Giving up...\n\n");
/*RJHG*//*		return 0; Removed to allow a call to exit_to_mon() */
/*RJHG*/		exit_to_mon(); /* Go to monitor on fail (like Sun-2) */
		}
	}
	return 2;			/* Must be spinning... */
}
