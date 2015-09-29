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
# Mon Apr 27 17:02:05 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/procMach.c procDebug.c procExec.c procExit.c procFork.c procID.c procMisc.c procTable.c procEnviron.c procFamily.c procRpc.c procTaskThread.c procTimer.c procServer.c procSysCall.c procMigrate.c
HDRS		= sun3.md/migVersion.h sun3.md/procMach.h sun3.md/procMachInt.h migrate.h proc.h procInt.h procMigrate.h procServer.h procTypes.h procUnixStubs.h
MDPUBHDRS	= sun3.md/procMach.h
OBJS		= sun3.md/procDebug.o sun3.md/procEnviron.o sun3.md/procExec.o sun3.md/procExit.o sun3.md/procFamily.o sun3.md/procFork.o sun3.md/procID.o sun3.md/procMach.o sun3.md/procMisc.o sun3.md/procRpc.o sun3.md/procServer.o sun3.md/procSysCall.o sun3.md/procTable.o sun3.md/procTaskThread.o sun3.md/procTimer.o sun3.md/procMigrate.o
CLEANOBJS	= sun3.md/procMach.o sun3.md/procDebug.o sun3.md/procExec.o sun3.md/procExit.o sun3.md/procFork.o sun3.md/procID.o sun3.md/procMisc.o sun3.md/procTable.o sun3.md/procEnviron.o sun3.md/procFamily.o sun3.md/procRpc.o sun3.md/procTaskThread.o sun3.md/procTimer.o sun3.md/procServer.o sun3.md/procSysCall.o sun3.md/procMigrate.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
