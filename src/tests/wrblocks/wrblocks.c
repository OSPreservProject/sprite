/*
 * wrblocks - create a file by writing a specified number of blocks to it.
 */
#include "sys/file.h"
#include "sig.h"
#include "stdio.h"
#include "option.h"
#include "errno.h"
#include "spriteTime.h"

/*
 * Options set by command line arguments
 */
int errorTest = 0;
int numKbytes = 10;
int blockSize = 16;

Option optionArray[] = {
    OPT_INT,  "e", (Address)&errorTest, "Do write() error cases",
    OPT_INT,  "k", (Address)&numKbytes, "Number of Kbytes for the file",
    OPT_INT,  "b", (Address)&blockSize, "Blocksize of writes, in Kbytes",
};
int numOptions = sizeof(optionArray) / sizeof(Option);


int Handler();
int gotSig = FALSE;



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
    char *buffer;
    int total, bytesWritten;
    Sig_Action		newAction, oldAction;
    Time before, after;
    double rate, tmp;
    int k;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    if (argc < 2) {
	openFileID = 1;
    } else {
	fileName = argv[1];

	openFileID = open(fileName, O_CREAT|O_WRONLY|O_TRUNC, 0666);
	if (openFileID < 0) {
	    fprintf(stderr, "Can't open \"%s\"\n", fileName);
	    perror("");
	    exit(1);
	}
    }
    /*
     * Set up signal handling, trap interrupts in order to test
     * the GEN_INTERRUPTED_BY_SIGNAL return code.
     */
    newAction.action = SIG_HANDLE_ACTION;
    newAction.handler = Handler;
    newAction.sigHoldMask = 0;
    Sig_SetAction(SIG_INTERRUPT, &newAction, &oldAction);

    buffer = (char *)malloc(1024 * blockSize);
    bzero((Address)buffer, 1024 * blockSize);

    if (!errorTest) {
	total = 0;
	Sys_GetTimeOfDay(&before, NULL, NULL);
	for (k=0 ; k<numKbytes ; k += blockSize) {
	    bytesWritten = write(openFileID, buffer, 1024 * blockSize);
	    if (bytesWritten < 0) {
		perror("Write failed:");
		break;
	    }
	    total += bytesWritten;
	    if (bytesWritten < 1024 * blockSize) {
		fprintf(stderr, "Short write (%d not %d)\n",
		    bytesWritten, 1024 * blockSize);
		break;
	    }
	}
	Sys_GetTimeOfDay(&after, NULL, NULL);
	rate = after.seconds - before.seconds;
	rate += (after.microseconds - before.microseconds)*.000001;
	rate = total/rate;
	fprintf(stderr,"%d bytes written at %.0f bytes/sec.\n", total, rate);
	exit(errno);
    } else {
	int numErrors = 0;
	printf("Write Error Tests\n");

	bytesWritten = write(-2, 0, 0);
	if (bytesWritten >= 0) {
	    printf("ERROR: write(-2) worked!\n");
	    numErrors++;
	} else {
	    perror("write(-2)");
	}

	bytesWritten = write(openFileID, -1, 10);
	if (bytesWritten >= 0) {
	    printf("ERROR: write{buffer = -1} worked!\n");
	    numErrors++;
	} else {
	    perror("write{buffer = -1}");
	}

	bytesWritten = write(openFileID, buffer, -1);
	if (bytesWritten >= 0) {
	    printf("ERROR: write{count < 0} worked!\n");
	    numErrors++;
	} else {
	    perror("write{count < 0}");
	}

	{
	    int readOnlyFD;
	    readOnlyFD = open("/dev/null", O_RDONLY, 0);
	    if (readOnlyFD < 0) {
		perror("Can't open \"/dev/null\"\n");
	    } else {
		bytesWritten = write(readOnlyFD, buffer, 10);
		if (bytesWritten >= 0) {
		    printf("ERROR: write{readonly stream} worked!\n");
		    numErrors++;
		} else {
		    perror("write{readonly stream}");
		}
	    }
	}

	{
	    char *newBuf = (char *)malloc(100 * 1024);
	    ReturnStatus status;

	    printf("Starting 100K write... "); fflush(stdout);
	    status = Fs_RawWrite(openFileID, 100 * 1024, newBuf, &bytesWritten);
	    if (gotSig) {
		printf("Got Signal, "); fflush(stdout);
	    }
	    if (bytesWritten >= 0) {
		printf("Wrote %d bytes\n", bytesWritten);
	    } else {
		printf("write status 0x%x\n", status);
	    }
	}

	close(openFileID);
	bytesWritten = write(openFileID, sizeof("oops"), "oops");
	if (bytesWritten >= 0) {
	    printf("ERROR: write{closed stream} worked!\n");
	    numErrors++;
	} else {
	    perror("write{closed stream}");
	}
	if (numErrors) {
	    printf("Write Test had %d errors\n", numErrors);
	} else {
	    printf("No errors\n");
	}
	exit(numErrors);
    }
}

int
Handler()
{
    gotSig = TRUE;
}

