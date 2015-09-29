/* 
 * client.c --
 *
 *	The client part of some multi-program synchronization primatives.
 *	The routines here interface to the server; initial contact,
 *	waiting for the start message, and notification of completion.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/tests/pdevtest/RCS/client.c,v 1.1 88/04/17 10:20:46 brent Exp Locker: brent $ SPRITE (Berkeley)";
#endif not lint


#include <sprite.h>
#include <status.h>
#include <stdio.h>
#include <fs.h>
#include <sys/file.h>
#include <dev/pdev.h>
#include "pdevInt.h"

extern char *pdev;

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

    statePtr->clientStreamID = open(pdev, O_RDWR);
    if (statePtr->clientStreamID < 0) {
	perror("ClientSetup: error opening pseudo device");
	exit(status);
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
    char buffer[MAX_SIZE];
    register int i;

    statePtr = (ClientState *)data;
    if (size > MAX_SIZE) {
	size = MAX_SIZE;
    }
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
	amountRead = read(statePtr->clientStreamID, buffer, size);
	if (amountRead < 0) {
	    perror("ClientRead: error on read");
	    break;
	} else if (amountRead != size) {
	    fprintf(stderr, "Read #%d was short (%d < %d)\n",
			i, amountRead, size);
	    break;
	}
	if (size > 0 && buffer[0] != 'z') {
	    fprintf(stderr, "Bad data returned <%c>\n", buffer[0]);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ClientWrite --
 *
 *	Write from a pseudo-device.  The amount and number of repetitions
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
    char buffer[MAX_SIZE];

    statePtr = (ClientState *)data;
    if (size > MAX_SIZE) {
	size = MAX_SIZE;
    }
    do {
	amountWrite = write(statePtr->clientStreamID, buffer, size);
	if (amountWrite < 0) {
	    perror("ClientWrite: error on write");
	    break;
	} if (amountWrite != size) {
	    fprintf(stderr, "Short write %d < %d\n", amountWrite,
					size);
	}
    } while (--reps > 0);
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
    int command;
    char inBuffer[MAX_SIZE];
    char outBuffer[MAX_SIZE];

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

    statePtr = (ClientState *)data;
    close(statePtr->clientStreamID);
}
