/* 
 * migPdev.c --
 *
 *	Routines to manage pseudo-devices for the migration daemon.
 *	This is also the only file that deals with byte ordering issues.
 *
 *	Most operations are done using ioctls.  Reads and writes are used
 * 	for the following purpose:
 *
 *	- the loadavg daemons write load vectors asynchronously,
 *	- user processes can read load vectors for their own host
 *	  (synchronously), and
 *	- user processes can select on the pdev to find out about changes
 *	  in the state of idle hosts (new ones available or ones being
 *	  reclaimed).
 *
 * Copyright 1989, 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/daemons/migd/RCS/migPdev.c,v 2.1 90/07/05 13:19:38 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <string.h>
#include <errno.h>
#include <status.h>
#include "syslog.h"
#include <bstring.h>
#include <fs.h>
#include <kernel/fs.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <host.h>
#include <pdev.h>
#include <fmt.h>
#include "migd.h"
#include "migPdev.h"
#include "global.h"


static int		PdevOpen(), PdevClose(), PdevWrite();
static int		PdevRead(), PdevIoctl();

static Pdev_CallBacks	service = {
				PdevOpen,	/* open */
				PdevRead,	/* read */
				PdevWrite,	/* write */
				PdevIoctl,	/* ioctl */
				PdevClose,	/* close */
				NULL, NULL	/* get/set attr */
			};

typedef struct {
    int (*routine)();		/* routine to call */
    char *inFmt;		/* string to pass to Fmt_Convert for input */
    char *outFmt;		/* string to pass to Fmt_Convert for output */
} IoctlCallback;

    
/*
 * Define fmt conversions for the structures in mig.h.  This is only used
 * here and is separated from the structures because users need not be
 * aware of conversion issues.
 *								    *
 ********************************************************************
 *			       IMPORTANT NOTE 			    *
 ********************************************************************
 *								    *
 * Changes to the structures in mig.h must also be reflected below. *
 *								    *
 ********************************************************************
 */

/*
 * The Mig_LoadVector is imbedded in a Mig_Info.  With old compilers
 * we can't concatenate anyway.
 */
#define MIGD_LOADVEC_FMT "{w8d3}"
#define MIGD_INFO_FMT "{w12{w8d3}}"
#define MIGD_UP_IN_FMT MIGD_INFO_FMT

#define MIGD_GETINFO_IN_FMT "{w2}"
#ifdef __STDC__
#define MIGD_GETINFO_OUT_FMT "w2" MIGD_INFO_FMT "*"
#else /* __STDC__ */
#define MIGD_GETINFO_OUT_FMT "w2{w12{w8d3}}*"
#endif /* __STDC__ */


#define MIGD_GETIDLE_IN_FMT "{w4}"
#define MIGD_GETIDLE_OUT_FMT "w*"



#define MIGD_DONE_IN_FMT "w*"
#define MIGD_REMOVE_IN_FMT MIGD_DONE_IN_FMT

#define MIGD_CHANGE_IN_FMT "w"
#define MIGD_PARAMS_FMT "{w4d2}"
#define MIGD_STATS_FMT "{w12{b12w45{w20}2}8}"
#define MIGD_GET_UPDATE_OUT_FMT "w"
#define MIGD_EVICT_OUT_FMT "w"

/* 
 * When a  daemon starts up, it does an ioctl with a full Mig_Info to
 * start things off.  Then it does writes with new load vectors, and
 * ioctls to flag special cases such as eviction.
 */
#define MIGD_DAEMON_IN_FMT MIGD_INFO_FMT

