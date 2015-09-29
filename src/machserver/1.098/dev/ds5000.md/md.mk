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
# Sun Aug 11 17:48:23 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/devConfig.c ds5000.md/devConsole.c ds5000.md/devDC7085.c ds5000.md/devFsOpTable.c ds5000.md/devInit.c ds5000.md/devScsiTapeConfig.c ds5000.md/devTtyAttach.c ds5000.md/font.c ds5000.md/devStdFB.c ds5000.md/devGraphics.c ds5000.md/devSCSIC90Mach.c ds5000.md/devSCSIC90.c devNet.c devTty.c devNull.c devQueue.c devSCSI.c devScsiDevice.c devSCSITape.c devConsoleCmd.c devSCSIHBA.c devSyslog.c devBlockDevice.c devDiskStats.c devExabyteTape.c devSCSIDisk.c devRawBlockDev.c
HDRS		= ds5000.md/console.h ds5000.md/dc7085.h ds5000.md/devDependent.h ds5000.md/devGraphicsInt.h ds5000.md/devInt.h ds5000.md/devSCSIC90.h ds5000.md/devSCSIC90Int.h ds5000.md/devSCSIC90Mach.h ds5000.md/devStdFBInt.h ds5000.md/graphics.h ds5000.md/scsiC90.h ds5000.md/sii.h ds5000.md/ttyAttach.h dev.h devBlockDevice.h devDiskLabel.h devDiskStats.h devFsOpTable.h devNet.h devNull.h devQueue.h devSCSIDisk.h devSCSITape.h devSyslog.h devTypes.h devVid.h diskStats.h exabyteTape.h rawBlockDev.h scsiDevice.h scsiHBA.h scsiHBADevice.h scsiTape.h tty.h
MDPUBHDRS	= ds5000.md/devDependent.h ds5000.md/devSCSIC90.h ds5000.md/devSCSIC90Mach.h
OBJS		= ds5000.md/devBlockDevice.o ds5000.md/devConfig.o ds5000.md/devConsole.o ds5000.md/devConsoleCmd.o ds5000.md/devDC7085.o ds5000.md/devDiskStats.o ds5000.md/devExabyteTape.o ds5000.md/devFsOpTable.o ds5000.md/devGraphics.o ds5000.md/devInit.o ds5000.md/devNet.o ds5000.md/devNull.o ds5000.md/devQueue.o ds5000.md/devRawBlockDev.o ds5000.md/devSCSI.o ds5000.md/devSCSIC90.o ds5000.md/devSCSIC90Mach.o ds5000.md/devSCSIDisk.o ds5000.md/devSCSIHBA.o ds5000.md/devSCSITape.o ds5000.md/devScsiDevice.o ds5000.md/devScsiTapeConfig.o ds5000.md/devStdFB.o ds5000.md/devSyslog.o ds5000.md/devTty.o ds5000.md/devTtyAttach.o ds5000.md/font.o
CLEANOBJS	= ds5000.md/devConfig.o ds5000.md/devConsole.o ds5000.md/devDC7085.o ds5000.md/devFsOpTable.o ds5000.md/devInit.o ds5000.md/devScsiTapeConfig.o ds5000.md/devTtyAttach.o ds5000.md/font.o ds5000.md/devStdFB.o ds5000.md/devGraphics.o ds5000.md/devSCSIC90Mach.o ds5000.md/devSCSIC90.o ds5000.md/devNet.o ds5000.md/devTty.o ds5000.md/devNull.o ds5000.md/devQueue.o ds5000.md/devSCSI.o ds5000.md/devScsiDevice.o ds5000.md/devSCSITape.o ds5000.md/devConsoleCmd.o ds5000.md/devSCSIHBA.o ds5000.md/devSyslog.o ds5000.md/devBlockDevice.o ds5000.md/devDiskStats.o ds5000.md/devExabyteTape.o ds5000.md/devSCSIDisk.o ds5000.md/devRawBlockDev.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
