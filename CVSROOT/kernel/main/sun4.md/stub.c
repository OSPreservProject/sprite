#include "sprite.h"
#include "mach.h"

int	debugOne = 0;
int	debugTwo = 0;
int	debugThree = 0;
int	debugFour = 0;

int	proc_PCBTable[100];

int *
Mach_GetEtherAddress(etherAddressPtr)
int *	etherAddressPtr;
{
	*etherAddressPtr = 0x08002000;
	etherAddressPtr++;
	*etherAddressPtr = 0xef480000;
	return etherAddressPtr;
}

int	*sched_MutexPtr;

VmMachTracePMEG()
{
    panic("VmMachTracePMEG called.\n");
}

Dump_Register_Events()
{
    panic("Dump_Register_Events called\n");
}

Dump_Show_Local_Menu()
{
    panic("Dump_Show_Local_Menu called.\n");
}

Prof_Init()
{
    panic("Prof_Init called.\n");
}

Prof_Start()
{
    panic("Prof_Start called.\n");
}

Prof_Disable()
{
    panic("Prof_Disable called.\n");
}

Prof_Profil()
{
    panic("Prof_Profil called.\n");
}

Prof_End()
{
    panic("Prof_End called.\n");
}

Prof_DumpStub()
{
    panic("Prof_DumpStub called.\n");
}

Mach_GetStackPointer()
{
    panic("Mach_GetStackPointer called.\n");
}

Mach_MonTrap()
{
    panic("Mach_MonTrap called.\n");
}

Mach_EncapState()
{
    panic("Mach_EncapState called.\n");
}

Mach_GetEncapState()
{
    panic("Mach_GetEncapState called.\n");
}

Mach_GetEncapSize()
{
    panic("Mach_GetEncapSize called.\n");
}

Mach_DeencapState()
{
    panic("Mach_DeencapState called.\n");
}

Mach_CanMigrate()
{
    panic("Mach_CanMigrate called.\n");
}
