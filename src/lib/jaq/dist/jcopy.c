/* 
 * jcopy.c --
 *
 *	Perform volume-to-volume copy on the Jaquith system
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
 *     "For every problem there is a solution which is
 *     simple, clean and wrong."
 * -- Henry Louis Mencken
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jquery.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"

static char printBuf[T_MAXSTRINGLEN];

static FILE *memDbg = NULL;   /* stream for memory tracing */

int jDebug;                   /* Internal debugging only */
int syserr = 0;               /* Our personal record of errno */

static char hostName[80];
static char userName[80];

#define DEF_SRCVOL -1
#define DEF_DESTVOL -1
#define DEF_VERBOSE 0
#define DEF_UPDATEFREQ 50

typedef struct MgrData {
    int sock;
    int volId;
    int stream;
} MgrData;

static void  CheckOptions   _ARGS_ ((Parms *parmsPtr));
static int   GetDeviceCnt   _ARGS_ ((int sock, int *countPtr));
static int   MgrAcquire     _ARGS_ ((MgrData *mgrDataPtr));
static int   MgrRelease     _ARGS_ ((MgrData *mgrDataPtr));
static int   PerformCopy    _ARGS_ ((int srcVol, int destVol));
static void  CopyVolume     _ARGS_ ((int srcVol, int destVol));

typedef struct parmTag {
    int srcVol;
    int destVol;
    char *root;
    char *mgrServer;
    int mgrPort;
    int verbose;
    int updateFreq;
    int bufSize;
    char *volFile;
} Parms;

Parms parms = {
    DEF_SRCVOL,
    DEF_DESTVOL,
    DEF_ROOT,
    DEF_MGRSERVER,
    DEF_MGRPORT,
    DEF_VERBOSE,
    DEF_UPDATEFREQ,
    T_TARSIZE,
    DEF_VOLFILE
};

