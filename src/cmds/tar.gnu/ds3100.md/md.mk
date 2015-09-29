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
# Sun Mar 15 18:19:24 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sprite.c buffer.c create.c diffarch.c extract.c getoldopt.c getopt.c getopt1.c list.c names.c port.c rtape_lib.c tar.c update.c version.c wildmat.c
HDRS		= compatInt.h getopt.h open3.h port.h rmt.h tar.h
MDPUBHDRS	= 
OBJS		= ds3100.md/getoldopt.o ds3100.md/getopt.o ds3100.md/getopt1.o ds3100.md/list.o ds3100.md/names.o ds3100.md/port.o ds3100.md/rtape_lib.o ds3100.md/update.o ds3100.md/wildmat.o ds3100.md/sprite.o ds3100.md/buffer.o ds3100.md/create.o ds3100.md/diffarch.o ds3100.md/extract.o ds3100.md/tar.o ds3100.md/version.o
CLEANOBJS	= ds3100.md/sprite.o ds3100.md/buffer.o ds3100.md/create.o ds3100.md/diffarch.o ds3100.md/extract.o ds3100.md/getoldopt.o ds3100.md/getopt.o ds3100.md/getopt1.o ds3100.md/list.o ds3100.md/names.o ds3100.md/port.o ds3100.md/rtape_lib.o ds3100.md/tar.o ds3100.md/update.o ds3100.md/version.o ds3100.md/wildmat.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
