/*
 * fsNameOpsInt.h --
 *
 *	Internal definitions for pathname related operations that go
 *	with the other definitions in fsNameOps.h.  This file is
 *	needed because of the FsUnionData which is a pain to export
 *	to user programs because it references private
 *	file, device, and pseudo-device data.
 *
 * Copyright 1986 Regents of the University of California
 * All rights reserved.
 *
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _FSNAMEOPSINT
#define _FSNAMEOPSINT

#include "fsNameOps.h"
#include "fsFile.h"
#include "fsDevice.h"
#include "fsPdevState.h"

/*
 * The stream data is a reference to the following union.  The union is used
 * to facilitate byte-swaping in the RPC stubs.
 */
typedef	union	FsUnionData {
    FsFileState		fileState;
    FsDeviceState	devState;
    FsPdevState		pdevState;
} FsUnionData;

typedef	struct	FsOpenResultsParam {
    int			prefixLength;
    FsOpenResults	openResults;
    FsUnionData		openData;
} FsOpenResultsParam;

#endif /* _FSNAMEOPSINT */
