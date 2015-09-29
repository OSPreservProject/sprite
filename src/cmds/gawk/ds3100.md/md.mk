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
# Thu Mar 15 17:33:05 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= array.c awk.y builtin.c debug.c eval.c field.c io.c main.c missing.c msg.c node.c regex.c version.c
HDRS		= awk.h patchlevel.h regex.h
MDPUBHDRS	= 
OBJS		= ds3100.md/array.o ds3100.md/awk.o ds3100.md/builtin.o ds3100.md/debug.o ds3100.md/eval.o ds3100.md/field.o ds3100.md/io.o ds3100.md/main.o ds3100.md/missing.o ds3100.md/msg.o ds3100.md/node.o ds3100.md/regex.o ds3100.md/version.o
CLEANOBJS	= ds3100.md/array.o ds3100.md/awk.o ds3100.md/builtin.o ds3100.md/debug.o ds3100.md/eval.o ds3100.md/field.o ds3100.md/io.o ds3100.md/main.o ds3100.md/missing.o ds3100.md/msg.o ds3100.md/node.o ds3100.md/regex.o ds3100.md/version.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
