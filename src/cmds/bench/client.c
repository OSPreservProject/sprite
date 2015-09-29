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
static char rcsid[] = "$Header: /a/newcmds/devbench/RCS/client.c,v 1.6 89/01/04 15:36:27 david Exp $ SPRITE (Berkeley)";
#endif not lint


#include "sprite.h"
#include "status.h"
#include "sys/ioctl.h"
#include "sys/file.h"
#include "stdio.h"

extern char *pdev;
extern int errno;

char buffer[4096];
int bufSize = sizeof(buffer);

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
	Stat_PrintMsg(errno, "ClientSetup: error opening pseudo device");
	fflush(stderr);
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
    int reps;
{
    ClientState *statePtr;
    int amountRead;
    ReturnStatus status;
    char *buffer = (char *)malloc(size);

    statePtr = (ClientState *)data;
    do {
	amountRead = read(statePtr->clientStreamID, buffer, size);
	if (amountRead < 0) {
	    Stat_PrintMsg(errno, "ClientRead: error on read");
	    break;
	} if (amountRead != size) {
	    fprintf(stderr, "Short read %d < %d\n", amountRead, size);
	}
    } while (--reps > 0);
    free(buffer);
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
    int reps;
{
    ClientState *statePtr;
    int amountWrite;
    ReturnStatus status;
    char *buffer = (char *)malloc(size);

    statePtr = (ClientState *)data;
    do {
        amountWrite = write(statePtr->clientStreamID, buffer, size);
	if (amountWrite < 0) {
	    Stat_PrintMsg(errno, "ClientWrite: error on read");
	    break;
	} if (amountWrite != size) {
	    fprintf(stderr, "Short write %d < %d\n", amountWrite,
					size);
	}
    } while (--reps > 0);
    free(buffer);
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
    int reps;
{
    ClientState *statePtr;
    ReturnStatus status;
    Address inBuffer = (Address)malloc(size);
    Address outBuffer = (Address)malloc(size);
    int foo = 27;

    statePtr = (ClientState *)data;
    do {
	/*
	status = Fs_IOControl(statePtr->clientStreamID, foo, size, inBuffer,
				size, outBuffer);
	*/
	if (status != SUCCESS) {
	    Stat_PrintMsg(status, "ClientIOControl: error ");
	    break;
	}    } while (--reps > 0);
    free(inBuffer);
    free(outBuffer);
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
    ReturnStatus status;

    statePtr = (ClientState *)data;
    status = close(statePtr->clientStreamID);
    if (status != SUCCESS) {
	Stat_PrintMsg(status, "ClientDone: error on close");
    }
}
