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
# Tue May 21 15:12:36 PDT 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= common.c lpdchar.c wait.c printcap.c startdaemon.c displayq.c lpd.c printjob.c recvjob.c rmjob.c
HDRS		= compatInt.h lp.h lp.local.h
MDPUBHDRS	= 
OBJS		= sun4.md/common.o sun4.md/displayq.o sun4.md/lpd.o sun4.md/lpdchar.o sun4.md/printcap.o sun4.md/printjob.o sun4.md/recvjob.o sun4.md/rmjob.o sun4.md/startdaemon.o sun4.md/wait.o
CLEANOBJS	= sun4.md/common.o sun4.md/lpdchar.o sun4.md/wait.o sun4.md/printcap.o sun4.md/startdaemon.o sun4.md/displayq.o sun4.md/lpd.o sun4.md/printjob.o sun4.md/recvjob.o sun4.md/rmjob.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
