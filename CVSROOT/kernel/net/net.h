/*
 * net.h --
 *
 *	This defines the external definitions for the networking software.
 *
 * Copyright 1985, 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _NET
#define _NET

#include <sprite.h>
#include <user/net.h>
#include <syncTypes.h>
#include <netTypes.h>

/*
 * Forward Declarations.
 */
extern	Net_EtherStats	net_EtherStats;

extern	void	Net_Init();
extern  void    Net_Bin();
extern	void	Net_Reset();
extern	void	Net_Input();
extern	ReturnStatus	Net_Output();
extern	void	Net_OutputRawEther();
extern	void	Net_RecvPoll();
extern	void	Net_EtherOutputSync();
extern	int	Net_Intr();
extern	void	Net_GatherCopy();

extern	ReturnStatus Net_InstallRouteStub();
extern	ReturnStatus Net_InstallRoute();
extern	void	Net_SpriteIDToName();
extern	char *	Net_SpriteIDToMachType();
extern	void	Net_RouteInit();
extern	void	Net_NameToAddr();
extern	void	Net_AddrToName();

extern Net_Route *Net_IDToRoute();
extern int	 Net_AddrToID();

extern int Net_RouteMTU();
extern int Net_MaxProtoHdrSize();

extern Net_Route *Net_Arp();
extern int	Net_RevArp();

extern void	Net_HostPrint();	/* Moved to Sys_HostPrint */

extern void	Net_HdrDestString();

#endif /* _NET */
