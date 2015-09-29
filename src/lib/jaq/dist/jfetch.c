/* 
 * jfetch.c --
 *
 *	Perform indexing on Jaquith archive system.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jfetch.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"

static int    GetFileQuery   _ARGS_ ((int, QuerySpec *));
static char **GetRegExpr     _ARGS_ ((int, int *));
static int    ReturnFileData _ARGS_ ((int sock,
				      T_FileStat *statInfoPtr,
				      int tBufStream));
static int    RecallTBuf     _ARGS_ ((char *archPath, int *tBufStreamPtr,
				      T_FileStat *statInfoPtr,
				      ArchConfig *archConfigPtr,
				      int *inStreamPtr));
static int    ReceiveIndxProc _ARGS_ ((T_FileStat *statInfoPtr,
				       ReceiveBlk *receiveBlkPtr));
static int   VerifyClient      _ARGS_ ((char *archPath, int sock,
					Caller *callerPtr));


static char printBuf[T_MAXSTRINGLEN];
static char *logPath;         /* pathname of log file */
static FILE *memDbg = NULL;   /* stream for memory tracing */

int jDebug;                   /* Internal debugging only */
int syserr = 0;               /* Our personal record of errno */

#define DEF_SOCK -1
#define DEF_FLAGS -1
#define DEF_DEBUG 0
#define DEF_DISKLOW 70
#define DEF_DISKHIGH 80
#define DEF_USER ""
#define DEF_GROUP ""
#define DEF_HOST ""
#define DEF_CLEANER ""

#define DEF_FILE_CNT 3

#define ROOTSUFFIX "/_ROOT"

typedef struct ReceiveBlk {
    int sock;
    char *archPath;
    ArchConfig *archConfigPtr;
} ReceiveBlk;

typedef struct parmTag {
    int sock;
    char *arch;
    int flags;
    char *root;
    int debug;
    int diskLow;
    int diskHigh;
    char *userName;
    char *groupName;
    char *hostName;
    char *cleaner;
    int fsyncFreq;
} Parms;

Parms parms = {
    DEF_SOCK,
    DEF_ARCH,
    DEF_FLAGS,
    DEF_ROOT,
    DEF_DEBUG,
    DEF_DISKLOW,
    DEF_DISKHIGH,
    DEF_USER,
    DEF_GROUP,
    DEF_HOST,
    DEF_CLEANER,
    DEF_FSYNCFREQ
};

