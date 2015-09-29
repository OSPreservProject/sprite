/* 
 * client.c --
 *
 *	The client part of a pseudo-device benchmark.
 *	The routines here interface to the server; initial contact,
 *	waiting for the start message, and notification of completion.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/benchmarks/pdevtest/RCS/client.c,v 1.2 89/10/24 12:37:46 brent Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "status.h"
#include "errno.h"
#include "stdio.h"
#include "fs.h"
#include "sys/file.h"
#include "dev/pdev.h"

extern char *pdev;
extern Boolean selectP;

static char buffer[4096];
static int bufSize = sizeof(buffer);

typedef struct ClientState {
    int clientStreamID;
} ClientState;

/*
 *----------------------------------------------------------------------
 *
 * ClientSetup --
 *
 *	Establish contact with the server.
 *
 * Results:
 *	A pointer to state about the clients needed by ClientStart and
 *	ClientDone.
 *
 * Side effects:
 *	Creates named pipes and communicates with server
 *	This exits upon error.
 *
 *----------------------------------------------------------------------
 */

void
ClientSetup(dataPtr)
    ClientData *dataPtr;
{
    ClientState *statePtr;
    ReturnStatus status;

    statePtr = (ClientState *)malloc(sizeof(ClientState));

    statePtr->clientStreamID = open(pdev, O_RDWR, 0);
    if (statePtr->clientStreamID < 0) {
	perror(pdev);
	exit(errno);
    }
    *dataPtr = (ClientData)statePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ClientRead --
 *
 *	Read from a pseudo-device.  The amount and number of repetitions
 *	can be varied for measurment.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
ClientRead(data, size, reps)
    ClientData data;
    int size;
    register int reps;
{
    ClientState *statePtr;
    int amountRead;
    register ReturnStatus status;
    register char *bufPtr;
    register int i;

    if (size > bufSize) {
	bufPtr = (char *)malloc(size);
    } else {
	bufPtr = buffer;
    }
    statePtr = (ClientState *)data;
    for (i=0 ; i<reps ; i++) {
	amountRead = size;
	if (selectP) {
	    int numReady;
	    int mask = 1 << statePtr->clientStreamID;
	    status = Fs_Select(32, NULL, &mask, NULL, NULL, &numReady);
	    if (status != SUCCESS) {
		Stat_PrintMsg(status, "ClientRead: error on select");
		break;
	    }
	}
	amountRead = read(statePtr->clientStreamID, bufPtr, size);
	if (amountRead < 0) {
	    perror("ClientRead: error on read");
	    break;
	} else if (amountRead != size) {
	    fprintf(stderr, "Read #%d was short (%d < %d)\n",
			i, amountRead, size);
	    break;
	}
	if (size > 0 && bufPtr[0] != 'z') {
	    fprintf(stderr, "Bad data returned <%c>\n", bufPtr[0]);
	}
    }
    if (bufPtr != buffer) {
	free((char *)bufPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ClientWrite --
 *
 *	Write to a pseudo-device.  The amount and number of repetitions
 *	can be varied for measurment.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
ClientWrite(data, size, reps)
    ClientData data;
    int size;
    register int reps;
{
    ClientState *statePtr;
    int amountWrite;
    register ReturnStatus status;
    register char *bufPtr;

    if (size > bufSize) {
	bufPtr = (char *)malloc(size);
    } else {
	bufPtr = buffer;
    }
    statePtr = (ClientState *)data;
    do {
	amountWrite = write(statePtr->clientStreamID, bufPtr, size);
	if (amountWrite < 0) {
	    perror("ClientWrite: error on write");
	    break;
	} else if (amountWrite != size) {
	    fprintf(stderr, "Short write %d < %d\n", amountWrite,
					size);
	}
    } while (--reps > 0);
    if (bufPtr != buffer) {
	free((char *)bufPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ClientIOControl --
 *
 *	Do an IOControl to a pseudo-device.
 *	The amount of data passed in and number of repetitions
 *	can be varied for measurment.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
ClientIOControl(data, size, reps)
    ClientData data;
    int size;
    register int reps;
{
    ClientState *statePtr;
    int amountRead;
    register ReturnStatus status;
    register char *inBuffer;
    register char *outBuffer;
    int command;

    if (size > bufSize) {
	inBuffer = (char *)malloc(size);
	outBuffer = (char *)malloc(size);
    } else {
	inBuffer = outBuffer = buffer;
    }
    statePtr = (ClientState *)data;
    do {
	extern Boolean switchBuf;
	if (switchBuf && (reps % 7) == 0) {
	    command = IOC_PDEV_SET_BUF;
	} else {
	    command = 8 << 16;	/* To avoid range of generic I/O controls */
	}
	status = Fs_IOControl(statePtr->clientStreamID, command, size, inBuffer,
				size, outBuffer);
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "ClientIOControl: error ");
	    break;
	}    
    } while (--reps > 0);
    if (inBuffer != buffer) {
	free((char *)inBuffer);
	free((char *)outBuffer);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ClientDone --
 *
 *	Tell the server we're done.  This is just done by closing
 *	the pseudo stream.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

void
ClientDone(data)
    ClientData data;
{
    ClientState *statePtr;
    register ReturnStatus status;

    statePtr = (ClientState *)data;
    if (close(statePtr->clientStreamID) < 0) {
	perror("ClientDone: error on close");
    }
    free((char *)statePtr);
}
