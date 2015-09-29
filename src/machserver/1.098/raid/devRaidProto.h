#include "sprite.h"

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
#ifdef _FS
extern DevBlockDeviceHandle *DevDebugAttach _ARGS_((Fs_Device *devicePtr));
#endif

/*
 * devPrint.c
 */
#ifdef _DEVRAID
extern void PrintHandle _ARGS_((DevBlockDeviceHandle *handlePtr));
extern void PrintDevice _ARGS_((Fs_Device *devicePtr));
extern void PrintRequest _ARGS_((DevBlockDeviceRequest *requestPtr));
extern void PrintRaid _ARGS_((Raid *raidPtr));
extern void PrintTime _ARGS_((void));
#endif

/*
 * devRaidDisk.c (Put here to prevent circular reference.)
 */
#ifdef _DEVRAID
extern void Raid_ReportDiskAttachError _ARGS_((int type, int unit));
extern RaidDisk *Raid_MakeDisk _ARGS_((int col, int row, int type, int unit, int version, int numSector));
extern void Raid_FreeDisk _ARGS_((RaidDisk *diskPtr));
extern void Raid_FailDisk _ARGS_((Raid *raidPtr, int col, int row, int version));
extern void Raid_ReplaceDisk _ARGS_((Raid *raidPtr, int col, int row, int version, int type, int unit, int numValidSector));
#endif

/*
 * devRaidHardInit.c
 */
#ifdef _DEVRAID
extern void Raid_InitiateHardInit _ARGS_((Raid *raidPtr, int startStripe, int numStripe, void (*doneProc)(), ClientData clientData, int ctrlData));
#endif

/*
 * devRaidIOC.c
 */
#ifdef _DEVRAID
extern void Raid_ReportRequestError _ARGS_((RaidBlockRequest *reqPtr));
extern void Raid_ReportHardInitFailure _ARGS_((int stripeID));
extern void Raid_ReportParityCheckFailure _ARGS_((int stripeID));
extern void Raid_ReportReconstructionFailure _ARGS_((int col, int row));
#endif

/*
 * devRaidInitiate.c
 */
#ifdef _DEVRAID
extern void Raid_InitiateIORequests _ARGS_((RaidRequestControl *reqControlPtr, void (*doneProc)(), ClientData clientData));
extern void Raid_InitiateStripeIOs _ARGS_((Raid *raidPtr, int operation, unsigned firstSector, unsigned nthSector, Address buffer, void (*doneProc)(), ClientData clientData, int ctrlData));
extern void Raid_InitiateSimpleStripeIOs _ARGS_((Raid *raidPtr, int operation, unsigned firstSector, unsigned nthSector, Address buffer, void (*doneProc)(), ClientData clientData, int ctrlData));
#endif

/*
 * devRaidLock.c
 */
#ifdef _DEVRAID
extern void Raid_InitStripeLocks _ARGS_((void));
extern void Raid_SLockStripe _ARGS_((Raid *raidPtr, int stripe));
extern void Raid_CheckPoint _ARGS_((Raid *raidPtr));
extern void Raid_XLockStripe _ARGS_((Raid *raidPtr, int stripe));
extern void Raid_SUnlockStripe _ARGS_((Raid *raidPtr, int stripe));
extern void Raid_XUnlockStripe _ARGS_((Raid *raidPtr, int stripe));
extern void Raid_Lock _ARGS_((Raid *raidPtr));
extern void Raid_Unlock _ARGS_((Raid *raidPtr));
extern void Raid_BeginUse _ARGS_((Raid *raidPtr));
extern void Raid_EndUse _ARGS_((Raid *raidPtr));
extern void InitSema _ARGS_((Sema *semaPtr, char *name, int val));
extern void DownSema _ARGS_((Sema *semaPtr));
extern void UpSema _ARGS_((Sema *semaPtr));
#endif

/*
 * devRaidLog.c
 */
#ifdef _DEVRAID
extern void Raid_InitLog _ARGS_((Raid *raidPtr));
extern void Raid_EnableLog _ARGS_((Raid *raidPtr));
extern void Raid_DisableLog _ARGS_((Raid *raidPtr));
extern ReturnStatus Raid_ApplyLog _ARGS_((Raid *raidPtr));
extern ReturnStatus Raid_SaveDiskState _ARGS_((Raid *raidPtr, int col, int row, int type, int unit, int version, int numValidSector));
extern ReturnStatus Raid_SaveParam _ARGS_((Raid *raidPtr));
extern ReturnStatus Raid_SaveLog _ARGS_((Raid *raidPtr));
extern ReturnStatus Raid_SaveState _ARGS_((Raid *raidPtr));
extern ReturnStatus Raid_Configure _ARGS_((Raid *raidPtr, char *charBuf));
extern ReturnStatus Raid_RestoreState _ARGS_((Raid *raidPtr));
extern void Raid_MasterFlushLog _ARGS_((Raid *raidPtr));
extern void Raid_LogStripe _ARGS_((Raid *raidPtr, int stripeID));
#endif

/*
 * devRaidMap.c
 */
#ifdef _DEVRAID
extern void Raid_MapPhysicalToStripeID _ARGS_((Raid *raidPtr, int col, int row, unsigned sector, int *outStripeIDPtr));
extern void Raid_MapParity _ARGS_((Raid *raidPtr, unsigned sectorNum, int *outColPtr, int *outRowPtr, unsigned *sectorNumPtr));
extern void Raid_MapSector _ARGS_((Raid *raidPtr, unsigned sectorNum, int *outColPtr, int *outRowPtr, unsigned *sectorNumPtr));
#endif

/*
 * devRaidParityCheck.c
 */
#ifdef _DEVRAID
extern void Raid_InitiateParityCheck _ARGS_((Raid *raidPtr, int startStripe, int numStripe, void (*doneProc)(), ClientData clientData, int ctrlData));
#endif

/*
 * devRaidReconstruct.c
 */
#ifdef _DEVRAID
extern void Raid_InitiateReconstruction _ARGS_((Raid *raidPtr, int col, int row, int version, int numSector, int uSec, void (*doneProc)(), ClientData clientData, int ctrlData));
#endif

/*
 * strUtil.c
 */
extern char *ScanLine _ARGS_((char **ps1, char *s2));
extern char *ScanWord _ARGS_((char **ps1, char *s2));
