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
# Thu Feb 13 21:18:46 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= vmOps.c vmPager.c vmSegment.c vmSwapDir.c vmFsCache.c vmSubr.c vmMsgQueue.c vmSysCall.c
HDRS		= sun3.md/vmMachStat.h vm.h vmInt.h vmSwapDir.h vmTypes.h
MDPUBHDRS	= sun3.md/vmMachStat.h
OBJS		= sun3.md/vmFsCache.o sun3.md/vmMsgQueue.o sun3.md/vmOps.o sun3.md/vmPager.o sun3.md/vmSegment.o sun3.md/vmSubr.o sun3.md/vmSwapDir.o sun3.md/vmSysCall.o
CLEANOBJS	= sun3.md/vmOps.o sun3.md/vmPager.o sun3.md/vmSegment.o sun3.md/vmSwapDir.o sun3.md/vmFsCache.o sun3.md/vmSubr.o sun3.md/vmMsgQueue.o sun3.md/vmSysCall.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	=
