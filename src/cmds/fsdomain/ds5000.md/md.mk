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
# Fri Oct 11 14:32:24 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fsdomain.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds5000.md/fsdomain.o
CLEANOBJS	= ds5000.md/fsdomain.o
INSTFILES	= ds5000.md/md.mk ds5000.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
