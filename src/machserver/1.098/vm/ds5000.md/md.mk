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
# Fri Jul 26 18:19:28 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds5000.md/vm3max.c ds5000.md/vm3maxAsm.s vmCOW.c vmSeg.c vmStack.c vmBoot.c vmPage.c vmList.c vmMap.c vmMigrate.c vmServer.c vmSubr.c vmTrace.c vmSwapDir.c vmStubs.c vmSysCall.c vmPrefetch.c
HDRS		= ds5000.md/vm3maxConst.h ds5000.md/vmMach.h ds5000.md/vmMachInt.h ds5000.md/vmMachStat.h ds5000.md/vmMachTrace.h lock.h vm.h vmHack.h vmInt.h vmSwapDir.h vmTrace.h vmUnixStubs.h
MDPUBHDRS	= ds5000.md/vm3maxConst.h ds5000.md/vmMach.h ds5000.md/vmMachStat.h ds5000.md/vmMachTrace.h
OBJS		= ds5000.md/vm3max.o ds5000.md/vm3maxAsm.o ds5000.md/vmBoot.o ds5000.md/vmCOW.o ds5000.md/vmList.o ds5000.md/vmMap.o ds5000.md/vmMigrate.o ds5000.md/vmPage.o ds5000.md/vmPrefetch.o ds5000.md/vmSeg.o ds5000.md/vmServer.o ds5000.md/vmStack.o ds5000.md/vmStubs.o ds5000.md/vmSubr.o ds5000.md/vmSwapDir.o ds5000.md/vmSysCall.o ds5000.md/vmTrace.o
CLEANOBJS	= ds5000.md/vm3max.o ds5000.md/vm3maxAsm.o ds5000.md/vmCOW.o ds5000.md/vmSeg.o ds5000.md/vmStack.o ds5000.md/vmBoot.o ds5000.md/vmPage.o ds5000.md/vmList.o ds5000.md/vmMap.o ds5000.md/vmMigrate.o ds5000.md/vmServer.o ds5000.md/vmSubr.o ds5000.md/vmTrace.o ds5000.md/vmSwapDir.o ds5000.md/vmStubs.o ds5000.md/vmSysCall.o ds5000.md/vmPrefetch.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
