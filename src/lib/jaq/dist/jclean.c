/* 
 * jclean.c --
 *
 *	Write full tape buffers to archive device.
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
 *      If you understand, things are as they are.
 *      If you do not understand, things are as they are.
 *      -- Gensha, Zen Master
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jclean.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"
#include "jcleanInt.h"

typedef struct MgrData {
    int sock;
    int volId;
    int stream;
    int mgrPort;
    char *mgrServer;
} MgrData;

static void ProcessArchive _ARGS_ ((char *archPath, int tBufId));
static int  ProcessTBuf    _ARGS_ ((char *archPath, int tBufId,
				    MetaInfo *metaInfoPtr,
				    VolInfo *volInfoPtr,
				    FILE *volStream,
				    int outStream));
static int  PrepareVolume  _ARGS_ ((VolInfo *volInfoPtr,
				    MetaInfo *metaInfoPtr, int tBufId,
				    MgrData *mgrDataPtr, char *archPath,
				    FILE *volStream));
static int  MgrAcquire     _ARGS_ ((MgrData *mgrDataPtr));
static int  MgrRelease     _ARGS_ ((MgrData *mgrDataPtr));
static int  PrepareHdrFile _ARGS_ ((int stream, MetaInfo *metaInfoPtr,
				    int tBufId, char *archPath));
static int  PrepareBufFile _ARGS_ ((int stream, MetaInfo *metaInfoPtr,
				    int tBufId, char *archPath));
static void MakeSpace      _ARGS_ ((char *archPath));
static int  LogTBufBatch   _ARGS_ ((int volStream, Volinfo *volInfoPtr,
				    int firstTBuf, int lastTBuf));
static int  AssignNewVolume _ARGS_ (());


static char printBuf[T_MAXSTRINGLEN];
static char *logPath;         /* pathname of log file */
static FILE *memDbg = NULL;   /* stream for memory tracing */
static char archName[T_MAXSTRINGLEN];

int jDebug;                   /* Internal debugging only */
int syserr = 0;               /* Our personal record of errno */

Parms parms = {
    DEF_ARCHLIST,
    DEF_ROOT,
    DEF_USER,
    DEF_SERVER,
    DEF_DEBUG,
    DEF_TBUFID,
    DEF_NEWVOL,
    DEF_DISKLOW,
    DEF_DISKHIGH
};

Option optionArray[] = {
    {OPT_STRING, "arch", (char *)&parms.arch, "logical archive name"},
    {OPT_STRING, "root", (char *)&parms.root, "root of archive tree"},
    {OPT_STRING, "username", (char *)&parms.userName, "User issuing request"},
    {OPT_STRING, "hostname", (char *)&parms.hostName, "Machine name"},
    {OPT_INT, "debug", (char *)&parms.debug, "Enable debugging"},
    {OPT_INT, "tbufid", (char *)&parms.tBufId, "tbuf id to clean"},
    {OPT_INT, "newvol", (char *)&parms.newVol, "Start a new volume"},
    {OPT_INT, "disklow", (char *)&parms.diskLow, "Low percent disk full"},
    {OPT_INT, "diskhigh", (char *)&parms.diskHigh, "High percent disk full"}
};
int numOptions = sizeof(optionArray) / sizeof(Option);



