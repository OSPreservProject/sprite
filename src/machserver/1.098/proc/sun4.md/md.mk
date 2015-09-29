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
# Fri Jul 26 18:18:12 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/procMach.c procDebug.c procExit.c procID.c procExec.c procTable.c procFork.c procMisc.c procTimer.c machType68k.c machTypeMips.c procMigrate.c machTypeSparc.c machTypeSpur.c procRecovery.c procRpc.c procStubs.c procRemote.c procServer.c procEnviron.c machTypeSymm.c procFamily.c
HDRS		= sun4.md/migVersion.h sun4.md/procMach.h file.h migrate.h proc.h procInt.h procMigrate.h procServer.h procTypes.h procUnixStubs.h
MDPUBHDRS	= sun4.md/procMach.h
OBJS		= sun4.md/machType68k.o sun4.md/machTypeMips.o sun4.md/machTypeSparc.o sun4.md/machTypeSpur.o sun4.md/machTypeSymm.o sun4.md/procDebug.o sun4.md/procEnviron.o sun4.md/procExec.o sun4.md/procExit.o sun4.md/procFamily.o sun4.md/procFork.o sun4.md/procID.o sun4.md/procMach.o sun4.md/procMigrate.o sun4.md/procMisc.o sun4.md/procRecovery.o sun4.md/procRemote.o sun4.md/procRpc.o sun4.md/procServer.o sun4.md/procStubs.o sun4.md/procTable.o sun4.md/procTimer.o
CLEANOBJS	= sun4.md/procMach.o sun4.md/procDebug.o sun4.md/procExit.o sun4.md/procID.o sun4.md/procExec.o sun4.md/procTable.o sun4.md/procFork.o sun4.md/procMisc.o sun4.md/procTimer.o sun4.md/machType68k.o sun4.md/machTypeMips.o sun4.md/procMigrate.o sun4.md/machTypeSparc.o sun4.md/machTypeSpur.o sun4.md/procRecovery.o sun4.md/procRpc.o sun4.md/procStubs.o sun4.md/procRemote.o sun4.md/procServer.o sun4.md/procEnviron.o sun4.md/machTypeSymm.o sun4.md/procFamily.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
