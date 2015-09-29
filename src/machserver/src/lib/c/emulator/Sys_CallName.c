/* 
 * Sys_CallName.c --
 *
 *	Map sprited call numbers to names.
 *	XXX Should be machine-generated from spriteSrv.defs.  MIG will in 
 *	fact make a symbol table, but it puts it in the server side, 
 *	whereas we want it in the user side (or both).
 *
 * Copyright 1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that this copyright
 * notice appears in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/lib/c/emulator/RCS/Sys_CallName.c,v 1.2 92/06/10 15:21:43 kupfer Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include <sprite.h>
#include <sprited/sysCallNums.h>
#ifdef SPRITED
#include <user/sys.h>
#else
#include <sys.h>
#endif

static char *callNames[SYS_NUM_CALLS] = {
    "Test_PutDecimal",
    "Test_PutHex",
    "Test_PutOctal",
    "Test_PutMessage",
    "Test_PutString",
    "Test_GetString",		/* 5 */
    "Sys_Shutdown",
    "Sys_GetTimeOfDay",
    "Sys_Stats",
    "Sys_GetMachineInfo",
    "Sys_SetTimeOfDay",		/* 10 */
    NULL,
    NULL,
    NULL,
    NULL,
    "Vm_MapFile",		/* 15 */
    "Vm_CreateVA",
    "Vm_Cmd",
    NULL,
    NULL,
    NULL,			/* 20 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,			/* 25 */
    NULL,
    NULL,
    NULL,
    "Fs_Access",
    "Fs_AttachDisk",		/* 30 */
    "Fs_ChangeDir",
    "Fs_GetAttributes",
    "Fs_HardLink",
    "Fs_MakeDevice",
    "Fs_MakeDir",		/* 35 */
    "Fs_Open",
    "Fs_ReadLink",
    "Fs_RemoveDir",
    "Fs_Remove",
    "Fs_Rename",		/* 40 */
    "Fs_SetAttr",
    "Fs_SymLink",
    "Fs_Close",
    "Fs_CreatePipe",
    "Fs_GetAttributesID",	/* 45 */
    "Fs_GetNewID",
    "Fs_IOControl",
    "Fs_Read",
    "Fs_SetAttrID",
    "Fs_Write",			/* 50 */
    "Fs_Command",
    "Fs_SetDefPerm",
    "Fs_Select",
    "Fs_FileWriteBack",
    NULL,			/* 55 */
    NULL,
    NULL,
    NULL,
    "Net_InstallRoute",
    NULL,			/* 60 */
    "Proc_Fork",
    "Proc_GetIDs",
    "Proc_OldExecEnv",
    "Proc_RawExit",
    "Proc_Wait",		/* 65 */
    "Proc_Detach",
    "Proc_GetFamilyID",
    "Proc_GetGroupIDs",
    "Proc_GetPCBInfo",
    "Proc_SetFamilyID",		/* 70 */
    "Proc_SetGroupIDs",
    "Proc_SetIDs",
    "Proc_GetIntervalTimer",
    "Proc_SetIntervalTimer",
    "Proc_GetHostIDs",		/* 75 */
    "Proc_ExecEnv",
    NULL,
    NULL,
    NULL,
    NULL,			/* 80 */
    NULL,
    NULL,
    NULL,
    NULL,
    "Sig_Pause",		/* 85 */
    "Sig_Send",
    "Sig_SetAction",
    "Sig_SetHoldMask",
    "Sig_Return",
    "Sig_GetSignal",		/* 90 */
    "Test_PutTime",
    "Test_MemCheck",
    NULL,
    NULL,
    NULL,			/* 95 */
    "Test_Rpc",
    NULL,
    NULL,
    NULL
};


/*
 *----------------------------------------------------------------------
 *
 * Sys_CallName --
 *
 *	Map a Sprite call number to a name.
 *
 * Results:
 *	Returns a string corresponding to the number.  Returns NULL if
 *	there is no call for the given number.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Sys_CallName(number)
    int number;			/* the call number */
{
    if (number < 0 || number >= SYS_NUM_CALLS) {
	return NULL;
    }
    return callNames[number];
}
