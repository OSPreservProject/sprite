/* 
 * ultracmd.c --
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
#include "net.h"
#include "netUltra.h"
#include "/sprite/src/lib/include/dev/ultra.h"
#include "/sprite/src/lib/include/dev/hppi.h"
#include <status.h>
#include <fmt.h>
#include <ctype.h>
#include "hppicmd.h"

/*
 * The ucode file is in Intel format, which we don't have a constant for
 * at the moment so borrow the VAX format.
 */

#define CODE29K_FORMAT	FMT_SPARC_FORMAT

/*
 * Variables settable via the command line.
 */

Boolean	download = FALSE;	/* Download ucode to the adapter. */
char	*dir = "/sprite/lib/hppi/code";  /* Directory containing ucode. */
char	*file = NULL;		/* Ucode file to download. */
Boolean reset = FALSE;		/* Reset the adapter. */
Boolean getInfo = FALSE;	/* Get the adapter info. */
char	*dev = "/dev";		/* Directory containing the device. */
Boolean diag = FALSE;		/* Run diagnostic tests. */
Boolean extDiag = FALSE;	/* Run extended diagnostic tests. */
Boolean	external = FALSE;	/* Use external loopback when running
				 * extended diagnostic tests. */
char	*debug = NULL;		/* Set debugging output. */
Boolean init = FALSE;		/* Send init command to adapter. */
Boolean start = FALSE;		/* Send start command to the adapter. */
char	*address = NULL;	/* Set adapter's Ultranet address. */
char	*dsnd = NULL;		/* Send a datagram to the given host. */
Boolean	drcv = FALSE;		/* Receive a datagram */
int	count = 1;		/* Number of times to send the datagram. */
int	repeat = 1;		/* Number of times to repeat the send test.*/
int	size = 0;		/* Size of the datagram to send. */
char    *echo = NULL;		/* Host should echo datagrams back to
				 * sender.  Use for receiver of dsnd option. */
char	*trace = FALSE;		/* Set tracing of activity. */
char	*source = FALSE;	/* Send a steady stream of datagrams to
				 * the given host. */
char	*sink = FALSE;		/* Toggle sink of incoming datagrams. */
char	*stat = FALSE;		/* Manipulate collection of statistics. */
int	map = -1;		/* Set mapping threshold. */
char	*board = "DST";		/* SRC, DST, or IOP board? */
int	readReg = -1;		/* register to read */
int	writeReg = -1;		/* register to write */
int	writeValue = 0;		/* value to write into the above register */
int	tsap = 0;		/* TSAP value to set */
int	bcopy = -1;
int	sg = -1;		
int	noisyLoader = FALSE;
int	boardFlags = -1;
Boolean hardReset = FALSE;
Boolean setup = FALSE;
int	boardType;

