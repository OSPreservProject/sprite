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
# Thu Mar 26 19:13:15 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/devFsOpTable.c devTty.c devNull.c devMachKern.c devUnixConsole.c devSyslog.c
HDRS		= dev.h devBlockDevice.h devDiskLabel.h devFsOpTable.h devInt.h devNull.h devSyslog.h devTypes.h tty.h
MDPUBHDRS	= 
OBJS		= sun3.md/devFsOpTable.o sun3.md/devMachKern.o sun3.md/devNull.o sun3.md/devTty.o sun3.md/devUnixConsole.o sun3.md/devSyslog.o
CLEANOBJS	= sun3.md/devFsOpTable.o sun3.md/devTty.o sun3.md/devNull.o sun3.md/devMachKern.o sun3.md/devUnixConsole.o sun3.md/devSyslog.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
