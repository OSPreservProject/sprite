/* 
 * gate.h --
 *
 *	Header file for users of the Gate_ functions.  These
 *      functions access a database of gateways for use with
 *      communicating with Sprite hosts accross those gateways.
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * "$Header: /sprite/src/lib/include/RCS/gate.h,v 1.2 92/06/05 12:36:43 voelker Exp $"
 */

#ifndef _GATE
#define _GATE

#include "host.h"

/*
 * A gateway information file contains listings of gateway definitions,
 * one per line.  Each line has the form:
 *
 *     <descr> <netType> <netAddr> <inetAddr>
 *
 *
 *      descr               One word description of the gateway.
 *	netType 	    Type of local network by which the gateway is
 *	    	  	    connected.  Currently, only `ether', `ultra',
 *	    	  	    and `fddi' are understood.
 *	netAddr	  	    Address for the local network of the given type.
 *	inetAddr  	    The internet address of the network interface
 *                          of the host.
 *
 * When routes are installed to specific hosts on a machine, those routes
 * need to know the net addresses (e.g., ethernet address) of those
 * machines.  However, if the host is on another subnet, then the route
 * should use the net address of the gateway to that subnet instead of
 * the net address of the host.  The decision to route to the host
 * directly or to a gateway is done at route installation time, based
 * upon the internet address of the host.  The route installation
 * routines use the gateway file to translate internet addresses of hosts
 * on different subnets to net addresses to gateways to those hosts.
 *
 */

typedef struct Gate_Entry {
    char              *desc;        /* Description of gateway */
    Net_Address       netAddr;      /* Network address of the gateway */
    Net_InetAddress   inetAddr;     /* Internet address mask for those network
				     * interfaces that use this gateway */
} Gate_Entry;

/*
 * Accessor functions
 */
Gate_Entry *	Gate_ByInetAddr();	/* Find by Internet address */
Gate_Entry *	Gate_ByDesc();		/* Find entry by name */
Gate_Entry *	Gate_ByNetAddr();	/* Find by LAN address */
void		Gate_End();		/* Close gate description file */
Gate_Entry *	Gate_Next();		/* Retrieve next entry in file */
int		Gate_SetFile();		/* Change file to read for info */
int		Gate_Start();		/* Open gateway description file */


#endif /* _GATE */