Option optionArray[] = {
    {OPT_DOC, NULL, NULL, "Usage: ultracmd [options] [device]"},
    {OPT_DOC, NULL, NULL, "Default device is \"hppi0\""},
    {OPT_TRUE, "dl", (Address) &download,
	"Download micro-code into the adapter"},
    {OPT_TRUE, "noisy", (Address) &noisyLoader,
	 "Print lots of messages during download"},
    {OPT_STRING, "dir", (Address) &dir,
	"Directory containing the micro-code files"},
    {OPT_STRING, "f", (Address) &file,
	"Micro-code file to download (default is to automatically pick one)"},
    {OPT_TRUE, "r", (Address) &reset,
	"Reset the adapter"},
    {OPT_TRUE, "R", (Address) &hardReset,
	"Hard reset the adapter"},
    {OPT_TRUE, "t", (Address) &getInfo,
	"Get type information from the adapter"},
    {OPT_TRUE, "d", (Address) &diag,
	"Run diagnostic tests on adapter"},
    {OPT_TRUE, "ed", (Address) &extDiag,
	"Run extended diagnostic tests on adapter"},
    {OPT_TRUE, "ext", (Address) &external,
	"Use external loopback when running extended diagnostics"},
    {OPT_STRING, "dev", (Address) &dev,
	"Directory containing the device"},
    {OPT_STRING, "dbg", (Address) &debug,
	"Ultranet device debugging output (on/off)"},
    {OPT_INT, "flags", (Address) &boardFlags,
	"Flags to send to the boards."},
    {OPT_TRUE, "i", (Address) &init,
	"Send initialization command to adapter"},
    {OPT_TRUE, "s", (Address) &start,
	"Send start request to the adapter."},
    {OPT_STRING, "a", (Address) &address,
	"Set Ultranet address of adapter."},
    {OPT_STRING, "dsnd", (Address) &dsnd,
	"Send a datagram to the given address."},
    {OPT_TRUE, "drcv", (Address) &drcv,
	 "Receive a datagram from any address."},
    {OPT_INT, "cnt", (Address) &count,
	"Number of times to send a datagram (use with -dsnd)."},
    {OPT_INT, "repeat", (Address) &repeat,
	"Number of times to repeat the send-datagram test (use with -dsnd)."},
    {OPT_INT, "size", (Address) &size,
	"Size of the datagram (use with -dsnd)."},
    {OPT_STRING, "echo", (Address) &echo,
	"Echo received datagrams back to sender (on/off)"},
    {OPT_STRING, "trace", (Address) &trace,
	"Start and stop tracing of ultra driver activity (on/off)"},
    {OPT_STRING, "source", (Address) &source,
	"Send a stream of datagrams to the given address."},
    {OPT_STRING, "sink", (Address) &sink,
	"Toggle sink of incoming datagrams (on/off)"},
    {OPT_STRING, "stat", (Address) &stat,
	"Manipulate collection of ultranet statistics (on/off/clear/get)"},
    {OPT_INT, "m", (Address) &map,
	"Set mapping threshold."},
    {OPT_INT, "bcopy", (Address) &bcopy,
	"Bcopy test data size."},
    {OPT_INT, "sg", (Address) &sg,
	"SG bcopy test data size."},
    {OPT_TRUE, "setup", (Address)&setup,
	 "Set up the boards."},
    {OPT_STRING, "board", (Address) &board,
	 "Board to use (src | dst | iop)."},
    {OPT_INT, "rreg", (Address) &readReg,
	 "Read a board register."},
    {OPT_INT, "wreg", (Address) &writeReg,
	 "Write a board register."},
    {OPT_INT, "value", (Address) &writeValue,
	 "Value to write (use with -wreg)."},
    {OPT_INT, "tsap", (Address) &tsap,
	 "TSAP value to use on datagrams"},
};
int numOptions = sizeof(optionArray) / sizeof(Option);

/*
 * The following are the names of the diagnostic tests as listed in the
 * uvm man page provided in the Ultranet documentation.
 */
static char	*diagNames[] = {
    "EPROM checksum",
    "Abbreviated RAM check",
    "Interrupt controller and interval timer",
    "Internal loopback",
    "FIFO RAM check",
    "Checksum gate arrays",
    "NMI control logic"
};
/*
 * The following are the names of the extended diagnostic tests as
 * listed in the uvm man page.
 */
static char	*extDiagNames[] = {
    "EPROM checksum",
    "Full RAM check",
    "Interrupt controller and interval timer",
    "Internal or external loopback",
    "FIFO RAM check",
    "Checksum gate arrays",
    "NMI control logic",
    "DMA to hosts memory using VME bus",
    "Extended FIFO RAM check",
    "FIFO Control logic"
};

char	*myname;
void	Download();

#define MIN(a,b) ((a) < (b) ? (a) : (b))

char	*device = "hppi0";


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
 *	The state of the adapter board may be modified.
 *
 *----------------------------------------------------------------------
 */

