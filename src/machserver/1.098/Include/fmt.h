/*
 * fmt.h --
 *
 *	Declarations for the Fmt package.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/fmt.h,v 1.5 90/09/11 14:40:17 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _FMT
#define _FMT

#include <cfuncproto.h>

/*
 * Return values
 */
#define FMT_OK			0
#define FMT_CONTENT_ERROR	1
#define FMT_INPUT_TOO_SMALL	2
#define FMT_OUTPUT_TOO_SMALL	3
#define FMT_ILLEGAL_FORMAT	4

/*
 * Different data formats.
 */
typedef int Fmt_Format;

/*
 * For now we or in a high bit so we can be backwards compatible with
 * Swap_Buffer constants. Later on we'll get rid of this.
 */

#define FMT_68K_FORMAT		((Fmt_Format) 0x1000 | 1)
#define FMT_VAX_FORMAT		((Fmt_Format) 0x1000 | 2)
#define FMT_SPUR_FORMAT		((Fmt_Format) 0x1000 | 3)
#define FMT_MIPS_FORMAT		((Fmt_Format) 0x1000 | 4)
#define FMT_SPARC_FORMAT	((Fmt_Format) 0x1000 | 5)
#define FMT_SYM_FORMAT		((Fmt_Format) 0x1000 | 6)

/*
 * Define FMT_MY_FORMAT to be the "native" format
 */
#if defined(sun3) || defined(sun2)
#define FMT_MY_FORMAT	FMT_68K_FORMAT
#endif
#if defined(sun4)
#define FMT_MY_FORMAT	FMT_SPARC_FORMAT
#endif
#if defined(ds3100) || defined(mips)
#define FMT_MY_FORMAT	FMT_MIPS_FORMAT
#endif
#if defined(spur)
#define FMT_MY_FORMAT	FMT_SPUR_FORMAT
#endif
#if defined(vax)
#define FMT_MY_FORMAT	FMT_VAX_FORMAT
#endif
#if defined(sequent)
#define FMT_MY_FORMAT	FMT_SYM_FORMAT
#endif

/* procedures */

extern int Fmt_Convert _ARGS_((char *contents, Fmt_Format inFormat,
			       int *inSizePtr, char *inBuf,
			       Fmt_Format outFormat, int *outSizePtr,
			       char *outBuf));
extern int Fmt_Size _ARGS_((char *contents, Fmt_Format inFormat,
			    int *inSizePtr, Fmt_Format outFormat,
			    int *outSizePtr));

#endif /* _FMT */

