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
