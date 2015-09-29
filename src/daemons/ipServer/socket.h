/*
 * socket.h --
 *
 *	External declarations of the socket-related routines.
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
 * $Header: /sprite/src/daemons/ipServer/RCS/socket.h,v 1.3 89/08/15 19:55:41 rab Exp $ SPRITE (Berkeley)
 */

#ifndef _IPS_SOCKET
#define _IPS_SOCKET

#include "sprite.h"
#include "dev/net.h"

#include "sockInt.h"

/* constants */

/* data structures */

typedef struct Sock_SharedInfo *Sock_InfoPtr;

/* procedures */

extern void		Sock_Init();
extern ReturnStatus	Sock_Open();
extern ReturnStatus	Sock_Close();
extern ReturnStatus	Sock_Read();
extern ReturnStatus	Sock_Write();
extern ReturnStatus	Sock_IOControl();
extern int		Sock_Select();
extern void		Sock_ReturnError();
extern void		Sock_SetError();
extern Sock_InfoPtr	Sock_Match();
extern Sock_InfoPtr	Sock_Clone();
extern void		Sock_Destroy();
extern void		Sock_NotifyWaiter();
extern void		Sock_HaveUrgentData();
extern void		Sock_Connected();
extern void		Sock_Disconnected();
extern void		Sock_Disconnecting();
extern Boolean		Sock_IsOptionSet();
extern Boolean		Sock_HasUsers();
extern Boolean		Sock_IsRecvStopped();
extern Boolean		Sock_IsSendStopped();
extern void		Sock_UrgentDataNext();
extern void		Sock_StopRecv();
extern void		Sock_StopSending();
extern Sock_InfoPtr	Sock_ScanList();
extern void		Sock_BadRoute();
extern int		Sock_PrintInfo();

extern int		Sock_BufSize();
extern void		Sock_BufRemove();
extern ReturnStatus	Sock_BufAppend();
extern void		Sock_BufCopy();
extern void		Sock_BufAlloc();
extern ReturnStatus	Sock_BufFetch();

typedef enum{
    SOCK_RECV_BUF,
    SOCK_SEND_BUF,
} Sock_BufType;

typedef enum {
    SOCK_BUF_USED,
    SOCK_BUF_FREE,
    SOCK_BUF_MAX_SIZE,
} Sock_BufRequest;

#define SOCK_BUF_PEEK	NET_PEEK
#define SOCK_BUF_STREAM	0x80000000

#endif /* _IPS_SOCKET */
