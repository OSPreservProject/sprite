/* 
 * mon.c --
 *
 *     Routines to access the sun prom monitor.
 *
 * Copyright 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header$ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "machMon.h"
#include "machConst.h"
#include "machInt.h"
#include "devTimer.h"
#include "char.h"
#include "vmMachInt.h"
#include "mach.h"
#include "sys.h"

#ifdef SUN2
static	int	(*savedNmiVec)() = (int (*)()) 0;
#endif
extern	int	MachMonNmiNop();
static	Boolean	stoppedNMI = FALSE;


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonPutChar --
 *
 *     Call the monitor put character routine
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonPutChar(ch)
    int		ch;
{
    int		oldContext;

    if (!Char_IsAscii(ch)) {
	return;
    }
    DISABLE_INTR();
    oldContext = VmMachGetKernelContext();
    VmMachSetKernelContext(VMMACH_KERN_CONTEXT);
    romVectorPtr->putChar(ch);
    VmMachSetKernelContext(oldContext);
    ENABLE_INTR();
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonMayPut --
 *
 *     	Call the monitor put may put character routine.  This will return
 *	-1 if it couldn't put out the character.
 *
 * Results:
 *     -1 if couldn't emit the character.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

int
Mach_MonMayPut(ch)
    int		ch;
{
    int		oldContext;
    int		retValue;

    DISABLE_INTR();
    oldContext = VmMachGetKernelContext();
    VmMachSetKernelContext(VMMACH_KERN_CONTEXT);
    retValue = romVectorPtr->mayPut(ch);
    VmMachSetKernelContext(oldContext);
    ENABLE_INTR();
    return(retValue);
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonAbort --
 *
 *     	Abort to the monitor.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     None.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonAbort()
{
    int	oldContext;

    DISABLE_INTR();
    oldContext = VmMachGetKernelContext();
    VmMachSetKernelContext(VMMACH_KERN_CONTEXT);
    Mach_MonTrap(romVectorPtr->abortEntry);
    VmMachSetKernelContext(oldContext);
    ENABLE_INTR();
}

/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonReboot --
 *
 *     	Reboot the system.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *     System rebooted.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonReboot(rebootString)
    char	*rebootString;
{
    DISABLE_INTR();
    (void)VmMachGetKernelContext();
    VmMachSetKernelContext(VMMACH_KERN_CONTEXT);
    Mach_MonStartNmi();
    romVectorPtr->reBoot(rebootString);
    /*
     * If we reach this far something went wrong.
     */
    Sys_Panic(SYS_FATAL, "Mach_MonReboot: Reboot failed (I'm still alive aren't I?)\n");
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonStartNmi --
 *
 *	Allow the non-maskable (level 7) interrupts from the clock chip
 *	so the monitor can read the keyboard.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *	Non-maskable interrupts are allowed. On the Sun-2, the 
 *	trap vector is modified. 
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonStartNmi()
{
    if (stoppedNMI) {
#ifdef SUN2
	if (savedNmiVec != 0) {
	    machVectorTablePtr->autoVec[6] = savedNmiVec;
	}
#endif
#ifdef SUN3
	*Mach_InterruptReg |= MACH_ENABLE_LEVEL7_INTR;
#endif
	stoppedNMI = FALSE;
    }
}


/*
 * ----------------------------------------------------------------------------
 *
 * Mach_MonStopNmi --
 *
 * 	Disallow the non-maskable (level 7) interrupts.  
 *	On the Sun-2, this entails redirecting the interrupt. 
 *	On the Sun-3, the bit in the interrupt register for nmi's is 
 *	turned off.
 *
 * Results:
 *     None.
 *
 * Side effects:
 *	Non-maskable interrupts are disallowed. On the Sun-2, the trap 
 *	vector is modified.
 *
 * ----------------------------------------------------------------------------
 */

void
Mach_MonStopNmi()
{
    extern Boolean main_AllowNMI;

    /*
     * For debugging purposes, NMI's may need to be enabled.
     * If NMI's are disabled and the kernel goes into an infinite loop, 
     * then getting back to the monitor via L1-A is impossible 
     * However, if NMI's are enabled, level-7 interrupts are caused 
     * and it is possible that characters may be stolen by the monitor.
     * Also, spurious exceptions may occur.
     */
    if (!main_AllowNMI) {
	stoppedNMI = TRUE;
#ifdef SUN2
	savedNmiVec = machVectorTablePtr->autoVec[6];
	machVectorTablePtr->autoVec[6] = MachMonNmiNop;
#endif SUN2
#ifdef SUN3
	*Mach_InterruptReg &= ~MACH_ENABLE_LEVEL7_INTR;
#endif SUN3
    }
}
