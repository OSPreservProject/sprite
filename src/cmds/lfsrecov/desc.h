/*
 * desc.h --
 *
 *	Declarations of the interface the the file descriptor mantipulation
 *	routines in desc.c
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

#ifndef _DESC
#define _DESC

/* constants */

/* data structures */

/* procedures */

extern void RecordNewDesc _ARGS_((int fileNumber, int address, 
				LfsFileDescriptor *descPtr));
extern Boolean FindNewDesc _ARGS_((int fileNumber, int *addrPtr, 
				LfsFileDescriptor **descPtrPtr));
extern Boolean ScanNewDesc _ARGS_((ClientData *clientDataPtr, 
	int *fileNumberPtr, int *addressPtr, LfsFileDescriptor **descPtrPtr));

extern void ScanNewDescEnd _ARGS_((ClientData *clientDataPtr));

extern void RecordUnrefDesc _ARGS_((int fileNumber, int dirFileNumber, 
			Fslcl_DirEntry *dirEntryPtr));


extern Boolean ScanUnrefDesc _ARGS_((ClientData *clientDataPtr, 
				int *fileNumberPtr));

extern void ScanUnrefDescEnd _ARGS_((ClientData *clientDataPtr));

extern void RecovDescMapSummary _ARGS_((Lfs *lfsPtr, enum Pass pass, 
	LfsSeg *segPtr, int startAddress, int offset, 
	LfsSegSummaryHdr *segSummaryHdrPtr));


#endif /* _DESC */

