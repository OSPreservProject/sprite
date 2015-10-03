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
# Thu Aug 23 16:02:30 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= spur.md/timerSpur.c spur.md/timerSpurT0.s spur.md/timerTick.c timerClock.c timerQueue.c
HDRS		= spur.md/timerMach.h spur.md/timerSpurInt.h spur.md/timerTick.h timer.h timerInt.h
MDPUBHDRS	= spur.md/timerMach.h spur.md/timerTick.h
OBJS		= spur.md/timerSpur.o spur.md/timerSpurT0.o spur.md/timerTick.o spur.md/timerClock.o spur.md/timerQueue.o
CLEANOBJS	= spur.md/timerSpur.o spur.md/timerSpurT0.o spur.md/timerTick.o spur.md/timerClock.o spur.md/timerQueue.o
INSTFILES	= spur.md/md.mk spur.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
