/* 
 * installsun.c --
 *
 *	Check the header of a coff? file.
 *
 * Copyright 1990 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/admin/installboot/RCS/installsun.c,v 1.1 90/02/16 16:12:24 shirriff Exp $ SPRITE (Berkeley)";
#endif

#include <sprite.h>
#include <kernel/sun3.md/procMach.h>
#include <stdio.h>


/*
 *----------------------------------------------------------------------
 *
 * SunHeader -
 *
 *	See if the boot program has a sun header.
 *
 * Results:
 *	length of a.out header if succeeded, or -1 if there was an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
SunHeader(bootFID, loadAddr, execAddr, length)
    int bootFID;	/* Handle on the boot program */
    Address *loadAddr;	/* Address to start loading boot program. */
    Address *execAddr;	/* Address to start executing boot program. */
    int *length;	/* Length of boot program. */
{
    ProcExecHeader aout;
    int bytesRead;

    if (lseek(bootFID,0,0)<0) {
	perror("");
	return -1;
    }
    bytesRead = read(bootFID, (char *)&aout, sizeof(ProcExecHeader));
    if (bytesRead < 0 || aout.magic != PROC_OMAGIC) {
	return -1;
    }
    *loadAddr = 0;
    *execAddr = 0;
    *length = aout.code + aout.data;
    return sizeof(ProcExecHeader);
}
