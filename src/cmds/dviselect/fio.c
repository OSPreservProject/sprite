/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: fio.c,v 1.1 88/02/11 17:08:48 jim Exp $";
#endif

/*
 * File I/O subroutines for getting bytes, words, 3bytes, and longwords.
 */

#include <stdio.h>
#include "types.h"
#include "fio.h"

static char eofmsg[] = "unexpected EOF";

/* for symmetry: */
#define	fGetByte(fp, r)	((r) = getc(fp))
#define	Sign32(i)	(i)

#define make(name, func, signextend) \
i32 \
name(fp) \
	register FILE *fp; \
{ \
	register i32 n; \
 \
	func(fp, n); \
	if (feof(fp)) \
		error(1, 0, eofmsg); \
	return (signextend(n)); \
}

make(GetByte,  fGetByte,  Sign8)
make(GetWord,  fGetWord,  Sign16)
make(Get3Byte, fGet3Byte, Sign24)
make(GetLong,  fGetLong,  Sign32)
