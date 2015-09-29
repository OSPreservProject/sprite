/*
 * ip.h --
 *
 *	Declarations of external IP-related routines.
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
 * $Header: /sprite/src/daemons/ipServer/RCS/ip.h,v 1.3 89/08/15 19:55:29 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_IP
#define _IPS_IP

#include "sprite.h"

extern void		IP_Init();
extern ReturnStatus	IP_Input();
extern ReturnStatus	IP_Output();
extern void		IP_DelayedOutput();
extern void		IP_QueueOutput();
extern Address		IP_GetSrcRoute();
extern void		IP_SetProtocolHandler();
extern void		IP_FormatPacket();

#endif /* _IPS_IP */
