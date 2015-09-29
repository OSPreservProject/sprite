/*
 * NFS pseudo-filesystem server.
 *
 * This program is for a server process that acts as a gateway between
 * a Sprite pseudo-filesystem and a remote NFS server.  This allows a
 * NFS filesystem to be transparenty integrated into the Sprite distributed
 * filesystem.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/nfsmount/RCS/main.c,v 1.11 91/10/20 12:37:45 mottsmth Exp $ SPRITE (Berkeley)";
#endif not lint

#include "stdio.h"
#include "option.h"
#include "errno.h"
#include "nfs.h"
#include "strings.h"
#include <syslog.h>

char myhostname[1024];

/*
 * Command line options.
 */
char *prefix = "/nfs";
char *host = "ginger.Berkeley.EDU";
char *rfs = "/sprite";
static int trace;
int nfs_PdevWriteBehind = 1;

Option optionArray[] = {
	{OPT_DOC, "", (Address)NIL, "nfssrv [-t] rhost:/path /prefix"},
	{OPT_DOC, "", (Address)NIL, "(or use -p, -h, -r flags)"},
	{OPT_STRING, "p", (Address)&prefix,
		"Sprite prefix"},
	{OPT_STRING, "h", (Address)&host,
		"NFS host"},
	{OPT_STRING, "r", (Address)&rfs,
		"Remote filesystem name"},
	{OPT_FALSE, "sync", (Address)&nfs_PdevWriteBehind,
		"Disable write-behind"},
	{OPT_TRUE, "t", (Address)&trace,
		"Turn on tracing"},
	{OPT_GENFUNC, "m", (Address)NfsRecordMountPointProc,
		"NFS sub-mount point"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

struct timeval nfsTimeout = { 25, 0 };

void DoOpt();


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for NFS pseudo-filesystem server.  First the NFS
 *	system is mounted, and then the pseudo-filesystem is established.
 *	Lastly we drop into an Fs_Dispatch loop to handle pfs requests
 *	comming from the Sprite kernel.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Opens the pseudo-filesystem.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char *argv[];
{
    NfsState *nfsPtr;
    attrstat nfsAttr;
    Fs_Attributes spriteAttr;
    Fs_FileID rootID;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, OPT_ALLOW_CLUSTERING);

    while (argc > 1) {
	/*
	 * Look for "host:name [prefix]"
	 */
	register char *colonPtr = index(argv[1], ':');
	if (colonPtr != (char *)0) {
	    host = argv[1];
	    *colonPtr = '\0';
	    rfs = colonPtr + 1;
	    argc--;
	    argv++;
	} else {
	    prefix = argv[1];
	    argc--;
	    argv++;
	}
    }

    gethostname(myhostname, sizeof(myhostname));

    nfsPtr = (NfsState *)malloc(sizeof(NfsState));
    nfsPtr->host = host;
    nfsPtr->prefix = prefix;
    nfsPtr->nfsName = rfs;

    nfsPtr->mountClnt = Nfs_MountInitClient(host);
    if (nfsPtr->mountClnt == (CLIENT *)NULL) {
	exit(1);
    }
    nfsPtr->nfsClnt = Nfs_InitClient(host);
    if (nfsPtr->nfsClnt == (CLIENT *)NULL) {
	exit(1);
    }

    nfsPtr->mountHandle = Nfs_Mount(nfsPtr->mountClnt, nfsPtr->nfsName);
    if (nfsPtr->mountHandle == (nfs_fh *)NULL) {
	exit(1);
    }

    /*
     * Test NFS access by getting the attributes of the root.  This is
     * needed in order to set the rootID properly so it matches any
     * future stat() calls by clients.
     */
    if (!NfsProbe(nfsPtr, trace, &nfsAttr)) {
	Nfs_Unmount(nfsPtr->mountClnt, rfs);
	exit(1);
    }
    NfsToSpriteAttr(&nfsAttr.attrstat_u.attributes, &spriteAttr);
    rootID.serverID = spriteAttr.serverID;
    rootID.type = TYPE_ROOT;
    rootID.major = spriteAttr.domain;
    rootID.minor = spriteAttr.fileNumber;
    /*
     * Set ourselves up as the server of the pseudo-file-system.  We'll
     * see requests via the call-backs in nfsNameService.
     */
    if (trace) {
	printf("RootID <%d,%d,%d,%d>\n", rootID.serverID, rootID.type,
		    rootID.major, rootID.minor);
    }
    nfsPtr->pfsToken = Pfs_Open(prefix, &rootID, &nfsNameService,
	(ClientData)nfsPtr);
    if (nfsPtr->pfsToken == (Pfs_Token)NULL)  {
	if (trace) {
            printf("%s\n", pfs_ErrorMsg);
	}
        syslog(LOG_ERR, "nfsmount: %s", pfs_ErrorMsg);
	Nfs_Unmount(nfsPtr->mountClnt, rfs);
	exit(1);
    }

    if (trace) {
	pdev_Trace = pfs_Trace = trace;
	printf("NFS (traced): ");
    } else {
	printf("NFS: ");
    }
    printf("%s => %s:%s\n", prefix, host, rfs);

    if (!trace) {
	Proc_Detach(0);
    }

    while (1) {
	Fs_Dispatch();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * BadProc --
 *
 *	Called by accident.
 * 
 * Results:
 *	None.
 *
 * Side effects:
 *	Enter the debugger.
 *
 *----------------------------------------------------------------------
 */
int
BadProc()
{
    panic("Bad callback\n");
}


