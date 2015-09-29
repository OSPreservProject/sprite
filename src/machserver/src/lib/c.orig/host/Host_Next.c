/* 
 * Host_Next.c --
 *
 *	Source code for the Host_Next library procedure.
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
static char rcsid[] = "$Header: /sprite/src/lib/c/host/RCS/Host_Next.c,v 1.7 90/01/02 17:10:46 ouster Exp $ SPRITE (Berkeley)";
#endif not lint

#include <stdio.h>
#include <sprite.h>
#include <ctype.h>
#include <host.h>
#include <hostInt.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/*
 *-----------------------------------------------------------------------
 *
 * Host_Next --
 *
 *	Read the next line from the host file and break it into the
 *	appropriate fields of the structure.
 *
 * Results:
 *	The return value is a pointer to a Host_Entry structure
 *	containing the information from the next line of the file.
 *	This is a statically-allocated structure, which will only
 *	retain its value up to the next call to this procedure.
 *	If the end of the file is reached, or an error occurs, NULL
 *	is returned.
 *
 * Side Effects:
 *	The position in the file advances.
 *
 *-----------------------------------------------------------------------
 */
Host_Entry *
Host_Next()
{
#define BUFFER_SIZE 512
#define MAX_NAMES 20
    static Host_Entry	entry;
    static char      	inputBuf[BUFFER_SIZE];
    static char *	fields[MAX_NAMES+2];
    register char *	p;
    int			numFields;
    int    	  	c;

    if (hostFile == (FILE *) NULL) {
	return ((Host_Entry *) NULL);
    } else {
	/*
	 * First skip any comment lines or blank lines.
	 */

	while (1) {
	    c = getc(hostFile);
	    if (c != '#' && c != '\n') {
		break;
	    }
	    while ((c != '\n') && (c != EOF)) {
		c = getc(hostFile);
	    } 
	}
	ungetc(c, hostFile);

	/*
	 * <spriteID>
	 */

	if (fscanf(hostFile, "%d", &entry.id) != 1) {
	    return ((Host_Entry *) NULL);
	}

	/*
	 * <netType> and <netAddr>
	 */

	if (fscanf(hostFile, "%100s", inputBuf) != 1) {
	    return (Host_Entry *) NULL;
	}
	if (strcmp(inputBuf, "ether") == 0) {
	    /*
	     * A HOST_ETHER route address is the ethernet address of the
	     * machine.
	     */
	    int byte[6];
	    entry.netType = HOST_ETHER;
	    if (fscanf(hostFile, "%2x:%2x:%2x:%2x:%2x:%2x",
		    &byte[0], &byte[1], &byte[2], &byte[3], &byte[4], &byte[5])
		    != 6) {
		return (Host_Entry *) NULL;
	    }
	    entry.netAddr.etherAddr[0] = byte[0];
	    entry.netAddr.etherAddr[1] = byte[1];
	    entry.netAddr.etherAddr[2] = byte[2];
	    entry.netAddr.etherAddr[3] = byte[3];
	    entry.netAddr.etherAddr[4] = byte[4];
	    entry.netAddr.etherAddr[5] = byte[5];
	} else if (strcmp(inputBuf, "inet") == 0) {
	    /*
	     * A HOST_INET route address is the ethernet address of the
	     * first gateway machine. The ip address is taken from the
	     * field internetAddr.
	     */
	    int byte[6];
	    entry.netType = HOST_INET;
	    if (fscanf(hostFile, "%2x:%2x:%2x:%2x:%2x:%2x",
		    &byte[0], &byte[1], &byte[2], &byte[3], &byte[4], &byte[5])
		    != 6) {
		return (Host_Entry *) NULL;
	    }
	    entry.netAddr.etherAddr[0] = byte[0];
	    entry.netAddr.etherAddr[1] = byte[1];
	    entry.netAddr.etherAddr[2] = byte[2];
	    entry.netAddr.etherAddr[3] = byte[3];
	    entry.netAddr.etherAddr[4] = byte[4];
	    entry.netAddr.etherAddr[5] = byte[5];

	} else {
	    return (Host_Entry *) NULL;
	}

	/*
	 * <internetAddr>
	 */

	if (fscanf(hostFile, "%100s", inputBuf) != 1) {
	    return (Host_Entry *) NULL;
	}
	entry.inetAddr.s_addr = ntohl(inet_addr(inputBuf));

	/*
	 * <machType>:  first parse the remainder of the line up into
	 * fields.
	 */

	do {
	    c = getc(hostFile);
	} while (isspace(c));
	ungetc(c, hostFile);
	if (fgets(inputBuf, BUFFER_SIZE, hostFile) == NULL) {
	    return (Host_Entry *) NULL;
	}

	/*
	 * If the line didn't all fit in the buffer, throw away the
	 * remainder.
	 */

	for (p = inputBuf; *p !=0; p++) {
	    /* Null loop body */
	}
	if (p[-1] != '\n') {
	    do {
		c = getc(hostFile);
	    } while ((c != '\n') && (c != EOF));
	}

	for (p = inputBuf, numFields = 0; *p != 0; numFields++) {
	    fields[numFields] = p;
	    while (!isspace(*p)) {
		p++;
	    }
	    *p = 0;
	    p++;
	    while (isspace(*p)) {
		p++;
	    }
	    if (numFields == MAX_NAMES+1) {
		break;
	    }
	}
	if (numFields < 2) {
	    return (Host_Entry *) NULL;
	}
	entry.machType = fields[0];

	/*
	 * <name> and <aliases>
	 */

	entry.name = fields[1];
	entry.aliases = &fields[2];
	fields[numFields] = (char *) NULL;
    }
    return &entry;
}
