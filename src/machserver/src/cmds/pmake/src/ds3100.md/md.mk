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
# Mon May 11 14:10:00 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= arch.c compat.c cond.c dir.c job.c main.c make.c buf.c parse.c str.c suff.c targ.c var.c rmt.c
HDRS		= buf.h config.h job.h lst.h make.h nonints.h
MDPUBHDRS	= 
OBJS		= ds3100.md/arch.o ds3100.md/buf.o ds3100.md/compat.o ds3100.md/cond.o ds3100.md/dir.o ds3100.md/job.o ds3100.md/main.o ds3100.md/make.o ds3100.md/parse.o ds3100.md/rmt.o ds3100.md/str.o ds3100.md/suff.o ds3100.md/targ.o ds3100.md/var.o
CLEANOBJS	= ds3100.md/arch.o ds3100.md/compat.o ds3100.md/cond.o ds3100.md/dir.o ds3100.md/job.o ds3100.md/main.o ds3100.md/make.o ds3100.md/buf.o ds3100.md/parse.o ds3100.md/str.o ds3100.md/suff.o ds3100.md/targ.o ds3100.md/var.o ds3100.md/rmt.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	=
