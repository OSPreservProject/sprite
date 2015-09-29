/* 
 * jbuild.c --
 *
 *	Rebuild a Jaquith index from tape.
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
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jbuild.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"

static char printBuf[T_MAXSTRINGLEN];

static FILE *memDbg = NULL;   /* stream for memory tracing */

int jDebug;                   /* Internal debugging only */
int syserr = 0;               /* Our personal record of errno */

#define DEF_DEVNAME ""
#define DEF_VERBOSE 0
#define DEF_CLOBBER 0
#define DEF_NOINDEX 0
#define DEF_SAVE 0
#define DEF_FIRST 0
#define DEF_LAST -1
#define DEF_MATCHPATH ""
#define DEF_MATCHTBUF ""
#define DEF_BEFORE ""
#define DEF_AFTER ""
#define MAXFILE 100000        /* Completely arbitrary. */

static void  CheckOptions   _ARGS_ ((Parms *parmsPtr));
static void  RecoverVolume  _ARGS_ ((char *devName));
static int   RecoverFile    _ARGS_ ((int volStream));
static int   ProcessFile    _ARGS_ ((char *oldName,
				     int *fileCntPtr, int *skipCntPtr));
static int   RebuildIndx    _ARGS_ ((int fileStream, int *fileCntPtr,
				     int *skipCntPtr, char *archPath));
static int   NeedToRestore  _ARGS_ ((T_FileStat *statInfoPtr));

typedef struct parmTag {
    char *root;
    char *devName;
    int verbose;
    int clobber;
    int noIndex;
    int save;
    int first;
    int last;
    char *matchPath;
    char *matchTBuf;
    char *before;
    char *after;
    int bufSize;
    time_t beforeDate;        /* converted forms of parms.{before,after} */
    time_t afterDate;
} Parms;

Parms parms = {
    DEF_ROOT,
    DEF_DEVNAME,
    DEF_VERBOSE,
    DEF_CLOBBER,
    DEF_NOINDEX,
    DEF_SAVE,
    DEF_FIRST,
    DEF_LAST,
    DEF_MATCHPATH,
    DEF_MATCHTBUF,
    DEF_BEFORE,
    DEF_AFTER,
    T_TARSIZE
};

