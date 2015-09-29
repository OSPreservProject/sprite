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
# Mon Dec 14 15:41:07 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= ds3100.md/ccMachStat.c ds3100.md/printStats.c client.c server.c proc.c bench.c
HDRS		= ds3100.md/ccMachStat.h
MDPUBHDRS	= 
OBJS		= ds3100.md/ccMachStat.o ds3100.md/printStats.o ds3100.md/client.o ds3100.md/server.o ds3100.md/proc.o ds3100.md/bench.o
CLEANOBJS	= ds3100.md/ccMachStat.o ds3100.md/printStats.o ds3100.md/client.o ds3100.md/server.o ds3100.md/proc.o ds3100.md/bench.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
