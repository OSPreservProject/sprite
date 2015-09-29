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
# Thu Feb 13 21:22:13 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= vmOps.c vmPager.c vmSegment.c vmSwapDir.c vmFsCache.c vmSubr.c vmMsgQueue.c vmSysCall.c
HDRS		= ds3100.md/vmMachStat.h vm.h vmInt.h vmSwapDir.h vmTypes.h
MDPUBHDRS	= ds3100.md/vmMachStat.h
OBJS		= ds3100.md/vmFsCache.o ds3100.md/vmMsgQueue.o ds3100.md/vmOps.o ds3100.md/vmPager.o ds3100.md/vmSegment.o ds3100.md/vmSubr.o ds3100.md/vmSwapDir.o ds3100.md/vmSysCall.o
CLEANOBJS	= ds3100.md/vmOps.o ds3100.md/vmPager.o ds3100.md/vmSegment.o ds3100.md/vmSwapDir.o ds3100.md/vmFsCache.o ds3100.md/vmSubr.o ds3100.md/vmMsgQueue.o ds3100.md/vmSysCall.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
