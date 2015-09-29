/* 
 * jget.c --
 *
 *	Perform ls on Jaquith archive system.
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
 *       "The limits of my language mean the limits of my world."
 *        -- Ludwig Wittgenstein
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jget.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"
#include "jgetInt.h"

#define STDOUT 1

int syserr;
int jDebug;

static void  CheckOptions  _ARGS_ ((Parms *parmsPtr));
static void  SendStrings   _ARGS_ ((int argc, char **argv, int sock));
static void  ReadAndCopyData _ARGS_ ((int sock, Parms *parmsPtr,
				       char *relName));
static void  ReadAndMakeFiles _ARGS_ ((int sock, Parms *parmsPtr,
				       char *relName));
static int   OpenCreate    _ARGS_ ((T_FileStat *statInfoPtr, int clobber,
				    int *fileStreamPtr));
static int   OpenObject    _ARGS_ ((T_FileStat *statInfoPtr));
static int   RestoreAttrs  _ARGS_ ((T_FileStat *statInfoPtr));
static void  RestoreDirAttrs  _ARGS_ ((Q_Handle *dirQ));
static int   IsWriteable   _ARGS_ ((T_FileStat *statInfoPtr));

static char printBuf[T_MAXSTRINGLEN];
static FILE *memDbg = NULL;

static int myUid;
static char *myName;
static int myGid;
static char *myGroup;
static int rootUid;

Parms parms = {
    "",
    -1,
    "",
    DEF_RANGE,
    DEF_ASOF,
    DEF_SINCE,
    DEF_ABS,
    DEF_OWNER,
    DEF_GROUP,
    DEF_MAIL,
    DEF_RECURSE,
    DEF_MODDATE,
    DEF_ALL,
    DEF_FIRSTVER,
    DEF_LASTVER,
    DEF_FIRSTDATE,
    DEF_LASTDATE,
    DEF_CLOBBER,
    DEF_VERBOSE,
    DEF_TARGET,
    DEF_FILTER,
    DEF_TAR,
    T_TARSIZE
};

Option optionArray[] = {
    {OPT_STRING, "server", (char *)&parms.server, "Server host"},
    {OPT_INT, "port", (char *)&parms.port, "Port of server"},
    {OPT_STRING, "arch", (char *)&parms.arch, "Logical archive name"},
    {OPT_STRING, "range", (char *)&parms.range, "Date range"},
    {OPT_STRING, "asof", (char *)&parms.asof, "Date specification"},
    {OPT_STRING, "since", (char *)&parms.since, "Date specification"},
    {OPT_STRING, "abs", (char *)&parms.abs, "Abstract regular expression"},
    {OPT_STRING, "owner", (char *)&parms.owner, "Userid of owner"},
    {OPT_STRING, "group", (char *)&parms.group, "Group name"},
    {OPT_STRING, "mail", (char *)&parms.mail, "Mail address"},
    {OPT_TRUE, "moddate", (char *)&parms.modDate, "Use last-modification date, not archive date"},
    {OPT_INT, "first", (char *)&parms.firstVer, "Item number. See man page."},
    {OPT_INT, "last", (char *)&parms.lastVer, "Item number. See man page."},
    {OPT_TRUE, "all", (char *)&parms.all, "Synonym for -first 1 -last -1"},
    {OPT_STRING, "target", (char *)&parms.target, "Restore file hierarchy relative to specified directory"},
    {OPT_TRUE, "clobber", (char *)&parms.clobber, "Overwrite existing file."},
    {OPT_TRUE, "v", (char *)&parms.verbose, "Verbose mode."},
    {OPT_TRUE, "dir", (char *)&parms.recurse, "Restore directory and top-level contents only"},
    {OPT_TRUE, "filter", (char *)&parms.filter, "Restore raw data to stdout."},
    {OPT_TRUE, "tar", (char *)&parms.tar, "Restore data to stdout in tar format."},
    {OPT_INT, "tarbuf", (char *)&parms.tarBuf, "Buffer size of -tar option."}
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main driver for als program.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Sends requests across socket to tapestry server.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
int argc;
char *argv[];
{
    int sock;
    T_RespMsgHdr resp;
    char defaultName = '\0';
    char *defaultList = &defaultName;
    int nameCnt = 1;
    char **nameList = &defaultList;
    int i;

/*    memDbg = fopen("jls.mem", "w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckOptions(&parms);

    myUid = geteuid();
    myName = Utils_GetLoginByUid(myUid);
    myGid = getegid();
    myGroup = Utils_GetGroupByGid(myGid);
    rootUid = Utils_GetUidByLogin(ROOT_LOGIN);

    sock = Sock_SetupSocket(parms.port, parms.server, 1);

    Sock_SendReqHdr(sock, T_CMDGET, 0, parms.mail, parms.arch, 0);

    /* put out query parameters */
    Sock_WriteInteger(sock, parms.firstVer);
    Sock_WriteInteger(sock, parms.lastVer);
    Sock_WriteInteger(sock, parms.firstDate);
    Sock_WriteInteger(sock, parms.lastDate);
    Sock_WriteString(sock, parms.owner, 0);
    Sock_WriteString(sock, parms.group, 0);
    Sock_WriteInteger(sock, parms.modDate);
    Sock_WriteInteger(sock, parms.recurse);
    Sock_WriteString(sock, parms.abs, 0);

    /* get ok to continue */
    Sock_ReadRespHdr(sock, &resp);
    if (resp.status != T_SUCCESS) {
	fprintf(stdout,"%s", Utils_MakeErrorMsg(resp.status, resp.errno));
	close(sock);
	exit(-1);
    }

    if (argc > 1) {
	nameCnt = argc - 1;
	nameList = &argv[1];
    }

    for (i=0; i<nameCnt; i++) {
	nameList[i] = Utils_MakeFullPath(nameList[i]);
	Sock_WriteInteger(sock, 1);
	Sock_WriteString(sock, nameList[i], 0);
	if ((parms.filter) || (parms.tar)) {
	    ReadAndCopyData(sock, &parms, nameList[i]);
	} else {
	    ReadAndMakeFiles(sock, &parms, nameList[i]);
	}
    }

    Sock_ReadRespHdr(sock, &resp);
    if (resp.status != T_SUCCESS) {
	fprintf(stdout,"%s", Utils_MakeErrorMsg(resp.status, resp.errno));
    }

    close(sock);
    MEM_REPORT("jget", ALLROUTINES, SORTBYREQ);

    return(0);

}



