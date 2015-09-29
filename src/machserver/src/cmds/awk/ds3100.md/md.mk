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
# Mon Apr 13 20:59:20 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= awk.g.c awk.g.y awk.lx.l b.c freeze.c lib.c main.c parse.c proc.c proctab.c run.c token.c tran.c
HDRS		= awk.h
MDPUBHDRS	= 
OBJS		= ds3100.md/awk.g.o ds3100.md/awk.g.o ds3100.md/awk.lx.o ds3100.md/b.o ds3100.md/freeze.o ds3100.md/lib.o ds3100.md/main.o ds3100.md/parse.o ds3100.md/proc.o ds3100.md/proctab.o ds3100.md/run.o ds3100.md/token.o ds3100.md/tran.o
CLEANOBJS	= ds3100.md/awk.g.o ds3100.md/awk.g.o ds3100.md/awk.lx.o ds3100.md/b.o ds3100.md/freeze.o ds3100.md/lib.o ds3100.md/main.o ds3100.md/parse.o ds3100.md/proc.o ds3100.md/proctab.o ds3100.md/run.o ds3100.md/token.o ds3100.md/tran.o
INSTFILES	= Makefile local.mk
SACREDOBJS	= 
