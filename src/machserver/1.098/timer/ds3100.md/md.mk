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
# Tue Jul  2 14:16:34 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/timerMC.c ds3100.md/timerTick.c timerClock.c timerQueue.c timerStubs.c
HDRS		= ds3100.md/timerMach.h ds3100.md/timerTick.h timer.h timerInt.h timerUnixStubs.h
MDPUBHDRS	= ds3100.md/timerMach.h ds3100.md/timerTick.h
OBJS		= ds3100.md/timerClock.o ds3100.md/timerMC.o ds3100.md/timerQueue.o ds3100.md/timerStubs.o ds3100.md/timerTick.o
CLEANOBJS	= ds3100.md/timerMC.o ds3100.md/timerTick.o ds3100.md/timerClock.o ds3100.md/timerQueue.o ds3100.md/timerStubs.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
