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
# Mon Dec 14 17:38:31 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Host_ByID.c Host_ByName.c Host_End.c disklabel.c Host_Next.c Host_Start.c diskHeader.c diskIO.c diskPrint.c option.c fsmake.c
HDRS		= fsmake.h hostInt.h
MDPUBHDRS	= 
OBJS		= ds3100.md/Host_ByID.o ds3100.md/Host_ByName.o ds3100.md/Host_End.o ds3100.md/Host_Next.o ds3100.md/Host_Start.o ds3100.md/diskHeader.o ds3100.md/diskIO.o ds3100.md/diskPrint.o ds3100.md/disklabel.o ds3100.md/fsmake.o ds3100.md/option.o
CLEANOBJS	= ds3100.md/Host_ByID.o ds3100.md/Host_ByName.o ds3100.md/Host_End.o ds3100.md/disklabel.o ds3100.md/Host_Next.o ds3100.md/Host_Start.o ds3100.md/diskHeader.o ds3100.md/diskIO.o ds3100.md/diskPrint.o ds3100.md/option.o ds3100.md/fsmake.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile tags
SACREDOBJS	= 
