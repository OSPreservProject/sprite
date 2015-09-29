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
# Fri Nov  3 18:31:35 PST 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= List_Init.c List_Insert.c List_ListInsert.c List_Move.c List_Remove.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/List_Init.o ds3100.md/List_Insert.o ds3100.md/List_ListInsert.o ds3100.md/List_Move.o ds3100.md/List_Remove.o
CLEANOBJS	= ds3100.md/List_Init.o ds3100.md/List_Insert.o ds3100.md/List_ListInsert.o ds3100.md/List_Move.o ds3100.md/List_Remove.o
DISTDIR        ?= @(DISTDIR)
