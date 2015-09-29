/* 
 * sun23Print.c --
 *
 *	Contains the machine specific routine for printing the size if the
 *	machine is a sun2 or sun3 (they both have the same a.out format).
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/size/RCS/printMips.c,v 1.4 90/02/16 13:47:18 rab Exp $";
#endif /* not lint */

#include <assert.h>

#ifndef mips
#define mips 1
#endif

#ifndef LANGUAGE_C
#define LANGUAGE_C 1
#endif

#ifndef MIPSEL
#define MIPSEL
#endif

#if 0
#include <ds3100.md/aouthdr.h>
#include <ds3100.md/filehdr.h>
#include <ds3100.md/scnhdr.h>
#include <ds3100.md/sys/exec.h>
#include <ds3100.md/nlist.h>
#endif

#include <ds3100.md/a.out.h>
#include "size.h"


/*
 *----------------------------------------------------------------------
 *
 * PrintMips --
 *
 *	Prints out the size information for a decStation 3100
 *
 * Results:
 *	SUCCESS if size information was printed, FAILURE otherwise.
 *
 * Side effects:
 *	Stuff is printed.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
PrintMips(fp, printName, fileName, printHeadings, bufferSize, buffer)
    FILE	*fp;		/* file that header was read from */
    Boolean	printName;	/* TRUE => print name of file */
    char	*fileName;	/* name of file */
    Boolean	printHeadings;	/* TRUE => print column headings */
    int		bufferSize;	/* size of buffer */
    char	*buffer;	/* buffer containing header */
{

    struct exec         *header;
    struct aouthdr	*aoutheader;
    char		swappedHeader[sizeof(*header) * 2];
    int			swappedSize = sizeof(swappedHeader);
    int			status;

    assert(HEADERSIZE >= sizeof(struct exec));
    if (bufferSize < sizeof(struct exec)) {
	return FAILURE;
    }
    header = (struct exec *) buffer;
    aoutheader = &header->ex_o;
    /*
     * See if the magic number is ok.  If it is not, and the format
     * is not mips format, then swap to little-endian and see if
     * that fixes it.
     */
    if (N_BADMAG(*aoutheader) && (FMT_MIPS_FORMAT != hostFmt)) {
	status = Fmt_Convert("{h2w3h2h2w13}", FMT_MIPS_FORMAT, &bufferSize,
	    buffer, hostFmt, &swappedSize, swappedHeader);
	if (status) {
	    fprintf(stderr, "Fmt_Convert returned %d.\n", status);
	    return FAILURE;
	}
	header = (struct exec *) swappedHeader;
	aoutheader = &header->ex_o;
	if (N_BADMAG(*aoutheader)) {
	    return FAILURE;
	}
    }
    if (printHeadings) {
	printf("%-7s %-7s %-7s %-7s %-7s\n", 
	       "text", "data", "bss", "dec", "hex");
    }
    printf("%-7d %-7d %-7d %-7d %-7x",
		   header->a_text, header->a_data, header->a_bss,
		   header->a_text + header->a_data + header->a_bss,
		   header->a_text + header->a_data + header->a_bss);
    if (printName) {
	printf("\t%s\n", fileName);
    } else {
	printf("\n");
    }
    return SUCCESS;
}