/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main driver for cleaner program.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Sends tape buffers to device
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int argc;
    char *argv[];
{
    int retCode;
    int type;
    DIR *rootDirPtr;
    DirObject *entryPtr;
    struct stat statBuf;
    char archPath[T_MAXPATHLEN];
    static struct timeval curTime;    

/*    memDbg = fopen("jclean.mem","w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS+CHECKALLBLKS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    jDebug = parms.debug;

    if ((rootDirPtr=(DIR *)opendir(parms.root)) == (DIR *) NULL) {
	return 0;
    }

    while ((entryPtr=readdir(rootDirPtr)) != (DirObject *)NULL) {
	if ((*entryPtr->d_name != '.') &&
	    (Str_Match(entryPtr->d_name, parms.arch))) {
	    strcpy(archPath, parms.root);
	    strcat(archPath, "/");
	    strcat(archPath, entryPtr->d_name);
	    stat(archPath, &statBuf);
	    type = statBuf.st_mode & S_IFMT;
	    if (type == S_IFDIR) {
	        strcpy(archName, entryPtr->d_name);
		ProcessArchive(archPath, parms.tBufId);
	    }
	}
    }

    closedir(rootDirPtr);

    /* Clean out any old buffers, if diskspace is tight */
    MakeSpace();

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcessArchive -- 
 *
 *	Write out the buffers for specified archive
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	May issue mount requests to device synchronizer
 *
 *----------------------------------------------------------------------
 */

static void
ProcessArchive(archPath, requestedTBufId)
    char *archPath;           /* Pathname to desired archive. */
    int requestedTBufId;      /* desired buffer. -1 if unspecified */
{
    VolInfo volInfo;
    MetaInfo metaInfo;
    int tBufId;
    int curTBufId;
    int nextLogTBuf;
    int retCode = T_SUCCESS;
    int tBufCnt;
    int firstTBuf;
    int lastTBuf = -1;
    FILE *volStream;
    FILE *metaStream;
    Lock_Handle lockHandle;
    MgrData mgrData;
    ArchConfig archConfig;
    int i;

    mgrData.sock = -1;
    mgrData.volId = -1;
    mgrData.stream = -1;

    /* Try to get exclusive cleaner lock, else retire */
    if ((volStream=Admin_OpenVolInfo(archPath, &lockHandle))
	== (FILE *)NULL) {
	return;
    }

    logPath = Str_Cat(3, archPath, "/", ARCHLOGFILE);
    Log_AtomicEvent("Jclean", "Cleaning...", logPath);

    if (Admin_ReadArchConfig(archPath, &archConfig) != T_SUCCESS) {
	sprintf(printBuf, "Couldn't read archive config file. errno %d\n",
		syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
    }
    mgrData.mgrPort = archConfig.mgrPort;
    mgrData.mgrServer = archConfig.mgrServer;

    if (Admin_ReadVolInfo(volStream, &volInfo) != T_SUCCESS) {
	sprintf(printBuf, "Couldn't read volinfo file. errno %d\n",
		syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
    }
    firstTBuf = nextLogTBuf = volInfo.lastTBuf+1;

    /*
     * Now repeatedly run through files until there's nothing
     * left to clean.  
     */
    do {
	if (Admin_GetCurTBuf(archPath, &curTBufId) != T_SUCCESS) {
	    sprintf(printBuf, "Couldn't read current tbufd. errno %d\n",
		    syserr);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	    break;
	}
	tBufCnt = 0;
	if (volInfo.lastTBuf >= curTBufId) {
	    sprintf(printBuf, "Error: last tBuf %d, cur tBuf %d\n",
		    volInfo.lastTBuf,curTBufId);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	}
	for (tBufId=volInfo.lastTBuf+1; tBufId<curTBufId; tBufId++) {
	    metaStream = Admin_OpenMetaInfo(archPath, tBufId);
	    Admin_ReadMetaInfo(metaStream, &metaInfo);
	    Admin_CloseMetaInfo(metaStream);
	    if ((retCode=PrepareVolume(&volInfo, &metaInfo, tBufId,
			 &mgrData,archPath, volStream)) != T_SUCCESS) {
		sprintf(printBuf,"RetCode %d from PrepareVolume\n", retCode);
		Log_AtomicEvent("Jclean", printBuf, logPath);
		if (jDebug) {
		    fprintf(stderr,"%s", printBuf);
		}
		break;
	    }
	    if ((retCode=ProcessTBuf(archPath, tBufId, &metaInfo,&volInfo,
			     volStream, mgrData.stream)) != T_SUCCESS) {
		sprintf(printBuf,"RetCode %d from ProcessTBuf\n", retCode);
		Log_AtomicEvent("Jclean", printBuf, logPath);
		if (jDebug) {
		    fprintf(stderr,"(%d) %s", Time_Stamp(), printBuf);
		}
		break;
	    }
	    if ((tBufId-nextLogTBuf+1 >= LOGFREQ) &&
		((nextLogTBuf=LogTBufBatch(volStream, &volInfo,
					   nextLogTBuf, tBufId)) < 0)) {
	        break;
	    }
	    tBufCnt++;
	    lastTBuf = tBufId;
	}
    } while ((tBufCnt > 0) && (retCode == T_SUCCESS));

    if ((lastTBuf >= nextLogTBuf) &&
        ((nextLogTBuf=LogTBufBatch(volStream, &volInfo,
				   nextLogTBuf, lastTBuf)) < 0)) {
        lastTBuf = nextLogTBuf-1;
    }

    Admin_CloseVolInfo(&lockHandle, volStream);
    MgrRelease(&mgrData);

    if (lastTBuf == -1) {
	sprintf(printBuf, "No tbufs cleaned. retCode %d\n", retCode);
    } else {
        sprintf(printBuf, "Cleaned tbuf.%d to tbuf.%d. retCode %d\n",
	        firstTBuf, lastTBuf, retCode);
    }
    Log_AtomicEvent("Jclean", printBuf, logPath);
    if (jDebug) {
	fprintf(stderr, "(%d) %s", Time_Stamp(), printBuf);
    }

    MEM_FREE("ProcessArchive", logPath);
    MEM_REPORT("jclean", ALLROUTINES, SORTBYOWNER);
}



/*
 *----------------------------------------------------------------------
 *
 * ProcessTBuf -- 
 *
 *	Write a tbuf out to physical volume.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Consumes volume space.  Updating of volinfo file is not done
 *      for each tbuf individually, but in LOGFREQ batches for performance.
 *
 *----------------------------------------------------------------------
 */

static int
ProcessTBuf(archPath, tBufId, metaInfoPtr, volInfoPtr,
	    volStream, outStream)
    char *archPath;           /* archive path */
    int tBufId;               /* buffer id # */
    MetaInfo *metaInfoPtr;    /* buffer information */
    VolInfo *volInfoPtr;      /* volume information */
    FILE *volStream;          /* active vol info stream */
    int outStream;            /* active output stream */
{
    int tBufSize;
    int tHdrSize;
    char buf[T_TARSIZE];
    int sock;
    int retCode = T_SUCCESS;
    int tBufStream;
    int tHdrStream;
    int len;
    VolStatus volStatus;
    int totalLen;

    TBuf_Open(archPath, tBufId, &tBufStream, &tHdrStream, O_RDWR);

    if ((tHdrSize=PrepareHdrFile(tHdrStream, metaInfoPtr, tBufId, archPath)) < 0) {
        sprintf(printBuf, "Couldn't form hdr file. errno %d\n", syserr);
        Log_AtomicEvent("Jclean", printBuf, logPath);
	return T_FAILURE;
    }

    if ((tBufSize=PrepareBufFile(tBufStream, metaInfoPtr, tBufId, archPath)) < 0) {
        sprintf(printBuf, "Couldn't form buf file. errno %d\n", syserr);
        Log_AtomicEvent("Jclean", printBuf, logPath);
	return T_FAILURE;
    }

    /* Remaining volSpace will be decremented by this amount, */
    /* unless Dev_GetVolStatus can tell us more accurately. */
    totalLen = ((tHdrSize+tBufSize+(2*T_FILEMARKSIZE))>>10) + 1;

    if (jDebug) {
        fprintf(stderr,"(%d) jclean writing %d: hdr %d, buf %d\n",
		Time_Stamp(), tBufId, tHdrSize, tBufSize);
    }

    while (tHdrSize > 0) {
	len = (tHdrSize > sizeof(buf)) ? sizeof(buf) : tHdrSize;
	if (read(tHdrStream, buf, len) != len) {
	    sprintf(printBuf, "Short thdr read. errno %d\n", errno);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	    if (jDebug) {
	        fprintf(stderr, printBuf);
	    }
	    retCode = T_IOFAILED;
	    break;
	}
	if (Dev_WriteVolume(outStream, buf, sizeof(buf)) != sizeof(buf)) {
	    sprintf(printBuf, "Short thdr write. errno %d\n", syserr);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	    if (jDebug) {
	        fprintf(stderr, printBuf);
	    }
	    retCode = T_IOFAILED;
	    break;
	}
	tHdrSize -= len;
    }
    if (Dev_WriteEOF(outStream, 1) != T_SUCCESS) {
	sprintf(printBuf, "EOF after thdr failed. errno %d\n", syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	if (jDebug) {
	    fprintf(stderr, printBuf);
	}
	retCode = T_IOFAILED;
    }

    while ((retCode == T_SUCCESS) && (tBufSize > 0)) {
	len = (tBufSize > sizeof(buf)) ? sizeof(buf) : tBufSize;
	if (read(tBufStream, buf, len) != len) {
	    sprintf(printBuf, "Short tbuf read. errno %d\n", errno);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	    if (jDebug) {
	        fprintf(stderr, printBuf);
	    }
	    retCode = T_IOFAILED;
	    break;
	}
        if (Dev_WriteVolume(outStream, buf, sizeof(buf)) != sizeof(buf)) {
	    sprintf(printBuf, "Short tbuf write. errno %d\n", syserr);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	    if (jDebug) {
	        fprintf(stderr, printBuf);
	    }
	    retCode = T_IOFAILED;
	    break;
	}
	tBufSize -= len;
    }
    if (Dev_WriteEOF(outStream, 1) != T_SUCCESS) {
	sprintf(printBuf, "EOF after tbuf failed. errno %d\n", syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	if (jDebug) {
	    fprintf(stderr, printBuf);
	}
	retCode = T_IOFAILED;
    }

    TBuf_Close(tBufStream, tHdrStream, -1);

    volStatus.remaining = volInfoPtr->volSpace - totalLen;
    if (Dev_GetVolStatus(outStream, &volStatus) != T_SUCCESS) {
	sprintf(printBuf, "Couldn't get volume status. errno %d\n", syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
    }
    if (retCode == T_SUCCESS) {
	volInfoPtr->lastTBuf = tBufId;
	volInfoPtr->filemark += 2;
	volInfoPtr->volSpace = volStatus.remaining;
    }
    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * LogTBufBatch -- 
 *
 *	Update volinfo log to acknowledge procesing of buffer batch.
 *
 * Results:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
LogTBufBatch(volStream, volInfoPtr, firstTBuf, lastTBuf)
    int volStream;            /* volinfo file */
    VolInfo *volInfoPtr;      /* log record data */
    int firstTBuf;            /* first tbuf Id in batch */
    int lastTBuf;             /* last tbuf Id in batch */
{
    if (Admin_WriteVolInfo(volStream, volInfoPtr) != T_SUCCESS) {
	sprintf(printBuf,"Couldn't update log: tbuf.%d to tbuf.%d\n",
		firstTBuf, lastTBuf);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	if (jDebug) {
	    fprintf(stderr, "%s", printBuf);
	}
	return -1;
    } else {
	TBuf_Delete(parms.root, archName, firstTBuf, lastTBuf);
	sprintf(printBuf,"Deleted: tbuf.%d to tbuf.%d\n",
		firstTBuf, lastTBuf);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	if (jDebug) {
	    fprintf(stderr, "%s", printBuf);
	}
        return lastTBuf+1;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * PrepareVolume
 *
 *	Get a new volume and seek to proper filemark
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	Causes socket chatter to jukebox mgr.
 *
 * Note:
 *      To minimize tape motion, this thing remembers
 *      the current volume and does nothing if new one's the same.
 *
 *----------------------------------------------------------------------
 */

static int 
PrepareVolume(volInfoPtr, metaInfoPtr, tBufId, mgrDataPtr,
	      archPath, volStream)
    VolInfo *volInfoPtr;      /* current volume situation */
    MetaInfo *metaInfoPtr;    /* current buffer situation */
    int tBufId;               /* current buffer number */
    MgrData *mgrDataPtr;      /* active info for jmgr */
    char *archPath;           /* current archive path */
    FILE *volStream;          /* current vol info stream */
{
    int retCode = T_SUCCESS;
    int totalLen;

    totalLen = ((metaInfoPtr->tHdrSize+
		 metaInfoPtr->tBufSize+
		 (2*T_FILEMARKSIZE))>>10) + 1;

    if ((volInfoPtr->volId == -1) ||
	((parms.newVol) && (parms.tBufId == tBufId)) ||
	(totalLen+SLOPSPACEK > volInfoPtr->volSpace)) {
	if ((volInfoPtr->volId=AssignNewVolume()) == -1) {
	    return T_NOVOLUME;
	}
	volInfoPtr->filemark = 0;
	volInfoPtr->volSpace = T_VOLSIZEK;
	if ((Admin_AddTBufId(archPath, volInfoPtr->volId, tBufId)
	     != T_SUCCESS) ||
	    (Admin_WriteVolInfo(volStream, volInfoPtr) != T_SUCCESS)) {
	    sprintf(printBuf, "Meta info failed for new volume %d\n",
		    volInfoPtr->volId);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	    return T_ADMFAILED;
	}
    }

    /* If same volume as before, do nothing */
    if (volInfoPtr->volId == mgrDataPtr->volId) {
	return retCode;
    }

    /* else release old volume if any */
    MgrRelease(mgrDataPtr);

    /* acquire the new volume */
    mgrDataPtr->volId = volInfoPtr->volId;
    if (MgrAcquire(mgrDataPtr) != T_SUCCESS) {
	return T_FAILURE;
    }

    sprintf(printBuf, "Seeking fd %d past filemark %d\n",
	    mgrDataPtr->stream, volInfoPtr->filemark);
    Log_AtomicEvent("Jclean", printBuf, logPath);
    if (jDebug) {
	fprintf(stderr,"(%d) %s", Time_Stamp(), printBuf);
    }

    if (Dev_SeekVolume(mgrDataPtr->stream, volInfoPtr->filemark,
		       DEV_ABSOLUTE) != T_SUCCESS) {
	sprintf(printBuf, "Couldn't seek to block %d: errno %d\n",
		volInfoPtr->filemark, syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	return T_IOFAILED;
    }
    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * AssignNewVolume --
 *
 *	Assign and verify new volume Id
 *
 * Results:
 *	New volume Id or -1
 *
 * Side effects:
 *	Updates free volume list
 *
 *----------------------------------------------------------------------
 */

static int
AssignNewVolume()
{
    int volId;
    int retCode;
    VolOwner *volOwnerPtr;

    while (1) {
	volId = -1;
	if ((retCode=Admin_GetFreeVol(parms.root, &volId)) != T_SUCCESS) {
	    if (retCode == T_NOVOLUME) {
		sprintf(printBuf, "No free volumes!\n");
	    } else {
		sprintf(printBuf, "Couldn't get new volume: errno %d\n",
			syserr);
	    }
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	    return -1;
	}
	/* Ok, got a volume, now do a sanity check */
	volOwnerPtr = Admin_FindVolOwner(volId, parms.root, "*.arch");
	if (volOwnerPtr->owner == NULL) {
	    sprintf(printBuf, "Assigned new volume %d\n", volId);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	    return volId;
	} else {
	    sprintf(printBuf, "Oh my. Assigned volume %d, but it's already in use by %s. Retrying...\n",
		    volId, volOwnerPtr->owner);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	}
    }
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
    MgrData *mgrDataPtr;      /* jmgr status info */
{
    int retCode = T_SUCCESS;
    char devName[T_MAXPATHLEN];
    char *devNamePtr = devName;
    int status =T_SUCCESS;
    int sleepSecs = 5;
    int retryCnt = 0;

    if ((mgrDataPtr->sock=
	 Sock_SetupSocket(mgrDataPtr->mgrPort,
			  mgrDataPtr->mgrServer, 0)) == -1) {
	sprintf(printBuf, "Cleaner couldn't connect to jmgr at %s port %d. errno %d\n",
		mgrDataPtr->mgrServer, mgrDataPtr->mgrPort, syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	if (Utils_SendMail(ARCHMASTER, printBuf, "alert") != 0) {
	    sprintf(printBuf, "Got return code %d mailing alert to %s\n",
		    retCode, ARCHMASTER);
	    Log_AtomicEvent("Jclean", printBuf, logPath);
	}
	return T_FAILURE;
    }

    retCode = Sock_WriteString(mgrDataPtr->sock, parms.userName, 0);
    retCode += Sock_WriteInteger(mgrDataPtr->sock, S_CMDLOCK);
    retCode += Sock_WriteInteger(mgrDataPtr->sock, mgrDataPtr->volId);
    retCode += Sock_ReadInteger(mgrDataPtr->sock, &status);

    if ((retCode != T_SUCCESS) || (status != T_SUCCESS)) {
	sprintf(printBuf,"Couldn't acquire vol %d. Status %d, errno %d\n",
		mgrDataPtr->volId, status, syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	return T_FAILURE;
    }

    Sock_ReadString(mgrDataPtr->sock, &devNamePtr, 0);
    sprintf(printBuf, "Acquired vol %d in device %s\n",
	    mgrDataPtr->volId, devName);
    Log_AtomicEvent("Jclean", printBuf, logPath);
    if (jDebug) {
	fprintf(stderr, "(%d) %s", Time_Stamp(), printBuf);
    }

    /* Must sleep here, waiting for drive to spin up...*/
    while (((mgrDataPtr->stream=Dev_OpenVolume(devName, O_RDWR)) == -1) &&
	   (retryCnt++ < 10)) {
	sleep(sleepSecs);
    }
    if (mgrDataPtr->stream == -1) {
	sprintf(printBuf, "Couldn't open device %s. errno %d\n",
		devName, syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	return T_IOFAILED;
    }
    if (jDebug) {
	fprintf(stderr, "(%d) Opened device %s, fd %d.\n",
		Time_Stamp(), devName, mgrDataPtr->stream);
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
    MgrData *mgrDataPtr;      /* jmgr status info */
{
    int retCode = T_SUCCESS;
    int status =T_SUCCESS;

    if (mgrDataPtr->sock == -1) {
	return;
    }

    sprintf(printBuf, "Releasing vol %d.\n", mgrDataPtr->volId);
    Log_AtomicEvent("Jclean", printBuf, logPath);
    if (jDebug) {
	fprintf(stderr, "%s", printBuf);
    }

    retCode = Sock_WriteString(mgrDataPtr->sock, parms.userName, 0);
    retCode += Sock_WriteInteger(mgrDataPtr->sock, S_CMDFREE);
    retCode += Sock_WriteInteger(mgrDataPtr->sock, mgrDataPtr->volId);
    retCode += Sock_ReadInteger(mgrDataPtr->sock, &status);
    if ((retCode != T_SUCCESS) || (status != T_SUCCESS)) {
	sprintf(printBuf,"Couldn't release vol %d. Status %d, errno %d\n",
		mgrDataPtr->volId, status, syserr);
	Log_AtomicEvent("Jclean", printBuf, logPath);
    }

    close(mgrDataPtr->sock);
    Dev_CloseVolume(mgrDataPtr->stream);

    mgrDataPtr->sock = -1;
    mgrDataPtr->volId = -1;
    mgrDataPtr->stream = -1;

    return retCode;
}



/*
 *----------------------------------------------------------------------
 *
 * PrepareHdrFile --
 *
 *	Build header file corresponding to user file
 *
 * Results:
 *	Byte count of file.
 *
 * Side effects:
 *	Writes data out to thdr.<tbufID>
 *
 *----------------------------------------------------------------------
 */

static int
PrepareHdrFile(tHdrStream, metaInfoPtr, tBufId, archPath)
    int tHdrStream;           /* outgoing descriptor */
    MetaInfo *metaInfoPtr;    /* meta data */
    int tBufId;               /* Number of this buffer */
    char *archPath;           /* Full path of archive */
{
    static T_FileStat hdrStatInfo =
	{NULL, "", "root", "root", "", "",
	     0, 0, 0, 0, 0, 0, 0100600, 0, 0, 0, 0, 0};
    char fullName[T_MAXPATHLEN];
    static int myUid = -1;
    static int myGid = -1;
    static char *myName;
    static char *myGroup;
    int padCnt;
    int termCnt;
    struct stat unixStatBuf;
    int tHdrSize = metaInfoPtr->tHdrSize;

    if (myUid == -1) {
	myUid = (int) geteuid();
	myGid = (int) getegid();
	myName = Utils_GetLoginByUid(myUid);
	myGroup = Utils_GetGroupByGid(myGid);
    }

    /* Verify header file against meta info */
    sprintf(printBuf, "%s/thdr.%d%c", archPath, tBufId, '\0');
    stat(printBuf, &unixStatBuf);
    if (unixStatBuf.st_size > tHdrSize) {
        sprintf(printBuf, "Truncating %s/thdr.%d from %d to %d\n",
		archPath, tBufId, unixStatBuf.st_size, tHdrSize);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	if (jDebug) {
	    fprintf(stderr, "%s", printBuf);
	}
        ftruncate(tHdrStream, tHdrSize);
	TBuf_Seek(tHdrStream, 0);
    }

    sprintf(fullName,"%s/thdr.%d%c", archName, tBufId, '\0');
    hdrStatInfo.fileName = fullName;
    hdrStatInfo.vtime = Time_GetCurDate();
    hdrStatInfo.mtime = hdrStatInfo.vtime;
    hdrStatInfo.atime = hdrStatInfo.vtime;
    hdrStatInfo.uid = myUid;
    hdrStatInfo.gid = myGid;
    hdrStatInfo.uname = myName;
    hdrStatInfo.gname = myGroup;
    hdrStatInfo.size = tHdrSize-T_TBLOCK;

    if (TBuf_WriteTarHdr(tHdrStream, &hdrStatInfo) < 1) {
        fprintf(stderr,"MakeTarHdr failed\n");
	return -1;
    }

    if (TBuf_Seek(tHdrStream, tHdrSize) != T_SUCCESS) {
        fprintf(stderr,"seek failed\n");
        return -1;
    }

    if ((padCnt=TBuf_Pad(tHdrStream, tHdrSize, T_TBLOCK)) < 0) {
        fprintf(stderr,"pad failed\n");
        syserr = errno;
        return -1;
    }

    tHdrSize += padCnt;
    if ((termCnt=TBuf_Terminate(tHdrStream, tHdrSize)) < 0) {
        fprintf(stderr,"terminate failed\n");
        return -1;
    }

    if (TBuf_Seek(tHdrStream, 0) != T_SUCCESS) {
        fprintf(stderr,"seek 2 failed\n");
        return -1;
    }

    return tHdrSize+termCnt;
}


/*
 *----------------------------------------------------------------------
 *
 * PrepareBufFile --
 *
 *	Make sure buffer is ready for output.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May truncate buffer.
 *
 *----------------------------------------------------------------------
 */

static int
PrepareBufFile(tBufStream, metaInfoPtr, tBufId, archPath)
    int tBufStream;           /* outgoing descriptor */
    MetaInfo *metaInfoPtr;    /* meta data */
    int tBufId;               /* Number of this buffer */
    char *archPath;           /* Full path of archive */
{
    struct stat unixStatBuf;
    int tBufSize = metaInfoPtr->tBufSize+metaInfoPtr->tBufPad;

    /* Verify buffer file against meta info */
    sprintf(printBuf, "%s/tbuf.%d%c", archPath, tBufId, '\0');
    stat(printBuf, &unixStatBuf);
    if (unixStatBuf.st_size > tBufSize) {
        sprintf(printBuf, "Truncating %s/tbuf.%d from %d to %d\n",
		archPath, tBufId, unixStatBuf.st_size, metaInfoPtr->tBufSize);
	Log_AtomicEvent("Jclean", printBuf, logPath);
	if (jDebug) {
	    fprintf(stderr, "%s", printBuf);
	}
	TBuf_Seek(tBufStream, metaInfoPtr->tBufSize);
	TBuf_Terminate(tBufStream, metaInfoPtr->tBufSize);
        ftruncate(tBufStream, tBufSize);
	TBuf_Seek(tBufStream, 0);
    }

    return tBufSize;
}


/*
 *----------------------------------------------------------------------
 *
 * MakeSpace --
 *
 *	Remove old buffers til disk usage drops below threshold.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes buffers from disk.
 *
 *----------------------------------------------------------------------
 */

static void
MakeSpace()
{
    char archPath[T_MAXPATHLEN];
    char deadArch[T_MAXPATHLEN];
    int deadTBufId;
    int curTBufId;
    int percentUsed;
    long blocksFree;
    Lock_Handle lockHandle;
    VolInfo volInfo;
    FILE *volStream;

    while (1) {
	Admin_GetDiskUse(parms.root, &percentUsed, &blocksFree);
	if ((percentUsed < parms.diskLow) ||
	    (Admin_RemoveLRU(parms.root, deadArch, &deadTBufId)
	     != T_SUCCESS)) {
	    break;
	} 
	strcpy(archPath, parms.root);
	strcat(archPath, "/");
	strcat(archPath, deadArch);
	if ((volStream=Admin_OpenVolInfo(archPath, &lockHandle)) != NULL) {
	    Admin_ReadVolInfo(volStream, &volInfo);
	    Admin_CloseVolInfo(&lockHandle, volStream);
	    if (deadTBufId <= volInfo.lastTBuf) {
		sprintf(printBuf,"MakeSpace: deleting %s/tbuf.%d\n",
			deadArch, deadTBufId);
		strcat(archPath, "/");
		strcat(archPath, ARCHLOGFILE);
		Log_AtomicEvent("jclean", printBuf, archPath);
		if (jDebug) {
		    fprintf(stderr, "%s", printBuf);
		}
		TBuf_Delete(parms.root, deadArch, deadTBufId, deadTBufId);
	    }
	}
    }
}

