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
# Tue Dec 19 20:23:20 PST 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= dfa.c grep.c regex.c
HDRS		= dfa.h regex.h
MDPUBHDRS	= 
OBJS		= sun4.md/dfa.o sun4.md/grep.o sun4.md/regex.o
CLEANOBJS	= sun4.md/dfa.o sun4.md/grep.o sun4.md/regex.o
DISTDIR        ?= @(DISTDIR)
