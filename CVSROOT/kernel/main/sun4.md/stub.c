#include "sprite.h"
#include "mach.h"

int	debugOne = 0;
int	debugTwo = 0;
int	debugThree = 0;
int	debugFour = 0;

#ifdef sun4c
/*
 * Various prom things called in these routines don't exist on the sun4c.
 */
int
Test_GetLine()
{
}

int
Test_GetChar()
{
}
#endif

Mach_GetLastSyscall()
{
}

Proc_RpcMigInfo()
{
}
Proc_RpcMigInit()
{
}

VmMachTracePMEG()
{
    panic("VmMachTracePMEG called.\n");
}

Mach_GetStackPointer()
{
    panic("Mach_GetStackPointer called.\n");
}

ReturnStatus
Mach_EncapState()
{
    panic("Mach_EncapState called.\n");
}

ReturnStatus
Mach_GetEncapState()
{
    panic("Mach_GetEncapState called.\n");
}

ReturnStatus
Mach_GetEncapSize()
{
    panic("Mach_GetEncapSize called.\n");
}

ReturnStatus
Mach_DeencapState()
{
    panic("Mach_DeencapState called.\n");
}

Boolean
Mach_CanMigrate()
{
    panic("Mach_CanMigrate called.\n");
}

Vm_FreezeSegments()
{
}

Vm_MigrateSegment()
{
}

Vm_ReceiveSegmentInfo()
{
}
