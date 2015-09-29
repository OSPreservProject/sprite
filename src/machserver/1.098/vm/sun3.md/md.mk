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
# Fri Jul 26 18:19:45 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/vmSun.c sun3.md/vmSunAsm.s vmCOW.c vmSeg.c vmStack.c vmBoot.c vmPage.c vmList.c vmMap.c vmMigrate.c vmServer.c vmSubr.c vmTrace.c vmSwapDir.c vmStubs.c vmSysCall.c vmPrefetch.c
HDRS		= sun3.md/vmMach.h sun3.md/vmMachInt.h sun3.md/vmMachStat.h sun3.md/vmMachTrace.h sun3.md/vmSunConst.h lock.h vm.h vmHack.h vmInt.h vmSwapDir.h vmTrace.h vmUnixStubs.h
MDPUBHDRS	= sun3.md/vmMach.h sun3.md/vmMachStat.h sun3.md/vmMachTrace.h sun3.md/vmSunConst.h
OBJS		= sun3.md/vmBoot.o sun3.md/vmCOW.o sun3.md/vmList.o sun3.md/vmMap.o sun3.md/vmMigrate.o sun3.md/vmPage.o sun3.md/vmPrefetch.o sun3.md/vmSeg.o sun3.md/vmServer.o sun3.md/vmStack.o sun3.md/vmStubs.o sun3.md/vmSubr.o sun3.md/vmSun.o sun3.md/vmSunAsm.o sun3.md/vmSwapDir.o sun3.md/vmSysCall.o sun3.md/vmTrace.o
CLEANOBJS	= sun3.md/vmSun.o sun3.md/vmSunAsm.o sun3.md/vmCOW.o sun3.md/vmSeg.o sun3.md/vmStack.o sun3.md/vmBoot.o sun3.md/vmPage.o sun3.md/vmList.o sun3.md/vmMap.o sun3.md/vmMigrate.o sun3.md/vmServer.o sun3.md/vmSubr.o sun3.md/vmTrace.o sun3.md/vmSwapDir.o sun3.md/vmStubs.o sun3.md/vmSysCall.o sun3.md/vmPrefetch.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
