/* 
 * jmgr.c --
 *
 *	Jukebox manager for the Jaquith archive system.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Quote:
 *      "My obsession: a life that shrivels up, slowly rots, goes soft as pulp.
 *      This worry about decline grabs me by the throat as I awake.  In the
 *      brief interval between dream and waking, it flaunts before my eyes
 *      the frenzied dance of everything I would have liked to do, and never
 *      will. As I turn over and over in my bed, the fear of the too late, of
 *      the irreversible, propels me to the mirror to shave and get ready for
 *      the day.  And that is the moment of truth.  The moment of the old
 *      questions.  What am I today? Am I capable of renewel? What are the
 *      chances I might still produce something I do not expect of myself?
 *      For my life unfolds mainly in the yet to come, and is based on waiting.
 *      Mine is a life of preparation.  I enjoy the present only insofar as it
 *      is a promise of the future.  I am looking for the Promised Land and
 *      listening to the music of my tomorrows.  My food is anticipation.
 *      My drug is hope.''
 *      -- Francois Jacob, ``The Statue Within: An Autobiography''
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jmgr.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"
#include "jmgrInt.h"

Parms parms = {
    DEF_DETAIL,
    DEF_MGRLOG,
    DEF_MGRPORT,
    DEF_DEVFILE,
    DEF_VOLFILE,
    DEF_ROBOT,
    DEF_SKIPLABEL
};

Option optionArray[] = {
    {OPT_INT, "logdetail", (char *)&parms.logDetail, "set logging detail (1 2 4 8)"},
    {OPT_STRING, "logfile", (char *)&parms.logFile, "enable logging to file"},
    {OPT_INT, "port", (char *)&parms.port, "port to listen on"},
    {OPT_STRING, "devfile", (char *)&parms.devFile, "Device configuration file"},
    {OPT_STRING, "volfile", (char *)&parms.volFile, "Volume configuration file"},
    {OPT_STRING, "robot", (char *)&parms.robot, "Name of robot device"},
    {OPT_TRUE, "reset", (char *)&parms.reset, "Reset device"},
    {OPT_TRUE, "skiplabel", (char *)&parms.skipLabel, "Don't check volume label before use"}
};
int numOptions = sizeof(optionArray) / sizeof(Option);

/* File globals. */
int syserr = 0;               /* Our personal record of errno */
int jDebug;
static char printBuf[T_MAXSTRINGLEN]; /* utility sprintf buffer */
static SClient *clientList[MAXCLIENT];/* current-client array */
static int clientMax = MAXCLIENT; /* size of client array */
static SDev *devList;         /* our device list and... */
static int deviceMax;         /* ...sizeof same */
static Q_Handle *waitQ;       /* clients waiting for a volume */
static int *volList;          /* list of known volumes */
static Hash_Handle *volTab;   /* hash table of known volumes */
static int robotStream;       /* I/O channel of robot arm */
static FILE *memDbg;          /* stream for memory tracing */

/* The functions to be found here */
static void     CheckParms      _ARGS_ ((Parms *parmsPtr));
static SDev    *InitDevices     _ARGS_ ((char *filePtr));
static int     *InitVolumes     _ARGS_ ((char *filePtr,
					 Hash_Handle **hashTabPtr));
static void     PerformLoop     _ARGS_ ((int sock));
static int      ReadMsg         _ARGS_ ((SClient *clientPtr));
static int      AddClient       _ARGS_ ((int sock));
static void     DelClient       _ARGS_ ((SClient *clientPtr));
static SClient *FindClientBySocket _ARGS_ ((int sock));
static int      CheckAuth       _ARGS_ ((AuthHandle handle));
static int      ProcessCmd      _ARGS_ ((SClient *clientPtr));
static SDev    *FindFreeDevice  _ARGS_ ((int volId));
static void     AssignDevice    _ARGS_ ((SClient *clientPtr,
					 SDev *devPtr));
static void     ReleaseDevice   _ARGS_ ((SClient *clientPtr));
static int      SearchWaitQProc _ARGS_ ((Q_Handle *qPtr,
					 Q_ClientData datum,
					 int *retCode, int *callVal));
