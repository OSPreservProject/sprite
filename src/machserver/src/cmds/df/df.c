/* 
 * prefix.c --
 *
 *	Program to manipulate the prefix table.
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
static char rcsid[] = "$Header: /sprite/src/cmds/df/RCS/df.c,v 1.5 91/08/19 13:03:36 mendel Exp $ SPRITE (Berkeley)";
#endif not lint

#include <errno.h>
#include <fs.h>
#include <fsCmd.h>
#include <host.h>
#include <stdio.h>
#include <string.h>
#include <status.h>
#include <sysStats.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Main program for "df":  print disk free space info.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

main(argc, argv)
    int argc;
    char *argv[];
{
    register ReturnStatus status = SUCCESS;	/* status of system calls */
#define MAX_PREFIXES 100
    Fs_Prefix prefixes[MAX_PREFIXES];
    struct stat buf;
    int i, numDomains, maxLength;

    /*
     * For each argument, stat the argument in order to make sure that
     * there's a prefix table entry loaded for it.  If the file can't
     * be found, then mark the entry so we don't try to print it later.
     */

    for (i = 1; i < argc; i++) {
	if (stat(argv[i], &buf) < 0) {
	    fprintf(stderr, "%s couldn't find \"%s\": %s.\n", argv[0],
		    argv[i], strerror(errno));
	}
    }

    /*
     * Collect information for all known domains.
     */

    maxLength = 0;
    for (numDomains = 0; ; numDomains++) {
	bzero((char *) &prefixes[numDomains], sizeof(Fs_Prefix));
	status = Sys_Stats(SYS_FS_PREFIX_STATS, numDomains,
		(Address) &prefixes[numDomains]);
	if (status != SUCCESS) {
	    break;
	}
	i = strlen(prefixes[numDomains].prefix);
	if (i > maxLength) {
	    maxLength = i;
	}
    }

    /*
     * If no args were given, then print all domains.  Otherwise just
     * find the ones matching the names given.
     */

    if (argc == 1) {
	for (i = numDomains-1; i >= 0; i--) {
	    PrintDiskInfo(&prefixes[i], maxLength);
	}
    } else {
	for (i = 1; i < argc; i++) {
	    int j;

	    /*
	     * For each of the names given, find the domain that
	     * contains the file, by comparing server and domain ids
	     * between the file and the prefix table entries.
	     */

	    if (stat(argv[i], &buf) < 0) {
		continue;
	    }
	    for (j = numDomains-1; j >= 0; j--) {
		if ((prefixes[j].serverID == buf.st_serverID)
			&& (prefixes[j].domain == ((int) buf.st_dev))) {
		    PrintDiskInfo(&prefixes[j], maxLength);
		    break;
		}
	    }
	}
    }
    exit(0);
}

/*
 *----------------------------------------------------------------------
 *
 * PrintDiskInfo --
 *
 *	Given an Fs_Prefix entry, print disk utilization info for
 *	the prefix.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stuff gets printed on stdout.
 *
 *----------------------------------------------------------------------
 */

PrintDiskInfo(prefixPtr, nameSpace)
    register Fs_Prefix *prefixPtr;	/* Information about prefix. */
    int nameSpace;			/* Leave at least this much space
					 * in the "prefix name" column. */
{
    static char serverName[32];
    static int  prevServerID = -1;
    static int  firstTime = 1;
    int		inUse;
    int		free;
    int		avail;
    Host_Entry  *host;

    if (firstTime) {
	printf("%-*s  %-10s %10s %10s %10s %9s\n", nameSpace,
		"Prefix", "Server", "KBytes", "Used", "Avail", "% Used");
	firstTime = 0;
    }
    printf("%-*s", nameSpace, prefixPtr->prefix);

    if (prefixPtr->serverID > 0) {

	/*
	 * If the server ID is the same as the previous entry's ID, then
	 * we can reuse serverName and save a call to Host_ByID.
	 */
	if (prefixPtr->serverID == prevServerID) {
	    printf("  %-10s", serverName);
	} else {
	    host = Host_ByID(prefixPtr->serverID);
	    if (host != (Host_Entry *)NULL) {
		register int charCnt;
		for (charCnt = 0 ; charCnt < sizeof(serverName) ; charCnt++) {
		    if (host->name[charCnt] == '.' ||
			host->name[charCnt] == '\0') {
			serverName[charCnt] = '\0';
			break;
		    } else {
			serverName[charCnt] = host->name[charCnt];
		    }
		}
		serverName[sizeof(serverName)-1] = '\0';
		printf("  %-10s", serverName);
		prevServerID = prefixPtr->serverID;
	    } else {
		printf(" (%d)", prefixPtr->serverID);
	    }
	}
    } else {
	printf("  %-10s", "(none)");
    }
    if (prefixPtr->domainInfo.maxKbytes <= 0) {
	printf("\n");
	return;
    }

    free = prefixPtr->domainInfo.freeKbytes - 
				(0.1 * prefixPtr->domainInfo.maxKbytes);
    avail = 0.9 * prefixPtr->domainInfo.maxKbytes;
    inUse = avail - free;

    printf(" %10d %10d %10d %7d%%\n", 
	prefixPtr->domainInfo.maxKbytes, inUse,	free >= 0 ? free : 0,
	(int) (100.0 * (inUse / (float) avail)));
}
