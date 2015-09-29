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
# Mon Dec 14 17:38:35 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= Host_ByID.c Host_ByName.c Host_End.c disklabel.c Host_Next.c Host_Start.c diskHeader.c diskIO.c diskPrint.c option.c fsmake.c
HDRS		= fsmake.h hostInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/Host_ByID.o sun3.md/Host_ByName.o sun3.md/Host_End.o sun3.md/Host_Next.o sun3.md/Host_Start.o sun3.md/diskHeader.o sun3.md/diskIO.o sun3.md/disklabel.o sun3.md/fsmake.o sun3.md/option.o sun3.md/diskPrint.o
CLEANOBJS	= sun3.md/Host_ByID.o sun3.md/Host_ByName.o sun3.md/Host_End.o sun3.md/disklabel.o sun3.md/Host_Next.o sun3.md/Host_Start.o sun3.md/diskHeader.o sun3.md/diskIO.o sun3.md/diskPrint.o sun3.md/option.o sun3.md/fsmake.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile tags
SACREDOBJS	= 
