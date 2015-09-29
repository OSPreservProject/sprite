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
# Tue Dec 15 22:14:36 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= client.c pdevtest.c server.c printStats.c
HDRS		= pdevInt.h version.h
MDPUBHDRS	= 
OBJS		= sun2.md/client.o sun2.md/pdevtest.o sun2.md/server.o sun2.md/printStats.o
CLEANOBJS	= sun2.md/client.o sun2.md/pdevtest.o sun2.md/server.o sun2.md/printStats.o
INSTFILES	= sun2.md/md.mk sun2.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
