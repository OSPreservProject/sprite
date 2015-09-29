/* 
 * sysCode.c --
 *
 *	Miscellaneous routines for the system.
 *
 * Copyright (C) 1985 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /user5/kupfer/spriteserver/src/sprited/sys/RCS/sysCode.c,v 1.9 92/04/29 21:47:28 kupfer Exp $ SPRITE (Berkeley)";
#endif not lint

#include <sprite.h>
#include <mach.h>
#include <mach_error.h>
#include <mach/mach_host.h>
#include <mach/machine.h>
#include <mach/message.h>
#include <stdlib.h>
#include <string.h>

#include <machCalls.h>
#include <net.h>
#include <rpc.h>
#include <user/sys.h>
#include <sys.h>
#include <sysInt.h>
#include <timer.h>
#include <utils.h>

/* 
 * Request ports for making privileged system requests.
 */
mach_port_t sys_PrivHostPort;
mach_port_t sys_PrivProcSetPort;

/* 
 * Hardware information obtained from the kernel.
 */
static struct host_basic_info hostInfo;
static int sysArchCode;		/* machine architecture code */
static int sysTypeCode;		/* CPU subtype code */
static int sysNumCpus;		/* number of online CPUs */

/* Forward references: */
static void SysGetHostInfo _ARGS_((void));


/*
 * ----------------------------------------------------------------------------
 *
 * Sys_Init --
 *
 *	Initializes system-dependent data structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	See the initialization routines called.  Also gets various 
 *	privileged Mach request ports.
 *
 * ----------------------------------------------------------------------------
 */
    
void 
Sys_Init()
{
    int retCode;		/* UNIX errno code */
    kern_return_t kernStatus;
    mach_port_t defaultSetName; /* default processor set name */

    SysInitSysCall();

    /* 
     * See Dev_Init for a discussion of how to get privileged host ports.
     */
    retCode = Utils_UnixPidToTask(UTILS_PRIV_HOST_PID, &sys_PrivHostPort);
    if (retCode != 0) {
	printf("Sys_Init: can't get privileged host port: %s\n",
	       strerror(retCode));
	exit(1);
    }

    kernStatus = processor_set_default(mach_host_self(), &defaultSetName);
    if (kernStatus != KERN_SUCCESS) {
	panic("Sys_Init: can't get default processor set name: %s\n",
	      mach_error_string(kernStatus));
    }
    kernStatus = host_processor_set_priv(sys_PrivHostPort, defaultSetName,
					 &sys_PrivProcSetPort);
    if (kernStatus != KERN_SUCCESS) {
	panic("Sys_Init: can't get processor set control port: %s\n",
	      mach_error_string(kernStatus));
    }

    SysGetHostInfo();
}


/*
 *----------------------------------------------------------------------
 *
 * SysGetHostInfo --
 *
 *	Query Mach about the current host.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Saves the information from Mach and sets the Sprite machine 
 *	architecture and type codes.
 *
 *----------------------------------------------------------------------
 */

static void
SysGetHostInfo()
{
    kern_return_t kernStatus;
    mach_msg_type_number_t infoCount;

    infoCount = HOST_BASIC_INFO_COUNT;
    kernStatus = host_info(mach_host_self(), HOST_BASIC_INFO,
			   (host_info_t)&hostInfo, &infoCount);
    if (kernStatus != KERN_SUCCESS) {
	panic("SysGetHostInfo failed: %s\n",
	      mach_error_string(kernStatus));
    }
    if (infoCount != HOST_BASIC_INFO_COUNT) {
	panic("SysGetHostInfo: expected %d words, got %d.\n",
	      HOST_BASIC_INFO_COUNT, infoCount);
    }

    switch (hostInfo.cpu_type) {

    case CPU_TYPE_MC68020:
	sysArchCode = SYS_SUN3;
	switch (hostInfo.cpu_subtype) {
	case CPU_SUBTYPE_SUN3_50:
	    sysTypeCode = SYS_SUN_3_50;
	    break;
	case CPU_SUBTYPE_SUN3_160:
	    sysTypeCode = SYS_SUN_3_160;
	    break;
	case CPU_SUBTYPE_SUN3_60:
	    sysTypeCode = SYS_SUN_3_60;
	    break;
	default:
	    printf("SysGetHostInfo: unknown CPU subtype (%d)\n",
		   hostInfo.cpu_subtype);
	    sysTypeCode = 0;
	    break;
	}
	break;

    case CPU_TYPE_MIPS:
	/* XXX Some day we should distinguish 3100's from 5000's. */
	sysArchCode = SYS_DS3100;
	break;

    default:
	panic("SysGetHostInfo: unrecognized cpu type (%d)\n",
	      hostInfo.cpu_type);
	break;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetHostId --
 *
 *	This returns the Sprite Host Id for the system.  This Id is
 *	guaranteed to be unique accross all Sprite Hosts participating
 *	in the system.  This is plucked from the RPC system now,
 *	but perhaps should be determined from the filesystem.
 *
 * Results:
 *	The Sprite Host Id.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int
Sys_GetHostId()
{
    return(rpc_SpriteID);
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetMachineArch --
 *
 *	Get the machine architecture code.
 *
 * Results:
 *	Returns the code for the machine architecture (sun3, ds3100, etc.).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sys_GetMachineArch()
{
    return sysArchCode;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetMachineType --
 *
 *	Get the machine type.
 *
 * Results:
 *	Returns the machine implementation type number.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sys_GetMachineType()
{
    return sysTypeCode;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_GetNumProcessors --
 *
 *	Get the number of running processors.
 *
 * Results:
 *	Returns the count of available processors from that Mach gave us.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Sys_GetNumProcessors()
{
    return sysNumCpus;
}


/*
 *----------------------------------------------------------------------
 *
 * Sys_HostPrint --
 *
 *	Print out a statement concerning a host.  This maps to a
 *	string hostname if possible, and prints out the message.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	printf.
 *
 *----------------------------------------------------------------------
 */

static int lastDay[13] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

void
Sys_HostPrint(spriteID, string)
    int spriteID;
    char *string;
{
    char hostName[128];
    Time time;
    int offset;
    int seconds;
    Boolean dstFlag;
    Time_Parts timeParts;

    Timer_GetTimeOfDay(&time, &offset, &dstFlag);
    seconds = time.seconds + offset * 60;
    Time_ToParts(seconds, FALSE, &timeParts);
    /*
     * Until Time_ToParts makes the month count from 1, not zero.
     */
    timeParts.month += 1;
    /*
     * Gag, my own (simplified) daylight savings correction.
     */
    if (dstFlag) {
	if ((timeParts.month >= 4) &&	/* All of April */
	    (timeParts.month <= 10)) {	/* thru October */
	    timeParts.hours++;
	    if (timeParts.hours >= 24) {
		timeParts.hours = 0;
		timeParts.dayOfMonth++;
		if (timeParts.dayOfMonth > lastDay[timeParts.month]) {
		    timeParts.month++;
		    timeParts.dayOfMonth = 1;
		}
	    }
	}
    }
    printf("%d/%d/%d %d:%02d:%02d ", timeParts.month, timeParts.dayOfMonth,
	    timeParts.year, timeParts.hours, timeParts.minutes,
	    timeParts.seconds);

    Net_SpriteIDToName(spriteID, 128, hostName);
    if (*hostName == '\0') {
	printf("Sprite Host <%d> %s", spriteID, string);
    } else {
	printf("%s (%d) %s", hostName, spriteID, string);
    }
}