Option optionArray[] = {
    {OPT_STRING, "root", (char *)&parms.root, "Root of index tree"},
    {OPT_STRING, "dev", (char *)&parms.devName, "Device name"},
    {OPT_TRUE, "v", (char *)&parms.verbose, "Verbose mode"},
    {OPT_TRUE, "clobber", (char *)&parms.clobber, "Overwrite old index."},
    {OPT_TRUE, "noindex", (char *)&parms.noIndex, "Restore thdr files but don't rebuild index"},
    {OPT_TRUE, "save", (char *)&parms.save, "Save thdr file after restoration"},
    {OPT_INT, "first", (char *)&parms.first, "First thdr to restore."},
    {OPT_INT, "last", (char *)&parms.last, "Last thdr to restore."},
    {OPT_STRING, "matchpath", (char *)&parms.matchPath, "Restore subtrees whose path name matches globbing expression"},
    {OPT_STRING, "matchtbuf", (char *)&parms.matchTBuf, "Restore files whose tbuf number matches globbing expression"},
    {OPT_STRING, "before", (char *)&parms.before, "Restore files archived prior to date"},
    {OPT_STRING, "after", (char *)&parms.after, "Restore files archived since date"},
    {OPT_INT, "bufsize", (char *)&parms.bufSize, "Unit of tape I/O."}
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * jbuild --
 *
 *	Rebuild a Jaquith index from tape.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Modifies/creates jaquith disk index structure.
 *
 * Note:
 *      Only handles 1 tape at a time. Should be able to
 *      read filemap list and restore entire index automatically.
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
    uid_t myId;

/*    memDbg = fopen("jbuild.mem","w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckOptions(&parms);

    if (access(parms.root, W_OK) == -1) {
	Utils_Bailout("Can't find (or write permission denied): %s", parms.root);
    }

    RecoverVolume(parms.devName);

    MEM_REPORT("jbuild", ALLROUTINES, SORTBYOWNER);

    return T_SUCCESS;
}



/*
 *----------------------------------------------------------------------
 *
 * RecoverVolume -- 
 *
 *      Read and recover indexing info from Jaquith tape.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
RecoverVolume(devName)
    char *devName;            /* device name */
{
    int volStream;
    int retCode;
    int totFiles = 0;
    int totSkip = 0;
    int indxCnt = 0;
    int fileCnt;
    int skipCnt;
    char tmpName[T_MAXPATHLEN];

    if ((volStream=Dev_OpenVolume(devName, O_RDONLY)) == -1) {
	sprintf(printBuf, "Open of %s failed", devName);
	Utils_Bailout(printBuf, BAIL_PERROR);
    }

    sprintf(tmpName, "%s/rebuild.%x", parms.root, getpid());

    if (parms.verbose) {
	fprintf(stdout, "Seeking to file %d\n", parms.first);
    }
    if (Dev_SeekVolume(volStream, parms.first*2, DEV_ABSOLUTE) != T_SUCCESS) {
	sprintf(printBuf, "Couldn't seek to first file. Errno %d", syserr);
	Utils_Bailout(printBuf, BAIL_PERROR);
    }

    while ((parms.last == -1) || (fileCnt <= parms.last)) {
	if (RecoverFile(volStream, tmpName) != T_SUCCESS) {
	    break;
	}
	if (ProcessFile(tmpName, &fileCnt, &skipCnt) != T_SUCCESS) {
	    break;
	}
	if (fileCnt) {
	    totFiles += fileCnt;
	    totSkip += skipCnt;
	    indxCnt++;
	}
	if (Dev_SeekVolume(volStream, 1, DEV_RELATIVE) != T_SUCCESS) {
	    break;
	}
    }

    unlink(tmpName);
    close(volStream);

    fprintf(stdout, "Done. Rebuilt %d index files. Installed %d entries, skipped %d.\n",
	    indxCnt, totFiles, totSkip);

}


/*
 *----------------------------------------------------------------------
 *
 * RecoverFile --
 *
 *	Read one indx file.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Restore index file from tape.
 *
 *----------------------------------------------------------------------
 */

static int
RecoverFile(volStream, oldName)
    int volStream;            /* source volume stream */
    char *oldName;            /* destination file name */
{
    int len;
    static char *buf = NULL;
    int fileStream;

    if (buf == NULL) {
	buf = MEM_ALLOC("RecoverFile", parms.bufSize);
    }

    if ((fileStream=open(oldName, O_WRONLY+O_CREAT, 0660)) == -1) {
	sprintf(printBuf, "Open of %s failed", oldName);
	Utils_Bailout(printBuf, BAIL_PERROR);
    }

    while ((len=Dev_ReadVolume(volStream, buf, parms.bufSize)) > 0) {
	if (write(fileStream, buf, len) != len) {
	    sprintf(printBuf,"Short write. errno %d\n", syserr);
	    return T_FAILURE;
	}
	if (len < sizeof(buf)) {
	    break;
	}
    }

    close(fileStream);

    if (len < 0) {
	return T_FAILURE;
    } else {
	return T_SUCCESS;
    }

}


/*
 *----------------------------------------------------------------------
 *
 * ProcessFile --
 *
 *	Process the restored index file.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Overwrites old index info according to parms flags.
 *
 *----------------------------------------------------------------------
 */

static int
ProcessFile(oldName, fileCntPtr, skipCntPtr)
    char *oldName;            /* source filename */
    int *fileCntPtr;          /* receiving restoration count */
    int *skipCntPtr;          /* receiving skip count */
{
    FILE *fileStream;
    char tarHdr[T_TBLOCK];
    char newName[T_MAXPATHLEN];
    T_FileStat statInfo;
    static int unknownCnt = 0;
    char archPath[T_MAXPATHLEN];
    int retCode = T_SUCCESS;
    int fileCnt;
    int skipCnt;

    if ((fileStream=fopen(oldName, "r")) == (FILE *)NULL) {
	fprintf(stderr,	"Couldn't open %s. Errno %d\n", oldName, errno);
	return -1;
    }

    if (fread(tarHdr, sizeof(tarHdr), 1, fileStream) != 1) {
	retCode = T_FAILURE;
	fprintf(stderr,	"Couldn't read tar block.\n");
    } else if (TBuf_ParseTarHdr(tarHdr, &statInfo) != T_SUCCESS) {
	retCode = T_FAILURE;
	fprintf(stderr,	"Couldn't parse tar block.\n");
    } else if (*statInfo.fileName == '/') {
	retCode = T_FAILURE;
	fprintf(stderr,	"Unexpected thdr filename: %s.\n", statInfo.fileName);
    }
    
    if (retCode != T_SUCCESS) {
	sprintf(newName, "%s/thdr.%d.%d", parms.root, getpid(), unknownCnt++);
	if (rename(oldName, newName) < 0) {
	    fprintf(stdout,"Couldn't rename %s to %s. Errno %d\n",
		    oldName, newName, errno);
	} else {
	    fprintf(stdout,"Preserved file as %s.\n", newName);
	}
    } else {
	strcpy(newName, parms.root);
	strcat(newName, "/");
	strcat(newName, statInfo.fileName);
	if (access(newName, F_OK) != -1) {
	    if (!parms.clobber) {
		sprintf(printBuf, "File '%s' exists. Overwrite it? [y/n] ",
			newName);
		if (!Utils_GetOk(printBuf)) {
		    fclose(fileStream);
		    return T_SUCCESS;
		}
	    }
	    if (unlink(newName) == -1) {
		fprintf(stdout,"Couldn't remove %s. Errno %d\n",
			newName, errno);
	    }
	}
	if (rename(oldName, newName) < 0) {
	    fprintf(stdout,"Couldn't rename %s to %s. Errno %d\n",
		    oldName, newName, errno);
	} else {
	    if (parms.verbose) {
		fprintf(stdout,"Restored %s.\n", newName);
	    }
	    if (!parms.noIndex) {
	        strcpy(archPath, newName);
		*(STRRCHR(archPath, '/')) = '\0';
		if ((RebuildIndx(fileStream, &fileCnt, &skipCnt, archPath)
		     == T_SUCCESS) && (!parms.save)) {
		    unlink(newName);
		}
		if (parms.verbose) {
		    fprintf(stdout,"Installed %d entries, skipped %d.\n",
			    fileCnt, skipCnt);
		}
	    }
	}
    }

    fclose(fileStream);

    *fileCntPtr = fileCnt;
    *skipCntPtr = skipCnt;

    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * RebuildIndx --
 *
 *	Read through file, extracting and rebuilding entries.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
RebuildIndx(fileStream, fileCntPtr, skipCntPtr, archPath)
    int fileStream;           /* source fileStream */    
    int *fileCntPtr;          /* resulting file count */    
    int *skipCntPtr;          /* resulting skip count */    
    char *archPath;           /* owning archive */
{
    int retCode = T_SUCCESS;
    T_FileStat statInfo;
    char indxPath[T_MAXPATHLEN*2];
    char newIndxPath[T_MAXPATHLEN*2];
    FILE *indxStream = NULL;
    int fileCnt = 0;
    int skipCnt = 0;

    *fileCntPtr = 0;
    *indxPath = '\0';

    while (Indx_ReadIndxEntry(fileStream, &statInfo) == T_SUCCESS) {

	/* Need to restore this thing ? */
        if (!NeedToRestore(&statInfo)) {
	    skipCnt++;
	    continue;
	}

	/* find out what indx file file belongs to */
	Indx_MakeIndx(archPath, statInfo.fileName, newIndxPath);

	/* Open indx file, if necessary */
	if (strcmp(newIndxPath, indxPath) != 0) {
	    if (indxStream != NULL) {
		Indx_Close(indxStream);
	    }
	    if (Indx_Open(newIndxPath, "a", &indxStream) != T_SUCCESS) {
		sprintf(printBuf, "Open of %s failed. Errno %d",
			indxPath, syserr);
		Utils_Bailout(printBuf, BAIL_PRINT);
	    }
	    strcpy(indxPath, newIndxPath);
	}

	if (Indx_WriteIndxEntry(&statInfo, -1, indxStream) < 0) {
	    sprintf(printBuf, "Write of %s failed. Errno %d",
		    indxPath, syserr);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	}
	fileCnt++;
    }

    if (indxStream != NULL) {
	Indx_Close(indxStream);
    }

    *fileCntPtr = fileCnt;
    *skipCntPtr = skipCnt;

    return retCode;
}


/*
 *----------------------------------------------------------------------
 *
 * NeedToRestore -- 
 *
 *	Determine whether this entry should be restored or not
 *
 * Results:
 *	1 == need to restore; 0 == skip it
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
NeedToRestore(statInfoPtr)
    T_FileStat *statInfoPtr;
{
    char buf[20];

    if ((*parms.matchPath) &&
	(!Str_Match(statInfoPtr->fileName, parms.matchPath))) {
	return 0;
    }

    if (*parms.matchTBuf) {
	sprintf(buf, "%d", statInfoPtr->tBufId);
	if (!Str_Match(buf, parms.matchTBuf)) {
	    return 0;
	}
    }

    if ((*parms.before) &&
	(Time_Compare(statInfoPtr->vtime, parms.beforeDate, 0) > 0)) {
	return 0;
    }

    if ((*parms.after) &&
	(Time_Compare(statInfoPtr->vtime, parms.afterDate, 02) < 0)) {
	return 0;
    }

    return 1;
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
    Parms *parmsPtr;          /* command line args */
{
    char devName[T_MAXLINELEN];
    struct timeb timeB1;

    if (!*parmsPtr->devName) {
	fprintf(stderr,"Name of reader device? ");
	gets(devName);
	parmsPtr->devName = Str_Dup(devName);
    }
    if ((parmsPtr->first < 0) || (parmsPtr->first > MAXFILE)) {
	Utils_Bailout("Bad -first argument.", BAIL_PRINT);
    }
    if ((parmsPtr->last < -1) || (parmsPtr->last > MAXFILE)) {
	Utils_Bailout("Bad -last argument.", BAIL_PRINT);
    }
    if (*parmsPtr->before) {
	if (getindate(parmsPtr->before, &timeB1) != T_SUCCESS) {
	    Utils_Bailout("Need date in format: 20-Mar-1980:10:20:0",
			  BAIL_PRINT);
	}
	parmsPtr->beforeDate = timeB1.time;
    }
    if (*parmsPtr->after) {
	if (getindate(parmsPtr->after, &timeB1) != T_SUCCESS) {
	    Utils_Bailout("Need date in format: 20-Mar-1980:10:20:0",
			  BAIL_PRINT);
	}
	parmsPtr->afterDate = timeB1.time;
    }
    if ((*parmsPtr->before) && (*parmsPtr->after) &&
	(Time_Compare(parmsPtr->beforeDate, parmsPtr->afterDate, 0) > 0)) {
	Utils_Bailout("Contradictory -before and -after options.", BAIL_PRINT);
    }
    if ((parmsPtr->bufSize <= 0) && (parmsPtr->bufSize % T_TAPEUNIT)){
	sprintf(printBuf, "Need buffer size > 0 and a multiple of %d.\n",
		T_TAPEUNIT);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

}
