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
# Thu Aug 23 16:09:10 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= spur.md/vmSpur.c spur.md/vmSpurAsm.s vmBoot.c vmCOW.c vmList.c vmMap.c vmMigrate.c vmPage.c vmPrefetch.c vmSeg.c vmServer.c vmStack.c vmSubr.c vmSwapDir.c vmSysCall.c vmTrace.c
HDRS		= spur.md/vmMach.h spur.md/vmMachInt.h spur.md/vmMachStat.h spur.md/vmMachTrace.h spur.md/vmSpurConst.h lock.h vm.h vmInt.h vmSwapDir.h vmTrace.h
MDPUBHDRS	= spur.md/vmMach.h spur.md/vmMachStat.h spur.md/vmMachTrace.h spur.md/vmSpurConst.h
OBJS		= spur.md/vmSpur.o spur.md/vmSpurAsm.o spur.md/vmBoot.o spur.md/vmCOW.o spur.md/vmList.o spur.md/vmMap.o spur.md/vmMigrate.o spur.md/vmPage.o spur.md/vmPrefetch.o spur.md/vmSeg.o spur.md/vmServer.o spur.md/vmStack.o spur.md/vmSubr.o spur.md/vmSwapDir.o spur.md/vmSysCall.o spur.md/vmTrace.o
CLEANOBJS	= spur.md/vmSpur.o spur.md/vmSpurAsm.o spur.md/vmBoot.o spur.md/vmCOW.o spur.md/vmList.o spur.md/vmMap.o spur.md/vmMigrate.o spur.md/vmPage.o spur.md/vmPrefetch.o spur.md/vmSeg.o spur.md/vmServer.o spur.md/vmStack.o spur.md/vmSubr.o spur.md/vmSwapDir.o spur.md/vmSysCall.o spur.md/vmTrace.o
INSTFILES	= spur.md/md.mk spur.md/md.mk.sed spur.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
XCFLAGS	+= -fvolatile
