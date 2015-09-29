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
# Fri Feb 10 14:27:30 PST 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: Makefile.md,v 1.3 88/06/06 17:23:47 ouster Exp $
#
# Allow mkmf

SRCS		= syncLock.c syncSleep.c syncStat.c syncUser.c
HDRS		= sync.h syncInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/syncLock.o sun4.md/syncSleep.o sun4.md/syncStat.o sun4.md/syncUser.o
CLEANOBJS	= sun4.md/syncLock.o sun4.md/syncSleep.o sun4.md/syncStat.o sun4.md/syncUser.o
