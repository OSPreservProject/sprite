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

/*ARGSUSED*/
VmMachTracePMEG(pmeg)
int pmeg;
{
    panic("VmMachTracePMEG called.\n");
}

Mach_GetStackPointer()
{
    panic("Mach_GetStackPointer called.\n");
}
