/* 
 * sock.c --
 *
 *	Simple utility functions for managing socket communication
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/sock.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

#define BACKLOG 2
#define READ_TIMEOUT 60       /* Socket timeout before message, in secs */
#define WRITE_TIMEOUT 60 

typedef struct SockData {
    char *bufPtr;             /* current buffer pointer */
    int bufLen;               /* current buffer length */
} SockData;

extern int syserr;

/*
 * This lists holds the state of each active socket
 */
static SockData sockList[FD_SETSIZE];

/*
 * Utility buffer for formatting messages
 */
static char printBuf[T_MAXSTRINGLEN];


/*
 *----------------------------------------------------------------------
 *
 * Sock_SendRespHdr --
 *
 *	Send a response to client.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * If we're reporting a failure in status, then err is errno value
 * describing the problem.  If the problem was that the socket went
 * away then this reply will die too.
 *
 *----------------------------------------------------------------------
 */

int
Sock_SendRespHdr(sock, status, err)
    int sock;                 /* socket # */
    int status;               /* cmd status */
    int err;                  /* errno if status is no good */
{
    T_RespMsgHdr resp;

    resp.version = htonl(MSGVERSION);
    resp.len = htonl(0);
    resp.status = htonl(status);
    resp.errno = htonl(err);
    
    if (Sock_WriteNBytes(sock, (char *)&resp, sizeof(resp)) != sizeof(resp)) {
	return T_IOFAILED;
    } else {
	return T_SUCCESS;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_ReadRespHdr --
 *
 *	Retrieve a complete response hdr
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

void
Sock_ReadRespHdr(sock, mhdrPtr)
    int sock;                 /* incoming socket */
    T_RespMsgHdr *mhdrPtr;    /* receiving structure */
{
    int readCnt;
    readCnt = Sock_ReadNBytes(sock, (char *)mhdrPtr, sizeof(T_RespMsgHdr));
    if (readCnt != sizeof(T_RespMsgHdr)) {
	sprintf(printBuf, "Server's response is short %d bytes.\n",
		sizeof(T_RespMsgHdr)-readCnt);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    mhdrPtr->version = ntohl(mhdrPtr->version);
    mhdrPtr->len = ntohl(mhdrPtr->len);
    mhdrPtr->status = ntohl(mhdrPtr->status);
    mhdrPtr->errno = ntohl(mhdrPtr->errno);

    if (mhdrPtr->version != MSGVERSION) {
	Utils_Bailout("Message versions don't agree.\n", BAIL_PRINT);
    }

}


/*
 *----------------------------------------------------------------------
 *
 * Sock_SetSocket--
 *
 *	Save socket state so we can do partial reads.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stow away the number of bytes to be read and the bufPtr.
 *      Then we can update this state as we read junk.
 *
 *----------------------------------------------------------------------
 */

void
Sock_SetSocket(sock, bufPtr, bufLen) 
    int sock;                 /* socket # */
    char *bufPtr;             /* buffer to read into */
    int bufLen;               /* size of buffer */
{
    if (sock > FD_SETSIZE) {
	sprintf(printBuf, "SetSocket: Bad socket %d\n", sock);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    sockList[sock].bufPtr = bufPtr;
    sockList[sock].bufLen = bufLen;
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_ReadSocket--
 *
 *	Read as many bytes as possible up to a maximum.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Must have called SetSocket to setup the state for this
 *      descriptor otherwise all hell breaks loose.
 *
 *----------------------------------------------------------------------
 */

int
Sock_ReadSocket(sock) 
    int sock;                 /* socket # */
{
    int readCnt;
    char *bufPtr;
    int bufLen;

    if (sock > FD_SETSIZE) {
	sprintf(printBuf, "ReadSocket: Bad socket %d\n", sock);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    bufPtr = sockList[sock].bufPtr;
    bufLen = sockList[sock].bufLen;
    if ((bufPtr == (char *)NULL) || (bufLen < 1)) {
	sprintf(printBuf,"ReadSocket: Bad state for socket %d\n", sock);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    readCnt = read(sock, bufPtr, bufLen);

    if (readCnt < 1) {
	return T_IOFAILED;
    } else {
	sockList[sock].bufPtr += readCnt;
	sockList[sock].bufLen -= readCnt;
    }

    return (sockList[sock].bufLen);
}



/*
 *----------------------------------------------------------------------
 *
 * Sock_ReadInteger --
 *
 *	Read an integer object
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sock_ReadInteger(sock, intPtr) 
    int sock;                 /* socket # */
    int *intPtr;              /* buffer for integer */
{
    int value;
    int readCnt;

    readCnt = Sock_ReadNBytes(sock, (char *)&value, sizeof(int));
    if (readCnt != sizeof(int)) {
	return T_IOFAILED;
    } else {
	*intPtr = ntohl(value);
	return T_SUCCESS;
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Sock_ReadShort --
 *
 *	Read a short object
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sock_ReadShort(sock, shortPtr) 
    int sock;                 /* socket # */
    short *shortPtr;          /* buffer for short */
{
    short value;
    int readCnt;

    readCnt = Sock_ReadNBytes(sock, (char *)&value, sizeof(short));
    if (readCnt != sizeof(short)) {
	return T_IOFAILED;
    } else {
	*shortPtr = ntohs(value);
	return T_SUCCESS;
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Sock_ReadString --
 *
 *	Read a formatted string 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will allocate a buffer if memFlag == 1
 *
 *----------------------------------------------------------------------
 */

int
Sock_ReadString(sock, stringPtrPtr, memFlag) 
    int sock;                 /* socket # */
    char **stringPtrPtr;      /* buffer for string ptr */
    int memFlag;              /* 1 == allocate buffer */
{
    char *charPtr;
    int length;

    if ((Sock_ReadInteger(sock, &length) != T_SUCCESS) ||
	(length < 0)) {
	return T_IOFAILED;
    }

    if (memFlag) {
	charPtr = MEM_ALLOC("Sock_ReadString", (length+1)*sizeof(char));
    } else {
	charPtr = *stringPtrPtr;
    }
    
    if (length > 0) {
	if (Sock_ReadNBytes(sock, charPtr, length) != length) {
	    if (memFlag) {
		MEM_FREE("Sock_ReadString", charPtr);
	    }
	    return T_IOFAILED;
	}
    }

    charPtr[length] = '\0';
    *stringPtrPtr = charPtr;
    return T_SUCCESS;

}


/*
 *----------------------------------------------------------------------
 *
 * Sock_ReadFileStat --
 *
 *	Read T_FileStat
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May allocate space if memFlag is on.
 *
 *----------------------------------------------------------------------
 */

int
Sock_ReadFileStat(sock, statInfoPtr, memFlag)
    int sock;                 /* socket # */
    T_FileStat *statInfoPtr;  /* receiving structure */
    int memFlag;                 /* enable string allocation */
{
    int size;
    static char *buf = NULL;
    static int bufSize = 0;
    int count;
    char *fmt = "CCCCCCIIIIIIIIIIIS";
    char *arr[18];

    if ((Sock_ReadInteger(sock, &size) != T_SUCCESS) || (size < 0)) {
	return T_FAILURE;
    }
    if (size > bufSize) {
	if (buf) {
	    MEM_FREE("Sock_ReadFileStat", buf);
	}
	buf = (char *)MEM_ALLOC("Sock_ReadFileStat", size*sizeof(char));
	bufSize = size;
    }

    if (Sock_ReadNBytes(sock, buf, size) != size) {
	return T_FAILURE;
    }

    arr[0] = (char *)&statInfoPtr->fileName;
    arr[1] = (char *)&statInfoPtr->linkName;
    arr[2] = (char *)&statInfoPtr->uname;
    arr[3] = (char *)&statInfoPtr->gname;
    arr[4] = (char *)&statInfoPtr->abstract;
    arr[5] = (char *)&statInfoPtr->fileList;
    arr[6] = (char *)&statInfoPtr->vtime;
    arr[7] = (char *)&statInfoPtr->atime;
    arr[8] = (char *)&statInfoPtr->volId;
    arr[9] = (char *)&statInfoPtr->filemark;
    arr[10] = (char *)&statInfoPtr->tBufId;
    arr[11] = (char *)&statInfoPtr->offset;
    arr[12] = (char *)&statInfoPtr->mode;
    arr[13] = (char *)&statInfoPtr->uid;
    arr[14] = (char *)&statInfoPtr->gid;
    arr[15] = (char *)&statInfoPtr->mtime;
    arr[16] = (char *)&statInfoPtr->size;
    arr[17] = (char *)&statInfoPtr->rdev;

    Sock_UnpackData(fmt, buf, &count, arr);
/*
    fprintf(stderr, "file '%s', link '%s'\n",
	    statInfoPtr->fileName,statInfoPtr->linkName);
    fprintf(stderr, "u '%s', g '%s'\n",
	    statInfoPtr->uname,statInfoPtr->gname);
    fprintf(stderr, "abs '%s', list '%s'\n",
	    statInfoPtr->abstract,statInfoPtr->fileList);

    fprintf(stderr, "%x %x %x %x %x %x\n",
	    statInfoPtr->vtime, statInfoPtr->atime,
	    statInfoPtr->volId, statInfoPtr->filemark,
	    statInfoPtr->tBufId, statInfoPtr->offset);
    fprintf(stderr, "%x %x %x %x %x %x\n",
	    statInfoPtr->mode, statInfoPtr->uid,
	    statInfoPtr->gid, statInfoPtr->mtime,
	    statInfoPtr->size, (int)statInfoPtr->rdev);
*/
    return T_SUCCESS;
    
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_ReadNBytes --
 *
 *	Read arbitrary object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sock_ReadNBytes(sock, buf, bufLen)
    int sock;                 /* incoming socket */
    char *buf;                /* receiving buffer */
    int bufLen;               /* buffer size in bytes */
{
    int readCnt = 0;
    int totalCnt = 0;
    int retryCnt = 0;

    if (bufLen < 1) {
	fprintf(stderr,"Sock_ReadNBytes: bad length: %d\n", bufLen);
	fprintf("die %s\n", 0);
    }

    while (totalCnt < bufLen) {
	if ((readCnt=read(sock, buf+totalCnt, bufLen-totalCnt)) < 0) {
	    syserr = errno;
	    fprintf(stderr,"Sock_ReadNBytes: bad read on sock %d. errno %d\n",
		    sock, syserr);
	    /* A hack for Sprite with timeout problems */
	    if ((syserr != ETIMEDOUT) || (retryCnt++ >= 3)) {
		break;
	    }
	} else if (readCnt == 0) {
	    break;
	} else {
	    totalCnt += readCnt;
	}
    }
    return totalCnt;
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_WriteInteger --
 *
 *	Write an integer object
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sock_WriteInteger(sock, intVal) 
    int sock;                 /* socket # */
    int intVal;               /* integer to write */
{
    int writeCnt;

    intVal = htonl(intVal);
    writeCnt = Sock_WriteNBytes(sock, (char *)&intVal, sizeof(intVal));

    return (writeCnt == sizeof(intVal)) ? T_SUCCESS : T_IOFAILED;

}



/*
 *----------------------------------------------------------------------
 *
 * Sock_WriteShort --
 *
 *	Write a short object
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sock_WriteShort(sock, shortVal) 
    int sock;                 /* socket # */
    short shortVal;           /* short to write */
{
    int writeCnt;

    shortVal = htons(shortVal);
    writeCnt = Sock_WriteNBytes(sock, (char *)&shortVal, sizeof(shortVal));

    return (writeCnt == sizeof(short)) ? T_SUCCESS : T_IOFAILED;

}


/*
 *----------------------------------------------------------------------
 *
 * Sock_WriteString --
 *
 *	Write a formatted string 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will deallocate buffer if memFlag == 1
 *
 *----------------------------------------------------------------------
 */

int
Sock_WriteString(sock, charPtr, memFlag) 
    int sock;                 /* socket # */
    char *charPtr;            /* buffer pointer */
    int memFlag;                 /* 1 == deallocate after writing */
{
    int writeCnt;
    int length;

    length = strlen(charPtr);
    if (Sock_WriteInteger(sock, length) != T_SUCCESS) {
	return T_FAILURE;
    }

    if (length > 0) {
	writeCnt = Sock_WriteNBytes(sock, charPtr, length);
	if (writeCnt != length) {
	    return T_IOFAILED;
	} else if (memFlag) {
	    MEM_FREE("WriteString", charPtr);
	}
    }

    return T_SUCCESS;

}


/*
 *----------------------------------------------------------------------
 *
 * Sock_WriteFileStat --
 *
 *	Write a T_FileStat.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      It would be simpler but slower to write out the fields
 *      individually using the appropriate Sock_WriteFoo routine.
 *
 *----------------------------------------------------------------------
 */

int
Sock_WriteFileStat(sock, statInfoPtr, memFlag)
    int sock;                 /* socket # */
    T_FileStat *statInfoPtr;  /* file info block to write */
    int memFlag;                 /* 1 == deallocate after writing */
{
    char *buf;
    char *fmt = "CCCCCCIIIIIIIIIIIS";
    char *arr[18];
    int bufLen;
    int retCode = T_SUCCESS;

    arr[0] = statInfoPtr->fileName;
    arr[1] = statInfoPtr->linkName;
    arr[2] = statInfoPtr->uname;
    arr[3] = statInfoPtr->gname;
    arr[4] = statInfoPtr->abstract;
    arr[5] = statInfoPtr->fileList;
    arr[6] = (char *)&statInfoPtr->vtime;
    arr[7] = (char *)&statInfoPtr->atime;
    arr[8] = (char *)&statInfoPtr->volId;
    arr[9] = (char *)&statInfoPtr->filemark;
    arr[10] = (char *)&statInfoPtr->tBufId;
    arr[11] = (char *)&statInfoPtr->offset;
    arr[12] = (char *)&statInfoPtr->mode;
    arr[13] = (char *)&statInfoPtr->uid;
    arr[14] = (char *)&statInfoPtr->gid;
    arr[15] = (char *)&statInfoPtr->mtime;
    arr[16] = (char *)&statInfoPtr->size;
    arr[17] = (char *)&statInfoPtr->rdev;
    
    buf = Sock_PackData(fmt, arr, &bufLen);

    if ((Sock_WriteInteger(sock, bufLen) != T_SUCCESS) ||
	(Sock_WriteNBytes(sock, buf, bufLen) != bufLen)) {
	retCode = T_IOFAILED;
    } else if (memFlag) {
	Utils_FreeFileStat(statInfoPtr, 0);
    }

    MEM_FREE("Sock_WriteFileStat", buf);
    
    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_WriteNBytes --
 *
 *	Write arbitrary object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sock_WriteNBytes(sock, buf, bufLen)
    int sock;                 /* outgoing socket */
    char *buf;                /* source buffer */
    int bufLen;               /* buffer size in bytes */
{
    int writeCnt = 0;
    int totalCnt = 0;

    if (bufLen < 1) {
	fprintf(stderr,"Sock_WriteNBytes: bad length: %d\n", bufLen);
	fprintf("die %s\n", 0);
    }

    while (totalCnt < bufLen) {
	if ((writeCnt=write(sock, buf+totalCnt, bufLen-totalCnt)) < 1) {
	    syserr = errno;
	    fprintf(stderr,"Sock_WriteNBytes: writeCnt %d on sock %d. errno %d\n",
		    writeCnt, sock, syserr);
	    break;
	} else {
	    totalCnt += writeCnt;
	}
    }
    return totalCnt;
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_SetupSocket --
 *
 *	Open socket to the server.
 *
 * Results:
 *	socket number.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
Sock_SetupSocket(port, server, dieFlag)
    int port;                 /* port number */
    char *server;             /* server name */
    int dieFlag;              /* Death if we fail */
{
    struct sockaddr_in sockName;
    int nameLen = sizeof(struct sockaddr_in);
    struct hostent *peerInfo;
    int sock;

    sockName.sin_family = AF_INET;
    sockName.sin_port = htons(port);

    if ((peerInfo=gethostbyname(server)) == NULL) {
	if (dieFlag) {
	    sprintf(printBuf, "Can't connect to server %s: gethostbyname",
		    server);
	    Utils_Bailout(printBuf, BAIL_HERROR);
	} else {
	    syserr = errno;
	    return T_FAILURE;
	}
    }

    bcopy(peerInfo->h_addr_list[0], &sockName.sin_addr,
	  peerInfo->h_length);

    if ((sock=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	if (dieFlag) {
	    sprintf(printBuf, "Can't connect to server %s: socket", server);
	    Utils_Bailout(printBuf, BAIL_PERROR);
	} else {
	    syserr = errno;
	    return T_FAILURE;
	}
    }

    if (connect(sock, (struct sockaddr *)&sockName, nameLen) == -1) {
	if (dieFlag) {
	    sprintf(printBuf, "Can't connect to server %s, port %d: connect",
		    server, port);
	    Utils_Bailout(printBuf, BAIL_PERROR);
	} else {
	    syserr = errno;
	    return T_FAILURE;
	}
    }

    return(sock);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_SetupEarSocket --
 *
 *	Prepare the initial socket connection to receive incoming
 * connection requests.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Creates a TCP socket.
 *
 *----------------------------------------------------------------------
 */

int
Sock_SetupEarSocket(portPtr)
    int *portPtr;             /* port number */
{
    int earSocket;
    struct sockaddr_in earName;
    int nameLen = sizeof(struct sockaddr_in);

    if ((earSocket=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	Utils_Bailout("Can't set up listening socket: socket", BAIL_PERROR);
    }

    bzero((char*)&earName, nameLen);
    earName.sin_family = AF_INET;
    earName.sin_addr.s_addr = INADDR_ANY;
    earName.sin_port = htons((u_short)(*portPtr));

    if (bind(earSocket, (struct sockaddr *)&earName, nameLen)) {
	sprintf(printBuf, "Can't set up listening socket on port %d: bind", (int)*portPtr);
	Utils_Bailout(printBuf, BAIL_PERROR);
    }
	
    if (getsockname(earSocket, (struct sockaddr *)&earName, &nameLen)) {
	Utils_Bailout("Can't set up listing socket: getsockname", BAIL_PERROR);
    }

    *portPtr = (int) ntohs(earName.sin_port);
    if (listen(earSocket, BACKLOG) == -1) {
	Utils_Bailout("Can't set up listing socket: listen", BAIL_PERROR);
    }

    /* set the socket to close on exec */
    if (fcntl(earSocket, F_SETFD, 1) == -1) {
	Utils_Bailout("Can't set up listing socket: fcntl", BAIL_PERROR);
    }

    return(earSocket);

}


/*
 *----------------------------------------------------------------------
 *
 * Sock_PackData --
 *
 *	Convert array of objects strings to network form.
 *
 * Results:
 *	Buffer holding network-ready bytes, and its length.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

char *
Sock_PackData(fmt, objArray, packedLenPtr)
    char *fmt;                /* format string. I=int, S=short, C=char */
    char **objArray;          /* array of ptrs to objects */
    int *packedLenPtr;        /* length of resulting packed buffer */
{
    int i;
    int intVal = 0;
    int hIntVal;
    short shortVal;
    char *buf;
    char *workingPtr;

    for (i=0; fmt[i]; i++) {
	switch (fmt[i]) {
	case 'I':
	case 'i':
	    intVal += sizeof(int);
	    break;
	case 'S':
	case 's':
	    intVal += sizeof(short);
	    break;
	case 'C':
	case 'c':
	    intVal += strlen(objArray[i])+sizeof(int);
	    break;
	default:
	    sprintf(printBuf, "Sock_PackData: bad object type: %c\n", fmt[i]);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	    break;
	}

    }

    *packedLenPtr = intVal;
    buf = workingPtr = (char *)MEM_ALLOC("Sock_PackData", intVal*sizeof(char));

    for (i=0; fmt[i]; i++) {
	switch (fmt[i]) {
	case 'I':
	case 'i':
	    intVal = htonl(*(int *)objArray[i]);
	    bcopy((char *)&intVal, workingPtr, sizeof(int));
	    workingPtr += sizeof(int);
	    break;
	case 'S':
	case 's':
	    shortVal = htons(*(short *)objArray[i]);
	    bcopy((char *)&shortVal, workingPtr, sizeof(short));
	    workingPtr += sizeof(short);
	    break;
	case 'C':
	case 'c':
	    intVal = strlen(objArray[i]);
	    hIntVal = htonl(intVal);
	    bcopy((char *)&hIntVal, workingPtr, sizeof(int));
	    workingPtr += sizeof(int);
	    bcopy(objArray[i], workingPtr, intVal);
	    workingPtr += intVal;
	    break;
	default:
	    sprintf(printBuf, "Sock_PackData: bad object type: %c\n", fmt[i]);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	    break;
	}

    }

    return(buf);
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_UnpackData --
 *
 *	Convert network bytes to array of objects.
 *
 * Results:
 *	Array of objects is filled in.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
Sock_UnpackData(fmt, buf, countPtr, objArray)
    char *fmt;                /* format string. I=int, S=short, C=char */
    char *buf;                /* buf to be parsed */
    int *countPtr;            /* number of bytes processed */
    char **objArray[];        /* list of object ptr locations */
{
    int i;
    int intVal;
    int shortVal;
    char *workingPtr = buf;
    char *stringPtr;
    
    for (i=0; fmt[i]; i++) {
	switch (fmt[i]) {
	case 'I':
	case 'i':
	    bcopy(workingPtr, (char *)&intVal, sizeof(int));
	    intVal = (int)ntohl(intVal);
	    bcopy(&intVal, (char *)objArray[i], sizeof(int));
	    workingPtr += sizeof(int);
	    break;
	case 'S':
	case 's':
	    bcopy(workingPtr, (char *)&shortVal, sizeof(short));
	    shortVal = (short)ntohs(shortVal);
	    bcopy(&shortVal, (char *)objArray[i], sizeof(short));
	    workingPtr += sizeof(short);
	    break;
	case 'C':
	case 'c':
	    bcopy(workingPtr, (char *)&intVal, sizeof(int));
	    workingPtr += sizeof(int);
	    intVal = (int)ntohl(intVal);
	    stringPtr =
		(char *) MEM_ALLOC("Sock_UnpackData",(intVal+1)*sizeof(char));
	    bcopy((char *)&stringPtr, (char *)objArray[i], sizeof(char *));
	    bcopy(workingPtr, stringPtr, intVal);
	    *(stringPtr+intVal) = '\0';
	    workingPtr += intVal;
	    break;
	default:
	    sprintf(printBuf, "Sock_UnpackData: bad object type: %c\n",fmt[i]);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	    break;
	}

    }

    *countPtr = (workingPtr-buf);
    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Sock_SendReqHdr --
 *
 *	Send a formatted request header
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

void
Sock_SendReqHdr(sock, cmd, ticket, mail, arch, flags)
    int sock;                 /* outgoing socket */
    int cmd;                  /* command */
    AuthHandle ticket;        /* authorization ticket */
    char *mail;               /* mail address */
    char *arch;               /* archive name */
    int flags;                /* force and sync flags */
{
    int localFlags = flags;
    static char fmt[] = "CCCC";
    char *objList[6];
    int packedObjLen;
    char *packedObjBuf;
    int packedHdrLen;
    char *packedHdrBuf;
    uid_t shortUid;
    gid_t shortGid;
    int version = MSGVERSION;
    static char hdrFmt[] = "IIIII";

    shortUid = getuid();
    shortGid = getgid();
    objList[0] = Utils_GetLoginByUid((int)shortUid);
    objList[1] = Utils_GetGroupByGid((int)shortGid);
    objList[2] = mail;
    objList[3] = arch;
    
    if (cmd == T_CMDLS) {
	localFlags |= T_NODATA;
    }

    packedObjBuf = Sock_PackData(fmt, objList, &packedObjLen);

    objList[0] = (char *)&version;
    objList[1] = (char *)&cmd;
    objList[2] = (char *)&packedObjLen;
    objList[3] = (char *)&localFlags;
    objList[4] = (char *)&ticket;

    packedHdrBuf = Sock_PackData(hdrFmt, objList, &packedHdrLen);

    Sock_WriteNBytes(sock, packedHdrBuf, packedHdrLen);
    Sock_WriteNBytes(sock, packedObjBuf, packedObjLen);

    MEM_FREE("Sock_SendReqHdr", packedHdrBuf);
    MEM_FREE("Sock_SendReqHdr", packedObjBuf);

}
