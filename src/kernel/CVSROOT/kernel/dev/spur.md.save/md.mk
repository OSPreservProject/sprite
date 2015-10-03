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
# Thu Aug 23 13:01:15 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= spur.md/devCC.c spur.md/devCCAsm.s spur.md/devConfig.c spur.md/devFsOpTable.c spur.md/devInit.c spur.md/devScsiTapeConfig.c spur.md/devTtyAttach.c spur.md/devVid.c devBlockDevice.c devConsoleCmd.c devDiskStats.c devNet.c devNull.c devQueue.c devRawBlockDev.c devSCSI.c devSCSIDisk.c devSCSIHBA.c devSCSITape.c devScsiDevice.c devSyslog.c devTty.c
HDRS		= spur.md/devAddrs.h spur.md/devCC.h spur.md/devInt.h spur.md/devTypesInt.h spur.md/devUart.h spur.md/ttyAttach.h spur.md/uartConst.h dev.h devBlockDevice.h devDiskLabel.h devDiskStats.h devFsOpTable.h devNet.h devNull.h devQueue.h devSCSIDisk.h devSCSITape.h devSyslog.h devTypes.h devVid.h diskStats.h kernelTime.h rawBlockDev.h scsi.h scsiDevice.h scsiHBA.h scsiHBADevice.h scsiTape.h tty.h
MDPUBHDRS	= spur.md/devAddrs.h spur.md/devCC.h spur.md/devUart.h
OBJS		= spur.md/devCC.o spur.md/devCCAsm.o spur.md/devConfig.o spur.md/devFsOpTable.o spur.md/devInit.o spur.md/devScsiTapeConfig.o spur.md/devTtyAttach.o spur.md/devVid.o spur.md/devBlockDevice.o spur.md/devConsoleCmd.o spur.md/devDiskStats.o spur.md/devNet.o spur.md/devNull.o spur.md/devQueue.o spur.md/devRawBlockDev.o spur.md/devSCSI.o spur.md/devSCSIDisk.o spur.md/devSCSIHBA.o spur.md/devSCSITape.o spur.md/devScsiDevice.o spur.md/devSyslog.o spur.md/devTty.o
CLEANOBJS	= spur.md/devCC.o spur.md/devCCAsm.o spur.md/devConfig.o spur.md/devFsOpTable.o spur.md/devInit.o spur.md/devScsiTapeConfig.o spur.md/devTtyAttach.o spur.md/devVid.o spur.md/devBlockDevice.o spur.md/devConsoleCmd.o spur.md/devDiskStats.o spur.md/devNet.o spur.md/devNull.o spur.md/devQueue.o spur.md/devRawBlockDev.o spur.md/devSCSI.o spur.md/devSCSIDisk.o spur.md/devSCSIHBA.o spur.md/devSCSITape.o spur.md/devScsiDevice.o spur.md/devSyslog.o spur.md/devTty.o
INSTFILES	= spur.md/md.mk spur.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