/*
 *----------------------------------------------------------------------
 *
 * ReadAndCopyData --
 *
 *	Retrieve data for one query and spew it to stdout
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
ReadAndCopyData(sock, parmsPtr, pathName)
    int sock;                 /* incoming socket */
    Parms *parmsPtr;          /* globals */
    char *pathName;           /* user-specified pathname */
{
    int retCode;
    char *buf;
    T_FileStat statInfo;
    int totalSize = 0;
    int fileSize;
    int size;
    int cnt;
    char *oldFileName;
    int pathLen;

    Str_StripDots(pathName);
    pathLen = STRRCHR(pathName, '/') - pathName;
    
    buf = (char *)MEM_ALLOC("ReadAndCopyData", parmsPtr->tarBuf*sizeof(char));

    while ((retCode=Sock_ReadFileStat(sock, &statInfo, 1))
	   == T_SUCCESS) {
	if (!*statInfo.fileName) {
	    if ((parmsPtr->tar) && (totalSize > 0)) {
		TBuf_Terminate(STDOUT, totalSize);
	    }
	    MEM_FREE("ReadAndCopyData", buf);
	    return;
	}
	if (*parmsPtr->target) {
	    oldFileName = statInfo.fileName;
	    statInfo.fileName = Str_Cat(2, parmsPtr->target, 
					oldFileName+pathLen);
	    MEM_FREE("ReadAndCopyData", oldFileName);
	}
	size = statInfo.size;
	if (parmsPtr->tar) {
	    fileSize = TBuf_WriteTarHdr(STDOUT, &statInfo) + size;
	    totalSize += fileSize;
	}
	while (size > 0) {
	    cnt = (size > parmsPtr->tarBuf) ? parmsPtr->tarBuf : size;
	    if ((cnt=read(sock, buf, cnt)) < 0) {
		retCode = T_IOFAILED;
		sprintf(printBuf, "Reading %s", statInfo.fileName);
		perror(printBuf);
		break;
	    }
	    size -= cnt;
	    if (parmsPtr->tar) {
		cnt = parmsPtr->tarBuf;
	    }
	    if ((retCode == T_SUCCESS) &&
		(write(STDOUT, buf, cnt) != cnt)) {
		retCode = T_IOFAILED;
		sprintf(printBuf, "Writing %s", statInfo.fileName);
		perror(printBuf);
	    }
	}
	if (retCode == T_SUCCESS) {
	    if (parmsPtr->verbose) {
		fprintf(stderr,"%s\n", statInfo.fileName);
	    }
	    if (parmsPtr->tar) {
		totalSize += TBuf_Pad(STDOUT, fileSize, T_TBLOCK);
	    }
	    Utils_FreeFileStat(&statInfo, 0);
	}
    }
    MEM_FREE("ReadAndCopyData", buf);
    printf("Got error trying to read T_FileStat\n");

}


