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
# Tue Oct  3 16:24:07 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.3 88/06/06 17:23:47 ouster Exp Locker: rab $
#
# Allow mkmf

SRCS		= whence.c wishCmd.c wishDisplay.c wishGarbage.c wishGather.c wishHandlers.c wishMain.c wishScroll.c wishSelect.c wishUtils.c
HDRS		= wishInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/whence.o sun3.md/wishCmd.o sun3.md/wishDisplay.o sun3.md/wishGarbage.o sun3.md/wishGather.o sun3.md/wishHandlers.o sun3.md/wishMain.o sun3.md/wishScroll.o sun3.md/wishSelect.o sun3.md/wishUtils.o
CLEANOBJS	= sun3.md/whence.o sun3.md/wishCmd.o sun3.md/wishDisplay.o sun3.md/wishGarbage.o sun3.md/wishGather.o sun3.md/wishHandlers.o sun3.md/wishMain.o sun3.md/wishScroll.o sun3.md/wishSelect.o sun3.md/wishUtils.o
DISTDIR        ?= @(DISTDIR)
