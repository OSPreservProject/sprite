/* 
 * jput.c --
 *
 *	Perform a write on Jaquith archive system.
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
 *      "Action causes more trouble than thought."
 *      -- Jenny Holzer
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jput.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"
#include "jputInt.h"

int syserr;
char **progOptions;

static void  CheckOptions     _ARGS_ ((Parms *parmsPtr));
static int   CheckOneFile     _ARGS_ ((int sock, char *fileName,
				       int recurse));
static int   PutOneFile       _ARGS_ ((int sock, char *fileName,
				       int recurse, int followLink,
				       int cross));
static int   PutOneFileStat   _ARGS_ ((int sock, struct stat *statPtr,
				       char *fileName, char *abstract));
static char *ListOneDirectory _ARGS_ ((char *dirName));
static char *GetFileAbstract  _ARGS_ ((char *prog, char *progOptions,
				       char *fileName));
static char **ParseProgOptions _ARGS_ ((char *prog, char *options));

static char printBuf[T_MAXSTRINGLEN];
static FILE *memDbg = NULL;

static uid_t myUid;
static char *myName;
static gid_t myGid;
static char *myGroup;
static gid_t rootUid;

static Parms parms = {
    "",
    -1,
    "",
    DEF_ABSTRACT,
    DEF_ABSFILTER,
    DEF_MAIL,
    DEF_FORCE,
    DEF_SYNC,
    DEF_NEWVOL,
    DEF_LOCAL,
    DEF_RECURSE,
    DEF_LINK,
    DEF_VERBOSE,
    DEF_MODTIME,
    DEF_MODTIMEVAL,
    DEF_ABSFILTEROPT,
    DEF_PRUNE,
    DEF_PRUNEPATH,
    DEF_IGNORE,
    DEF_CROSS,
    DEF_ACKFREQ
};

Option optionArray[] = {
    {OPT_STRING, "server", (char *)&parms.server, "Server hostname"},
    {OPT_INT, "port", (char *)&parms.port, "Port of server"},
    {OPT_STRING, "arch", (char *)&parms.arch, "Logical archive name"},
    {OPT_STRING, "mail", (char *)&parms.mail, "Mail address"},
    {OPT_STRING, "abs", (char *)&parms.abstract, "Text abstract"},
    {OPT_STRING, "absfilter", (char *)&parms.absFilter, "Program to generate text abstract"},
    {OPT_STRING, "absfilteropt", (char *)&parms.absFilterOpt, "Options for abstract filter program"},
    {OPT_TRUE, "link", (char *)&parms.link, "Follow symbolic links"},
    {OPT_TRUE, "v", (char *)&parms.verbose, "Verbose"},
    {OPT_STRING, "mod", (char *)&parms.modTime, "Put files modified since date"},
    {OPT_CONSTANT(T_FORCE), "force", (char *)&parms.force, "Force files to archive, even if not modified since last archived"},
    {OPT_CONSTANT(T_SYNC), "sync", (char *)&parms.sync, "Write data to volume synchronously"},
    {OPT_CONSTANT(T_NEWVOL), "newvol", (char *)&parms.newvol, "Start a new volume. The -sync option is required"},

    {OPT_CONSTANT(T_LOCAL), "local", (char *)&parms.local, "File system is local to server"},

    {OPT_TRUE, "dir", (char *)&parms.recurse, "Put directory and top-level contents only"},
/*
    {OPT_TRUE, "filter", (char *)&parms.filter, "Run filter on file before archiving it."},
    {OPT_TRUE, "filteropt", (char *)&parms.filterOpt, "Options for filter program"},
*/
    {OPT_STRING, "prune", (char *)&parms.prune, "Don't archive subtrees whose simple name matches globbing expression"},
    {OPT_STRING, "prunepath", (char *)&parms.prunePath, "Don't archive subtrees whose full path name matches globbing expression"},
    {OPT_STRING, "ignore", (char *)&parms.ignore, "Don't archive and don't record files whose name matches globbing expression"},
    {OPT_TRUE, "crossremote", (char *)&parms.cross, "Cross over remote link boundaries (Sprite systems only)"},
    {OPT_INT, "ackfreq", (char *)&parms.ackFreq, "Acknowledge every Nth file"}
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main driver for jput program.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Sends requests across socket to Jaquith server.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
int argc;
char *argv[];
{
    int sock;
    int retCode;
    int i;
    T_RespMsgHdr resp;
    char defaultName = '\0';
    char *defaultList = &defaultName;
    char **nameList = &defaultList;
    char *fileName;
    int nameCnt = 1;
    static T_FileStat dummyFile =
	{"", "", "", "", "", "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/*    memDbg = fopen("jput.mem", "w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS+CHECKALLBLKS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckOptions(&parms);

    sock = Sock_SetupSocket(parms.port, parms.server, 1);

    Sock_SendReqHdr(sock, T_CMDPUT, 0, parms.mail,  parms.arch,
		    (parms.force | parms.sync | parms.newvol | parms.local));

    /* Announce acknowledgement frequency */
    if (Sock_WriteInteger(sock, parms.ackFreq) != T_SUCCESS) {
	sprintf(printBuf, "Couldn't send ack frequency. Errno %d\n", syserr);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }

    /* get ok to proceed */
    Sock_ReadRespHdr(sock, &resp);
    if (resp.status != T_SUCCESS) {
	fprintf(stdout,"%s", Utils_MakeErrorMsg(resp.status, resp.errno));
	close(sock);
	exit(-1);
    }

    if (argc > 1) {
	nameCnt = argc-1;
	nameList = &argv[1];
    }

    /*
     * For each item in list, conditionally put out an entry for
     * all the parts leading up to it.
     */
    for (i=0; i<nameCnt; i++) {
        fileName = Utils_MakeFullPath(nameList[i]);
	if (((*parms.prunePath) && (Str_Match(fileName, parms.prunePath))) ||
	    ((*parms.prune) && (Str_Match(nameList[i], parms.prune)))) {
	    fprintf(stderr,"Warning: pruning top-level item: %s\n",
		    nameList[i]);
	} else if ((!*parms.ignore) ||
		   (!Str_Match(nameList[i], parms.ignore))) {
	    retCode = CheckOneFile(sock, fileName, parms.recurse);
	    MEM_FREE("jput", fileName);
	}
    }

    /* Put end of list marker */
    Sock_WriteFileStat(sock, &dummyFile, 0);

    /* see what happened */
    Sock_ReadRespHdr(sock, &resp);
    if (resp.status != T_SUCCESS) {
	fprintf(stdout,"%s", Utils_MakeErrorMsg(resp.status, resp.errno));
    }

    close(sock);

    MEM_REPORT("jput", ALLROUTINES, SORTBYREQ);

    return(0);

}


/*
 *----------------------------------------------------------------------
 *
 * CheckOneFile -- 
 *
 *	Write a single file's meta info over socket and get response
 *
 * Results:
 *	Error if file not found or sock dies.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static int
CheckOneFile(sock, fileName, recurse)
    int sock;                 /* outgoing socket */
    char *fileName;           /* full name of file */
    int recurse;              /* recursion flag */
{
    struct stat unixStatBuf;
    char partName[T_MAXPATHLEN];
    int retCode;
    char **partList;
    int partCnt;
    int i;
    char *insidePtr;

    if (strlen(fileName) > T_MAXPATHLEN) {
	fprintf(stderr, "%s: pathname exceeds %d characters.\n",
		fileName, T_MAXPATHLEN);
	return T_FAILURE;
    }
    if (parms.link) {
	retCode = stat(fileName, &unixStatBuf);
    } else {
	retCode = lstat(fileName, &unixStatBuf);
    }
    if (retCode < 0) {
	perror(fileName);
	return errno;
    }

    if ((!S_ISADIR(unixStatBuf.st_mode)) &&
	(*(fileName+strlen(fileName)-1) == '/')) {
	fprintf(stderr,"%s: not a directory\n", fileName);
	return T_FAILURE;
    }

    if (Time_Compare(unixStatBuf.st_mtime, parms.modTimeVal, 0) <= 0) {
	return T_SUCCESS;
    }

    /*
     * put out all the directories leading up to the file
     */
    partList = Str_Split(fileName, '/', &partCnt, 0, &insidePtr);
    retCode = PutOneFile(sock, "/", 0, 0, 0);
    for (i=1,*partName='\0'; i<partCnt-1; i++) {
	strcat(partName, "/");
	strcat(partName, partList[i]);
	retCode = PutOneFile(sock, partName, 0, 1, 0);
    }

    retCode = PutOneFile(sock, fileName, parms.recurse, parms.link, 1);
    MEM_FREE("jput", partList);
    MEM_FREE("jput", (char *)insidePtr);

    return retCode;
}



/*
 *----------------------------------------------------------------------
 *
 * PutOneFile -- 
 *
 *	Write a single file's name and contents out over socket.
 *
 * Results:
 *	Error if file not found or sock dies.
 *
 * Side effects:
 *	none.
 *
 * Note:
 *      This algorithm is horrid. Presently we are sending the current
 *      file metadata to the server which does a comparison with the
 *      metadata stored in the index. The server then returns a 
 *      "do or don't archive" flag to tell us whether to send the
 *      file or not. What we should be doing is just issuing a standard
 *      jls-style command for the file and doing the comparison ourselves.
 *
 *----------------------------------------------------------------------
 */

static int
PutOneFile(sock, fileName, recurse, followLink, cross)
    int sock;                 /* outgoing socket */
    char *fileName;           /* full name of file */
    int recurse;              /* recursion flag */
    int followLink;           /* follow symbolic links */
    int cross;                /* cross remote links (Sprite only) */
{
    int retCode;
    int fileStream;
    char *abstract;
    char buf[T_BUFSIZE];
    DIR *dirStream;
    DirObject *entryPtr;
    char newName[2*T_MAXPATHLEN];
    int newLen;
    int readCnt;
    int len;
    int size;
    int type;
    struct stat unixStatBuf;
    char *newNamePtr = newName;
    int dontArchive = 0;
    char nullByte = '\0';
    static int filesSinceAck = 0;

    MEM_REPORT("jput", ALLROUTINES, SORTBYREQ);

    if (followLink) {
	retCode = stat(fileName, &unixStatBuf);
    } else {
	retCode = lstat(fileName, &unixStatBuf);
    }
    if (retCode == -1) {
	sprintf(printBuf, "%s: Stat failed", fileName);
	perror(printBuf);
	return retCode;
    }
    if ((myUid != rootUid) &&
	(access(fileName, R_OK) != 0)) {
	fprintf(stdout, "%s: permission denied.\n", fileName);
	unixStatBuf.st_size = 0; /* put out a dummy file */
    } 
    if (*parms.absFilter) {
	abstract = GetFileAbstract(parms.absFilter, progOptions, fileName);
    } else {
	abstract = parms.abstract;
    }

    if ((retCode=PutOneFileStat(sock, &unixStatBuf, fileName, abstract))
	!= T_SUCCESS) {
	return retCode;
    }

    if (!parms.force) {
	if (Sock_ReadString(sock, &newNamePtr, 0) != T_SUCCESS) {
	    sprintf(printBuf, "%s: server name confirmation failed\n",
		    fileName);
	    Utils_Bailout(printBuf, BAIL_PRINT); 
	}
	if (strcmp(newNamePtr, fileName) != 0) {
	    sprintf(printBuf, "expected name '%s' != '%s'\n",
		    fileName, newNamePtr);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	}
	if (Sock_ReadInteger(sock, &dontArchive) != T_SUCCESS) {
	    sprintf(printBuf, "%s: server go-ahead confirmation failed\n",
		    fileName);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	}
	if ((dontArchive != 0) && (dontArchive != 1)) {
	    sprintf(printBuf, "go-ahead for %s is strange: %d\n",
		    fileName, dontArchive);
	    Utils_Bailout(printBuf, BAIL_PRINT);
	}
    }

    if ((!dontArchive) && (parms.verbose)) {
	printf("%s %d\n", fileName, unixStatBuf.st_size);
    }
    if ((!dontArchive) &&
	(!parms.local) &&
	((int)unixStatBuf.st_size > 0)) {
	size = (int)unixStatBuf.st_size;
	if ((fileStream=open(fileName, O_RDONLY, 0)) == -1) {
	    perror(fileName);
	    strncpy(buf, &nullByte, sizeof(buf));
	    while (size > 0) {
		len = (size > sizeof(buf)) ? sizeof(buf) : size;
		if (Sock_WriteNBytes(sock, buf, len) != len) {
		    exit(-1);
		}
		size -= len;
	    }
	    return T_IOFAILED;
	}
	
	while (size > 0) {
	    len = (size > sizeof(buf)) ? sizeof(buf) : size;
	    if ((readCnt=Sock_ReadNBytes(fileStream, buf, len)) > 0) {
		if (Sock_WriteNBytes(sock, buf, readCnt) != readCnt) {
		    fprintf(stderr,"Connection to server died.\n");
		    exit(-1);
		}
		size -= readCnt;
	    } else {
		break;
	    }
	}
	
	/*
	 * If didn't complete, print message and send zeros
	 */
	if (size > 0) {
	    fprintf(stderr,"PutOneFile: short %d bytes for '%s'. errno %d\n",
		    size, fileName, errno);
	    strncpy(buf, &nullByte, sizeof(buf));
	    while (size > 0) {
		len = (size > sizeof(buf)) ? sizeof(buf) : size;
		if (Sock_WriteNBytes(sock, buf, len) != len) {
		    exit(-1);
		}
		size -= len;
	    }
	}
	close(fileStream);
    }

    /*
     * Time to wait for ack ?
     */
    if (parms.ackFreq > 0) {
	if (++filesSinceAck >= parms.ackFreq) {
	    Sock_ReadString(sock, &newNamePtr, 0);
	    if (strcmp(newName, fileName) != 0) {
		Utils_Bailout("Ack failed:\n\tGot %s\n\tExpected %s\n",
			      newName, fileName);
	    }
	    filesSinceAck = 0;
	}
    }

    /*
     * If this file is a directory (or we are crossing remote links)
     * and we're dumping recursively, we have more work to do.
     */
    type = unixStatBuf.st_mode & S_IFMT;
    if ((recurse > 0) &&
	((type == S_IFDIR) || (cross && (type == S_IFRLNK)))) {
	if ((dirStream=opendir(fileName)) == (DIR *)NULL) {
	    return T_IOFAILED;
	}
	recurse--;
	strcpy(newName, fileName);
	newLen = strlen(newName);
	*(newName+newLen++) = '/';
	while ((entryPtr=readdir(dirStream)) != (DirObject *)NULL) {
	    strcpy(newName+newLen, entryPtr->d_name);
	    if ((strcmp(entryPtr->d_name, ".") == 0) ||
		(strcmp(entryPtr->d_name, "..") == 0) ||
		(Str_Match(entryPtr->d_name, parms.ignore)) ||
		((*parms.prunePath)&&(Str_Match(newName, parms.prunePath))) ||
		((*parms.prune)&&(Str_Match(entryPtr->d_name, parms.prune)))) {
		  continue;
	    }
	    retCode = PutOneFile(sock, newName, recurse,
				 followLink, parms.cross);
	}
	closedir(dirStream);
    }
    
    return retCode;

}


/*
 *----------------------------------------------------------------------
 *
 * PutOneFileStat -- 
 *
 *	Write a single file's status info
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies size field to tell caller not to dump data.
 *
 *----------------------------------------------------------------------
 */

static int
PutOneFileStat(sock, unixStatPtr, fileName, abstract)
    int sock;                 /* outgoing socket */
    struct stat *unixStatPtr; /* outgoing metadata */
    char *fileName;           /* outgoing complete file path */
    char *abstract;           /* outgoing abstract */
{
    T_FileStat statInfo;
    int retCode;
    int  type = unixStatPtr->st_mode & S_IFMT; 
    char linkName[T_MAXPATHLEN];
    char null = '\0';
    static Hash_Handle *uidHandle = (Hash_Handle *)NULL;
    static Hash_Handle *gidHandle = (Hash_Handle *)NULL;
    char *uname;
    char *gname;

    if (uidHandle == (Hash_Handle *) NULL) {
	uidHandle = Hash_Create("login", 59, Utils_StringHashProc, 1);
	gidHandle = Hash_Create("group", 59, Utils_StringHashProc, 1);
    }

    if (Hash_Lookup(uidHandle, (Hash_Key)&unixStatPtr->st_uid,
		    sizeof(unixStatPtr->st_uid),
		    (Hash_ClientData *)&uname) == T_FAILURE) {
	uname = Utils_GetLoginByUid(unixStatPtr->st_uid);
	Hash_Insert(uidHandle, (Hash_Key)&unixStatPtr->st_uid,
		    sizeof(unixStatPtr->st_uid), (Hash_ClientData)uname);
    }
    if (Hash_Lookup(gidHandle, (Hash_Key)&unixStatPtr->st_gid,
		    sizeof(unixStatPtr->st_gid),
		    (Hash_ClientData *)&gname) == T_FAILURE) {
	gname = Utils_GetGroupByGid(unixStatPtr->st_gid);
	Hash_Insert(gidHandle, (Hash_Key)&unixStatPtr->st_gid,
		    sizeof(unixStatPtr->st_gid), (Hash_ClientData)gname);
    }

    statInfo.fileName = fileName;
    statInfo.linkName = linkName;
    statInfo.uname = uname;
    statInfo.gname = gname;
    statInfo.abstract = abstract;
    statInfo.fileList = &null;

    if (!S_ISREG(type)) {
	unixStatPtr->st_size = 0;
    }
    if (S_ISADIR(type)) {
	statInfo.fileList = ListOneDirectory(fileName);
    }
    if (S_ISALNK(type)) {
	linkName[readlink(fileName, linkName, T_MAXPATHLEN)] = '\0';
	/* This is a tar restriction */
	if (strlen(linkName) > LINKNAMELEN) {
	    fprintf(stderr, "%s: truncating link name to %d chars.\n",
		    fileName, LINKNAMELEN);
	    strcpy(linkName+LINKNAMELEN-10, ".truncated");
	}
    } else {
	*linkName = '\0';
    }

    statInfo.size = unixStatPtr->st_size;
    statInfo.atime = unixStatPtr->st_atime;
    statInfo.mtime = unixStatPtr->st_mtime;
    statInfo.volId = 0;
    statInfo.filemark = 0;
    statInfo.tBufId = 0;
    statInfo.offset = 0;
    statInfo.uid = (int) unixStatPtr->st_uid;
    statInfo.gid = (int) unixStatPtr->st_gid;
    statInfo.mode = unixStatPtr->st_mode;
    statInfo.vtime = 0;
    if ((type == S_IFCHR) || (type == S_IFBLK)) {
	statInfo.rdev = unixStatPtr->st_rdev;
    } else {
	statInfo.rdev = 0;
    }
    
    retCode = Sock_WriteFileStat(sock, &statInfo, 0);

    if (S_ISADIR(type)) {
	MEM_FREE("PutOneFileStat", statInfo.fileList);
    }

    return(retCode);

}



/*
 *----------------------------------------------------------------------
 *
 * ListOneDirectory --
 *
 *	Form a string of filename in a directory.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

static char *
ListOneDirectory(dirName)
char *dirName;
{
    DIR *dirStream;
    DirObject *entryPtr;
    int cnt = 0;
    int i = 0;
    char *bufPtr;
    char *namePtr;
    int nameLen;
    char **nameArray;
    int totLen = 1;

    if ((dirStream=opendir(dirName)) == (DIR *)NULL) {
	return Str_Dup("");
    }

    while ((entryPtr=readdir(dirStream)) != (DirObject *)NULL) {
	if ((strcmp(entryPtr->d_name, ".") != 0) &&
	    (strcmp(entryPtr->d_name, "..") != 0) &&
	    (Str_Match(entryPtr->d_name, parms.ignore) == 0)) {
	    cnt++;
	}
    }

    if (cnt == 0) {
        closedir(dirStream);
	return Str_Dup("");
    }

    nameArray = (char **)MEM_ALLOC("ListOneDirectory",cnt*sizeof(char *));

    rewinddir(dirStream);
    
    while (i < cnt) {
	if ((entryPtr=readdir(dirStream)) == (DirObject *)NULL) {
	    break;
	}
	nameLen = strlen(entryPtr->d_name) + 1;
	if ((strcmp(entryPtr->d_name, ".") != 0) &&
	    (strcmp(entryPtr->d_name, "..") != 0) &&
	    (Str_Match(entryPtr->d_name, parms.ignore) == 0)) {
	    nameArray[i] = MEM_ALLOC("ListOneDirectory", nameLen);
	    strcpy(nameArray[i], entryPtr->d_name);
	    totLen += nameLen;
	    i++;
	}
    }

    closedir(dirStream);

    bufPtr = namePtr = (char *)MEM_ALLOC("ListOneDirectory", totLen);

    for (cnt=0; cnt < i; cnt++) {
	strcpy(namePtr,nameArray[cnt]);
	nameLen = strlen(nameArray[cnt]);
	namePtr += nameLen;
	*namePtr++ = ' ';
	MEM_FREE("ListOneDirectory", nameArray[cnt]);
    }
    *(namePtr-1) = '\0';

    MEM_FREE("ListOneDirectory", (char *)nameArray);

    return bufPtr;
}


/*
 *----------------------------------------------------------------------
 *
 * ReportError --
 *
 *	Translate error into reasonable msg
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
ReportError(status, errno)
int status;
int errno;
{
    /* This is all temporary */
    char *msg = "?????";
    char *msgPtr;

    if ((errno < 0) || (errno > sys_nerr)) {
	msgPtr = msg;
    } else {
	msgPtr = sys_errlist[errno];
    }
    fprintf(stderr,"status = %d. errno = %d\n%s\n",
	    status, errno, msgPtr);
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
 *	Will set global variable progOptions if -absfilteropt is given.
 *
 *----------------------------------------------------------------------
 */

static void
CheckOptions(parmsPtr)
Parms *parmsPtr;
{
    struct timeb timeB1;
    char *envPtr;

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
    if (strlen(parmsPtr->abstract) > T_MAXSTRINGLEN) {
	sprintf(printBuf,"Abstract cannot exceed %d chars.", T_MAXSTRINGLEN);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if ((*parmsPtr->abstract) && (*parmsPtr->absFilter)) {
	Utils_Bailout("Can't supply both -abs and -absfilter options.",
		      BAIL_PRINT);
    }
    if ((!*parmsPtr->absFilter) && (*parmsPtr->absFilterOpt)) {
	Utils_Bailout("Need -absfilter option with -absfilteropt.",
		      BAIL_PRINT);
    }
    if ((progOptions=ParseProgOptions(parmsPtr->absFilter,
				      parmsPtr->absFilterOpt)) == NULL) {
	 Utils_Bailout("Couldn't parse -absfilteropt argument.", BAIL_PRINT);
    }
    if (strcmp(parmsPtr->modTime, DEF_MODTIME) != 0) {
	if (getindate(parmsPtr->modTime, &timeB1) != T_SUCCESS) {
	    Utils_Bailout("Need date in format: 20-Mar-1980:10:20:0",
		    BAIL_PRINT);
	}
	parmsPtr->modTimeVal = timeB1.time;
    }
    if ((parmsPtr->newvol) && (!parmsPtr->sync)) {
	Utils_Bailout("Need -sync option with -newvol option.", BAIL_PRINT);
    }
    if (parmsPtr->ackFreq < 0) {
	Utils_Bailout("Argument for -ackfreq must be positive.", BAIL_PRINT);
    }

    myUid = geteuid();
    myName = Utils_GetLoginByUid(myUid);
    myGid = getegid();
    myGroup = Utils_GetGroupByGid(myGid);
    rootUid = Utils_GetUidByLogin(ROOT_LOGIN);
    if ((parmsPtr->local) && (myUid != rootUid)) {
	sprintf(printBuf, "Must be %s to use -local option\n", ROOT_LOGIN);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    MEM_FREE("CheckOptions", myName);

}



/*
 *----------------------------------------------------------------------
 *
 * GetFileAbstract --
 *
 *	Invoke child program to generate abstract.
 *
 * Results:
 *      static character string.
 *
 * Side effects:
 *	Spawns child and reads data from child's stdout.
 *
 *----------------------------------------------------------------------
 */

static char *
GetFileAbstract(prog, progOptions, fileName)
    char *prog;
    char **progOptions;
    char *fileName;
{
    static char buf[16*1024]; /* arbitrary limit */
    int pipeStreams[20];
    int val;
    int total = 0;
    int remaining = sizeof(buf)-1;
    char *ptr = buf;

    *buf = '\0';

    if (pipe(pipeStreams) < 0) {
	return buf;
    } 

    if ((val=fork()) < 0) {
	return buf;
    } else if (val == 0) {
	MEM_CONTROL(0, NULL, 0, 0);
	close(pipeStreams[0]);
	close(1);
	dup(pipeStreams[1]);
	progOptions[1] = fileName;
	execvp(prog, progOptions);
	fprintf(stderr,"child: exec returned\n");
	_exit(-1);
    } else {
	close(pipeStreams[1]);
	while ((remaining > 0) &&
	       (val=read(pipeStreams[0], ptr, remaining)) > 0) {
	    total += val;
	    remaining -= val;
	    ptr += val;
	}
	close(pipeStreams[0]);
	*(buf+total) = '\0';
    }

    return buf;
}



/*
 *----------------------------------------------------------------------
 *
 * ParseProgOptions --
 *
 *	Break up user's callback program options.
 *
 * Results:
 *      static array of part ptrs. 
 *
 * Side effects:
 *	Directly munges the options argument
 *
 *----------------------------------------------------------------------
 */

static char **
ParseProgOptions(prog, options)
    char *prog;               /* name of program */
    char *options;            /* option string */
{
    static char *progOptions[100];
    static char matchGroup[]  = "\"'`";
    char *matchPtr;
    int count = 2;
    char *ptr = options;

    progOptions[0] = prog;
/*  progOptions[1] = will be the file jput needs an abstract for */

    while (1) {
	while ((*ptr) && (isspace(*ptr))) {
	    ptr++;
	}
	if (!*ptr) {
	    break;
	}
	if (count >= 100) {
	    Utils_Bailout("Too complex: -absfilteropt argument.", BAIL_PRINT);
	}
	progOptions[count++] = ptr;
	if ((matchPtr=STRCHR(matchGroup, *ptr++)) == NULL) {
	    while ((*ptr) && (!isspace(*ptr))) {
		ptr++;
	    }
	} else {
	    while ((*ptr) && (*ptr != *matchPtr)) {
		ptr++;
	    }
	    if (!*ptr++) {
		Utils_Bailout("Mismatched group in -absfilteropt argument.",
			BAIL_PRINT);
	    }
	}
	if (!*ptr) {
	    break;
	}
	*ptr++ = '\0';
    }

    progOptions[count] = NULL;

    return progOptions;
}

