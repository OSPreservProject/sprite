/* 
 * fwrite.c --
 *
 *	Source code for the "fwrite" library procedure.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/stdio/RCS/fwrite.c,v 1.2 91/12/12 22:09:22 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <bstring.h>

/*
 *----------------------------------------------------------------------
 *
 * fwrite --
 *
 *	This procedure outputs binary data to a buffered stream.
 *
 * Results:
 *	The return value is the number of complete items actually written.
 *	It may be less than numItems if an error condition was encountered;
 *	in this case, there may be an additional partial item output after
 *	the complete items.
 *
 * Side effects:
 *	Up to numItems*size bytes are written into the stream from memory at
 *	buf.
 *
 *----------------------------------------------------------------------
 */

int
fwrite(bufferPtr, size, numItems, stream)
    register char *bufferPtr;	/* Origin of items to be written on stream.
				 * Must contain numItems*size bytes. */
    int size;			/* Size of each item to be written. */
    int numItems;		/* Number of items to be written. */
    register FILE *stream;	/* Stream where bytes are to be written. */
{

    register int num, byteCount, itemCount;

    for (itemCount = 0; itemCount < numItems; itemCount++) {
        for (byteCount = size; byteCount > 0;) {
            if (stream->writeCount <=1 || stream->flags & STDIO_LINEBUF) {
                if (fputc(*bufferPtr, stream) == EOF) {
                    return(itemCount);
                }
                bufferPtr++;
                byteCount--;
            } else {
                num = stream->writeCount-1 < byteCount ? stream->writeCount-1
                        : byteCount;
                bcopy(bufferPtr, stream->lastAccess+1, num);
                stream->writeCount -= num;
                stream->lastAccess += num;
                bufferPtr += num;
                byteCount -= num;
            }
        }
    }
    return(numItems);
}
