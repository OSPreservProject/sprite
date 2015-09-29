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
# Wed Aug 21 12:51:35 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= funmap.c history.c keymaps.c readline.c
HDRS		= chardefs.h history.h keymaps.h readline.h
MDPUBHDRS	= 
OBJS		= ds3100.md/funmap.o ds3100.md/history.o ds3100.md/keymaps.o ds3100.md/readline.o
CLEANOBJS	= ds3100.md/funmap.o ds3100.md/history.o ds3100.md/keymaps.o ds3100.md/readline.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
