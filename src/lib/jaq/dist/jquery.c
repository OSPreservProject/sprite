/* 
 * jquery.c --
 *
 *	Perform status query on Jaquith archive system.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jquery.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"

static int    ProcessDevices  _ARGS_ ((int sock));
static int    ProcessArchList _ARGS_ ((int sock));

static char printBuf[T_MAXSTRINGLEN];

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

typedef struct parmTag {      /* Only really need a little of this ... */
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
    {OPT_INT, "disklow", (char *)&parms.diskLow, "Low usable disk (%)"},
    {OPT_INT, "diskhigh", (char *)&parms.diskHigh, "High usable disk (%)"},
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
 * jquery --
 *
 *	Main driver for status operations
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	none.
 *
 * Note: jquery is invoked from the Jaquith switchboard process,
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
    int retCode;

/*    memDbg = fopen("jquery.mem","w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    sock = parms.sock;
    jDebug = parms.debug;

    ProcessDevices(sock);
    ProcessArchList(sock);

    MEM_REPORT("jquery", ALLROUTINES, SORTBYOWNER);

    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcessDevices --
 *
 *	Call jukebox manager to get device status
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
ProcessDevices(sock) 
    int sock;                 /* client socket */
{
    int status = -1;
    int mgrSock;
    int retCode = T_SUCCESS;
    int count = -1;
    int i;
    int volId;
    char devName[80];
    char *devPtr = devName;
    char hostName[80];
    char *hostPtr = hostName;
    char userName[80];
    char *userPtr = userName;
    char *archPath;
    char *logPath;
    ArchConfig archConfig;

    archPath = Str_Cat(3, parms.root, "/", parms.arch);
    logPath = Str_Cat(3, archPath, "/", ARCHLOGFILE);

    if (Admin_ReadArchConfig(archPath, &archConfig) != T_SUCCESS) {
	sprintf(printBuf,"Admin_ReadArchConfig failed: errno %d\n", syserr);
	Log_AtomicEvent("jupdate", printBuf, logPath);
	Sock_SendRespHdr(sock, T_ADMFAILED, syserr);
	return -1;
    }


    if ((mgrSock=Sock_SetupSocket(archConfig.mgrPort,
				  archConfig.mgrServer, 0))
	!= T_FAILURE) {
	retCode = Sock_WriteString(mgrSock, parms.userName, 0);
	retCode += Sock_WriteInteger(mgrSock, S_CMDSTAT);
	retCode += Sock_WriteInteger(mgrSock, 0);
	retCode += Sock_ReadInteger(mgrSock, &status);
	if (retCode == T_SUCCESS) {
	    Sock_ReadInteger(mgrSock, &count);
	}
    }    

    Sock_WriteInteger(sock, count);

    for (i=0; i<count; i++) {
	if ((Sock_ReadString(mgrSock, &devPtr, 0) != T_SUCCESS) ||
	    (Sock_ReadInteger(mgrSock, &volId) != T_SUCCESS) ||
	    (Sock_ReadString(mgrSock, &hostPtr, 0) != T_SUCCESS) ||
	    (Sock_ReadString(mgrSock, &userPtr, 0) != T_SUCCESS)) {
	    return(T_FAILURE);
	}
	if ((Sock_WriteString(sock, devPtr, 0) != T_SUCCESS) ||
	    (Sock_WriteInteger(sock, volId) != T_SUCCESS) ||
	    (Sock_WriteString(sock, hostPtr, 0) != T_SUCCESS) ||
	    (Sock_WriteString(sock, userPtr, 0) != T_SUCCESS)) {
	    return(T_FAILURE);
	}
    }

    if (count > -1) {
	count = -1;
	Sock_ReadInteger(mgrSock, &count);
    }
    Sock_WriteInteger(sock, count);

    for (i=0; i<count; i++) {
	if ((Sock_ReadInteger(mgrSock, &volId) != T_SUCCESS) ||
	    (Sock_ReadString(mgrSock, &hostPtr, 0) != T_SUCCESS)||
	    (Sock_ReadString(mgrSock, &userPtr, 0) != T_SUCCESS)) {
	    return T_FAILURE;
	}
	if ((Sock_WriteInteger(sock, volId) != T_SUCCESS) ||
	    (Sock_WriteString(sock, hostPtr, 0) != T_SUCCESS)||
	    (Sock_WriteString(sock, userPtr, 0) != T_SUCCESS)) {
	    return T_FAILURE;
	}
    }

    close(mgrSock);

    return T_SUCCESS;
}


/*
 *----------------------------------------------------------------------
 *
 * ProcessArchList --
 *
 *	Return list of logical archives to caller
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
ProcessArchList(sock) 
    int sock;                 /* client socket */
{
    DIR *dirStream;
    DirObject *entryPtr;
    char *namePtr;

    if ((dirStream=opendir(parms.root)) != (DIR *)NULL) {
	while ((entryPtr=readdir(dirStream)) != (DirObject *)NULL) {
	    if (Str_Match(entryPtr->d_name, "*.arch")) {
		*(entryPtr->d_name+strlen(entryPtr->d_name)-5) = '\0';
		Sock_WriteString(sock, entryPtr->d_name, 0);
	    }
	}
	closedir(dirStream);
    }

    Sock_WriteString(sock, "", 0);

    return T_SUCCESS;

}

