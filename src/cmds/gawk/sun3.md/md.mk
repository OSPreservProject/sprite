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
# Thu Mar 15 14:51:37 PST 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= array.c awk.y builtin.c debug.c eval.c field.c io.c main.c missing.c msg.c node.c regex.c version.c
HDRS		= awk.h patchlevel.h regex.h
MDPUBHDRS	= 
OBJS		= sun3.md/array.o sun3.md/awk.o sun3.md/builtin.o sun3.md/debug.o sun3.md/eval.o sun3.md/field.o sun3.md/io.o sun3.md/main.o sun3.md/missing.o sun3.md/msg.o sun3.md/node.o sun3.md/regex.o sun3.md/version.o
CLEANOBJS	= sun3.md/array.o sun3.md/awk.o sun3.md/builtin.o sun3.md/debug.o sun3.md/eval.o sun3.md/field.o sun3.md/io.o sun3.md/main.o sun3.md/missing.o sun3.md/msg.o sun3.md/node.o sun3.md/regex.o sun3.md/version.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
