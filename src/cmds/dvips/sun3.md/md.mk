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
# Thu Dec 17 17:01:37 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= dopage.c dosection.c dospecial.c download.c drawPS.c dviinput.c dvips.c fontdef.c header.c loadfont.c makefont.c output.c prescan.c repack.c resident.c scalewidth.c scanpage.c search.c skippage.c tfmload.c unpack.c virtualfont.c
HDRS		= debug.h paths.h structures.h
MDPUBHDRS	= 
OBJS		= sun3.md/dopage.o sun3.md/dosection.o sun3.md/dospecial.o sun3.md/download.o sun3.md/drawPS.o sun3.md/dviinput.o sun3.md/dvips.o sun3.md/fontdef.o sun3.md/header.o sun3.md/loadfont.o sun3.md/makefont.o sun3.md/output.o sun3.md/prescan.o sun3.md/repack.o sun3.md/resident.o sun3.md/scalewidth.o sun3.md/scanpage.o sun3.md/search.o sun3.md/skippage.o sun3.md/tfmload.o sun3.md/unpack.o sun3.md/virtualfont.o
CLEANOBJS	= sun3.md/dopage.o sun3.md/dosection.o sun3.md/dospecial.o sun3.md/download.o sun3.md/drawPS.o sun3.md/dviinput.o sun3.md/dvips.o sun3.md/fontdef.o sun3.md/header.o sun3.md/loadfont.o sun3.md/makefont.o sun3.md/output.o sun3.md/prescan.o sun3.md/repack.o sun3.md/resident.o sun3.md/scalewidth.o sun3.md/scanpage.o sun3.md/search.o sun3.md/skippage.o sun3.md/tfmload.o sun3.md/unpack.o sun3.md/virtualfont.o
INSTFILES	= sun3.md/md.mk sun3.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
