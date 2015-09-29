/* 
 * admin.c--
 *
 *	Simple utility functions for archive handlers
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * Quote:
 *      "We should not introduce errors through sloppiness, we should do it
 *      carefully and systematically."
 *      -- Edsgar Dijkstra, A Discipline of Programming
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/admin.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

extern int jDebug;            /* Internal debugging only */
extern int syserr;            /* Our personal record of errno */

static char printBuf[T_MAXSTRINGLEN];

#define CNFGSUFFIX "/config"
#define VOLSUFFIX "/volinfo"
#define AUTHSUFFIX "/auth"
#define FILESUFFIX "/filemap"
#define CURSUFFIX "/tbuf"
#define LRUSUFFIX "tbuf.lru"
#define NEWSUFFIX "tbuf.lru.new"
#define FREESUFFIX "/freevols"
#define FREENEWSUFFIX ".new"
#define VCOMMENT "# vol Id, filemark num, vol space (K), last buf Id\n"
#define BCOMMENT "# buf size, hdr size, file cnt, buffer pad for tar\n"
#define MCOMMENT "# vol Id, first tBuf Id\n"
#define TCOMMENT "# Current tbuf number\n"
#define CCOMMENT "# jmgr server, jmgr port, target buffer size\n"
#define MINFREEVOLS 3


/*
 *----------------------------------------------------------------------
 *
 * Admin_CheckAuth --
 *
 *	Determine permissions for caller
 *
 * Results:
 *	
 *
 * Side effects:
 *	Allocates heap space.
 *
 *----------------------------------------------------------------------
 */

