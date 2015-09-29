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
# Sun Mar 15 18:19:36 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sprite.c buffer.c create.c diffarch.c extract.c getoldopt.c getopt.c getopt1.c list.c names.c port.c rtape_lib.c tar.c update.c version.c wildmat.c
HDRS		= compatInt.h getopt.h open3.h port.h rmt.h tar.h
MDPUBHDRS	= 
OBJS		= sun4.md/buffer.o sun4.md/create.o sun4.md/diffarch.o sun4.md/extract.o sun4.md/getoldopt.o sun4.md/getopt.o sun4.md/getopt1.o sun4.md/list.o sun4.md/names.o sun4.md/port.o sun4.md/rtape_lib.o sun4.md/sprite.o sun4.md/tar.o sun4.md/update.o sun4.md/wildmat.o sun4.md/version.o
CLEANOBJS	= sun4.md/sprite.o sun4.md/buffer.o sun4.md/create.o sun4.md/diffarch.o sun4.md/extract.o sun4.md/getoldopt.o sun4.md/getopt.o sun4.md/getopt1.o sun4.md/list.o sun4.md/names.o sun4.md/port.o sun4.md/rtape_lib.o sun4.md/tar.o sun4.md/update.o sun4.md/version.o sun4.md/wildmat.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