Option optionArray[] = {
    {OPT_INT, "srcvol", (char *)&parms.srcVol, "Source volume name"},
    {OPT_INT, "destvol", (char *)&parms.destVol, "Destination volume name"},
    {OPT_STRING, "root", (char *)&parms.root, "root of index tree"},
    {OPT_STRING, "mgrserver", (char *)&parms.mgrServer, "Hostname of jukebox server"},
    {OPT_INT, "mgrport", (char *)&parms.mgrPort, "Port of jukebox server"},
    {OPT_TRUE, "v", (char *)&parms.verbose, "Verbose mode"},
    {OPT_INT, "updatefreq", (char *)&parms.updateFreq, "With -v flag, the frequency of copy status update"},
    {OPT_INT, "bufsize", (char *)&parms.bufSize, "Write unit in bytes"},
    {OPT_STRING, "volfile", (char *)&parms.volFile, "Volume configuration file"}
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * jcopy --
 *
 *	Main driver for volume-to-volume copy operation
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;                 /* See Option Array */
    char *argv[];
{
    int retCode;
    char *myName;
    uid_t myUid;

/*    memDbg = fopen("jcopy.mem","w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckOptions(&parms);

    myUid = geteuid();
    myName = Utils_GetLoginByUid(myUid);
    strcpy(userName, myName);
    gethostname(hostName, sizeof(hostName));

    CopyVolume(parms.srcVol, parms.destVol);

    MEM_REPORT("jcopy", ALLROUTINES, SORTBYOWNER);

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * CopyVolume -- 
 *
 *      Copy one Jaquith volume's contents to another.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      The idea is that an old tape is being rolled forward to a new one.
 *      We want to keep the Jaquith files up to date so we know that
 *      the volume is gone. 
 *
 *----------------------------------------------------------------------
 */

static void
CopyVolume(srcVol, destVol)
    int srcVol;               /* source volume Id */
    int destVol;              /* destination volume Id */
{
    int count = 0;
    int retCode;
    VolOwner *volOwnerPtr;
    VolConfig *volList;
    VolConfig *itemPtr;
    int volCnt;
    int i;
    char filePath[T_MAXPATHLEN];

    /*
     * Quick check to see if a tape-to-tape copy is even possible here
     */
    if (GetDeviceCnt(&count) != T_SUCCESS) {
        sprintf(printBuf,"Couldn't get device count from jmgr\n");
        Utils_Bailout(printBuf, BAIL_PRINT);
    }

    if (count < 2) {
        sprintf(printBuf,"Jmgr has only %d device. Can't do copy.\n", count);
        Utils_Bailout(printBuf, BAIL_PRINT);
    }

    /*
     * Acquire a new volume if one wasn't specified
     */
    if ((destVol == DEF_DESTVOL) ||
	(Utils_GetOk("Remove destination volume from free list? [y/n] "))) {
	if (Admin_GetFreeVol(parms.root, &destVol) != T_SUCCESS) {
	    sprintf(printBuf, "Couldn't get take volume from free list. errno %d\n", syserr);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	} else if (parms.verbose) {
	    fprintf(stdout, "Volume %d removed from free list.\n", destVol);
	}
    }

    /*
     * Safety overwrite check
     */
    if (destVol == srcVol) {
	sprintf(printBuf, "Source and destination volumes aren't distinct.\n");
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    /*
     * Destination volume already assigned to someone ?
     */
    volOwnerPtr = Admin_FindVolOwner(destVol, parms.root, "*.arch");
    if ((volOwnerPtr != NULL) &&
	(volOwnerPtr->owner != NULL)) {
	/* zap trailing '.arch' */
	volOwnerPtr->owner[strlen(volOwnerPtr->owner)-5] = '\0';
	sprintf(printBuf,
		"Volume %d already owned by archive '%s'. Continue? [y/n] ",
		destVol, volOwnerPtr->owner);
	if (!Utils_GetOk(printBuf)) {
	    exit(-1);
	}
    }

    /*
     * Do we even know about this tape ?
     */
    volOwnerPtr = Admin_FindVolOwner(srcVol, parms.root, "*.arch");
    if ((volOwnerPtr != NULL) &&
	(volOwnerPtr->owner == NULL)) {
	if (!Utils_GetOk("Source volume isn't owned by any archive. Contine? [y/n] ")) {
	    exit(-1);
	}
    }

    /*
     * Should source volume be retired ?
     */
    if (Utils_GetOk("Should source volume be removed from use? [y/n] ")) {
	Admin_ReadVolConfig(parms.volFile, volList, &volCnt);
	volList = (VolConfig *)MEM_ALLOC("CopyVolume",
					 volCnt*sizeof(VolConfig));
	if (Admin_ReadVolConfig(parms.volFile, volList, &volCnt)!= T_SUCCESS) {
	    sprintf(printBuf,"Couldn't read %s. Errno %d.\n",
		    parms.volFile, syserr);
	    volCnt = 0;
	}
	for (i=0,itemPtr=volList; i<volCnt; i++,itemPtr++) {
	    if (itemPtr->volId == srcVol) {
		break;
	    }
	}
	if (i >= volCnt) {
	    sprintf(printBuf,
		    "Couldn't find source volume in %s. Continue? [y/n] ",
		    parms.volFile);
	    if (!Utils_GetOk(printBuf)) {
		exit(-1);
	    }
	} else if (Admin_WriteVolConfig(parms.volFile, volList, volCnt)
		   != T_SUCCESS) {
	    sprintf(printBuf,
		    "Couldn't remove source volume from %s. Errno %d. Continue? [y/n] ",
		    parms.volFile, syserr);
	    if (!Utils_GetOk(printBuf)) {
		exit(-1);
	    }
	}
	MEM_FREE("CopyVolume", volList);
    }

    /*
     * Finally, we can do the copy itself
     */
    retCode = PerformCopy(srcVol, destVol);
    
    if ((retCode != T_SUCCESS) || (parms.verbose)) {
        fprintf(stderr,"Done with return code %d.\n", retCode);
    }
}



/*
 *----------------------------------------------------------------------
 *
 * PerformCopy --
 *
 *	Obtain the volumes and copy source stream to destination stream
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
PerformCopy(srcVol, destVol)
    int srcVol;               /* source volume id */
    int destVol;              /* destination volume id */
{
    int srcStream;
    int destStream;
    char *buf;
    int len = 0;
    int totCnt = 0;
    int updateCnt = 0;
    int lineCnt = 0;
    MgrData mgrSrc;
    MgrData mgrDest;

    buf = MEM_ALLOC("PerformCopy", parms.bufSize);

    mgrSrc.volId = srcVol;
    if (MgrAcquire(&mgrSrc) != T_SUCCESS) {
        sprintf(printBuf,"Couldn't get source volume %d.\n", srcVol);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if (parms.verbose) {
        fprintf(stderr,"Mounted source volume %d.\n", srcVol);
    }

    mgrDest.volId = destVol;
    if (MgrAcquire(&mgrDest) != T_SUCCESS) {
        sprintf(printBuf,"Couldn't get destination volume %d.\n", destVol);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if (parms.verbose) {
        fprintf(stderr,"Mounted destination volume %d.\n", destVol);
    }

    if (Dev_SeekVolume(srcStream, 0, DEV_ABSOLUTE) != T_SUCCESS) {
	fprintf(stderr,"Couldn't rewind source volume\n. errno %d\n",
		syserr);
	fflush(stderr);
	return T_IOFAILED;
    } else if (parms.verbose) {
	fprintf(stdout,"Rewound source volume.\n");
	fflush(stdout);
    }

    if (Dev_SeekVolume(destStream, 0, DEV_ABSOLUTE) != T_SUCCESS) {
	fprintf(stderr,"Couldn't rewind destination volume\n. errno %d\n",
		syserr);
	return T_IOFAILED;
    } else if (parms.verbose) {
	fprintf(stdout,"Rewound destination volume.\n");
	fflush(stdout);
    }

    if (parms.verbose) {
	fprintf(stdout,"Copying...");
	fflush(stdout);
    }

    while (len != EOF) {
	while ((len=Dev_ReadVolume(srcStream, buf, parms.bufSize)) > 0) {
	    if (Dev_WriteVolume(destStream, buf, len) != len) {
		fprintf(stderr,"\nShort write. errno %d\n", syserr);
		fflush(stderr);
		return T_IOFAILED;
	    }
	}
	if (len != EOF) {
	    totCnt++;
	}
	Dev_WriteEOF(destStream, 1);
	if ((parms.verbose) && (++updateCnt == parms.updateFreq)) {
	    updateCnt = 0;
	    if (lineCnt++ == 10) {
		lineCnt = 0;
		fputc('\n', stdout);
	    }
	    fprintf(stdout, " %d", totCnt);
	    fflush(stdout);
	}
    }

    if (parms.verbose) {
	fprintf(stdout, "\nCopied %d files.\n", totCnt);
	fflush(stdout);
    }

    if (MgrRelease(&mgrSrc) != T_SUCCESS) {
        fprintf(stderr,"Couldn't release volume %d.\n", srcVol);
    }

    if (MgrRelease(&mgrDest) != T_SUCCESS) {
        fprintf(stderr,"Couldn't release volume %d.\n", destVol);
    }

    MEM_FREE("PerformCopy", buf);

    return T_SUCCESS;

}



/*
 *----------------------------------------------------------------------
 *
 * GetDeviceCnt --
 *
 *	Call jukebox manager to get device count
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
GetDeviceCnt(countPtr) 
    int *countPtr;            /* receiving ptr */
{
    int status = -1;
    int mgrSock;
    int retCode = T_SUCCESS;
    int count;
    int i;
    int volId;
    char devName[80];
    char *devPtr = devName;
    char *hostPtr = hostName;
    char *userPtr = userName;

    count = -1;

    mgrSock = Sock_SetupSocket(parms.mgrPort, parms.mgrServer, 1);
    retCode = Sock_WriteString(mgrSock, userName, 0);
    retCode += Sock_WriteInteger(mgrSock, S_CMDSTAT);
    retCode += Sock_WriteInteger(mgrSock, 0);
    retCode += Sock_ReadInteger(mgrSock, &status);
    if (retCode == T_SUCCESS) {
        Sock_ReadInteger(mgrSock, &count);
    }    

    *countPtr = count;

    /* throw away device info. We just want the count. */
    for (i=0; i<count; i++) {
	if ((Sock_ReadString(mgrSock, &devPtr, 0) != T_SUCCESS) ||
	    (Sock_ReadInteger(mgrSock, &volId) != T_SUCCESS) ||
	    (Sock_ReadString(mgrSock, &hostPtr, 0) != T_SUCCESS) ||
	    (Sock_ReadString(mgrSock, &userPtr, 0) != T_SUCCESS)) {
	    return(T_FAILURE);
	}
    }

    if (count > -1) {
	count = -1;
	Sock_ReadInteger(mgrSock, &count);
    }

    /* throw away queuedevice info. */
    for (i=0; i<count; i++) {
	if ((Sock_ReadInteger(mgrSock, &volId) != T_SUCCESS) ||
	    (Sock_ReadString(mgrSock, &hostPtr, 0) != T_SUCCESS)||
	    (Sock_ReadString(mgrSock, &userPtr, 0) != T_SUCCESS)) {
	    return T_FAILURE;
	}
    }

    close(mgrSock);

    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * MgrAcquire
 *
 *	Talk to jukebox manager to acquire a volume
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	Acquires or releases volume
 *
 *----------------------------------------------------------------------
 */

static int
MgrAcquire(mgrDataPtr)
    MgrData *mgrDataPtr;
{
    int retCode = T_SUCCESS;
    char devName[T_MAXPATHLEN];
    char *devNamePtr = devName;
    int status =T_SUCCESS;
    int sleepSecs = 5;
    int retryCnt = 0;

    mgrDataPtr->sock= Sock_SetupSocket(parms.mgrPort, parms.mgrServer, 1);

    retCode = Sock_WriteString(mgrDataPtr->sock, userName, 0);
    retCode += Sock_WriteInteger(mgrDataPtr->sock, S_CMDLOCK);
    retCode += Sock_WriteInteger(mgrDataPtr->sock, mgrDataPtr->volId);
    retCode += Sock_ReadInteger(mgrDataPtr->sock, &status);

    if ((retCode != T_SUCCESS) || (status != T_SUCCESS)) {
	fprintf(stderr,"Couldn't acquire vol %d. Status %d, errno %d\n",
		mgrDataPtr->volId, status, syserr);
	return T_FAILURE;
    }

    Sock_ReadString(mgrDataPtr->sock, &devNamePtr, 0);

    /* Must sleep here, waiting for drive to spin up...*/
    while (((mgrDataPtr->stream=open(devName, O_RDWR, 0600)) == -1) &&
	   (retryCnt++ < 10)) {
	sleep(sleepSecs);
    }
    if (mgrDataPtr->stream == -1) {
	fprintf(stderr, "Couldn't open device %s. errno %d\n",
		devName, errno);
	return T_IOFAILED;
    }
    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * MgrRelease
 *
 *	Talk to jukebox manager to release a volume
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	Drops connection to jmgr
 *
 *----------------------------------------------------------------------
 */

static int
MgrRelease(mgrDataPtr)
    MgrData *mgrDataPtr;
{
    int retCode = T_SUCCESS;
    int status =T_SUCCESS;

    if (mgrDataPtr->sock == -1) {
	return T_SUCCESS;
    }

    retCode = Sock_WriteString(mgrDataPtr->sock, userName, 0);
    retCode += Sock_WriteInteger(mgrDataPtr->sock, S_CMDFREE);
    retCode += Sock_WriteInteger(mgrDataPtr->sock, mgrDataPtr->volId);
    retCode += Sock_ReadInteger(mgrDataPtr->sock, &status);
    if ((retCode != T_SUCCESS) || (status != T_SUCCESS)) {
	fprintf(stderr,"Couldn't release vol %d. Status %d, errno %d\n",
		mgrDataPtr->volId, status, syserr);
    }

    close(mgrDataPtr->sock);
    close(mgrDataPtr->stream);

    mgrDataPtr->sock = -1;
    mgrDataPtr->volId = -1;
    mgrDataPtr->stream = -1;

    return retCode;
}



/*
 *----------------------------------------------------------------------
 *
 * CheckOptions -- 
 *
 *	Make sure command line options look reasonable
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
CheckOptions(parmsPtr)
    Parms *parmsPtr;
{
    if (Utils_CheckName(parmsPtr->mgrServer, 1) == T_FAILURE) {
	sprintf(printBuf, "Bad mgr server name: '%s'.\n",
		parmsPtr->mgrServer);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if ((parmsPtr->mgrPort < 1) || (parmsPtr->mgrPort > SHRT_MAX)) {
	sprintf(printBuf, "Bad mgr port number %d.\n",
		parmsPtr->mgrPort);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if (parmsPtr->srcVol < 0) {
	sprintf(printBuf, "Bad source volume id.\n");
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if ((parmsPtr->destVol < 0) && (parmsPtr->destVol != DEF_DESTVOL)) {
	sprintf(printBuf, "Bad destination volume id.\n");
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if (parmsPtr->updateFreq <= 0) {
	sprintf(printBuf, "Update frequency must be > 0.\n");
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if ((parmsPtr->bufSize <= 0) && (parmsPtr->bufSize % T_TAPEUNIT)){
	sprintf(printBuf, "Need buffer size > 0 and a multiple of %d.\n",
		T_TAPEUNIT);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
}

