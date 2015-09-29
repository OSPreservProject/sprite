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
# Wed Jan  6 15:08:45 PST 1993
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= xmalloc.c getopt1.c stpcpy.c error.c getopt.c cat.c
HDRS		= getopt.h system.h
MDPUBHDRS	= 
OBJS		= sun4.md/xmalloc.o sun4.md/getopt1.o sun4.md/stpcpy.o sun4.md/error.o sun4.md/getopt.o sun4.md/cat.o
CLEANOBJS	= sun4.md/xmalloc.o sun4.md/getopt1.o sun4.md/stpcpy.o sun4.md/error.o sun4.md/getopt.o sun4.md/cat.o
INSTFILES	= Makefile
SACREDOBJS	= 
