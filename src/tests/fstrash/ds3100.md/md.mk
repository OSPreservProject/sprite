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
# Tue Dec 15 22:11:18 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fstrash.c fsSetFD.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= ds3100.md/fstrash.o ds3100.md/fsSetFD.o
CLEANOBJS	= ds3100.md/fstrash.o ds3100.md/fsSetFD.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile Makefile.sed tags
SACREDOBJS	= 
