/* 
 * vmelinkcmd.c --
 *
 *	Program for dealing with the Ultranet VME adapter board..
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#ifndef lint
static char rcsid[] = "$Header: /sprite/lib/forms/RCS/proto.c,v 1.3 90/01/12 12:03:36 douglis Exp $ SPRITE (Berkeley)";
#endif /* not lint */

#include "sprite.h"
#include "option.h"
#include <stdio.h>
#include <sys/file.h>
#include <fs.h>
#include <status.h>
#include <fmt.h>
#include <ctype.h>
#include <dev/xbus.h>

#define	SRC_BUFFER	0x1
#define	DST_BUFFER	0x2

/*
 * Variables settable via the command line.
 */
char		*device = "/dev/xbus0";
char*		debugString = NULL;	/* turn debugging on or off? */
Boolean		resetBoards = FALSE;	/* reset the boards? */
char*		rregStr = NULL;
char*		wregStr = NULL;
unsigned int	regValue = 0;
unsigned int	xorSize = 0x1000;
unsigned int	xorDest = 0x0;
unsigned int	xorArray[DEV_XBUS_MAX_XOR_BUFS + 4];
Boolean		doXor = FALSE;
int		whichBuffer = 0;
unsigned int	bufAddr = 0;
unsigned int	bufSize = 0x100;
char*		parityString = NULL;
char		*myname = "xbuscmd";
int		readXorSrcBuffers ();
char*		vmeBoardStr = NULL;
char*		useIntrStatusStr = NULL;

unsigned int	stripeUnit = 0x8000;
unsigned int	testStart = 0;
unsigned int	testEnd = 0;
unsigned int	numDisks = 2;
unsigned int	maxNumReq = 10;
Boolean		testStop = FALSE;
Boolean		testPrintStats = FALSE;

char	statString[DEV_XBUS_TEST_MAX_STAT_STR+10];

Option optionArray[] = {
    {OPT_DOC, NULL, NULL, "Usage: xbuscmd [options]"},
    {OPT_TRUE, "reset", (Address)&resetBoards,
	 "Reset the xbus boards."},
    {OPT_STRING, "debug", (Address)&debugString,
	 "Turn debugging ON or OFF."},
    {OPT_STRING, "parity", (Address)&parityString,
	 "Turn parity checking ON or OFF"},
    {OPT_STRING, "vmeBoard", (Address)&vmeBoardStr,
	 "VME link board to lock for accesses."},
    {OPT_STRING, "device", (Address)&device,
	 "Set the xbus device."},
    {OPT_STRING, "rreg", (Address)&rregStr,
	 "Read an xbus register."},
    {OPT_STRING, "wreg", (Address)&wregStr,
	 "Write an xbus register."},
    {OPT_INT, "value", (Address)&regValue,
	 "Value to use for writing to a register."},
    {OPT_CONSTANT(SRC_BUFFER), "send", (Address)&whichBuffer,
	 "Send data over the HIPPI-S bus"},
    {OPT_CONSTANT(DST_BUFFER), "recv", (Address)&whichBuffer,
	 "Receive data over the HIPPI-D bus"},
    {OPT_INT, "bufAddr", (Address)&bufAddr,
	 "XBUS address of the HIPPI buffer"},
    {OPT_INT, "bufSize", (Address)&bufSize,
	 "Size (in bytes) of the HIPPI buffer"},
    {OPT_INT, "xorSize", (Address)&xorSize,
	 "Size (in bytes) of the XOR buffers."},
    {OPT_INT, "xorDest", (Address)&xorDest,
	 "XOR destination buffer (in XBUS address space)."},
    {OPT_GENFUNC, "xorSrc", (Address)readXorSrcBuffers,
	 "XOR source buffer(s) (in XBUS address space)."},
    {OPT_STRING, "useIntrStatus", (Address)&useIntrStatusStr,
	 "Turn use of interrupt status ON or OFF."},
    {OPT_INT, "testStart", (Address)&testStart,
	 "Start of XBUS memory for overall test."},
    {OPT_INT, "testEnd", (Address)&testEnd,
	 "End of XBUS memory for overall test."},
    {OPT_INT, "stripeUnit", (Address)&stripeUnit,
	 "Stripe unit size for XBUS overall test."},
    {OPT_INT, "numDisks", (Address)&numDisks,
	 "Number of disks to use for XBUS overall test."},
    {OPT_INT, "maxNumReq", (Address)&maxNumReq,
	 "Maximum number of requests to do."},
    {OPT_TRUE, "testStop", (Address)&testStop,
	 "Stop the XBUS overall test."},
    {OPT_TRUE, "testStats", (Address)&testPrintStats,
	 "Print the statistics for the last XBUS overall test."},
};

