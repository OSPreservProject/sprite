/* 
 * strUtil.c --
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
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif /* not lint */

#include <ctype.h>
#include <strings.h>
#include "sprite.h"
#include "fs.h"


/*
 *----------------------------------------------------------------------
 *
 * ReadFile --
 *
 *	Reads upto bufLen characters from fileName into fileBuf.
 *
 * Results:
 *	fileBuf contains contents of fileName.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef TESTING
#include <sys/file.h>

ReturnStatus
ReadFile(fileName, bufLen, fileBuf)
    char *fileName;
    int   bufLen;
    char *fileBuf;
{
    int fd;

    fd = open(fileName, O_RDONLY, 0);
    if (fd < 0) {
        return FAILURE;
    }
    bufLen = read(fd, fileBuf, bufLen);
    if (bufLen < 0) {
        return FAILURE;
    }
    fileBuf[bufLen] = '\0';
    close(fd);
    return SUCCESS;
}
#else
ReturnStatus
ReadFile(fileName, bufLen, fileBuf)
    char *fileName;
    int   bufLen;
    char *fileBuf;
{
    ReturnStatus status;
    Fs_Stream *streamPtr;

    status = Fs_Open(fileName, FS_READ | FS_FOLLOW, FS_FILE, 0, &streamPtr);
    if (status != SUCCESS) {
        return status;
    }
    status = Fs_Read(streamPtr, (Address) fileBuf, 0, &bufLen);
    if (status != SUCCESS) {
        return status;
    }
    fileBuf[bufLen] = '\0';
    (void)Fs_Close(streamPtr);
    return SUCCESS;
}
#endif TESTING


/*
 *----------------------------------------------------------------------
 *
 * ReadFile --
 *
 *	Copy first line in ps1 to s2 replacing \n with \0.
 *	Change ps1 to point to the 1st character of second line.
 *
 * Results:
 *	NIL if no line is found s2 otherwise.
 *
 * Side effects:
 *	ps1 points to next line.
 *
 *----------------------------------------------------------------------
 */

char *
ScanLine(ps1, s2)
    char **ps1, *s2;
{
    char *s1, *retstr;

    retstr = s2;
    s1 = *ps1;
    while ( *s1 != '\0' && *s1 != '\n' ) {
        *s2++ = *s1++;
    }
    if ( *s1 == '\0' ) {
        return ( (char *) NIL );
    }
    *s2 = '\0';
    *ps1 = s1+1;
    return ( retstr );
}


/*
 *----------------------------------------------------------------------
 *
 * ReadFile --
 *
 *	Copy first word in *ps1 to s2 (words are delimited by white space).
 *	Change ps1 to point to the 2nd character after the first word
 *	(i.e. skip over the delimiting whitespace).  This allows ps1 and
 *	s2 to initially point to the same character string.
 *
 * Results:
 *	Returns s2 or NIL if no word is found.
 *
 * Side effects:
 *	ps1 points to the first character after the delimiting whitespace.
 *
 *----------------------------------------------------------------------
 */

char *
ScanWord(ps1, s2)
    char **ps1, *s2;
{
    char *s1, *retstr;

    retstr = s2;
    for ( s1 = *ps1; *s1 != '\0' && isspace(*s1); s1++ ) {
    }
    while ( *s1 != '\0' && !isspace(*s1) ) {
        *s2++ = *s1++;
    }
    if ( *s1 == '\0' ) {
        return ( (char *) NIL );
    }
    *s2 = '\0';
    *ps1 = s1+1;
    return ( retstr );
}
