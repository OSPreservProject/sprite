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
# Thu Dec 17 16:57:21 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Scandir.c bcopy.c ded.c getname.c subr.c
HDRS		= ded.h
MDPUBHDRS	= 
OBJS		= sun3.md/Scandir.o sun3.md/bcopy.o sun3.md/ded.o sun3.md/getname.o sun3.md/subr.o
CLEANOBJS	= sun3.md/Scandir.o sun3.md/bcopy.o sun3.md/ded.o sun3.md/getname.o sun3.md/subr.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
