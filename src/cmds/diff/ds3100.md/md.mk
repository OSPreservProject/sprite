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
# Thu Dec 17 16:58:47 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= analyze.c context.c diff.c dir.c ed.c getopt.c getopt1.c ifdef.c io.c normal.c regex.c util.c version.c
HDRS		= diff.h getopt.h limits.h regex.h
MDPUBHDRS	= 
OBJS		= ds3100.md/analyze.o ds3100.md/context.o ds3100.md/diff.o ds3100.md/dir.o ds3100.md/ed.o ds3100.md/getopt.o ds3100.md/getopt1.o ds3100.md/ifdef.o ds3100.md/io.o ds3100.md/normal.o ds3100.md/regex.o ds3100.md/util.o ds3100.md/version.o
CLEANOBJS	= ds3100.md/analyze.o ds3100.md/context.o ds3100.md/diff.o ds3100.md/dir.o ds3100.md/ed.o ds3100.md/getopt.o ds3100.md/getopt1.o ds3100.md/ifdef.o ds3100.md/io.o ds3100.md/normal.o ds3100.md/regex.o ds3100.md/util.o ds3100.md/version.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile
SACREDOBJS	= 
