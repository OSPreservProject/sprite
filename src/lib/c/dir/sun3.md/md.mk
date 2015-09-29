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
# Mon Jun  8 14:24:08 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= closedir.c opendir.c readdir.c seekdir.c telldir.c scandir.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/closedir.o sun3.md/opendir.o sun3.md/readdir.o sun3.md/seekdir.o sun3.md/telldir.o sun3.md/scandir.o
CLEANOBJS	= sun3.md/closedir.o sun3.md/opendir.o sun3.md/readdir.o sun3.md/seekdir.o sun3.md/telldir.o sun3.md/scandir.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
