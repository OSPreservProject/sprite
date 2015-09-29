/*
 * usage.h --
 *
 *	Declarations of the interface the the seg usage mantipulation
 *	routines in usage.c
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

#ifndef _USAGE
#define _USAGE

/* constants */

/* data structures */

/* procedures */

extern void MarkSegDirty _ARGS_((int segNumber));


extern void RecordSegInLog _ARGS_((int segNo, int blockOffset));

extern Boolean AddrOlderThan _ARGS_((int addr1, int addr2));

extern Boolean SegIsPartOfLog _ARGS_((int segNo));

extern void  RecovSegUsageSummary _ARGS_((Lfs *lfsPtr, enum Pass pass,
	LfsSeg *segPtr, int startAddress, int offset, 
	LfsSegSummaryHdr *segSummaryHdrPtr));

extern void RollSegUsageForward _ARGS_((Lfs *lfsPtr));

#endif /* _USAGE */

