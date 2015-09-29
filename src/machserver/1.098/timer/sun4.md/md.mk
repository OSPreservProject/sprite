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
# Tue Jul  2 14:16:47 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4.md/timerTick.c sun4.md/timerIntersil.c timerClock.c timerQueue.c timerStubs.c
HDRS		= sun4.md/timerIntersilInt.h sun4.md/timerMach.h sun4.md/timerTick.h timer.h timerInt.h timerUnixStubs.h
MDPUBHDRS	= sun4.md/timerMach.h sun4.md/timerTick.h
OBJS		= sun4.md/timerClock.o sun4.md/timerIntersil.o sun4.md/timerQueue.o sun4.md/timerStubs.o sun4.md/timerTick.o
CLEANOBJS	= sun4.md/timerTick.o sun4.md/timerIntersil.o sun4.md/timerClock.o sun4.md/timerQueue.o sun4.md/timerStubs.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
