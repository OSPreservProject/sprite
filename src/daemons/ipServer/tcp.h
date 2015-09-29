/*
 * tcp.h --
 *
 *	Declarations of external TCP-related routines.
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
 * $Header: /sprite/src/daemons/ipServer/RCS/tcp.h,v 1.6 89/08/15 19:55:46 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_TCP
#define _IPS_TCP

#include "sprite.h"

/*
 * TCP_BUF_SIZE is the size for the send and recieve socket buffers for
 * a TCP connection.  This size is used in turn to define
 * TCP_REQUEST_BUF_SIZE which is the size of the pseudo-device request
 * buffer.  This size means that if the client writes more that TCP_BUF_SIZE
 * to the pseudo-device the kernel will block it until the first TCP_BUF_SIZE
 * bytes have been removed from the request buffer.  If the request buffer
 * were larger then large writes would only be partially serviced by TCP,
 * and the extra bytes would just have to be re-written by the kernel.
 */
#define TCP_BUF_SIZE		4096
#define TCP_REQUEST_BUF_SIZE	(TCP_BUF_SIZE)

extern void		TCP_Init();
extern ReturnStatus	TCP_SocketOpen();
extern ReturnStatus	TCP_SocketClose();
extern ReturnStatus	TCP_SocketRead();
extern ReturnStatus	TCP_SocketWrite();
extern int		TCP_SocketSelect();
extern ReturnStatus	TCP_SocketBind();
extern ReturnStatus	TCP_SocketConnect();
extern ReturnStatus	TCP_SocketListen();
extern ReturnStatus	TCP_SocketAccept();
extern ReturnStatus	TCP_SocketClone();
extern ReturnStatus	TCP_SocketShutdown();
extern ReturnStatus	TCP_SocketGetOpt();
extern ReturnStatus	TCP_SocketSetOpt();
extern void		TCP_SocketDestroy();
extern void		TCP_PrintInfo();

#endif /* _IPS_TCP */
