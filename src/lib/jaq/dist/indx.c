/* 
 * indx.c --
 *
 *	Perform indexing on Jaquith archive system.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/indx.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"

#define DEF_DIR_PERM 0755
#define DEF_FILE_PERM 0644
#define DEF_TAB_SIZE 277      /* arbitrary. Table will grow as needed */

#define SLASHSUFFIX "slash"
#define FILESUFFIX "_jaquith.files"
#define INDX_VERSION 1

extern int jDebug;
extern int syserr;

static int  MatchPath      _ARGS_ ((char **partList,
				    char *dirPath,
				    Q_Handle *indxQueue));
static int  IsReadable     _ARGS_ ((T_FileStat *statInfoPtr,
				    Caller *callerPtr));
static int  ReadIndxFile   _ARGS_ ((char *path, T_FileStat **listPtr,
				    int *cntPtr));
static char *ReadStringItem _ARGS_ ((FILE *indxStream));



/*
 *----------------------------------------------------------------------
 *
 * Indx_Open --
 *
 *	Locate and open an index buffer file
 *
 * Results:
 *	stream ptr or NULL
 *
 * Side effects:
 *	Consumes a file descriptor.
 *      Preserve name of current open file.
 *
 * Note:
 *      All the directories leading to the file are created
 *      as necessary.
 *
 *----------------------------------------------------------------------
 */

int
Indx_Open(pathName, openOptions, indxStrPtr)
    char *pathName;           /* name of index file */
    char *openOptions;        /* read, write, etc. */
    FILE **indxStrPtr;        /* receiving object */
{
    char *curPtr;

    /*
     * First, be optimistic...
     */
    if ((*indxStrPtr=fopen(pathName, openOptions)) != (FILE *)NULL) {
	return T_SUCCESS;
    } else if (errno != ENOENT) {
	syserr = errno;
	fprintf(stderr,"Indx_Open: couldn't open %s; eno %d\n",
		pathName,syserr);
	return T_INDXFAILED;
    } else if (*openOptions == 'r') {
	return T_SUCCESS;
    }
    
    /*
     * some part of the path is not there and we're writing...
     */
    curPtr = pathName;
    
    while (curPtr != NULL) {
	curPtr = STRCHR(curPtr+1, '/');
	*curPtr = '\0';
	if (access(pathName, X_OK) == -1) {
	    break;
	}
	*curPtr = '/';
    } 

    while (curPtr != NULL) {
	*curPtr = '\0';
	if (mkdir(pathName, DEF_DIR_PERM) == -1) {
	    syserr = errno;
	    return T_INDXFAILED;
	}
	*curPtr = '/';
	curPtr = STRCHR(curPtr+1, '/');
    } 

    /*
     * Now retry the open
     */
    if ((*indxStrPtr=fopen(pathName, openOptions)) == (FILE *)NULL) {
	syserr = errno;
	fprintf(stderr,"Indx_Open: open failed %d\n", errno);
	return T_INDXFAILED;
    }

    return T_SUCCESS;

}



