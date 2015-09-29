/*
 * Copyright (c) 1987 University of Maryland Department of Computer Science.
 * All rights reserved.  Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: gripes.c,v 1.1 88/02/11 17:08:51 jim Exp $";
#endif

/*
 * Common errors (`gripes').
 */

#include <stdio.h>
#include "types.h"

static char areyousure[] = "Are you sure this is a DVI file?";

extern	errno;

/*
 * DVI file requests a font it never defined.
 */
GripeNoSuchFont(n)
	i32 n;
{

	error(0, 0, "DVI file wants font %ld, which it never defined", n);
	error(1, 0, areyousure);
	/* NOTREACHED */
}

/*
 * DVI file redefines a font.
 */
GripeFontAlreadyDefined(n)
	i32 n;
{

	error(0, 0, "DVI file redefines font %ld", n);
	error(1, 0, areyousure);
	/* NOTREACHED */
}

/*
 * Unexpected DVI opcode.
 */
GripeUnexpectedOp(s)
	char *s;
{

	error(0, 0, "unexpected %s", s);
	error(1, 0, areyousure);
	/* NOTREACHED */
}

/*
 * Missing DVI opcode.
 */
GripeMissingOp(s)
	char *s;
{

	error(0, 0, "missing %s", s);
	error(1, 0, areyousure);
	/* NOTREACHED */
}

/*
 * Cannot find DVI postamble.
 */
GripeCannotFindPostamble()
{

	error(0, 0, "cannot find postamble");
	error(1, 0, areyousure);
	/* NOTREACHED */
}

/*
 * Inconsistent DVI value.
 */
GripeMismatchedValue(s)
	char *s;
{

	error(0, 0, "mismatched %s", s);
	error(1, 0, areyousure);
	/* NOTREACHED */
}

/*
 * Undefined DVI opcode.
 */
GripeUndefinedOp(n)
	int n;
{

	error(0, 0, "undefined DVI opcode %d");
	error(1, 0, areyousure);
	/* NOTREACHED */
}

/*
 * Cannot allocate memory.
 */
GripeOutOfMemory(n, why)
	int n;
	char *why;
{

	error(1, errno, "ran out of memory allocating %d bytes for %s",
		n, why);
	/* NOTREACHED */
}

/*
 * Cannot get a font.
 * RETURNS TO CALLER
 */
GripeCannotGetFont(name, mag, dsz, dev, fullname)
	char *name;
	i32 mag, dsz;
	char *dev, *fullname;
{
	int e = errno;
	char scale[40];

	if (mag == dsz)		/* no scaling */
		scale[0] = 0;
	else
		(void) sprintf(scale, " scaled %d",
			(int) ((double) mag / (double) dsz * 1000.0 + .5));

	error(0, e, "cannot get font %s%s", name, scale);
	if (fullname)
		error(0, 0, "(wanted, e.g., \"%s\")", fullname);
	else {
		if (dev)
			error(1, 0, "(there are no fonts for the %s engine!)",
				dev);
		else
			error(1, 0, "(I cannot find any fonts!)");
		/* NOTREACHED */
	}
}

/*
 * Font checksums do not match.
 * RETURNS TO CALLER
 */
GripeDifferentChecksums(font, tfmsum, fontsum)
	char *font;
	i32 tfmsum, fontsum;
{

	error(0, 0, "\
WARNING: TeX and I have different checksums for font\n\
\t\"%s\"\n\
\tPlease notify your TeX maintainer\n\
\t(TFM checksum = 0%o, my checksum = 0%o)",
		font, tfmsum, fontsum);
}

/*
 * A font, or several fonts, are missing, so no output.
 */
GripeMissingFontsPreventOutput(n)
	int n;
{
	static char s[2] = {'s', 0};

	error(1, 0, "%d missing font%s prevent%s output (sorry)", n,
		n > 1 ? s : &s[1], n == 1 ? s : &s[1]);
	/* NOTREACHED */
}
