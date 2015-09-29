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
# Fri Jul 26 18:17:39 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/procMach.c procDebug.c procExit.c procID.c procExec.c procTable.c procFork.c procMisc.c procTimer.c machType68k.c machTypeMips.c procMigrate.c machTypeSparc.c machTypeSpur.c procRecovery.c procRpc.c procStubs.c procRemote.c procServer.c procEnviron.c machTypeSymm.c procFamily.c
HDRS		= ds5000.md/migVersion.h ds5000.md/procMach.h file.h migrate.h proc.h procInt.h procMigrate.h procServer.h procTypes.h procUnixStubs.h
MDPUBHDRS	= ds5000.md/procMach.h
OBJS		= ds5000.md/machType68k.o ds5000.md/machTypeMips.o ds5000.md/machTypeSparc.o ds5000.md/machTypeSpur.o ds5000.md/machTypeSymm.o ds5000.md/procDebug.o ds5000.md/procEnviron.o ds5000.md/procExec.o ds5000.md/procExit.o ds5000.md/procFamily.o ds5000.md/procFork.o ds5000.md/procID.o ds5000.md/procMach.o ds5000.md/procMigrate.o ds5000.md/procMisc.o ds5000.md/procRecovery.o ds5000.md/procRemote.o ds5000.md/procRpc.o ds5000.md/procServer.o ds5000.md/procStubs.o ds5000.md/procTable.o ds5000.md/procTimer.o
CLEANOBJS	= ds5000.md/procMach.o ds5000.md/procDebug.o ds5000.md/procExit.o ds5000.md/procID.o ds5000.md/procExec.o ds5000.md/procTable.o ds5000.md/procFork.o ds5000.md/procMisc.o ds5000.md/procTimer.o ds5000.md/machType68k.o ds5000.md/machTypeMips.o ds5000.md/procMigrate.o ds5000.md/machTypeSparc.o ds5000.md/machTypeSpur.o ds5000.md/procRecovery.o ds5000.md/procRpc.o ds5000.md/procStubs.o ds5000.md/procRemote.o ds5000.md/procServer.o ds5000.md/procEnviron.o ds5000.md/machTypeSymm.o ds5000.md/procFamily.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
