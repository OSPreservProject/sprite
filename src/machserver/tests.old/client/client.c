/* 
 * client.c --
 *
 *	Test program for printf server.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user6/kupfer/spriteserver/src/client/RCS/client.c,v 1.6 91/08/30 16:06:00 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <ctype.h>
#include <mach.h>
#include <mach_error.h>
#include <status.h>
#include <stdio.h>
#include <user/proc.h>
#include "spriteSrv.h"

#define SHARED_ERROR_REGION	1

mach_port_t serverPort;		/* port for making Sprite requests */

/* Forward references */

static int GetLength _ARGS_((char *fileName));
static void MakeFile _ARGS_((char *fileName));
static void MapFile _ARGS_((char *fileName, boolean_t readOnly,
			    int length, Address *startAddrPtr));
static void PrintBuffer _ARGS_((char *fileName, char *bufPtr, int length));
static void WriteToBuffer _ARGS_((char *fileName, char *bufPtr, int length));

main()
{
    kern_return_t kernStatus;
#ifdef SHARED_ERROR_REGION
    int *errorPtr = (int *)PROC_SHARED_REGION_START;
#endif
    char *fromName = "testInput"; /* name of file to copy from */
    char *fromBuffer;		/* mapped "from" file */
    char *toName = "testOutput"; /* name of file to copy to */
    char *toBuffer;		/* mapped "to" file */
    int fileLength;

    kernStatus = task_get_bootstrap_port(mach_task_self(), &serverPort);
    if (kernStatus != KERN_SUCCESS) {
#if SHARED_ERROR_REGION
	*errorPtr = kernStatus;
#endif
	thread_suspend(mach_thread_self());	
    }

    fileLength = GetLength(fromName);
#if 0
    if (fileLength < 0) {
	Test_PutMessage(serverPort, "bailing out.\n");
	goto bailOut;
    }
#endif

    MapFile(fromName, TRUE, fileLength, &fromBuffer);
    MapFile(toName, FALSE, fileLength, &toBuffer);

    if (fromBuffer != 0 && toBuffer != 0) {
	bcopy(fromBuffer, toBuffer, fileLength);
    }

 bailOut:
    Sys_Shutdown(serverPort);
}


/*
 *----------------------------------------------------------------------
 *
 * MapFile --
 *
 *	Map the named file into our address space.
 *
 * Results:
 *	Fills in the starting location, which is set to 0 
 *	if there was a problem.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
MapFile(fileName, readOnly, length, startAddrPtr)
    char *fileName;		/* name of file to map */
    boolean_t readOnly;		/* map read-only or read-write? */
    int length;			/* number of bytes to map */
    Address *startAddrPtr;	/* OUT: where the file was mapped to */
{
    kern_return_t kernStatus;
    ReturnStatus status;

    kernStatus = Vm_MapFileStub(serverPort, fileName, strlen(fileName)+1,
			    readOnly, 0, length, &status, startAddrPtr);
    if (kernStatus != KERN_SUCCESS) {
	Test_PutMessage(serverPort, "Couldn't map file: ");
	Test_PutMessage(serverPort, mach_error_string(kernStatus));
	Test_PutMessage(serverPort, "\n");
	*startAddrPtr = 0;
    } else if (status != SUCCESS) {
	Test_PutMessage(serverPort, "Couldn't map file: ");
	Test_PutMessage(serverPort, Stat_GetMsg(status));
	Test_PutMessage(serverPort, "\n");
	*startAddrPtr = 0;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * GetLength --
 *
 *	Get the length of a file.
 *
 * Results:
 *	Returns the length of the file, in bytes.  Returns -1 if there 
 *	was an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetLength(fileName)
    char *fileName;
{
    ReturnStatus status;
    int length;
    kern_return_t kernStatus;

    kernStatus = TempFs_LengthStub(serverPort, fileName, strlen(fileName)+1,
				   &status, &length);
    if (kernStatus != KERN_SUCCESS) {
	Test_PutMessage(serverPort, "Couldn't get file length: ");
	Test_PutMessage(serverPort, mach_error_string(kernStatus));
	Test_PutMessage(serverPort, "\n");
	return -1;
    }
    if (status != SUCCESS) {
	Test_PutMessage(serverPort, "Couldn't get file length: ");
	Test_PutMessage(serverPort, Stat_GetMsg(status));
	Test_PutMessage(serverPort, "\n");
	return -1;
    }

    return length;
}