static IoctlCallback ioctlCallbacks[] = {
    { Global_GetLoadInfo, 	MIGD_GETINFO_IN_FMT, MIGD_GETINFO_OUT_FMT}, 
    { Global_GetIdle, 		MIGD_GETIDLE_IN_FMT, MIGD_GETIDLE_OUT_FMT}, 
    { Global_DoneIoctl,    	MIGD_DONE_IN_FMT, (char *) NULL}, 
    { Global_RemoveHost, 	MIGD_REMOVE_IN_FMT, (char *) NULL},
    { Global_HostUp,	 	MIGD_UP_IN_FMT, (char *) NULL},
    { Global_ChangeState,	MIGD_CHANGE_IN_FMT, (char *) NULL},
    { Migd_GetParms,		(char *) NULL, MIGD_PARAMS_FMT},
    { Migd_SetParms,		MIGD_PARAMS_FMT, (char *) NULL},
    { Global_GetStats,		(char *) NULL, MIGD_STATS_FMT},
    { Global_GetUpdate,		(char *) NULL, MIGD_GET_UPDATE_OUT_FMT},
    { Migd_EvictIoctl,		(char *) NULL, MIGD_EVICT_OUT_FMT},
    { Global_ResetStats,	(char *) NULL, (char *) NULL},
};

/*
 * Pdev identifier for the master end of the pdev, when open.
 */
static Pdev_Token	pdev = (Pdev_Token) NULL;

static char *pdevName;			/* File name of the pdev. */ 

static int nextOpenID = 0;		/* Identifier for next pdev
					   open by a client. */



List_Links openStreamListHdr; 	/* List of open pdevs. */
List_Links *openStreamList; 	/* Pointer to list, used in calls. */

int migPdev_Debug = 0;




/*
 *----------------------------------------------------------------------
 *
 * MigPdev_Init --
 *
 *	Initialize data structures for this module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Open stream list is set up.
 *
 *----------------------------------------------------------------------
 */

void
MigPdev_Init()
{

    openStreamList = &openStreamListHdr;
    List_Init(openStreamList);
    if (migPdev_Debug > 5) {
	pdev_Trace = 1;
    }
}
    

/*
 *----------------------------------------------------------------------
 *
 * MigPdev_OpenMaster --
 *
 *	Set up the master end of a pseudo-device.  This can be either
 * 	the host-specific pdev or the global pdev, as specified by the
 *	global variable migd_GlobalMaster.
 *
 * Results:
 *	0 for successful completion, -1 for error.
 *
 * Side effects:
 *	Opens pseudo-device as master.
 *
 *----------------------------------------------------------------------
 */

int
MigPdev_OpenMaster()
{
    struct stat atts;
    
    if (migPdev_Debug > 1) {
	PRINT_PID;
	fprintf(stderr, "MigPdev_OpenMaster -\n");
    }

    if (migd_GlobalMaster) {
	pdevName = migd_GlobalPdevName;
    } else {
	pdevName = migd_LocalPdevName;
    }
    
    /*
     * Open pdev.  If it's the global pdev and we can't open it, then
     * presumably someone else beat us to it.  That's fine, let
     * him handle it.  In any case, punt the error to our caller.
     */
    if (migPdev_Debug > 2) {
	PRINT_PID;
	fprintf(stderr, "MigPdev_OpenMaster - Open %s\n", pdevName);
    }
#ifdef notdef
    pdev_Trace = 0;
#endif
    /*
     * If we're opening the local pdev, and we've gotten this far, we
     * know the global master recognizes us as the only daemon on this
     * host.  Therefore, ensure against leftover masters by removing the
     * pdev in case it already exists.  Note that statting the file
     * may result in an error even though it actually exists, due to
     * trying to get attributes associated with a nonexistent pdev master.
     */
    if (!migd_GlobalMaster) {
	if (migPdev_Debug > 0 && stat(pdevName, &atts) == 0) {
	    fprintf(stderr, "Warning: removing leftover pdev %s.\n",
		   pdevName);
	}
	(void) unlink(pdevName);
    }
    pdev = Pdev_Open(pdevName, NULL, 0, 0, &service, NULL);
    if (pdev == NULL) {
	if (migPdev_Debug > 0) {
	    PRINT_PID;
	    fprintf(stderr, "MigPdev_OpenMaster: %s\n", pdev_ErrorMsg);
	}
	errno = ENODEV;
	return (-1);
    }

    if (migPdev_Debug > 1) {
	PRINT_PID;
	fprintf(stderr, "MigPdev_OpenMaster - completed successfully\n");
    }

    return (0);
}


/*
 *----------------------------------------------------------------------
 *
 * MigPdev_End --
 *
 *	Terminate service of the master end of the pdev.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The pdev is removed and closed.
 *
 *----------------------------------------------------------------------
 */