int
main(argc, argv)
    int		argc;
    char	*argv[];
{
    int			argsLeft;
    char		deviceName[FS_MAX_PATH_NAME_LENGTH];
    int			fd;
    ReturnStatus	status;

    myname = argv[0];
    argsLeft = Opt_Parse(argc, argv, optionArray, numOptions, 0);
    if (argsLeft > 2) {
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }
    if (download + reset + getInfo + diag + extDiag + (debug != NULL) +
	init + start + (address != NULL) + (dsnd != NULL) + drcv +
	(echo != NULL) + (trace != NULL) + (source != NULL) +
	(sink != NULL) + (stat != NULL) + (tsap != 0) +
	(map >= 0) + hardReset + setup +
	(readReg != -1) + (writeReg != -1) + (boardFlags != -1) > 1) {
	printf("You can only specify one of the following options:\n");
	printf("-dl, -r, -t, -d, -ed, -dbg, -i, -s, -a, -dsnd, -echo\n");
	printf("-trace, -source, -stat, -m, -R, -rreg, -wreg, -flags\n");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }
    if ((external == TRUE) && (extDiag == FALSE)) {
	printf("The -ext option can only be used with the -ed option\n");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }

    if (!strcasecmp (board, "dst")) {
	boardType = DEV_HPPI_DST_BOARD;
    } else if (!strcasecmp (board, "src")) {
	boardType = DEV_HPPI_SRC_BOARD;
    } else if (!strcasecmp (board, "iop")) {
	boardType = DEV_HPPI_IOP_BOARD;
    } else {
	printf ("-board must be \"dst\", \"src\" or \"iop\"\n");
	Opt_PrintUsage (argv[0], optionArray, numOptions);
	exit (1);
    }
    if (argsLeft == 2) {
	device = argv[1];
    }
    sprintf(deviceName, "%s/%s", dev, device);
    fd = open(deviceName, O_RDONLY);
    if (fd < 0) {
	printf("Can't open device \"%s\"\n", deviceName);
	perror("");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }
    if (debug) {
	int flag;
	if (!strcasecmp(debug, "on")) {
	    flag = 1;
	} else if (!strcasecmp(debug, "off")) {
	    flag = 0;
	} else {
	    printf("Invalid parameter to -dbg. Must be \"on\" or \"off\"\n");
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	status = Fs_IOControl(fd, IOC_HPPI_DEBUG, sizeof(int), &flag, 0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	}
    }
    if (reset) {
	status = Fs_IOControl(fd, IOC_HPPI_RESET, 0, NULL, 0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	}
    }
    if (hardReset) {
	status = Fs_IOControl(fd, IOC_HPPI_HARD_RESET, 0, NULL, 0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	}
    }

    if (setup) {
	status = Fs_IOControl (fd, IOC_HPPI_SETUP, 0, NULL, 0, 0);
	if (status != SUCCESS) {
	    printf ("setup: Fs_IOControl returned 0x%x\n", status);
	}
    }

    if (writeReg >= 0) {
	Dev_HppiRegCmd regCmd;
	regCmd.board = boardType;
	regCmd.offset = writeReg;
	regCmd.value = writeValue;
	status = Fs_IOControl (fd, IOC_HPPI_WRITE_REG, sizeof (regCmd),
			       &regCmd, 0, NULL);
	if (status != SUCCESS) {
	    printf ("Fs_IOControl returned 0x%x\n", status);
	} else {
	    printf ("%s board register at 0x%x offset was written with 0x%x\n",
		    board, writeReg, writeValue);
	}
    }

    if (readReg >= 0) {
	Dev_HppiRegCmd regCmd, regReply;
	regCmd.board = boardType;
	regCmd.offset = readReg;
	regCmd.value = 0;
	status = Fs_IOControl (fd, IOC_HPPI_READ_REG, sizeof (regCmd),
			       &regCmd, sizeof (regReply), &regReply);
	if (status != SUCCESS) {
	    printf ("Fs_IOControl returned 0x%x\n", status);
	} else {
	    printf ("%s board register at offset 0x%x was read as 0x%x\n",
		    board, regReply.offset, regReply.value);
	}
    }

    if (boardFlags >= 0) {
	status = Fs_IOControl (fd, IOC_HPPI_SET_BOARD_FLAGS,
			       sizeof (boardFlags), &boardFlags, 0, NULL);
	if (status != SUCCESS) {
	    printf ("Fs_IOControl returned 0x%x\n", status);
	} else {
	    printf ("Board flags set to 0x%x\n", boardFlags);
	}
    }
#if 0
    if (getInfo) {
	Dev_UltraAdapterInfo	info;
	status = Fs_IOControl(fd, IOC_ULTRA_GET_ADAP_INFO, 0, NULL, 
		    sizeof(Dev_UltraAdapterInfo), &info);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} else {
	    printf(
    "Model 99-%04d-%04d, Revision %d, Options %d, Firmware %d, Serial %d\n",
		info.hwModel, info.hwVersion, info.hwRevision, 
		info.hwOption, info.version, info.hwSerial);
	}
    }
    if (diag) {
	Dev_UltraDiag		cmd;
	status = Fs_IOControl(fd, IOC_ULTRA_DIAG, 0, NULL, 
		    sizeof(Dev_UltraDiag), &cmd);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} else {
	    if (cmd.error == 0) {
		printf("All diagnostic tests passed.\n");
	    } else {
		int	tmp = cmd.error;
		int	i;
		printf("The following diagnostic tests failed:\n");
		for (i = 0; i < sizeof(int); i++) {
		    if (tmp & 1) {
			printf("%2d: %s\n", i+1, diagNames[i]);
		    }
		    tmp >>= 1;
		}
	    }
	}
    }
    if (extDiag) {
	Dev_UltraExtendedDiag	cmd;
	cmd.externalLoopback = external;
	status = Fs_IOControl(fd, IOC_ULTRA_EXTENDED_DIAG, sizeof(cmd), 
		    (Address) &cmd, sizeof(cmd), (Address) &cmd);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} else {
	    if (cmd.error == 0) {
		printf("All extended diagnostic tests passed.\n");
	    } else {
		int	tmp = cmd.error;
		int	i;
		printf("The following extended diagnostic tests failed:\n");
		for (i = 0; i < sizeof(int); i++) {
		    if (tmp & 1) {
			printf("%2d: %s\n", i+1, extDiagNames[i]);
		    }
		    tmp >>= 1;
		}
	    }
	}
    }
