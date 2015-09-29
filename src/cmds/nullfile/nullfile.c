/* 
 * nullfile.c --
 *
 *	Creates a file full of zeros (null characters).
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
static char rcsid[] = "$Header: /sprite/src/admin/nullfile/RCS/nullfile.c,v 1.1 91/04/14 22:08:17 jhh Exp Locker: jhh $ SPRITE (Berkeley)";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>

#define min(a,b) (((a) < (b)) ? (a) : (b) )
#define BUFFER_SIZE 4096

static char 	*progName;

void		Usage();

void
main(argc, argv)
    int 	argc;
    char	*argv[];
{
    char 	*name;
    int		count;
    FILE	*fp;
    static char buffer[BUFFER_SIZE];
    int		bytesWritten;
    int		bytesToWrite;

    progName = argv[0];
    if (argc != 3) {
	Usage();
    }
    name = argv[1];
    if (sscanf(argv[2], " %d", &count) != 1) {
	Usage();
    }
    if (count < 0) {
	fprintf(stderr,"Count argument must be positive.\n");
	Usage();
    }
    fp = fopen(name,"w+");
    if (fp == NULL) {
	fprintf(stderr,"Unable to open file %s.\n", name);
	exit(1);
    }
    bzero(buffer, BUFFER_SIZE);
    for (bytesWritten = 0; bytesWritten < count;) {
	bytesToWrite = min(BUFFER_SIZE, count - bytesWritten);
	fwrite(buffer, sizeof(char), bytesToWrite, fp);
	bytesWritten += bytesToWrite;
   }
   fclose(fp);
   exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * Usage --
 *
 *	Prints out the help message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Usage()
{
	(void) fprintf(stderr,"Usage: %s file count\n",progName);
	exit(1);
}
