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
# Mon Jun  8 14:26:13 PDT 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= _ExecArgs.c execl.c execle.c execlp.c execv.c execvp.c
HDRS		= 
MDPUBHDRS	= 
OBJS		= sun4.md/_ExecArgs.o sun4.md/execl.o sun4.md/execle.o sun4.md/execlp.o sun4.md/execv.o sun4.md/execvp.o
CLEANOBJS	= sun4.md/_ExecArgs.o sun4.md/execl.o sun4.md/execle.o sun4.md/execlp.o sun4.md/execv.o sun4.md/execvp.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile
SACREDOBJS	= 
