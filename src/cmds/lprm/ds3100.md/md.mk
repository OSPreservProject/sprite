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
# Wed Jun 10 17:29:01 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= common.c lprm.c printcap.c rmjob.c startdaemon.c
HDRS		= lp.h lp.local.h
MDPUBHDRS	= 
OBJS		= ds3100.md/common.o ds3100.md/lprm.o ds3100.md/printcap.o ds3100.md/rmjob.o ds3100.md/startdaemon.o
CLEANOBJS	= ds3100.md/common.o ds3100.md/lprm.o ds3100.md/printcap.o ds3100.md/rmjob.o ds3100.md/startdaemon.o
INSTFILES	= ds3100.md/md.mk ds3100.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
