/*
 * wrblocks - create a file by writing a specified number of blocks to it.
 */
#include <sys/file.h>
#include "fs.h"
#include "stdio.h"
#include "option.h"
#include "errno.h"

/*
 * Options set by command line arguments
 */
int numKbytes = 10;
int blockSize = 16;

Option optionArray[] = {
    OPT_INT,  "k", (Address)&numKbytes, "Number of Kbytes for the file",
    OPT_INT,  "b", (Address)&blockSize, "Blocksize of writes, in Kbytes",
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Open the specified file and writes numKbytes 1K blocks to it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Writes the file.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    int openFileID;
    char *fileName;
    char *bufPtr;
    int bytesWritten;
    int k;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    if (argc < 2) {
	openFileID = 1;
    } else {
	fileName = argv[1];

	openFileID = open(fileName, O_CREAT|O_WRONLY, 0666);
	if (openFileID < 0) {
	    fprintf(stderr, "Can't open \"%s\"\n", fileName);
	    perror("");
	    exit(1);
	}
    }

    bufPtr = (char *)malloc(1024 * blockSize);
    bzero((Address)bufPtr, 1024 * blockSize);
    for (k=0 ; k<numKbytes ; k += blockSize) {
	bytesWritten = write(openFileID,  bufPtr, 1024 * blockSize);
	if (bytesWritten < 1024 * blockSize) {
	    if (bytesWritten < 0) {
		perror("Write failed:");
	    } else  {
		fprintf(stderr, "Short write (%d not %d)\n",
		    bytesWritten, 1024 * blockSize);
	    }
	    break;
	}
    }
    exit(errno);
}
