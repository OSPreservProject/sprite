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
# Tue Jul  2 14:16:51 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/timerTick.c sun4c.md/timerSun4c.c timerClock.c timerQueue.c timerStubs.c
HDRS		= sun4c.md/timerMK48T12Int.h sun4c.md/timerMach.h sun4c.md/timerTick.h timer.h timerInt.h timerUnixStubs.h
MDPUBHDRS	= sun4c.md/timerMach.h sun4c.md/timerTick.h
OBJS		= sun4c.md/timerClock.o sun4c.md/timerQueue.o sun4c.md/timerStubs.o sun4c.md/timerSun4c.o sun4c.md/timerTick.o
CLEANOBJS	= sun4c.md/timerTick.o sun4c.md/timerSun4c.o sun4c.md/timerClock.o sun4c.md/timerQueue.o sun4c.md/timerStubs.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
