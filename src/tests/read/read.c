#include "sprite.h"
#include "time.h"
#include "fs.h"
#include "stdio.h"
#include "option.h"

static char *buffer;

int	repeats = 1;
int	blockSize = 16384;
int	msToPause = -1;
int	loops = -1;
Boolean errorTest = FALSE;

Option optionArray[] = {
    {OPT_INT, "r", (Address) &repeats,
     "\tNumber of times to repeat read (Default 1)."},
    {OPT_INT, "b", (Address) &blockSize, 
     "\tBlock size to use for reading (Default 16384)."},
    {OPT_INT, "p", (Address)&msToPause,
     "\tMilliseconds to pause between reads of each block. "},
    {OPT_INT, "l", (Address)&loops,
     "\tNumber of times to loop between reads of each block. "},
    {OPT_TRUE, "e", (Address)&errorTest,
     "\tTest error cases. "},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

int Handler();
int gotSig = FALSE;

main(argc, argv)
int argc;
char **argv;
{
    int 		cnt, total;
    double 		rate, tmp;
    Time 		before, after;
    int			newOffset;
    int		status;
    Time		pauseTime;
    Sig_Action		newAction, oldAction;
    register	int	i;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    /*
     * Set up signal handling, trap interrupts in order to test
     * the GEN_INTERRUPTED_BY_SIGNAL return code.
     */
    newAction.action = SIG_HANDLE_ACTION;
    newAction.handler = Handler;
    newAction.sigHoldMask = 0;
    Sig_SetAction(SIG_INTERRUPT, &newAction, &oldAction);

    buffer = (char *)malloc(blockSize);
    total = 0;
    if (msToPause > 0) {
	pauseTime.seconds = 0;
	pauseTime.microseconds = msToPause * 1000;
    }

    if (errorTest) {
	int numErrors = 0;
	printf("Read Error Tests NOT IMPLEMENTED\n"); 

#ifdef notdef
	Fs_Write(1, 3, "? ", &cnt);
	status = Fs_Read(-2, 0, 0, &cnt);
	if (status == SUCCESS) {
	    printf("ERROR: Fs_Read(-2) worked!\n");
	    numErrors++;
	} else {
	    Stat_PrintMsg(status, "Fs_Read(-2)");
	}

	Fs_Write(1, 3, "? ", &cnt);
	status = Fs_Read(0, 10, -1, &cnt);
	if (status == SUCCESS) {
	    printf("ERROR: Fs_Read{buffer = -1} worked!\n");
	    numErrors++;
	} else {
	    Stat_PrintMsg(status, "Fs_Read{buffer = -1}");
	}

	Fs_Write(1, 3, "? ", &cnt);
	status = Fs_Read(0, -1, buffer, &cnt);
	if (status == SUCCESS) {
	    printf("ERROR: Fs_Read{count < 0} worked!\n");
	    numErrors++;
	} else {
	    Stat_PrintMsg(status, "Fs_Read{count < 0}");
	}

	/*
	 * This uses Fs_RawRead because the library routine Fs_Read
	 * dies on a bad amountReadPtr.
	 */
	Fs_Write(1, 3, "? ", &cnt);
	status = Fs_RawRead(0, 10, buffer, 0);
	if (status == SUCCESS) {
	    printf("ERROR: Fs_RawRead{&cnt = 0} worked!\n");
	    numErrors++;
	} else {
	    Stat_PrintMsg(status, "Fs_RawRead{&cnt = 0}");
	}

	{
	    int outFD2;
	    status = Fs_Open("/dev/null", FS_WRITE, 0,&outFD2);
	    if (status != SUCCESS) {
		fprintf(stderr, "Could not open %s for writing, status %x\n",
			       "/dev/null", status);
	    } else {
		status = Fs_Read(outFD2, 10, buffer, &cnt);
		if (status == SUCCESS) {
		    printf("ERROR: Fs_Read{writeonly stream} worked!\n");
		    numErrors++;
		} else {
		    Stat_PrintMsg(status, "Fs_Read{writeonly stream}");
		}
	    }
	}

	{
	    char *newBuf = (char *)malloc(100 * 1024);
	    printf("Starting 100K read... "); Io_Flush(io_StdOut);
	    status = Fs_RawRead(0, 100 * 1024, newBuf, &cnt);
	    if (gotSig) {
		printf("Got Signal, "); Io_Flush(io_StdOut);
	    }
	    if (status == SUCCESS) {
		printf("Read %d bytes\n", cnt);
	    } else {
		Stat_PrintMsg(status, "read");
	    }
	}

	Fs_Close(0);
	Fs_Write(1, 3, "? ", &cnt);
	status = Fs_Read(0, 10, buffer, &cnt);
	if (status == SUCCESS) {
	    printf("ERROR: Fs_Read{closed stream} worked!\n");
	    numErrors++;
	} else {
	    Stat_PrintMsg(status, "Fs_Read{closed stream}");
	}
	if (numErrors) {
	    printf("Read Test had %d errors\n", numErrors);
	} else {
	    printf("No errors\n");
	}
#endif
	exit(numErrors);
    } else {
	Sys_GetTimeOfDay(&before, NULL, NULL);
	for ( ; repeats > 0; repeats--) {
	    lseek(0, 0, 0);
	    while (1) {
		if (loops > 0) {
		    for (i = loops; i > 0; i --) {
		    }
		}
		cnt = read(0, buffer, blockSize, buffer);
		total += cnt;
		if (cnt < blockSize) break;
	    }
	}
	Sys_GetTimeOfDay(&after, NULL, NULL);
	rate = after.seconds - before.seconds;
	rate += (after.microseconds - before.microseconds)*.000001;
	rate = total/rate;
	printf("%d bytes read at %.0f bytes/sec.\n", total, rate);
    }
}

int
Handler()
{
    gotSig = TRUE;
}
