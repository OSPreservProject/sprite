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
# Sun Nov  5 23:36:06 PST 1989
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.4 89/10/09 21:28:30 rab Exp $
#
# Allow mkmf

SRCS		= spriteBW2.c spriteCG4.c spriteCG6.c spriteCursor.c spriteGC.c spriteInit.c spriteIo.c spriteKbd.c spriteMouse.c
HDRS		= spriteddx.h
MDPUBHDRS	= 
OBJS		= sun3.md/spriteBW2.o sun3.md/spriteCG2M.o sun3.md/spriteCG6.o sun3.md/spriteCursor.o sun3.md/spriteGC.o sun3.md/spriteInit.o sun3.md/spriteIo.o sun3.md/spriteKbd.o sun3.md/spriteMouse.o sun3.md/spriteCG4.o
CLEANOBJS	= sun3.md/spriteBW2.o sun3.md/spriteCG4.o sun3.md/spriteCG6.o sun3.md/spriteCursor.o sun3.md/spriteGC.o sun3.md/spriteInit.o sun3.md/spriteIo.o sun3.md/spriteKbd.o sun3.md/spriteMouse.o
DISTDIR        ?= @(DISTDIR)
