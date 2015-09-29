/* 
 * jls.c --
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
 *      "Say the name."
 *      -- Boris Badinov (Rocky & Bullwinkle show)
 *
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jls.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"
#include "jlsInt.h"

int syserr;

static void  CheckOptions     _ARGS_ ((Parms *parmsPtr));
static void  SendStrings      _ARGS_ ((int argc, char **argv, int sock));
static void  ReadAndShowFiles _ARGS_ ((int sock, Parms *parmsPtr));
static char *CvtModeToString  _ARGS_ ((int mode));
static void  FormatRegular    _ARGS_ ((T_FileStat *statInfoPtr,
				       Parms *parmsPtr,
				       char *workingDir,
				       int workingDirLen));
static void  FormatRaw        _ARGS_ ((T_FileStat *statInfoPtr));

static char printBuf[T_MAXSTRINGLEN];
static FILE *memDbg = NULL;

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
    DEF_SYNC,
    DEF_MAIL,
    DEF_RECURSE,
    DEF_MODDATE,
    DEF_ALL,
    DEF_FIRSTVER,
    DEF_LASTVER,
    DEF_FIRSTDATE,
    DEF_LASTDATE,
    DEF_LPRINTFMT,
    DEF_IPRINTFMT,
    DEF_UPRINTFMT,
    DEF_APRINTFMT,
    DEF_SPRINTFMT,
    DEF_GPRINTFMT,
    DEF_DIRONLY,
    DEF_RAW
};

Option optionArray[] = {
    {OPT_STRING, "server", (char *)&parms.server, "Server host"},
    {OPT_INT, "port", (char *)&parms.port, "Port of server"},
    {OPT_STRING, "arch", (char *)&parms.arch, "Logical archive name"},
    {OPT_STRING, "range", (char *)&parms.range, "Date range"},
    {OPT_STRING, "asof", (char *)&parms.asof, "Date specification"},
    {OPT_STRING, "since", (char *)&parms.since, "Date specification"},
    {OPT_STRING, "abs", (char *)&parms.abs, "Abstract regular expression"},
    {OPT_STRING, "owner", (char *)&parms.owner, "Userid of owner "},
    {OPT_STRING, "group", (char *)&parms.group, "Group name"},
    {OPT_TRUE, "sync", (char *)&parms.sync, "Sync data to tape"},
    {OPT_STRING, "mail", (char *)&parms.mail, "Mail address"},
    {OPT_CONSTANT(INT_MAX), "R", (char *)&parms.recurse, "List directories recursively"},
    {OPT_TRUE, "moddate", (char *)&parms.modDate, "Use last-modification date, not archive date"},
    {OPT_INT, "first", (char *)&parms.firstVer, "Item number. See man page."},
    {OPT_INT, "last", (char *)&parms.lastVer, "Item number. See man page."},
    {OPT_TRUE, "all", (char *)&parms.all, "synonym for -first 1 -last -1"},
    {OPT_TRUE, "l", (char *)&parms.lPrintFmt, "Show data in long format. See below."},
    {OPT_TRUE, "i", (char *)&parms.iPrintFmt, "Show buffer Id"},
    {OPT_TRUE, "u", (char *)&parms.uPrintFmt, "Show archive date instead of last-mode date"},
    {OPT_TRUE, "a", (char *)&parms.aPrintFmt, "Show abstract"},
    {OPT_TRUE, "s", (char *)&parms.sPrintFmt, "Show size in 1K blocks"},
    {OPT_TRUE, "g", (char *)&parms.gPrintFmt, "Show group name"},
    {OPT_TRUE, "d", (char *)&parms.dirOnly, "Show directory only"},
    {OPT_TRUE, "raw", (char *)&parms.raw, "Raw output"}
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

/*    memDbg = fopen("jls.mem", "w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckOptions(&parms);

    sock = Sock_SetupSocket(parms.port, parms.server, 1);

    Sock_SendReqHdr(sock, T_CMDLS, 0, parms.mail,  parms.arch, 0);

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

    /* put out regular expressions */
    if (argc > 1) {
	SendStrings(--argc, &argv[1], sock);
    } else {
	SendStrings(1, &defaultList, sock);
    }

    ReadAndShowFiles(sock, &parms);

    Sock_ReadRespHdr(sock, &resp);
    if (resp.status != T_SUCCESS) {
	fprintf(stdout,"%s", Utils_MakeErrorMsg(resp.status, resp.errno));
    }

    close(sock);
    MEM_REPORT("jls", ALLROUTINES, SORTBYREQ);

    return(0);

}


