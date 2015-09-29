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
# Fri Jul 26 18:17:20 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/procMach.c procDebug.c procExit.c procID.c procExec.c procTable.c procFork.c procMisc.c procTimer.c machType68k.c machTypeMips.c procMigrate.c machTypeSparc.c machTypeSpur.c procRecovery.c procRpc.c procStubs.c procRemote.c procServer.c procEnviron.c machTypeSymm.c procFamily.c
HDRS		= ds3100.md/migVersion.h ds3100.md/procMach.h file.h migrate.h proc.h procInt.h procMigrate.h procServer.h procTypes.h procUnixStubs.h
MDPUBHDRS	= ds3100.md/procMach.h
OBJS		= ds3100.md/machType68k.o ds3100.md/machTypeMips.o ds3100.md/machTypeSparc.o ds3100.md/machTypeSpur.o ds3100.md/machTypeSymm.o ds3100.md/procDebug.o ds3100.md/procEnviron.o ds3100.md/procExec.o ds3100.md/procExit.o ds3100.md/procFamily.o ds3100.md/procFork.o ds3100.md/procID.o ds3100.md/procMach.o ds3100.md/procMigrate.o ds3100.md/procMisc.o ds3100.md/procRecovery.o ds3100.md/procRemote.o ds3100.md/procRpc.o ds3100.md/procServer.o ds3100.md/procStubs.o ds3100.md/procTable.o ds3100.md/procTimer.o
CLEANOBJS	= ds3100.md/procMach.o ds3100.md/procDebug.o ds3100.md/procExit.o ds3100.md/procID.o ds3100.md/procExec.o ds3100.md/procTable.o ds3100.md/procFork.o ds3100.md/procMisc.o ds3100.md/procTimer.o ds3100.md/machType68k.o ds3100.md/machTypeMips.o ds3100.md/procMigrate.o ds3100.md/machTypeSparc.o ds3100.md/machTypeSpur.o ds3100.md/procRecovery.o ds3100.md/procRpc.o ds3100.md/procStubs.o ds3100.md/procRemote.o ds3100.md/procServer.o ds3100.md/procEnviron.o ds3100.md/machTypeSymm.o ds3100.md/procFamily.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
