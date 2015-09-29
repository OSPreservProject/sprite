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
# Wed Jun 10 17:46:52 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= commands.l debug.c getinfo.c main.c list.c send.c skip.c subr.c
HDRS		= res.h
MDPUBHDRS	= 
OBJS		= sun4.md/commands.o sun4.md/debug.o sun4.md/getinfo.o sun4.md/list.o sun4.md/main.o sun4.md/send.o sun4.md/skip.o sun4.md/subr.o
CLEANOBJS	= sun4.md/commands.o sun4.md/debug.o sun4.md/getinfo.o sun4.md/main.o sun4.md/list.o sun4.md/send.o sun4.md/skip.o sun4.md/subr.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk tags
SACREDOBJS	= 
