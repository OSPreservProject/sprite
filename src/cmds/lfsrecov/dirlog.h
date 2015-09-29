/*
 * dirlog.h --
 *
 *	Declarations of the interface the the directory change log
 *	mantipulation routines in dirlog.c
 *
 * Copyright 1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /sprite/lib/forms/RCS/proto.h,v 1.7 91/02/09 13:24:52 ouster Exp $ SPRITE (Berkeley)
 */

#ifndef _DIRLOG
#define _DIRLOG

/* constants */

/* data structures */

/* procedures */

extern void RecordLogEntryStart _ARGS_((LfsDirOpLogEntry *entryPtr, 
					int addr));
extern Boolean FindStartEntryAddr _ARGS_((int logSeqNum, int *addrPtr));

extern char *DirOpFlagsToString _ARGS_((int opFlags));

extern void ShowDirLogBlock _ARGS_((LfsDirOpLogBlockHdr *hdrPtr, int addr));

extern void RecovDirLogBlock _ARGS_((LfsDirOpLogBlockHdr *hdrPtr, int addr, 
			enum Pass pass));

extern void RecovDirLogEntry _ARGS_((LfsDirOpLogEntry *entryPtr, int addr));

#endif /* _DIRLOG */


