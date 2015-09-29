/*
 * monitorClient.h --
 *
 *	Declarations of public routines for the file system monitor library.
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
 * $Header: /sprite/src/lib/monitorClient/RCS/monitorClient.h,v 1.1 89/01/11 12:04:44 mlgray Exp $ SPRITE (Berkeley)
 */

#ifndef _MONITOR
#define _MONITOR

/* constants */

/* data structures */
/* Port for client to read from monitor */
extern	int	monClient_ReadPort;		/* maybe this should be hidden,
						 * but then I need to have
						 * routines to set the file
						 * descriptor mask for
						 * selection, and to check if
						 * there's input to the client.
						 */

/* procedures */

#ifdef NOTDEF
/* These are the real def's, from when Boolean was okay... */
extern	Boolean	MonClient_ChangeDir();
extern	Boolean	MonClient_AddDir();
extern	Boolean	MonClient_DeleteDir();
extern	Boolean	MonClient_Register();
extern	void	Mon_StatDirs();
#else /* NOTDEF */
extern	int	MonClient_ChangeDir();
extern	int	MonClient_AddDir();
extern	int	MonClient_DeleteDir();
extern	int	MonClient_Register();
extern	void	Mon_StatDirs();
#endif /* NOTDEF */

#endif _MONITOR
