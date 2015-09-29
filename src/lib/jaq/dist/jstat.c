/* 
 * jstat.c --
 *
 *	Perform a status request on Jaquith archive system.
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
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/jstat.c,v 1.0 91/01/07 18:02:37 mottsmth Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "jaquith.h"
#include "option.h"
#include "jstatInt.h"

int syserr;

static char printBuf[T_MAXSTRINGLEN];
static FILE *memDbg = NULL;

static int headers = 0;

Parms parms = {
    "",
    -1,
    "",
    DEF_MAIL,
    DEF_DEVICES,
    DEF_QUEUES,
    DEF_ARCHLIST
};

Option optionArray[] = {
    {OPT_STRING, "server", (char *)&parms.server, "Server hostname"},
    {OPT_INT, "port", (char *)&parms.port, "Port of server"},
    {OPT_STRING, "arch", (char *)&parms.arch, "Name of archive for -devices -queues option"},
    {OPT_STRING, "mail", (char *)&parms.mail, "Mail address"},
    {OPT_TRUE, "devices", (char *)&parms.devices, "Report device info"},
    {OPT_TRUE, "queues", (char *)&parms.queues, "Report queue info"},
    {OPT_TRUE, "archlist", (char *)&parms.archList, "Report logical archives"}
};
int numOptions = sizeof(optionArray) / sizeof(Option);

static void CheckOptions       _ARGS_ ((Parms *parmsPtr));
static void ProcessDevices     _ARGS_ ((int sock, int displayFlag));
static void ProcessQueues      _ARGS_ ((int sock, int displayFlag));
static void ProcessArchList    _ARGS_ ((int sock, int displayFlag));


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main driver for jstat program.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Sends status requests across socket to Jaquith server.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
int argc;
char *argv[];
{
    int sock;

/*    memDbg = fopen("jstat.mem", "w"); */
    MEM_CONTROL(8192, memDbg, TRACEMEM+TRACECALLS, 4096);

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    CheckOptions(&parms);

    sock = Sock_SetupSocket(parms.port, parms.server, 1);

    Sock_SendReqHdr(sock, T_CMDSTAT, 0, parms.mail,  parms.arch, 0);

    ProcessDevices(sock, parms.devices);
    ProcessQueues(sock, parms.queues);
    ProcessArchList(sock, parms.archList);

    close(sock);

    MEM_REPORT("jstat", ALLROUTINES, SORTBYREQ);

    return 0;
}



/*
 *----------------------------------------------------------------------
 *
 * ProcessDevices --
 *
 *	Read and possibly report device status
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Reads from socket. Will print to stdout if display != 0
 *
 *----------------------------------------------------------------------
 */

static void
ProcessDevices(sock, display)
    int sock;                 /* server socket connection */
    int display;              /* display flag. 1==print it */
{
    int count;
    char devName[T_MAXSTRINGLEN];
    char *devPtr = devName;
    char hostName[T_MAXSTRINGLEN];
    char *hostPtr = hostName;
    char userName[T_MAXSTRINGLEN];
    char *userPtr = userName;
    int volId;
    char volIdStr[T_MAXSTRINGLEN];
    int i;

    if (Sock_ReadInteger(sock, &count) != T_SUCCESS) {
	Utils_Bailout("Server died during transmission\n", BAIL_PRINT);
    }

    if (display && headers) {
	if (count == 0) {
	    printf("\nStrange. Server is not managing any devices!\n");
	} else if (count == -1) {
	    printf("\nCouldn't get device status. (Jmgr dead?)\n");
	} else {
	    printf("\nDevice Status:\n");
	}

    }

    for (i=0; i<count; i++) {
	if ((Sock_ReadString(sock, &devPtr, 0) != T_SUCCESS) ||
	    (Sock_ReadInteger(sock, &volId) != T_SUCCESS) ||
	    (Sock_ReadString(sock, &hostPtr, 0) != T_SUCCESS) ||
	    (Sock_ReadString(sock, &userPtr, 0) != T_SUCCESS)) {
	    Utils_Bailout("Server died during transmission\n", BAIL_PRINT);
	}
	if (display) {
	    if (volId == -1) {
		strcpy(volIdStr, "No volume mounted.");
	    } else if (*userName == '\0')  {
		sprintf(volIdStr, "volume %d", volId);
	    } else {
		sprintf(volIdStr, "volume %d\t%s@%s", volId, userName, hostName);
	    }
	    printf("%s\t%s\n", devName, volIdStr);
	}
    }
}



/*
 *----------------------------------------------------------------------
 *
 * ProcessQueues --
 *
 *	Read and possibly report queue status
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Reads from socket. Will print to stdout if display != 0
 *
 *----------------------------------------------------------------------
 */

static void
ProcessQueues(sock, display)
    int sock;                 /* server socket connection */
    int display;              /* display flag. 1==print it */
{
    int count;
    char hostName[80];
    char *hostPtr = hostName;
    char userName[80];
    char *userPtr = userName;
    int volId;
    int i;

    if (Sock_ReadInteger(sock, &count) != T_SUCCESS) {
	Utils_Bailout("Server died during transmission\n", BAIL_PRINT);
    }

    if (display && headers) {
	if (count == 0) {
	    printf("\nNo entries in device queue.\n");
	} else if (count == -1) {
	    printf("\nCouldn't get queue status. (Jmgr dead?)\n");
	} else {
	    printf("\nDevice Queue:\n");
	}
    }

    for (i=0; i<count; i++) {
	if ((Sock_ReadInteger(sock, &volId) != T_SUCCESS) ||
	    (Sock_ReadString(sock, &hostPtr, 0) != T_SUCCESS)||
	    (Sock_ReadString(sock, &userPtr, 0) != T_SUCCESS)) {
	    Utils_Bailout("Server died during transmission\n", BAIL_PRINT);
	}
	if (display) {
	    printf("%s@%s\tvol %d\n", userName, hostName, volId);
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ProcessArchList --
 *
 *	Read and possibly report archive list.
 *
 * Results:
 *	none.
 *
 * Side effects:
 *	Reads from socket. Will print to stdout if display != 0
 *
 *----------------------------------------------------------------------
 */

static void
ProcessArchList(sock, display)
    int sock;                 /* server socket connection */
    int display;              /* display flag. 1==print it */
{
    char archName[80];
    char *archPtr = archName;

    if (display) {
	if (headers) {
	    printf("\nLogical archives:\n");
	}
	while (Sock_ReadString(sock, &archPtr, 0) == T_SUCCESS) {
	    if (*archName) {    
		printf("%s\n", archName);
	    } else {
		break;
	    }
	}
    }
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
    Parms *parmsPtr;          /* parameter list */
{
    char *envPtr;

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
    if ((!parmsPtr->mail) && 
	(Utils_CheckName(parmsPtr->mail, 1) == T_FAILURE)) {
	sprintf(printBuf, "Bad mail address: '%s'\n", parmsPtr->mail);
	Utils_Bailout(printBuf, BAIL_PRINT);
    }
    if ((!parmsPtr->devices) && (!parmsPtr->queues) && (!parmsPtr->archList)) {
	Utils_Bailout("Need -devices, -queues, or -archlist option\n", 
		BAIL_PRINT);
    }
}

