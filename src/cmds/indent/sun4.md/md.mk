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
# Tue Dec 19 01:22:38 PST 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= args.c indent.c io.c lexi.c parse.c pr_comment.c
HDRS		= indent_codes.h indent_globs.h
MDPUBHDRS	= 
OBJS		= sun4.md/args.o sun4.md/indent.o sun4.md/io.o sun4.md/lexi.o sun4.md/parse.o sun4.md/pr_comment.o
CLEANOBJS	= sun4.md/args.o sun4.md/indent.o sun4.md/io.o sun4.md/lexi.o sun4.md/parse.o sun4.md/pr_comment.o
DISTDIR        ?= @(DISTDIR)
