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
    return "Things are bad.\n";
}

VmMach_UnmapIntelPage()
{
}

DevNetEtherHandler()
{
}

Vm_CopyOut()
{
}

Vm_StringNCopy()
{
}

Vm_CopyIn()
{
}

Sched_ContextSwitchInt()
{
}

int	vm_Tracing = 0;

Vm_StoreTraceTime()
{
}

Dev_GatherDiskStats()
{
}

Sched_GatherProcessInfo()
{
}

Vm_BootAlloc()
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

Vm_PinUserMem()
{
}

Vm_UnpinUserMem()
{
}

Rpc_Reply()
{
}

Address	vmMemEnd;

int	vmKernMemSize = 4096 * 1024;

Address
Vm_RawAlloc(numBytes)
    int	numBytes;
{
    Address	retAddr;

    retAddr = vmMemEnd;

    retAddr = (Address) (((unsigned) retAddr + 7) & ~7);
    vmMemEnd += (numBytes + 7) & ~7;

    if (vmMemEnd > (Address) (mach_KernStart + vmKernMemSize)) {
	panic("Ran out of memory!\n");
    }

    return retAddr;
}

extern	unsigned	int	end;

void
Vm_BootInit()
{
    vmMemEnd = (Address) &end;
    vmMemEnd = (Address) (((int) vmMemEnd + 7) & ~7);
    return;
}
