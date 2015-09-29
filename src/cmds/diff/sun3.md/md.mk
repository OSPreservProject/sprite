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
# Thu Dec 17 16:58:52 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= analyze.c context.c diff.c dir.c ed.c getopt.c getopt1.c ifdef.c io.c normal.c regex.c util.c version.c
HDRS		= diff.h getopt.h limits.h regex.h
MDPUBHDRS	= 
OBJS		= sun3.md/analyze.o sun3.md/context.o sun3.md/diff.o sun3.md/dir.o sun3.md/ed.o sun3.md/getopt.o sun3.md/getopt1.o sun3.md/ifdef.o sun3.md/io.o sun3.md/normal.o sun3.md/regex.o sun3.md/util.o sun3.md/version.o
CLEANOBJS	= sun3.md/analyze.o sun3.md/context.o sun3.md/diff.o sun3.md/dir.o sun3.md/ed.o sun3.md/getopt.o sun3.md/getopt1.o sun3.md/ifdef.o sun3.md/io.o sun3.md/normal.o sun3.md/regex.o sun3.md/util.o sun3.md/version.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
