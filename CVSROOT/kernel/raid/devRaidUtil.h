/*
 * devRaidUtil.h --
 *
 *	Miscellaneous mapping macros.
 *
 * Copyright 1989 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header$ SPRITE (Berkeley)
 */

#ifndef _DEVRAIDUTIL
#define _DEVRAIDUTIL

#include <sprite.h>
#include "devBlockDevice.h"
#include "devRaid.h"

/*
 * Maps a RAID sector to stripe ID and visa versa.
 */
#define SectorToStripeID(raidPtr, sector)	\
    ((int)((sector) / (raidPtr)->sectorsPerStripeUnit) / (raidPtr)->numDataCol)

#define StripeIDToSector(raidPtr, stripeID)	\
    ((stripeID) * (raidPtr)->sectorsPerStripeUnit * (raidPtr)->numDataCol)

/*
 * Maps RAID sector to stripe unit ID.
 */
#define SectorToStripeUnitID(raidPtr, sector)	\
    ((sector) / (raidPtr)->sectorsPerStripeUnit)

/*
 * Maps RAID/disk byte addresses to RAID/disk sector addresses and visa versa.
 */
#define ByteToSector(raidPtr, byteAddr)		\
    ((byteAddr) >> (raidPtr)->logBytesPerSector)

#define SectorToByte(raidPtr, sector)		\
    ((sector) << (raidPtr)->logBytesPerSector)

/*
 * Determine first/Nth sector of stripe.
 */
#define FirstSectorOfStripe(raidPtr, sector)	\
    (((sector)/(raidPtr)->dataSectorsPerStripe)*(raidPtr)->dataSectorsPerStripe)

#define NthSectorOfStripe(raidPtr, sector)	\
((((sector)/(raidPtr)->dataSectorsPerStripe)+1)*(raidPtr)->dataSectorsPerStripe)

/*
 * Determine first/Nth sector of stripe unit.
 */
#define FirstSectorOfStripeUnit(raidPtr, sector)	\
    (((sector)/(raidPtr)->sectorsPerStripeUnit)*(raidPtr)->sectorsPerStripeUnit)

#define NthSectorOfStripeUnit(raidPtr, sector)	\
((((sector)/(raidPtr)->sectorsPerStripeUnit)+1)*(raidPtr)->sectorsPerStripeUnit)

/*
 * Determine byte offset within stripe unit.
 */
#define StripeUnitOffset(raidPtr, byteAddr)	\
	((byteAddr) % (raidPtr)->bytesPerStripeUnit)

#define XorRaidRequests(reqControlPtr, raidPtr, destBuf)		\
    (Raid_XorRangeRequests(reqControlPtr, raidPtr, destBuf,		\
	    0, raidPtr->bytesPerStripeUnit))

#define AddRaidDataRequests(reqControlPtr, raidPtr, operation, firstSector, nthSector, buffer, ctrlData)			\
    (Raid_AddDataRangeRequests(reqControlPtr, raidPtr, operation, 	\
	    firstSector, nthSector, buffer, ctrlData,			\
	    0, raidPtr->bytesPerStripeUnit))

#define AddRaidParityRequest(reqControlPtr, raidPtr, operation, firstSector, buffer, ctrlData)			\
    (Raid_AddParityRangeRequest(reqControlPtr, raidPtr, operation, 	\
	    firstSector, buffer, ctrlData,			\
	    0, raidPtr->bytesPerStripeUnit))

extern DevBlockDeviceRequest *Raid_MakeBlockDeviceRequest _ARGS_((Raid *raidPtr,
 int operation, unsigned diskSector, int numSectorsToTransfer, Address buffer, void (*doneProc)(), ClientData clientData, int ctrlData));
extern void Raid_FreeBlockDeviceRequest _ARGS_((DevBlockDeviceRequest *requestPtr));
extern RaidIOControl *Raid_MakeIOControl _ARGS_((void (*doneProc)(), ClientData clientData));
extern void Raid_FreeIOControl _ARGS_((RaidIOControl *IOControlPtr));
extern RaidRequestControl *Raid_MakeRequestControl _ARGS_((Raid *raidPtr));
extern void Raid_FreeRequestControl _ARGS_((RaidRequestControl *reqControlPtr));
extern RaidStripeIOControl *Raid_MakeStripeIOControl _ARGS_((Raid *raidPtr, int operation, unsigned firstSector, unsigned nthSector, Address buffer, void (*doneProc)(), ClientData clientData, int ctrlData));
extern void Raid_FreeStripeIOControl _ARGS_((RaidStripeIOControl *stripeIOControlPtr));
extern RaidReconstructionControl *Raid_MakeReconstructionControl _ARGS_((Raid *raidPtr, int col, int row, RaidDisk *diskPtr, void (*doneProc)(), ClientData clientData, int ctrlData));
extern void Raid_FreeReconstructionControl _ARGS_((RaidReconstructionControl *reconstructionControlPtr));
extern void Raid_RangeRestrict _ARGS_((int start, int len, int rangeOffset, int rangeLen, int fieldLen, int *newStart, int *newLen));
extern void Raid_XorRangeRequests _ARGS_((RaidRequestControl *reqControlPtr, Raid *raidPtr, char *destBuf, int rangeOffset, int rangeLen));
extern void Raid_AddParityRangeRequest _ARGS_((RaidRequestControl *reqControlPtr, Raid *raidPtr, int operation, unsigned sector, Address buffer, int ctrlData, int rangeOffset, int rangeLen));
extern void Raid_AddDataRangeRequests _ARGS_((RaidRequestControl *reqControlPtr, Raid *raidPtr, int operation, unsigned firstSector, unsigned nthSector, Address buffer, int ctrlData, int rangeOffset, int rangeLen));

#endif /* _DEVRAIDUTIL */
