
/***********************************************************************
 *
 * netHppi.h
 *
 *	The include file for other modules to use the HPPI network
 *	routines.
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/src/lib/include/RCS/netHppi.h,v 1.2 92/07/06 14:40:39 elm Exp Locker: elm $
 *
 ***********************************************************************
 */

#ifndef _NETHPPI
#define	_NETHPPI

#include <sprite.h>
#include <list.h>
#include <net.h>
#include <kernel/netTypes.h>
#include <netUltra.h>

/*
 * A callback routine for HPPI stuff.
 */
typedef void (*Net_HppiCallback)();

/*
 * Single scatter-gather element.
 */
typedef struct Net_HppiSGElem {
    Address	addr;
    int		length;
} Net_HppiSGElem;

/*
 * This structure is passed between the HPPI module and higher-level
 * software.  It contains all the connection information that lower
 * and higher level modules should share.
 */
typedef struct Net_HppiConnection {
    Net_Interface*	interPtr;	/* network interface to use */
    unsigned int	connectionId;	/* OUT: connection identifier */
    unsigned int	flags;		/* flags for connection */
    ReturnStatus	status;		/* OUT: connection status */
    ClientData		userData;
    Net_HppiCallback	callbackProc;
    Net_UltraAddress	destAddress;	/* HPPI address to connect to */
    unsigned int	outstandingRecvs;  /* max # of outstanding requests */
    unsigned int	outstandingSends;  /* (for send & receive) */
} Net_HppiConnection;

/*
 * This structure describes a single transfer over the HPPI.  There
 * will be many of these per connection, and there may be several
 * outstanding at once.
 */
typedef struct Net_HppiDataRequest {
    List_Links		links;
    Net_Interface*	interPtr;	/* interface request belongs to */
    unsigned int	connectionId;	/* connection data should go on */
    unsigned int	flags;		/* flags for request */
    unsigned int	status;		/* OUT: status of connection */
    ClientData		userData;
    Net_HppiCallback	callbackProc;
    unsigned int	xferSize;	/* total size of transfer */
    unsigned int	xferOffset;	/* offset of first byte within conn */
    unsigned int	sgSize;		/* # of elements in following array */
    Net_HppiSGElem	sg[1];		/* scatter-gather array */
} Net_HppiDataRequest;

#define	NET_HPPI_DATA_LAST_SEND	0x1	/* last send in message */

extern ReturnStatus	Net_HppiConnectionOpen _ARGS_((Net_HppiConnection*
						       connStatusPtr));
extern ReturnStatus	Net_HppiConnectionListen _ARGS_((Net_HppiConnection*
							connStatusPtr));
extern ReturnStatus	Net_HppiReceiveData _ARGS_
				((Net_HppiConnection* connPtr,
				  Net_HppiDataRequest* dataReqPtr));
extern ReturnStatus	Net_HppiSendData _ARGS_((Net_HppiConnection* connPtr,
				  Net_HppiDataRequest* dataReqPtr));
extern ReturnStatus	Net_HppiConnectionClose _ARGS_((Net_HppiConnection*
							connPtr));
extern void		Net_HppiLoopback _ARGS_((int hppiNum, int size,
						 void (*callbackProc)(),
						 ClientData clientData));
extern void		Net_HppiSrcPattern _ARGS_((int hppiNum, int size,
						   void (*callbackProc)(),
						   ClientData clientData));
#endif	/* _NETHPPI */
