/* Yet another test copy program, this time using stdio. */

#ifndef lint
static char rcsid[] = "$Header: /r3/kupfer/spriteserver/tests/stdioCopy/RCS/copy.c,v 1.1 91/12/12 22:45:52 kupfer Exp $";
#endif

#include <sprite.h>
#include <stdio.h>
#include <stdlib.h>

static void CopyFile();

int
main(argc, argv)
    int argc;
    char *argv[];
{
    if (argc != 3) {
	fprintf(stderr, "usage: copy from to\n");
	exit(1);
    }
    CopyFile(argv[1], argv[2]);
    
    return 0;
}

static void
CopyFile(fromFileName, toFileName)
    char *fromFileName;
    char *toFileName;
{
    FILE *fromPtr;
    FILE *toPtr;
    Address buffer;
    int bufferSize;		/* number of bytes in buffer */

    fromPtr = fopen(fromFileName, "r");
    if (fromPtr == NULL) {
	fprintf(stderr, "Can't open %s for reading\n", fromFileName);
	exit(1);
    }
    toPtr = fopen(toFileName, "w+");
    if (toPtr == NULL) {
	fprintf(stderr, "Can't open %s for writing\n", toFileName);
	exit(1);
    }

    bufferSize = 512;
    buffer = malloc(bufferSize);
    if (buffer == NULL) {
	fprintf(stderr, "Couldn't malloc buffer\n");
	exit(1);
    }

    while (fgets(buffer, bufferSize, fromPtr) != NULL) {
	fputs(buffer, toPtr);
    }
}