/*----------------------------------------------------------------------
 *
 * readXorSrcBuffers --
 *
 *	Option parser to read an unknown number of source buffers from
 *	the command line.  It will allow more than one buffer to go
 *	with the -xorSrc option.
 *
 *----------------------------------------------------------------------
 */
static
int
readXorSrcBuffers (optStr, argc, argv)
char*	optStr;
int	argc;
char**	argv;
{
    int		numBufs, i;
    char*	c;

    for (numBufs = 0; numBufs < argc; numBufs++) {
	xorArray[numBufs+3] = strtoul (argv[numBufs], &c, 0);
	if (c == argv[numBufs]) {
	    break;
	}
    }
    if (numBufs == 0) {
	printf ("%s: must have at least one source buffer for %s.\n",
		myname, optStr);
	Opt_PrintUsage(myname, optionArray, Opt_Number (optionArray));
	exit(1);
    }

    xorArray[0] = numBufs;
    doXor = TRUE;
    for (i = numBufs; i < argc; i++) {
	argv[i-numBufs] = argv[i];
    }
    return (argc - numBufs);
}

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	Parse the arguments and call the correct procedure.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The state of the xbus board may be modified.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int		argc;
    char	*argv[];
{
    int			argsLeft;
    int			fd = -1;
    unsigned int	regNum;
    char		*c;
    ReturnStatus	status;
    extern int		errno;

    argsLeft = Opt_Parse(argc, argv, optionArray, Opt_Number (optionArray), 0);
    if ((argsLeft > 1) || (argc < 2)) {
	Opt_PrintUsage(myname, optionArray, Opt_Number (optionArray));
	exit(1);
    }

    if ((wregStr != NULL) && (rregStr != NULL)) {
	printf ("%s: may not both read and write a register.\n");
	Opt_PrintUsage(myname, optionArray, Opt_Number (optionArray));
	exit(1);
    } else if (wregStr != NULL) {
	regNum = strtoul (wregStr, &c, 0);
    } else if (rregStr != NULL) {
	regNum = strtoul (rregStr, &c, 0);
    }

    if (((wregStr != NULL) + (rregStr != NULL) + (whichBuffer != 0) +
	 (parityString != NULL) + (doXor == TRUE) + (testStart != testEnd) +
	 (vmeBoardStr != NULL) + (useIntrStatusStr != NULL) +
	 (testStop == TRUE) + (testPrintStats == TRUE)) > 1) {
	printf ("%s: one thing at a time, please!\n", myname);
	Opt_PrintUsage(myname, optionArray, Opt_Number (optionArray));
	exit(1);
    }	

    if ((fd = open (device, O_RDONLY, 0666)) < 0) {
	printf ("%s: Error 0x%x opening %s\n", myname, errno, device);
	Opt_PrintUsage(myname, optionArray, Opt_Number (optionArray));
	exit (1);
    }

    if (debugString != NULL) {
	if (!strcasecmp (debugString, "on")) {
	    status = Fs_IOControl (fd, IOC_XBUS_DEBUG_ON, 0, NULL, 0, NULL);
	} else if (!strcasecmp (debugString, "off")) {
	    status = Fs_IOControl (fd, IOC_XBUS_DEBUG_OFF,0, NULL, 0, NULL);
	} else {
	    printf ("%s: -debug requires ON or OFF, not %s!\n", myname,
		    debugString);
	    exit (1);
	}
	if (status != SUCCESS) {
	    printf ("%s: modifying debug flag returned 0x%x\n",myname,status);
	}
    }

    if (resetBoards) {
	status = Fs_IOControl (fd, IOC_XBUS_RESET, 0, NULL, 0, NULL);
	if (status != SUCCESS) {
	    printf ("%s: XBUS_RESET returned 0x%x\n", myname, status);
	}
    }
    if (vmeBoardStr != NULL) {
	int	vmeBoard;

	vmeBoard = strtol (vmeBoardStr, NULL, 0);
	status = Fs_IOControl (fd, IOC_XBUS_LOCK_VME, sizeof (vmeBoard),
			       (Address)&vmeBoard, 0, NULL);
	if (status != SUCCESS) {
	    printf ("%s: couldn't set VME board to lock to %d.\n",
		    myname, vmeBoard);
	}
    } else if (useIntrStatusStr != NULL) {
	int		oldUse, newUse;
	if (!strcasecmp (useIntrStatusStr, "on")) {
	    newUse = TRUE;
	} else if (!strcasecmp (useIntrStatusStr, "off")) {
	    newUse = FALSE;
	} else {
	    printf ("%s: -useIntrStatus requires ON or OFF, not %s!\n", myname,
		    useIntrStatusStr);
	    exit (1);
	}
	status = Fs_IOControl (fd, IOC_XBUS_USE_INTR_STATUS, sizeof (newUse),
			       (Address)&newUse, sizeof (oldUse),
			       (Address)&oldUse);
	if (status != SUCCESS) {
	    printf ("%s: turning interrupt status use %s returned 0x%x\n",
		    myname, useIntrStatusStr, status);
	} else {
	    printf ("%s: interrupt status use was %s, is now %s.\n", myname,
		    oldUse ? "ON" : "OFF", useIntrStatusStr);
	}
    } else if (parityString != NULL) {
	int	parity;

	if (!strcasecmp (parityString, "on")) {
	    parity = TRUE;
	} else if (!strcasecmp (parityString, "off")) {
	    parity = FALSE;
	} else {
	    printf ("%s: -parity requires ON or OFF, not %s!\n", myname,
		    parityString);
	    exit (1);
	}
	status = Fs_IOControl (fd, IOC_XBUS_CHECK_PARITY, sizeof (parity),
			       (Address)&parity, 0, NULL);
	if (status != SUCCESS) {
	    printf ("%s: turning parity %s returned 0x%x\n", myname,
		    parityString, status);
	}
    } else if ((rregStr != NULL) || (wregStr != NULL)) {
	DevXbusRegisterAccess acc;
	int	iocNum;

	acc.registerNum = regNum;
	acc.value = regValue;
	iocNum = (rregStr != NULL) ? IOC_XBUS_READ_REG : IOC_XBUS_WRITE_REG;
	status = Fs_IOControl (fd, iocNum, sizeof(acc),&acc,sizeof(acc),&acc);
	if (status != SUCCESS) {
	    printf ("%s: register access returned 0x%x\n", myname, status);
	} else {
	    printf ("Register 0x%03x = 0x%08x\n", acc.registerNum, acc.value);
	}
    } else if ((testStart != 0) && (testEnd != 0)) {
	unsigned int	testArgs[5];

	testArgs[0] = testStart;
	testArgs[1] = testEnd;
	testArgs[2] = stripeUnit;
	testArgs[3] = numDisks;
	testArgs[4] = maxNumReq;
	status = Fs_IOControl (fd, IOC_XBUS_TEST_START, sizeof (testArgs),
			       testArgs, 0, NULL);
	if (status != SUCCESS) {
	    printf ("%s: unable to start XBUS test (status = 0x%x).\n", myname,
		    status);
	}
    } else if (testStop) {
	status = Fs_IOControl (fd, IOC_XBUS_TEST_STOP, 0, NULL, 0, NULL);
	if (status != SUCCESS) {
	    printf ("%s: unable to stop XBUS test (status = 0x%x).\n", myname,
		    status);
	}
    } else if (testPrintStats) {
	status = Fs_IOControl (fd, IOC_XBUS_TEST_STATS, 0, NULL,
			       sizeof (statString), statString);
	if (status != SUCCESS) {
	    printf ("%s: unable to print XBUS test stats (status = 0x%x).\n",
		    myname, status);
	} else {
	    printf ("Here is the statistics string:\n");
	    puts (statString);
	}
    } else if (whichBuffer != 0) {
	DevXbusRegisterAccess acc;
	unsigned int reg;

	reg = (whichBuffer == SRC_BUFFER) ? DEV_XBUS_REG_HIPPIS_CTRL_FIFO :
	    DEV_XBUS_REG_HIPPID_CTRL_FIFO;
	acc.registerNum = reg;
	acc.value = bufSize >> 2;
	status = Fs_IOControl (fd, IOC_XBUS_WRITE_REG, sizeof(acc),
			       &acc, sizeof(acc), &acc);
	if (status != SUCCESS) {
	    printf ("%s: unable to push extent (error 0x%x).\n",myname,status);
	} else {
	    acc.registerNum = reg;
	    acc.value = bufAddr >> 2;
	    status = Fs_IOControl (fd, IOC_XBUS_WRITE_REG, sizeof(acc),&acc,
				   sizeof(acc), &acc);
	    if (status != SUCCESS) {
		printf ("%s: unable to push buffer address (error 0x%x).\n",
			myname, status);
	    } else {
		printf ("%s: queued 0x%x byte buffer at 0x%x for %s.\n",
			myname, bufSize, bufAddr,
			((whichBuffer == SRC_BUFFER) ? "hippis" : "hippid"));
	    }
	}
    } else if (doXor) {
	int	nbufs = xorArray[0];

	xorArray[1] = xorSize;
	xorArray[2] = xorDest;
	status = Fs_IOControl (fd, IOC_XBUS_DO_XOR,
			       (nbufs + 3) * sizeof (xorArray[0]),
			       xorArray, 0, NULL);
	if (status != SUCCESS) {
	    printf ("%s: doing XOR returned 0x%x\n", myname, status);
	}
    }

    close (fd);
}
