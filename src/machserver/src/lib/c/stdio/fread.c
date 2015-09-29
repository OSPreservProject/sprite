/* 
 * fread.c --
 *
 *	Source code for the "fread" library procedure.
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
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/src/lib/c/stdio/RCS/fread.c,v 1.2 91/12/12 22:09:21 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <bstring.h>

/*
 *----------------------------------------------------------------------
 *
 * fread --
 *
 *	This procedure inputs binary data from a buffered stream.
 *
 * Results:
 *	The return value is the number of complete items actually read.
 *	It may be less than numItems if an end-of-file or error condition
 *	was encountered;  in this case, there may be an additional
 *	partial item in buf after the complete items.
 *
 * Side effects:
 *	Up to numItems*size bytes are read from stream to the memory at
 *	buf.
 *
 *----------------------------------------------------------------------
 */

int
fread(bufferPtr, size, numItems, stream)
    register char *bufferPtr;	/* Place to put the items that are read.
				 * Must have enough space for numItems*size
				 * bytes. */
    int size;			/* Size of each item to be read. */
    int numItems;		/* Number of items to be read from stream. */
    register FILE *stream;	/* Stream from which bytes are to be read. */
{
    register int num, c, byteCount, itemCount;

    for (itemCount = 0; itemCount < numItems; itemCount++) {
        for (byteCount = size; byteCount > 0;) {
            if (stream->readCount>1) {
                num = stream->readCount < byteCount ? stream->readCount
                        : byteCount;
                bcopy(stream->lastAccess+1, bufferPtr, num);
                stream->lastAccess += num;
                stream->readCount -= num;
                byteCount -= num;
                bufferPtr += num;
            } else {
                c = getc(stream);
                if (c == EOF) {
                    return(itemCount);
                }
                *bufferPtr = c;
                bufferPtr++;
                byteCount--;
            }
        }
    }
    return(numItems);
}