/*
 *----------------------------------------------------------------------
 *
 * SendStrings --
 *
 *	Send a list of regular expressions
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Shoves data out the socket.
 *
 *----------------------------------------------------------------------
 */

static void
SendStrings(nameCnt, nameList, sock)
    int nameCnt;              /* # strings */
    char **nameList;          /* array of string ptrs */
    int sock;                 /* destination socket */
{
    char *fileList;
    int packedLen;
    char fmt[MAXARG];
    char *fmtPtr;
    int i;
    int base = 0;
    int quantity;

    Sock_WriteInteger(sock, nameCnt);

    while (base < nameCnt) {
	quantity = (nameCnt-base > MAXARG) ? MAXARG : nameCnt-base;
	for (i=base, fmtPtr=fmt; i<base+quantity; i++) {
	    nameList[i] = Utils_MakeFullPath(nameList[i]);
	    *fmtPtr++ = 'C';
	}
	*fmtPtr = '\0';
	fileList = Sock_PackData(fmt, nameList+base, &packedLen);
	if (Sock_WriteNBytes(sock, fileList, packedLen) != packedLen) {
	    fprintf(stderr,"Failed to write strings to socket: errno %d\n",
		    syserr);
	}
	MEM_FREE("main", fileList);
	base += quantity;
    }

}



/*
 *----------------------------------------------------------------------
 *
 * ReadAndShowFiles --
 *
 *	Retrieve and display all the filenames
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
ReadAndShowFiles(sock, parmsPtr)
    int sock;                 /* incoming socket */
    Parms *parmsPtr;          /* parameter block */
{
    char workingDir[T_MAXPATHLEN];
    int workingDirLen;
    static T_FileStat statInfo;

    Utils_GetWorkingDir(workingDir, &workingDirLen);

    while (Sock_ReadFileStat(sock, &statInfo, 1) == T_SUCCESS) {
	if (!*statInfo.fileName) {
	    return;
	}
	if (parmsPtr->raw) {
	    FormatRaw(&statInfo);
	} else {
	    FormatRegular(&statInfo, parmsPtr, workingDir, workingDirLen);
	}
	Utils_FreeFileStat(&statInfo, 0);
    }

    printf("Got error trying to read T_FileStat\n");

}



/*
 *----------------------------------------------------------------------
 *
 * FormatRegular --
 *
 *	Format output according to command line flags.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Dumps file metadata to stdout
 *
 *----------------------------------------------------------------------
 */

static void
FormatRegular(statInfoPtr, parmsPtr, workingDir, workingDirLen)
    T_FileStat *statInfoPtr;
    Parms *parmsPtr;
    char *workingDir;
    int workingDirLen;
{
    char datePtr[26];
    char *filePtr;
    static char cwd[2] = ".";

    if (parmsPtr->iPrintFmt) {
	/* filemark is the id for the header. data is in filemark+1 */
	fprintf(stdout,"%6d %6d %6d ",
		statInfoPtr->tBufId, statInfoPtr->volId,
		statInfoPtr->filemark+1);
    }
    if (parmsPtr->sPrintFmt) {
	fprintf(stdout,"%6d ", (statInfoPtr->size+1024)>>10);
    }
    if (strncmp(workingDir, statInfoPtr->fileName, workingDirLen) == 0){
	if (workingDirLen == 1) {
	    filePtr = statInfoPtr->fileName + 1;
	} else if (strlen(statInfoPtr->fileName) > workingDirLen) {
	    filePtr = statInfoPtr->fileName + workingDirLen + 1;
	} else {
	    filePtr = cwd;
	}
    } else {
	filePtr = statInfoPtr->fileName;
    }
    if (parmsPtr->lPrintFmt) {
	if (parmsPtr->uPrintFmt) {
	    strcpy(datePtr,Time_CvtToString(&statInfoPtr->vtime));
	} else {
	    strcpy(datePtr,Time_CvtToString(&statInfoPtr->mtime));
	}
	fprintf(stdout,"%s %8s",
		CvtModeToString(statInfoPtr->mode), statInfoPtr->uname);
	if (parmsPtr->gPrintFmt) {
	    fprintf(stdout," %8s", statInfoPtr->gname);
	}
	fprintf(stdout," %8d %s %s", statInfoPtr->size, datePtr, filePtr);
	if (S_ISALNK(statInfoPtr->mode)) {
	    fprintf(stdout," -> %s", statInfoPtr->linkName);
	}
    } else {
	fprintf(stdout,"%s", filePtr);
    }
    if (parmsPtr->aPrintFmt) {
	fprintf(stdout, "\n\t%s\n", statInfoPtr->abstract);
    } else {
	fputc('\n', stdout);
    }

}



