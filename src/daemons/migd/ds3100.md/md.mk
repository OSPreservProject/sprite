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
# Tue Jun 16 20:34:22 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= main.c migd.c util.c migPdev.c Mig_GetPdevName.c global.c
HDRS		= global.h migPdev.h migd.h version.h
MDPUBHDRS	= 
OBJS		= ds3100.md/Mig_GetPdevName.o ds3100.md/global.o ds3100.md/main.o ds3100.md/migPdev.o ds3100.md/migd.o ds3100.md/util.o
CLEANOBJS	= ds3100.md/main.o ds3100.md/migd.o ds3100.md/util.o ds3100.md/migPdev.o ds3100.md/Mig_GetPdevName.o ds3100.md/global.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk TAGS
SACREDOBJS	= 
