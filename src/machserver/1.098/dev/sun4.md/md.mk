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
# Sun Aug 11 17:48:42 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/devConsole.c sun4.md/devEmulexTape.c sun4.md/devInit.c sun4.md/devJaguarHBA.c sun4.md/devMouse.c sun4.md/devSCSI0.c sun4.md/devSCSI3.c sun4.md/devScsiTapeConfig.c sun4.md/devSysgenTape.c sun4.md/devTtyAttach.c sun4.md/devXylogics450.c sun4.md/devZ8530.c sun4.md/devTMR.c sun4.md/devVidSun4.s sun4.md/devFsOpTable.c sun4.md/devConfig.c sun4.md/devGraphics.c devNet.c devTty.c devNull.c devQueue.c devSCSI.c devScsiDevice.c devSCSITape.c devConsoleCmd.c devSCSIHBA.c devSyslog.c devBlockDevice.c devDiskStats.c devExabyteTape.c devSCSIDisk.c devRawBlockDev.c
HDRS		= sun4.md/console.h sun4.md/devAddrs.h sun4.md/devInt.h sun4.md/devMultibus.h sun4.md/devTMR.h sun4.md/devfb.h sun4.md/emulexTape.h sun4.md/jaguar.h sun4.md/jaguarDefs.h sun4.md/mouse.h sun4.md/scsi0.h sun4.md/scsi3.h sun4.md/sysgenTape.h sun4.md/ttyAttach.h sun4.md/xylogics450.h sun4.md/z8530.h dev.h devBlockDevice.h devDiskLabel.h devDiskStats.h devFsOpTable.h devNet.h devNull.h devQueue.h devSCSIC90Int.h devSCSIDisk.h devSCSITape.h devSyslog.h devTypes.h devVid.h diskStats.h exabyteTape.h rawBlockDev.h scsiDevice.h scsiHBA.h scsiHBADevice.h scsiTape.h tty.h
MDPUBHDRS	= sun4.md/devAddrs.h sun4.md/devMultibus.h sun4.md/devTMR.h sun4.md/devfb.h
OBJS		= sun4.md/devBlockDevice.o sun4.md/devConfig.o sun4.md/devConsole.o sun4.md/devConsoleCmd.o sun4.md/devDiskStats.o sun4.md/devEmulexTape.o sun4.md/devExabyteTape.o sun4.md/devFsOpTable.o sun4.md/devGraphics.o sun4.md/devInit.o sun4.md/devJaguarHBA.o sun4.md/devMouse.o sun4.md/devNet.o sun4.md/devNull.o sun4.md/devQueue.o sun4.md/devRawBlockDev.o sun4.md/devSCSI.o sun4.md/devSCSI0.o sun4.md/devSCSI3.o sun4.md/devSCSIDisk.o sun4.md/devSCSIHBA.o sun4.md/devSCSITape.o sun4.md/devScsiDevice.o sun4.md/devScsiTapeConfig.o sun4.md/devSysgenTape.o sun4.md/devSyslog.o sun4.md/devTMR.o sun4.md/devTty.o sun4.md/devTtyAttach.o sun4.md/devVidSun4.o sun4.md/devXylogics450.o sun4.md/devZ8530.o
CLEANOBJS	= sun4.md/devConsole.o sun4.md/devEmulexTape.o sun4.md/devInit.o sun4.md/devJaguarHBA.o sun4.md/devMouse.o sun4.md/devSCSI0.o sun4.md/devSCSI3.o sun4.md/devScsiTapeConfig.o sun4.md/devSysgenTape.o sun4.md/devTtyAttach.o sun4.md/devXylogics450.o sun4.md/devZ8530.o sun4.md/devTMR.o sun4.md/devVidSun4.o sun4.md/devFsOpTable.o sun4.md/devConfig.o sun4.md/devGraphics.o sun4.md/devNet.o sun4.md/devTty.o sun4.md/devNull.o sun4.md/devQueue.o sun4.md/devSCSI.o sun4.md/devScsiDevice.o sun4.md/devSCSITape.o sun4.md/devConsoleCmd.o sun4.md/devSCSIHBA.o sun4.md/devSyslog.o sun4.md/devBlockDevice.o sun4.md/devDiskStats.o sun4.md/devExabyteTape.o sun4.md/devSCSIDisk.o sun4.md/devRawBlockDev.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
