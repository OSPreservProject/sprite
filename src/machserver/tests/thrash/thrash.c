/* 
 * thrash.c --
 *
 *	Test program to cause lots of paging.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/tests/thrash/RCS/thrash.c,v 1.3 92/01/22 13:22:14 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <mach.h>
#include <ctype.h>
#include <status.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <test.h>
#include <vm.h>

/* 
 * Name of the big mapped file to create.
 */
#define FILE_NAME	"DummyFile"

/* 
 * Number of iterations to walk through the file.
 */
#define ITERATIONS	3

Boolean gentle = FALSE;		/* pause between faults? */

/* Forward references */

static void MakeFile _ARGS_((char *fileName));

main(argc, argv)
    int argc;
    char *argv[];
{
    int bufferLength = 32 * 1024 * 1024;
    char *buffer;
    int iter;			/* iteration count */
    char *cp;
    ReturnStatus status;

    if (argc > 1) {
	gentle = TRUE;
    }

    status = Vm_MapFile(FILE_NAME, FALSE, (off_t)0,
			(vm_size_t)bufferLength, &buffer);
    if (status != SUCCESS) {
	Test_PutMessage("Couldn't map file: ");
	Test_PutMessage(Stat_GetMsg(status));
	Test_PutMessage("\n");
	goto bailOut;
    }

    /* 
     * Touch all the pages in the file, then wait a bit before 
     * repeating.  Verify that the value you wrote is still there.  
     * Pause before touching each page so as to reduce the load on 
     * Mach.  (Running at full speed eventually causes the ethernet 
     * interface to get a bus error, at least on a sun3.)
     */
    for (iter = 0; iter < ITERATIONS; iter++) {
	Test_PutMessage("iteration ");
	Test_PutDecimal(iter+1);
	Test_PutMessage(": ");
	for (cp = buffer; cp < buffer + bufferLength;
	     cp += vm_page_size) {
	    if (gentle) {
		msleep(250);
	    }
	    if ((cp - buffer) % (20 * vm_page_size) == 0) {
		Test_PutDecimal(100 * (unsigned)(cp - buffer) / bufferLength);
		Test_PutMessage(" ");
	    }
	    if (iter > 0) {
		if (*cp != 'a' + iter - 1) {
		    Test_PutMessage("thrash: wrong value (");
		    Test_PutHex(*(unsigned char *)cp);
		    Test_PutMessage(") at offset ");
		    Test_PutHex(cp - buffer);
		    Test_PutMessage("\n");
		}
	    }
	    *cp = 'a' + iter;
	}

	Test_PutMessage("pausing\n");
	msleep(1000);
    }

 bailOut:
    exit(0);
}
