/*
 * fileInt.h --
 *
 *	Declarations for things shared by the various stdio procedures
 *	that implement I/O to and from files.  The stuff in here is
 *	only used internally by stdio, and isn't exported to the outside
 *	world.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /user5/kupfer/spriteserver/src/lib/c/stdio/RCS/fileInt.h,v 1.3 92/03/23 15:02:31 kupfer Exp $ SPRITE (Berkeley)
 */

#ifndef _FILEINT
#define _FILEINT

#ifndef _STDIO
#include <stdio.h>
#endif

#include <cfuncproto.h>

/*
 * The streams for files are kept in a linked list pointed to by
 * stdioFileStreams.
 */

extern FILE		*stdioFileStreams;

extern unsigned char	stdioTempBuffer[];	/* Temporary buffer to use for
						 * writable streams. */

extern unsigned char 	stdioStderrBuffer[];	/* Static buffer for stderr */

extern int	StdioFileCloseProc _ARGS_((FILE *stream));
extern int	StdioFileOpenMode _ARGS_((char *access));
extern void	StdioFileReadProc _ARGS_((FILE *stream));
extern void	StdioFileWriteProc _ARGS_((FILE *stream, int flush));

#endif _FILEINT