#endif
    if (download) {
	Download(fd, dir, file, boardType);
    }
#if 0
    if (init) {
	status = Fs_IOControl(fd, IOC_ULTRA_INIT, 0, NULL, 0, NULL);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} 
    }
#endif
    if (start) {
	status = Fs_IOControl(fd, IOC_HPPI_START, 0, NULL, 0, NULL);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} 
    }
    if (address) {
	Net_UltraAddress	ultraAddress;
	int			count;
	int			group;
	int			unit;
	count = sscanf(address,"%d/%d", &group, &unit);
	if (count != 2) {
	    printf("%s: argument to -a is of form <group>/<unit>\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	if (group < 1 || group > 1000) {
	    printf("%s: group must be 1-1000\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	if (unit < 32 || unit > 62) {
	    printf("%s: unit must be 32-62\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	Net_UltraAddressSet(&ultraAddress, group, unit);
	status = Fs_IOControl(fd, IOC_HPPI_ADDRESS, sizeof(Net_UltraAddress),
		    &ultraAddress, 0, NULL);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} 
    }

    if (tsap != 0) {
	printf ("Setting tsap to 0x%08x\n", tsap);
	status = Fs_IOControl (fd, IOC_HPPI_SET_TSAP, sizeof (tsap),
			       &tsap, 0, NULL);
	if (status != SUCCESS) {
	    printf ("Fs_IOControl returned 0x%x\n", status);
	}
    }

    if (drcv) {
	status = Fs_IOControl(fd, IOC_HPPI_RECV_DGRAM, 0, NULL, 0, NULL);
	if (status != SUCCESS) {
	    printf ("Fs_IOControl returned 0x%x\n", status);
	}
    }

    if (dsnd) {
	Dev_UltraSendDgram	dgram;
	int			pid;
	int			n;
	int			group;
	int			unit;
	n = sscanf(dsnd,"%d/%d", &group, &unit);
	if (n != 2) {
	    printf("%s: argument to -dsnd is of form <group>/<unit>\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	if (group < 1 || group > 1000) {
	    printf("%s: group must be 1-1000\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	if (unit < 32 || unit > 62) {
	    printf("%s: unit must be 32-62\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	Net_UltraAddressSet(&dgram.address.address.ultra, group, unit);
	dgram.count = count;
	dgram.size = size;
	dgram.useBuffer = FALSE;
	while (repeat > 0) {
	    repeat--;
	    status = Fs_IOControl(fd, IOC_HPPI_SEND_DGRAM, 
			sizeof(dgram), &dgram, sizeof(dgram), &dgram);
	    if (status != SUCCESS) {
		printf("Fs_IOControl returned 0x%x\n", status);
		break;
	    }
	    Time_Divide(dgram.time, count, &dgram.time);
	    printf("%d\t%d.%06d\n", size, dgram.time.seconds, 
		dgram.time.microseconds);
	}
    }
    if (echo) {
	Dev_UltraEcho		echoParam;
	if (!strcasecmp(echo, "on")) {
	    echoParam.echo = TRUE;
	} else if (!strcasecmp(echo, "off")) {
	    echoParam.echo = FALSE;
	} else {
	    printf("Invalid parameter to -echo. Must be \"on\" or \"off\"\n");
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	status = Fs_IOControl(fd, IOC_HPPI_ECHO, sizeof(echoParam),
	    &echoParam, 0, NULL);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	}
    }
    if (trace) {
	int flag;
	if (!strcasecmp(trace, "on")) {
	    flag = 1;
	} else if (!strcasecmp(trace, "off")) {
	    flag = 0;
	} else {
	    printf("Invalid parameter to -trace. Must be \"on\" or \"off\"\n");
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	status = Fs_IOControl(fd, IOC_HPPI_TRACE, sizeof(int), &flag, 0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	}
    }
    if (source) {
	Dev_UltraSendDgram	dgram;
	int			pid;
	int			n;
	int			group;
	int			unit;
	n = sscanf(source,"%d/%d", &group, &unit);
	if (n != 2) {
	    printf("%s: argument to -dsnd is of form <group>/<unit>\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	if (group < 1 || group > 1000) {
	    printf("%s: group must be 1-1000\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	if (unit < 32 || unit > 62) {
	    printf("%s: unit must be 32-62\n", myname);
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	Net_UltraAddressSet(&dgram.address.address.ultra, group, unit);
	dgram.count = count;
	dgram.size = size;
	dgram.useBuffer = FALSE;
	while (repeat > 0) {
	    repeat--;
	    status = Fs_IOControl(fd, IOC_HPPI_SOURCE, 
			sizeof(dgram), &dgram, sizeof(dgram), &dgram);
	    if (status != SUCCESS) {
		printf("Fs_IOControl returned 0x%x\n", status);
		break;
	    }
	    printf("%d\t%d.%06d\n", size, dgram.time.seconds, 
		dgram.time.microseconds);
	}
    }
    if (sink) {
	Dev_UltraSink		sinkParam;
	int flag;
	int packets;
	if (!strcasecmp(sink, "on")) {
	    flag = 1;
	} else if (!strcasecmp(sink, "off")) {
	    flag = 0;
	} else {
	    printf("Invalid parameter to -sink. Must be \"on\" or \"off\"\n");
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	status = Fs_IOControl(fd, IOC_HPPI_SINK, sizeof(int), &flag, 
			sizeof(sinkParam), &sinkParam);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} else {
	    printf("%d packets sunk\n", sinkParam.packets);
	}
    }
    if (stat) {
	Dev_UltraStats		statParam;
	int			ioctl;
	int			flag;
	int			i;
	if (!strcasecmp(stat, "on")) {
	    flag = 1;
	    ioctl = IOC_HPPI_COLLECT_STATS;
	} else if (!strcasecmp(stat, "off")) {
	    flag = 0;
	    ioctl = IOC_HPPI_COLLECT_STATS;
	} else if (!strcasecmp(stat, "clear")) {
	    ioctl = IOC_HPPI_CLEAR_STATS;
	} else if (!strcasecmp(stat, "get")) {
	    ioctl = IOC_HPPI_GET_STATS;
	} else {
	    printf("Invalid parameter to -stat.\n");
	    printf("Must be \"on\",\"off\",\"clear\", or \"get\"\n");
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	switch(ioctl) {
	    case IOC_HPPI_COLLECT_STATS:
		status = Fs_IOControl(fd, ioctl, sizeof(int), &flag, 0,0);
		break;
	    case IOC_HPPI_CLEAR_STATS:
		status = Fs_IOControl(fd, ioctl, 0,0,0,0);
		break;
	    case IOC_HPPI_GET_STATS:
		status = Fs_IOControl(fd, ioctl, 0, 0, 
				sizeof(statParam), &statParam);
		break;
	}
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} else {
	    switch(ioctl) {
		case IOC_HPPI_COLLECT_STATS:
		    break;
		case IOC_HPPI_CLEAR_STATS:
		    break;
		case IOC_HPPI_GET_STATS:
		    printf("Packets sent:\t%9d\n", statParam.packetsSent);
		    printf("Bytes sent  :\t%9d\n", statParam.bytesSent);
		    printf("Histogram of packets sent:\n");
		    for (i = 0; i < 33; i++) {
			printf("[%2d,%2d):\t%9d\n", i, i+1, 
			    statParam.sentHistogram[i]);
		    }
		    printf("Packets received:\t%9d\n", 
			    statParam.packetsReceived);
		    printf("Bytes received  :\t%9d\n", 
			    statParam.bytesReceived);
		    printf("Histogram of packets received:\n");
		    for (i = 0; i < 33; i++) {
			printf("[%2d,%2d):\t%9d\n", i, i+1, 
			    statParam.receivedHistogram[i]);
		    }
		    break;
	    }
	}
    }
    if (map >= 0) {
	status = Fs_IOControl(fd, IOC_HPPI_MAP_THRESHOLD, sizeof(int), &map, 
			0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} 
    }
#if 0
    if (bcopy >= 0) {
	Time	time;
	status = Fs_IOControl(fd, IOC_ULTRA_BCOPY_TEST, sizeof(int), &bcopy, 
			sizeof(Time), &time);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} else {
	    printf("%d\t%d.%06d\n", bcopy, time.seconds, 
		time.microseconds);
	}
    }
    if (sg >= 0) {
	Time	time;
	status = Fs_IOControl(fd, IOC_ULTRA_SG_BCOPY_TEST, sizeof(int), &sg, 
			sizeof(Time), &time);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} else {
	    printf("%d\t%d.%06d\n", sg, time.seconds, 
		time.microseconds);
	}
    }
#endif
    close(fd);
    exit(0);
}

/*
 *----------------------------------------------------------------------
 *
 * Download --
 *
 *	Downloads the Ultranet adapter ucode onto the board..
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The ucode is loaded onto the board.
 *
 *----------------------------------------------------------------------
 */

void
Download(devFD, dir, file, boardType)
    int		devFD;		/* Handle on the ultranet adapter. */
    char	*dir;		/* Name of directory containing ucode files.*/
    char	*file;		/* If non-NULL then name of the file to
				 * download. */
    int		boardType;	/* determines whether src or dst board is
				 * being loaded */
{
    file29Hdr		fhdr;
    optionalFile29Hdr	ofhdr;
    section29Hdr	*sectionPtr, *curSectPtr;
    int		fd;
    char	path[FS_MAX_PATH_NAME_LENGTH];
    int		bytesRead;
    int totalWordsExpected = 0;
    int sectionCtr;
    int tmp;
    struct {
	Dev_HppiLoadHdr	hdr;
	int buffer[MAX_PACKET_SIZE/sizeof(int)];
    } loadCmd;
    int			readBuffer[MAX_PACKET_SIZE/sizeof(int)];
    Dev_HppiGo		goCmd;
    ReturnStatus	status;
    int			fmtStatus;
    int			inSize;
    int			outSize;
    int			bytesSent = 0;
    int			address;
    int			readNow;
    int			i;

	
    if (file == NULL) {
	if (boardType == DEV_HPPI_SRC_BOARD) {
	    sprintf(path, "%s/srcCode", dir);
	} else {
	    sprintf(path, "%s/dstCode", dir);
	}
    } else {
	sprintf(path,"%s/%s", dir, file);
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
	printf("%s: could not open code file \"%s\"\n", myname, path);
	perror(NULL);
	exit (1);
    }

    if (read (fd, readBuffer, sizeof (fhdr)) != sizeof (fhdr)) {
	printf ("%s: could not read file header (%s)\n", myname, path);
	perror (NULL);
	exit (1);
    }

    inSize = outSize = sizeof (fhdr);
    fmtStatus = Fmt_Convert ("h2w3h2", CODE29K_FORMAT, &inSize, readBuffer,
			     FMT_MY_FORMAT, &outSize, &fhdr);
    if (fmtStatus != FMT_OK) {
	printf ("%s: couldn't format convert file header\n", myname);
	perror (NULL);
	exit (1);
    }

    if (noisyLoader) {
	printf ("\nFILE HEADER information:\nid: %x\n", fhdr.id);
	printf ("Total section headers: %d\n", fhdr.sectionHdrCtr);
	printf ("File creation date: %s\n", ctime (&fhdr.timestamp));
	printf ("Symbols starts at file offset: %x\n", fhdr.symbolFp);
	printf ("Total symbols count: %ld\n", fhdr.symbolCtr);
		printf ("Optional file header length: %d\n",
			fhdr.optionalFhdrLength);
		printf ("File header flags: 0x%x <octal: %o>\n",
			fhdr.fhdrFlags, fhdr.fhdrFlags);
    }

    if (fhdr.optionalFhdrLength != 0)
    {
	if (read (fd, readBuffer, sizeof (ofhdr)) != sizeof (ofhdr)) {
	    printf ("%s: could not load code\n", myname);
	    exit (1);
	}

	inSize = outSize = sizeof (ofhdr);
	fmtStatus = Fmt_Convert ("h2w6", CODE29K_FORMAT, &inSize, readBuffer,
				 FMT_MY_FORMAT, &outSize, &ofhdr);
	if (fmtStatus != FMT_OK) {
	    printf ("%s: couldn't format convert optional file header\n",
		    myname);
	    perror (NULL);
	    exit (1);
	}

	if (noisyLoader) {
	    printf ("OPTIONAL FILE HEADER information:\nID: %x, version: %x\n",
		    ofhdr.id, ofhdr.version);
	    printf ("Size of executible codes: %lx\n", ofhdr.executibleSize);
	    printf ("Size of constant data: %lx\n", ofhdr.dataSize);
	    printf ("Size of uninitialized data: %lx\n", ofhdr.bssSize);
	    printf ("Executible code entry: %lx\n", ofhdr.entryAddr);
	    printf ("Base of .text section: %lx\n", ofhdr.codeBaseAddr);
	    printf ("Base of constant data: %lx\n", ofhdr.dataBaseAddr);
	}
	totalWordsExpected = (ofhdr.executibleSize + ofhdr.dataSize) / 4;
    }
    sectionCtr = fhdr.sectionHdrCtr;

/*
      Read in all the section headers from the file and store them
      in memory. After the section headers comes the loadable data. But
      the load addresses are embedded in the section headers. Therefore,
      the headers have to be extracted before the loadable data can be
      read.
*/

    if (sectionCtr > 0)
    {
	tmp = sectionCtr * sizeof (section29Hdr);
	if ((sectionPtr = (section29Hdr *)malloc (tmp)) == NULL) {
	    printf ("%s: can't allocate section headers\n", myname);
	    perror (NULL);
	    exit (1);
	}
	if (read (fd, readBuffer, tmp) != tmp) {
	    printf ("%s: can't read %d section headers\n", myname, sectionCtr);
	    perror (NULL);
	    exit (1);
	}

	inSize = outSize = tmp;
	fmtStatus = Fmt_Convert ("{b8w6h2w}*", CODE29K_FORMAT, &inSize,
				 readBuffer, FMT_MY_FORMAT, &outSize,
				 sectionPtr);
	if (fmtStatus != FMT_OK) {
	    printf ("%s: couldn't format convert section header\n", myname);
	    perror (NULL);
	    exit (1);
	}

    }

    curSectPtr = sectionPtr;
    while (sectionCtr-- > 0) {
	address = curSectPtr->loadAddr;

	if (!(((curSectPtr->sectionFlag) == STYPE_REG) ||
	      (((curSectPtr->sectionFlag) & STYPE_TEXT) == STYPE_TEXT)	||
	      (((curSectPtr->sectionFlag) & STYPE_DATA) == STYPE_DATA)	||
	      (((curSectPtr->sectionFlag) & STYPE_LIT) == STYPE_LIT))) {
	    continue;
	}

	if (noisyLoader) {
	    printf ("SECTION HEADER <%s> information:\n",
		    curSectPtr->sectionName);
	    printf ("Load to address: %lx, size: %ld bytes\n",
		    curSectPtr->loadAddr, curSectPtr->sectionSize);
	    printf ("Virtual address: %lx\n", curSectPtr->virtualAddr);
	    printf ("Raw data starts at offset: %lx\n", curSectPtr->rawdFp);
	    printf ("Relocation info starts at offset: %lx\n",
		    curSectPtr->relocFp);
	    printf ("Line number info starts at offset: %lx\n",
		    curSectPtr->lineFp);
	    printf ("Total relocation entries: %d\n", curSectPtr->relocCtr);
	    printf ("Total line number entries: %d\n", curSectPtr->lineCtr);
	    printf ("Section flag = 0x%x (%o)\n",
		    curSectPtr->sectionFlag, curSectPtr->sectionFlag);
	}

/*
 *	Read and load one section.  First, insure that the current
 *	file pointer is the same as curSectPtr->rawdFp.
 */
	tmp = lseek (fd, (long)0, L_INCR);
	if (tmp != curSectPtr->rawdFp) {
	    if (noisyLoader) {
		printf ("Current fp = %x, section raw data fp = %x\n",
			tmp, curSectPtr->rawdFp);
	    }
	    lseek (fd, curSectPtr->rawdFp, L_SET);
	}

	while (curSectPtr->sectionSize > 0) {
	    if (curSectPtr->sectionSize > MAX_PACKET_SIZE) {
		readNow = MAX_PACKET_SIZE;
	    } else {
		readNow = curSectPtr->sectionSize;
	    }
	    loadCmd.hdr.board = boardType;
	    loadCmd.hdr.size = readNow;
	    loadCmd.hdr.startAddress = address;
	    if (read (fd, (Address)readBuffer, readNow) != readNow) {
		printf ("%s: read error section %d with %d bytes left.\n",
			myname, sectionCtr, curSectPtr->sectionSize);
		perror (NULL);
		exit (1);
	    }

	    inSize = outSize = readNow;

	    fmtStatus = Fmt_Convert ("w*", CODE29K_FORMAT, &inSize, readBuffer,
				     FMT_MY_FORMAT, &outSize, loadCmd.buffer);

	    status = Fs_IOControl (devFD, IOC_HPPI_LOAD, sizeof (loadCmd),
				   (Address)&loadCmd, 0, NULL);
	    if (status != SUCCESS) {
		printf ("%s: Fs_IOControl returned 0x%x\n", myname, status);
#if 0
		perror (NULL);
		exit (1);
#endif
	    }
	    curSectPtr->sectionSize -= readNow;
	    bytesSent += readNow;
	    address += readNow;
	    if (noisyLoader) {
		putchar ('.');
		fflush (stdout);
	    }
	}
	curSectPtr++;
    }

    if (sectionPtr != NULL) {
	free (sectionPtr);
    }

#ifdef	NOT_NOW
    if (bytesSent != totalWordsExpected) {
	printf ("Expected to load %d words, but sent %d\n", totalWordsExpected,
		bytesSent);
    }
#endif

    printf ("%s: loaded %d (0x%x) words\n", myname, (bytesSent / 4),
	    (bytesSent / 4));

    /*
     * Now send the start address.
     */
    goCmd.startAddress = ofhdr.entryAddr;
    goCmd.board = boardType;
    status = Fs_IOControl(devFD, IOC_HPPI_GO, sizeof(goCmd), 
		(Address) &goCmd, 0, NULL);
    if (status != SUCCESS) {
	printf("Fs_IOControl returned 0x%x\n", status);
	exit(1);
    }
}


