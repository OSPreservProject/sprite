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
# Sun Mar 15 18:19:29 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sprite.c buffer.c create.c diffarch.c extract.c getoldopt.c getopt.c getopt1.c list.c names.c port.c rtape_lib.c tar.c update.c version.c wildmat.c
HDRS		= compatInt.h getopt.h open3.h port.h rmt.h tar.h
MDPUBHDRS	= 
OBJS		= sun3.md/buffer.o sun3.md/create.o sun3.md/diffarch.o sun3.md/extract.o sun3.md/getoldopt.o sun3.md/getopt.o sun3.md/getopt1.o sun3.md/list.o sun3.md/names.o sun3.md/port.o sun3.md/rtape_lib.o sun3.md/sprite.o sun3.md/tar.o sun3.md/update.o sun3.md/wildmat.o sun3.md/version.o
CLEANOBJS	= sun3.md/sprite.o sun3.md/buffer.o sun3.md/create.o sun3.md/diffarch.o sun3.md/extract.o sun3.md/getoldopt.o sun3.md/getopt.o sun3.md/getopt1.o sun3.md/list.o sun3.md/names.o sun3.md/port.o sun3.md/rtape_lib.o sun3.md/tar.o sun3.md/update.o sun3.md/version.o sun3.md/wildmat.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags TAGS
SACREDOBJS	= 
