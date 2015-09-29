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
# Mon Dec 14 15:41:13 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sun3.md/ccMachStat.c sun3.md/printStats.c client.c server.c proc.c bench.c
HDRS		= sun3.md/ccMachStat.h
MDPUBHDRS	= 
OBJS		= sun3.md/ccMachStat.o sun3.md/printStats.o sun3.md/client.o sun3.md/server.o sun3.md/proc.o sun3.md/bench.o
CLEANOBJS	= sun3.md/ccMachStat.o sun3.md/printStats.o sun3.md/client.o sun3.md/server.o sun3.md/proc.o sun3.md/bench.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
