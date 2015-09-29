#
# Prototype Makefile for machine-dependent directories.
#
# A file of this form resides in each ".md" subdirectory of a
# command.  Its name is typically "md.mk".  During makes in the
# parent directory, this file (or a similar file in a sibling
# subdirectory) is included to define machine-specific things
# such as additional source and object files.
#
# This Makefile is automatically generated.
# DO NOT EDIT IT OR YOU MAY LOSE YOUR CHANGES.
#
# Generated from /sprite/lib/mkmf/Makefile.md
# Sun Aug 11 17:48:33 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/devConfig.c sun3.md/devInit.c sun3.md/devMouse.c sun3.md/devSCSI3.c sun3.md/devScsiTapeConfig.c sun3.md/devSysgenTape.c sun3.md/devTMR.c sun3.md/devZ8530.c sun3.md/devVidSun3.s sun3.md/devSCSI0.c sun3.md/devFsOpTable.c sun3.md/devEmulexTape.c sun3.md/devGraphics.c sun3.md/devConsole.c sun3.md/devJaguarHBA.c sun3.md/devTtyAttach.c sun3.md/devXylogics450.c devNet.c devTty.c devNull.c devQueue.c devSCSI.c devScsiDevice.c devSCSITape.c devConsoleCmd.c devSCSIHBA.c devSyslog.c devBlockDevice.c devDiskStats.c devExabyteTape.c devSCSIDisk.c devRawBlockDev.c
HDRS		= sun3.md/console.h sun3.md/devAddrs.h sun3.md/devInt.h sun3.md/devMultibus.h sun3.md/devTMR.h sun3.md/devfb.h sun3.md/emulexTape.h sun3.md/jaguar.h sun3.md/jaguarDefs.h sun3.md/mouse.h sun3.md/scsi0.h sun3.md/scsi3.h sun3.md/sysgenTape.h sun3.md/ttyAttach.h sun3.md/xylogics450.h sun3.md/z8530.h dev.h devBlockDevice.h devDiskLabel.h devDiskStats.h devFsOpTable.h devNet.h devNull.h devQueue.h devSCSIC90Int.h devSCSIDisk.h devSCSITape.h devSyslog.h devTypes.h devVid.h diskStats.h exabyteTape.h rawBlockDev.h scsiDevice.h scsiHBA.h scsiHBADevice.h scsiTape.h tty.h
MDPUBHDRS	= sun3.md/devAddrs.h sun3.md/devMultibus.h sun3.md/devTMR.h sun3.md/devfb.h
OBJS		= sun3.md/devBlockDevice.o sun3.md/devConfig.o sun3.md/devConsole.o sun3.md/devConsoleCmd.o sun3.md/devDiskStats.o sun3.md/devEmulexTape.o sun3.md/devExabyteTape.o sun3.md/devFsOpTable.o sun3.md/devGraphics.o sun3.md/devInit.o sun3.md/devJaguarHBA.o sun3.md/devMouse.o sun3.md/devNet.o sun3.md/devNull.o sun3.md/devQueue.o sun3.md/devRawBlockDev.o sun3.md/devSCSI.o sun3.md/devSCSI0.o sun3.md/devSCSI3.o sun3.md/devSCSIDisk.o sun3.md/devSCSIHBA.o sun3.md/devSCSITape.o sun3.md/devScsiDevice.o sun3.md/devScsiTapeConfig.o sun3.md/devSysgenTape.o sun3.md/devSyslog.o sun3.md/devTMR.o sun3.md/devTty.o sun3.md/devTtyAttach.o sun3.md/devVidSun3.o sun3.md/devXylogics450.o sun3.md/devZ8530.o
CLEANOBJS	= sun3.md/devConfig.o sun3.md/devInit.o sun3.md/devMouse.o sun3.md/devSCSI3.o sun3.md/devScsiTapeConfig.o sun3.md/devSysgenTape.o sun3.md/devTMR.o sun3.md/devZ8530.o sun3.md/devVidSun3.o sun3.md/devSCSI0.o sun3.md/devFsOpTable.o sun3.md/devEmulexTape.o sun3.md/devGraphics.o sun3.md/devConsole.o sun3.md/devJaguarHBA.o sun3.md/devTtyAttach.o sun3.md/devXylogics450.o sun3.md/devNet.o sun3.md/devTty.o sun3.md/devNull.o sun3.md/devQueue.o sun3.md/devSCSI.o sun3.md/devScsiDevice.o sun3.md/devSCSITape.o sun3.md/devConsoleCmd.o sun3.md/devSCSIHBA.o sun3.md/devSyslog.o sun3.md/devBlockDevice.o sun3.md/devDiskStats.o sun3.md/devExabyteTape.o sun3.md/devSCSIDisk.o sun3.md/devRawBlockDev.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
