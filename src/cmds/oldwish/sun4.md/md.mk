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
# Fri Jan 19 12:23:55 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= whence.c wishCmd.c wishDisplay.c wishGarbage.c wishGather.c wishHandlers.c wishMain.c wishScroll.c wishSelect.c wishUtils.c
HDRS		= wishInt.h
MDPUBHDRS	= 
OBJS		= sun4.md/whence.o sun4.md/wishCmd.o sun4.md/wishDisplay.o sun4.md/wishGarbage.o sun4.md/wishGather.o sun4.md/wishHandlers.o sun4.md/wishMain.o sun4.md/wishScroll.o sun4.md/wishSelect.o sun4.md/wishUtils.o
CLEANOBJS	= sun4.md/whence.o sun4.md/wishCmd.o sun4.md/wishDisplay.o sun4.md/wishGarbage.o sun4.md/wishGather.o sun4.md/wishHandlers.o sun4.md/wishMain.o sun4.md/wishScroll.o sun4.md/wishSelect.o sun4.md/wishUtils.o
DISTDIR        ?= @(DISTDIR)
