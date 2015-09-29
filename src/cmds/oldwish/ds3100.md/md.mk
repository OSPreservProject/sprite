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
# Tue Oct 16 13:49:10 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= whence.c wishCmd.c wishDisplay.c wishGarbage.c wishGather.c wishHandlers.c wishMain.c wishScroll.c wishSelect.c wishUtils.c
HDRS		= wishInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/whence.o ds3100.md/wishCmd.o ds3100.md/wishDisplay.o ds3100.md/wishGarbage.o ds3100.md/wishGather.o ds3100.md/wishHandlers.o ds3100.md/wishMain.o ds3100.md/wishScroll.o ds3100.md/wishSelect.o ds3100.md/wishUtils.o
CLEANOBJS	= ds3100.md/whence.o ds3100.md/wishCmd.o ds3100.md/wishDisplay.o ds3100.md/wishGarbage.o ds3100.md/wishGather.o ds3100.md/wishHandlers.o ds3100.md/wishMain.o ds3100.md/wishScroll.o ds3100.md/wishSelect.o ds3100.md/wishUtils.o
INSTFILES	= Makefile local.mk tags
SACREDOBJS	= 