/*
 *----------------------------------------------------------------------
 *
 * ReadAndMakeFiles --
 *
 *	Retrieve and build all the files for one query
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
ReadAndMakeFiles(sock, parmsPtr, pathName)
    int sock;                 /* incoming socket */
    Parms *parmsPtr;          /* globals */
    char *pathName;           /* user-specified pathname */
{
    int fileStream;
    int retCode;
    char buf[T_BUFSIZE];
    T_FileStat statInfo;
    int size;
    int cnt;
    char *oldFileName;
    int pathLen;
    Q_Handle *dirQ;
    T_FileStat *newInfoPtr;

    dirQ = Q_Create("dirList", 0);

    Str_StripDots(pathName);
    pathLen = STRRCHR(pathName, '/') - pathName;
    
    while ((retCode=Sock_ReadFileStat(sock, &statInfo, 1))
	   == T_SUCCESS) {
	if (!*statInfo.fileName) {
	    RestoreDirAttrs(dirQ);
	    Q_Destroy(dirQ);
	    return;
	}
	if (*parmsPtr->target) {
	    oldFileName = statInfo.fileName;
	    statInfo.fileName = Str_Cat(2, parmsPtr->target, 
					oldFileName+pathLen);
	    MEM_FREE("ReadAndMakeFiles", oldFileName);
	}
	if ((retCode=OpenCreate(&statInfo, parmsPtr->clobber, &fileStream))
	    != T_SUCCESS) {
	    perror(statInfo.fileName);
	}
	size = statInfo.size;
	while (size > 0) {
	    cnt = (size > sizeof(buf)) ? sizeof(buf) : size;
	    if ((cnt=read(sock, buf, cnt)) < 0) {
		retCode = T_IOFAILED;
		sprintf(printBuf, "Reading %s", statInfo.fileName);
		perror(printBuf);
		break;
	    }
	    if ((retCode == T_SUCCESS) &&
		(write(fileStream, buf, cnt) != cnt)) {
		retCode = T_IOFAILED;
		sprintf(printBuf, "Writing %s", statInfo.fileName);
		perror(printBuf);
	    }
	    size -= cnt;
	}
	if ((retCode == T_SUCCESS) && (parmsPtr->verbose)) {
	    fprintf(stdout,"%s\n", statInfo.fileName);
	}
	if (retCode == T_SUCCESS) {
	    /*
	     * If it's a directory and it's not writeable, save its permissions
	     * so we can restore them after we've created all its children.
	     */
	    if (S_ISDIR(statInfo.mode) &&
		(!IsWriteable(&statInfo))) {
		newInfoPtr = Utils_CopyFileStat(&statInfo);
		Q_Add(dirQ, (Q_ClientData) newInfoPtr, Q_TAILQ);
	    } else {
		RestoreAttrs(&statInfo);
	    }
	    Utils_FreeFileStat(&statInfo, 0);
	}
	if (fileStream > 0) {
	    close(fileStream);
	}
    }
    printf("Got error trying to read T_FileStat\n");

}


