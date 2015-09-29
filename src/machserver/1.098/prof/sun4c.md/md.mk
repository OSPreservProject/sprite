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
# Tue Jul  2 14:01:14 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun4c.md/mcount.s sun4c.md/profStack.s sun4c.md/profSubr.c sun4c.md/_mcount.c profMigrate.c profProfil.c
HDRS		= sun4c.md/profInt.h prof.h
MDPUBHDRS	= 
OBJS		= sun4c.md/_mcount.o sun4c.md/mcount.o sun4c.md/profMigrate.o sun4c.md/profProfil.o sun4c.md/profStack.o sun4c.md/profSubr.o
CLEANOBJS	= sun4c.md/mcount.o sun4c.md/profStack.o sun4c.md/profSubr.o sun4c.md/_mcount.o sun4c.md/profMigrate.o sun4c.md/profProfil.o
INSTFILES	= sun4c.md/md.mk sun4c.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
