/*
 * host.h --
 *	Header file for users of the Host_ functions. These functions
 *	access a database of all Sprite hosts on the local network, giving
 *	various information about the host that is needed to communicate
 *	with it in various ways...
 *
 * Copyright (c) 1987 by the Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	"$Header: /sprite/src/lib/include/RCS/host.h,v 1.6 90/11/06 16:51:15 jhh Exp $ SPRITE (Berkeley)"
 */

#ifndef _HOST
#define _HOST

#ifndef _TYPES
#include <sys/types.h>
#endif
#ifndef _IN
#include <netinet/in.h>
#endif

#include <sys/stat.h>
/*
 * A host information file contains lines of the following form:
 *
 * <spriteID> <netType> <netAddr> <inetAddr> <machType> <name> <aliases>
 *
 * Where the fields are as follows:
 *	spriteID	    The Sprite ID for the host (small number)
 *	netType 	    Type of local network by which the machine is
 *	    	  	    connected. Currently, only "ether" and "inet" 
 *	    	  	    are understood.
 *	netAddr	  	    Address for the local network
 *	inetAddr  	    The internet address of the host
 *	machType	    The machine type.  This string value is used
 *			    when expanding $MACHINE in pathnames.
 *	name	  	    The official name for the machine
 *	aliases	  	    Other names for the machine
 */

/*
 * A Host_Entry contains the broken out fields of the host table line
 * for a host.
 *
 * Sprite hosts may be connected to the network by different local-area-nets
 * (LANs). The netType field of the Host_Entry tells what type of network
 * it is. The Addr field in the netAddr union should be chosen based on
 * this type.
 *
 */
typedef enum {
    HOST_ETHER,
    HOST_INET
} Host_NetType;

#define HOST_ETHER_ADDRESS_SIZE 6

typedef struct {
    char    	  	*name;      /* Primary name */
    char    	  	**aliases;  /* Other names */
    int	    	  	id;	    /* Sprite ID */
    struct in_addr	inetAddr;   /* Internet address */
    Host_NetType  	netType;    /* Network used to connect to host */
    union {
	unsigned char	etherAddr[HOST_ETHER_ADDRESS_SIZE];
				    /* Ethernet address of machine or 
				     * first gateway if netType is HOST_INET. */
    }	    	  	netAddr;    /* Address for that network */
   char			*machType;  /* Machine type, i.e "sun3", "spur" */
} Host_Entry;

/*
 * Accessor functions
 */
Host_Entry *	Host_ByID();		/* Find host entry by Sprite ID */
Host_Entry *	Host_ByInetAddr();	/* Find by Internet address */
Host_Entry *	Host_ByName();		/* Find entry by name */
Host_Entry *	Host_ByNetAddr();	/* Find by LAN address */
void		Host_End();		/* Close host description file */
Host_Entry *	Host_Next();		/* Retrieve next entry in file */
int		Host_SetFile();		/* Change file to read for info */
int		Host_Start();		/* Open host description file */

#endif /* _HOST */
