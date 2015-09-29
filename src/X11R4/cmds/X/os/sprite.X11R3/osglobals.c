/*-
 * osglobals.c --
 *	Declarations of variables global to the os layer.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 * Copyright (c) 1987 by Adam de Boor, UC Berkeley
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */
#ifndef lint
static char rcsid[] =
"$Header: /mic/X11R3/src/cmds/Xsprite/os/sprite/RCS/osglobals.c,v 1.4 89/10/08 17:03:30 tve Exp Locker: tve $ SPRITE (Berkeley)";
#endif lint

#include    "spriteos.h"

/*
 * Bit masks for Fs_Select in scheduler
 */
int	*ClientsWithInputMask,	/* Clients with input waiting in their
				 * buffers */
	*LastSelectMask,    	/* Result of last Fs_Select call */
	*EnabledDevicesMask,	/* All enabled devices */
	*SavedAllClientsMask,	/* AllClients mask during grab */
	*AllClientsMask,    	/* All active clients */
	*SavedAllStreamsMask,	/* AllStreams mask during grab */
	*AllStreamsMask;    	/* All active streams (includes devices) */

int	NumActiveStreams,   	/* Number of active streams */
	MaxClients = MAXCLIENTS,
	PseudoDevice;	    	/* Stream ID of X pseudo-device */

char	whichByteIsFirst;	/* Local byte order ('l' or 'B') */

Bool	GrabDone = FALSE;   	/* TRUE if listening to only one client */
ClientPtr grabbingClient;
List_Links	allStreams;	/* All open streams */
