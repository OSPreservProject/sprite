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
# Wed Jun 10 17:54:17 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= arch.c compat.c cond.c dir.c job.c main.c make.c buf.c parse.c str.c suff.c targ.c var.c rmt.c
HDRS		= buf.h config.h job.h lst.h make.h nonints.h
MDPUBHDRS	= 
OBJS		= sun4.md/arch.o sun4.md/buf.o sun4.md/compat.o sun4.md/cond.o sun4.md/dir.o sun4.md/job.o sun4.md/main.o sun4.md/make.o sun4.md/parse.o sun4.md/rmt.o sun4.md/str.o sun4.md/suff.o sun4.md/targ.o sun4.md/var.o
CLEANOBJS	= sun4.md/arch.o sun4.md/compat.o sun4.md/cond.o sun4.md/dir.o sun4.md/job.o sun4.md/main.o sun4.md/make.o sun4.md/buf.o sun4.md/parse.o sun4.md/str.o sun4.md/suff.o sun4.md/targ.o sun4.md/var.o sun4.md/rmt.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile TAGS
SACREDOBJS	=
