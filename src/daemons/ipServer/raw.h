/*
 * raw.h --
 *
 *	External declarations of the raw IP socket routines.
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
 * $Header: /sprite/src/daemons/ipServer/RCS/raw.h,v 1.5 89/08/15 19:55:34 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_RAW
#define _IPS_RAW

#include "sprite.h"

/*
 * RAW_REQUEST_BUF_SIZE defines the size of the pseudo-device request
 * buffer used to handle requests on raw sockets.   This limits the size
 * of the datagram that can be written to the raw socket to 2048 bytes.
 */
#define RAW_REQUEST_BUF_SIZE	(2048)

extern ReturnStatus	Raw_SocketOpen();
extern ReturnStatus	Raw_SocketClose();
extern ReturnStatus	Raw_SocketRead();
extern ReturnStatus	Raw_SocketWrite();
extern int		Raw_SocketSelect();
extern ReturnStatus	Raw_SocketBind();
extern ReturnStatus	Raw_SocketConnect();
extern ReturnStatus	Raw_SocketShutdown();
extern void		Raw_Input();

#endif /* _IPS_RAW */
