/* 
 * devRaid.h --
 *
 *	Declarations for RAID device drivers.
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

#ifndef _DEVRAID
#define _DEVRAID

#include "sprite.h"
#include "sync.h"
#include "fs.h"
#include "devBlockDevice.h"

#ifndef MIN
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#endif  MIN

#ifndef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#endif  MAX

/*
 * Data structure for each disk used by raid device.
 */
typedef enum {
    RAID_DISK_INVALID, RAID_DISK_READY, RAID_DISK_FAILED, RAID_DISK_REPLACED,
    RAID_DISK_RECONSTRUCT
} RaidDiskState;

typedef struct RaidDisk {
    Sync_Semaphore	  mutex;
    RaidDiskState	  state;
    unsigned		  numValidSector; /* Used during reconstruction. */
    int			  version;
    int			  useCount;
    Fs_Device	          device;
    DevBlockDeviceHandle *handlePtr;
} RaidDisk;

/*
 * Data structure each RAID device.
 */
typedef enum { RAID_INVALID, RAID_BUSY, RAID_VALID } RaidState;

typedef struct Raid {
    RaidState		 state;
    Sync_Semaphore	 mutex;
    Fs_Device		*devicePtr; /* Device corresponding to this raid. */
    int			 numRow;
    int			 numCol;
    RaidDisk	      ***disk;	    /* 2D array of disks (column major) */

    unsigned		 numSector;
    int		 	 numStripe;
    int			 dataSectorsPerStripe;
    int		 	 sectorsPerDisk;
    int		 	 bytesPerStripeUnit;
    int		 	 dataBytesPerStripe;

    int		 	 numDataCol;
    int		 	 logBytesPerSector;
    int		 	 sectorsPerStripeUnit;
    int		 	 rowsPerGroup;
    int		 	 stripeUnitsPerDisk;
    int		 	 groupsPerArray;
    char		 parityConfig;
} Raid;

/*
 * RaidHandle.
 */
typedef struct RaidHandle {		/* Subclass of DevBlockDeviceHandle. */
    DevBlockDeviceHandle blockHandle;	/* Must be FIRST field. */
    Fs_Device		*devPtr;	/* Device corresponding to handle */
    Raid		*raidPtr;
} RaidHandle;

/*
 * RaidBlockRequest
 */
typedef enum RaidBlockRequestState {	/* Subclass of DevBlockDeviceRequest */
    REQ_FAILED, REQ_READY, REQ_COMPLETED, REQ_PENDING
} RaidBlockRequestState;

typedef struct RaidBlockRequest {
    DevBlockDeviceRequest devReq;
    RaidBlockRequestState state;
    ReturnStatus	  status;
    Raid		 *raidPtr;
    int			  col;
    int			  row;
    RaidDisk		 *diskPtr;
    int			  version;
} RaidBlockRequest;

/*
 * Raid Control structures for syncronizing/communicating with
 * interrupt routines.
 */
typedef struct RaidIOControl {
    Sync_Semaphore	 mutex;
    int			 numIO;
    void	       (*doneProc)();
    ClientData		 clientData;
    ReturnStatus	 status;
    int			 amountTransferred;
    int			 numFailed;
    RaidBlockRequest	*failedReqPtr;
} RaidIOControl;

typedef struct RaidRequestControl {
    RaidBlockRequest	*reqPtr;
    int			 numReq;
    int			 numFailed;
    RaidBlockRequest	*failedReqPtr;
} RaidRequestControl;

typedef struct RaidStripeIOControl {
    Raid		*raidPtr;
    int			 operation;
    unsigned		 firstSector;
    unsigned		 nthSector;
    Address		 buffer;
    void	       (*doneProc)();
    ClientData		 clientData;
    void	       (*recoverProc)();
    int			 ctrlData;
    RaidRequestControl	*reqControlPtr;
    char		*parityBuf;
    char		*readBuf;
    int			 rangeOff;
    int			 rangeLen;
} RaidStripeIOControl;

typedef struct RaidReconstructionControl {
    Raid		*raidPtr;
    int			 col;
    int			 row;
    RaidDisk		*diskPtr;
    int			 stripeID;
    int			 numStripe;
    int			 ctrlData;
    RaidRequestControl	*reqControlPtr;
    char		*parityBuf;
    char		*readBuf;
} RaidReconstructionControl;

#endif _DEVRAID
