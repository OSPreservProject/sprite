/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: findpost.c,v 1.1 88/02/11 17:08:48 jim Exp $";
#endif

/*
 * FindPostAmble - Find the postamble of a DVI file.
 *
 * N.B.: This routine assumes that ftell() returns byte offsets,
 * not magic cookies.
 */

#include <stdio.h>
#include "types.h"
#include "dvicodes.h"
#include "fio.h"

/*
 * I am making the assumption that 530 bytes will always be enough
 * to find the end of the DVI file.  12 should suffice, as there
 * should be at most seven DVI_FILLER bytes, preceded by the version
 * number, preceded by the four byte postamble pointer; but at least
 * one VMS TeX must pad to a full `sector'.
 */
#ifdef vms
#define POSTSIZE	530	/* make only VMS pay for its errors; */
#else
#define POSTSIZE	16	/* others get to use something reasonable */
#endif

long	ftell();

FindPostAmble(f)
	register FILE *f;
{
	register long offset;
	register char *p;
	register int i;
	register i32 n;
	char postbuf[POSTSIZE];

	/*
	 * Avoid fseek'ing beyond beginning of file; it may
	 * give odd results.
	 */
	fseek(f, 0L, 2);		/* seek to end */
	offset = ftell(f) - POSTSIZE;	/* and compute where to go next */
	if (offset < 0L)		/* but make sure it is positive */
		offset = 0L;
	fseek(f, offset, 0);
	p = postbuf;
	for (i = 0; i < POSTSIZE; i++) {
		*p++ = getc(f);
		if (feof(f)) {
			p--;
			break;
		}
	}

	/*
	 * Now search backwards for the VERSION byte.  The postamble
	 * pointer will be four bytes behind that.
	 */
	while (--i >= 0) {
		if (UnSign8(*--p) == DVI_VERSION)
			goto foundit;
		if (UnSign8(*p) != DVI_FILLER)
			break;
	}
	return (-1);		/* cannot find postamble ptr */

foundit:
	/*
	 * Change offset from the position at the beginning of postbuf
	 * to the position of the VERSION byte, and seek to four bytes
	 * before that.  Then get a long and use its value to seek to
	 * the postamble itself.
	 */
	offset += p - postbuf;
	fseek(f, offset - 4L, 0);
	fGetLong(f, n);
	offset = n;
	fseek(f, offset, 0);
	return (0);		/* success */
}
