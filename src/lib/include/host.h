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
 *	"$Header: /sprite/src/lib/include/RCS/host.h,v 1.9 92/06/05 12:36:11 voelker Exp $ SPRITE (Berkeley)"
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
#include <net.h>

/*
 * A host information file contains two types of lines.  The
 * first type of line describes a Sprite host:
 *
 *        <spriteID> <machType> <fullname> <aliases>
 *
 * The second type of line describes the types of networks that
 * the host is connected to:
 *
 *        <netType> <netAddr> <inetAddr> 
 *
 * The fields are as follows:
 *
 *	spriteID	    The Sprite ID for the host (small number)
 *	machType	    The machine type.  This string value is used
 *			    when expanding $MACHINE in pathnames.
 *	fullname	    The official name for the machine.
 *	aliases	  	    Other names for the machine.
 *
 *	netType 	    Type of local network by which the machine is
 *	    	  	    connected.  Currently, only `ether', `ultra',
 *	    	  	    and `fddi' are understood.
 *	netAddr	  	    Address for the local network of the given type.
 *	inetAddr  	    The internet address of the network interface
 *                          of the host.
 *
 * Each network interface has a given Internet address, so the Internet
 * address can uniquely identify a specific network interface.  But,
 * a host may have multiple network interfaces, so an Internet address
 * does not uniquely identify a host.
 *
 * After each line that defines a Sprite host follow the lines describing
 * the networks that the host is connected to.
 *
 */

/*
 * Should be the same as NET_MAX_INTERFACES in netTypes.h
 */
#define HOST_MAX_INTERFACES         3

/*
 * A Host_NetInt structure defines the internet and network addresses
 * for a network interface on a host. 
 */
typedef struct Host_NetInt {
    Net_InetAddress     inetAddr;   /* Internet Address */
    Net_Address         netAddr;    /* Address of network interface */
} Host_NetInt;

/*
 * A Host_Entry structure contains the name, etc., of a host, plus
 * the addresses of its network interfaces.  Empty interfaces are
 * denoted by having nets[].netAddr.type set to NET_ADDRESS_NONE.
 */
typedef struct {
    char    	  	*name;      /* Primary name */
    char    	  	**aliases;  /* Other names */
    int	    	  	id;	    /* Sprite ID */
   char			*machType;  /* Machine type, i.e "sun3", "spur" */
   int			numNets;    /* Number of network interfaces. */
   Host_NetInt          nets[HOST_MAX_INTERFACES];
                                    /* Internet and physical address for
				     * each possible network interface */
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
