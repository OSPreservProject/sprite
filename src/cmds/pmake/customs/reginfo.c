/*-
 * reginfo.c --
 *	Find out who's registered.
 *
 * Copyright (c) 1988, 1989 by the Regents of the University of California
 * Copyright (c) 1988, 1989 by Adam de Boor
 * Copyright (c) 1989 by Berkeley Softworks
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any non-commercial purpose
 * and without fee is hereby granted, provided that the above copyright
 * notice appears in all copies.  The University of California,
 * Berkeley Softworks and Adam de Boor make no representations about
 * the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 */
#ifndef lint
static char *rcsid =
"$Id: reginfo.c,v 1.8 89/11/14 13:46:15 adam Exp $ SPRITE (Berkeley)";
#endif lint

#include    "customs.h"
#include    <stdio.h>
#include    <netdb.h>
#include    <sys/time.h>

typedef struct {
    char    	  *name;
    long    	  avail;
    long    	  rating;
    long    	  arch;
    long    	  numClients;
    long    	  *clients;
} Host;

main(argc, argv)
    int	    argc;
    char    **argv;
{
    char    	  	infoBuf[MAX_INFO_SIZE];
    Host    	  	*allHosts;
    int	    	  	i, j;
    int	    	  	numHosts;
    char    	  	*cp;
    struct sockaddr_in	sin;
    struct hostent	*he;

    if (Customs_Master(&sin) != RPC_SUCCESS) {
	Customs_PError("Customs_Master");
	printf("Couldn't find master\n");
	exit(1);
    }
    he = gethostbyaddr(&sin.sin_addr, sizeof(sin.sin_addr), AF_INET);
    printf ("Master Agent at ");
    if (he == (struct hostent *)NULL) {
	printf("%s\n", InetNtoA(sin.sin_addr));
    } else {
	printf ("%s\n", he->h_name);
    }
    
    if (Customs_Info(&sin, infoBuf) != RPC_SUCCESS) {
	Customs_PError("Customs_Info");
	printf("Couldn't read registration info\n");
    }
    cp = infoBuf;
    numHosts = *(int *)cp;
    cp += sizeof(int);
    allHosts = (Host *)malloc(numHosts * sizeof(Host));
    for (i = 0; i < numHosts; i++) {
	allHosts[i].name = cp;
	cp += strlen(cp) + 1;
	cp = Customs_Align(cp, char *);
	allHosts[i].avail = *(long *)cp;
	cp += sizeof(long);
	allHosts[i].rating = *(long *)cp;
	cp += sizeof(long);
	allHosts[i].arch = *(long *)cp;
	cp += sizeof(long);
	allHosts[i].numClients = *(long *)cp;
	cp += sizeof(long);
	allHosts[i].clients = (long *)cp;
	cp += allHosts[i].numClients * sizeof(long);
    }
    
    for (i = 0; i < numHosts; i++) {
	printf ("%s (arch = %d): ", allHosts[i].name, allHosts[i].arch);
	if (allHosts[i].avail & AVAIL_DOWN) {
	    printf ("host down\n");
	} else if (allHosts[i].avail & AVAIL_IDLE) {
	    printf ("not idle\n");
	} else if (allHosts[i].avail & AVAIL_SWAP) {
	    printf ("not enough swap space\n");
	} else if (allHosts[i].avail & AVAIL_LOAD) {
	    printf ("load average too high\n");
	} else if (allHosts[i].avail & AVAIL_IMPORTS) {
	    printf ("too many imported jobs\n");
	} else {
	    printf ("available (index = %d)\n", allHosts[i].rating);
	}
	printf ("\tclients: ");
	if (allHosts[i].numClients != 0) {
	    for (j = 0; j < allHosts[i].numClients; j++) {
		printf ("%s ", allHosts[allHosts[i].clients[j]].name);
	    }
	    putchar('\n');
	} else {
	    printf ("ALL\n");
	}
    }
    printf ("Last allocated: %s\n", allHosts[*(long *)cp].name);
}
