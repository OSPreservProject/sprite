/* 
 * installdec.c --
 *
 *	Check the header of a coff file.
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
static char rcsid[] = "$Header: /sprite/src/admin/installboot/RCS/installdec.c,v 1.1 90/02/16 16:12:07 shirriff Exp $ SPRITE (Berkeley)";
#endif

#include <sprite.h>
#include <kernel/ds3100.md/procMach.h>
#include <stdio.h>


/*
 *----------------------------------------------------------------------
 *
 * DecHeader -
 *
 *	Check if the header is a dec (coff) file.
 *
 * Results:
 *	An error code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
ReturnStatus
DecHeader(bootFID, loadAddr, execAddr, length)
    int bootFID;	/* Handle on the boot program */
    Address *loadAddr;	/* Address to start loading boot program. */
    Address *execAddr;	/* Address to start executing boot program. */
    int *length;	/* Length of the boot program. */
{
    ProcExecHeader aout;
    int bytesRead;

    if (lseek(bootFID,0,0)<0) {
	perror("");
	return FAILURE;
    }
    bytesRead = read(bootFID, (char *)&aout, sizeof(ProcExecHeader));
    if (bytesRead < 0) {
	return FAILURE;
    }
    if (aout.aoutHeader.magic != PROC_OMAGIC) {
	return FAILURE;
    }
    *loadAddr = (Address) aout.aoutHeader.codeStart;
    *execAddr = (Address) aout.aoutHeader.entry;
    *length = aout.aoutHeader.codeSize + aout.aoutHeader.heapSize;
    if (lseek(bootFID,PROC_CODE_FILE_OFFSET(aout),0)<0) {
	perror("");
	return FAILURE;
    }
    printf("Input file is coff format\n");
    return SUCCESS;
}
