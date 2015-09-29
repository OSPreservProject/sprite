/* 
 * hostname.c --
 *
 *	A very simple program that prints out the textual name of
 *	the host on which it is running.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/hostname/RCS/hostname.c,v 1.6 91/11/29 16:43:21 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <option.h>
#include <host.h>
#include <sys.h>

/*
 * This is for the sprite Proc_GetIDs call, until it is added to the unix
 * library.
 */
#include <sprite.h>
#include <status.h>
#include <proc.h>

int getIDs = 0;
int virtual = 0;
int physical = 0;
int machType = -1;

Option optionArray[] = {
    {OPT_TRUE, "v", (char *)&virtual,
	 "Print the name of the host on which the process is effectively executing (its 'home node' -- DEFAULT)."},
    {OPT_TRUE, "p", (char *)&physical,
	 "Print the name of the host on which the process is physically executing ."},
    {OPT_TRUE, "i", (char *)&getIDs,
	 "Print hostIDs instead of host names."},
    {OPT_TRUE, "type", (char *)&machType,
	 "Print machine type for host."},
};

static void PrintInfo();

main(argc, argv)
    int argc;
    char **argv;
{
    int spriteID;
    char *myName;
    int virtualHost;
    int physicalHost;
    int status = SUCCESS;

    argc = Opt_Parse(argc, argv, optionArray, Opt_Number(optionArray), 
		     OPT_ALLOW_CLUSTERING);

    myName = argv[0];
    
    if (argc > 1) {
	fprintf(stderr, "%s: warning: extra arguments; assuming old format.\n",
		myName);
	getIDs = 1;
    }
    if (machType != -1) {
	Sys_MachineInfo	info;

	status = Sys_GetMachineInfo(sizeof (info), &info);
	switch (info.architecture) {
	case SYS_SUN2:
	    printf("sun2\n");
	    break;
	case SYS_SUN3:
	    switch (info.type) {
	    case SYS_SUN_3_75:
		printf("sun3/75\n");
		break;
/*
	    Duplicate to 3/75!
	    case SYS_SUN_3_160:
		printf("sun3/160\n");
		break;
*/
	    case SYS_SUN_3_50:
		printf("sun3/50\n");
		break;
	    case SYS_SUN_3_60:
		printf("sun3/60\n");
		break;
	    default:
		printf("sun3\n");
		break;
	    }
	    break;
	case SYS_SUN4:
	    switch (info.type & SYS_SUN_ARCH_MASK) {
	    case SYS_SUN_4_C:
		printf("sun4c\n");
		break;
	    default:
		printf("sun4\n");
		break;
	    }
	    break;
	case SYS_DS3100:
	    printf("ds3100\n");
	    break;
	case SYS_SYM:
	    printf("symmetry\n");
	    break;
	case SYS_MICROVAX_2:
	    printf("microvax2\n");
	    break;
	case SYS_SPUR:
	    printf("spur\n");
	    break;
	case SYS_DS5000:
	    printf("ds5000\n");
	    break;
	default:
	    printf("unknown\n");
	    status = 1;
	    break;
	}
	exit(status);
    }

    if (!virtual & !physical) {
	virtual = 1;
    }
    
    status = Proc_GetHostIDs(&virtualHost, &physicalHost);
    if (status != SUCCESS) {
	errno = Compat_MapCode(status);
	perror("Proc_GetHostIDs");
	exit(1);
    }
    if (virtual) {
	PrintInfo(virtualHost, physical ? "virtual" : (char *) NULL);
    }
    if (physical) {
	PrintInfo(physicalHost, virtual ? "physical" : (char *) NULL);
    }
    exit(0);
}



/*
 *----------------------------------------------------------------------
 *
 * PrintInfo --
 *
 *	Print the information contained in a Host_Entry structure, as
 *	specified by command line arguments, and with any other string
 * 	specified by the caller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Data is written to stdout.
 *
 *----------------------------------------------------------------------
 */

static void
PrintInfo(spriteID, prefix)
    int spriteID;
    char *prefix;
{
    extern int errno;
    Host_Entry *hostPtr;

    hostPtr = Host_ByID(spriteID);
    if (hostPtr == (Host_Entry *) NULL) {
	perror("Host_ByID");
	exit(1);
    }
    if (prefix) {
	printf("%s: ", prefix);
    }
    if (getIDs) {
	printf("%d\n", hostPtr->id);
    } else {
	printf("%s\n", hostPtr->name);
    }
}