Option optionArray[] = {
    {OPT_INT, "socket", (char *)&parms.sock, "socket #"},
    {OPT_STRING, "archive", (char *)&parms.arch, "archive name"},
    {OPT_INT, "flags", (char *)&parms.flags, "option flags"},
    {OPT_STRING, "root", (char *)&parms.root, "root of index tree"},
    {OPT_TRUE, "debug", (char *)&parms.debug, "enable debugging output"},
    {OPT_INT, "disklow", (char *)&parms.diskLow, "Low limit for usable disk (%)"},
    {OPT_INT, "diskhigh", (char *)&parms.diskHigh, "High limit for usable disk (%)"},
    {OPT_STRING, "username", (char *)&parms.userName, "Name of requestor"},
    {OPT_STRING, "groupname", (char *)&parms.groupName, "Group name of requestor"},
    {OPT_STRING, "hostname", (char *)&parms.hostName, "Machine name of requestor"},
    {OPT_STRING, "cleaner", (char *)&parms.cleaner, "cleaning program"},
    {OPT_INT, "fsyncfreq", (char *)&parms.fsyncFreq, "Do fsync every N files. Ignored."}
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * jfetch --
 *
 *	Main driver for indexing operations
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 * Note: jfetch is invoked from the Jaquith switchboard process,
 *       not from the command line.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;                 /* See Option Array */
    char *argv[];
{
    int sock;
    int i;
    QuerySpec fileSpec;
    int retCode;
    FILE *indxStream;
    Q_Handle *indxQueue;
    T_FileStat *statInfoPtr;
    int regExpCnt;
    char **regList;
    char *indxPath;
    char *archPath;
    int indxCnt;
    ReceiveBlk receiveBlk;
    char null = '\0';
    char perm;
    int ignoreTopLevelDir;
    ArchConfig archConfig;
    static char star[] = "*";
    static T_FileStat dummyFile =
	{"", "", "", "", "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    Caller caller;

/*    memDbg = fopen("jfetch.mem","w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    sock = parms.sock;
    jDebug = parms.debug;

    archPath = Str_Cat(3, parms.root, "/", parms.arch);
    logPath = Str_Cat(3, archPath, "/", ARCHLOGFILE);

    sprintf(printBuf,"Caller %s@%s, flags 0x%x\n",
	    parms.userName, parms.hostName, parms.flags);
    Log_AtomicEvent("jfetch", printBuf, logPath);

    /* Authorize user to send data, or not, as the case may be */
    if (VerifyClient(archPath, sock, &caller) != T_SUCCESS) {
	exit(0);
    }

    if (Admin_ReadArchConfig(archPath, &archConfig) != T_SUCCESS) {
	sprintf(printBuf,"Admin_ReadArchConfig failed: errno %d\n", syserr);
	Log_AtomicEvent("jfetch", printBuf, logPath);
	Sock_SendRespHdr(sock, T_ADMFAILED, syserr);
	return -1;
    }

    if ((retCode=GetFileQuery(sock, &fileSpec)) != T_SUCCESS) {
	if (jDebug) {
	    fprintf(stderr,"didn't get query\n");
	}
	Sock_SendRespHdr(sock, retCode, 0);
	return -1;
    }

    Sock_SendRespHdr(sock, T_SUCCESS, 0);
    regList = GetRegExpr(sock, &regExpCnt);

    if ((retCode != T_SUCCESS) || (regList == NULL)) {
	Log_AtomicEvent("jfetch", "Couldn't get request\n", logPath);
	Sock_SendRespHdr(sock, T_BADMSGFMT, syserr);
	exit(T_BADMSGFMT);
    }

    receiveBlk.archPath = archPath;
    receiveBlk.sock = sock;
    receiveBlk.archConfigPtr = &archConfig;

    indxQueue = Q_Create("indx", 0);

    for (i=0; (i<regExpCnt) && (retCode == T_SUCCESS); i++) {
	sprintf(printBuf,"Request: %s\n", regList[i]);
	Log_AtomicEvent("jfetch", printBuf, logPath);

	if ((fileSpec.fileName=STRRCHR(regList[i], '/')) != NULL) {
	    fileSpec.fileName++;
	} else {
	    fprintf(stderr,"No last slash\n");
	    retCode = T_BADMSGFMT;
	    break;
	}
	if ((retCode=Indx_MakeIndxList(archPath, regList[i], indxQueue))
	    != T_SUCCESS) {
	    fprintf(stderr,"Bad retcode from Indx_MakeIndxList\n");
	    break;
	}
	indxCnt = Q_Count(indxQueue);
	if (jDebug) {
	    fprintf(stderr,"indxQueue has %d entries. rc %d\n",
		    indxCnt, retCode);
	}
	if ((parms.flags & T_NODATA) && (fileSpec.recurse > 0)) {
	    ignoreTopLevelDir = 1;
	} else {
	    ignoreTopLevelDir = 0;
	}
	while ((indxCnt-- > 0) && (retCode == T_SUCCESS)) {
	    indxPath = (char *)Q_Remove(indxQueue);
	    if ((retCode=Indx_Match(&fileSpec, indxPath, archPath, &caller,
				    ReceiveIndxProc, &receiveBlk,
				    ignoreTopLevelDir)) != T_SUCCESS) {
		fprintf(stderr,"Bad retcode from Indx_match\n");
	    }
	    MEM_FREE("jfetch", indxPath);
	}
	MEM_FREE("jfetch",regList[i]);
	MEM_REPORT("jfetch", ALLROUTINES, SORTBYOWNER);
    }
	
    Sock_WriteFileStat(sock, &dummyFile, 0);

    sprintf(printBuf,"Done. retCode %d, errno %d\n", retCode, syserr);
    Log_AtomicEvent("jfetch", printBuf, logPath);

    Sock_SendRespHdr(sock, retCode, syserr);

    if (fileSpec.compAbstract) {
	MEM_FREE("jfetch",
		 (char *)fileSpec.compAbstract);/* allocated by regcomp */
    }

    Q_Destroy(indxQueue);
    MEM_FREE("jfetch", (char *)regList);
    MEM_FREE("jfetch", archPath);

    MEM_REPORT("jfetch", ALLROUTINES, SORTBYOWNER);

    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * RecallTBuf --
 *
 *	Recall a tbuf from tape if necessary and open it
 *
 * Results:
 *      return code
 *
 * Side effects:
 *	Opens tbuf.
 *
 *----------------------------------------------------------------------
 */

static int
RecallTBuf(archPath, tBufStreamPtr, statInfoPtr, archConfigPtr, inStreamPtr)
    char *archPath;           /* full path to archive */
    int *tBufStreamPtr;       /* receiving stream (updated) */
    T_FileStat *statInfoPtr;  /* file specification (updated) */
    ArchConfig *archConfigPtr; /* archive configuration params */
    int *inStreamPtr;         /* active volume descriptor */
{
    int retCode;
    int size;
    int tBufId = statInfoPtr->tBufId;
    int volId = statInfoPtr->volId;
    int filemark = statInfoPtr->filemark+1;
    int status;
    char buf[T_TARSIZE];
    int len;
    int pid;
    int percentUsed;
    long blocksFree;
    char lowChar[20];
    char highChar[20];
    int sleepSecs = 5;
    int retryCnt = 0;
    static char devName[T_MAXPATHLEN];
    static char *devNamePtr = devName;
    static int curVolId = -1;
    static int curFilemark = 0;
    static int sock;
    VolStatus volStatus;
    char recallPath[T_MAXPATHLEN];

    /* see if the buffer's already here */
    if (TBuf_Open(archPath, tBufId, tBufStreamPtr, NULL, O_RDONLY)
	!= T_SUCCESS) {
	return T_BUFFAILED;
    } else if (*tBufStreamPtr != -1) {
	if (jDebug) {
	    fprintf(stderr,"Using tbuf.%d\n", tBufId);
	}
	return T_SUCCESS;
    }

    sprintf(printBuf,"Recalling tbuf.%d from volume %d, location %d\n",
	    tBufId, volId, filemark);
    Log_AtomicEvent("jfetch", printBuf, logPath);
    if (jDebug) {
	fprintf(stderr,"(%d) %s", Time_Stamp(), printBuf);
    }

    Admin_GetDiskUse(parms.root, &percentUsed, &blocksFree);

    /* Let cleaner make some space */
    if ((percentUsed > parms.diskHigh) &&
	(Admin_AvailVolInfo(archPath))) {
	fprintf(stderr,"fetch: Below threshold or availvolinfo failed\n");
	if ((pid=fork()) == -1) {
	    sprintf(printBuf, "Fork of cleaner failed: %d\n", errno);
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	} else if (pid == 0) {
	    sprintf(lowChar, "%d", parms.diskLow);
	    sprintf(highChar, "%d", parms.diskHigh);
	    execlp(parms.cleaner, parms.cleaner,
		   "-root", parms.root,
		   "-arch", "*.arch",
		   "-tbufid", "-1",
		   "-username", parms.userName,
		   "-hostname", parms.hostName,
		   "-debug", (jDebug) ? "1" : "0",
		   "-newvol", "0",
		   "-disklow", lowChar,
		   "-diskhigh", highChar,
		   NULL);
	    sprintf(printBuf,"Execlp of cleaner failed: %d\n", errno);
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	    _exit(T_EXECFAILED);
	}
    }

    /* Need to acquire the volume? */
    if (curVolId != volId) {
	if (curVolId != -1) {
	    if (jDebug) {
		fprintf(stderr, "recallTBuf: releasing vol %d\n", curVolId);
	    }
	    retCode = Sock_WriteString(sock, parms.userName, 0);
	    retCode += Sock_WriteInteger(sock, S_CMDFREE);
	    retCode += Sock_WriteInteger(sock, curVolId);
	}
	if (jDebug) {
	    fprintf(stderr, "(%d) RecallTBuf: acquiring vol %d\n",
		    Time_Stamp(), volId);
	}

	if ((sock=Sock_SetupSocket(archConfigPtr->mgrPort,
				   archConfigPtr->mgrServer, 0)) == -1) {
	    sprintf(printBuf,"Jfetch couldn't connect to jmgr at %s port %d. errno %d\n",
		    archConfigPtr->mgrServer, archConfigPtr->mgrPort, syserr);
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	    if (Utils_SendMail(ARCHMASTER, printBuf, "alert") != 0) {
		sprintf(printBuf, "Got return code %d mailing alert to %s\n",
			retCode, ARCHMASTER);
		Log_AtomicEvent("Jfetch", printBuf, logPath);
	    }
	    return T_FAILURE;
	}
	
	retCode = Sock_WriteString(sock, parms.userName, 0);
	retCode += Sock_WriteInteger(sock, S_CMDLOCK);
	retCode += Sock_WriteInteger(sock, volId);
	retCode += Sock_ReadInteger(sock, &status);	

	if ((retCode != T_SUCCESS) || (status != T_SUCCESS)) {
	    sprintf(printBuf,"Bad return code from jmgr\n");
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	    return T_ROBOTFAILED;
	}

	Sock_ReadString(sock, &devNamePtr, 0);

	/*
	 * ok we have the volume. Quick check to make sure
	 * someone else didn't bring the buffer in while we
	 * were waiting.
	 */
	if ((TBuf_Open(archPath, tBufId, tBufStreamPtr, NULL, O_RDONLY)
	     == T_SUCCESS) &&
	    (*tBufStreamPtr != -1)) {
	    sprintf(printBuf,"Recall aborted. tbuf.%d already here.\n",
		    tBufId);
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	    return T_SUCCESS;
	}

	/* Close any old stream */
	if (*inStreamPtr != -1) {
	    Dev_CloseVolume(*inStreamPtr);
	    *inStreamPtr == -1;
	}

	/* Must sleep here, waiting for drive to spin up...*/
	while (((*inStreamPtr=Dev_OpenVolume(devName, O_RDONLY)) == -1) &&
	       (retryCnt++ < 10)) {
	    sleep(sleepSecs);
	}
	if (*inStreamPtr == -1) {
	    sprintf(printBuf,"Couldn't open device %s. errno %d\n",
		    devName, syserr);
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	    return T_IOFAILED;
	} else if (jDebug) {
	    fprintf(stderr, "(%d) RecallTBuf: opened vol %d\n",
		    Time_Stamp(), volId);
	}

	Dev_GetVolStatus(*inStreamPtr, &volStatus);
	if ((volStatus.position != 0) &&
	    (Dev_SeekVolume(*inStreamPtr, 0, DEV_ABSOLUTE) != T_SUCCESS)) {
	    sprintf(printBuf,"Couldn't rewind device %s. errno %d\n",
		    devName, syserr);
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	    curFilemark = -1;
	    return T_IOFAILED;
	}
	curVolId = volId;
	curFilemark = 0;
    }

    /* Now reposition if necessary */
    if (curFilemark != filemark)  {
	curFilemark = filemark - curFilemark;
	if (Dev_SeekVolume(*inStreamPtr, curFilemark, DEV_RELATIVE)
	 != T_SUCCESS) {
	    sprintf(printBuf,"Couldn't seek device %s, %d. errno %d\n",
		    devName, curFilemark, syserr);
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	    curFilemark = -1;
	    return T_IOFAILED;
	}
    }
	
    if (jDebug) {
	fprintf(stderr, "(%d) RecallTBuf: seeked vol %d\n",
		Time_Stamp(), volId);
    }

    /* open the tbuf stream */
    sprintf(recallPath, "%s/recall.%d\0", archPath, tBufId);
    if ((*tBufStreamPtr=open(recallPath, O_RDWR+O_CREAT, 0600)) < 0) {
	sprintf(printBuf,"Couldn't open recall.%d\n", tBufId);
	Log_AtomicEvent("jfetch", printBuf, logPath);
	Sock_WriteString(sock, parms.userName, 0);
	Sock_WriteInteger(sock, S_CMDFREE);
	Sock_WriteInteger(sock, volId);
	return retCode;
    }

    /* Finally, we can read in the buffer */
    size = 0;
    while ((len=Dev_ReadVolume(*inStreamPtr, buf, sizeof(buf))) > 0) {
	if (write(*tBufStreamPtr, buf, len) != len) {
	    curFilemark = -1;
	    sprintf(printBuf,"Short write to %s. errno %d\n",
		    recallPath, syserr);
	    Log_AtomicEvent("jfetch", printBuf, logPath);
	    return T_IOFAILED;
	}
	size += len;
	/* I thought a filemark would give us EOF but apparently */
	/* not. So we'll interpret a short read as EOF */
	if (len < sizeof(buf)) {
	    break;
	}
    }

    sprintf(buf, "%s/tbuf.%d\0", archPath, tBufId);    
    if (rename(recallPath, buf) < 0) {
	sprintf(printBuf,"Couldn't rename recall.%d to tbuf.%d. errno %d\n",
		tBufId, tBufId, errno);
    } else {
	sprintf(printBuf,"Restored tbuf.%d\n", tBufId);
    }
    Log_AtomicEvent("jfetch", printBuf, logPath);
    if (jDebug) {
	fprintf(stderr, "(%d) %s",  Time_Stamp(), printBuf);
    }

    curFilemark = filemark+1;

    return retCode;

}



/*
 *----------------------------------------------------------------------
 *
 * GetFileQuery --
 *
 *	Obtain query specification block
 *
 * Results:
 *      return code
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
GetFileQuery(sock, fileSpecPtr)
    int sock;                 /* incoming data socket */
    QuerySpec *fileSpecPtr;   /* receiving structure */
{
    int retCode;
    char *abstract;

    retCode = Sock_ReadInteger(sock, &fileSpecPtr->firstVer);
    retCode += Sock_ReadInteger(sock, &fileSpecPtr->lastVer);
    retCode += Sock_ReadInteger(sock, (int *)&fileSpecPtr->firstDate);
    retCode += Sock_ReadInteger(sock, (int *)&fileSpecPtr->lastDate);
    retCode += Sock_ReadString(sock, &fileSpecPtr->owner, 1);
    retCode += Sock_ReadString(sock, &fileSpecPtr->group, 1);
    retCode += Sock_ReadInteger(sock, &fileSpecPtr->flags);
    retCode += Sock_ReadInteger(sock, &fileSpecPtr->recurse);
    retCode += Sock_ReadString(sock, &abstract, 1);

    sprintf(printBuf,"Query: fv=%d, lv=%d, fd=%s, ld=%s, o=%s, g=%s, flg 0x%x, rec=%d, abs='%s'\n", 
	    fileSpecPtr->firstVer, fileSpecPtr->lastVer,
	    Time_CvtToString(&fileSpecPtr->firstDate),
	    Time_CvtToString(&fileSpecPtr->lastDate),
	    fileSpecPtr->owner, fileSpecPtr->group,
	    fileSpecPtr->flags, fileSpecPtr->recurse, abstract);
    Log_AtomicEvent("jfetch", printBuf, logPath);

    if (*abstract) {
	fileSpecPtr->compAbstract = regcomp(abstract);
    } else {
	fileSpecPtr->compAbstract = NULL;
    }

    MEM_FREE("GetFileQuery", abstract);

    return retCode;

}



/*
 *----------------------------------------------------------------------
 *
 * GetRegExpr --
 *
 *	Read the regular expressions from socket
 *
 * Results:
 *      ptr to array of reg expressions
 *
 * Side effects:
 *	none.
 *
 * Note:
 *      Very inefficient right now. Sorry.
 *
 *----------------------------------------------------------------------
 */

static char **
GetRegExpr(sock, countPtr)
    int sock;                 /* incoming socket */
    int *countPtr;            /* number of expressions read */
{
    int i;
    char **ftab = NULL;
    int count;

    Sock_ReadInteger(sock, &count);

    ftab = (char **)MEM_ALLOC("GetRegExpr",count*sizeof(char *));
    
    for (i=0; i<count; i++) {
	if (Sock_ReadString(sock, ftab+i, 1) == T_IOFAILED) {
	    if (ftab) {
		MEM_FREE("GetRegExpr", (char *)ftab);
	    }
	    *countPtr = 0;
	    return NULL;
	}
	Str_StripDots(ftab[i]);
    }

    *countPtr = count;

    return ftab;
}



/*
 *----------------------------------------------------------------------
 *
 * ReceiveIndxProc --
 *
 *	Accept data produced by indx code
 *
 * Results:
 *      none.
 *
 * Side effects:
 *	Sends info over socket to caller.
 *
 * Note:
 *      This is a callback routine used by MatchVersionsProc in indx.
 *
 *----------------------------------------------------------------------
 */

static int
ReceiveIndxProc(statInfoPtr, receiveBlkPtr)
    T_FileStat *statInfoPtr;  /* index information */
    ReceiveBlk *receiveBlkPtr; /* callback info block */
{
    char *archPath = receiveBlkPtr->archPath;
    int sock = receiveBlkPtr->sock;
    ArchConfig *archConfigPtr = receiveBlkPtr->archConfigPtr;
    static int curTBufId = -1;
    static int tBufStream;
    int retCode;
    static int inStream = -1;

    Admin_CvtTBufId(archPath, statInfoPtr->tBufId,
		    &statInfoPtr->volId, &statInfoPtr->filemark);
    if (parms.flags & T_NODATA) {
	if (jDebug) {
	    fprintf(stderr,"ReceiveIndxProc: sending %s\n",
		    statInfoPtr->fileName);
	}
	retCode = Sock_WriteFileStat(sock, statInfoPtr, 0);
    } else {
	if (curTBufId != statInfoPtr->tBufId) {
	    if (curTBufId != -1) {
		TBuf_Close(tBufStream, -1, -1);
	    }
	    if ((retCode=RecallTBuf(archPath, &tBufStream, statInfoPtr,
			   archConfigPtr, &inStream)) != T_SUCCESS) {
		return retCode;
	    }
	    curTBufId = statInfoPtr->tBufId;
	    Admin_UpdateLRU(parms.root, parms.arch, curTBufId, curTBufId);
	}
	retCode = ReturnFileData(sock, statInfoPtr, tBufStream);
    }
    
    return retCode;
}

    

/*
 *----------------------------------------------------------------------
 *
 * ReturnFileData --
 *
 *	Pump file data back to user
 *
 * Results:
 *      none.
 *
 * Side effects:
 *	Sends info over socket to caller.
 *
 *----------------------------------------------------------------------
 */

static int
ReturnFileData(sock, statInfoPtr, tBufStream)
    int sock;                 /* outgoing socket */
    T_FileStat *statInfoPtr;  /* file meta info */
    int tBufStream;           /* source buffer */
{
    int size;
    int count;
    char buf[T_BUFSIZE];
    int retCode;

    if ((retCode=TBuf_FindFile(tBufStream, statInfoPtr)) != T_SUCCESS) {
	if (jDebug) {
	    fprintf(stderr,"Failed to find file %s\n",
		    statInfoPtr->fileName);
	}
	return retCode;
    }
    size = statInfoPtr->size;
    retCode = Sock_WriteFileStat(sock, statInfoPtr, 0);

    while ((retCode == T_SUCCESS) && (size > 0)) {
	count = (size > sizeof(buf)) ? sizeof(buf) : size;
	if ((count=read(tBufStream, buf, count)) < 0) {
	    syserr = errno;
	    retCode = T_BUFFAILED;
	    break;
	}
	if (Sock_WriteNBytes(sock, buf, count) != count) {
	    retCode = T_IOFAILED;
	    break;
	}
	size -= count;
    }

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * VerifyClient --
 *
 *	Check authorization of client for this archive
 *
 * Results:
 *	return Code
 *
 * Side effects:
 *	Sends negative ack if not authorized else send go-ahead msg.
 *      Builds Caller structure for future use.
 *
 *----------------------------------------------------------------------
 */

static int
VerifyClient(archPath, sock, callerPtr)
    char *archPath;           /* archive pathname */
    int sock;                 /* caller socket */
    Caller *callerPtr;        /* caller id */
{
    int retCode;
    char perm;

    callerPtr->userName = Str_Dup(parms.userName);
    callerPtr->hostName = Str_Dup(parms.hostName);
    callerPtr->groupName = Str_Dup(parms.groupName);

    if ((retCode=Admin_CheckAuth(archPath, callerPtr, &perm)) != T_SUCCESS) {
	sprintf(printBuf,"Admin_CheckAuth failed: errno %d\n", syserr);
	Log_AtomicEvent("jfetch", printBuf, logPath);
	Sock_SendRespHdr(sock, retCode, syserr);
	return T_NOACCESS;
    }

    if ((perm != 'R') && (perm != 'W') && (perm != 'O')) {
	sprintf(printBuf,"Access denied: %s %s from %s\n",
		parms.userName, parms.groupName, parms.hostName);
	Log_AtomicEvent("jfetch", printBuf, logPath);
	Sock_SendRespHdr(sock, T_NOACCESS, 0);
	return T_NOACCESS;
    }

    /*
     * if user is owner, change his name to '*' so Str_Match succeeds.
     */
    if (perm == 'O') {
	*(callerPtr->userName) = '*';
	*(callerPtr->userName+1) = '\0';
    }

    return T_SUCCESS;
}
