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
static char rcsid[] = "$Header: /user6/jhh/hosttest/RCS/Host_Next.c,v 1.2 92/05/29 16:30:15 voelker Exp Locker: jhh $ SPRITE (Berkeley)";
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

static ReturnStatus GetInterfaces();
static Net_EtherAddress emptyEtherAddr;
static Net_FDDIAddress  emptyFDDIAddr;



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
    static char      	hostBuffer[BUFFER_SIZE];
    static char      	interBuffer[BUFFER_SIZE];
    static char *	aliases[MAX_NAMES+1];
    register char *	p;
    int			numAliases;
    int    	  	c;
    int                 i;
    register int        result;
    char		*token;
    int			offset;
    Boolean 		backup = FALSE;
    Boolean		foundOne;
    Net_AddressType     currentType;

    if (hostFile == (FILE *) NULL) {
	return ((Host_Entry *) NULL);
    }
    foundOne = FALSE;
    while (!foundOne) {
	if (feof(hostFile)) {
	    return NULL;
	}
	offset = ftell(hostFile);
	fgets(hostBuffer, BUFFER_SIZE, hostFile);
	if (strchr(hostBuffer, '\n') == NULL)  {
	    do {
		c = getc(hostFile);
	    } while ((c != '\n') && (c != EOF));
	}
	/* 
	 * Skip the line if it is a comment or blank.
	 */
	if ((hostBuffer[0] == '#') || (hostBuffer[0] == '\n')) {
	    continue;
	}

	/*
	 * First get the host line, then the interface lines.
	 */

	/*
	 * <spriteID>
	 */

	token = strtok(hostBuffer, " \t");
	if (token == NULL) {
	    continue;
	}

	if (sscanf(token, "%d", &entry.id) != 1) {
	    continue;
	}

	/*
	 * <machType>:  first parse the remainder of the line up to
	 * fields.
	 */
	token = strtok(NULL, " \t");
	if (token == NULL) {
	    continue;
	}
	entry.machType = token;

	/*
	 * <name> and <aliases>
	 */

	token = strtok(NULL, " \t");
	if (token == NULL) {
	    continue;
	}
	entry.name = token;
	numAliases = 0;
	token = strtok(NULL, " \t\n");
	while(token != NULL) {
	    if (numAliases == MAX_NAMES) {
		break;
	    }
	    aliases[numAliases] = token;
	    numAliases++;
	    token = strtok(NULL, " \t\n");
	}
	aliases[numAliases] = (char *) NULL;
	foundOne = TRUE;
    }

    if (!foundOne) {
	return NULL;
    }
    entry.aliases = aliases;
    foundOne = FALSE;
    /*
     * Now get the interface lines.
     */
    for (i = 0; i < HOST_MAX_INTERFACES; i++) {
	entry.nets[i].netAddr.type = NET_ADDRESS_NONE;
    }
    i = 0;
    while (!feof(hostFile)) {
	if ( i >= HOST_MAX_INTERFACES) {
	    break;
	}
	offset = ftell(hostFile);
	fgets(interBuffer, BUFFER_SIZE, hostFile);
	if (strchr(interBuffer, '\n') == NULL)  {
	    do {
		c = getc(hostFile);
	    } while ((c != '\n') && (c != EOF));
	}

	/* 
	 * Skip the line if it is a comment or blank.
	 */
	if ((interBuffer[0] == '#') || (interBuffer[0] == '\n')) {
	    continue;
	}

	token = strtok(interBuffer, " \t");
	if (token == NULL) {
	    continue;
	}
	/*
	 * Check to see if we have found a host line.  If so, stop
	 * gathering interface lines. Host lines begin with a digit.
	 */

	if (isdigit(token[0])) {
	    backup = TRUE;
	    break;
	}
	/*
	 * Reset the entry in case the previous line was invalid, and we 
	 * now are reusing it's entry.
	 */
	entry.nets[i].netAddr.type = NET_ADDRESS_NONE;
	/*
	 * <netType> and <netAddr>
	 */
	
	if (strcmp(token, "ether") == 0) {
	    currentType = NET_ADDRESS_ETHER;
	} else if (strcmp(token, "ultra") == 0) {
	    currentType = NET_ADDRESS_ULTRA;
	} else if (strcmp(token, "fddi") == 0) {
	    currentType = NET_ADDRESS_FDDI;
	} else {
	    continue;
	}
	
	token = strtok(NULL, " \t\n");
	if (token == NULL) {
	    continue;
	}
	entry.nets[i].netAddr.type = currentType;
	result = Net_StringToAddr(token, currentType,
				     &entry.nets[i].netAddr);
	if (result != SUCCESS) {
	    continue;
	}
	/*
	 * If the address is invalid, then the entry is invalid.
	 */
	switch (currentType) {
	case NET_ADDRESS_ETHER:
	    if (!Net_EtherAddrCmp(emptyEtherAddr, 
				  entry.nets[i].netAddr.address.ether)) {
		continue;
	    }
	case NET_ADDRESS_FDDI:
	    if (!Net_FDDIAddrCmp(emptyFDDIAddr,
				 entry.nets[i].netAddr.address.fddi)) {
		continue;
	    }
	case NET_ADDRESS_ULTRA:
	default:
	    break;
	}
    
	/*
	 * <internetAddr>
	 */
	
	token = strtok(NULL, " \t\n");
	if (token == NULL) {
	    continue;
	}
	if (token[0] == '*') {
	    /*
	     * Empty internet address.
	     */
	    entry.nets[i].inetAddr = 0;
	} else {
	    entry.nets[i].inetAddr = Net_StringToInetAddr(token);
	}
	/*
	 * Only go on to the next entry if we have successfully parsed
	 * a valid one.
	 */
	foundOne = TRUE;
	i++;
    }
    if (backup) {
	fseek(hostFile, offset, 0);
    }
    if (!foundOne) {
	return NULL;
    }
    entry.numNets = i;
    return &entry;
}
