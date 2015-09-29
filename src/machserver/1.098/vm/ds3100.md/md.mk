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
# Fri Jul 26 18:19:11 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/vmPmaxAsm.s ds3100.md/vmPmax.c vmCOW.c vmSeg.c vmStack.c vmBoot.c vmPage.c vmList.c vmMap.c vmMigrate.c vmServer.c vmSubr.c vmTrace.c vmSwapDir.c vmStubs.c vmSysCall.c vmPrefetch.c
HDRS		= ds3100.md/vmMach.h ds3100.md/vmMachInt.h ds3100.md/vmMachStat.h ds3100.md/vmMachTrace.h ds3100.md/vmPmaxConst.h lock.h vm.h vmHack.h vmInt.h vmSwapDir.h vmTrace.h vmUnixStubs.h
MDPUBHDRS	= ds3100.md/vmMach.h ds3100.md/vmMachStat.h ds3100.md/vmMachTrace.h ds3100.md/vmPmaxConst.h
OBJS		= ds3100.md/vmBoot.o ds3100.md/vmCOW.o ds3100.md/vmList.o ds3100.md/vmMap.o ds3100.md/vmMigrate.o ds3100.md/vmPage.o ds3100.md/vmPmax.o ds3100.md/vmPmaxAsm.o ds3100.md/vmPrefetch.o ds3100.md/vmSeg.o ds3100.md/vmServer.o ds3100.md/vmStack.o ds3100.md/vmStubs.o ds3100.md/vmSubr.o ds3100.md/vmSwapDir.o ds3100.md/vmSysCall.o ds3100.md/vmTrace.o
CLEANOBJS	= ds3100.md/vmPmaxAsm.o ds3100.md/vmPmax.o ds3100.md/vmCOW.o ds3100.md/vmSeg.o ds3100.md/vmStack.o ds3100.md/vmBoot.o ds3100.md/vmPage.o ds3100.md/vmList.o ds3100.md/vmMap.o ds3100.md/vmMigrate.o ds3100.md/vmServer.o ds3100.md/vmSubr.o ds3100.md/vmTrace.o ds3100.md/vmSwapDir.o ds3100.md/vmStubs.o ds3100.md/vmSysCall.o ds3100.md/vmPrefetch.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
