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
# Mon Apr 27 17:03:14 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/procMach.c procDebug.c procExec.c procExit.c procFork.c procID.c procMisc.c procTable.c procEnviron.c procFamily.c procRpc.c procTaskThread.c procTimer.c procServer.c procSysCall.c procMigrate.c
HDRS		= ds3100.md/migVersion.h ds3100.md/procMach.h ds3100.md/procMachInt.h migrate.h proc.h procInt.h procMigrate.h procServer.h procTypes.h procUnixStubs.h
MDPUBHDRS	= ds3100.md/procMach.h
OBJS		= ds3100.md/procDebug.o ds3100.md/procEnviron.o ds3100.md/procExec.o ds3100.md/procExit.o ds3100.md/procFamily.o ds3100.md/procFork.o ds3100.md/procID.o ds3100.md/procMach.o ds3100.md/procMisc.o ds3100.md/procRpc.o ds3100.md/procServer.o ds3100.md/procSysCall.o ds3100.md/procTable.o ds3100.md/procTaskThread.o ds3100.md/procTimer.o ds3100.md/procMigrate.o
CLEANOBJS	= ds3100.md/procMach.o ds3100.md/procDebug.o ds3100.md/procExec.o ds3100.md/procExit.o ds3100.md/procFork.o ds3100.md/procID.o ds3100.md/procMisc.o ds3100.md/procTable.o ds3100.md/procEnviron.o ds3100.md/procFamily.o ds3100.md/procRpc.o ds3100.md/procTaskThread.o ds3100.md/procTimer.o ds3100.md/procServer.o ds3100.md/procSysCall.o ds3100.md/procMigrate.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
