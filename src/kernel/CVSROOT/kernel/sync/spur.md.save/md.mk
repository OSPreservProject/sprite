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
# Thu Aug 23 15:56:33 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= syncLock.c syncLockStat.c syncSleep.c syncStat.c syncSysV.c syncUser.c
HDRS		= sync.h syncInt.h syncLock.h
MDPUBHDRS	= 
OBJS		= spur.md/syncLock.o spur.md/syncLockStat.o spur.md/syncSleep.o spur.md/syncStat.o spur.md/syncSysV.o spur.md/syncUser.o
CLEANOBJS	= spur.md/syncLock.o spur.md/syncLockStat.o spur.md/syncSleep.o spur.md/syncStat.o spur.md/syncSysV.o spur.md/syncUser.o
INSTFILES	= spur.md/md.mk spur.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
