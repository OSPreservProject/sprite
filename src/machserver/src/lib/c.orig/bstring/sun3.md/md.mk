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
# Tue Oct 24 00:27:52 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= bcmp.c bcopy.c bzero.c ffs.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/bcmp.o sun3.md/bcopy.o sun3.md/bzero.o sun3.md/ffs.o
CLEANOBJS	= sun3.md/bcmp.o sun3.md/bcopy.o sun3.md/bzero.o sun3.md/ffs.o
DISTDIR        ?= @(DISTDIR)
