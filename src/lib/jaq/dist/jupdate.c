/* 
 * jupdate.c --
 *
 *	Perform index jupdate on Jaquith archive system.
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
 *     Bookkeeper: Mr. Goldwyn, our files are bulging with paperwork we no
 *                 longer need.  May I have your permission to destroy all
 *                 records before 1945?
 *     Goldwyn:    Certainly.  Just be sure to keep a copy of everything.
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jupdate.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"

static char  printBuf[T_MAXSTRINGLEN];
static char *logPath;         /* pathname of log file */
static FILE *memDbg = NULL;   /* stream for memory tracing */

int jDebug;                   /* Internal debugging only */
int syserr = 0;               /* Our personal record of errno */

#define DEF_SOCK -1
#define DEF_FLAGS 0
#define DEF_DEBUG 0
#define DEF_USER ""
#define DEF_GROUP ""
#define DEF_HOST ""
#define DEF_DISKLOW 70
#define DEF_DISKHIGH 80
#define DEF_CLEANER ""

#define TBUFTARGETLEN 1024*T_TAPEUNIT
#define METACHARS "*?,\\{}()[]"

typedef struct ReceiveBlk {
    T_FileStat *statInfoPtr;
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

static int   ReadOneFile      _ARGS_ ((int sock, int tBufStream, 
				       int *byteCntPtr,
				       T_FileStat *statInfoPtr));
static int   FlushTBuf        _ARGS_ ((int tBufId, Parms *parmsPtr,
				       char *archPath));
static int   GetFileIndx      _ARGS_ ((T_FileStat *statInfoPtr,char *indxPath,
				       char *archPath, Caller *callerPtr));
static char *SortDirectoryList _ARGS_ ((char *list));
static int   StringCompareProc _ARGS_ ((char **a, char **b));
static int   ReceiveIndxProc   _ARGS_ ((T_FileStat *statInfoPtr,
					ReceiveBlk *receiveBlkPtr));
static void  ReapChildren      _ARGS_ (());
static int   VerifyClient      _ARGS_ ((char *archPath, int sock,
					Caller *callerPtr));

Option optionArray[] = {
    {OPT_INT, "socket", (char *)&parms.sock, "socket #"},
    {OPT_STRING, "archive", (char *)&parms.arch, "archive name"},
    {OPT_INT, "flags", (char *)&parms.flags, "option flags"},
    {OPT_STRING, "root", (char *)&parms.root, "root of index tree"},
    {OPT_TRUE, "debug", (char *)&parms.debug, "enable debugging output"},
    {OPT_INT, "disklow", (char *)&parms.diskLow, "Low usable disk (%)"},
    {OPT_INT, "diskhigh", (char *)&parms.diskHigh, "High usable disk (%)"},
    {OPT_STRING, "username", (char *)&parms.userName, "Name of requestor"},
    {OPT_STRING, "groupname", (char *)&parms.groupName, "Group name of requestor"},
    {OPT_STRING, "hostname", (char *)&parms.hostName, "Machine name of requestor"},
    {OPT_STRING, "cleaner", (char *)&parms.cleaner, "cleaning program"},
    {OPT_INT, "fsyncfreq", (char *)&parms.fsyncFreq, "Do fsync every N files"}
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main driver for jupdate program.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 * Note: jupdate is invoked from the Jaquith switchboard process,
 *       not from the command line. Argument 1 must be the fd of
 *       the incoming pipe.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int sock;
    int retCode = 0;
    int tBufStream;
    int tHdrStream;
    T_FileStat statInfo;
    int byteCnt;
    int hdrByteCnt;
    char *archPath;
    MetaInfo metaInfo;
    VolInfo volInfo;
    int remSpace;
    FILE *indxStream = (FILE *)NULL;
    FILE *metaStream = (FILE *)NULL;
    char indxPath[2*T_MAXPATHLEN];
    char newIndxPath[2*T_MAXPATHLEN];
    char *entryPtr;
    int  entrySize;
    int curTBufId;
    int dontArchive = 0;
    char oldFileName[T_MAXPATHLEN];
    int fileCnt = 0;
    int skipCnt = 0;
    char perm;
    long blocksFree;
    int percentUsed;
    int memCnt = 0;
    int userKBytes = 0;
    int indxBytes = 0;
    int i;
    int filesSinceFsync = 0;
    int filesSinceAck = 0;
    ArchConfig archConfig;
    time_t timeStamp;
    Caller caller;
    int ackFreq;


    memDbg = fopen("/jaquith/jtest/jupdate.mem", "w");
    MEM_CONTROL(1024*1024, memDbg, TRACEMEM+TRACECALLS+CHECKALLBLKS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    sock = parms.sock;
    jDebug = parms.debug;

    archPath = Str_Cat(3, parms.root, "/", parms.arch);
    logPath = Str_Cat(3, archPath, "/", ARCHLOGFILE);
    *indxPath = '\0';

    sprintf(printBuf,"Caller %s@%s, flags 0x%x\n",
	    parms.userName, parms.hostName, parms.flags);
    Log_AtomicEvent("jupdate", printBuf, logPath);

    /* get the ack frequency */
    Sock_ReadInteger(sock, &ackFreq);
    if (ackFreq < 0) {
	sprintf(printBuf,"Bad ack frequency: %d\n", ackFreq);
	fprintf(stderr,"%s",printBuf);
	Log_AtomicEvent("jupdate", printBuf, logPath);
	Sock_SendRespHdr(sock, T_BADMSGFMT, syserr);
	return -1;
    }

    /* Authorize user to send data, or not, as the case may be */
    if (VerifyClient(archPath, sock, &caller) != T_SUCCESS) {
	exit(0);
    }

    if (Admin_ReadArchConfig(archPath, &archConfig) != T_SUCCESS) {
	sprintf(printBuf,"Admin_ReadArchConfig failed: errno %d\n", syserr);
	fprintf(stderr,"%s",printBuf);
	Log_AtomicEvent("jupdate", printBuf, logPath);
	Sock_SendRespHdr(sock, T_ADMFAILED, syserr);
	return -1;
    }

    if (Admin_GetCurTBuf(archPath, &curTBufId) != T_SUCCESS) {
	sprintf(printBuf,"Admin_GetCurTBuf failed: errno %d\n", syserr);
	Log_AtomicEvent("jupdate", printBuf, logPath);
	Sock_SendRespHdr(sock, T_ADMFAILED, syserr);
	return -1;
    }

    if ((metaStream=Admin_OpenMetaInfo(archPath, curTBufId)) == NULL) {
	sprintf(printBuf,"Admin_OpenMetaInfo failed: errno %d\n", syserr);
	Log_AtomicEvent("jupdate", printBuf, logPath);
	Sock_SendRespHdr(sock, T_ADMFAILED, syserr);
	return -1;
    }

    if (Admin_ReadMetaInfo(metaStream, &metaInfo) != T_SUCCESS) {
	sprintf(printBuf,"Admin_ReadMetaInfo failed: errno %d\n", syserr);
	Log_AtomicEvent("jupdate", printBuf, logPath);
	Sock_SendRespHdr(sock, T_ADMFAILED, syserr);
	return -1;
    }

    if (jDebug) {
	fprintf(stderr,"jupdate: arch %s, tBufSize %d tHdrSize %d tBuf %d\n",
		parms.arch, metaInfo.tBufSize, metaInfo.tHdrSize, curTBufId);
    }

    if ((TBuf_Open(archPath, curTBufId,
		   &tBufStream, &tHdrStream, O_WRONLY) != T_SUCCESS) ||
	(TBuf_Seek(tBufStream, metaInfo.tBufSize) != T_SUCCESS) ||
	(TBuf_Seek(tHdrStream, metaInfo.tHdrSize) != T_SUCCESS)) {
	sprintf(printBuf,"Couldn't open or seek tbuf: errno %d\n", syserr);
	Log_AtomicEvent("jupdate", printBuf, logPath);
	retCode = T_BUFFAILED;
    }

    /*
     * Must use a single timestamp for entire update operation,
     * otherwise retrieval is messed up. Retrieving a directory omits
     * some of its files because their timestamps are later than the
     * parent's, making them look like a later version.
     */
    timeStamp = Time_GetCurDate();

    /*
     * Now process the incoming files one by one.
     * Each file is preceded by its meta information
     */
    while (retCode == T_SUCCESS) {
	if (Sock_ReadFileStat(sock, &statInfo, 1) != T_SUCCESS) {
	    return T_BUFFAILED;
	}
	if (!*statInfo.fileName) {
	    break; /* we're done */
	}

	filesSinceFsync++;
	statInfo.vtime = timeStamp;
	strcpy(oldFileName, statInfo.fileName);
	Str_StripDots(statInfo.fileName);

	/* Open a new index file if its different from the current one */
	Indx_MakeIndx(archPath, statInfo.fileName, newIndxPath);
	if (strcmp(indxPath, newIndxPath) != 0) {
	    Indx_Close(indxStream);
	    strcpy(indxPath, newIndxPath);
	    retCode = Indx_Open(indxPath, "a", &indxStream);
	}

	if (S_ISADIR(statInfo.mode)) {
	    statInfo.fileList = SortDirectoryList(statInfo.fileList);
	}

	/* See if we really need to archive this file or not */
	if (!(parms.flags & T_FORCE)) {
	    dontArchive = GetFileIndx(&statInfo, indxPath, archPath, &caller);
	    if ((Sock_WriteString(sock, oldFileName, 0) != T_SUCCESS) ||
		(Sock_WriteInteger(sock, dontArchive) != T_SUCCESS)) {
		sprintf(printBuf,"Couldn't confirm file %s: errno %d\n",
			oldFileName, syserr);
		Log_AtomicEvent("jupdate", printBuf, logPath);
		retCode = T_BUFFAILED;
		break;
	    } else if (dontArchive) {
		skipCnt++;
		if ((ackFreq > 0) && (++filesSinceAck >= ackFreq)) {
		    Sock_WriteString(sock, oldFileName, 0);
		    filesSinceAck = 0;
		}
		Utils_FreeFileStat(&statInfo, 0);
		if (memCnt++ == 100) {
		    memCnt == 0;
		    MEM_REPORT("jupdate", ALLROUTINES, SORTBYREQ);
		}
		continue;
	    }
	}
	fileCnt++;

	/*
	 * Heuristic for deciding if we should start a new tbuf:
	 * If the new file will overflow the desired tbuf length
	 * by more than the overhead involved in a new tape buffer,
	 * then start a new one.
	 * (overhead = filemark size; ignores index entry space)
	 */
	remSpace = archConfig.tBufSize - metaInfo.tBufSize;
	if (statInfo.size-remSpace > T_FILEMARKSIZE) {
	    metaInfo.tBufPad =
	        TBuf_Close(tBufStream, tHdrStream, metaInfo.tBufSize);
	    Admin_WriteMetaInfo(metaStream, &metaInfo);
	    Admin_CloseMetaInfo(metaStream);
	    metaInfo.tBufSize = 0;
	    metaInfo.tHdrSize = T_TBLOCK;
	    metaInfo.fileCnt = 0;
	    metaInfo.tBufPad = 0;
	    Admin_SetCurTBuf(archPath, curTBufId+1);
	    Admin_OpenMetaInfo(archPath, curTBufId+1);
	    Admin_GetDiskUse(parms.root, &percentUsed, &blocksFree);
	    if ((parms.flags & T_SYNC) ||
		(percentUsed >= parms.diskHigh)) {
		FlushTBuf(curTBufId, &parms, archPath);
	    } else if (blocksFree < 2*(statInfo.size>>10)) {
		parms.flags |= T_SYNC;
		FlushTBuf(curTBufId, &parms, archPath);
		parms.flags &= ~T_SYNC;
	    }
	    curTBufId++;
/*
	    if (jDebug) {
		sprintf(printBuf,"Starting tbuf.%d for %s\n",
			curTBufId, statInfo.fileName);
		Log_AtomicEvent("jupdate", printBuf, logPath);
	    }
*/
	    retCode = TBuf_Open(archPath, curTBufId,
				&tBufStream, &tHdrStream, O_WRONLY);
	    TBuf_Seek(tHdrStream, T_TBLOCK);
	}

	/* Now copy in user's file into buffer */
	if ((retCode=ReadOneFile(sock, tBufStream, &byteCnt, &statInfo))
	    != T_SUCCESS) {
	    sprintf(printBuf,"ReadOneFile failed: errno %d\n", syserr);
	    Log_AtomicEvent("jupdate", printBuf, logPath);
	    break;
	}

	/* Update the header file and indx file */
	statInfo.tBufId = curTBufId;
	statInfo.offset = metaInfo.tBufSize;
	if ((hdrByteCnt=Indx_WriteIndxEntry(&statInfo, 
					    tHdrStream, indxStream)) <= 0) {
	    sprintf(printBuf,"Write of index info failed: errno %d\n", syserr);
	    Log_AtomicEvent("jupdate", printBuf, logPath);
	    retCode = T_IOFAILED;
	    break;
	}

	metaInfo.tBufSize += byteCnt;
	metaInfo.tHdrSize += hdrByteCnt;
	metaInfo.fileCnt++;

	/* Sync this stuff to disk for safety, if requested */
	if ((parms.fsyncFreq > 0) && (filesSinceFsync > parms.fsyncFreq)) {
	    fsync(tBufStream);
	    fsync(tHdrStream);
	    fsync(fileno(indxStream));
	}

	/* If all went well, update the meta info to make it official */
	if ((retCode=Admin_WriteMetaInfo(metaStream, &metaInfo)) != T_SUCCESS) {
	    sprintf(printBuf,"Admin_WriteMetaInfo failed: errno %d\n", syserr);
	    Log_AtomicEvent("jupdate", printBuf, logPath);
	    retCode = T_IOFAILED;
	    break;
	}

	/* Sync this stuff to disk for safety, if requested */
	if ((parms.fsyncFreq > 0) && (filesSinceFsync > parms.fsyncFreq)) {
	    fsync(fileno(metaStream));
	    filesSinceFsync = 0;
	}

	userKBytes += byteCnt>>10;
	indxBytes += hdrByteCnt;

	/* Acknowledge this stuff, if requested */
	if ((ackFreq > 0) && (++filesSinceAck >= ackFreq)) {
	    Sock_WriteString(sock, oldFileName, 0);
	    filesSinceAck = 0;
	}

	Utils_FreeFileStat(&statInfo, 0);

	if (memCnt++ == 100) {
	    memCnt == 0;
	    MEM_REPORT("jupdate", ALLROUTINES, SORTBYREQ);
	}

    }

    Utils_FreeFileStat(&statInfo, 0);

    MEM_REPORT("jupdate", ALLROUTINES, SORTBYREQ);

    metaInfo.tBufPad = TBuf_Close(tBufStream, tHdrStream, metaInfo.tBufSize);
    Admin_WriteMetaInfo(metaStream, &metaInfo);
    Admin_CloseMetaInfo(metaStream);
    if (indxStream) {
	Indx_Close(indxStream);
    }

    /* Flush final stuff if necessary */
    if ((retCode == T_SUCCESS) && (parms.flags & T_SYNC)) {
	Admin_SetCurTBuf(archPath, curTBufId+1);
	FlushTBuf(curTBufId, &parms, archPath);
    }

    Sock_SendRespHdr(sock, retCode, syserr);

    sprintf(printBuf,"Done. KBytes: user %d, indx %d. Files: put %d, skipped %d. RetCode %d, errno %d.\n",
	    userKBytes, indxBytes>>10, fileCnt, skipCnt, retCode, syserr);
    Log_AtomicEvent("jupdate", printBuf, logPath);
    if (jDebug) {
	fprintf(stderr,"%s %s", parms.arch, printBuf);
    }

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * GetFileIndx --
 *
 *	Check indx to see if we really need to archive the file
 *
 * Results:
 *      "don't archive flag".
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
GetFileIndx(statInfoPtr, indxPath, archPath, callerPtr)
    T_FileStat *statInfoPtr;  /* File in question */
    char *indxPath;           /* pathname of indx file */
    char *archPath;           /* pathname of archive */
    Caller *callerPtr;        /* caller id */
{
    static QuerySpec query = {
	-1, -1,               /* versions */
	(time_t)-1, (time_t)-1, /* dates */
	"", "",               /* id's */
	0,                    /* flags */
	0,                    /* recursion */
	(regexp *)NULL,       /* compiled abstract */
	""                    /* filename */
	};
    ReceiveBlk receiveBlk;
    int retCode;

    query.fileName = STRRCHR(statInfoPtr->fileName, '/') + 1;
    receiveBlk.statInfoPtr = statInfoPtr;

    retCode = Indx_Match(&query, indxPath, archPath, callerPtr,
			 ReceiveIndxProc, (int *)&receiveBlk, 0);
    return retCode;
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
ReceiveIndxProc(oldInfoPtr, callVal)
    T_FileStat *oldInfoPtr;   /* Index information */
    int *callVal;             /* Callback info block */
{
    int dontArchive = 0;
    ReceiveBlk *receiveBlkPtr = (ReceiveBlk *)callVal;
    T_FileStat *statInfoPtr = receiveBlkPtr->statInfoPtr;
    int oldMode = oldInfoPtr->mode;
    int mode = statInfoPtr->mode;

    if (oldMode == mode) {
	if (S_ISADIR(statInfoPtr->mode)) {
	    if (strcmp(statInfoPtr->fileList,oldInfoPtr->fileList) == 0) {
/*
		if (jDebug) {
		    fprintf(stderr, "RecvIndxProc: dir unchanged\n");
		}
*/
		dontArchive = 1;	    
	    }
	} else {
	    if (Time_Compare(statInfoPtr->mtime,oldInfoPtr->vtime,0) <= 0) {
/*
		if (jDebug) {
		    fprintf(stderr, "RecvIndxProc: file unchanged\n");
		}
*/
		dontArchive = 1;
	    }
	}
    }

    return dontArchive;

}



/*
 *----------------------------------------------------------------------
 *
 * ReadOneFile --
 *
 *	Slurp a filename and its contents from socket into workFile
 *
 * Results:
 *      return code.
 *
 * Side effects:
 *	None.
 *
 *
 *----------------------------------------------------------------------
 */

static int
ReadOneFile(sockStream, destStream, byteCntPtr, statInfoPtr)
    int sockStream;           /* possible source descriptor */
    int destStream;           /* destination descriptor */
    int *byteCntPtr;          /* number of bytes written (updated) */
    T_FileStat *statInfoPtr;  /* file information (updated) */
{
    int retCode = T_SUCCESS;
    char buf[T_BUFSIZE];
    int count;
    int byteCnt;
    int size = statInfoPtr->size;
    int srcStream = sockStream;

    *byteCntPtr = 0;
    byteCnt = TBuf_WriteTarHdr(destStream, statInfoPtr);

#ifdef sprite
    if ((parms.flags & T_LOCAL) && (size > 0)) {
	if ((srcStream=open(statInfoPtr->fileName, O_RDONLY, 0)) == -1) {
	    syserr = errno;
	    sprintf(printBuf,"Couldn't open file %s. errno %d\n",
		    statInfoPtr->fileName, syserr);
	    Log_AtomicEvent("jupdate", printBuf, logPath);
	    size = 0;
	}
    }
#endif
    while (size > 0) {
	count = (size > sizeof(buf)) ? sizeof(buf) : size;
	if (Sock_ReadNBytes(srcStream, buf, count) != count) {
	    sprintf(printBuf,"Read %d bytes for %s failed. errno %d\n",
		    count, statInfoPtr->fileName, syserr);
	    Log_AtomicEvent("jupdate", printBuf, logPath);
	    return T_IOFAILED;
	}
	if (write(destStream, buf, count) != count) {
	    sprintf(printBuf,"Write %d bytes for %s failed. errno %d\n",
		    count, statInfoPtr->fileName, syserr);
	    Log_AtomicEvent("jupdate", printBuf, logPath);
	    return T_IOFAILED;
	}
	size -= count;
	byteCnt += count;
    }

#ifdef sprite
    if (srcStream != sockStream) {
	close(srcStream);
    }
#endif

    count = TBuf_Pad(destStream, byteCnt, T_TBLOCK);
    byteCnt += count;

    *byteCntPtr = byteCnt;

    return retCode;
}



/*
 *----------------------------------------------------------------------
 *
 * FlushTBuf --
 *
 *	Put tbuf out on media.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Erases tbuf if space is below threshold.
 *
 *----------------------------------------------------------------------
 */

static int
FlushTBuf(tBufId, parmsPtr, archPath)
    int tBufId;               /* buffer Identifier */
    Parms *parmsPtr;          /* parameter block */
    char *archPath;           /* current archive */
{
    int pid;
    union wait status;
    char buf[20];
    char buf2[20];
    char buf3[20];

    if (jDebug) {
	fprintf(stderr,"FlushTBuf: flushing %d\n", tBufId);
    }

    /*
     * If sync not needed and a cleaner already exists we're done.
     * There's a race here but it doesn't matter; sync hasn't been
     * requested and the buffer will go out by the timed cleaner later.
     */
    if (!(parmsPtr->flags & T_SYNC) && (!Admin_AvailVolInfo(archPath))) {
	return T_SUCCESS;
    }

    /* Remove any dead beasties ... */
    ReapChildren();

    /* Need to fork a cleaner process */
    if ((pid=fork()) == -1) {
	syserr = errno;
	sprintf(printBuf, "Couldn't fork cleaner. errno %d\n", syserr);
	Log_AtomicEvent("FlushTBuf", printBuf, logPath);
	return T_FAILURE;
    } else if (pid == 0) {
	MEM_CONTROL(0, NULL, 0, 0);
	close(parmsPtr->sock);
	sprintf(buf, "%d", tBufId);
	sprintf(buf2, "%d", parmsPtr->diskLow);
	sprintf(buf3, "%d", parmsPtr->diskHigh);
	execlp(parmsPtr->cleaner, parmsPtr->cleaner,
	       "-root", parmsPtr->root,
	       "-arch", parmsPtr->arch,
	       "-tbufid", buf,
	       "-disklow", buf2,
	       "-diskhigh", buf3,
	       "-username", parmsPtr->userName,
	       "-hostname", parmsPtr->hostName,
	       "-debug", (jDebug) ? "1" : "0",
	       "-newvol", (parmsPtr->flags & T_NEWVOL) ? "1" : "0",
	       NULL);
	sprintf(printBuf, "Failed to exec %s: errno %d\n",
		parmsPtr->cleaner, errno);
	Log_AtomicEvent("FlushTBuf", printBuf, logPath);
	_exit(-1);
    }

    parmsPtr->flags &= ~T_NEWVOL;

    /* wait around if we were so instructed ... */
    if ((parmsPtr->flags & T_SYNC) &&
	((wait(&status) != pid) || (status.w_T.w_Retcode != 0))) {
	syserr = errno;
	sprintf(printBuf,"Cleaner retcode %d. Wait errno %d\n",
		status.w_T.w_Retcode, syserr);
	Log_AtomicEvent("FlushTBuf", printBuf, logPath);
	return T_FAILURE;
    } else {
	return T_SUCCESS;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * SortDirectoryList --
 *
 *	Put directory list in canonical form.
 *
 * Results:
 *	Return code.
 *
 * Side effects:
 *	Allocates space for sorted list.
 *
 * Note:
 *      Canonical form is comma-separated, sorted list
 *      of names surrounded by curlies. This is so Str_Match
 *      will do the right thing.
 *
 *----------------------------------------------------------------------
 */

static char *
SortDirectoryList(list)
    char *list;               /* list of items to be sorted */
{
    char **partsList;
    int partsCnt;
    char *bufPtr = NULL;
    char *workPtr;
    char *quotedString;
    int len;
    int i;
    char *insidePtr;

    quotedString = Str_Quote(list, METACHARS);
    MEM_FREE("SortDirectoryList", list);
    len = strlen(quotedString) + 3;
    partsList = Str_Split(quotedString, ' ', &partsCnt, 1, &insidePtr);
    MEM_FREE("SortDirectoryList", quotedString);

    if (partsCnt > 0) {
	qsort((char *)partsList, partsCnt, sizeof(char *), StringCompareProc);
	workPtr = bufPtr = MEM_ALLOC("SortDirectoryList", len);

	*workPtr++ = '{';
	for (i=0; i<partsCnt; i++) {
	    strcpy(workPtr, partsList[i]);
	    workPtr += strlen(partsList[i]);
	    *workPtr++ = ',';
	}
	*(workPtr-1) = '}';
	*workPtr = '\0';
    } else {
	bufPtr = Str_Dup("{}");
    }

    MEM_FREE("SortDirectoryList", partsList);
    MEM_FREE("SortDirectoryList", insidePtr);

    return bufPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * StringCompareProc --
 *
 *	Compare to string pointers.
 *
 * Results:
 *	<1, 0, >1
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      This is a callback routine used by qsort.
 *
 *----------------------------------------------------------------------
 */

static int
StringCompareProc(a, b)
    char **a;                 /* string item 1 */
    char **b;                 /* string item 2 */
{
    return strcmp(*a, *b);
}


/*
 *----------------------------------------------------------------------
 *
 * ReapChildren --
 *
 *	Collect cadavers
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Reaps cleaner child.
 *
 * Note:
 *      Catching children with sigchld would interrupt all the socket
 *      I/O going on and I don't want to have to restart that stuff.
 *
 *----------------------------------------------------------------------
 */

static void
ReapChildren()
{
    int pid;
    union wait wstatus;

    while ((pid=wait3(&wstatus, WNOHANG, NULL)) > 0) {
	if (wstatus.w_T.w_Retcode != 0) {
	    sprintf(printBuf,"Cleaner retcode %d\n",
		    wstatus.w_T.w_Retcode);
	    Log_AtomicEvent("Jupdate", printBuf, logPath);
	}
    }
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
	Log_AtomicEvent("jupdate", printBuf, logPath);
	Sock_SendRespHdr(sock, retCode, syserr);
	return T_NOACCESS;
    }

    if ((perm != 'W') && (perm != 'O')) {
	sprintf(printBuf,"Access denied: %s %s from %s\n",
		parms.userName, parms.groupName, parms.hostName);
	Log_AtomicEvent("jupdate", printBuf, logPath);
	Sock_SendRespHdr(sock, T_NOACCESS, 0);
	return T_NOACCESS;
    }

    /* Give the go-ahead message */
    Sock_SendRespHdr(sock, T_SUCCESS, 0);

    /*
     * if user is owner, change his name to '*' so Str_Match succeeds.
     */
    if (perm == 'O') {
	*(callerPtr->userName) = '*';
	*(callerPtr->userName+1) = '\0';
    }

    return T_SUCCESS;
}

/* debug only */
static int
GetIncTime(init)
    int init;
{
    static struct timeval lastTime;
    static struct timeval curTime;
    int sec,usec;

    if (init) {
        gettimeofday(&lastTime, NULL);
	return 0;
    } else {
        gettimeofday(&curTime, NULL);
	sec = curTime.tv_sec - lastTime.tv_sec;
	if (curTime.tv_usec < lastTime.tv_usec) {
	  sec--;
	  usec = 1000000 - (lastTime.tv_usec - curTime.tv_usec);
	} else {
	  usec = curTime.tv_usec - lastTime.tv_usec;
        }
/*
	fprintf(stderr,"last: %x %x\n cur: %x %x\n",
		lastTime.tv_sec, lastTime.tv_usec,
		curTime.tv_sec, curTime.tv_usec);
*/
	lastTime = curTime;
	return (sec*1000+usec/1000);
    }
}

/* debug only */
static int
GetMsecs()
{
    static struct timeval curTime;
    int sec,usec;

    gettimeofday(&curTime, NULL);
    return (curTime.tv_sec*1000+curTime.tv_usec/1000);
}
