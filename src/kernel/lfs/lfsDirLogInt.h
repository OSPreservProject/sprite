/*
 * lfsDirLogInt.h --
 *
 *	Declarations of data structure internal to the implemenation of
 *	a directory change log for a LFS file system.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /cdrom/src/kernel/Cvsroot/kernel/lfs/lfsDirLogInt.h,v 1.2 92/09/03 18:13:27 shirriff Exp $ SPRITE (Berkeley)
 */

#ifndef _LFSDIRLOGINT
#define _LFSDIRLOGINT

#include <lfsDirOpLog.h>

extern LfsDirOpLogEntry *LfsDirLogEntryAlloc _ARGS_((struct Lfs *lfsPtr, 
			int entrySize, int logSeqNum, Boolean *foundPtr));

#endif /* _LFSDIRLOGINT */

