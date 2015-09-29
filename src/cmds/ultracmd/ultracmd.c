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
#include <status.h>
#include <fmt.h>

/*
 * The ucode file is in Intel format, which we don't have a constant for
 * at the moment so borrow the VAX format.
 */

#define UCODE_FORMAT	FMT_VAX_FORMAT

/*
 * Variables settable via the command line.
 */

Boolean	download = FALSE;	/* Download ucode to the adapter. */
char	*dir = "/sprite/lib/ultra/ucode";  /* Directory containing ucode. */
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
int	bcopy = -1;		
int	sg = -1;		
Boolean hardReset = FALSE;

Option optionArray[] = {
    {OPT_DOC, NULL, NULL, "Usage: ultracmd [options] [device]"},
    {OPT_DOC, NULL, NULL, "Default device is \"ultra0\""},
    {OPT_TRUE, "dl", (Address) &download,
	"Download micro-code into the adapter"},
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
    {OPT_TRUE, "i", (Address) &init,
	"Send initialization command to adapter"},
    {OPT_TRUE, "s", (Address) &start,
	"Send start request to the adapter."},
    {OPT_STRING, "a", (Address) &address,
	"Set Ultranet address of adapter."},
    {OPT_STRING, "dsnd", (Address) &dsnd,
	"Send a datagram to the given address."},
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

char	*device = "ultra0";


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
	init + start + (address != NULL) + (dsnd != NULL) +
	(echo != NULL) + (trace != NULL) + (source != NULL) +
	(sink != NULL) + (stat != NULL) + (map >= 0) + hardReset > 1) {
	printf("You can only specify one of the following options:\n");
	printf("-dl, -r, -t, -d, -ed, -dbg, -i, -s, -a, -dsnd, -echo\n");
	printf("-trace, -source, -stat, -m, -R\n");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
    }
    if ((external == TRUE) && (extDiag == FALSE)) {
	printf("The -ext option can only be used with the -ed option\n");
	Opt_PrintUsage(argv[0], optionArray, numOptions);
	exit(1);
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
	status = Fs_IOControl(fd, IOC_ULTRA_DEBUG, sizeof(int), &flag, 0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	}
    }
    if (reset) {
	status = Fs_IOControl(fd, IOC_ULTRA_RESET, 0, NULL, 0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	}
    }
    if (hardReset) {
	status = Fs_IOControl(fd, IOC_ULTRA_HARD_RESET, 0, NULL, 0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	}
    }
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
    if (download) {
	Download(fd, dir, file);
    }
    if (init) {
	status = Fs_IOControl(fd, IOC_ULTRA_INIT, 0, NULL, 0, NULL);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} 
    }
    if (start) {
	status = Fs_IOControl(fd, IOC_ULTRA_START, 0, NULL, 0, NULL);
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
	status = Fs_IOControl(fd, IOC_ULTRA_ADDRESS, sizeof(Net_UltraAddress),
		    &ultraAddress, 0, NULL);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
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
	Net_UltraAddressSet(&dgram.address.ultra, group, unit);
	dgram.count = count;
	dgram.size = size;
	dgram.useBuffer = FALSE;
	while (repeat > 0) {
	    repeat--;
	    status = Fs_IOControl(fd, IOC_ULTRA_SEND_DGRAM, 
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
	status = Fs_IOControl(fd, IOC_ULTRA_ECHO, sizeof(echoParam),
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
	status = Fs_IOControl(fd, IOC_ULTRA_TRACE, sizeof(int), &flag, 0, 0);
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
	Net_UltraAddressSet(&dgram.address.ultra, group, unit);
	dgram.count = count;
	dgram.size = size;
	dgram.useBuffer = FALSE;
	while (repeat > 0) {
	    repeat--;
	    status = Fs_IOControl(fd, IOC_ULTRA_SOURCE, 
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
	status = Fs_IOControl(fd, IOC_ULTRA_SINK, sizeof(int), &flag, 
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
	    ioctl = IOC_ULTRA_COLLECT_STATS;
	} else if (!strcasecmp(stat, "off")) {
	    flag = 0;
	    ioctl = IOC_ULTRA_COLLECT_STATS;
	} else if (!strcasecmp(stat, "clear")) {
	    ioctl = IOC_ULTRA_CLEAR_STATS;
	} else if (!strcasecmp(stat, "get")) {
	    ioctl = IOC_ULTRA_GET_STATS;
	} else {
	    printf("Invalid parameter to -stat.\n");
	    printf("Must be \"on\",\"off\",\"clear\", or \"get\"\n");
	    Opt_PrintUsage(argv[0], optionArray, numOptions);
	    exit(1);
	}
	switch(ioctl) {
	    case IOC_ULTRA_COLLECT_STATS:
		status = Fs_IOControl(fd, ioctl, sizeof(int), &flag, 0,0);
		break;
	    case IOC_ULTRA_CLEAR_STATS:
		status = Fs_IOControl(fd, ioctl, 0,0,0,0);
		break;
	    case IOC_ULTRA_GET_STATS:
		status = Fs_IOControl(fd, ioctl, 0, 0, 
				sizeof(statParam), &statParam);
		break;
	}
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} else {
	    switch(ioctl) {
		case IOC_ULTRA_COLLECT_STATS:
		    break;
		case IOC_ULTRA_CLEAR_STATS:
		    break;
		case IOC_ULTRA_GET_STATS:
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
	status = Fs_IOControl(fd, IOC_ULTRA_MAP_THRESHOLD, sizeof(int), &map, 
			0, 0);
	if (status != SUCCESS) {
	    printf("Fs_IOControl returned 0x%x\n", status);
	} 
    }
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
    close(fd);
    if (status != SUCCESS) {
	exit(1);
    }
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
Download(devFD, dir, file)
    int		devFD;		/* Handle on the ultranet adapter. */
    char	*dir;		/* Name of directory containing ucode files.*/
    char	*file;		/* If non-NULL then name of the file to
				 * download. */
{
    int		fd;
    char	path[FS_MAX_PATH_NAME_LENGTH];
    int		bytesRead;
    struct	{
	unsigned long	unused;
	unsigned long	length;
	unsigned long	address;
    } header;
    char buffer[0x8000];	/* Biggest possible size of data. */
    Dev_UltraLoad	loadCmd;
    Dev_UltraGo		goCmd;
    ReturnStatus	status;
    int			fmtStatus;
    int			inSize;
    int			outSize;
    int			startAddress;
    int			bytesSent;
    int			address;

    if (file == NULL) {
	Dev_UltraAdapterInfo	info;
	status = Fs_IOControl(devFD, IOC_ULTRA_GET_ADAP_INFO, 0, NULL, 
		    sizeof(Dev_UltraAdapterInfo), &info);
	if (status != SUCCESS) {
	    if (status == DEV_BUSY) {
		printf("You must reset the adapter first.\n");
		exit(1);
	    }
	    printf("%s: Fs_IOControl to get adapter info failed 0x%x.\n", 
		myname, status);
	    exit(1);
	}
	sprintf(path,"%s/uvm%d.ult", dir, info.hwModel);
    } else {
	sprintf(path,"%s/%s", dir, file);
    }
    fd = open(path, O_RDONLY);
    if (fd < 0) {
	printf("%s: could not open ucode file \"%s\"\n", myname, path);
	perror(NULL);
    }
    bytesRead = read(fd, buffer, sizeof(header));
    while(bytesRead > 0) {
	/*
	 * The load address is stored in the ucode file in 
	 * Intel byte order.
	 */
	inSize = sizeof(buffer);
	outSize = sizeof(int);
	fmtStatus = Fmt_Convert("w", UCODE_FORMAT, &inSize, 
			(Address) &buffer[8], FMT_MY_FORMAT, &outSize,
			(Address) &header.address);
	if (fmtStatus != 0) {
	    printf("%s: Fmt_Convert of header returned %d\n", myname,
		fmtStatus);
	    exit(1);
	}
	if (outSize != sizeof(int)) {
	    printf("%s: header changed size after conversion.\n", myname);
	    exit(1);
	}
	/*
	 * The length field is stored in sun byte order.
	 */
	inSize = sizeof(buffer);
	outSize = sizeof(int);
	fmtStatus = Fmt_Convert("w", FMT_68K_FORMAT, &inSize, 
			(Address) &buffer[4], FMT_MY_FORMAT, &outSize,
			(Address) &header.length);
	if (fmtStatus != 0) {
	    printf("%s: Fmt_Convert of header returned %d\n", myname,
		fmtStatus);
	    exit(1);
	}
	if (outSize != sizeof(int)) {
	    printf("%s: header changed size after conversion.\n", myname);
	    exit(1);
	}
	/*
	 * If the length field is 0 then the address field contains the 
	 * start address.
	 */
	if (header.length == 0) {
	    startAddress = header.address;
	    break;
	}
#if 0
	printf("Header address = 0x%x\n", header.address);
	printf("Header length = %d 0x%x\n", header.length, header.length);
#endif
	/* 
	 * Break the data into pieces that can be sent via an ioctl and
	 * send it to the device.
	 */
	bytesSent = 0;
	address = header.address;
	while(bytesSent < bytesRead) {
	    int 	length;

	    length = MIN(header.length - bytesSent, sizeof(loadCmd.data));
	    loadCmd.address = address;
	    loadCmd.length = length;
	    /*
	     * Now read in the data.
	     */
	    bytesRead = read(fd, loadCmd.data, length);
	    if (bytesRead != length) {
		printf("%s: short read on data, %d expecting %d\n", myname,
		    bytesRead, length);
		exit(1);
	    }
#if 0
	    printf("Download 0x%x 0x%x\n", loadCmd.address, loadCmd.length);
#endif
	    status = Fs_IOControl(devFD, IOC_ULTRA_LOAD, sizeof(loadCmd), 
			(Address) &loadCmd, 0, NULL);
	    if (status != SUCCESS) {
		printf("Fs_IOControl returned 0x%x\n", status);
		exit(1);
	    }
	    bytesSent += length;
	    /*
	     * Now adjust the load address in case we are sending a
	     * block of data in multiple ioctls.
	     */
	    address += length;
	}
	bytesRead = read(fd, buffer, sizeof(header));
    }
    /*
     * Now send the start address.
     */
    goCmd.address = startAddress;
    status = Fs_IOControl(devFD, IOC_ULTRA_GO, sizeof(goCmd), 
		(Address) &goCmd, 0, NULL);
    if (status != SUCCESS) {
	printf("Fs_IOControl returned 0x%x\n", status);
	exit(1);
    }
}


