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
# Thu Dec 17 17:02:30 PST 1992
#
# For more information, refer to the mkmf manual page.
#
# $Header: /sprite/lib/mkmf/RCS/Makefile.md,v 1.6 90/03/12 23:28:42 jhh Exp $
#
# Allow mkmf

SRCS		= dopage.c dosection.c dospecial.c download.c dpicheck.c drawPS.c dviinput.c dvips.c finclude.c flib.c fontdef.c header.c loadfont.c makefont.c output.c prescan.c repack.c resident.c scalewidth.c scanpage.c search.c skippage.c tfmload.c unpack.c virtualfont.c squeeze.c afm2tfm.c
HDRS		= debug.h paths.h structures.h
MDPUBHDRS	= 
OBJS		= sun4.md/dopage.o sun4.md/dosection.o sun4.md/dospecial.o sun4.md/download.o sun4.md/dpicheck.o sun4.md/drawPS.o sun4.md/dviinput.o sun4.md/dvips.o sun4.md/finclude.o sun4.md/flib.o sun4.md/fontdef.o sun4.md/header.o sun4.md/loadfont.o sun4.md/makefont.o sun4.md/output.o sun4.md/prescan.o sun4.md/repack.o sun4.md/resident.o sun4.md/scalewidth.o sun4.md/scanpage.o sun4.md/search.o sun4.md/skippage.o sun4.md/tfmload.o sun4.md/unpack.o sun4.md/virtualfont.o sun4.md/squeeze.o sun4.md/afm2tfm.o
CLEANOBJS	= sun4.md/dopage.o sun4.md/dosection.o sun4.md/dospecial.o sun4.md/download.o sun4.md/dpicheck.o sun4.md/drawPS.o sun4.md/dviinput.o sun4.md/dvips.o sun4.md/finclude.o sun4.md/flib.o sun4.md/fontdef.o sun4.md/header.o sun4.md/loadfont.o sun4.md/makefont.o sun4.md/output.o sun4.md/prescan.o sun4.md/repack.o sun4.md/resident.o sun4.md/scalewidth.o sun4.md/scanpage.o sun4.md/search.o sun4.md/skippage.o sun4.md/tfmload.o sun4.md/unpack.o sun4.md/virtualfont.o sun4.md/squeeze.o sun4.md/afm2tfm.o
INSTFILES	= sun4.md/md.mk sun4.md/dependencies.mk Makefile local.mk
SACREDOBJS	= 
