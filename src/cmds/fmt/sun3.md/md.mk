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
# Thu Dec 17 17:06:57 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= fmt.c head.c
HDRS		= def.h glob.h local.h rcv.h v7.local.h
MDPUBHDRS	= 
OBJS		= sun3.md/fmt.o sun3.md/head.o
CLEANOBJS	= sun3.md/fmt.o sun3.md/head.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile
SACREDOBJS	= 
