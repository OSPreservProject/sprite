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
# Wed Jun 10 17:54:12 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= arch.c compat.c cond.c dir.c job.c main.c make.c buf.c parse.c str.c suff.c targ.c var.c rmt.c
HDRS		= buf.h config.h job.h lst.h make.h nonints.h
MDPUBHDRS	= 
OBJS		= sun3.md/arch.o sun3.md/buf.o sun3.md/compat.o sun3.md/cond.o sun3.md/dir.o sun3.md/job.o sun3.md/main.o sun3.md/make.o sun3.md/parse.o sun3.md/rmt.o sun3.md/str.o sun3.md/suff.o sun3.md/targ.o sun3.md/var.o
CLEANOBJS	= sun3.md/arch.o sun3.md/compat.o sun3.md/cond.o sun3.md/dir.o sun3.md/job.o sun3.md/main.o sun3.md/make.o sun3.md/buf.o sun3.md/parse.o sun3.md/str.o sun3.md/suff.o sun3.md/targ.o sun3.md/var.o sun3.md/rmt.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile TAGS
SACREDOBJS	=
