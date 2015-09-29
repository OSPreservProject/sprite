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
# Tue Dec 15 22:14:32 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= spur.md/printMachDepStats.c client.c pdevtest.c server.c printStats.c
HDRS		= pdevInt.h version.h
MDPUBHDRS	= 
OBJS		= spur.md/client.o spur.md/server.o spur.md/printMachDepStats.o spur.md/pdevtest.o spur.md/printStats.o
CLEANOBJS	= spur.md/printMachDepStats.o spur.md/client.o spur.md/pdevtest.o spur.md/server.o spur.md/printStats.o
INSTFILES	= spur.md/md.mk spur.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
