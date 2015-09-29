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
# Fri Jul 26 18:20:14 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/vmSun.c sun4c.md/vmSunAsm.s vmCOW.c vmSeg.c vmStack.c vmBoot.c vmPage.c vmList.c vmMap.c vmMigrate.c vmServer.c vmSubr.c vmTrace.c vmSwapDir.c vmStubs.c vmSysCall.c vmPrefetch.c
HDRS		= sun4c.md/vmMach.h sun4c.md/vmMachInt.h sun4c.md/vmMachStat.h sun4c.md/vmMachTrace.h sun4c.md/vmSunConst.h lock.h vm.h vmHack.h vmInt.h vmSwapDir.h vmTrace.h vmUnixStubs.h
MDPUBHDRS	= sun4c.md/vmMach.h sun4c.md/vmMachStat.h sun4c.md/vmMachTrace.h sun4c.md/vmSunConst.h
OBJS		= sun4c.md/vmBoot.o sun4c.md/vmCOW.o sun4c.md/vmList.o sun4c.md/vmMap.o sun4c.md/vmMigrate.o sun4c.md/vmPage.o sun4c.md/vmPrefetch.o sun4c.md/vmSeg.o sun4c.md/vmServer.o sun4c.md/vmStack.o sun4c.md/vmStubs.o sun4c.md/vmSubr.o sun4c.md/vmSun.o sun4c.md/vmSunAsm.o sun4c.md/vmSwapDir.o sun4c.md/vmSysCall.o sun4c.md/vmTrace.o
CLEANOBJS	= sun4c.md/vmSun.o sun4c.md/vmSunAsm.o sun4c.md/vmCOW.o sun4c.md/vmSeg.o sun4c.md/vmStack.o sun4c.md/vmBoot.o sun4c.md/vmPage.o sun4c.md/vmList.o sun4c.md/vmMap.o sun4c.md/vmMigrate.o sun4c.md/vmServer.o sun4c.md/vmSubr.o sun4c.md/vmTrace.o sun4c.md/vmSwapDir.o sun4c.md/vmStubs.o sun4c.md/vmSysCall.o sun4c.md/vmPrefetch.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
