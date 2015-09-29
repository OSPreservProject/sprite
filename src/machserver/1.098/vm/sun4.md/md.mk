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
# Fri Jul 26 18:20:00 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/vmSun.c sun4.md/vmSunAsm.s vmCOW.c vmSeg.c vmStack.c vmBoot.c vmPage.c vmList.c vmMap.c vmMigrate.c vmServer.c vmSubr.c vmTrace.c vmSwapDir.c vmStubs.c vmSysCall.c vmPrefetch.c
HDRS		= sun4.md/vmMach.h sun4.md/vmMachInt.h sun4.md/vmMachStat.h sun4.md/vmMachTrace.h sun4.md/vmSunConst.h lock.h vm.h vmHack.h vmInt.h vmSwapDir.h vmTrace.h vmUnixStubs.h
MDPUBHDRS	= sun4.md/vmMach.h sun4.md/vmMachStat.h sun4.md/vmMachTrace.h sun4.md/vmSunConst.h
OBJS		= sun4.md/vmBoot.o sun4.md/vmCOW.o sun4.md/vmList.o sun4.md/vmMap.o sun4.md/vmMigrate.o sun4.md/vmPage.o sun4.md/vmPrefetch.o sun4.md/vmSeg.o sun4.md/vmServer.o sun4.md/vmStack.o sun4.md/vmStubs.o sun4.md/vmSubr.o sun4.md/vmSun.o sun4.md/vmSunAsm.o sun4.md/vmSwapDir.o sun4.md/vmSysCall.o sun4.md/vmTrace.o
CLEANOBJS	= sun4.md/vmSun.o sun4.md/vmSunAsm.o sun4.md/vmCOW.o sun4.md/vmSeg.o sun4.md/vmStack.o sun4.md/vmBoot.o sun4.md/vmPage.o sun4.md/vmList.o sun4.md/vmMap.o sun4.md/vmMigrate.o sun4.md/vmServer.o sun4.md/vmSubr.o sun4.md/vmTrace.o sun4.md/vmSwapDir.o sun4.md/vmStubs.o sun4.md/vmSysCall.o sun4.md/vmPrefetch.o
INSTFILES	= sun4.md/md.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
