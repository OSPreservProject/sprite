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
# Thu Aug 23 15:53:34 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sigMigrate.c signals.c
HDRS		= sig.h sigInt.h
MDPUBHDRS	= 
OBJS		= spur.md/sigMigrate.o spur.md/signals.o
CLEANOBJS	= spur.md/sigMigrate.o spur.md/signals.o
INSTFILES	= spur.md/md.mk spur.md/dependencies.mk Makefile tags TAGS
SACREDOBJS	= 