/*
 *----------------------------------------------------------------------
 *
 * FormatRaw --
 *
 *	Dump the T_FileStat contents 
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
FormatRaw(statInfoPtr)
    T_FileStat *statInfoPtr;
{
    static char metaChars[] = "\"";
    char dateSpace[26];

    strcpy(dateSpace,Time_CvtToString(&statInfoPtr->mtime));
    *(dateSpace+3) = '\0';
    *(dateSpace+6) = '\0';

    printf("%s|%s|%s|%d|%s|%s|%s|%s|%s|%s|%s\n",
	    CvtModeToString(statInfoPtr->mode),
	    Str_Quote(statInfoPtr->uname, metaChars),
	    Str_Quote(statInfoPtr->gname, metaChars),
	    statInfoPtr->size,
	    dateSpace,
	    dateSpace+4,
	    dateSpace+7,
	    Str_Quote(statInfoPtr->fileName, metaChars),
	    (*statInfoPtr->linkName) ? "->" : "", 
	    Str_Quote(statInfoPtr->linkName, metaChars),
	    statInfoPtr->abstract);
/*
    printf("%d,%d,%d,%d,%d,%s,%d,%d\n",
	    Str_Quote(statInfoPtr->gname, metaChars),
	    Str_Quote(statInfoPtr->abstract, metaChars),
	    Time_CvtToString(&statInfoPtr->atime),
	    statInfoPtr->filemark,
	    statInfoPtr->tBufId,
	    statInfoPtr->mode,
	    statInfoPtr->uid,
	    statInfoPtr->gid,
	    Time_CvtToString(&statInfoPtr->mtime),
	    statInfoPtr->volId;
	    statInfoPtr->rdev);
*/
}



/*
 *----------------------------------------------------------------------
 *
 * CvtModeToString --
 *
 *	Make file's mode value printable
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static char *
CvtModeToString(mode)
    int mode;
{
    int type;
    static char modeStr[11];
    static char *modeArray[] = {
	"---",	"--x",	"-w-",	"-wx",
	"r--",	"r-x",	"rw-",	"rwx"
    };

    type = mode & S_IFMT;
    if (type == S_IFDIR) {
	modeStr[0] = 'd';
    } else if (type == S_IFCHR) {
	modeStr[0] = 'c';
    } else if (type == S_IFLNK) {
	modeStr[0] = 'l';
    } else if (type == S_IFBLK) {
	modeStr[0] = 'b';
    } else if (type == S_IFRLNK) {
	modeStr[0] = 'r';
    } else {
	modeStr[0] = '-';
    }

    strcpy(modeStr+1, modeArray[(mode>>6) & 7]);
    strcpy(modeStr+4, modeArray[(mode>>3) & 7]);
    strcpy(modeStr+7, modeArray[(mode) & 7]);

    return modeStr;
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
    struct timeb timeB1;
    struct timeb timeB2;
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
	if (getindate(parmsPtr->since, &timeB1) != T_SUCCESS) {
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
    if (parmsPtr->dirOnly) {
	if (parmsPtr->recurse > 1) {
	    Utils_Bailout("The -recurse and -d options are mutually exclusive.\n",
		    BAIL_PRINT);
	}
	parmsPtr->recurse = 0;
    }
    if ((parmsPtr->raw) &&
	(parmsPtr->lPrintFmt | parmsPtr->iPrintFmt |
	 parmsPtr->uPrintFmt | parmsPtr->aPrintFmt)) {
	Utils_Bailout("The -raw option doesn't mix with -a -l -i -u.\n",
		      BAIL_PRINT);
    }
}