/*
 *----------------------------------------------------------------------
 *
 * Indx_Close --
 *
 *	Close an index file
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
Indx_Close(indxStream)
    FILE *indxStream;         /* open index stream */
{
    if (indxStream != (FILE *)NULL) {
	fclose(indxStream);
    }
    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * Indx_ReadIndxEntry --
 *
 *	Parse an entry from an indx file.
 *
 * Results:
 *	returns T_FileStat.
 *
 * Side effects:
 *      May allocate some space. Will read indx file.
 *
 * Note:
 *      Each string item in the index record is preceded by a length.
 *      This is necessary since Unix filenames can have arbitrary characters
 *      in them. (Everything except '\0', ' ', and '/', I think).
 *
 *----------------------------------------------------------------------
 */

int
Indx_ReadIndxEntry(indxStream, statInfoPtr)
    FILE *indxStream;         /* source stream */
    T_FileStat *statInfoPtr;  /* receiving structure */
{
    int version;

    /*
     * if this read fails, we'll assume we're reading the last
     * (incomplete) entry and say we're done.
     */
    if (fscanf(indxStream, "%x", &version) != 1) {
	return T_FAILURE;
    }

    /*
     * opportunity here to do special processing depending 
     * on version number, if it ever becomes necessary.
     */
    if (version != INDX_VERSION) {
	if (jDebug) {
	    fprintf(stderr,"Indx_ReadIndxEntry: Bad index version: 0x%0x\n",
		    version);
	}
	return T_BUFFAILED;
    }

    if (fscanf(indxStream, "%x %x %x %x %x %x %x\n",
		    &statInfoPtr->size,
		    &statInfoPtr->mode,
		    &statInfoPtr->atime,
		    &statInfoPtr->mtime,
		    &statInfoPtr->vtime,
		    &statInfoPtr->tBufId,
		    &statInfoPtr->offset) != 7) {
	if (jDebug) {
	    fprintf(stderr,"Didn't get numbers\n");
	}
	return T_BUFFAILED;
    }

    if ((statInfoPtr->fileName=ReadStringItem(indxStream)) == NULL) {
	if (jDebug) {
	    fprintf(stderr,"Didn't get filename\n");
	}
	return T_BUFFAILED;
    }
    if ((statInfoPtr->uname=ReadStringItem(indxStream)) == NULL) {
	if (jDebug) {
	    fprintf(stderr,"Didn't get uname\n");
	}
	return T_BUFFAILED;
    }
    if ((statInfoPtr->gname=ReadStringItem(indxStream)) == NULL) {
	if (jDebug) {
	    fprintf(stderr,"Didn't get gname\n");
	}
	return T_BUFFAILED;
    }
    if ((statInfoPtr->linkName=ReadStringItem(indxStream)) == NULL) {
	if (jDebug) {
	    fprintf(stderr,"Didn't get linkname\n");
	}
	return T_BUFFAILED;
    }
    if ((statInfoPtr->abstract=ReadStringItem(indxStream)) == NULL) {
	if (jDebug) {
	    fprintf(stderr,"Didn't get file abstract\n");
	}
	return T_BUFFAILED;
    }
    if ((statInfoPtr->fileList=ReadStringItem(indxStream)) == NULL) {
	if (jDebug) {
	    fprintf(stderr,"Didn't get file list\n");
	}
	return T_BUFFAILED;
    }

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * Indx_WriteEntry --
 *
 *	Format an entry for the index buffer and write it out, maybe twice.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	May write to either thdr file or indx file or both.
 *
 * Note:
 *      There's no locking because the switchboard process is
 *      only creating 1 writer at a time to an archive. Readers
 *      may see an incomplete write at the end of the file and
 *      ignore it.
 *
 *
 *----------------------------------------------------------------------
 */

int
Indx_WriteIndxEntry(statInfoPtr, tHdrStream, indxStream)
    T_FileStat *statInfoPtr;  /* meta-info for new entry */
    int tHdrStream;           /* thdr file stream */
    FILE *indxStream;         /* disk indx file stream */
{
    char *tmpPtr;
    static char *bufPtr;
    static int bufLen = 0;
    char numBuf[8*10];
    int len;
    int numLen;
    int fnameLen;
    int linkLen;
    int absLen;
    int flistLen;
    int unameLen;
    int gnameLen;

    fnameLen = strlen(statInfoPtr->fileName);
    unameLen = strlen(statInfoPtr->uname);
    gnameLen = strlen(statInfoPtr->gname);
    linkLen = strlen(statInfoPtr->linkName);
    flistLen = strlen(statInfoPtr->fileList);
    absLen = strlen(statInfoPtr->abstract);

    len = fnameLen + linkLen + absLen + flistLen + unameLen + gnameLen +
	(6*10); /* space for the lengths of the six strings */

    if (len > bufLen) {
	if (bufLen > 0) {
	    MEM_FREE("Indx_MakeEntry", bufPtr);
	}
	bufLen = len;
	bufPtr = (char *)MEM_ALLOC("Indx_MakeEntry", len*sizeof(char));
    }

   sprintf(numBuf, "%x %x %x %x %x %x %x %x\n",
	   INDX_VERSION,
	   statInfoPtr->size,
	   statInfoPtr->mode,
	   statInfoPtr->atime,
	   statInfoPtr->mtime,
	   statInfoPtr->vtime,
	   statInfoPtr->tBufId,
	   statInfoPtr->offset);
    numLen = strlen(numBuf);

   /*
    *  Must be careful about printf on Ultrix 4.2 because it truncates
    *  strings to 32K. Large directories may have a fileList > 32K
    */
   sprintf(bufPtr, "%x %s\n%x %s\n%x %s\n%x %s\n%x %s\n%x ",
	   fnameLen, statInfoPtr->fileName,
	   unameLen, statInfoPtr->uname,
	   gnameLen, statInfoPtr->gname,
	   linkLen, statInfoPtr->linkName,
	   absLen, statInfoPtr->abstract,
	   flistLen);

    tmpPtr = bufPtr+strlen(bufPtr);
    strcpy(tmpPtr, statInfoPtr->fileList);
    tmpPtr += flistLen;
    *tmpPtr++ = '\n';
    *tmpPtr = '\0';
    len = tmpPtr-bufPtr;

    /* write out thdr stream, if requested */
    if (tHdrStream >= 0) {
	write(tHdrStream, numBuf, numLen);
	write(tHdrStream, bufPtr, len);
    }

    /* write out indx file stream, if requested (with simple filename) */ 
    if (indxStream != (FILE *)NULL) {
	fwrite(numBuf, sizeof(char), numLen, indxStream);
	if (fnameLen == 1) {
	    fprintf(indxStream, "%x %s\n", strlen(SLASHSUFFIX), SLASHSUFFIX);
	} else {
	    tmpPtr = STRRCHR(statInfoPtr->fileName, '/') + 1;
	    fprintf(indxStream, "%x %s\n", strlen(tmpPtr), tmpPtr);
	}
	tmpPtr = STRCHR(bufPtr, '\n') + 1;
	fputs(tmpPtr, indxStream);
    }

    /*
     * If the buffer is excessively large then assume it's a fluke
     * and free it. We'll get a more reasonable one next time.
     */
    if (bufLen > 32*1024) {
	bufLen = 0;
	MEM_FREE("Indx_MakeEntry", bufPtr);
    }

    return numLen+len;

}


/*
 *----------------------------------------------------------------------
 *
 * Indx_Match
 *
 *	Find all entries in a index file that match the pattern
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 * Note:
 *      The algorithm is convoluted. We don't know ahead of time
 *      how many versions of a file there are but we need the
 *      second-to-last, say. So for each file that matches all
 *      the other criteria, we'll keep a list of how many times
 *      we seen this thing. Then after we've been through the
 *      entire indx file, we'll grab the correct version(s) off
 *      the file queue.
 *
 *----------------------------------------------------------------------
 */

int
Indx_Match(fileSpecPtr, indxPath, archPath, callerPtr,
	   receiveProc, receiveBlkPtr, ignoreDir)
    QuerySpec *fileSpecPtr;   /* user specifications to be matched */
    char *indxPath;           /* indx path name */
    char *archPath;           /* current archive path name */
    Caller *callerPtr;        /* caller's identity */
    int (*receiveProc)();     /* Callback routine */
    int *receiveBlkPtr;       /* Callback datum */
    int ignoreDir;            /* Ignore directory. We want its contents */
{
    int retCode = T_SUCCESS;
    char curPath[2*T_MAXPATHLEN];
    int i;
    int fileCnt;
    T_FileStat *fileList;
    T_FileStat *statInfoPtr;
    T_FileStat *newInfoPtr;
    time_t tmpDate;
    QuerySpec newSpec;
    char newIndx[2*T_MAXPATHLEN];
    int first;
    int last;
    char *tmpPtr;
    int isReadable;
    Q_Handle *fileQueue;
    Hash_Handle *nameTab = NULL;
    int versionCnt;

/*
    if (jDebug) {
	fprintf(stderr,"indx_match: pattern %s, indx %s, recurse %d\n",
		fileSpecPtr->fileName, indxPath, fileSpecPtr->recurse);
    }
*/
    /* Read file and convert to an array of entries */
    if (ReadIndxFile(indxPath, &fileList, &fileCnt) != T_SUCCESS) {
	return T_FAILURE;
    } else if (fileCnt == 0) {
	return T_SUCCESS;
    }

    fileQueue = Q_Create("file", 1);

    if ((nameTab=Hash_Create("name", DEF_TAB_SIZE, Utils_StringHashProc, 0)) ==
	(Hash_Handle *)NULL) {
	return T_INDXFAILED;
    }

    if (*fileSpecPtr->fileName) {
	strcpy(curPath,indxPath+strlen(archPath)+sizeof(SLASHSUFFIX));
	*(STRRCHR(curPath, '/')+1) = '\0';
    } else {
	fileSpecPtr->fileName = Str_Dup(SLASHSUFFIX);
	*curPath = '\0';
    }

    for (i=0, statInfoPtr=fileList;i<fileCnt; i++,statInfoPtr++) {
	if (fileSpecPtr->flags & QUERY_MODDATE) {
	    tmpDate = statInfoPtr->mtime;
	} else {
	    tmpDate = statInfoPtr->vtime;
	}

	/* ok, put it through the tests...*/
	if (((*fileSpecPtr->owner) &&
	     (strcmp(statInfoPtr->uname, fileSpecPtr->owner) != 0)) ||
	    ((*fileSpecPtr->group) &&
	     (strcmp(statInfoPtr->gname, fileSpecPtr->group) != 0)) ||
	    ((fileSpecPtr->compAbstract != NULL) &&
	     (!regexec(fileSpecPtr->compAbstract,statInfoPtr->abstract))) ||
	    ((fileSpecPtr->firstDate != -1) &&
	     (Time_Compare(tmpDate,fileSpecPtr->firstDate,0) < 0)) ||
	    ((fileSpecPtr->lastDate != -1) &&
	     (Time_Compare(tmpDate,fileSpecPtr->lastDate,0) > 0)) ||
	    ((*fileSpecPtr->fileName) &&
	     (!Str_Match(statInfoPtr->fileName,fileSpecPtr->fileName))) ||
	    ((!(isReadable=IsReadable(statInfoPtr, callerPtr))) &&
	     (!(S_ISADIR(statInfoPtr->mode))))) {
	    continue;
	}

	/* Update hash table count. We'll use filemark as counter. Sorry. */
	if (Hash_Lookup(nameTab, statInfoPtr->fileName,
		strlen(statInfoPtr->fileName),
		(Hash_ClientData *)&statInfoPtr->filemark) == T_SUCCESS) {
	    statInfoPtr->filemark++;
	    if (Hash_Update(nameTab, statInfoPtr->fileName,
		strlen(statInfoPtr->fileName),
		(Hash_ClientData)statInfoPtr->filemark) != T_SUCCESS) {
		return T_INDXFAILED;
	    }
	} else {
	    statInfoPtr->filemark = 1;
	    if (Hash_Insert(nameTab, statInfoPtr->fileName,
		strlen(statInfoPtr->fileName),
		(Hash_ClientData)statInfoPtr->filemark) != T_SUCCESS) {
		return T_INDXFAILED;
	    }
	}

	newInfoPtr = Utils_CopyFileStat(statInfoPtr);
        Q_Add(fileQueue, (Q_ClientData)newInfoPtr, newInfoPtr->tBufId);
/*
	if (jDebug) {
	    fprintf(stderr,"Indx_Match: Enqueued %x %s\n",
		    newInfoPtr, newInfoPtr->fileName);
	}
*/
    }

    /*
     * Ok, we've acquired all the entries at this level.
     * Now prune them by version, and recurse where necessary.
     */
    fileCnt = Q_Count(fileQueue);
/*
    if (jDebug) {
        fprintf(stderr,"Checking %d files\n", fileCnt);
    }
*/
    for (i=0; (i < fileCnt) && (retCode == T_SUCCESS); i++) {
        statInfoPtr = (T_FileStat *) Q_Remove(fileQueue);
	tmpPtr = statInfoPtr->fileName;
	if (Hash_Lookup(nameTab, statInfoPtr->fileName,
			strlen(statInfoPtr->fileName),
			(Hash_ClientData *)&versionCnt)  != T_SUCCESS) {
	    fprintf(stderr,"Indx_Match: Couldn't look up %s\n", statInfoPtr->gname);
	    return T_INDXFAILED; /* shouldn't happen */
	}

	if (fileSpecPtr->firstVer < 0) {
	    first = versionCnt + fileSpecPtr->firstVer + 1;
	} else {
	    first = fileSpecPtr->firstVer;
	}
	if (fileSpecPtr->lastVer < 0) {
	    last = versionCnt + fileSpecPtr->lastVer + 1;
	} else {
	    last = fileSpecPtr->lastVer;
	}
/*
	fprintf(stderr,"first %d, last %d, cnt %d, this %d\n",
		first, last, versionCnt, statInfoPtr->filemark);
*/
	if ((statInfoPtr->filemark < first) ||
	    (statInfoPtr->filemark > last)) {
/*
	    if (jDebug) {
		fprintf(stderr,"Indx_Match: pruning %s\n",
		statInfoPtr->fileName);
	    }
*/
	    Utils_FreeFileStat(statInfoPtr, 1);
	    continue;
	}

        /* Hack: If it's a directory and we're at the top level */
        /* then don't return it; we're just interested in its children */
        if ((!S_ISADIR(statInfoPtr->mode)) || (!ignoreDir)) {
            if (*curPath) {
                statInfoPtr->fileName = Str_Cat(2, curPath, tmpPtr);
            } else {
                statInfoPtr->fileName = Str_Dup("/");
            }
            retCode = (*receiveProc)(statInfoPtr, receiveBlkPtr);
        }

	/* Need to recurse ? */
	if ((fileSpecPtr->recurse > 0) && (S_ISADIR(statInfoPtr->mode)) &&
	    (isReadable) && (*statInfoPtr->fileList)) {
	    strcpy(newIndx, indxPath);
	    strcpy(STRRCHR(newIndx, '/')+1, tmpPtr);
	    strcat(newIndx, "/");
	    strcat(newIndx, FILESUFFIX);
	    newSpec = *fileSpecPtr;
	    newSpec.recurse = fileSpecPtr->recurse - 1;
	    newSpec.fileName = Str_Dup(statInfoPtr->fileList);
	    newSpec.firstVer = -1;
	    newSpec.lastVer = -1;
/*
	    if (jDebug) {
		fprintf(stderr,"Indx_match: recursing %s\n", 
		newSpec.fileName);
	    }
*/
	    Indx_Match(&newSpec, newIndx, archPath, callerPtr,
		       receiveProc, receiveBlkPtr, 0);
	    MEM_FREE("Indx_Match", newSpec.fileName);
	}
	Utils_FreeFileStat(statInfoPtr, 1);
    }

    Q_Destroy(fileQueue);
    Hash_Destroy(nameTab);

    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * Indx_MakeIndx --
 *
 *	Create index file path for specified archive and file
 *
 * Results:
 *	returns name of index file.
 *
 * Side effects:
 *	Allocates string space.
 *
 *----------------------------------------------------------------------
 */

int
Indx_MakeIndx(archPath, filePath, indxPath)
    char *archPath;           /* name of archive. */
    char *filePath;           /* full path name of file */
    char *indxPath;           /* ptr to receiving space */
{
    char *slashPtr;

    if ((slashPtr=STRRCHR(filePath, '/')) == NULL) {
	return T_BADMSGFMT;
    } else {
	strcpy(indxPath, archPath);
	if (*(slashPtr+1)) {
	    strcat(indxPath, "/");
	    strcat(indxPath, SLASHSUFFIX);
	}
	*slashPtr = '\0';
	strcat(indxPath, filePath);
	strcat(indxPath, "/");
	strcat(indxPath, FILESUFFIX);
	*slashPtr = '/';
    }
    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * Indx_MakeIndxList --
 *
 *	Read through directory hierarchy finding index files
 *
 * Results:
 *	returns list of index files.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

int
Indx_MakeIndxList(archPath, filePath, indxQueue)
    char *archPath;           /* name of archive. */
    char *filePath;           /* reg exp describing file */
    Q_Handle *indxQueue;      /* work queue to collect index filenames */
{
    char dirPath[2*T_MAXPATHLEN];
    char **partList;
    int partCnt = 0;
    int retCode;
    char *insidePtr;

    strcpy(dirPath, archPath);
    if (*(filePath+1)) {
	strcat(dirPath, "/");
	strcat(dirPath, SLASHSUFFIX);
    }
    partList = Str_Split(filePath, '/', &partCnt, 0, &insidePtr);
    partList[partCnt-1] = NULL; /* Zap the simple filename */
    retCode = MatchPath(&partList[1], dirPath, indxQueue);

    MEM_FREE("Indx_MakeIndxList", (char *)partList);
    MEM_FREE("Indx_MakeIndxList", (char *)insidePtr);

    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * MatchPath
 *
 *	Recursively descend through tree matching pathname parts
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Will put matching names on queue.
 *
 *----------------------------------------------------------------------
 */

static int
MatchPath(partList, dirPath, indxQueue)
    char **partList;          /* pathname parts not yet matched */
    char *dirPath;            /* built-up pathname */
    Q_Handle *indxQueue;      /* list of matching filenames */
{
    DIR *dirStream;
    DirObject *entryPtr;
    char *dirPtr;
    struct stat statBuf;
    char *tmpPtr;
    char **newPartList;
/*
    if (jDebug) {
      fprintf(stderr,"partList '%s', dirPath '%s'\n",
	      *partList, dirPath);
    }
*/
    if (*partList == NULL) {
	newPartList = partList;
	tmpPtr = Str_Cat(3, dirPath, "/", FILESUFFIX);
	Q_Add(indxQueue, (Q_ClientData) tmpPtr, Q_TAILQ);
	return T_SUCCESS;
    } else {
	newPartList = partList+1;
    }
    
    if ((dirStream=opendir(dirPath)) == (DIR *)NULL) {
      if (errno == ENOENT) {
	return T_SUCCESS; /* no index file is ok */
      } else {
	syserr = errno;
	if (jDebug) {
	  fprintf(stderr,"MatchPath: couldn't open %s. errno %d\n",
		  dirPath, syserr);
	}
	return T_INDXFAILED;
      }
    }

    strcat(dirPath, "/");
    dirPtr = dirPath+strlen(dirPath);

    while ((entryPtr=readdir(dirStream)) != (DirObject *)NULL) {
	if ((strcmp(entryPtr->d_name, ".") != 0) &&
	    (strcmp(entryPtr->d_name, "..") != 0) &&
	    ((*partList == NULL) ||
	     (Str_Match(entryPtr->d_name, *partList)))) {
	    strcpy(dirPtr, entryPtr->d_name);
	    if ((stat(dirPath, &statBuf) >= 0) &&
		(S_ISADIR(statBuf.st_mode))) {
		if (MatchPath(newPartList, dirPath, indxQueue) 
		    != T_SUCCESS) {
		    return T_INDXFAILED;
		}
	    }
	}
    }

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * IsReadable --
 *
 *	Check for proper read permission.
 *
 * Results:
 *	1 == readable; 0 = protected.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
IsReadable(statInfoPtr, caller)
    T_FileStat *statInfoPtr;  /* file in question */
    Caller *caller;           /* caller's id */
{
    if ((statInfoPtr->mode & 04) ||
	(Str_Match(statInfoPtr->uname, caller->userName)) ||
	(Str_Match(statInfoPtr->gname, caller->groupName) &&
	 (statInfoPtr->mode & 040))) {
	return 1;
    }
    if (jDebug) {
	fprintf(stderr,"No Access: %s ('%s %s 0%o' != '%s %s')\n",
		statInfoPtr->fileName, statInfoPtr->uname,
		statInfoPtr->gname, statInfoPtr->mode,
		caller->userName, caller->groupName);
    }

    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * ReadIndxFile --
 *
 *	Read and store contents of an indx file.
 *
 * Results:
 *	returns list T_FileStat ptrs.
 *
 * Side effects:
 *      May allocate some space. May read indx file.
 *
 *----------------------------------------------------------------------
 */

static int
ReadIndxFile(indxPath, fileListPtr, cntPtr)
    char *indxPath;           /* Index file to be read */
    T_FileStat **fileListPtr; /* receiving list ptr */
    int *cntPtr;              /* receiving count ptr */
{
    int i;
    FILE *indxStream;
    int retCode;
    T_FileStat *statInfoPtr;
    static char savedIndxPath[2*T_MAXPATHLEN];
    static T_FileStat *savedFileList = NULL;
    static int savedListSize = 0;
    static int savedFileCnt = 0;
    int newListSize;
    T_FileStat *newFileList;

    /*
     * If requested file is the one we have stored, then there's nothing to do.
     * The file may have been changed during our update operation, but
     * it's too costly to do repeated stat operations just to catch this case.
     * Nobody else can be updating this file so we'd only be checking our
     * own updates and few users will do: 'jput file file'
     */
    if (strcmp(indxPath, savedIndxPath) == 0) {
/*
	    fprintf(stderr,"Same indx file: %s\n", indxPath);
*/
	    *fileListPtr = savedFileList;
	    *cntPtr = savedFileCnt;
	    return T_SUCCESS;
    }

    /* release old entries */
/*
    fprintf(stderr,"Freeing indx file: %s, %d entries\n", indxPath, savedFileCnt);
*/
    for (i=0,statInfoPtr=savedFileList; i<savedFileCnt; i++,statInfoPtr++) {
        Utils_FreeFileStat(statInfoPtr, 0);
    }

    strcpy(savedIndxPath, indxPath);
    *cntPtr = savedFileCnt = 0;
    *fileListPtr = NULL;

    /* Attempt the open */
    if ((retCode=Indx_Open(indxPath, "r", &indxStream)) != T_SUCCESS) {
	fprintf(stderr,"Indx_Match: couldn't open %s\n", indxPath);
	return retCode;
    }

    /* file does not exist */
    if (indxStream == NULL) {
	return T_SUCCESS;
    }

    while (1) {
	/* Make room in the list */
	if (savedFileCnt >= savedListSize) {
	    newListSize = savedListSize + 500;
	    newFileList = (T_FileStat *)MEM_ALLOC("ReadIndxFile",
					  newListSize*sizeof(T_FileStat));
/*
            fprintf(stderr,"Enlarging indx table space from %d to %d\n",
		    savedListSize, newListSize);
*/
	    for (i=0; i<savedListSize; i++) {
		newFileList[i] = savedFileList[i];
	    }
	    if (savedFileList != NULL) {
		MEM_FREE("ReadIndxFile", savedFileList);
	    }
	    savedListSize = newListSize;
	    savedFileList = newFileList;
	}
	if ((retCode=Indx_ReadIndxEntry(indxStream,
			   &savedFileList[savedFileCnt])) != T_SUCCESS) {
	    if ((jDebug) && (retCode != T_FAILURE)) {
		fprintf(stderr,"ReadIndxEntry failed for %s\n", indxPath);
	    }
	    break;
	}
	savedFileCnt++;
    }

    Indx_Close(indxStream);
/*
    fprintf(stderr,"Refilled indx table with %d entries.\n", savedFileCnt);
*/
    *fileListPtr = savedFileList;
    *cntPtr = savedFileCnt;

    return T_SUCCESS;
}
	     

/*
 *----------------------------------------------------------------------
 *
 * ReadStringItem --
 *
 *	Read a '\n' terminated string item from index file.
 *
 * Results:
 *	returns duplicated copy of string.
 *
 * Side effects:
 *      May allocate some space.
 *
 *----------------------------------------------------------------------
 */

static char *
ReadStringItem(indxStream)
    FILE *indxStream;         /* input stream */
{
    char *string;
    int stringLen;
    char *ptr;

    if (fscanf(indxStream, "%x%*c", &stringLen) != 1) {
	fprintf(stderr,"Didn't get string len\n");
	return NULL;
    }
    if ((stringLen < 0) || (stringLen > 1000000)) {
	fprintf(stderr,"got bad string len 0x%x\n", stringLen);
	return NULL;
    }

    string = ptr =(char *)MEM_ALLOC("ReadStringItem", (stringLen+1)*sizeof(char));

    while (stringLen-- > 0) {
	*ptr++ = (char) getc(indxStream);
    }

    *ptr = '\0';

    if (getc(indxStream) != (int)'\n') {
	MEM_FREE("ReadStringItem", string);
	fprintf(stderr,"Didn't get final 'cr'\n");
	return NULL;
    }

    return string;
}

