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

#ifdef KERNEL
#include <sprite.h>
#include <user/net.h>
#include <syncTypes.h>
#include <netTypes.h>
#include <user/netEther.h>
#include <user/netInet.h>
#include <user/netUltra.h>
#include <netRoute.h>
#else
#include <sprite.h>
#include <net.h>
#include <kernel/syncTypes.h>
#include <kernel/netTypes.h>
#include <netEther.h>
#include <netInet.h>
#include <netUltra.h>
#include <kernel/netRoute.h>
#endif

extern int		net_NetworkHeaderSize[];

extern void Net_Init _ARGS_((void));
extern void Net_Bin _ARGS_((void));
extern void Net_Reset _ARGS_((Net_Interface *interPtr));
extern void Net_Input _ARGS_((Net_Interface *interPtr, Address packetPtr, 
			int packetLength));
extern ReturnStatus Net_Output _ARGS_((int spriteID, 
			Net_ScatterGather *gatherPtr, int gatherLength, 
			Sync_Semaphore *mutexPtr, Net_Route *routePtr));
extern ReturnStatus Net_RawOutput _ARGS_((Net_Interface *interPtr, 
			Address headerPtr, 
			Net_ScatterGather *gatherPtr, int gatherLength));
extern void Net_RecvPoll _ARGS_((Net_Interface *interPtr));
extern ReturnStatus Net_RawOutputSync _ARGS_((Net_Interface *interPtr, 
			Address headerPtr, Net_ScatterGather *gatherPtr, 
			int gatherLength));
extern int Net_Intr _ARGS_((Net_Interface *interPtr));
extern void Net_GatherCopy _ARGS_((register Net_ScatterGather *scatterGatherPtr,
			int scatterGatherLength, register Address destAddr));
extern void Net_SetPacketHandler _ARGS_((Net_Interface *interPtr, 
			void (*handler)()));
extern void Net_RemovePacketHandler _ARGS_((Net_Interface *interPtr));
extern ReturnStatus Net_InstallRouteStub _ARGS_((int size, 
			Net_RouteInfo *routeInfoPtr));
#ifdef KERNEL
extern ReturnStatus Net_InstallRoute _ARGS_((int spriteID, 
			Net_Interface *interPtr, Net_Address *netAddressPtr, 
			int protocol, char *hostname, char *machType, 
			ClientData userData));
#endif
extern void Net_DeleteRoute _ARGS_((Net_Route *routePtr));
extern void Net_SpriteIDToName _ARGS_((int spriteID, int bufferLen, 
			char *buffer));
extern void Net_SpriteIDToMachType _ARGS_((int spriteID, int bufferLen, 
			char *buffer));
extern void Net_RouteInit _ARGS_((void));
extern void Net_ReleaseRoute _ARGS_((Net_Route *routePtr));
extern Net_Route *Net_IDToRoute _ARGS_((int spriteID, int index, Boolean doArp,
			Sync_Semaphore *mutexPtr, int size));
extern int Net_AddrToID _ARGS_((Net_NetworkType netType, int protocol, 
			Net_Address *addressPtr));
extern Net_Interface *Net_GetInterface _ARGS_((Net_NetworkType netType, 
			int number));
extern Net_Interface *Net_NextInterface _ARGS_((Boolean running, 
			int *indexPtr));
extern void Net_HostPrint _ARGS_((int spriteID, char *string));
extern void Net_ArpInit _ARGS_((void));
extern ReturnStatus Net_Arp _ARGS_((int spriteID, Sync_Semaphore *mutexPtr));
extern int  Net_RevArp _ARGS_((Net_Route *routePtr, int protocol, 
		    Net_Address *netAddressPtr, Sync_Semaphore *mutexPtr));
extern void Net_HdrDestString _ARGS_((Net_NetworkType netType, int protocol, 
			Address headerPtr, int bufferLen, char *buffer));
extern int  Net_HdrToID _ARGS_((Net_NetworkType netType, int protocol,
			Address headerPtr));
extern ReturnStatus	Net_IDToRouteStub _ARGS_((int spriteID, int size,
			    Address argPtr));
extern ReturnStatus	Net_GetStats _ARGS_((Net_NetworkType netType,
			    Net_Stats *statPtr));

#endif /* _NET */
