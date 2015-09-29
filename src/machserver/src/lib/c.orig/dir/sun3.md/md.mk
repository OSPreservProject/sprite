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
# Tue Oct 24 00:30:45 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= closedir.c opendir.c readdir.c scandir.c seekdir.c telldir.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/closedir.o sun3.md/opendir.o sun3.md/readdir.o sun3.md/scandir.o sun3.md/seekdir.o sun3.md/telldir.o
CLEANOBJS	= sun3.md/closedir.o sun3.md/opendir.o sun3.md/readdir.o sun3.md/scandir.o sun3.md/seekdir.o sun3.md/telldir.o
DISTDIR        ?= @(DISTDIR)
