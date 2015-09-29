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
# Sun Aug 11 17:48:15 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/devConfig.c ds3100.md/devDC7085.c ds3100.md/devSII.c ds3100.md/devTtyAttach.c ds3100.md/devInit.c ds3100.md/font.c ds3100.md/devConsole.c ds3100.md/devScsiTapeConfig.c ds3100.md/devGraphics.c ds3100.md/devFsOpTable.c devNet.c devTty.c devNull.c devQueue.c devSCSI.c devScsiDevice.c devSCSITape.c devConsoleCmd.c devSCSIHBA.c devSyslog.c devBlockDevice.c devDiskStats.c devExabyteTape.c devSCSIDisk.c devRawBlockDev.c
HDRS		= ds3100.md/console.h ds3100.md/dc7085.h ds3100.md/devDependent.h ds3100.md/devInt.h ds3100.md/graphics.h ds3100.md/sii.h ds3100.md/ttyAttach.h dev.h devBlockDevice.h devDiskLabel.h devDiskStats.h devFsOpTable.h devNet.h devNull.h devQueue.h devSCSIC90Int.h devSCSIDisk.h devSCSITape.h devSyslog.h devTypes.h devVid.h diskStats.h exabyteTape.h rawBlockDev.h scsiDevice.h scsiHBA.h scsiHBADevice.h scsiTape.h tty.h
MDPUBHDRS	= ds3100.md/devDependent.h
OBJS		= ds3100.md/devBlockDevice.o ds3100.md/devConfig.o ds3100.md/devConsole.o ds3100.md/devConsoleCmd.o ds3100.md/devDC7085.o ds3100.md/devDiskStats.o ds3100.md/devExabyteTape.o ds3100.md/devFsOpTable.o ds3100.md/devGraphics.o ds3100.md/devInit.o ds3100.md/devNet.o ds3100.md/devNull.o ds3100.md/devQueue.o ds3100.md/devRawBlockDev.o ds3100.md/devSCSI.o ds3100.md/devSCSIDisk.o ds3100.md/devSCSIHBA.o ds3100.md/devSCSITape.o ds3100.md/devSII.o ds3100.md/devScsiDevice.o ds3100.md/devScsiTapeConfig.o ds3100.md/devSyslog.o ds3100.md/devTty.o ds3100.md/devTtyAttach.o ds3100.md/font.o
CLEANOBJS	= ds3100.md/devConfig.o ds3100.md/devDC7085.o ds3100.md/devSII.o ds3100.md/devTtyAttach.o ds3100.md/devInit.o ds3100.md/font.o ds3100.md/devConsole.o ds3100.md/devScsiTapeConfig.o ds3100.md/devGraphics.o ds3100.md/devFsOpTable.o ds3100.md/devNet.o ds3100.md/devTty.o ds3100.md/devNull.o ds3100.md/devQueue.o ds3100.md/devSCSI.o ds3100.md/devScsiDevice.o ds3100.md/devSCSITape.o ds3100.md/devConsoleCmd.o ds3100.md/devSCSIHBA.o ds3100.md/devSyslog.o ds3100.md/devBlockDevice.o ds3100.md/devDiskStats.o ds3100.md/devExabyteTape.o ds3100.md/devSCSIDisk.o ds3100.md/devRawBlockDev.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
