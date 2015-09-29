
/*
 * @(#)idprom.c 1.4 88/02/08
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "idprom.h"

/*
 * Read the ID prom and check it.
 * Arguments are a format number and an address to store prom contents at.
 *
 * Result is format number if prom has the right format and good checksum.
 * Result is -1		   if prom has the right format and bad checksum.
 * Result is prom's format if prom has the wrong format.
 *
 * If the PROM is in the wrong format, the addressed area is not changed.
 *
 * This routine must know the size, and checksum algorithm, of each format.
 * (Currently there's only one.)
 */

int
idprom(format, idp)
	unsigned char format;
	register struct idprom *idp;
{
	unsigned char *cp, sum=0, promform;
	int len;
	short i;

	len = GetIDProm((unsigned char *)idp, sizeof(*idp));
	if (len != sizeof(*idp))
		return -1;
	promform = idp->id_format;
	if (format != promform)
		return promform;
	cp = (unsigned char *)idp;
	for (i=0; i<16; i++)
		sum ^= *cp++;
	if (sum != 0)
		return -1;
	return promform;
}
