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
# Sun Aug 11 17:48:51 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/devConfig.c sun4c.md/devConsole.c sun4c.md/devInit.c sun4c.md/devMouse.c sun4c.md/devEmulexTape.c sun4c.md/devTtyAttach.c sun4c.md/devVidSun4.s sun4c.md/devZ8530.c sun4c.md/devSCSIC90.c sun4c.md/devGraphics.c sun4c.md/devFsOpTable.c sun4c.md/devScsiTapeConfig.c sun4c.md/devSCSIC90Mach.c devNet.c devTty.c devNull.c devQueue.c devSCSI.c devScsiDevice.c devSCSITape.c devConsoleCmd.c devSCSIHBA.c devSyslog.c devBlockDevice.c devDiskStats.c devExabyteTape.c devSCSIDisk.c devRawBlockDev.c
HDRS		= sun4c.md/console.h sun4c.md/devAddrs.h sun4c.md/devInt.h sun4c.md/devMultibus.h sun4c.md/devSCSIC90.h sun4c.md/devSCSIC90Int.h sun4c.md/devSCSIC90Mach.h sun4c.md/devfb.h sun4c.md/emulexTape.h sun4c.md/mouse.h sun4c.md/scsiC90.h sun4c.md/ttyAttach.h sun4c.md/z8530.h dev.h devBlockDevice.h devDiskLabel.h devDiskStats.h devFsOpTable.h devNet.h devNull.h devQueue.h devSCSIDisk.h devSCSITape.h devSyslog.h devTypes.h devVid.h diskStats.h exabyteTape.h rawBlockDev.h scsiDevice.h scsiHBA.h scsiHBADevice.h scsiTape.h tty.h
MDPUBHDRS	= sun4c.md/devAddrs.h sun4c.md/devMultibus.h sun4c.md/devSCSIC90.h sun4c.md/devSCSIC90Mach.h sun4c.md/devfb.h
OBJS		= sun4c.md/devBlockDevice.o sun4c.md/devConfig.o sun4c.md/devConsole.o sun4c.md/devConsoleCmd.o sun4c.md/devDiskStats.o sun4c.md/devEmulexTape.o sun4c.md/devExabyteTape.o sun4c.md/devFsOpTable.o sun4c.md/devGraphics.o sun4c.md/devInit.o sun4c.md/devMouse.o sun4c.md/devNet.o sun4c.md/devNull.o sun4c.md/devQueue.o sun4c.md/devRawBlockDev.o sun4c.md/devSCSI.o sun4c.md/devSCSIC90.o sun4c.md/devSCSIC90Mach.o sun4c.md/devSCSIDisk.o sun4c.md/devSCSIHBA.o sun4c.md/devSCSITape.o sun4c.md/devScsiDevice.o sun4c.md/devScsiTapeConfig.o sun4c.md/devSyslog.o sun4c.md/devTty.o sun4c.md/devTtyAttach.o sun4c.md/devVidSun4.o sun4c.md/devZ8530.o
CLEANOBJS	= sun4c.md/devConfig.o sun4c.md/devConsole.o sun4c.md/devInit.o sun4c.md/devMouse.o sun4c.md/devEmulexTape.o sun4c.md/devTtyAttach.o sun4c.md/devVidSun4.o sun4c.md/devZ8530.o sun4c.md/devSCSIC90.o sun4c.md/devGraphics.o sun4c.md/devFsOpTable.o sun4c.md/devScsiTapeConfig.o sun4c.md/devSCSIC90Mach.o sun4c.md/devNet.o sun4c.md/devTty.o sun4c.md/devNull.o sun4c.md/devQueue.o sun4c.md/devSCSI.o sun4c.md/devScsiDevice.o sun4c.md/devSCSITape.o sun4c.md/devConsoleCmd.o sun4c.md/devSCSIHBA.o sun4c.md/devSyslog.o sun4c.md/devBlockDevice.o sun4c.md/devDiskStats.o sun4c.md/devExabyteTape.o sun4c.md/devSCSIDisk.o sun4c.md/devRawBlockDev.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
