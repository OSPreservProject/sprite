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
# Fri Jul 26 18:17:54 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/procMach.c procDebug.c procExit.c procID.c procExec.c procTable.c procFork.c procMisc.c procTimer.c machType68k.c machTypeMips.c procMigrate.c machTypeSparc.c machTypeSpur.c procRecovery.c procRpc.c procStubs.c procRemote.c procServer.c procEnviron.c machTypeSymm.c procFamily.c
HDRS		= sun3.md/a.out.h sun3.md/migVersion.h sun3.md/procMach.h file.h migrate.h proc.h procInt.h procMigrate.h procServer.h procTypes.h procUnixStubs.h
MDPUBHDRS	= sun3.md/procMach.h
OBJS		= sun3.md/machType68k.o sun3.md/machTypeMips.o sun3.md/machTypeSparc.o sun3.md/machTypeSpur.o sun3.md/machTypeSymm.o sun3.md/procDebug.o sun3.md/procEnviron.o sun3.md/procExec.o sun3.md/procExit.o sun3.md/procFamily.o sun3.md/procFork.o sun3.md/procID.o sun3.md/procMach.o sun3.md/procMigrate.o sun3.md/procMisc.o sun3.md/procRecovery.o sun3.md/procRemote.o sun3.md/procRpc.o sun3.md/procServer.o sun3.md/procStubs.o sun3.md/procTable.o sun3.md/procTimer.o
CLEANOBJS	= sun3.md/procMach.o sun3.md/procDebug.o sun3.md/procExit.o sun3.md/procID.o sun3.md/procExec.o sun3.md/procTable.o sun3.md/procFork.o sun3.md/procMisc.o sun3.md/procTimer.o sun3.md/machType68k.o sun3.md/machTypeMips.o sun3.md/procMigrate.o sun3.md/machTypeSparc.o sun3.md/machTypeSpur.o sun3.md/procRecovery.o sun3.md/procRpc.o sun3.md/procStubs.o sun3.md/procRemote.o sun3.md/procServer.o sun3.md/procEnviron.o sun3.md/machTypeSymm.o sun3.md/procFamily.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
