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
# Wed May  8 17:22:12 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= args.c indent.c io.c lexi.c parse.c pr_comment.c
HDRS		= indent_codes.h indent_globs.h
MDPUBHDRS	= 
OBJS		= ds3100.md/args.o ds3100.md/indent.o ds3100.md/io.o ds3100.md/lexi.o ds3100.md/parse.o ds3100.md/pr_comment.o
CLEANOBJS	= ds3100.md/args.o ds3100.md/indent.o ds3100.md/io.o ds3100.md/lexi.o ds3100.md/parse.o ds3100.md/pr_comment.o
INSTFILES	= Makefile
SACREDOBJS	= 