/*
 *----------------------------------------------------------------------
 *
 * RestoreDirAttrs --
 *
 *	Run through list of protected directories restoring their attrs.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Frees directory items after they're restored.
 *
 *----------------------------------------------------------------------
 */

static void
RestoreDirAttrs(dirQ)
    Q_Handle *dirQ;           /* source item to be restored */
{
    int count;
    T_FileStat *statInfoPtr;

    count = Q_Count(dirQ);
    while (count-- > 0) {
	statInfoPtr = (T_FileStat *)Q_Remove(dirQ);
	RestoreAttrs(statInfoPtr);
	Utils_FreeFileStat(statInfoPtr, 1);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * RestoreAttrs --
 *
 *	Recreate file's attributes
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
RestoreAttrs(statInfoPtr)
    T_FileStat *statInfoPtr;
{
    int retCode = T_SUCCESS;
#ifdef SYSV
    struct utimbuf fileTimes;
#else
    struct timeval fileTimes[2];
#endif

    if (!S_ISALNK(statInfoPtr->mode)) {
#ifdef SYSV
	fileTimes.actime = statInfoPtr->atime;
	fileTimes.modtime = statInfoPtr->mtime;
        if (utime(statInfoPtr->fileName, &fileTimes) < 0) {
#else
        fileTimes[0].tv_sec = statInfoPtr->atime;
	fileTimes[0].tv_usec = 0;
        fileTimes[1].tv_sec = statInfoPtr->mtime;
	fileTimes[1].tv_usec = 0;
        if (utimes(statInfoPtr->fileName, fileTimes) < 0) {
#endif
	    sprintf(printBuf, "Restoring access times %s",
		    statInfoPtr->fileName);
	    perror(printBuf);
	}
	if (chmod(statInfoPtr->fileName, statInfoPtr->mode) < 0) {
	    sprintf(printBuf, "Restoring permissions %s",
		    statInfoPtr->fileName);
	    perror(printBuf);
	}
    }

    if (chown(statInfoPtr->fileName, 
	      (uid_t)statInfoPtr->uid, (gid_t)statInfoPtr->gid) < 0) {
	sprintf(printBuf, "Restoring owner/group %s",
		statInfoPtr->fileName);
	perror(printBuf);
    }


    return retCode;
}



/*
 *----------------------------------------------------------------------
 *
 * OpenCreate --
 *
 *	Open file, creating directories as needed.
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
OpenCreate(statInfoPtr, clobber, fileStreamPtr)
    T_FileStat *statInfoPtr;
    int clobber;
    int *fileStreamPtr;
{
    char *curPtr;
    char *safeName;

    if (clobber) {
	if (S_ISDIR(statInfoPtr->mode)) {
	    safeName = Str_Quote(statInfoPtr->fileName);
	    sprintf(printBuf, "rm -rf %s", safeName);
	    system(printBuf);
	    MEM_FREE("OpenCreate",safeName);
	} else {
	    unlink(statInfoPtr->fileName);
	}
    }

    *fileStreamPtr = OpenObject(statInfoPtr);

    if (*fileStreamPtr > -1) {
	return T_SUCCESS;
    } else if (errno != ENOENT) {
	return T_FAILURE;
    }
    
    /*
     * some part of the path is not there
     */
    curPtr = statInfoPtr->fileName;
    
    while (curPtr != NULL) {
	if ((curPtr=STRCHR(curPtr+1, '/')) == NULL) {
	    return T_FAILURE;
	}
	*curPtr = '\0';
	if (access(statInfoPtr->fileName, X_OK) == -1) {
	    break;
	}
	*curPtr = '/';
    } 

    while (curPtr != NULL) {
	*curPtr = '\0';
	if (mkdir(statInfoPtr->fileName, 0700) == -1) {
	    return T_FAILURE;
	}
	*curPtr = '/';
	curPtr = STRCHR(curPtr+1, '/');
    } 

    /*
     * Now retry the open
     */
    *fileStreamPtr = OpenObject(statInfoPtr);

    return (*fileStreamPtr == -1) ? T_FAILURE : T_SUCCESS;

}


/*
 *----------------------------------------------------------------------
 *
 * OpenObject --
 *
 *	Do the right thing to create the object.
 *
 * Results:
 *	Open stream or resulting return code.
 *
 * Side effects:
 *	Opens file, makes directory, link or device.
 *
 *----------------------------------------------------------------------
 */

static int
OpenObject(statInfoPtr)
    T_FileStat *statInfoPtr;
{
    int result = T_SUCCESS;
    int mode = statInfoPtr->mode & S_IFMT;
    char *fileName = statInfoPtr->fileName;
    int perm = statInfoPtr->mode & 0777;

    switch (mode) {
    case S_IFDIR:
	mkdir(fileName, 0700); /* we'll change the permission later */
	break;
    case S_IFLNK:
	result = symlink(statInfoPtr->linkName, fileName);
	break;
    case S_IFBLK:
    case S_IFCHR:
	result = mknod(fileName, perm, statInfoPtr->rdev);
	break;
    case S_IFIFO:
	result = mknod(fileName, S_IFIFO+perm, statInfoPtr->rdev);
	break;
    case S_IFRLNK:
	result = MAKERMTLINK(statInfoPtr->linkName, fileName, 1);
	break;
    default:
	result = open(statInfoPtr->fileName,
			  O_WRONLY+O_CREAT+O_EXCL, perm);
	break;
    }

    return result;
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
    struct timeb timeB1;
    struct timeb timeB2;
    char *envPtr;
    int targetLen;
    struct stat unixStatBuf;

    if ((!*parmsPtr->arch) &&
	((envPtr=getenv(ENV_ARCHIVE_VAR)) != (char *)NULL)) {
	parmsPtr->arch = envPtr;
    }
    if ((*parmsPtr->arch) &&
	(Utils_CheckName(parmsPtr->arch, 1) == T_FAILURE)) {
	sprintf(printBuf,
		"Bad archive name: '%s'.\nUse -arch or set %s environment variable.\n",
		parmsPtr->arch, ENV_ARCHIVE_VAR);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if ((!*parmsPtr->server) &&
	((parmsPtr->server=getenv(ENV_SERVER_VAR)) == (char *)NULL)) {
	parmsPtr->server = DEF_SERVER;
    }
    if (Utils_CheckName(parmsPtr->server, 1) == T_FAILURE) {
	sprintf(printBuf,
		"Bad server name: '%s'.\nUse -server or set %s environment variable.\n",
		parmsPtr->server, ENV_SERVER_VAR);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if (parmsPtr->port == -1) {
	if ((envPtr=getenv(ENV_PORT_VAR)) == (char *)NULL) {
	    parmsPtr->port = DEF_PORT;
	} else {
	    Utils_CvtInteger(envPtr, 1, SHRT_MAX, &parmsPtr->port);
	}
    }
    if ((parmsPtr->port < 1) || (parmsPtr->port > SHRT_MAX)) {
	sprintf(printBuf,
		"Bad port number %d.\nUse -port or set %s environment variable\n",
		parmsPtr->port, ENV_PORT_VAR);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if ((*parmsPtr->mail) && 
	(Utils_CheckName(parmsPtr->mail, 1) != T_SUCCESS)) {
	sprintf(printBuf, "Bad mail address: '%s'\n", parmsPtr->mail);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if ((strcmp(parmsPtr->owner, DEF_OWNER) != 0) &&
	(Utils_GetUidByLogin(parmsPtr->owner) == T_FAILURE)) {
	sprintf(printBuf, "Don't know user '%s'. Continue? [y/n] ",
		parmsPtr->owner);
        if (!Utils_GetOk(printBuf)) {
	    exit(-1);
	}
    }
    if ((strcmp(parmsPtr->group, DEF_GROUP) != 0) &&
	(Utils_GetGidByGroup(parmsPtr->group) == T_FAILURE)) {
	sprintf(printBuf, "Don't know group '%s'. Continue? [y/n] ",
		parmsPtr->group);
        if (!Utils_GetOk(printBuf)) {
	    exit(-1);
	}
    }
    if ((*parmsPtr->asof) && (*parmsPtr->since)) {
	Utils_Bailout("Conflicting parameters: -asof -since\n", BAIL_PRINT);
    }
    if (*parmsPtr->asof) {
	if (getindate(parmsPtr->asof, &timeB1) != T_SUCCESS) {
	    Utils_Bailout("Need date in format: 20-Mar-1980:10:20:0",
			  BAIL_PRINT);
	}
	parmsPtr->firstDate = 0;
	parmsPtr->lastDate = timeB1.time;
    }
    if (*parmsPtr->since) {
	if (getindate(parmsPtr->asof, &timeB1) != T_SUCCESS) {
	    Utils_Bailout("Need date in format: 20-Mar-1980:10:20:0",
			  BAIL_PRINT);
	}
	parmsPtr->firstDate = timeB1.time;
	parmsPtr->lastDate = Time_GetCurDate();
    }
    if (*parmsPtr->range) {
	if (getindatepair(parmsPtr->range, &timeB1, &timeB2) !=
	    T_SUCCESS) {
	    Utils_Bailout("Need date pair in format: date,date2", BAIL_PRINT);
	}
	parmsPtr->firstDate = timeB1.time;
	parmsPtr->lastDate  = timeB2.time;
    }
    if (parmsPtr->all) {
	if ((parms.firstVer != DEF_FIRSTVER) ||
	    (parms.lastVer != DEF_LASTVER)) {
	    Utils_Bailout("Don't mix -all option with -first or -last",
		    BAIL_PRINT);
	} else {
	    parms.firstVer = 1;
	    parms.lastVer = -1;
	}
    }
    if ((parms.firstVer == 0) || (parms.lastVer == 0)) {
	Utils_Bailout("Version number cannot be 0.\n", BAIL_PRINT);
    }
    if (*parmsPtr->target) {
	if (stat(parmsPtr->target, &unixStatBuf) != 0) {
	    Utils_Bailout("Couldn't read or access target directory\n",
			  BAIL_PRINT);
	}
	if (!S_ISADIR(unixStatBuf.st_mode)) {
	    Utils_Bailout("Specified target isn't a directory.\n", BAIL_PRINT);
	}
    }
    targetLen = strlen(parmsPtr->target) - 1;
    if (*(parmsPtr->target+targetLen) == '/') {
	*(parmsPtr->target+targetLen) = '\0';
    }
    if ((parmsPtr->tarBuf < 0) ||
	(parmsPtr->tarBuf % T_TBLOCK)) {
	sprintf(printBuf, "tarbuf argument must be > 0 and a multiple of %d.\n",
		T_TBLOCK);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * IsWriteable --
 *
 *	Check for proper write permission.
 *
 * Results:
 *	1 == writable; 0 = protected.
 *
 * Side effects:
 *	None.
 *
 * Note:
 *      Uses global uid, gid variables
 *
 *----------------------------------------------------------------------
 */

static int
IsWriteable(unixStatPtr)
    struct stat *unixStatPtr; /* file in question */
{
    int perm = unixStatPtr->st_mode & 0777;

    if ((myUid == rootUid) ||
	(perm & 02) ||
	((perm & 020) && (myGid == unixStatPtr->st_gid)) ||
	((perm & 0200) && (myUid == unixStatPtr->st_uid))) {
	return 1;
    }

    return 0;
}
