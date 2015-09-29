/*
 * rpcecho - do a series of rpc Echoes to a server for testing.
 */
#include "sprite.h"
#include "fs.h"
#include "fsCmd.h"
#include "stdio.h"
#include "rpc.h"
#include "netEther.h"
#include "kernel/rpcPacket.h"
#include "proc.h"
#include "option.h"
#include "host.h"
#include "sysStats.h"

/*
 * Default values for parameters
 */
char *server = NULL;
int numEchoes = 100;
#define DATASIZE	32
int size = DATASIZE;
int maxSize = 16 * 1024;
int numReps = 10;

#define BYTES_PER_FRAG	(NET_ETHER_MAX_BYTES - sizeof(RpcHdr))
/*
 * Buffer areas for data sent and received
 */
#define BUFSIZE (16 * 1024)
char inDataBuffer[BUFSIZE];
char outDataBuffer[BUFSIZE];

/*
 * Server flags set by command line arguments
 */
Boolean doEchoes = TRUE;
Boolean doSends = FALSE;
#ifdef SPRITED_USERPRIORITY
Boolean fastCS = FALSE;
#endif
Boolean trace = FALSE;
Boolean allSizes = FALSE;
Boolean reverse = FALSE;


Option optionArray[] = {
    OPT_INT, "n", (Address)&numEchoes, "Number of RPCs to do",
    OPT_INT, "d", (Address)&size, "Datasize to transmit",
    OPT_TRUE, "D", (Address)&allSizes, "Do tests at all sizes",
    OPT_TRUE, "R", (Address)&reverse, "Do all sizes, high-to-low",
    OPT_TRUE, "e", (Address)&doEchoes, "Echo off RPC server (default)",
    OPT_INT, "r", (Address)&numReps, "Number of reps for each size",
    OPT_TRUE, "s", (Address)&doSends, "Send instead of Echo",
    OPT_TRUE, "t", (Address)&trace, "Trace records taken (runs slower)",
#ifdef SPRITED_USERPRIORITY
    OPT_TRUE, "c", (Address)&fastCS, "High priority",
#endif
    OPT_STRING, "h", (Address)&server, "name of target host",
};
int numOptions = sizeof(optionArray) / sizeof(Option);

main(argc, argv)
    int argc;
    char *argv[];
{
    int error;
    int openFileId;
    char c;
    char *fileName;
    Time deltaTime;
    Boolean oldTrace;
    int command;
    Host_Entry *entryPtr;
    int serverID;
    int myID;
    Rpc_CltStat cltStat;

    argc = Opt_Parse(argc, argv, optionArray, numOptions);

    if (server == NULL) {
	fprintf(stderr, "Need a target host (-h hostname)\n");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }
    if (size < 0) {
	size = 0;
    } else if (size > 16384) {
	size = 16384;
    }

    entryPtr = Host_ByName(server);
    if (entryPtr == (Host_Entry *) NULL) {
	fprintf(stderr, "Unable to get host number for host '%s'\n",
		       server);
	exit(1);
    }
    serverID = entryPtr->id;

    if (doSends) {
	printf("Rpc Send Test: N = %d, Host = %s (%d),",
			numEchoes, server, serverID);
    } else {
	printf("Rpc Send Test: N = %d, Host = %s (%d),",
			numEchoes, server, serverID);
    }
    if (reverse) allSizes = TRUE;
    if (allSizes) {
	printf(" all sizes\n");
    } else {
        printf(" size = %d\n", size);
    }

#ifdef SPRITED_USERPRIORITY
    if (fastCS) {
	error = Proc_SetPriority(PROC_MY_PID, PROC_NO_INTR_PRIORITY, FALSE);
	if (error != SUCCESS) {
	    perror( "");
	}
    }
#endif

    if (trace) {
	oldTrace = TRUE;
	(void) Fs_Command(FS_SET_RPC_TRACING, sizeof(int), &oldTrace);
    }
    command = doSends ? TEST_RPC_SEND : TEST_RPC_ECHO ;

    if (!allSizes) {
	error = DoTest(command, serverID, numEchoes, size,
			inDataBuffer, outDataBuffer, &deltaTime, &cltStat);
	printf("N = %d Size = ", numEchoes);
	PrintTime(size, &deltaTime, &cltStat);
	if (error != 0) {
	    printf("status %x\n", error);
	}
    } else {
	int extra;
	error = DoTest(command, serverID, 10, 32,
			inDataBuffer, outDataBuffer, &deltaTime, &cltStat);
	if (error != SUCCESS) {
	    fprintf(stderr, "RPC test failed: %x\n", error);
	    exit(error);
	}
	if (size != DATASIZE) {
	    maxSize = size;
	}
	if (! reverse) {
	    for (size = 0; size < maxSize ; size += BYTES_PER_FRAG) {
		DoIt(command, serverID, numEchoes, size);
		for (extra = 400 ; (extra < BYTES_PER_FRAG) &&
			(size+extra < maxSize) ; extra += 400) {
		    DoIt(command, serverID, numEchoes, size+extra);
		}
	    }
	} else {
	    size = (maxSize / BYTES_PER_FRAG) * BYTES_PER_FRAG;
	    for ( ; size >= 0 ; size -= BYTES_PER_FRAG) {
		DoIt(command, serverID, numEchoes, size);
		for (extra = 250 ; (extra < BYTES_PER_FRAG) &&
			(size-extra >= 0) ; extra += 400) {
		    DoIt(command, serverID, numEchoes, size-extra);
		}
	    }
	}
    }
    if (trace) {
	(void) Fs_Command(FS_SET_RPC_TRACING, sizeof(int), &oldTrace);
    }

    exit(error);
}

