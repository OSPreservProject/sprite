/*
 * udp.h --
 *
 *	Declarations of external UDP-related routines.
 *
 * Copyright 1987 Regents of the University of California
 * All rights reserved.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header: /sprite/src/daemons/ipServer/RCS/udp.h,v 1.5 89/08/15 19:55:55 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_UDP
#define _IPS_UDP

#include "sprite.h"

/*
 * UDP_MAX_DATAGRAM_SIZE defines how big a datagram can be sent via UDP.
 * It has to be at least a bit over 8K to allow NFS to send 8K of data
 * plus some header information.
 * UDP_REQUEST_BUF_SIZE defines the size of the pseudo-device request buffer.
 * It is made large enough to hold the datagram, plus pseudo-device
 * header, plus a UDP/IP header.  This way we can assemble (and fragment)
 * datagrams as they sit in the pseudo-device request buffer.
 */
#define UDP_MAX_DATAGRAM_SIZE	9000
#define UDP_REQUEST_BUF_SIZE	(UDP_MAX_DATAGRAM_SIZE)

/*
 * UDP_WRITE_BEHIND - if this is TRUE then asynchronous writes are allowed
 * to the UDP request buffer.
 */
#define UDP_WRITE_BEHIND	TRUE

extern void		UDP_Init();
extern void		UDP_RequestHandler();
extern ReturnStatus	UDP_SocketOpen();
extern ReturnStatus	UDP_SocketClose();
extern ReturnStatus	UDP_SocketRead();
extern ReturnStatus	UDP_SocketWrite();
extern int		UDP_SocketSelect();
extern ReturnStatus	UDP_SocketBind();
extern ReturnStatus	UDP_SocketConnect();
extern ReturnStatus	UDP_SocketShutdown();
extern void		UDP_SocketInput();

#endif /* _IPS_UDP */