void
MigPdev_End()
{
    int status;

    if (migPdev_Debug > 2) {
	PRINT_PID;
	fprintf(stderr, "MigPdev_End -\n");
    }

    Pdev_Close(pdev);
    pdev = NULL;

    status = unlink(pdevName);
    if (status == -1) {
	SYSLOG2(LOG_WARNING, "couldn't remove %s: %s\n",
	       pdevName, strerror(errno));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * MigPdev_Stat --
 *
 *	Perform an fstat on the descriptor associated with the pdev we
 * 	are controlling.  .... OBSOLETE (never used)
 *
 * Results:
 *	On error, -1 is returned and errno is set to the error returned
 *	by fstat; else 0 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

#ifdef 0 
int
MigPdev_Stat(attsPtr)
    struct stat *attsPtr;
{
    int stream = Pdev_GetStreamID(pdev);
    
    if (migPdev_Debug > 2) {
	PRINT_PID;
	fprintf(stderr, "MigPdev_Stat -\n");
    }
    return(fstat(stream, attsPtr));
}
#endif


/*
 *----------------------------------------------------------------------
 *
 * SendSignal --
 *
 *	Send a signal over a stream.  Only used for signalling other
 *	daemons upon normal master termination (for debugging).
 *
 * Results:
 *	0 is returned, since we don't want to stop partway through.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */


static int 
SendSignal(streamPtr, clientData)
    Pdev_Stream *streamPtr;	/* Pdev stream, containing streamID */
    ClientData clientData;	/* Signal to send, as a ClientData */
{
#ifdef USE_PDEV_SIGNALS
    Pdev_Signal sig;
#endif /* USE_PDEV_SIGNALS */
    int status;
    Migd_OpenStreamInfo *cltPtr =
	(Migd_OpenStreamInfo *) streamPtr->clientData;

    if (cltPtr->type != MIGD_DAEMON) {
	return(0);
    }
    if (migPdev_Debug > 3) {
	PRINT_PID;
	fprintf(stderr, "SendSignal - sending signal to process %x\n",
	       cltPtr->processID);
    }

#ifdef USE_PDEV_SIGNALS
    sig.signal = (unsigned int) clientData;
    sig.code = 0;
    status = Fs_IOControl(streamPtr->streamID, IOC_PDEV_SIGNAL_OWNER,
			sizeof(Pdev_Signal), (char *) &sig, 0, (char *) NULL);
#else /* USE_PDEV_SIGNALS */
    status = Sig_Send((unsigned int) clientData, cltPtr->processID, 0);
#endif /* USE_PDEV_SIGNALS */
    if (status != SUCCESS && migPdev_Debug > 0) {
	PRINT_PID;
	fprintf(stderr, "SendSignal: error sending signal to process %x: %s\n",
	       cltPtr->processID, Stat_GetMsg(status));
    }
    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * MigPdev_SignalClients --
 *
 *	Send a signal to all clients.
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
MigPdev_SignalClients(sig)
    int sig;			/* Signal to send. */
{
    if (migPdev_Debug > 2) {
	PRINT_PID;
	fprintf(stderr, "MigPdev_SignalClients -\n");
    }

    (void) Pdev_EnumStreams(pdev, SendSignal, (ClientData) sig);
}

/*
 *----------------------------------------------------------------------
 *
 * MigPdev_MakeReady --
 *
 *	Make a pdev stream readable, so someone selecting on it
 *	will know to do an ioctl.  The ClientData is either NULL or
 *	a function to determine whether to make this stream selectable.
 *
 * Results:
 *	If an error occurs during the ioctl, then that error is returned,
 *	else 0.
 *
 * Side effects:
 *	The stream is marked.
 *
 *----------------------------------------------------------------------
 */


int 
MigPdev_MakeReady(streamPtr, clientData)
    Pdev_Stream *streamPtr;	/* Pdev stream, containing streamID */
    ClientData clientData;	/* A function for selection, or NULL. */
{
    int (*func)();
    int status;
    Migd_OpenStreamInfo *cltPtr = (Migd_OpenStreamInfo *) streamPtr->clientData;

    func = (int (*) ()) clientData;
    if (func != (int (*) ()) NULL) {
	if (!(*func)(cltPtr)) {
	    return(0);
	}
    }
    cltPtr->defaultSelBits |= FS_READ;
    status = Fs_IOControl(streamPtr->streamID, IOC_PDEV_READY, sizeof(int),
			(char *) &cltPtr->defaultSelBits, 0, (char *) NULL);
    /*
     * Note any errors, but only return non-zero status if we had
     * no selection function (hence were notifying a particular
     * client rather than a bunch at once).
     */
    if (status != 0) {
	if (migPdev_Debug > 0) {
	    PRINT_PID;
	    fprintf(stderr, "Error making stream selectable for client %x: %s\n",
		   cltPtr->processID, Stat_GetMsg(status));
	}
	if (func != (int (*) ()) NULL) {
	    return(0);
	}
	return(status);
    }
    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * DaemonSelectPredicate --
 *
 *	Selection predicate to just get clients that are daemons.
 *
 * Results:
 *	1 if a daemon, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
DaemonSelectPredicate(cltPtr)
    Migd_OpenStreamInfo *cltPtr;
{
    if (cltPtr->type == MIGD_DAEMON) {
	return(Global_IsHostUp(cltPtr->host));
    } else {
	return(0);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MigPdev_NotifyDaemons --
 *
 *	Make migration daemons exceptable.  (To notify a particular
 *	client, use MigPdev_MakeReady directly).
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
MigPdev_NotifyDaemons()
{

    if (migPdev_Debug > 2) {
	PRINT_PID;
	fprintf(stderr, "MigPdev_NotifyDaemons -\n");
    }

    (void) Pdev_EnumStreams(pdev, MigPdev_MakeReady,
			    (ClientData) DaemonSelectPredicate);
}

/*
 *----------------------------------------------------------------------
 *
 * PdevOpen --
 *
 *	Service an open request for the pdev.
 *
 * Results:
 *	0, or an error condition.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static int
PdevOpen(data, streamPtr, buff, flags, processID, host, user, selPtr)
    ClientData data;		/* Ignored. */
    Pdev_Stream *streamPtr;	/* Information for the stream. */
    char *buff;			/* Ignored */
    int flags;			/* Flags passed to Fs_Open. */
    int processID;		/* Id of process doing open. */
    int host;			/* Host on which opened. */
    int user;			/* Userid of opener. */
    int *selPtr;		/* Bits for Fs_Select. */
{
    Migd_OpenStreamInfo *cltPtr;

    if (migPdev_Debug > 2) {
	PRINT_PID;
	fprintf(stderr, "PdevOpen - request from %d:%x\n", host, processID);
    }

    if (migd_Quit) {
	if (migPdev_Debug > 0) {
	    PRINT_PID;
	    fprintf(stderr,
		   "PdevOpen - can't handle request from %d:%x because we are shutting down.\n",
		   host, processID);
	}
	return(ESHUTDOWN);
    }
    
    /* Check valid open flags */
    flags &= FS_USER_FLAGS;
    if (flags & ~(FS_WRITE | FS_READ)) {
	if (migPdev_Debug > 0) {
	    PRINT_PID;
	    fprintf(stderr, "PdevOpen - bad flags %x\n", flags);
	}
	return(EACCES);
    }

    cltPtr = mnew(Migd_OpenStreamInfo);
    bzero((char *) cltPtr, sizeof(Migd_OpenStreamInfo));
    cltPtr->streamPtr = streamPtr;
    cltPtr->user = user;
    cltPtr->host = host;
    cltPtr->processID = processID;
    cltPtr->openID = nextOpenID;
    /*
     * By default, processes can't read or write the pdev, they must
     * use ioctls.  They can use an ioctl to register a long-term
     * interest, such as if they're a loadavg daemon or a load widget,
     * but until then the default is not to be able to select on this
     * stream.  (In addition, they can select on the stream and be
     * told it's readable if there's a change in status, but again,
     * it's unreadable by default until that happens.)  This default
     * value is used to make it easy for routines to reset the select
     * bits as appropriate.
     */
    if (migd_GlobalMaster) {
	cltPtr->defaultSelBits = 0;
	cltPtr->type = ((user == 0) && (flags & FS_WRITE)) ?
	    MIGD_DAEMON : MIGD_USER;
    } else {
	cltPtr->defaultSelBits = FS_READ;
	cltPtr->type = MIGD_LOCAL;
    }
    nextOpenID++;
    streamPtr->clientData = (ClientData) cltPtr;
    List_Init(&cltPtr->currentRequests);
    List_Init(&cltPtr->messages);
    List_InitElement(&cltPtr->nextStream);
    List_Insert((List_Links *)cltPtr, LIST_ATREAR(openStreamList));
    cltPtr->waitPtr = (Migd_WaitList *) NULL;
    
    *selPtr = cltPtr->defaultSelBits;

    if (migPdev_Debug > 3) {
	PRINT_PID;
	fprintf(stderr, "PdevOpen - returning status 0\n");
    }

    return(0);
}


/*
 *----------------------------------------------------------------------
 *
 * PdevClose --
 *
 *	Service a close request for the pdev.  Clean up state, either
 * 	marking a host as down if the daemon on that host closed its
 *	connection, or freeing hosts used by a client if it closes the
 *	connection without freeing them first.
 *
 * Results:
 *	0, or an error condition.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
PdevClose(streamPtr)
    Pdev_Stream *streamPtr;	/* Information for the stream */
{
    Migd_OpenStreamInfo *cltPtr;


    cltPtr = (Migd_OpenStreamInfo *) streamPtr->clientData;
    if (cltPtr == (Migd_OpenStreamInfo *) NULL) {
	/*
	 * Stream wasn't fully opened -- if, for example, we were quitting
	 * at the time.
	 */
	return(0);
    }

    if (migPdev_Debug > 3) {
	PRINT_PID;
	fprintf(stderr, "PdevClose - pid %x\n", cltPtr->processID);
    }

    switch (cltPtr->type) {
	case MIGD_DAEMON: {
	    if (migPdev_Debug > 1) {
		PRINT_PID;
		fprintf(stderr, "PdevClose - daemon %x on host %d exited\n",
		       cltPtr->processID, cltPtr->host);
	    }
	    (void) Global_HostDown(cltPtr->host, 1);
	    break;
	}
	case MIGD_USER: {
	    if (migPdev_Debug > 2) {
		PRINT_PID;
		fprintf(stderr, "PdevClose - process closed pdev\n");
	    }
	    if (migd_GlobalMaster) {
		/*
		 * Clean up after the process, by releasing hosts and
		 * recording statistics.
		 */
		(void) Global_Done(cltPtr, MIG_ALL_HOSTS, 1);
	    }
	    break;
	}
	case MIGD_CLOSED: 
	case MIGD_NEW: {
	    if (migPdev_Debug > 0) {
		PRINT_PID;
		fprintf(stderr,
		       "PdevClose - never heard from process %x, or connection is unusable.\n",
		       cltPtr->processID);
	    }
	    break;
	}
	case MIGD_LOCAL: {
	    if (migPdev_Debug > 2) {
		PRINT_PID;
		fprintf(stderr,
		       "PdevClose - local process %x exited.\n",
		       cltPtr->processID);
	    }
	    break;
	default: {
	    PRINT_PID;
	    fprintf(stderr,
		    "PdevClose - bad type (%d) for client type.\n",
		    (int) cltPtr->type);
	    }
	    break;
	}
    }
    
    List_Remove((List_Links *)cltPtr);
    free((Address) cltPtr);
    return (0);
}


/*
 *----------------------------------------------------------------------
 *
 * PdevRead --
 *
 *	Service a read request for the pdev.  This is permitted when a
 *	process wants the load vector for its own host, and talks to
 *	its daemon directly.  The master daemon doesn't service read
 *	requests; requests for load go through the ioctl interface, which
 *	also deals with byte swapping.  Note: we don't check the hostID
 * 	of the requester, since after all the hostID could be a physical
 *	host that's different from us but the same byte order, but
 *	if someone on a different architecture tries to read our structure
 *	they may get garbage.
 * 
 * Results:
 *	0 for success, EINVAL is returned if we're not the local daemon
 *	or the read isn't at least the size of a Mig_Info structure.  
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/* ARGSUSED */
static int
PdevRead(streamPtr, readPtr, freeItPtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Service stream the client specified in its
				 * kernel call. */
    Pdev_RWParam *readPtr;	/* Read parameter block.  Indicates size,
				 * buffer, plus various IDs */
    Boolean *freeItPtr;		/* Not used here. */
    int *selectBitsPtr;		/* Store new select state of stream here. */
    Pdev_Signal *sigPtr;	/* Signal information for returning signal. */
{
    Migd_OpenStreamInfo *cltPtr;

    cltPtr = (Migd_OpenStreamInfo *) streamPtr->clientData;
    *selectBitsPtr = cltPtr->defaultSelBits;

    if (migPdev_Debug > 3) {
	PRINT_PID;
	fprintf(stderr, "PdevRead - request from %d:%x\n",
	       cltPtr->host, cltPtr->processID);
    }
    if (migd_Quit) {
	sigPtr->signal = SIG_TERM;
    }
    if (migd_GlobalMaster) {
	/*
	 * Wrong interface to communicate with the global master.
	 */
	return (EINVAL);
    }
    if (readPtr->length < sizeof(Mig_Info)) {
	if (migPdev_Debug > 0) {
	    PRINT_PID;
	    fprintf(stderr, "PdevRead - short read from %d:%x\n",
		   cltPtr->host, cltPtr->processID);
	}
	return(EINVAL);
    }

    Migd_GetLocalLoad(readPtr->buffer);
    readPtr->length = sizeof(Mig_Info);
    return (0);
}


/*
 *----------------------------------------------------------------------
 *
 * PdevWrite --
 *
 *	Service a write request for the pdev.  Write requests can be from
 * 	daemons, telling us a new load vector, or from clients, requesting
 *	or returning one or more hosts.
 *
 * Results:
 *	An error status, or 0 for success, is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static int
PdevWrite(streamPtr, async, writePtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Service stream the client specified in its
				 * kernel call. */
    int async;			/* Non-zero means this is an asynchronous
				 * write request. */
    Pdev_RWParam *writePtr;	/* Write parameter block.  Indicates size,
				 * offset, and buffer, among other things */
    int *selectBitsPtr;		/* Just set to default. */
    Pdev_Signal *sigPtr;	/* Signal to return, if any */
{
    Migd_OpenStreamInfo *cltPtr;
    Mig_LoadVector loadVec;
    Mig_LoadVector *loadPtr;
    int status;
    int inBufSize;
    int outBufSize;

    cltPtr = (Migd_OpenStreamInfo *) streamPtr->clientData;

    if (migPdev_Debug > 3) {
	PRINT_PID;
	fprintf(stderr, "PdevWrite - packet from %d:%x\n", cltPtr->host,
	       cltPtr->processID);
    }

    if (migd_Quit) {
	sigPtr->signal = SIG_TERM;
    }

    *selectBitsPtr = cltPtr->defaultSelBits;

    /*
     * Process packet.
     */
    if (cltPtr->type != MIGD_DAEMON) {
	if (migPdev_Debug > 1) {
	    PRINT_PID;
	    fprintf(stderr, "PdevWrite - got invalid write from %d:%x\n",
		   cltPtr->host, cltPtr->processID);
	}
	return(EPERM);
    }
    if (writePtr->length < sizeof(Mig_LoadVector)) {
	if (migPdev_Debug > 0) {
	    SYSLOG3(LOG_WARNING,
		    "PdevWrite - short write from process %x: %d bytes of %d\n",
		    cltPtr->processID, writePtr->length,
		    sizeof(Mig_LoadVector));
	}
	return(EINVAL);
    }
    if (cltPtr->format != FMT_MY_FORMAT) {
	inBufSize = writePtr->length;
	outBufSize = sizeof(Mig_LoadVector);
	status = Fmt_Convert(MIGD_LOADVEC_FMT,
			     cltPtr->format,
			     &inBufSize,
			     writePtr->buffer,
			     FMT_MY_FORMAT,
			     &outBufSize,
			     (char *) &loadVec);
	if (status != FMT_OK || inBufSize != writePtr->length ||
	    outBufSize != sizeof(Mig_LoadVector)) {
	    PRINT_PID;
	    fprintf(stderr,
		   "PdevWrite - error swapping Mig_LoadVector structure, status = %d, swapped %d->%d of %d.\n",
		   status, inBufSize, outBufSize, sizeof(Mig_LoadVector));
	    return(EINVAL);
	}
	loadPtr = &loadVec;
    } else {
	loadPtr = (Mig_LoadVector *) writePtr->buffer;
    }
    status = Global_UpdateLoad(cltPtr, loadPtr);
    if (status == 0 && !async) {
	writePtr->length = sizeof(Mig_LoadVector);
    }
    
    return(status);
}


/*
 *----------------------------------------------------------------------
 *
 * PdevIoctl --
 *
 *	Basic entry point for communications with a server.  This
 *	routine handles byte-swapping, but the real work is done
 *	elsewhere.
 *
 * Results:
 *	An error status, or 0 for success, is returned.
 *
 * Side effects:
 *	Variable.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
static int
PdevIoctl(streamPtr, ioctlPtr, selectBitsPtr, sigPtr)
    Pdev_Stream *streamPtr;	/* Service stream the client specified in its
				 * kernel call. */
    Pdev_IOCParam *ioctlPtr;	/* I/O control parameters */
    int *selectBitsPtr;		/* Store new select state here. */
    Pdev_Signal *sigPtr;	/* Returned signal, if any */
{
    Migd_OpenStreamInfo *cltPtr;
    int status;
    Address inBuffer;
    Address outBuffer;
    int bufSize;		/* Temporary holder for buffer size passed
				   to Fmt_Convert. */
    int inBufSize;		/* Size of the real input buffer. */
    int outBufSize;		/* Size of the real output buffer. */
    int command;
    char *swapBuffer = NULL;
    IoctlCallback *callbackPtr;

    cltPtr = (Migd_OpenStreamInfo *) streamPtr->clientData;
    command = ioctlPtr->command;

    if (migPdev_Debug > 3) {
	PRINT_PID;
	fprintf(stderr, "PdevIoctl - command %d from %x\n", command,
	       cltPtr->processID);
    }

    /*
     * Set the selectBits ptr up front in case we return an error early on.
     */
    *selectBitsPtr = cltPtr->defaultSelBits;
    if (migd_Quit) {
	sigPtr->signal = SIG_TERM;
    }

    outBufSize = ioctlPtr->outBufSize;
    inBufSize = ioctlPtr->inBufSize;

    if (command == IOC_SET_OWNER) {
	return(0);
    }
    
    if (command <= IOC_GENERIC_LIMIT ||
	command > IOC_MIG_LASTCMD) {
	if (migPdev_Debug > 2) {
	    SYSLOG2(LOG_WARNING, "undefined ioctl from %x: %d\n",
		    cltPtr->processID, command);
	}
	return(EINVAL);
    }

	

    callbackPtr = &ioctlCallbacks[command - IOC_GENERIC_LIMIT - 1];

    /*
     * We need to swap if we're different byte orders.  We swap right now if
     * this ioctl takes input (inFmt is non-null).  We allocate extra
     * space for swapping later if outFmt is non-null. We save the format away
     * for use in read/write calls.
     */
    cltPtr->format = ioctlPtr->format;
    if (ioctlPtr->format != FMT_MY_FORMAT) {
	if (callbackPtr->inFmt != (char *) NULL) {
	    bufSize = inBufSize;
	    swapBuffer = Malloc(inBufSize);
	    status = Fmt_Convert(callbackPtr->inFmt,
				 ioctlPtr->format,
				 &inBufSize,
				 ioctlPtr->inBuffer,
				 FMT_MY_FORMAT,
				 &bufSize,
				 swapBuffer);
	    if (status != FMT_OK || inBufSize != ioctlPtr->inBufSize) {
		SYSLOG3(LOG_ERR,
		       "PdevIoctl - error swapping ioctl input buffer, status = %d, swapped %d of %d.\n",
		       status, inBufSize, ioctlPtr->inBufSize);
		free(swapBuffer);
		return(EINVAL);
	    }
	    inBuffer = swapBuffer;
	    inBufSize = bufSize;
	} else {
	    inBuffer = ioctlPtr->inBuffer;
	}
	if (callbackPtr->outFmt != (char *) NULL) {
	    outBuffer = Malloc(outBufSize);
	} else {
	    outBuffer = ioctlPtr->outBuffer;
	}
    } else {
	inBuffer = ioctlPtr->inBuffer;
	outBuffer = ioctlPtr->outBuffer;
    }
    

    status = (*callbackPtr->routine)
	(cltPtr, ioctlPtr->command, inBuffer, inBufSize, outBuffer,
	 &outBufSize);

#ifdef lint
    status = Global_GetLoadInfo(cltPtr, ioctlPtr->command, inBuffer,
				inBufSize, outBuffer, &outBufSize);
    status = Global_GetIdle(cltPtr, ioctlPtr->command, inBuffer, inBufSize,
			    outBuffer, &outBufSize);
    status = Global_DoneIoctl(cltPtr, ioctlPtr->command, inBuffer, inBufSize,
				 outBuffer, &outBufSize);
    status = Global_RemoveHost(cltPtr, ioctlPtr->command, inBuffer, inBufSize,
			       outBuffer, &outBufSize);
    status = Global_HostUp(cltPtr, ioctlPtr->command, inBuffer, inBufSize,
			       outBuffer, &outBufSize);
    status = Global_ChangeState(cltPtr, ioctlPtr->command, inBuffer, inBufSize,
			       outBuffer, &outBufSize);
    status = Global_GetStats(cltPtr, ioctlPtr->command, inBuffer, inBufSize,
			       outBuffer, &outBufSize);
#endif /* lint */

    if (status == 0) {
	/*
	 * This time we need to swap if we're different byte orders and if
	 * this ioctl returns output (outFmt is non-null).
	 */
	if (ioctlPtr->format != FMT_MY_FORMAT &&
	    callbackPtr->outFmt != (char *) NULL) {
	    int origSize;
	    
	    bufSize = outBufSize;
	    origSize = outBufSize;
	    status = Fmt_Convert(callbackPtr->outFmt,
				 FMT_MY_FORMAT,
				 &bufSize,
				 outBuffer,
				 ioctlPtr->format,
				 &outBufSize,
				 ioctlPtr->outBuffer);
	    if (status != FMT_OK || bufSize != origSize) {
		SYSLOG3(LOG_ERR,
		       "PdevIoctl - error swapping ioctl output buffer, status = %d, swapped %d of %d.\n",
		       status, bufSize, origSize);
		status = EINVAL;
	    }
	}
    }
    /*
     * Free up extra buffers.
     */
    if (inBuffer != ioctlPtr->inBuffer) {
	free(inBuffer);
    }
    if (outBuffer != ioctlPtr->outBuffer) {
	free(outBuffer);
    }

    /*
     * Reset the selectBits ptr based on current value.
     */
    *selectBitsPtr = cltPtr->defaultSelBits;

    /*
     * Return a unix status since that's what pdev expects.  It will
     * convert to a sprite value if needed.
     */
    return (status);
}


/*
 *----------------------------------------------------------------------
 *
 * MigPdev_CloseAllStreams --
 *
 *	Close all open client streams, and the stream for any existing
 *	pdev master.  This occurs when we fork a process
 * 	to become the new global master: the child closes its streams.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory for the client records is freed.
 *
 *----------------------------------------------------------------------
 */

void
MigPdev_CloseAllStreams()
{
#ifdef notdef
    List_Links *listPtr;
    Migd_OpenStreamInfo *cltPtr;

    while (!List_IsEmpty(openStreamList)) {
	listPtr = List_First(openStreamList);
	List_Remove(listPtr);
	cltPtr = (Migd_OpenStreamInfo *) listPtr;
	close(cltPtr->streamPtr->streamID);
	free(cltPtr);
    }
#endif
    if (pdev != (Pdev_Token) NULL) {
	Pdev_Close(pdev);
    }
}
