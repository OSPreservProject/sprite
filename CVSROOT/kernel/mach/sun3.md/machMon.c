/* 
 * machMon.c --
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
#include "ctype.h"
#include "mach.h"
#include "vmMach.h"
#include "sys.h"
#ifdef sun4c
#include "stdio.h"
#include "string.h"
#endif

extern int VmMachGetKernelContext _ARGS_ ((void));
extern void VmMachSetKernelContext _ARGS_((int value));

#ifdef sun2
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

    if (!isascii(ch)) {
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
    Mach_MonTrap((Address) (romVectorPtr->abortEntry));
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
    panic("Mach_MonReboot: Reboot failed (I'm still alive aren't I?)\n");
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
#ifdef sun2
	if (savedNmiVec != 0) {
	    machVectorTablePtr->autoVec[6] = savedNmiVec;
	}
#endif
#ifdef sun3
	*Mach_InterruptReg |= MACH_ENABLE_LEVEL7_INTR;
#endif
#ifdef sun4
	*Mach_InterruptReg |= MACH_ENABLE_ALL_INTERRUPTS;
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
#ifdef sun2
	savedNmiVec = machVectorTablePtr->autoVec[6];
	machVectorTablePtr->autoVec[6] = MachMonNmiNop;
#endif
#ifdef sun3
	*Mach_InterruptReg &= ~MACH_ENABLE_LEVEL7_INTR;
#endif
#ifdef sun4
	*Mach_InterruptReg &= ~MACH_ENABLE_ALL_INTERRUPTS;
#endif
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Mach_MonTraverseDevTree --
 *
 *	Traverse the device tree, and call the given function
 *	for each node found.  To start at the root of the tree,
 *	the node argument should be set to 0.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#ifdef sun4c
void
Mach_MonTraverseDevTree(node, func, clientData)
    unsigned	int	node;
    int		(*func)();
    Address     clientData;
{
    unsigned	int		newNode;
    char 	name[64];
    int		length;
    struct	config_ops	*configPtr;

    configPtr = romVectorPtr->v_config_ops;
    if (node == 0) {
	node = configPtr->devr_next(0);
    }
    while (node != 0) {
	length = configPtr->devr_getproplen(node, "name");
	if (length > 0) {
	    if (length > sizeof (name)) {
		panic("Mach_MonTraverseDevTree: buffer too small.\n");
	    }
	    configPtr->devr_getprop(node, "name", name);
	    if ((*func)(node, name, clientData))
		return;
	}
	if (newNode = configPtr->devr_child(node)) {
	    Mach_MonTraverseDevTree(newNode, func, clientData);
	}
	node = configPtr->devr_next(node);
    }
}


static int
PrintNode(node, name, clientData)
    unsigned	int	node;
    char		*name;
    void		*clientData;
{
    struct	config_ops	*configPtr;
    char	*prop;

    configPtr = romVectorPtr->v_config_ops;
    prop = 0;
    while (1) {
	prop = (char *)configPtr->devr_nextprop(node, prop);
	if (prop && *prop) {
	    printf("%s: %s\n", name, prop);
	} else {
	    break;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_MonTraverseAndPrintDevTree --
 *
 *	Traverse the device tree, and display the attributes
 *	of each node found.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Mach_MonTraverseAndPrintDevTree()
{
    Mach_MonTraverseDevTree(0, PrintNode, 0);
}


struct ConfigBuf {
    char	*name;
    char	*attr;
    char	*buf;
    int		buflen;
    int		length;
};

static int
CheckNode(node, name, clientData)
    unsigned	int		node;
    char			*name;
    struct	ConfigBuf	*clientData;
{
    int		length;
    struct	config_ops	*configPtr;

    configPtr = romVectorPtr->v_config_ops;
    if (strcmp(name, clientData->name) == 0
	    || strcmp(clientData->name, "*") == 0) {
	length = configPtr->devr_getproplen(node, clientData->attr);
	if (length <= 0) {
	    return 0;
	} else if (length > clientData->buflen) {
	    printf("Data size (%d) is greater than buffer size (%d)\n",
		length, clientData->buflen);
	} else {
	    configPtr->devr_getprop(node, clientData->attr, clientData->buf);
	}
	clientData->length = length;
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Mach_MonSearchProm --
 *
 *	Search through the prom devices to find out system attributes.
 *	If attr is given, and name is "*", the info will be retrieved
 *	from the first node with that attribute, otherwise the name
 *	must match exactly.
 *
 * Results:
 *	length of info retrieved.
 *
 *----------------------------------------------------------------------
 */
int
Mach_MonSearchProm(name, attr, buf, buflen)
    char	*name;
    char	*attr;
    char	*buf;
    int		buflen;
{
    struct ConfigBuf data;

    data.length = 0;
    data.name = name;
    data.attr = attr;
    data.buf = buf;
    data.buflen = buflen;
    Mach_MonTraverseDevTree(0, CheckNode, &data);
    return data.length;
}
#endif /* sun4c */
