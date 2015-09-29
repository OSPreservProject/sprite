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
# Tue Mar 13 20:42:12 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4nw.md/procMach.c machType68k.c machTypeMips.c machTypeSparc.c machTypeSpur.c procDebug.c procEnviron.c procExec.c procExit.c procFamily.c procFork.c procID.c procMigrate.c procMisc.c procRecovery.c procRemote.c procRpc.c procServer.c procTable.c procTimer.c
HDRS		= sun4nw.md/migVersion.h sun4nw.md/procMach.h file.h migrate.h proc.h procInt.h procMigrate.h
MDPUBHDRS	= sun4nw.md/procMach.h
OBJS		= sun4nw.md/procDebug.o sun4nw.md/procEnviron.o sun4nw.md/procExec.o sun4nw.md/procExit.o sun4nw.md/procFamily.o sun4nw.md/procFork.o sun4nw.md/procID.o sun4nw.md/procMach.o sun4nw.md/procMigrate.o sun4nw.md/procMisc.o sun4nw.md/procRecovery.o sun4nw.md/procRemote.o sun4nw.md/procRpc.o sun4nw.md/procServer.o sun4nw.md/procTable.o sun4nw.md/procTimer.o sun4nw.md/machType68k.o sun4nw.md/machTypeMips.o sun4nw.md/machTypeSparc.o sun4nw.md/machTypeSpur.o
CLEANOBJS	= sun4nw.md/procMach.o sun4nw.md/machType68k.o sun4nw.md/machTypeMips.o sun4nw.md/machTypeSparc.o sun4nw.md/machTypeSpur.o sun4nw.md/procDebug.o sun4nw.md/procEnviron.o sun4nw.md/procExec.o sun4nw.md/procExit.o sun4nw.md/procFamily.o sun4nw.md/procFork.o sun4nw.md/procID.o sun4nw.md/procMigrate.o sun4nw.md/procMisc.o sun4nw.md/procRecovery.o sun4nw.md/procRemote.o sun4nw.md/procRpc.o sun4nw.md/procServer.o sun4nw.md/procTable.o sun4nw.md/procTimer.o
INSTFILES	= sun4nw.md/md.mk sun4nw.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
