#include "sprite.h"
#include "mach.h"

Rpc_Dispatch()
{
}

Sys_HostPrint()
{
}

Proc_CallFunc()
{
}

Dev_SyslogReturnBuffer()
{
}

Dev_SyslogDebug()
{
}

Mach_MonReboot()
{
}
int	debugOne = 0;
int	debugTwo = 0;
int	debugThree = 0;
int	debugFour = 0;

int	proc_PCBTable[100];

int	proc_RunningProcesses = 1;

int *
Mach_GetEtherAddress(etherAddressPtr)
int *	etherAddressPtr;
{
	*etherAddressPtr = 0x08002000;
	etherAddressPtr++;
	*etherAddressPtr = 0xef480000;
	return etherAddressPtr;
}

char	*
SpriteVersion()
{
    return "Sun4 kernel.\n";
}

#ifdef NOTDEF
VmMach_UnmapIntelPage()
{
}
#endif NOTDEF

DevNetEtherHandler()
{
}

#ifdef NOTDEF
Vm_CopyOut()
{
}

Vm_StringNCopy()
{
}

Vm_CopyIn()
{
}
#endif NOTDEF

Sched_ContextSwitchInt()
{
}

#ifdef NOTDEF
Vm_StoreTraceTime()
{
}
#endif NOTDEF

Dev_GatherDiskStats()
{
}

Sched_GatherProcessInfo()
{
}

int	*sched_MutexPtr;

Sched_MoveInQueue()
{
}

Mach_CheckSpecialHandling()
{
}

Rpc_Call()
{
}

Rpc_Reply()
{
}

Mem_Bin()
{
}

Proc_PushLockStack()
{
}
Proc_RemoveFromLockStack()
{
}
Proc_NeverMigrate()
{
}
VmMachTracePMEG()
{
}
Fs_GetEncapSize()
{
}
Fs_Close()
{
}
Fs_DeencapStream()
{
}
Fs_StreamCopy()
{
}
Fs_EncapStream()
{
}
Fs_GetPageFromFS()
{
}
Sig_SendProc()
{
}
Sig_Send()
{
}
Fs_WaitForHost()
{
}
Fs_GetSegPtr()
{
}
Fs_GetFileHandle()
{
}
Fs_GetFileName()
{
}
Sys_GetHostId()
{
}
Fs_Read()
{
}
Fs_Open()
{
}
Fs_PageRead()
{
}
Fs_PageCopy()
{
}
Proc_GetEffectiveProc()
{
}
Fs_CacheBlocksUnneeded()
{
}
Fs_PageWrite()
{
}
Fs_Remove()
{
}
Proc_Unlock()
{
}
Proc_Lock()
{
}
Fs_Write()
{
}

_free()
{
}
