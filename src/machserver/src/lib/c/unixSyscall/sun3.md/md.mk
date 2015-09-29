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
# Sun Dec  8 18:10:04 PST 1991
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= compatMapCode.c errno.c lseek.c read.c write.c wait.c
HDRS		= compatInt.h
MDPUBHDRS	= 
OBJS		= sun3.md/compatMapCode.o sun3.md/errno.o sun3.md/lseek.o sun3.md/read.o sun3.md/write.o sun3.md/wait.o
CLEANOBJS	= sun3.md/compatMapCode.o sun3.md/errno.o sun3.md/lseek.o sun3.md/read.o sun3.md/write.o sun3.md/wait.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
