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
# Wed Jul 11 12:53:54 PDT 1990
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= constype.c kbd_mode.c sunBW2.c sunCG2C.c sunCG2M.c sunCG3C.c sunCG4C.c sunCG6C.c sunCursor.c sunInit.c sunIo.c sunKbd.c sunKeyMap.c sunMouse.c sunUtils.c
HDRS		= sun.h
MDPUBHDRS	= 
OBJS		= sun4.md/constype.o sun4.md/kbd_mode.o sun4.md/sunBW2.o sun4.md/sunCG2C.o sun4.md/sunCG2M.o sun4.md/sunCG3C.o sun4.md/sunCG4C.o sun4.md/sunCG6C.o sun4.md/sunCursor.o sun4.md/sunInit.o sun4.md/sunIo.o sun4.md/sunKbd.o sun4.md/sunKeyMap.o sun4.md/sunMouse.o sun4.md/sunUtils.o
CLEANOBJS	= sun4.md/constype.o sun4.md/kbd_mode.o sun4.md/sunBW2.o sun4.md/sunCG2C.o sun4.md/sunCG2M.o sun4.md/sunCG3C.o sun4.md/sunCG4C.o sun4.md/sunCG6C.o sun4.md/sunCursor.o sun4.md/sunInit.o sun4.md/sunIo.o sun4.md/sunKbd.o sun4.md/sunKeyMap.o sun4.md/sunMouse.o sun4.md/sunUtils.o
INSTFILES	= Makefile
SACREDOBJS	= 