int
Admin_CheckAuth(archPath, callerPtr, permPtr)
    char *archPath;           /* full path or archive */
    Caller *callerPtr;        /* caller's id */
    char *permPtr;            /* receiving permissions */
{
    char pathName[T_MAXPATHLEN];
    FILE *archStream;
    char *buf;
    char namePattern[T_MAXLINELEN];
    char groupPattern[T_MAXLINELEN];
    char hostPattern[T_MAXLINELEN];
    char *userName = callerPtr->userName;
    char *groupName = callerPtr->groupName;
    char *hostName = callerPtr->hostName;
    char permString[T_MAXLINELEN];

    *permPtr = ' ';

    strcpy(pathName,archPath);
    strcat(pathName, AUTHSUFFIX);

    if ((archStream=fopen(pathName, "r")) == (FILE *)NULL) {
	syserr = errno;
	if (access(archPath, X_OK) == -1) {
	    fprintf(stderr, "access for %s failed\n", pathName);
	    return T_NOARCHIVE;
	} else {
	    fprintf(stderr, "open for %s failed: %d\n", pathName, syserr);
	    return T_NOACCESS;
	}
    }

    while ((buf=Utils_ReadLine(archStream, 1)) != NULL) {
	if (sscanf(buf, "%s %s %s %s",
		   namePattern, groupPattern, hostPattern, permString) != 4) {
	    break;
	}
	if ((Str_Match(hostName, hostPattern)) &&
	    (Str_Match(groupName, groupPattern)) &&
	    (Str_Match(userName, namePattern))) {
	    *permString = toupper(*permString);
	    if (*permString == 'O') {
		*permPtr = *permString;
		break;
	    } else if ((*permString != 'R') || (*permPtr != 'W')) {
		*permPtr = *permString;
	    }
	}
    } 


    fclose(archStream);

    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_AddAuth --
 *
 *	Add an entry to archive authorization file
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
Admin_AddAuth(archPath, callerPtr, perm)
    char *archPath;           /* full path or archive */
    Caller *callerPtr;        /* caller's id */
    char perm;                /* permission */
{
    char pathName[T_MAXPATHLEN];
    FILE *archStream;
    Lock_Handle lockHandle;

    strcpy(pathName,archPath);
    strcat(pathName, AUTHSUFFIX);

    if (Lock_Acquire(pathName, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    }

    archStream = fopen(pathName, "a");
    if (archStream == (FILE *)NULL) {
	syserr = errno;
	Lock_Release(&lockHandle);
	if (syserr == ENOENT) {
	    return T_NOARCHIVE;
	} else {
	    return T_FAILURE;
	}
    }

    fprintf(archStream, "%s %s %s %c\n",
	    callerPtr->userName, callerPtr->groupName, 
	    callerPtr->hostName, perm);

    Lock_Release(&lockHandle);
    fclose(archStream);

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * Admin_ReadArchConfig
 *
 *	Read the archive's config file
 *
 * Results:
 *	current config data
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_ReadArchConfig(archPath, archConfigPtr)
    char *archPath;           /* full path or archive */
    ArchConfig *archConfigPtr;     /* Receiving space */
{
    char pathName[T_MAXPATHLEN];
    FILE *archStream;
    char *buf;
    int retCode = T_SUCCESS;

    strcpy(pathName,archPath);
    strcat(pathName, CNFGSUFFIX);

    archStream = fopen(pathName, "r");
    if (archStream == (FILE *)NULL) {
	if (errno == ENOENT)  {
	    strcpy(archConfigPtr->mgrServer,DEF_MGRSERVER);
	    archConfigPtr->mgrPort = DEF_MGRPORT;
	    archConfigPtr->tBufSize = DEF_TBUFSIZE;
	    return Admin_WriteArchConfig(archPath, archConfigPtr);
	} else {
	    syserr = errno;
	}
    } else {
	buf = Utils_ReadLine(archStream, 1);
	if (sscanf(buf, "%s %d %d",
		   archConfigPtr->mgrServer,
		   &archConfigPtr->mgrPort,
		   &archConfigPtr->tBufSize) != 3) {
	    syserr = EINVAL;
	    retCode = T_ADMFAILED;
	}
	fclose(archStream);
    }

    return retCode;
	
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_WriteArchConfig
 *
 *	Write the current configuration info
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will (re)create file 'config' in logical archive
 *
 *----------------------------------------------------------------------
 */

int
Admin_WriteArchConfig(archPath, archConfigPtr)
    char *archPath;           /* full path or archive */
    ArchConfig *archConfigPtr;     /* Source info */
{
    char pathName[T_MAXPATHLEN];
    FILE *archStream;
    int retCode = T_SUCCESS;

    strcpy(pathName,archPath);
    strcat(pathName, CNFGSUFFIX);

    archStream = fopen(pathName, "w");
    if (archStream == (FILE *)NULL) {
	syserr = errno;
	retCode = T_ADMFAILED;
    } else {
	fprintf(archStream, "%s%s %d %d\n",
		CCOMMENT, archConfigPtr->mgrServer,
		archConfigPtr->mgrPort,	archConfigPtr->tBufSize);
	fclose(archStream);
    }

    return retCode;
	
}

/*
 *----------------------------------------------------------------------
 *
 * Admin_GetCurTBuf
 *
 *	Read the current tbuf number
 *
 * Results:
 *	current tbufId
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_GetCurTBuf(archPath, tBufIdPtr)
    char *archPath;           /* full path or archive */
    int *tBufIdPtr;           /* Receiving space */
{
    char pathName[T_MAXPATHLEN];
    FILE *archStream;
    char *buf;
    int retCode = T_SUCCESS;
    Lock_Handle lockHandle;

    strcpy(pathName,archPath);
    strcat(pathName, CURSUFFIX);

    if (Lock_Acquire(pathName, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    }

    archStream = fopen(pathName, "r");
    if (archStream == (FILE *)NULL) {
	if (errno == ENOENT)  {
	    Lock_Release(&lockHandle);
	    return Admin_SetCurTBuf(archPath, 0);
	}
    } else {
	buf = Utils_ReadLine(archStream, 1);
	sscanf(buf, "%d", tBufIdPtr);
	fclose(archStream);
	Lock_Release(&lockHandle);
    }

    return retCode;
	
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_SetCurTbuf
 *
 *	Write the current tbuf number
 *
 * Results:
 *	current tbufId
 *
 * Side effects:
 *	Will (re)create file 'tbuf' in logical archive
 *
 * Note:
 *      Locking necessary to synchronize writer and cleaner.
 *
 *----------------------------------------------------------------------
 */

int
Admin_SetCurTBuf(archPath, tBufId)
    char *archPath;           /* full path or archive */
    int tBufId;               /* buffer id */
{
    char pathName[T_MAXPATHLEN];
    FILE *archStream;
    int retCode = T_SUCCESS;
    Lock_Handle lockHandle;

    strcpy(pathName,archPath);
    strcat(pathName, CURSUFFIX);

    if (Lock_Acquire(pathName, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    }

    archStream = fopen(pathName, "w");
    if (archStream == (FILE *)NULL) {
	syserr = errno;
	retCode = T_ADMFAILED;
    } else {
	fprintf(archStream, "%s%d\n", TCOMMENT, tBufId);
	fsync(fileno(archStream));
	fclose(archStream);
    }

    Lock_Release(&lockHandle);

    return retCode;
	
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_AvailVolInfo --
 *
 *	Check to see if volinfo is available; ie whether
 *      a cleaner exists. This is used by jupdate so it
 *      knows whether it needs to spaqn a new cleaner.
 *      A race exists between the check and the action
 *      but it doesn't matter; it's just a performance hack.
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_AvailVolInfo(archPath)
    char *archPath;           /* full path of archive */
{
   char pathName[T_MAXPATHLEN];    
   int retCode;

   strcpy(pathName, archPath);
   strcat(pathName, VOLSUFFIX);
   strcat(pathName, ".LOCK");

   retCode = access(pathName, R_OK);

   return ((retCode == -1) ? 1 : 0);

}



/*
 *----------------------------------------------------------------------
 *
 * Admin_OpenVolInfo --
 *
 *	Open and lock the volume file for modifications
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

FILE *
Admin_OpenVolInfo(archPath, lockHandlePtr)
    char *archPath;           /* Full path of archive */
    Lock_Handle *lockHandlePtr; /* Receiving lock handle */
{
    char pathName[T_MAXPATHLEN];
    FILE *volStream;
    VolInfo vInfo;

    strcpy(pathName,archPath);
    strcat(pathName, VOLSUFFIX);

    if (Lock_Acquire(pathName, LOCK_NOBLOCK, lockHandlePtr) != T_SUCCESS) {
	return NULL;
    }

    volStream = fopen(pathName, "r+");
    if (volStream == (FILE *)NULL) {
	if (errno == ENOENT)  {
	    if ((volStream = fopen(pathName, "w+")) != (FILE *)NULL) {
		vInfo.volId = -1;
		vInfo.filemark = 0;
		vInfo.volSpace = -1;
		vInfo.lastTBuf = -1;
		Admin_WriteVolInfo(volStream, &vInfo);
	    }
	}
    }

    if (volStream == (FILE *)NULL) {
	syserr = errno;
	fprintf(stderr,"Admin_OpenVolInfo: couldn't open %s\n",pathName);
    }

    return volStream;

}


/*
 *----------------------------------------------------------------------
 *
 * Admin_CloseVolInfo --
 *
 *	Close and unlock the volume file
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_CloseVolInfo(lockHandlePtr, volStream)
    Lock_Handle *lockHandlePtr;  /* item to unlock */
    FILE *volStream;          /* associated file stream ptr */
{
    fclose(volStream);
    Lock_Release(lockHandlePtr);
    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * Admin_ReadVolInfo --
 *
 *	Read the current volume state 
 *
 * Results:
 *	Filled in structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_ReadVolInfo(volStream, vInfoPtr)
    FILE *volStream;          /* stream designator */
    VolInfo *vInfoPtr;        /* receiving structure */
{
    char *buf;
    int retCode = T_SUCCESS;

    rewind(volStream);
    if ((buf=Utils_ReadLine(volStream, 1)) == (char *)NULL) {
	fprintf(stderr,"Admin_ReadVolInfo: No data in volinfo file.\n");
	retCode = T_ADMFAILED;
    } else {
	sscanf(buf, "%d %d %d %d",
	       &vInfoPtr->volId, &vInfoPtr->filemark,
	       &vInfoPtr->volSpace, &vInfoPtr->lastTBuf);
    }

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_WriteVolInfo --
 *
 *	Update the current volume state 
 *
 * Results:
 *	return code.
 *
 * Side effects:
 *	Modifies archive data file.
 *
 *----------------------------------------------------------------------
 */

int
Admin_WriteVolInfo(volStream, vInfoPtr)
    FILE *volStream;          /* stream designator */
    VolInfo *vInfoPtr;        /* receiving structure */
{
    int retCode = T_SUCCESS;

    rewind(volStream);
    fprintf(volStream, "%s%d %d %d %d\n",
	    VCOMMENT, vInfoPtr->volId, vInfoPtr->filemark,
	    vInfoPtr->volSpace, vInfoPtr->lastTBuf);
    /*
     * Unfortunately, we must pay the price here because
     * we're just about to delete the buffer.
     */
    fsync(fileno(volStream));

    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * Admin_OpenMetaInfo --
 *
 *	Open the meta file
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

FILE *
Admin_OpenMetaInfo(archPath, tBufId)
    char *archPath;           /* full path of archive */
    int tBufId;               /* active tbuf id number */
{
    char pathName[T_MAXPATHLEN];
    FILE *metaStream;
    MetaInfo metaInfo;

    sprintf(pathName, "%s/meta.%d%c", archPath, tBufId, '\0');

    metaStream = fopen(pathName, "r+");
    if (metaStream == (FILE *)NULL) {
	if (errno == ENOENT)  {
	    if ((metaStream=fopen(pathName, "w+")) != (FILE *)NULL) {
	        metaInfo.tBufSize = 0;
	        metaInfo.tHdrSize = T_TBLOCK; /* Size of tar hdr. sorry */
	        metaInfo.fileCnt = 0;
	        metaInfo.tBufPad = 0;
	        Admin_WriteMetaInfo(metaStream, &metaInfo);
		fflush(metaStream);
	    }
	}
    }

    if (metaStream == (FILE *)NULL) {
	syserr = errno;
	fprintf(stderr,"Admin_OpenMetaInfo: couldn't open %s\n",pathName);
    }

    return metaStream;

}


/*
 *----------------------------------------------------------------------
 *
 * Admin_CloseMetaInfo --
 *
 *	Close meta file
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_CloseMetaInfo(metaStream)
    FILE *metaStream;         /* associated file stream ptr */
{
    fclose(metaStream);
    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_ReadMetaInfo --
 *
 *	Read the current buffer state 
 *
 * Results:
 *	Ptr to block of specified size or NULL;
 *
 * Side effects:
 *	Allocates heap space.
 *
 * Note:
 *     Locking shouldn't be necessary. Jupdate and Jclean 
 *     don't share the same tbufs.
 * 
 *----------------------------------------------------------------------
 */

int
Admin_ReadMetaInfo(metaStream, metaInfoPtr)
    FILE *metaStream;         /* associated file stream ptr */
    MetaInfo *metaInfoPtr;    /* receiving structure */
{
    char *buf;
    int retCode = T_SUCCESS;

    rewind(metaStream);
    if ((buf=Utils_ReadLine(metaStream, 1)) == (char *)NULL) {
	retCode = T_ADMFAILED;
    } else {
        sscanf(buf, "%d %d %d %d",
	       &metaInfoPtr->tBufSize,
	       &metaInfoPtr->tHdrSize,
	       &metaInfoPtr->fileCnt,
	       &metaInfoPtr->tBufPad);
    }

    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * Admin_WriteMetaInfo --
 *
 *	Update the current buffer state 
 *
 * Results:
 *	return code.
 *
 * Side effects:
 *	Modifies archive data file.
 *
 * Note:
 *     Locking shouldn't be necessary. Only Jupdate calls this routine.
 *     Jupdate decides whether or not to do an fsync.
 *
 *----------------------------------------------------------------------
 */

int
Admin_WriteMetaInfo(metaStream, metaInfoPtr)
    FILE *metaStream;         /* associated file stream ptr */
    MetaInfo *metaInfoPtr;    /* receiving structure */
{
    int retCode = T_SUCCESS;

    rewind(metaStream);
    fprintf(metaStream, "%s%d %d %d %d\n",
	    BCOMMENT, metaInfoPtr->tBufSize, metaInfoPtr->tHdrSize,
	    metaInfoPtr->fileCnt, metaInfoPtr->tBufPad);

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_CvtTBufId --
 *
 *	Convert a buffer number to a volId and filemark pair.
 *
 * Results:
 *	volume Id
 *      block Id
 *
 * Side effects:
 *	Modifies archive data file.
 *
 *----------------------------------------------------------------------
 */

int
Admin_CvtTBufId(archPath, tBufId, volIdPtr, filemarkPtr)
    char *archPath;           /* path to active archive */
    int tBufId;               /* source buffer id */
    int *volIdPtr;            /* receiving volume id */
    int *filemarkPtr;         /* receiving block id */
{
    char pathName[T_MAXPATHLEN];
    FILE *fileStream;
    int tmpVolId;
    int newTBufId;
    int tmpTBufId;
    int retCode = T_ADMFAILED;
    char *bufPtr;

    strcpy(pathName,archPath);
    strcat(pathName, FILESUFFIX);

    *volIdPtr = -1;
    *filemarkPtr = -1;

    fileStream = fopen(pathName, "r");
    if (fileStream == (FILE *)NULL) {
	retCode == T_SUCCESS;
    } else {
	while ((bufPtr=Utils_ReadLine(fileStream, 1)) != NULL) {
	    if (sscanf(bufPtr, "%d %d", &tmpVolId, &tmpTBufId) != 2) {
		break;
	    }
	    if (tBufId < tmpTBufId) {
		break;
	    }
	    *volIdPtr = tmpVolId;
	    newTBufId = tmpTBufId;
	}
	if (*volIdPtr != -1) {
	    *filemarkPtr = 2*(tBufId - newTBufId);
	    retCode = T_SUCCESS;
	}
	fclose(fileStream);
    }

    return retCode;

}



/*
 *----------------------------------------------------------------------
 *
 * Admin_AddTBufId --
 *
 *	Add a new volume-tBufId pair to list
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies archive data file.
 *
 *----------------------------------------------------------------------
 */

int
Admin_AddTBufId(archPath, volId, tBufId)
    char *archPath;           /* archive */
    int volId;                /* new volume */
    int tBufId;               /* starting tbuf id */
{
    char pathName[T_MAXPATHLEN];
    FILE *fileStream;
    int retCode = T_SUCCESS;
    Lock_Handle lockHandle;
    time_t curDate;

    strcpy(pathName,archPath);
    strcat(pathName, FILESUFFIX);

    if (Lock_Acquire(pathName, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    }

    fileStream = fopen(pathName, "a");
    if (fileStream == (FILE *)NULL) {
	syserr = errno;
	fprintf(stderr,"Admin_AddTBufId: couldn't open %s\n",pathName);
	retCode = T_ADMFAILED;
    } else {
	if (ftell(fileStream) == 0) {
	    fprintf(fileStream, "%s", MCOMMENT);
	}
	curDate = Time_GetCurDate();
	fprintf(fileStream, "%d %d %s\n",
		volId, tBufId, Time_CvtToString(&curDate));
	fclose(fileStream);
    }

    Lock_Release(&lockHandle);
    return retCode;

}

/*
 *----------------------------------------------------------------------
 *
 * Admin_UpdateLRU --
 *
 *	Update tbuf LRU list
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	Rewrites LRU list
 *
 *----------------------------------------------------------------------
 */

int
Admin_UpdateLRU(rootPath, archive, firstTBuf, lastTBuf)
    char *rootPath;           /* path to root */
    char *archive;            /* archive name */
    int firstTBuf;            /* fist tBufId to update */
    int lastTBuf;             /* last tBufId to update */
{
    FILE *lruStream;
    FILE *newStream;
    char lruPath[T_MAXPATHLEN];
    char newPath[T_MAXPATHLEN];
    char oldArch[T_MAXPATHLEN];
    int tBufId;
    char *bufPtr;
    Lock_Handle lockHandle;

    strcpy(lruPath, rootPath);
    strcat(lruPath, "/");
    strcat(lruPath, LRUSUFFIX);

    strcpy(newPath, rootPath);
    strcat(newPath, "/");
    strcat(newPath, NEWSUFFIX);

    if (Lock_Acquire(lruPath, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    }

    lruStream = fopen(lruPath, "r");
    if (lruStream == (FILE *)NULL) {
	if (errno != ENOENT) {
	    Lock_Release(&lockHandle);
	    return -1;
	}
    }

    newStream = fopen(newPath, "w");
    if (newStream == (FILE *)NULL) {
	syserr = errno;
	Lock_Release(&lockHandle);
	return T_FAILURE;
    }

    for (tBufId=lastTBuf; tBufId >= firstTBuf; tBufId--) {
        fprintf(newStream, "%s %d\n", archive, tBufId);
    }

    if (lruStream != (FILE *)NULL) {
	while ((bufPtr=Utils_ReadLine(lruStream, 0)) != NULL) {
	    sscanf(bufPtr,"%s %d\n", oldArch, &tBufId);
	    if ((tBufId < firstTBuf) ||
		(tBufId > lastTBuf) ||
		(strcmp(oldArch,archive) != 0)) {
		fputs(bufPtr, newStream);
	    }
	}
	fclose(lruStream);
	unlink(lruPath);
    }

    fclose(newStream);
    rename(newPath, lruPath);
    Lock_Release(&lockHandle);
    return T_SUCCESS;

}



/*
 *----------------------------------------------------------------------
 *
 * Admin_RemoveLRU --
 *
 *	Remove a tape buffer from LRU list
 *
 * Results:
 *	return code
 *
 * Side effects:
 *	Rewrites LRU list
 *
 *----------------------------------------------------------------------
 */

int 
Admin_RemoveLRU(rootPath, archive, tBufIdPtr)
    char *rootPath;           /* path to this archive */
    char *archive;            /* ptr to receiving archive */
    int *tBufIdPtr;           /* receiving tbuf number */
{
    FILE *lruStream;
    char lruPath[T_MAXPATHLEN];
    char *bufPtr;
    int pos = 0;
    int oldPos;
    int retCode = T_FAILURE;
    Lock_Handle lockHandle;

    strcpy(lruPath, rootPath);
    strcat(lruPath, "/");
    strcat(lruPath, LRUSUFFIX);

    if (Lock_Acquire(lruPath, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    }

    lruStream = fopen(lruPath, "r");
    syserr = errno;

    if (lruStream != (FILE *)NULL) {
	while ((bufPtr=Utils_ReadLine(lruStream, 1)) != NULL) {
	    sscanf(bufPtr, "%s %d", archive, tBufIdPtr);
	    oldPos = pos;
	    pos = ftell(lruStream);
	    retCode = T_SUCCESS;
	}
	fclose(lruStream);
    }

    if (retCode == T_SUCCESS) {
	truncate(lruPath, oldPos);
    }

    Lock_Release(&lockHandle);
    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * Admin_GetDiskUse --
 *
 *	Figure out how much of the disk is in use
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      Really need to take into account # of free inodes
 *      and disk quota, if any, but too much trouble.
 *
 *----------------------------------------------------------------------
 */

#ifdef sprite
#include <sys/types.h>
#include <sys/stat.h>
#include <sprite.h>
#include <fs.h>
#include <sysStats.h>

int
Admin_GetDiskUse(fileName, percentUsedPtr, blocksFreePtr)
    char *fileName;           /* File system to check */
    int *percentUsedPtr;      /* Percent of banks free */
    long *blocksFreePtr;      /* Number of free blocks */
{
    Fs_Prefix prefix;
    struct stat unixStatBuf;
    int tot;
    int numDomain = 0;

    if (stat(fileName, &unixStatBuf) < 0) {
	return T_FAILURE;
    }
	    
    while (Sys_Stats(SYS_FS_PREFIX_STATS, numDomain++, (Address) &prefix)
	   == SUCCESS) {
	if ((prefix.serverID == unixStatBuf.st_serverID) &&
	    (prefix.domain == ((int) unixStatBuf.st_dev))) {
	    tot = 0.9 * prefix.domainInfo.maxKbytes;
	    *blocksFreePtr = prefix.domainInfo.freeKbytes -
		(0.1 * prefix.domainInfo.maxKbytes);
	    *percentUsedPtr = (int) (100.0 * ((tot-*blocksFreePtr) /
					      (float) tot));
	    return T_SUCCESS;
	}
    }

    return 0;

}

#endif

#ifdef ultrix

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>

int
Admin_GetDiskUse(fileName, percentUsedPtr, blocksFreePtr)
    char *fileName;           /* File system to check */
    int *percentUsedPtr;      /* Percent of banks free */
    long *blocksFreePtr;      /* Number of free blocks */
{
    struct fs_data statFsBuf;
    int tot;

    if (statfs(fileName, &statFsBuf) == -1) {
	return T_FAILURE;
    } else {
	tot = statFsBuf.fd_req.btot * 0.9; /* 10% is reserved */
	*blocksFreePtr = statFsBuf.fd_req.bfreen;
	*percentUsedPtr = (int) (100.0 * ((tot-*blocksFreePtr) /
					  (float) tot));
	return T_SUCCESS;
    }

}

#endif

#if (!defined(sprite) && !defined(ultrix))

#include <sys/types.h>
#ifdef OSF1
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif
int
Admin_GetDiskUse(fileName, percentUsedPtr, blocksFreePtr)
    char *fileName;           /* File system to check */
    int *percentUsedPtr;      /* Percent of banks free */
    long *blocksFreePtr;      /* Number of free blocks */
{
    struct statfs statFsBuf;
    long tot;

    if (statfs(fileName, &statFsBuf) == -1) {
	return T_FAILURE;
    } else {
	tot = statFsBuf.f_blocks;
	*blocksFreePtr = statFsBuf.f_bavail;
	*percentUsedPtr = (int) (100.0 * ((tot-*blocksFreePtr) /
					  (float) tot));
	return T_SUCCESS;
    }

}

#endif


/*
 *----------------------------------------------------------------------
 *
 * Admin_FindVolOwner --
 *
 *	Scan the filemaps to locate owner of specified volume
 *
 * Results:
 *	ptr to static string or NULL
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

VolOwner *
Admin_FindVolOwner(volId, root, archPattern)
    int volId;                /* volume of interest */
    char *root;               /* root of tree */
    char *archPattern;        /* archives to search */
{
    char pathName[T_MAXPATHLEN];
    FILE *fileStream;
    DIR *rootDirPtr;
    DirObject *entryPtr;
    struct stat unixStatBuf;
    char *bufPtr;
    int tmpTBufId;
    int tmpVolId;
    static VolOwner volOwner;

    volOwner.owner = NULL;
    volOwner.minTBuf = -1;
    volOwner.maxTBuf = -1;

    if ((rootDirPtr=(DIR *)opendir(root)) == (DIR *) NULL) {
	return NULL;
    }

    while ((entryPtr=readdir(rootDirPtr)) != (DirObject *)NULL) {
	if ((*entryPtr->d_name != '.') &&
	    (Str_Match(entryPtr->d_name, archPattern))) {
	    strcpy(pathName, root);
	    strcat(pathName, "/");
	    strcat(pathName, entryPtr->d_name);
	    stat(pathName, &unixStatBuf);
	    if (!S_ISADIR(unixStatBuf.st_mode)) {
	        continue;
	    }
	    strcat(pathName, FILESUFFIX);
	    fileStream = fopen(pathName, "r");
	    if (fileStream != (FILE *)NULL) {
		while ((bufPtr=Utils_ReadLine(fileStream, 1)) != NULL) {
		    if (sscanf(bufPtr, "%d %d", &tmpVolId, &tmpTBufId) != 2) {
		        break;
		    }
		    if (tmpVolId == volId) {
		        volOwner.owner = Str_Dup(entryPtr->d_name);
			volOwner.minTBuf = tmpTBufId;
			volOwner.maxTBuf = INT_MAX;
			if (((bufPtr=Utils_ReadLine(fileStream, 1)) != NULL) &&
			    (sscanf(bufPtr, "%d %d",
				    &tmpVolId, &tmpTBufId)) == 2) {
			    volOwner.maxTBuf = tmpTBufId-1;
			}
			break;
		    }
	        }
	        fclose(fileStream);
	    }
	}
    }

    closedir(rootDirPtr);

    return &volOwner;
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_ReadDevConfig --
 *
 *	Read the device configuration file
 *
 * Results:
 *	ptr to dynamic array.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_ReadDevConfig(devFile, devList, cntPtr)
    char *devFile;            /* config file name */
    DevConfig *devList;       /* array space */
    int *cntPtr;              /* # slots (on input); # items (on output)*/
{
    int retCode = T_SUCCESS;
    FILE *configFile;
    char *bufPtr;
    char *locPtr;
    DevConfig *itemPtr = devList;
    int devCnt = 0;

    if ((configFile=fopen(devFile, "r")) == NULL) {
	syserr = errno;
	return T_FAILURE;
    }
    
    while ((bufPtr=Utils_ReadLine(configFile, 0)) != NULL) {
	devCnt++;
    }

    if (devCnt > *cntPtr) {
	syserr = ENOSPC;
	*cntPtr = devCnt;
	fclose(configFile);
	return T_FAILURE;
    }

    rewind(configFile);

    while ((bufPtr=Utils_ReadLine(configFile, 1)) != NULL) {
	locPtr = bufPtr;
	while ((*locPtr) && !isspace(*locPtr)) {
	    locPtr++;
	}
	*locPtr++ = '\0';
	strcpy(itemPtr->name,bufPtr);
	if (Utils_CvtInteger(locPtr, 0, 5000, &itemPtr->location) != T_SUCCESS) {
	    errno = 0;
	    retCode = T_ADMFAILED;
	    break;
	}
	itemPtr++;
    }

    fclose(configFile);
    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_WriteDevConfig --
 *
 *	Write the device configuration file
 *
 * Results:
 *	return code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_WriteDevConfig(devFile, devList, cnt)
    char *devFile;            /* device file */
    DevConfig *devList;       /* array */
    int cnt;                  /* # slots */
{
    FILE *configFile;
    int retCode = T_SUCCESS;
    DevConfig *itemPtr = devList;

    if ((configFile=fopen(devFile, "w")) == NULL) {
	syserr = errno;
	return T_FAILURE;
    }

    while (cnt-- > 0) {
	fprintf(configFile, "%s %d", itemPtr->name, itemPtr->location);
	itemPtr++;
    }

    fclose(configFile);
    return retCode;

}

/*
 *----------------------------------------------------------------------
 *
 * Admin_ReadVolConfig --
 *
 *	Read the volume configuration file
 *
 * Results:
 *	return code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_ReadVolConfig(volFile, volList, cntPtr)
    char *volFile;            /* config file */
    VolConfig *volList;       /* array space */
    int *cntPtr;              /* # slots (on input); # items (on output)*/
{
    FILE *configFile;
    int volCnt = 0;
    char *bufPtr;
    int retCode = T_SUCCESS;
    VolConfig *itemPtr = volList;

    if ((configFile=fopen(volFile, "r")) == NULL) {
	syserr = errno;
	return T_FAILURE;
    }
    
    while ((bufPtr=Utils_ReadLine(configFile, 0)) != NULL) {
	volCnt++;
    }

    if (volCnt > *cntPtr) {
	syserr = ENOSPC;
	*cntPtr = volCnt;
	fclose(configFile);
	return T_FAILURE;
    }

    rewind(configFile);

    while ((bufPtr=Utils_ReadLine(configFile, 1)) != NULL) {
	if ((sscanf(bufPtr, "%s %d", 
		    itemPtr->volLabel, &itemPtr->location) != 2) ||
	    ((itemPtr->volId=Dev_CvtVolLabel(itemPtr->volLabel)) == -1)) {
	    syserr = EINVAL;
	    retCode = T_FAILURE;
	    break;
	}
	itemPtr++;
    }

    fclose(configFile);
    return retCode;
}

/*
 *----------------------------------------------------------------------
 *
 * Admin_WriteVolConfig --
 *
 *	Write the volume configuration file
 *
 * Results:
 *	return code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Admin_WriteVolConfig(volFile, volList, cnt)
    char *volFile;            /* config file */
    VolConfig *volList;       /* array */
    int cnt;                  /* # slots */
{
    FILE *configFile;
    int retCode = T_SUCCESS;
    VolConfig *itemPtr = volList;

    if ((configFile=fopen(volFile, "w")) == NULL) {
	syserr = errno;
	return T_FAILURE;
    }

    while (cnt-- > 0) {
	fprintf(configFile, "%s %d", itemPtr->volLabel, itemPtr->location);
	itemPtr++;
    }

    fclose(configFile);
    return retCode;

}



/*
 *----------------------------------------------------------------------
 *
 * Admin_GetFreeVol --
 *
 *	Get a new volume from the free pool.
 *
 * Results:
 *	Integer volume id and return code.
 *
 * Side effects:
 *	Updates the volume free pool file.
 *
 * Note:
 *      On entry, *volIdPtr is id of desired volume or -1 if any will do
 *
 * Quote:
 *      "I love deadlines. I like the whooshing sound 
 *      they make as they fly by."
 *      -- Douglas Adams
 *
 *----------------------------------------------------------------------
 */

int
Admin_GetFreeVol(root, volIdPtr)
    char *root;               /* root of Jaquith tree */
    int *volIdPtr;            /* receiving variable */
{
    FILE *volStream;
    FILE *newStream;
    char pathName[T_MAXPATHLEN];
    char newPathName[T_MAXPATHLEN];
    char *buf;
    int cnt = 0;
    int retCode = T_FAILURE;
    Lock_Handle lockHandle;
    int tmpVolId;
    int desiredVol = *volIdPtr;

    strcpy(pathName, root);
    strcat(pathName, FREESUFFIX);
    strcpy(newPathName, pathName);
    strcat(newPathName, FREENEWSUFFIX);

    if (Lock_Acquire(pathName, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    } else if ((volStream=fopen(pathName, "r")) == (FILE *)NULL) {
	Lock_Release(&lockHandle);
	return T_NOVOLUME;
    } else if ((newStream=fopen(newPathName, "w")) == (FILE *)NULL) {
	fclose(volStream);
	Lock_Release(&lockHandle);
	return T_NOVOLUME;
    } 

    while ((buf=Utils_ReadLine(volStream, 1)) != (char *)NULL) {
	if (Utils_CvtInteger(buf, 0, INT_MAX, &tmpVolId) != T_SUCCESS) {
	    fclose(newStream);
	    fclose(volStream);
	    unlink(newPathName);
	    Lock_Release(&lockHandle);
	    return T_NOVOLUME;
	}
	if ((desiredVol == -1) || (desiredVol == tmpVolId)) {
	    *volIdPtr = tmpVolId;
	    retCode = T_SUCCESS;
	    while ((buf=Utils_ReadLine(volStream, 0)) != (char *)NULL) {
		fputs(buf, newStream);
		cnt++;
	    }
	    break;
	} else {
	    fputs(buf, newStream);
	    fputc('\n', newStream);
	    cnt++;
	}
    }

    /* Can't lose update or might assign the volume twice */
    fsync(fileno(newStream));

    fclose(newStream);
    fclose(volStream);
    unlink(pathName);
    rename(newPathName, pathName);

    Lock_Release(&lockHandle);

    if (cnt <= MINFREEVOLS) {
	sprintf(printBuf, "Warning: Only %d free volumes left in %s.\n",
		cnt, pathName);
	Utils_SendMail(ARCHMASTER, printBuf, "warning");
    }

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * Admin_PutFreeVol --
 *
 *	Add a new volume to the free pool.
 *
 * Results:
 *	Return code.  It's an error if volume already exists.
 *
 * Side effects:
 *	New volume will go at bottom of list.
 *
 *----------------------------------------------------------------------
 */

int
Admin_PutFreeVol(root, volId)
    char *root;               /* root of Jaquith tree */
    int volId;                /* receiving variable */
{
    FILE *volStream;
    FILE *newStream;
    char pathName[T_MAXPATHLEN];
    char newPathName[T_MAXPATHLEN];
    char *buf;
    int retCode = T_SUCCESS;
    Lock_Handle lockHandle;
    int tmpVolId;

    strcpy(pathName, root);
    strcat(pathName, FREESUFFIX);
    strcpy(newPathName, pathName);
    strcat(newPathName, FREENEWSUFFIX);

    if (Lock_Acquire(pathName, LOCK_BLOCK, &lockHandle) != T_SUCCESS) {
	return T_FAILURE;
    } else if ((volStream=fopen(pathName, "r")) == (FILE *)NULL) {
	Lock_Release(&lockHandle);
	return T_FAILURE;
    } else if ((newStream=fopen(newPathName, "w")) == (FILE *)NULL) {
	fclose(volStream);
	Lock_Release(&lockHandle);
	return T_FAILURE;
    } 

    while ((buf=Utils_ReadLine(volStream, 1)) != (char *)NULL) {
	if (Utils_CvtInteger(buf, 0, INT_MAX, &tmpVolId) != T_SUCCESS) {
	    fclose(newStream);
	    fclose(volStream);
	    unlink(newPathName);
	    Lock_Release(&lockHandle);
	    syserr = EINVAL;
	    return T_FAILURE;
	}
	if (volId == tmpVolId) {
	    fclose(newStream);
	    fclose(volStream);
	    unlink(newPathName);
	    Lock_Release(&lockHandle);
	    syserr = EEXIST;
	    return T_FAILURE;
	}
	fputs(buf, newStream);
	fputc('\n', newStream);
    }

    /* Ok, not found. Add it to list */
    fprintf(newStream, "%d\n", volId);

    fclose(newStream);
    fclose(volStream);
    unlink(pathName);
    rename(newPathName, pathName);

    Lock_Release(&lockHandle);

    return retCode;
}
