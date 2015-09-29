/* 
 * jaquith.c --
 *
 *	Coordinator for the Jaquith archive system.
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
 *      "Beware of the man who works hard to learn something, learns it,
 *      and finds himself no wiser than before," Bokonon tells us.  "He
 *      is full of murderous resentment of people who are ignorant with-
 *      out having come by their ignorance the hard way."
 *      -- Kurt Vonnegut, _Cat's Cradle_
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jaquith.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "jaquithInt.h"
#include "option.h"

int syserr;
int jDebug;

static Parms parms = {
    DEF_DETAIL,
    DEF_JAQLOG,
    DEF_PORT,
    DEF_ROOT,
    DEF_DEFARCH,
    DEF_CONFIG, /* presently unused */
    DEF_GET,
    DEF_PUT,
    DEF_CLEAN,
    DEF_STATUS,
    DEF_CHILDDBG,
    DEF_DISKLOW,
    DEF_DISKHIGH,
    DEF_NETMASK,
    DEF_FSYNCFREQ
};

Option optionArray[] = {
    {OPT_INT, "logdetail", (char *)&parms.logDetail, "set logging detail (1 2 4 8)"},
    {OPT_INT, "port", (char *)&parms.port, "Port to listen on"},
    {OPT_STRING, "root", (char *)&parms.root, "Root of index"},
    {OPT_STRING, "logfile",(char *)&parms.logFile, "Enable logging to file"},
    {OPT_STRING, "config", (char *)&parms.config, "Configuration file"},
    {OPT_STRING, "getexec", (char *)&parms.getExec, "Jfetch exec path"},
    {OPT_STRING, "putexec", (char *)&parms.putExec, "Jupdate exec path"},
    {OPT_STRING, "cleanexec", (char *)&parms.cleanExec, "Jclean exec path"},
    {OPT_STRING, "statexec", (char *)&parms.statExec, "Jquery exec path"},
    {OPT_TRUE, "childdbg", (char *)&parms.childDbg, "Spawn children with debug"},
    {OPT_STRING, "disklow", (char *)&parms.diskLow, "Low percent of disk that can be used"},
    {OPT_STRING, "diskhigh", (char *)&parms.diskHigh, "High percent of disk that can be used"},
    {OPT_STRING, "netmask", (char *)&parms.netMask, "Mask to restrict inet connections. Form: 128.32.*.*"},
    {OPT_INT, "fsyncfreq", (char *)&parms.fsyncFreq, "Tell jupdate to fsync every N files"},
    {OPT_TRUE, "test", (char *)&parms.test, "Run in test mode. Local connections only"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

/* File globals. */
static char printBuf[T_MAXSTRINGLEN];
static TClient *clientList[MAXCLIENT];
static int clientMax = MAXCLIENT;
static int deadChild = 0;
static QInfo *qList = NULL;
static char *progList[5] = { NULL, NULL, NULL, NULL, NULL };
static FILE *memDbg;          /* stream for memory tracing */

/* The functions to be found here */
static void     CheckOptions   _ARGS_ ((Parms *parmsPtr));
static void     MakeRoot       _ARGS_ ((char *root));
static void     PerformLoop    _ARGS_ ((int sock));
static int      ReadMsg        _ARGS_ ((TClient *clientPtr));
static int      ReadMsgHdr     _ARGS_ ((TClient *clientPtr));
static int      ReadMsgBody    _ARGS_ ((TClient *clientPtr));
static int      AddClient      _ARGS_ ((int sock));
static void     DelClient      _ARGS_ ((TClient *clientPtr));
static int      FindClientByPid _ARGS_ ((int pid));
static TClient *FindClientBySocket _ARGS_ ((int sock));
static void     ReapChild      _ARGS_ (());
static void     SigChldHandler _ARGS_ ((int sig));
static void     InstallHandlers _ARGS_ (());
static void     StartManagers  _ARGS_ (());
static void     RemoveLocks    _ARGS_ ((char *root));
static int      CheckAuth      _ARGS_ ((AuthHandle handle));
static void     ProcessCmd     _ARGS_ ((TClient *clientPtr));
static void     SpawnChild     _ARGS_ ((TClient *clientPtr));
static QInfo   *FindQueueInfo  _ARGS_ ((char *archName));
static QInfo   *MakeQueueInfo  _ARGS_ ((char *archName));
static void     SendMailResp   _ARGS_ ((TClient *clientPtr,
					int status, int syserr));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Jaquith is the coordinator program for the archive system.
 *      It spawns jfetch, jupdate and jquery to carry out client requests.
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

/*    memDbg = fopen("jaquith.mem","w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    for (i=0; i<clientMax; i++) {
	clientList[i] = (TClient *)NULL;
    }

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckOptions(&parms);

    MakeRoot(parms.root);

    InstallHandlers();

    StartManagers();

    RemoveLocks(parms.root);

    earSocket = Sock_SetupEarSocket(&parms.port);

    PerformLoop(earSocket); 

    exit(0);
}


/*
 *----------------------------------------------------------------------
 *
 * CheckOptions --
 *
 *	Validate and install parameters
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
CheckOptions(parmsPtr)
    Parms *parmsPtr;          /* pointer to parameter block */
{
    int val;
    struct in_addr hostaddr;
    struct hostent *hostPtr;
    char hostname[T_MAXSTRINGLEN];

    if ((parmsPtr->port <= 0) || (parmsPtr->port > MAXPORT)) {
	Utils_Bailout("CheckOptions: bad port number.\n",BAIL_PRINT);
    }
    if (parmsPtr->logFile != (char *)NULL) {
	Log_Open(parmsPtr->logFile);
	Log_SetDetail(parmsPtr->logDetail);
    }
    if (parmsPtr->root[0] != '/') {
	Utils_Bailout("CheckOptions: root must be an absolute path.\n",
		      BAIL_PRINT);
    }
    if (access(parmsPtr->getExec, X_OK) == -1) {
	sprintf(printBuf, "CheckOptions: can't find or access %s\n",
		parmsPtr->getExec, X_OK);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if (access(parmsPtr->putExec, X_OK) == -1) {
	sprintf(printBuf, "CheckOptions: can't find or access %s\n",
		parmsPtr->putExec, X_OK);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if (access(parmsPtr->cleanExec, X_OK) == -1) {
	sprintf(printBuf, "CheckOptions: can't find or access %s\n",
		parmsPtr->cleanExec, X_OK);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if (access(parmsPtr->statExec, X_OK) == -1) {
	sprintf(printBuf, "CheckOptions: can't find or access %s\n",
		parmsPtr->statExec, X_OK);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    progList[T_CMDLS] = parmsPtr->getExec;
    progList[T_CMDGET] = parmsPtr->getExec;
    progList[T_CMDPUT] = parmsPtr->putExec;
    progList[T_CMDSTAT] = parmsPtr->statExec;
    if ((Utils_CvtInteger(parmsPtr->diskLow, 0, 100, &val) != T_SUCCESS) ||
	(Utils_CvtInteger(parmsPtr->diskHigh, 0, 100, &val) != T_SUCCESS)) {
	Utils_Bailout("CheckOptions: bad disk limit value.\n", BAIL_PRINT);
    }
    if (parmsPtr->test) {
	if (strcmp(parmsPtr->netMask, DEF_NETMASK) != 0) {
	    Utils_Bailout("Test mode prohibits use of netmask\n", BAIL_PRINT);
	}
	if (gethostname(hostname, sizeof(hostname)) == -1) {
	    Utils_Bailout("gethostname", BAIL_PERROR);
	}
	if ((hostPtr=gethostbyname(hostname)) == NULL) {
	    Utils_Bailout("gethostbyname", BAIL_PERROR);
	}
	bcopy(hostPtr->h_addr_list[0], &hostaddr.s_addr, hostPtr->h_length);
	parmsPtr->netMask = Str_Dup(inet_ntoa(hostaddr));
    }
}


/*
 *----------------------------------------------------------------------
 *
 * MakeRoot --
 *
 *	Create the top level directory, if it doesn't exist.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Initializes the Jaquith files.
 *
 *----------------------------------------------------------------------
 */

static void
MakeRoot(rootName)
    char *rootName;           /* name of index root */
{
    struct stat statBuf;
    int retCode;
    char pathName[T_MAXPATHLEN];

    errno = 0;
    retCode = stat(rootName, &statBuf);
    if ((retCode != -1) && (S_ISADIR(statBuf.st_mode))) {
	sprintf(printBuf,"Checking directory %s: %s\n",
		rootName, sys_errlist[errno]);
	Log_Event("MakeRoot", printBuf, LOG_MINOR);
    } else if ((retCode == -1) && (errno == ENOENT)) {
	sprintf(printBuf, "Creating %s as the archive root\n", rootName);
	retCode = mkdir(rootName, DEF_DIR_PERM);
	Log_Event("MakeRoot", printBuf, LOG_MAJOR);
    } else {
	sprintf(printBuf,"Bad directory %s: %s\n",
		pathName, sys_errlist[errno]);
	Log_Event("MakeRoot", printBuf, LOG_FAIL);
	exit(-1);
    }

    strcpy(pathName, rootName);
    strcat(pathName, "/freevols");
    errno = 0;
    retCode = stat(pathName, &statBuf);
    if (retCode != -1) {
	sprintf(printBuf,"Checking file %s: %s\n",
		pathName, sys_errlist[errno]);
	Log_Event("MakeRoot", printBuf, LOG_MINOR);
    } else if ((retCode == -1) && (errno == ENOENT)) {
	sprintf(printBuf, "Creating %s\n", pathName);
	retCode = creat(pathName, DEF_PERM);
	Log_Event("MakeRoot", printBuf, LOG_MAJOR);
    } else {
	sprintf(printBuf,"Bad file %s: %s\n",
		pathName, sys_errlist[errno]);
	Log_Event("MakeRoot", printBuf, LOG_FAIL);
	exit(-1);
    }

    strcpy(pathName, rootName);
    strcat(pathName, "/");
    strcat(pathName, DEF_DEFARCH);
    strcat(pathName, ".arch");
    errno = 0;
    retCode = stat(pathName, &statBuf);
    if ((retCode != -1) && (S_ISADIR(statBuf.st_mode))) {
	sprintf(printBuf,"Checking directory %s: %s\n",
		pathName, sys_errlist[errno]);
	Log_Event("MakeRoot", printBuf, LOG_MINOR);
    } else if ((retCode == -1) && (errno == ENOENT)) {
	if (mkdir(pathName, DEF_DIR_PERM) != -1) {
	    sprintf(printBuf, "Creating directory %s\n", pathName);
	    Log_Event("MakeRoot", printBuf, LOG_MAJOR);
	} else {
	    sprintf(printBuf, "Couldn't make directory %s\n", pathName);
	    Log_Event("MakeRoot", printBuf, LOG_FAIL);
	}
    } else {
	sprintf(printBuf,"Bad directory %s: %s\n",
		pathName, sys_errlist[errno]);
	Log_Event("MakeRoot", printBuf, LOG_FAIL);
	exit(-1);
    }

    strcat(pathName, "/auth");
    errno = 0;
    retCode = stat(pathName, &statBuf);
    if (retCode != -1) {
	sprintf(printBuf,"Checking file %s: %s\n",
		pathName, sys_errlist[errno]);
	Log_Event("MakeRoot", printBuf, LOG_MINOR);
    } else if ((retCode == -1) && (errno == ENOENT)) {
	sprintf(printBuf, "Creating %s\n", pathName);
	strcpy(pathName, rootName);
	strcat(pathName, "/");
	strcat(pathName, DEF_DEFARCH);
	strcat(pathName, ".arch");
	Admin_AddAuth(pathName, "root", "*", 'O');
	Log_Event("MakeRoot", printBuf, LOG_MAJOR);
    } else {
	sprintf(printBuf,"Bad file %s: %s\n",
		pathName, sys_errlist[errno]);
	Log_Event("MakeRoot", printBuf, LOG_FAIL);
	exit(-1);
    }

}



/*
 *----------------------------------------------------------------------
 *
 * StartManagers --
 *
 *	Read the manager list and start 'em running
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *	
 *----------------------------------------------------------------------
 */

static void
StartManagers()
{
    
}


/*
 *----------------------------------------------------------------------
 *
 * InstallHandlers --
 *
 *	Install SIGCHLD handler.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	
 *----------------------------------------------------------------------
 */

static void
InstallHandlers()
{

    if (signal(SIGCHLD, SigChldHandler) == (void(*)()) -1) {
	Utils_Bailout("signal", BAIL_PERROR);
    }

} /* InstallHandlers */


/*
 *----------------------------------------------------------------------
 *
 * RemoveLocks --
 *
 *	Remove any dead lock files.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will perform unlink on the offending files.
 *
 *----------------------------------------------------------------------
 */

static void
RemoveLocks(root)
    char *root;               /* root of tree */
{
    char pathName[T_MAXPATHLEN];
    DIR *rootDirPtr;
    DirObject *entryPtr;
    struct stat unixStatBuf;

    if ((rootDirPtr=(DIR *)opendir(root)) == (DIR *) NULL) {
	return;
    }

    while ((entryPtr=readdir(rootDirPtr)) != (DirObject *)NULL) {
	if (Str_Match(entryPtr->d_name, "*.arch")) {
	    strcpy(pathName, root);
	    strcat(pathName, "/");
	    strcat(pathName, entryPtr->d_name);
	    stat(pathName, &unixStatBuf);
	    if (S_ISADIR(unixStatBuf.st_mode)) {
		Lock_RemoveAll(pathName, LOCK_RMCONFIRM);
	    }
	}
    }

    closedir(rootDirPtr);

}



/*
 *----------------------------------------------------------------------
 *
 * PerformLoop --
 *
 *	Main command loop.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Accepts connections from clients and handles command
 *      messages to and from them.
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
    TClient *clientPtr;

    FD_ZERO(&readSet);
    FD_SET(earSocket,&readSet);

    sprintf(printBuf,"Start listening on port %d\n", parms.port);
    Log_Event("PerformLoop", printBuf, LOG_MAJOR);

    while(1) {
	copySet = readSet;
	numReady = select(FD_SETSIZE, &copySet, (fd_set *)NULL, 
			  (fd_set *)NULL, (struct timeval *)NULL);
	ReapChild();
	if (numReady == -1) {
	    continue;	    /* we were interrupted by a SIGCHLD */
	}
	for (i=0; i<FD_SETSIZE; i++) {
	    if ((i == earSocket) && (FD_ISSET(i, &copySet))) {
		if ((newSocket=AddClient(earSocket)) != T_FAILURE) {
		    FD_SET(newSocket, &readSet);
		}
	    } else if (FD_ISSET(i, &copySet)) {
		clientPtr = FindClientBySocket(i);
		retCode = ReadMsg(clientPtr);
		if (retCode != T_ACTIVE) {
		    FD_CLR(i, &readSet);
		    if (retCode == T_SUCCESS) {
			ProcessCmd(clientPtr);
		    } else {
			SendMailResp(clientPtr, retCode, syserr);
			DelClient(clientPtr);
		    }
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
 *	Obtain the msg header so we can determine what to do.
 * Then read in the rest of the msg according to its type.
 * Note: Due to timing, we cannot count on getting the whole header at
 * once, so we have to be prepared to read it in pieces and build it.
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
    TClient *clientPtr;       /* ptr to client */
{
    int retCode;

    if (clientPtr->msgBuf == (char *)NULL) {
	Log_Event("ReadMsg", "reading msg hdr...\n", LOG_TRACE);
	retCode = ReadMsgHdr(clientPtr);
    } else {
	Log_Event("ReadMsg", "reading msg body...\n", LOG_TRACE);
	retCode = ReadMsgBody(clientPtr);
    }
    
    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * ReadMsgHdr --
 *
 *	Obtain and validate the msg header.
 *
 * Results:
 *	Return code saying we've got the hdr, or still reading
 *      pieces of it.
 *
 * Side effects:
 *	none.
 *
 * Note: 
 *      This whole thing is disgusting. It was an early routine
 *      and should be rewritten.
 *
 *----------------------------------------------------------------------
 */

static int
ReadMsgHdr(clientPtr)
    TClient *clientPtr;       /* ptr to client */
{
    int retCode = T_SUCCESS;
    int readCnt;
    int sock = clientPtr->socket;
    T_ReqMsgHdr *reqPtr = &(clientPtr->hdr);

    readCnt = Sock_ReadSocket(sock);

    if (readCnt < 0) {
	syserr = errno;
	return T_IOFAILED;
    } else if (readCnt > 0) {
	return(T_ACTIVE);
    }

    /* Verify the complete header */
    if ((reqPtr->version=ntohl(reqPtr->version)) != MSGVERSION) {
	retCode = T_BADVERSION;
    }
    if (((reqPtr->cmd=ntohl(reqPtr->cmd)) < 0) ||
	(reqPtr->cmd >= T_MAXCMDS)) {
	retCode = T_BADCMD;
    }
    if (((reqPtr->len=ntohl(reqPtr->len)) < 1) ||
	(reqPtr->len > T_MAXMSGLEN)) {
	retCode = T_BADMSGFMT;
    }

    reqPtr->flags = ntohl(reqPtr->flags);

    if (CheckAuth(ntohl(reqPtr->ticket)) == T_FAILURE) {
	retCode = T_NOACCESS;
    }

    if (retCode != T_SUCCESS) {
	Sock_SendRespHdr(sock, retCode, syserr);
    } else {
	clientPtr->msgBuf = (char *)MEM_ALLOC("ReadMsgHdr", reqPtr->len);
	Sock_SetSocket(sock, clientPtr->msgBuf, reqPtr->len);
    }

    return T_ACTIVE;

}


/*
 *----------------------------------------------------------------------
 *
 * ReadMsgBody --
 *
 *	Obtain the msg body. The size will depend on the len
 *      field supplied in the header.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      None.
 *
 * Note: 
 *      This whole thing is disgusting. It was an early routine
 *      and should be rewritten.
 *
 *----------------------------------------------------------------------
 */

static int
ReadMsgBody(clientPtr)
    TClient *clientPtr;       /* ptr to client */
{
    int readCnt;
    int sock = clientPtr->socket;
    int count;
    char *arr[4];
    static char fmt[5] = "CCCC";

    readCnt = Sock_ReadSocket(sock);
    if (readCnt < 0) {
	syserr = errno;
	return T_IOFAILED;
    } else if (readCnt > 0) {
	return(T_ACTIVE);
    }

    /* Ok, we've got the fixed part of the body. Parse it */
    arr[0] = (char *)&clientPtr->userName;
    arr[1] = (char *)&clientPtr->groupName;
    arr[2] = (char *)&clientPtr->mailName;
    arr[3] = (char *)&clientPtr->archName;
    Sock_UnpackData(fmt, clientPtr->msgBuf, &count, arr);
    if (!*clientPtr->archName) {
	MEM_FREE("ReadMsgBody", clientPtr->archName);
	clientPtr->archName = Str_Dup(parms.defArch);
    }
    clientPtr->msgPtr = clientPtr->msgBuf+count;
    clientPtr->msgLen = clientPtr->hdr.len-count;

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
    TClient *clientPtr;
    int hostLen;
    int i;
    char *hostAddr;

    if ((newSocket=accept(earSocket, &newName, &nameLen)) == -1) {
	syserr = errno;
	sprintf(printBuf,"accept failed: Errno %d\n", errno);
	Log_Event("AddClient", printBuf, LOG_FAIL);
	return T_FAILURE;
    }
    if ((peerInfo=gethostbyaddr((char *)&newName.sin_addr,
				sizeof(newName.sin_addr),
				AF_INET)) == (struct hostent *)NULL) {
	syserr = errno;
	sprintf(printBuf,"gethostbyaddr failed: Errno %d\n", errno);
	Log_Event("AddClient", printBuf, LOG_FAIL);
	close(newSocket);
	return T_FAILURE;
    }

    if (((hostAddr=inet_ntoa(newName.sin_addr)) == NULL) ||
	(!Str_Match(hostAddr, parms.netMask)) ||
	(ntohs(newName.sin_port) >= NUMPRIVPORTS)) {
	sprintf(printBuf,"Access denied to %s (%s). Netmask = %s\n",
		peerInfo->h_name, hostAddr, parms.netMask);
	Log_Event("AddClient", printBuf, LOG_FAIL);
	Sock_SendRespHdr(newSocket, T_NOACCESS, 0, 0);
	close(newSocket);
	return T_FAILURE;
    }

    sprintf(printBuf, "Connection from %s (%s)\n",
	    peerInfo->h_name, hostAddr);
    Log_Event("AddClient", printBuf, LOG_MAJOR);

    hostLen = strlen(peerInfo->h_name) + 1;
    clientPtr =	(TClient *)MEM_ALLOC("AddClient", sizeof(TClient)+hostLen);
    clientPtr->msgLen = 0;
    clientPtr->msgBuf = (char *)NULL;
    clientPtr->msgPtr = (char *)NULL;
    clientPtr->archInfo = (QInfo *)NULL;
    clientPtr->mailName = (char *)NULL;
    clientPtr->hostName = (char *)clientPtr + sizeof(TClient);
    strcpy(clientPtr->hostName, peerInfo->h_name);
    clientPtr->socket = newSocket;
    clientPtr->date = Time_GetCurDate();

    /* Allocate a table slot for the guy ... */
    for (i=0; i<clientMax; i++) {
	if (clientList[i] == (TClient *)NULL) break;
    }

    /* temporary. should grow table */
    if (i == clientMax) {
	Log_Event("AddClient", "Client table full", LOG_FAIL);
	close(newSocket);
	return T_FAILURE;
    }
    clientList[i] = clientPtr;
    clientPtr->indx = i;

    /* Finally, set socket state so we can read hdr in bits&pieces */
    Sock_SetSocket(newSocket, (char *)&clientPtr->hdr,
		   sizeof (clientPtr->hdr));

    return(newSocket);

}


/*
 *----------------------------------------------------------------------
 *
 * ProcessCmd --
 *
 *	Fork a child for the command if possible, else enqueue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Either a new process, or an en-fattened queue.
 *
 *----------------------------------------------------------------------
 */

static void
ProcessCmd(clientPtr)
    TClient *clientPtr;       /* ptr to client */
{
    char *archName = clientPtr->archName;
    QInfo *infoPtr;

    if (clientPtr->hdr.cmd == T_CMDNULL) {
	Sock_SendRespHdr(clientPtr->socket, T_SUCCESS, 0);
	return;
    }

    if ((infoPtr=FindQueueInfo(archName)) == (QInfo *)NULL) {
	infoPtr = MakeQueueInfo(archName);
    }
    clientPtr->archInfo = infoPtr;

    if ((clientPtr->hdr.cmd != T_CMDPUT) ||
	(infoPtr->activeWriter == (TClient *)NULL)) {
	SpawnChild(clientPtr);
    } else {
	sprintf(printBuf, "Enqueue 0x%x slot %d for archive %s\n",
		clientPtr, clientPtr->indx, clientPtr->archName);
	Log_Event("ProcessCmd", printBuf, LOG_MAJOR);
	Q_Add(infoPtr->qPtr,(Q_ClientData)clientPtr, Q_TAILQ);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * SpawnChild --
 *
 *	Fork a child to do something useful.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static void
SpawnChild(clientPtr)
    TClient *clientPtr;       /* ptr to client */
{
    int pid = 0;
    char buf1[DECINTLEN+1];
    char buf2[DECINTLEN+1];
    char buf3[DECINTLEN+1];
    char buf4[T_MAXSTRINGLEN];
    char *progName;

    if (clientPtr->hdr.cmd == T_CMDPUT) {
	clientPtr->archInfo->activeWriter = clientPtr;
    }

    if ((pid=fork()) == -1) {
	sprintf(printBuf,"fork failed: %s\n", sys_errlist[errno]);
	Log_Event("SpawnChild", printBuf, LOG_FAIL);
	return;
    }

    if (pid == 0) {
	sprintf(buf1, "%d", clientPtr->socket);
	sprintf(buf2, "%d", clientPtr->hdr.flags);
	sprintf(buf3, "%d", parms.fsyncFreq);
	strcpy(buf4, clientPtr->archName);
	strcat(buf4, ".arch");
	progName = progList[clientPtr->hdr.cmd];
	execlp(progName, progName,
	       "-socket", buf1,
	       "-archive", buf4,
	       "-flags", buf2,
	       "-root", parms.root,
	       "-disklow", parms.diskLow,
	       "-diskhigh", parms.diskHigh,
	       "-username", clientPtr->userName,
	       "-groupname", clientPtr->groupName,
	       "-hostname", clientPtr->hostName,
	       "-cleaner", parms.cleanExec,
	       "-fsyncfreq", buf3,
	       (parms.childDbg) ? "-debug" : " ",
	       NULL);
	_exit(T_EXECFAILED);
    } else {
	sprintf(printBuf,"Spawn child 0x%x slot %d\n", 
		pid, clientPtr->indx);
	Log_Event("SpawnChild", printBuf, LOG_MINOR);
	clientPtr->pid = pid;
	close(clientPtr->socket);
	clientPtr->socket = NO_SOCKET;
    }

}


/*
 *----------------------------------------------------------------------
 *
 * DelClient --
 *
 *	Exorcise a client from list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
DelClient(clientPtr)
    TClient *clientPtr;       /* ptr to client */
{
    int sock = clientPtr->socket;
    int indx = clientPtr->indx;
    QInfo *infoPtr = clientPtr->archInfo;

    sprintf(printBuf,"client 0x%x sock %d archInfo 0x%x\n",
	    clientPtr->pid, sock, infoPtr);
    Log_Event("DelClient", printBuf, LOG_TRACE);

    if (sock != NO_SOCKET) {
	close(sock);
    }
    if ((infoPtr != (QInfo *)NULL) && (infoPtr->activeWriter == clientPtr)) {
	    infoPtr->activeWriter = (TClient *)NULL;
    }
    if (clientPtr->msgBuf != (char *)NULL) {
	MEM_FREE("DelClient", clientPtr->msgBuf);
    }
    MEM_FREE("DelClient", (char *)clientPtr->userName);
    MEM_FREE("DelClient", (char *)clientPtr->groupName);
    MEM_FREE("DelClient", (char *)clientPtr->mailName);
    MEM_FREE("DelClient", (char *)clientPtr->archName);
    MEM_FREE("DelClient", (char *)clientPtr);
    clientList[indx] = (TClient *)NULL;
}


/*
 *----------------------------------------------------------------------
 *
 * FindClientByPid --
 *
 *	Locate a client by pid.
 *
 * Results:
 *	Index of desired client or -1 if not found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
FindClientByPid(pid)
    int pid;                  /* process id */
{
    int i;

    for (i=0; i<MAXCLIENT; i++) {
	if ((clientList[i] != (TClient *)NULL) &&
	    (clientList[i]->pid == pid)) {
	    sprintf(printBuf,"Found pid 0x%x, slot %d\n", pid, i);
	    Log_Event("FindClientByPid", printBuf, LOG_TRACE);
	    return i;
	}
    }

    return T_FAILURE;
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

static TClient *
FindClientBySocket(sock)
    int sock;                 /* socket number */
{
    int i;

    for (i=0; i<MAXCLIENT; i++) {
	if ((clientList[i] != (TClient *)NULL) &&
	    (clientList[i]->socket == sock)) {
	    sprintf(printBuf,"Found socket %d (0x%x), slot %d\n",
		    sock, clientList[i]->pid, i);
	    Log_Event("FindClientBySocket", printBuf, LOG_TRACE);
	    return clientList[i];
	}
    }

    return ((TClient *)NULL);
}


/*
 *----------------------------------------------------------------------
 *
 * FindQueueInfo -- 
 *
 *      Locate queue for archive
 *
 * Results:
 *	QUEUE pointer.
 *
 * Side effects:
 *      None, but uses global qList.
 *
 *----------------------------------------------------------------------
 */

static QInfo *
FindQueueInfo(archName)
    char *archName;           /* archive name */
{
    QInfo *infoPtr = qList;

    while (infoPtr != (QInfo *)NULL) {
	if (strcmp(infoPtr->name,archName) == 0) {
	    sprintf(printBuf,"Found queue %s (0x%x)\n",
		    archName, infoPtr);
	    Log_Event("FindQueueInfo", printBuf, LOG_TRACE);
	    return (infoPtr);
	}
	infoPtr = infoPtr->link;
    }

    return(NULL);
}


/*
 *----------------------------------------------------------------------
 *
 * MakeQueueInfo -- 
 *
 *      Create a queue for an archive.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      Q_Handle handle is put in global qList.
 *
 *----------------------------------------------------------------------
 */

static QInfo *
MakeQueueInfo(archName)
    char *archName;           /* archive name */
{
    QInfo *infoPtr;

    infoPtr = (QInfo *)MEM_ALLOC("MakeQueue",
				 sizeof(QInfo)+strlen(archName)+1);
    infoPtr->qPtr = Q_Create(archName, 0);
    infoPtr->name = (char *)infoPtr+sizeof(QInfo);
    infoPtr->activeWriter = (TClient *)NULL;
    strcpy(infoPtr->name,archName);
    infoPtr->link = qList;
    qList = infoPtr;

    sprintf(printBuf,"Made queue: %s 0x%x\n", archName, infoPtr);
    Log_Event("MakeQueueInfo", printBuf, LOG_MINOR);

    return (infoPtr);

}


/*
 *----------------------------------------------------------------------
 *
 * ReapChild
 *	Clean up upon child termination.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Deallocates TClient structure
 *
 *----------------------------------------------------------------------
 */

static void
ReapChild()
{
    int pid;
    int indx;
    union wait wstatus;
    TClient *clientPtr;
    int oldmask;
    QInfo *infoPtr;
    
    oldmask = sigblock(sigmask(SIGCHLD));
    while (deadChild > 0) {
	pid = wait3(&wstatus, WNOHANG, NULL);
	if (pid == 0) {
	    sprintf(printBuf,
		    "wait3 returned 0 but deadChild is %d; resetting to 0\n",
		    deadChild);
	    Log_Event("ReapChild", printBuf, LOG_FAIL);
	    deadChild = 0;
	    break;
	} 

	deadChild--;
	sprintf(printBuf,"Child pid 0x%x status 0x%x; deadChild %d\n",
		pid, wstatus.w_status, deadChild);
	Log_Event("ReapChild",printBuf, LOG_TRACE);

	if ((indx=FindClientByPid(pid)) == T_FAILURE) {
	    sprintf(printBuf,"Couldn't find child pid 0x%x\n", pid);
	    Log_Event("ReapChild", printBuf, LOG_FAIL);
	} else {
	    clientPtr = clientList[indx];
	    SendMailResp(clientPtr, wstatus.w_status, syserr);
	    infoPtr = clientPtr->archInfo;
	    if (infoPtr == (QInfo *)NULL) {
		sprintf(printBuf,"No info for archive %s\n",
			clientPtr->archName);
		Log_Event("ReapChild", printBuf, LOG_FAIL);
	    } else {
		DelClient(clientPtr);
		if ((Q_Count(infoPtr->qPtr) > 0) &&
		    (infoPtr->activeWriter == (TClient *)NULL)) {
		    clientPtr = (TClient *)Q_Remove(infoPtr->qPtr);
		    SpawnChild(clientPtr);
		}
	    }
	}
    }
    sigsetmask(oldmask);

}


/*
 *----------------------------------------------------------------------
 *
 * SigChldHandler
 *
 *	Catch SIGCHLD interrupts.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Updates global counter which will be checked in PerformLoop.
 *
 *----------------------------------------------------------------------
 */

static void
SigChldHandler(signal)
    int signal;               /* signal type */
{
    deadChild++;
    sprintf(printBuf,"deadChild now %d\n",deadChild);
    Log_Event("SigChldHandler",printBuf, LOG_TRACE);

} /* SigChldHandler */



/*
 *----------------------------------------------------------------------
 *
 * SendMailResp --
 *
 *	Send response by mail if requested
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	May invoke shell and mailer
 *
 *----------------------------------------------------------------------
 */

static void
SendMailResp(clientPtr, status, syserr)
    TClient *clientPtr;
    int status;
    int syserr;
{
    int retCode;
    static char *cmdName[T_MAXCMDS] =
	{"Null", "jls", "jput", "jget", "jstat"};

    if (*clientPtr->mailName) {
	sprintf(printBuf, "mailing response 0x%x,0x%x to %s\n",
		status, syserr, clientPtr->mailName);
	Log_Event("SendMailResp", printBuf, LOG_MINOR);
	sprintf(printBuf,
		"Jaquith %s request\n\tFrom: %s@%s\n\tDate: %s\ncomplete with status 0x%x, errno 0x%x\n",
		cmdName[clientPtr->hdr.cmd],
		clientPtr->userName, clientPtr->hostName,
		Time_CvtToString(&clientPtr->date),
		status, syserr);
	if (Utils_SendMail(clientPtr->mailName, printBuf, "response") != 0) {
	    sprintf(printBuf, "Got return code %d trying to mail response %d,%d to %s\n",
		    retCode, status, syserr, clientPtr->mailName);
	    Log_Event("SendMailResp", printBuf, LOG_FAIL);
	}
    }

}


/*
 *----------------------------------------------------------------------
 *
 * CheckAuth --
 *
 *	Check a client's authorization ticket.
 *      Should use Kerberos eventually.
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