DoIt(command, serverID, numEchoes, size)
    int command, serverID, numEchoes, size;
{
    Time deltaTime;
    register int i;
    ReturnStatus error;
    Rpc_CltStat cltStat;

    fprintf(stderr, "%5d bytes\n", size);
    for (i=0 ; i<numReps ; i++) {
	error = DoTest(command, serverID, numEchoes, size ,
		inDataBuffer, outDataBuffer, &deltaTime, &cltStat);
	PrintTime(size, &deltaTime, &cltStat);
	if (error != SUCCESS) {
	    perror( "RPC test failed:");
	    exit(error);
	}
    }
}

PrintTime(size, timePtr, cltStatPtr)
    int size;
    Time *timePtr;
    Rpc_CltStat *cltStatPtr;
{
    printf("%d\t%d.%06d ", size, timePtr->seconds, timePtr->microseconds);
    if (cltStatPtr->timeouts > 0) {
	printf("timeouts %d ", cltStatPtr->timeouts);
    }
    if (cltStatPtr->resends > 0) {
	printf("resends %d ", cltStatPtr->resends);
    }
    if (cltStatPtr->acks > 0) {
	printf("acks %d ", cltStatPtr->acks);
    }
    printf("\n");
    fflush(stdout);
}

ReturnStatus
DoTest(command, serverID, numEchoes, size, inDataBuffer, outDataBuffer,
	timePtr, cltStatPtr)
    int command;
    int serverID;
    int numEchoes;
    int size;
    Address inDataBuffer;
    Address outDataBuffer;
    Time *timePtr;
    Rpc_CltStat *cltStatPtr;
{
    Rpc_EchoArgs echoArgs;
    ReturnStatus error;
    Rpc_CltStat startCltStat;

    if (Sys_Stats(SYS_RPC_CLT_STATS, TRUE, &startCltStat) != SUCCESS) {
	bzero((char *)&startCltStat, sizeof(Rpc_CltStat));
    }

    echoArgs.serverID = serverID;
    echoArgs.n = numEchoes;
    echoArgs.size = size;
    echoArgs.inDataPtr = inDataBuffer;
    echoArgs.outDataPtr = outDataBuffer;
    echoArgs.deltaTimePtr = timePtr;
    error = Test_Rpc(command, &echoArgs);

    if (Sys_Stats(SYS_RPC_CLT_STATS, TRUE, cltStatPtr) != SUCCESS) {
	bzero((char *)cltStatPtr, sizeof(Rpc_CltStat));
    } else {
	register int *beforePtr, *afterPtr;
	register int wordCount = 0;

	for (wordCount = 0,
	     beforePtr = (int *)&startCltStat, afterPtr = (int *)cltStatPtr;
	     wordCount < sizeof(Rpc_CltStat) / sizeof(int) ;
	     wordCount++, beforePtr++, afterPtr++) {
	    *afterPtr = *afterPtr - *beforePtr;
	}
    }

    return(error);
}
