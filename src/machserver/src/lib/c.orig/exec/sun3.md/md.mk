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
# Tue Oct 24 00:33:30 PDT 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= _ExecArgs.c execl.c execle.c execlp.c execv.c execvp.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun3.md/_ExecArgs.o sun3.md/execl.o sun3.md/execle.o sun3.md/execlp.o sun3.md/execv.o sun3.md/execvp.o
CLEANOBJS	= sun3.md/_ExecArgs.o sun3.md/execl.o sun3.md/execle.o sun3.md/execlp.o sun3.md/execv.o sun3.md/execvp.o
DISTDIR        ?= @(DISTDIR)
