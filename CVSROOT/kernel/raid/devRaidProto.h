#include "cfuncproto.h"
#include "bitvec.h"
#include "devRaidLock.h"
#include "devRaidLog.h"
#include "devRaid.h"

/*
 * bitvec.c
 */
extern BitVec MakeBitVec _ARGS_((int n));
extern int ClearBitVec _ARGS_((BitVec bitVec, int n));
extern int GetBitIndex _ARGS_((BitVec bitVec, int i, int n));

/*
 * bxor.c
 */
extern void Xor2 _ARGS_((register int numBytes, char *sourcePtr, char *destPtr));

/*
 * debugMem.c
 */
extern void InitDebugMem _ARGS_((void));
extern void Free _ARGS_((char *memPtr));
extern char *Malloc _ARGS_((unsigned size));

/*
 * devDebug.c
 */
extern DevBlockDeviceHandle *DevDebugAttach _ARGS_((Fs_Device *devicePtr));

/*
 * devPrint.c
 */
extern int PrintHandle _ARGS_((DevBlockDeviceHandle *handlePtr));
extern int PrintDevice _ARGS_((Fs_Device *devicePtr));
extern int PrintRequest _ARGS_((DevBlockDeviceRequest *requestPtr));
extern int PrintRaid _ARGS_((Raid *raidPtr));
extern int PrintTime _ARGS_((void));

/*
 * devRaid.c
 */
extern DevBlockDeviceHandle *DevRaidAttach _ARGS_((Fs_Device *devicePtr));

/*
 * devRaidDisk.c
 */
extern void ReportRaidDiskAttachError _ARGS_((int type, int unit));
extern RaidDisk *MakeRaidDisk _ARGS_((int col, int row, int type, int unit, int version, int numSector));
extern void FreeRaidDisk _ARGS_((RaidDisk *diskPtr));
extern void FailRaidDisk _ARGS_((Raid *raidPtr, int col, int row, int version));
extern void ReplaceRaidDisk _ARGS_((Raid *raidPtr, int col, int row, int version, int type, int unit, int numValidSector));

/*
 * devRaidHardInit.c
 */
extern void InitiateHardInit _ARGS_((Raid *raidPtr, int startStripe, int numStripe, void (*doneProc)(), ClientData clientData, int ctrlData));

/*
 * devRaidIOC.c
 */
extern void ReportRequestError _ARGS_((RaidBlockRequest *reqPtr));
extern void ReportHardInitFailure _ARGS_((int stripeID));
extern void ReportParityCheckFailure _ARGS_((int stripeID));
extern void ReportReconstructionFailure _ARGS_((int col, int row));

/*
 * devRaidInitiate.c
 */
extern void InitiateIORequests _ARGS_((RaidRequestControl *reqControlPtr, void (*doneProc)(), ClientData clientData));
extern void InitiateStripeIOs _ARGS_((Raid *raidPtr, int operation, unsigned firstSector, unsigned nthSector, Address buffer, void (*doneProc)(), ClientData clientData, int ctrlData));
extern void InitiateSimpleStripeIOs _ARGS_((Raid *raidPtr, int operation, unsigned firstSector, unsigned nthSector, Address buffer, void (*doneProc)(), ClientData clientData, int ctrlData));

/*
 * devRaidLock.c
 */
extern void InitStripeLocks _ARGS_((void));
extern void SLockStripe _ARGS_((Raid *raidPtr, int stripe));
extern int CheckPointRaid _ARGS_((Raid *raidPtr));
extern void XLockStripe _ARGS_((Raid *raidPtr, int stripe));
extern void SUnlockStripe _ARGS_((Raid *raidPtr, int stripe));
extern void XUnlockStripe _ARGS_((Raid *raidPtr, int stripe));
extern void LockRaid _ARGS_((Raid *raidPtr));
extern void UnlockRaid _ARGS_((Raid *raidPtr));
extern void BeginRaidUse _ARGS_((Raid *raidPtr));
extern void EndRaidUse _ARGS_((Raid *raidPtr));
extern void InitSema _ARGS_((Sema *semaPtr, char *name, int val));
extern void DownSema _ARGS_((Sema *semaPtr));
extern void UpSema _ARGS_((Sema *semaPtr));

/*
 * devRaidLog.c
 */
