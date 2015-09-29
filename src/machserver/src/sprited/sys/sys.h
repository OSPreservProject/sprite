/*
 * sys.h --
 *
 *     Routines and types for the sys module.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 *
 * $Header: /user5/kupfer/spriteserver/src/sprited/sys/RCS/sys.h,v 1.11 92/07/13 21:12:56 kupfer Exp $ SPRITE (Berkeley)
 *
 */

#ifndef _SYS
#define _SYS

#include <sprite.h>
#include <spriteTime.h>
#include <cfuncproto.h>
#include <list.h>
#include <mach.h>
#include <mig_errors.h>

/* 
 * This type defines the buffers that are used to store MIG messages.  
 * We could theoretically have two types, one for request messages and 
 * one for reply messages, but there's not enough difference between 
 * the two types to justify the extra complexity.  Unused buffers are 
 * kept on a linked list.
 * 
 * In order to page-align buffers (for the expected performance win), 
 * we allocate the actual buffer that Mach sees separately and keep a 
 * pointer to the memory.  Note that the Mach message header actually 
 * resides at the beginning of the buffer.
 */
    
#define SYS_MAX_REQUEST_SIZE	8192 /* bytes in a buffer */

typedef struct {
    List_Links		links;	/* for the free list; unused if not on free 
				 * list */
    mig_reply_header_t	*bufPtr; /* actual message buffer */
} Sys_MsgBuffer;

/* 
 * Instrumentation information, per system call.
 */
typedef struct {
    int numCalls;		/* number of invocations of the call */
    Time timeSpent;		/* total time spent doing the call */
    Time copyInTime;		/* time spent in copyin */
    Time copyOutTime;		/* time spent in copyout */
} Sys_CallCount;

#ifdef SPRITED
/* 
 * Declarations that might conflict with user declarations.
 */

extern	mach_port_t sys_RequestPort;

extern	Boolean	sys_ShuttingDown;	/* Set when halting */
extern	Boolean	sys_ErrorShutdown;	/* Set after a bad trap or error */

extern	mach_port_t sys_PrivHostPort;	/* we use this port to make privileged 
					 * requests */ 
extern	mach_port_t sys_PrivProcSetPort;

extern Sys_CallCount sys_CallCounts[];
extern Boolean sys_CallProfiling;

extern void	printf _ARGS_(());
extern void	Sys_Fprintf _ARGS_(());
extern int	Sys_GetHostId _ARGS_((void));
extern int	Sys_GetMachineArch _ARGS_((void));
extern int	Sys_GetMachineType _ARGS_((void));
extern int	Sys_GetNumProcessors _ARGS_((void));
extern void	Sys_Init _ARGS_((void));
extern void	Sys_HostPrint _ARGS_((int spriteID, char *string));
extern void	Sys_ReplyAndFree _ARGS_((Sys_MsgBuffer *requestPtr,
					Sys_MsgBuffer *replyPtr));
extern void	Sys_ServerLoop _ARGS_((void));
extern void	Sys_Shutdown _ARGS_((int flags));
#endif /* SPRITED */

#endif /* _SYS */
