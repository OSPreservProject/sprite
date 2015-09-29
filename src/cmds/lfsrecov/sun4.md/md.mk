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
# Mon Dec 14 17:42:22 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= desc.c dirlog.c files.c lfsrecov.c usage.c
HDRS		= desc.h dirlog.h fileop.h lfsrecov.h usage.h
MDPUBHDRS	= 
OBJS		= sun4.md/desc.o sun4.md/dirlog.o sun4.md/files.o sun4.md/lfsrecov.o sun4.md/usage.o
CLEANOBJS	= sun4.md/desc.o sun4.md/dirlog.o sun4.md/files.o sun4.md/lfsrecov.o sun4.md/usage.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
