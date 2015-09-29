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
# Fri Jul 26 18:18:37 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/procMach.c procDebug.c procExit.c procID.c procExec.c procTable.c procFork.c procMisc.c procTimer.c machType68k.c machTypeMips.c procMigrate.c machTypeSparc.c machTypeSpur.c procRecovery.c procRpc.c procStubs.c procRemote.c procServer.c procEnviron.c machTypeSymm.c procFamily.c
HDRS		= sun4c.md/migVersion.h sun4c.md/procMach.h file.h migrate.h proc.h procInt.h procMigrate.h procServer.h procTypes.h procUnixStubs.h
MDPUBHDRS	= sun4c.md/procMach.h
OBJS		= sun4c.md/machType68k.o sun4c.md/machTypeMips.o sun4c.md/machTypeSparc.o sun4c.md/machTypeSpur.o sun4c.md/machTypeSymm.o sun4c.md/procDebug.o sun4c.md/procEnviron.o sun4c.md/procExec.o sun4c.md/procExit.o sun4c.md/procFamily.o sun4c.md/procFork.o sun4c.md/procID.o sun4c.md/procMach.o sun4c.md/procMigrate.o sun4c.md/procMisc.o sun4c.md/procRecovery.o sun4c.md/procRemote.o sun4c.md/procRpc.o sun4c.md/procServer.o sun4c.md/procStubs.o sun4c.md/procTable.o sun4c.md/procTimer.o
CLEANOBJS	= sun4c.md/procMach.o sun4c.md/procDebug.o sun4c.md/procExit.o sun4c.md/procID.o sun4c.md/procExec.o sun4c.md/procTable.o sun4c.md/procFork.o sun4c.md/procMisc.o sun4c.md/procTimer.o sun4c.md/machType68k.o sun4c.md/machTypeMips.o sun4c.md/procMigrate.o sun4c.md/machTypeSparc.o sun4c.md/machTypeSpur.o sun4c.md/procRecovery.o sun4c.md/procRpc.o sun4c.md/procStubs.o sun4c.md/procRemote.o sun4c.md/procServer.o sun4c.md/procEnviron.o sun4c.md/machTypeSymm.o sun4c.md/procFamily.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