static int      LoadVolume      _ARGS_ ((SDev *devPtr, int volId));
static int      UnloadVolume    _ARGS_ ((SDev *devPtr));
static int      AllocateVolume  _ARGS_ ((char *archive, int *volIdPtr));
static int      ParseVolFile    _ARGS_ ((char *buf, int *volIDptr,
					 int *locationPtr));
static int      SendDevStatus   _ARGS_ ((int sock));
static int      SendQStatusProc _ARGS_ ((Q_Handle *qPtr,
					 Q_ClientData *datum,
					 int *retCodePtr, int *callVal));
static void     SigIntHandler   _ARGS_ ((int sig));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Jmgr is the physical manager for the archive device.
 *
 * The game plan:
 *      Setup an array of descriptors for the devices we're managing.
 *      Read in the initial mapping of volumes to devices.
 *      Start accepting locking/unlocking requests for a particular volume.
 *      If we can satisfy the request on a free device, do so.
 *      Otherwise, enqueue the requestor.
 *      When a device is released, the policy routine is run to
 *      select the next client.  Currently this is FIFO.
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
main(argc, argv)
    int argc;                 /* main command line argument count */
    char *argv[];             /* main command line arguments */
{
    int earSocket;
    int i;

/*
    memDbg = fopen("jmgr.mem","w");
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);
*/
    for (i=0; i<clientMax; i++) {
	clientList[i] = (SClient *)NULL;
    }

    if (signal(SIGINT, SigIntHandler) == (void(*)()) -1) {
	Utils_Bailout("signal", BAIL_PERROR);
    }

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckParms(&parms);
    
    if (Dev_InitRobot(parms.robot, &robotStream) != T_SUCCESS) {
	sprintf(printBuf, "Couldn't init robot %s: errno = %d",
		parms.robot, syserr);
	Log_Event("Main", printBuf, LOG_FAIL);
	exit(-1);
    }

    waitQ = Q_Create("waitq", 0);

    devList = InitDevices(parms.devFile);

    volList = InitVolumes(parms.volFile, &volTab);

    earSocket = Sock_SetupEarSocket(&parms.port);

    Dev_DisplayMsg(robotStream, ACTIVE_MSG, MSG_STEADY);

    PerformLoop(earSocket); 

    exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * InitDevices --
 *
 *	Initialize device table
 *
 * Results:
 *	ptr to device table.
 *
 * Side effects:
 *	Allocates array of SDev and updates global variable deviceMax
 *
 * See sample config file "devconfig" for data layout
 *
 *----------------------------------------------------------------------
 */

static SDev *
InitDevices(devFile)
    char *devFile;            /* name of device configuration file */
{
    int i;
    FILE *configFile;
    SDev *tab;
    int devCnt = 0;
    int devStream;
    int volId;
    char volIdStr[T_MAXLINELEN];
    DevConfig *itemPtr;
    DevConfig *devList;

    Admin_ReadDevConfig(devFile, NULL, &devCnt);
    devList = (DevConfig *)MEM_ALLOC("InitDevices",
				     devCnt*sizeof(DevConfig));
    if (Admin_ReadDevConfig(devFile, devList, &devCnt) != T_SUCCESS) {
	sprintf(printBuf,"Can't read %s. errno %d\n", devFile, syserr);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    tab = (SDev *)MEM_ALLOC("InitDevices", devCnt*sizeof(SDev));

    for (i=0,itemPtr=devList; i<devCnt; i++,itemPtr++) {
	tab[i].skipCnt = 0;
	tab[i].volId = NOVOLUME;
	tab[i].clientPtr = (SClient *)NULL;
	tab[i].location = itemPtr->location;
	tab[i].name = Str_Dup(itemPtr->name);
	/* Can't know what is in drive so ask */
	sprintf(printBuf, "What volume is mounted in device %s? (-1 == none) ",
		tab[i].name);
	tab[i].volId = Utils_GetInteger(printBuf, -1, INT_MAX);
    }

    MEM_FREE("InitDevices", devList);
    deviceMax = devCnt; /* ugly global value */

    return (tab);
    
}



/*
 *----------------------------------------------------------------------
 *
 * InitVolumes --
 *
 *	Initialize volume map
 *
 * Results:
 *	ptr to volume map.
 *
 * Side effects:
 *	Allocates array of int
 *
 * See sample config file "vconfig" for data layout.
 * This file should be built by a utility that invokes
 * the barcode reader on the jukebox.
 *
 *----------------------------------------------------------------------
 */

static int *
InitVolumes(volFile, hashPtrPtr)
    char *volFile;            /* name of volume configuration file */
    Hash_Handle **hashPtrPtr; /* receiving hash table ptr */
{
    VolConfig *volList;
    VolConfig *itemPtr;
    int volCnt = -1;
    Hash_Handle *hashPtr;
    Hash_Handle *tmpHashPtr;
    int i;

    Admin_ReadVolConfig(volFile, NULL, &volCnt);
    volList = (VolConfig *)MEM_ALLOC("InitVolumes",
				     volCnt*sizeof(VolConfig));
    if (Admin_ReadVolConfig(volFile, volList, &volCnt) != T_SUCCESS) {
	sprintf(printBuf,"Can't read %s. errno %d\n", volFile, syserr);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if ((hashPtr=Hash_Create("vol", 2*volCnt+1, Utils_StringHashProc, 0))
	== NULL) {
	Utils_Bailout("Couldn't create hash table1 for volume info\n",
		      BAIL_PRINT);
    }
    if ((tmpHashPtr=Hash_Create("loc", 2*volCnt+1, Utils_StringHashProc, 0))
	== NULL) {
	Utils_Bailout("Couldn't create hash table2 for volume info\n",
		      BAIL_PRINT);
    }

    for (i=0,itemPtr=volList; i<volCnt; i++,itemPtr++) {
	if (Hash_Insert(hashPtr, (char *)(&itemPtr->volId),
			sizeof(itemPtr->volId),
			(Hash_ClientData)itemPtr) != T_SUCCESS) {
	    sprintf(printBuf, "Duplicate volume '%d' in file %s?",
		    itemPtr->volId, volFile);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	}
	if (Hash_Insert(tmpHashPtr, (char *)(&itemPtr->location),
			sizeof(itemPtr->location),
			(Hash_ClientData) 1) != T_SUCCESS) {
	    sprintf(printBuf, "Duplicate location '%d' in file %s?",
		    itemPtr->location, volFile);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	}
    }

    Hash_Destroy(tmpHashPtr);
    *hashPtrPtr = hashPtr;

    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * CheckParms --
 *
 *	Validate parameters
 *
 * Results:
 *	returns ptr to primary info block.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CheckParms(parmsPtr)
    Parms *parmsPtr;          /* pointer to parameter block */
{

    if ((parmsPtr->port <= 0) || (parmsPtr->port > MAXPORT)) {
	Utils_Bailout("Checkparms: bad port number.\n", BAIL_PRINT);
    }
    if (parmsPtr->logFile != (char *)NULL) {
	Log_Open(parmsPtr->logFile);
	Log_SetDetail(parmsPtr->logDetail);
    }
    if (Utils_CheckName(parmsPtr->devFile, 1) != T_SUCCESS) {
	Utils_Bailout("CheckParms: bad device file path.\n", BAIL_PRINT);
    }
    sprintf(printBuf,"Start. (port %d)\n", parmsPtr->port);
    Log_Event("CheckParms", printBuf, LOG_MAJOR);

}


/*
 *----------------------------------------------------------------------
 *
 * PerformLoop --
 *
 *	Main command loop. We accept locking and unlocking requests
 *      for the devices we're managing.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Accepts connections from clients and handles command
 * messages to and from them.
 *
 * Note:
 *      The client holds the socket during the entire period from
 * request to release so this limits our concurrency.
 *
 *----------------------------------------------------------------------
 */

static void
PerformLoop(earSocket)
    int earSocket;            /* socket Number */
{
    int i;
    int numReady;
    int retCode;
    int newSocket;
    fd_set readSet;
    fd_set copySet;
    SClient *clientPtr;

    FD_ZERO(&readSet);
    FD_SET(earSocket,&readSet);

    while(1) {
	copySet = readSet;
	numReady = select(FD_SETSIZE, &copySet, (fd_set *)NULL,
			  (fd_set *)NULL, (struct timeval *)NULL);
	if (numReady == -1) {
	    continue;	    /* shouldn't happen */
	}
	for (i=0; i<FD_SETSIZE; i++) {
	    if ((i == earSocket) && (FD_ISSET(i, &copySet))) {
		newSocket = AddClient(earSocket);
		FD_SET(newSocket, &readSet);
	    } else if (FD_ISSET(i, &copySet)) {
		clientPtr = FindClientBySocket(i);
		retCode = ReadMsg(clientPtr);
		if (retCode == T_SUCCESS) {
		    retCode = ProcessCmd(clientPtr);
		}
		if (retCode != T_ACTIVE) {
		    FD_CLR(i, &readSet);
		    ReleaseDevice(clientPtr);
		    DelClient(clientPtr);
		}
	    }
	}
	MEM_REPORT("PerformLoop", ALLROUTINES, SORTBYOWNER);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * ReadMsg --
 *
 *	Super simplistic msg reading. I haven't defined an complete
 * message format. We're just expecting a userid followed by a
 * cmd word (S_CMDLOCK, S_CMDFREE, etc.), and a volume number. 
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * 
 *----------------------------------------------------------------------
 */

static int
ReadMsg(clientPtr)
    SClient *clientPtr;       /* ptr to client */
{
    int sock = clientPtr->socket;

    if (Sock_ReadString(sock, &clientPtr->userName, 1) != T_SUCCESS) {
	Log_Event("ReadMsg", "Couldn't read client name, client died.\n",
		  LOG_FAIL);
	clientPtr->retCode = T_IOFAILED;
	return T_FAILURE;
    }

    if (Sock_ReadInteger(sock, &clientPtr->cmd) != T_SUCCESS) {
	Log_Event("ReadMsg", "Couldn't read command, client died.\n",
		  LOG_FAIL);
	clientPtr->retCode = T_IOFAILED;
	return T_FAILURE;
    }

    /* this may be junk for some command types */
    if (Sock_ReadInteger(sock, &clientPtr->volId) != T_SUCCESS) {
	Log_Event("ReadMsg", "Couldn't read volume id\n", LOG_FAIL);
	clientPtr->retCode = T_IOFAILED;
	return T_FAILURE;
    }

    return T_SUCCESS;

}


/*
 *----------------------------------------------------------------------
 *
 * AddClient --
 *
 *	Accept an incoming connection request.
 *
 * Results:
 *	Returns updated readSet and descriptor count for select.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
AddClient(earSocket)
    int earSocket;            /* socket number */
{
    int newSocket;
    struct sockaddr_in newName;
    int nameLen = sizeof(struct sockaddr_in);
    struct hostent *peerInfo;
    SClient *clientPtr;
    int hostLen;
    int i;

    if ((newSocket=accept(earSocket, &newName, &nameLen)) == -1) {
	sprintf(printBuf,"accept failed: %s", sys_errlist[errno]);
	Log_Event("AddClient", printBuf, LOG_FAIL);
    }
    if ((peerInfo=gethostbyaddr((char *)&newName.sin_addr,
				nameLen, AF_INET)) == NULL) {
	sprintf(printBuf,"gethostbyaddr failed: %s", sys_errlist[errno]);
	Log_Event("AddClient", printBuf, LOG_FAIL);
    }
    hostLen = strlen(peerInfo->h_name) + 1;
    clientPtr =	(SClient *)MEM_ALLOC("AddClient", sizeof(SClient)+hostLen);
    clientPtr->volId = NOVOLUME;
    clientPtr->devPtr = (SDev *)NULL;
    clientPtr->socket = newSocket;
    clientPtr->hostName = Str_Dup(peerInfo->h_name);

    /* Allocate a table location for the guy ... */
    for (i=0; i<clientMax; i++) {
	if (clientList[i] == (SClient *)NULL) break;
    }
    if (i == clientMax) {
	Log_Event("AddClient", "Client table is full\n", LOG_FAIL);
    }
    clientList[i] = clientPtr;
    clientPtr->indx = i;
    sprintf(printBuf, "Connection from %s on socket %d, index %d\n",
	    clientPtr->hostName, newSocket, i);
    Log_Event("AddClient", printBuf, LOG_MAJOR);

    return(newSocket);

}


/*
 *----------------------------------------------------------------------
 *
 * ProcessCmd --
 *
 *	Handle the client request.
 *      We do S_CMD{NULL,LOCK,FREE,STAT} at the moment.
 *
 * For lock requests we (eventually) return the device name as a string.
 * For unlock requests we just return T_SUCCESS.
 *
 * Results:
 *	status.
 *
 * Side effects:
 *	Either a success message, or an en-fattened queue.
 *
 *----------------------------------------------------------------------
 */

static int
ProcessCmd(clientPtr)
    SClient *clientPtr;       /* ptr to client */
{
    int status = T_FAILURE;
    int volId = clientPtr->volId;
    int cmd = clientPtr->cmd;
    SDev *devPtr;
    
    sprintf(printBuf, "Cmd %d vol %d\n", cmd, volId);
    Log_Event("ProcessCmd", printBuf, LOG_MAJOR);

    switch(cmd) {
    /*
     * Do nothing
     */
    case S_CMDNULL: 
	clientPtr->retCode = T_SUCCESS;
	break;
    /*
     * Request a volume
     */
    case S_CMDLOCK: 
	if (clientPtr->devPtr != (SDev *)NULL) {
	    clientPtr->retCode = T_BADCMD;
	} else {
	    clientPtr->retCode = T_SUCCESS;
	    status = T_ACTIVE;
	    devPtr = FindFreeDevice(volId);
	    if (devPtr != (SDev *)NULL) {
		AssignDevice(clientPtr, devPtr);
	    } else {
		sprintf(printBuf, "Enqueue 0x%x index %d for volume %d\n",
			clientPtr, clientPtr->indx, volId);
		Log_Event("ProcessCmd", printBuf, LOG_MINOR);
		Q_Add(waitQ, (Q_ClientData)clientPtr, Q_TAILQ);
	    }
	}
	break;
    /*
     * Done. release volume.
     */
    case S_CMDFREE:
	devPtr = clientPtr->devPtr;
	if (devPtr == (SDev *)NULL) {
	    clientPtr->retCode = T_BADCMD;
	} else if ((devPtr->volId != volId) ||
		   (devPtr->clientPtr != clientPtr)) {
	    clientPtr->retCode = T_BADCMD;
	} else {
	    clientPtr->retCode = T_SUCCESS;
	}
	break;

    case S_CMDSTAT:
	Sock_WriteInteger(clientPtr->socket, T_SUCCESS);
	SendDevStatus(clientPtr->socket);
	status = Q_Count(waitQ);
	Sock_WriteInteger(clientPtr->socket, status);
	Q_Iterate(waitQ, SendQStatusProc, (int *)clientPtr->socket);
	break;

    default:
	clientPtr->retCode = T_BADCMD;
	break;
    }

    return status;
}


/*
 *----------------------------------------------------------------------
 *
 * AssignDevice --
 *
 *	Assign a device to a client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      We send a response message to the client.
 *
 *----------------------------------------------------------------------
 */

static void
AssignDevice(clientPtr, devPtr)
    SClient *clientPtr;       /* ptr to client */
    SDev *devPtr;             /* assigned device */
{
    int sock = clientPtr->socket;
    int volId = clientPtr->volId;

    sprintf(printBuf, "Assigning 0x%x index %d vol %d to device %s\n",
	    clientPtr, clientPtr->indx, clientPtr->volId,
	    devPtr->name);
    Log_Event("AssignDevice", printBuf, LOG_MINOR);

    if (devPtr->volId != volId) {
	if (devPtr->volId != NOVOLUME) {
	    if (UnloadVolume(devPtr) != T_SUCCESS) {
		Sock_WriteInteger(sock, T_ROBOTFAILED);
		return;
	    }
	}
	if (LoadVolume(devPtr, volId) != T_SUCCESS) {
	    Sock_WriteInteger(sock, T_ROBOTFAILED);
	    return;
	}
    }

    volId = clientPtr->volId;
    devPtr->clientPtr = clientPtr;
    clientPtr->devPtr = devPtr;

    Sock_WriteInteger(sock, T_SUCCESS);
    Sock_WriteString(sock, devPtr->name, 0);

}


/*
 *----------------------------------------------------------------------
 *
 * ReleaseDevice --
 *
 *	Relinquish a device and give it to a waiting client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The policy mechanism is invoked to select a waiting client.
 *
 *----------------------------------------------------------------------
 */

static void
ReleaseDevice(clientPtr)
    SClient *clientPtr;       /* ptr to client */
{
    SDev *devPtr = clientPtr->devPtr;

    if (devPtr != (SDev *)NULL) {
	sprintf(printBuf, "Client 0x%x releasing vol %d device %s\n",
		clientPtr, devPtr->volId, devPtr->name);
	Log_Event("ReleaseDevice", printBuf, LOG_MINOR);
	devPtr->clientPtr = NULL;
	if (Q_Count(waitQ) > 0) {
	    clientPtr = (SClient *)Q_Iterate(waitQ, SearchWaitQProc, (int *)0);
	    if (clientPtr != (SClient *)NULL) {
		AssignDevice(clientPtr, devPtr);
	    }
	}
    } else {
	sprintf(printBuf, "Client 0x%x. No device to release\n", clientPtr);
	Log_Event("ReleaseDevice", printBuf, LOG_MINOR);
    }


}


/*
 *----------------------------------------------------------------------
 *
 * SearchWaitQProc --
 *
 *	Callback function implementing waiting queue search policy
 *
 * Results:
 *	client ptr.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      This is used indirectly by the SelectClient procedure.
 *
 *----------------------------------------------------------------------
 */

static int
SearchWaitQProc(qPtr, datum, retCodePtr, callVal)
    Q_Handle *qPtr;            /* wait queue */
    Q_ClientData datum;       /* client structure */
    int *retCodePtr;          /* receiving return code location */
    int *callVal;             /* unused */
{
    SClient *clientPtr = (SClient *)datum;
    SDev *devPtr;

    sprintf(printBuf, "Considering 0x%x for vol %d\n",
	    clientPtr, clientPtr->volId);
    Log_Event("SearchWaitQProc", printBuf, LOG_MINOR);
    if ((devPtr=FindFreeDevice(clientPtr->volId)) == NULL) {
	*retCodePtr = (int)NULL;
	return Q_ITER_CONTINUE;
    } else {
	clientPtr->devPtr = devPtr;
	*retCodePtr = (int)clientPtr;
	return Q_ITER_REMOVE_STOP;
    }

}



/*
 *----------------------------------------------------------------------
 *
 * FindFreeDevice --
 *
 *	Locate an idle device
 *
 * Results:
 *	device ptr.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      If a device is free but the desired volume is mounted in
 *      a busy device then a busy status is returned.
 *
 *----------------------------------------------------------------------
 */

static SDev *
FindFreeDevice(volId)
    int volId;                /* volume Id desired */
{
    int i;
    SDev *devPtr;

    for (i=0; i<deviceMax; i++) {
	if (devList[i].volId == volId) {
	    if (devList[i].clientPtr == (SClient *)NULL) {
		sprintf(printBuf, "Found vol %d avail in drive %s\n",
			volId, devList[i].name);
		Log_Event("FindFreeDevice", printBuf, LOG_MINOR);
		return devList+i;
	    } else {
		sprintf(printBuf, "Found vol %d but drive %s is in use\n",
			volId, devList[i].name);
		Log_Event("FindFreeDevice", printBuf, LOG_MINOR);
		return (SDev *)NULL;
	    }
	}
    }

    for (i=0; i<deviceMax; i++) {
	if (devList[i].volId == NOVOLUME) {
	    sprintf(printBuf, "Found empty device %s\n", devList[i].name);
	    Log_Event("FindFreeDevice", printBuf, LOG_MINOR);
    	    return devList+i;
	}
    }

    for (i=0; i<deviceMax; i++) {
	if (devList[i].clientPtr == (SClient *)NULL) {
	    sprintf(printBuf, "Found idle device %s\n", devList[i].name);
	    Log_Event("FindFreeDevice", printBuf, LOG_MINOR);
	    return devList+i;
	}
    }

    return (SDev *)NULL;

}


/*
 *----------------------------------------------------------------------
 *
 * LoadVolume --
 *
 *	Issue commands to move a volume into specified device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Robot is probably activated.
 *
 *----------------------------------------------------------------------
 */

static int
LoadVolume(devPtr, volId)
    SDev *devPtr;             /* receiving device */
    int volId;                /* identifier of volume to be loaded */
{
    int homeLoc;
    VolConfig *volPtr;
    int retCode;
    int chkId;
    char volLabel[T_MAXLABELLEN];

    if (Hash_Lookup(volTab, (char *)&volId, sizeof(volPtr->volId),
		    (Hash_ClientData *)&volPtr) != T_SUCCESS) {
	sprintf(printBuf, "Unknown volume %d specified.\n", volId);
	Log_Event("LoadVolume", printBuf, LOG_FAIL);
	return T_FAILURE;
    }

    homeLoc = volPtr->location;
    devPtr->volId = volId;

    sprintf(printBuf, "Loading %d into %s from location %d\n",
	    volId, devPtr->name, homeLoc);
    Log_Event("LoadVolume", printBuf, LOG_MINOR);
    
    if (!parms.skipLabel) {
	if (Dev_ReadVolLabel(robotStream, homeLoc, volLabel, &chkId) != T_SUCCESS) {
	    sprintf(printBuf, "Couldn't read volume label in slot %d. Continuing.\n",
		    homeLoc);
	    Log_Event("LoadVolume", printBuf, LOG_FAIL);
	} else if (chkId != volId) {
	    sprintf(printBuf, "Found volId %d (\"%s\") in slot %d but expected %d. Load aborted.\n",
		    chkId, volLabel, homeLoc, volId);
	    Log_Event("LoadVolume", printBuf, LOG_FAIL);
	    return T_ROBOTFAILED;
	}
    }
    retCode = Dev_MoveVolume(robotStream, homeLoc, devPtr->location);

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * UnloadVolume --
 *
 *	Issue commands necessary to move a volume from specified device.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Robot is activated.
 *
 *----------------------------------------------------------------------
 */

static int
UnloadVolume(devPtr)
    SDev *devPtr;             /* source device */
{
    int volId = devPtr->volId;
    VolConfig *volPtr;
    int homeLoc;
    int retCode;

    if (Hash_Lookup(volTab, (char *)&volId, sizeof(volPtr->volId),
		    (Hash_ClientData *)&volPtr) != T_SUCCESS) {
	sprintf(printBuf, "Unknown volume %d specified.\n", volId);
	Log_Event("UnloadVolume", printBuf, LOG_FAIL);
	return T_FAILURE;
    }

    homeLoc = volPtr->location;
    devPtr->volId = NOVOLUME;

    sprintf(printBuf, "Unloading %d from %s to location %d\n",
	    volId, devPtr->name, homeLoc);
    Log_Event("UnloadVolume", printBuf, LOG_MINOR);

    if ((retCode=Dev_UnloadVolume(devPtr->name)) != T_SUCCESS) {
	sprintf(printBuf, "Couldn't unload device %s: errno %d",
		devPtr->name, errno);
	Log_Event("UnloadVolume", printBuf, LOG_FAIL);
    } else {
	retCode = Dev_MoveVolume(robotStream, devPtr->location, homeLoc);
    }

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * SendDevStatus --
 *
 *	Return current state of device affairs.
 *
 * Results:
 *	status.
 *
 * Side effects:
 *      We send a device summary
 *
 *----------------------------------------------------------------------
 */

static int
SendDevStatus(sock)
    int sock;                 /* socket to ship summary to */
{
    int i;

    /*
     * Must adhere to S_DevStat layout even though
     * we're not actually using the structure
     */
    Sock_WriteInteger(sock, deviceMax);
    for (i=0; i<deviceMax; i++) {
	Sock_WriteString(sock, devList[i].name, 0);
	Sock_WriteInteger(sock, devList[i].volId);
	if (devList[i].clientPtr == (SClient *)NULL) {
	    Sock_WriteString(sock, "", 0);
	    Sock_WriteString(sock, "", 0);
	} else {
	    Sock_WriteString(sock, devList[i].clientPtr->hostName, 0);
	    Sock_WriteString(sock, devList[i].clientPtr->userName, 0);
	}
    }

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * SendQStatusProc --
 *
 *	Send client's info
 *
 * Results:
 *	iteration status for Q_Iterate routine.
 *
 * Side effects:
 *	We send the status of each Q member
 *
 * Note:
 *      This is a callback routine used by Q_Iterate
 *
 *----------------------------------------------------------------------
 */

static int
SendQStatusProc(qPtr, datum, retCodePtr, callVal)
    Q_Handle *qPtr;            /* q identifier */
    Q_ClientData datum;       /* ptr to client */
    int *retCodePtr;          /* receiving return code location */
    int *callVal;             /* sock to write to */
{
    SClient *clientPtr = (SClient *)datum;
    int sock = (int)callVal;

    /*
     * Must adhere to S_QStat layout even though
     * we're not actually using the structure
     * (The 'count' field has been sent already).
     */
    Sock_WriteInteger(sock, clientPtr->volId);
    Sock_WriteString(sock, clientPtr->hostName, 0);
    Sock_WriteString(sock, clientPtr->userName, 0);

    return Q_ITER_CONTINUE;
}


/*
 *----------------------------------------------------------------------
 *
 * DelClient --
 *
 *	Exorcise a client from list.
 *
 * Results:
 *	
 *
 * Side effects:
 *	We send a message telling the client what happened.
 *
 *----------------------------------------------------------------------
 */

static void
DelClient(clientPtr)
    SClient *clientPtr;       /* ptr to client */
{
    int sock = clientPtr->socket;
    int indx = clientPtr->indx;
    int retCode = clientPtr->retCode;

    sprintf(printBuf,"index %d sock %d\n", indx, sock);
    Log_Event("DelClient", printBuf, LOG_TRACE);

    Sock_WriteInteger(sock, retCode);
    close(sock);

    MEM_FREE("DelClient", (char *)clientPtr->hostName);
    MEM_FREE("DelClient", (char *)clientPtr->userName);
    MEM_FREE("DelClient", (char *)clientPtr);
    clientList[indx] = (SClient *)NULL;

}


/*
 *----------------------------------------------------------------------
 *
 * FindClientBySocket --
 *
 *	Locate a client by socket.
 *
 * Results:
 *	Ptr to client or NULL;
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static SClient *
FindClientBySocket(sock)
    int sock;                 /* socket number */
{
    int i;

    for (i=0; i<MAXCLIENT; i++) {
	if ((clientList[i] != (SClient *)NULL) &&
	    (clientList[i]->socket == sock)) {
	    sprintf(printBuf,"Found socket %d, index %d\n", sock, i);
	    Log_Event("FindClientBySocket", printBuf, LOG_TRACE);
	    return clientList[i];
	}
    }

    sprintf(printBuf,"Failed to find sock %d\n", sock);
    Log_Event("FindClientBySocket", printBuf, LOG_FAIL);

    return ((SClient *)NULL);

}


/*
 *----------------------------------------------------------------------
 *
 * CheckAuth --
 *
 *	Check a client's authorization ticket.
 *      Will use Kerberos eventually.
 *
 * Results:
 *	 0 == ok; 1 == unauthorized.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
CheckAuth(ticket)
    AuthHandle ticket;        /* kerberos ticket */
{
    return T_SUCCESS;

}


/*
 *----------------------------------------------------------------------
 *
 * SigIntHandler
 *
 *	Catch SIGINT interrupts.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Kills program
 *
 *----------------------------------------------------------------------
 */

static void
SigIntHandler(signal)
    int signal;               /* signal type */
{
    if (robotStream != -1) {
	Dev_DisplayMsg(robotStream, READY_MSG, MSG_STEADY);
    }
    exit(-1);
}
