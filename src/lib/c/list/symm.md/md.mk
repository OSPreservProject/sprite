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
# Tue Nov 10 15:47:25 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= List_Init.c List_Move.c List_Insert.c List_ListInsert.c List_Remove.c List_Verify.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= symm.md/List_Init.o symm.md/List_Move.o symm.md/List_Insert.o symm.md/List_ListInsert.o symm.md/List_Remove.o symm.md/List_Verify.o
CLEANOBJS	= symm.md/List_Init.o symm.md/List_Move.o symm.md/List_Insert.o symm.md/List_ListInsert.o symm.md/List_Remove.o symm.md/List_Verify.o
INSTFILES	= symm.md/md.mk symm.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