extern void InitRaidLog _ARGS_((Raid *raidPtr));
extern void EnableLog _ARGS_((Raid *raidPtr));
extern void DisableLog _ARGS_((Raid *raidPtr));
extern ReturnStatus ApplyRaidLog _ARGS_((Raid *raidPtr));
extern ReturnStatus SaveDiskState _ARGS_((Raid *raidPtr, int col, int row, int type, int unit, int version, int numValidSector));
extern ReturnStatus SaveRaidParam _ARGS_((Raid *raidPtr));
extern ReturnStatus SaveRaidLog _ARGS_((Raid *raidPtr));
extern ReturnStatus SaveRaidState _ARGS_((Raid *raidPtr));
extern ReturnStatus RaidConfigure _ARGS_((Raid *raidPtr, char *charBuf));
extern ReturnStatus RestoreRaidState _ARGS_((Raid *raidPtr));
extern void MasterFlushLog _ARGS_((Raid *raidPtr));
extern void LogStripe _ARGS_((Raid *raidPtr, int stripeID));

/*
 * devRaidMap.c
 */
extern void MapPhysicalToStripeID _ARGS_((Raid *raidPtr, int col, int row, unsigned sector, int *outStripeIDPtr));
extern void MapParity _ARGS_((Raid *raidPtr, unsigned sectorNum, int *outColPtr, int *outRowPtr, unsigned *sectorNumPtr));
extern void MapSector _ARGS_((Raid *raidPtr, unsigned sectorNum, int *outColPtr, int *outRowPtr, unsigned *sectorNumPtr));

/*
 * devRaidParityCheck.c
 */
extern void InitiateParityCheck _ARGS_((Raid *raidPtr, int startStripe, int numStripe, void (*doneProc)(), ClientData clientData, int ctrlData));

/*
 * devRaidReconstruct.c
 */
extern void InitiateReconstruction _ARGS_((Raid *raidPtr, int col, int row, int version, int numSector, int uSec, void (*doneProc)(), ClientData clientData, int ctrlData));

/*
 * devRaidUtil.c
 */
extern DevBlockDeviceRequest *MakeBlockDeviceRequest _ARGS_((Raid *raidPtr, int operation, unsigned diskSector, int numSectorsToTransfer, Address buffer, void (*doneProc)(), ClientData clientData, int ctrlData));
extern void FreeBlockDeviceRequest _ARGS_((DevBlockDeviceRequest *requestPtr));
extern RaidIOControl *MakeIOControl _ARGS_((void (*doneProc)(), ClientData clientData));
extern void FreeIOControl _ARGS_((RaidIOControl *IOControlPtr));
extern RaidRequestControl *MakeRequestControl _ARGS_((Raid *raidPtr));
extern void FreeRequestControl _ARGS_((RaidRequestControl *reqControlPtr));
extern RaidStripeIOControl *MakeStripeIOControl _ARGS_((Raid *raidPtr, int operation, unsigned firstSector, unsigned nthSector, Address buffer, void (*doneProc)(), ClientData clientData, int ctrlData));
extern void FreeStripeIOControl _ARGS_((RaidStripeIOControl *stripeIOControlPtr));
extern RaidReconstructionControl *MakeReconstructionControl _ARGS_((Raid *raidPtr, int col, int row, RaidDisk *diskPtr, void (*doneProc)(), ClientData clientData, int ctrlData));
extern void FreeReconstructionControl _ARGS_((RaidReconstructionControl *reconstructionControlPtr));
extern void RangeRestrict _ARGS_((int start, int len, int rangeOffset, int rangeLen, int fieldLen, int *newStart, int *newLen));
extern void XorRaidRangeRequests _ARGS_((RaidRequestControl *reqControlPtr, Raid *raidPtr, char *destBuf, int rangeOffset, int rangeLen));
extern void AddRaidParityRangeRequest _ARGS_((RaidRequestControl *reqControlPtr, Raid *raidPtr, int operation, unsigned sector, Address buffer, int ctrlData, int rangeOffset, int rangeLen));
extern void AddRaidDataRangeRequests _ARGS_((RaidRequestControl *reqControlPtr, Raid *raidPtr, int operation, unsigned firstSector, unsigned nthSector, Address buffer, int ctrlData, int rangeOffset, int rangeLen));

/*
 * strUtil.c
 */
extern char *ScanLine _ARGS_((char **ps1, char *s2));
extern char *ScanWord _ARGS_((char **ps1, char *s2));
