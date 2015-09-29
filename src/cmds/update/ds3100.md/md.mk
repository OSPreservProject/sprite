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
# Sun Mar  8 22:37:08 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= option.c regexp.c strtoul.c update.c
HDRS		= cfuncproto.h option.h regexp.h
MDPUBHDRS	= 
OBJS		= ds3100.md/option.o ds3100.md/regexp.o ds3100.md/strtoul.o ds3100.md/update.o
CLEANOBJS	= ds3100.md/option.o ds3100.md/regexp.o ds3100.md/strtoul.o ds3100.md/update.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
