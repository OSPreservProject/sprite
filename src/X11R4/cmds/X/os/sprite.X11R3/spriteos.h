/*-
 * spriteos.h --
 *	Internal data for Sprite OS layer.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	"$Header: /mic/X11R3/src/cmds/Xsprite/os/sprite/RCS/spriteos.h,v 1.12 89/10/04 19:18:15 tve Exp Locker: tve $ SPRITE (Berkeley)"
 */
#ifndef _OS_H
#define _OS_H

/*
 * Because both Sprite and X define the type 'Time' (and they're incompatible),
 * we have to kludge things by making the definition of Time in the Sprite
 * files be SpriteTime. Henceforth, that is what the Sprite Time value should
 * be referred to as.
 */
#include    "buf.h"
#include    <list.h>

#include    "X.h"
#include    "Xmd.h"
#include    "os.h"
#include    "dixstruct.h"

/*
 * Scheduling interval. After MAX_PACKETS requests have been processed,
 * the client is forced to yield the server to the next client.
 */
#define MAX_PACKETS	10

/*
 * Various aspects of a client must be tracked to handle the protocol.
 * These are stored in the ClntPrivRec hanging from the osPrivate field of
 * the ClientRec.
 *
 * There are two distinct types of connections being used here. One is
 * over a Pseudo-device where the connection is very synchronous. The
 * other is over a TCP stream, as managed by the TCP module. There are
 * various differences between the two, not the least of which that there
 * is only one stream for the TCP connection. To embody this, each client
 * has a Read and a Write function, as well as a stream-private pointer
 * that is manipulated only by the controlling module. To determine when
 * a client is ready, two select masks are also maintained.
 */
typedef struct {
    char    	  *(*readProc)();   /* Function to read from the client */
    int	    	  (*writeProc)();   /* Function to write to the client */
    void    	  (*closeProc)();   /* Function to close down the client */

    int	    	  *mask;	    /* Mask of all streams for this client
				     * (set by the controlling module) */
    int	    	  *ready;	    /* Mask of streams that are ready
				     * (set by the scheduler) */
    int	    	  maskWidth;  	    /* Width of said masks */
    pointer 	  devicePrivate;    /* Data private to the controlling module */
} ClntPrivRec, *ClntPrivPtr;

/*
 * Global data
 */
extern int  	  	*AllClientsMask;    	/* All active clients */
extern int  	  	*SavedAllClientsMask;   /* When grabbed */
extern int  	  	*AllStreamsMask;  	/* All streams to check */
extern int  	  	*SavedAllStreamsMask;   /* When grabbed */
extern int  	  	*LastSelectMask;	/* Result of Fs_Select */
extern int  	  	*EnabledDevicesMask;    /* Mask from devices */
extern int  	  	*ClientsWithInputMask;  /* Mask of clients with input
					         * still in their buffers */
extern int  	  	NumActiveStreams;   	/* The number of active streams
						 * used in the various bit
						 * masks */
#ifdef TCPCONN
extern int  	  	TCP_Conn;   	    	/* TCP listening socket */
#endif TCPCONN

extern int  	  	Pdev_Conn;	    	/* New Pseudo-device control
						 * stream ID */

extern char 	  	*display;	    	/* Our display number */
extern Bool 	  	GrabDone;   	    	/* TRUE if listening to only
						 * one client */
extern ClientPtr  	grabbingClient;   	/* Client that performed the
						 * grab. */
extern char 	  	whichByteIsFirst;   	/* Our byte order */
extern List_Links	allStreams; 	    	/* All open streams */
extern int  	  	spriteCheckInput;
extern void 	  	spriteInputAvail();
extern void 	  	ExpandMasks();
extern Bool 	  	clientsDoomed;
/*
 * Debug control:
 *	There is one bit per module. It is up to the module what debug
 *	information to print.
 *	DBG(module) returns TRUE if debugging is on for that module.
 */
extern int	    	debug;
#define DEBUG_SCHED	0x00000001
#define DEBUG_CONN	0x00000002
#define DEBUG_PDEV 	0x00000004
#define DEBUG_TCP 	0x00000010

#define I(a)	  	a
#ifdef __STDC__
#define CONCAT(a,b)	a##b
#else
#define CONCAT(a,b)	I(a)b
#endif /* __STDC__ */
#define DBG(module)	(debug & CONCAT(DEBUG_,module))

#define FamilySprite	3   	/* Pseudo-device access control -- should
				 * be in X.h */

#endif _OS_H
