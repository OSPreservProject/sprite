/*
 * header.h --
 *
 *	Declarations of ...
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

#define IsInRange(diskPtr, startSector, numSector) 	\
    ((startSector) + (numSector) <= (diskPtr)->numValidSector)

#define SectorToStripeID(raidPtr, sector)	\
    ((int)((sector) / (raidPtr)->sectorsPerStripeUnit) / (raidPtr)->numDataCol)

#define StripeIDToSector(raidPtr, stripeID)	\
    ((stripeID) * (raidPtr)->sectorsPerStripeUnit * (raidPtr)->numDataCol)

#define SectorToStripeUnitID(raidPtr, sector)	\
    ((sector) / (raidPtr)->sectorsPerStripeUnit)

#define ByteToSector(raidPtr, byteAddr)		\
    ((byteAddr) >> (raidPtr)->logBytesPerSector)

#define SectorToByte(raidPtr, sector)		\
    ((sector) << (raidPtr)->logBytesPerSector)

#define FirstSectorOfStripe(raidPtr, sector)	\
    (((sector)/(raidPtr)->dataSectorsPerStripe)*(raidPtr)->dataSectorsPerStripe)

#define NthSectorOfStripe(raidPtr, sector)	\
((((sector)/(raidPtr)->dataSectorsPerStripe)+1)*(raidPtr)->dataSectorsPerStripe)

#define FirstSectorOfStripeUnit(raidPtr, sector)	\
    (((sector)/(raidPtr)->sectorsPerStripeUnit)*(raidPtr)->sectorsPerStripeUnit)

#define NthSectorOfStripeUnit(raidPtr, sector)	\
((((sector)/(raidPtr)->sectorsPerStripeUnit)+1)*(raidPtr)->sectorsPerStripeUnit)

#define StripeUnitOffset(raidPtr, byteAddr)	\
	((byteAddr) % (raidPtr)->bytesPerStripeUnit)

#define XorRaidRequests(reqControlPtr, raidPtr, destBuf)		\
    (XorRaidRangeRequests(reqControlPtr, raidPtr, destBuf,		\
	    0, raidPtr->bytesPerStripeUnit))

#define AddRaidDataRequests(reqControlPtr, raidPtr, operation, firstSector, nthSector, buffer, ctrlData)			\
    (AddRaidDataRangeRequests(reqControlPtr, raidPtr, operation, 	\
	    firstSector, nthSector, buffer, ctrlData,			\
	    0, raidPtr->bytesPerStripeUnit))

#define AddRaidParityRequest(reqControlPtr, raidPtr, operation, firstSector, buffer, ctrlData)			\
    (AddRaidParityRangeRequest(reqControlPtr, raidPtr, operation, 	\
	    firstSector, buffer, ctrlData,			\
	    0, raidPtr->bytesPerStripeUnit))

extern DevBlockDeviceRequest *MakeBlockDeviceRequest();
extern RaidDisk *MakeRaidDisk();
extern RaidIOControl *MakeIOControl();
extern RaidRequestControl *MakeRequestControl();
extern RaidStripeIOControl *MakeStripeIOControl();
extern RaidReconstructionControl *MakeReconstructionControl();
extern void FreeBlockDeviceRequest();
extern void FreeRaidDisk();
extern void FreeIOControl();
extern void FreeRequestControl();
extern void FreeStripeIOControl();
extern void FreeReconstructionControl();
extern void MapPhysicalToStripeID();
extern void MapParity();
extern void MapSector();
extern void RangeRestrict();
extern void XorRaidRangeRequests();
extern void AddRaidParityRangeRequest();
extern void AddRaidDataRangeRequests();
