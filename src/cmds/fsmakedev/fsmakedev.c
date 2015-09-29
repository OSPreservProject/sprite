/* 
 * fsmakedev.c --
 *
 *	Make a device file.
 *
 * Copyright (C) 1986 Regents of the University of California
 * All rights reserved.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/src/cmds/fsmakedev/RCS/fsmakedev.c,v 1.3 90/09/11 13:49:11 jhh Exp $ SPRITE (Berkeley)";
#endif not lint

#include "sprite.h"
#include "option.h"
#include "fs.h"
#include "kernel/fs.h"
#include "stdio.h"

/*
 * Constants settable via the command line.
 */
char *deviceName;		/* Set to "/dev/rsd0" */
char	*permissionString = "0644";
int serverID = -1;
int deviceType = 0;
int deviceUnit = 0;

Option optionArray[] = {
    {OPT_STRING, "p", (Address)&permissionString,
	"Permission bits in octal (default is 0644)"},
    {OPT_INT, "s", (Address)&serverID,
	"Server ID, (default is localhost)"},
    {OPT_INT, "d", (Address)&deviceType,
	"Device type, (default is 0)"},
    {OPT_INT, "u", (Address)&deviceUnit,
	"Device unit, (default is 0)"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);


/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Creates a device file.  Each device has a serverID that
 *	identfies what host has the device, a device type, and
 *	a device unit number.  The serverID is -1 by default,
 *	which means it is a ``generic'' device that is found
 *	on the local host.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Calls Fs_MakeDevice.
 *
 *----------------------------------------------------------------------
 */
main(argc, argv)
    int argc;
    char *argv[];
{
    ReturnStatus status;	/* status of system calls */
    int flags = 0;
    Fs_Device device;
    int	permissions;
    int n;

    argc = Opt_Parse(argc, argv, optionArray, numOptions, 0);

    if (argc < 2) {
	fprintf(stderr, "Specify device file name.\n");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(FAILURE);
    } else {
	deviceName = argv[1];
    }
    n = sscanf(permissionString, " %o", &permissions);
    if (n != 1) {
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(FAILURE);
    }
    device.serverID = serverID;
    device.type = deviceType;
    device.unit = deviceUnit;
    device.data = (ClientData)0;
    status = Fs_MakeDevice(deviceName, &device, permissions);
    if (status != SUCCESS) {
	fprintf(stderr, "%s \"%s\" <%x,%x,%x>:", argv[0], deviceName,
				  device.serverID, device.type, device.unit);
	Stat_PrintMsg(status, "");
    }
    exit(status);
}
