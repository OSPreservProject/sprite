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
# Thu Aug  2 13:52:04 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= sunBW2.c sunCG3C.c sunCG4C.c sunCG6C.c sunCursor.c sunInit.c sunIo.c sunKbd.c sunKeyMap.c sunMouse.c sunUtils.c
HDRS		= keymap.h sun.h
MDPUBHDRS	= 
OBJS		= sun3.md/sunBW2.o sun3.md/sunCG3C.o sun3.md/sunCG4C.o sun3.md/sunCG6C.o sun3.md/sunCursor.o sun3.md/sunInit.o sun3.md/sunIo.o sun3.md/sunKbd.o sun3.md/sunKeyMap.o sun3.md/sunMouse.o sun3.md/sunUtils.o
CLEANOBJS	= sun3.md/sunBW2.o sun3.md/sunCG3C.o sun3.md/sunCG4C.o sun3.md/sunCG6C.o sun3.md/sunCursor.o sun3.md/sunInit.o sun3.md/sunIo.o sun3.md/sunKbd.o sun3.md/sunKeyMap.o sun3.md/sunMouse.o sun3.md/sunUtils.o
INSTFILES	= sun3.md/md.mk Makefile local.mk
SACREDOBJS	= 
